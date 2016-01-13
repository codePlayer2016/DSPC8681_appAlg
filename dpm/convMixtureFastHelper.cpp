#include "DPM.h"
#include <math.h>
#include <limits.h>
#include <algorithm>
#include <float.h>
#include <stdlib.h>
#include <fstream>

extern std::ofstream foutDebug;

int myFFT2 (CvMat *xin, CvMat *yout, int num, int inPlace);
int myIFFT2 (CvMat *xin, CvMat *yout, int num);

// Order rectangles by decreasing area.
class AreaComparator
{
public:
	AreaComparator(const zftdt::DPMVector<std::pair<CvRect, int> > & rectangles) 
		:rectangles_(rectangles)
	{
	}
	/// Returns whether rectangle @p a comes before @p b.
	bool operator()(int a, int b) const
	{
		const int areaA = rectangles_[a].first.width * rectangles_[a].first.height;//rectangles_[a].first.area();
		const int areaB = rectangles_[b].first.width * rectangles_[b].first.height;//rectangles_[b].first.area();
		
		return (areaA > areaB) || ((areaA == areaB) && (rectangles_[a].first.height >
														rectangles_[b].first.height));
	}
	
private:
	const zftdt::DPMVector<std::pair<CvRect, int> > & rectangles_;
};

// Order free gaps (rectangles) by position and then by size
struct PositionComparator
{
	// Returns whether rectangle @p a comes before @p b
	bool operator()(const CvRect & a, const CvRect & b) const
	{
		return (a.y < b.y) ||
			   ((a.y == b.y) &&
				((a.x < b.x) ||
				 ((a.x == b.x) &&
				  ((a.height > b.height) ||
				   ((a.height == b.height) && (a.width > b.width))))));
	}
};


static int getFullDetectionValidLevels(const zftdt::HOGPyramid& pyramid, 
    const zftdt::Mixture& mixture, zftdt::DPMVector<unsigned char>& mask)
{
    int numLevels = pyramid.levels.size;
    int interval = pyramid.interval;
    int padx = pyramid.padx, pady = pyramid.pady;
    int minValidLevel = -1;
#if OPT_LEVEL_MAXFLTSIZE
    CvSize minSize = mixture.maxSizeOfFilters;
#endif
    minSize.width += pyramid.padx * 2;
    minSize.height += pyramid.pady * 2;
    mask.size = numLevels;
	memset(mask.ptr, 0, mask.size * sizeof(mask[0]));


	    for (int i = interval; i < numLevels; i++)
    {
        if (pyramid.levels[i - interval] && pyramid.levels[i] &&
            pyramid.levels[i]->height >= minSize.height &&
            pyramid.levels[i]->width / NbFeatures >= minSize.width)
        {
            mask[i - interval] = 1;
            mask[i] = 1;
            if (minValidLevel == -1)
                minValidLevel = i - interval;
        }
    }
    return minValidLevel;
}

static bool equals(const zftdt::DPMVector<unsigned char>& lhs, const zftdt::DPMVector<unsigned char>& rhs)
{
    if (lhs.size != rhs.size)
        return false;
    int size = lhs.size;
    for (int i = 0; i < size; i++)
    {
        if (lhs[i] != rhs[i])
            return false;
    }
    return true;
}

// Global mutex for all static helper arrays and the non-thread-safe parts of fftw
//static cv::Mutex helperMutex;


static void getPyramidLevelSizes(const zftdt::HOGPyramid& pyramid, zftdt::DPMVector<CvSize>& sizes)
{

	const zftdt::DPMVector<CvMat*>& levels = pyramid.levels;
    int numLevels = pyramid.levels.size;
    sizes.size = numLevels;
    for (int i = 0; i < numLevels; i++)
        sizes[i] = !levels[i] ? cvSize(levels[i]->width, levels[i]->height): cvSize(0, 0);
}
static bool equals(const zftdt::DPMVector<CvSize>& lhs, const zftdt::DPMVector<CvSize>& rhs)
{
    if (lhs.size != rhs.size) return false;
    int length = lhs.size;
    for (int i = 0; i < length; i++)
    {
        //if (lhs[i] != rhs[i])
		if (lhs[i].width != rhs[i].width || lhs[i].height != rhs[i].height)
            return false;
    }
    return true;
}


static int blf(zftdt::DPMVector<std::pair<CvRect, int> > & rectangles, int maxWidth, int maxHeight)
{
	// Order the rectangles by decreasing area. If a rectangle is bigger than MaxRows x MaxCols
	// returns -1
	static zftdt::DPMVector<int> ordering(DPM_PYRAMID_MAX_LEVELS + 1);
	ordering.size = rectangles.size;
	for (int i = 0; i < rectangles.size; ++i) {
		if ((rectangles[i].first.width > maxWidth) || (rectangles[i].first.height > maxHeight))
			return -1;
		
		ordering[i] = i;
	}
	
	//sort(ordering.begin(), ordering.end(), AreaComparator(rectangles));
	std::sort(&ordering[0], &ordering[rectangles.size - 1], AreaComparator(rectangles));
	
	// Index of the plane containing each rectangle
	for (int i = 0; i < rectangles.size; ++i)
		rectangles[i].second = -1;
	
	static zftdt::DPMVector<std::set<CvRect, PositionComparator> > gaps(DPM_PYRAMID_MAX_LEVELS + 1);
	gaps.size = 0;
	
	// Insert each rectangle in the first gap big enough
	for (int i = 0; i < rectangles.size; ++i) {
		std::pair<CvRect, int> & rect = rectangles[ordering[i]];
		
		// Find the first gap big enough
		std::set<CvRect, PositionComparator>::iterator g;
		
		for (int i = 0; (rect.second == -1) && (i < gaps.size); ++i) {
			for (g = gaps[i].begin(); g != gaps[i].end(); ++g) {
				if ((g->width >= rect.first.width) && (g->height >= rect.first.height)) {
					rect.second = i;
					break;
				}
			}
		}
		
		// If no gap big enough was found, add a new plane
		if (rect.second == -1) {
			std::set<CvRect, PositionComparator> plane;
			plane.insert(cvRect(0, 0, maxWidth, maxHeight)); // The whole plane is free
			//
			//gaps.size++;
			//assert(gaps.size <= gaps.maxNum);
			//gaps[gaps.size - 1] = plane;
			gaps.push_back(plane);
			g = gaps[gaps.size - 1].begin();
			rect.second = static_cast<int>(gaps.size) - 1;
		}
		
		// Insert the rectangle in the gap
		rect.first.x = g->x;
		rect.first.y = g->y;
		
		// Remove all the intersecting gaps, and add newly created gaps
		for (g = gaps[rect.second].begin(); g != gaps[rect.second].end();) {
            if (!(((rect.first.x + rect.first.width - 1) < g->x) || ((rect.first.y + rect.first.height - 1) < g->y) ||
                   (rect.first.x > (g->x + g->width - 1)) || (rect.first.y > (g->y + g->height - 1)))) {
				// Add a gap to the left of the new rectangle if possible
				if (g->x < rect.first.x)
					gaps[rect.second].insert(cvRect(g->x, g->y, rect.first.x - g->x,
													  g->height));
				
				// Add a gap on top of the new rectangle if possible
				if (g->y < rect.first.y)
					gaps[rect.second].insert(cvRect(g->x, g->y, g->width,
													  rect.first.y - g->y));
				
				// Add a gap to the right of the new rectangle if possible
				if ((g->x + g->width - 1) > (rect.first.x + rect.first.width - 1))
					gaps[rect.second].insert(cvRect(rect.first.x + rect.first.width, g->y,
													  g->x + g->width - rect.first.x - rect.first.width,
													  g->height));
				
				// Add a gap below the new rectangle if possible
                if ((g->y + g->height - 1) > (rect.first.y + rect.first.height - 1))
					gaps[rect.second].insert(cvRect(g->x, rect.first.y + rect.first.height, g->width,
													  g->y + g->height - rect.first.y - rect.first.height));
				
				// Remove the intersecting gap
				gaps[rect.second].erase(g++);
			}
			else {
				++g;
			}
		}
	}
	
	return gaps.size;
}

namespace zftdt
{

ConvMixtureFastHelper::HelperArr ConvMixtureFastHelper::arr;

static inline unsigned getMiniPow2B(unsigned n)
{
	//n is 32 bit unsigned 
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;
	return n;
}

ConvMixtureFastHelper::ConvMixtureFastHelper(const HOGPyramid& pyramid, const Mixture& mixture)
	:levelSizes(DPM_PYRAMID_MAX_LEVELS + 1),validLevelMask(DPM_PYRAMID_MAX_LEVELS + 1),filtersFFT(DPM_PYRAMID_MAX_LEVELS + 1),
	idxMapRectToLevel(DPM_PYRAMID_MAX_LEVELS + 1),mergeRects(DPM_PYRAMID_MAX_LEVELS + 1), scores(mixture.models.size + 1,pyramid.levels.size + 1)
{
	idxMapRectToLevel.size = 0;//clear idxMapRectToLevel
	mergeRects.size = 0;
    activeCount = MAX_ACTIVE_COUNT;
    ptrMixture = &mixture;
	//initialized
	//levelSizes = zftdt::DPMVector<cv::Size>(DPM_PYRAMID_MAX_LEVELS);
	//validLevelMask = zftdt::DPMVector<unsigned char>(DPM_PYRAMID_MAX_LEVELS);
    getPyramidLevelSizes(pyramid, levelSizes);
    int interval = pyramid.interval;
    int minValidLevel = getFullDetectionValidLevels(pyramid, mixture, validLevelMask);

	int cols = pyramid.levels[minValidLevel + interval]->width / NbFeatures; 
    int rows = pyramid.levels[minValidLevel + interval]->height;
    maxCols = pyramid.levels[minValidLevel + interval]->width / NbFeatures; 
    maxRows = pyramid.levels[minValidLevel + interval]->height;
	
    //maxCols = static_cast<int>(pow(2.0F, int(log(float(maxCols)) / log(2.0F) + 1.0F)));
    //maxRows = static_cast<int>(pow(2.0F, int(log(float(maxRows)) / log(2.0F) + 1.0F)));
	//get minimal power of 2
	maxCols = getMiniPow2B(maxCols);
	maxRows = getMiniPow2B(maxRows);
    normVal = 1.0f / (maxCols * maxRows);    
	//std::cout << "minValidLevel: " << minValidLevel << " maxCols: " << maxCols << " maxRows" << maxRows << std::endl;
    int numLevels = pyramid.levels.size;
	//printf("mergeRects:\n");
	for (int i = interval; i < numLevels; i++)
    {
        if (validLevelMask[i])
        {
        	const std::pair<CvRect, int>& pairI= std::make_pair(cvRect(0, 0, pyramid.levels[i]->width / NbFeatures, pyramid.levels[i]->height), i);
        	//mergeRects.size++;
        	//assert(mergeRects.size < mergeRects.maxNum);
            //mergeRects[mergeRects.size - 1] = pairI;
            mergeRects.push_back(pairI);
            //idxMapRectToLevel.size++;
            //assert(idxMapRectToLevel.size < idxMapRectToLevel.maxNum);
			//idxMapRectToLevel[idxMapRectToLevel.size - 1] = i;
            idxMapRectToLevel.push_back(i);

			//printf("(%d	%d	%d	%d)->%d\n",mergeRects[mergeRects.size - 1].first.x,mergeRects[mergeRects.size - 1].first.y,
			//	mergeRects[mergeRects.size - 1].first.width,mergeRects[mergeRects.size - 1].first.height,mergeRects[mergeRects.size - 1].second);
        }
    }

    numPlanes = blf(mergeRects, maxCols, maxRows);
	//printf("after BLF:\n");
	//for(int i =0 ; i < mergeRects.size; i++)
	//{
	//	printf("(%d	%d	%d	%d)->%d\n",mergeRects[i].first.x,mergeRects[i].first.y,
	//			mergeRects[i].first.width,mergeRects[i].first.height,mergeRects[i].second);
	//}

    int dims[2] = {maxRows, maxCols};

	//此处是构造函数，而且只运行一次，因此filterMirror，filterFFT就在此处分配释放内存
	//cv::Mat filterMirror(maxRows, maxCols * NbFeatures, CV_32FC1);
	CvMat *filterMirror = cvCreateMat(maxRows, maxCols * NbFeatures, CV_32FC1);
	assert(filterMirror != NULL);
	//新建一个filterFFT Mat去初始化fftwf_plan是多余的
    //cv::Mat filterFFT(maxRows, (maxCols + 2) * NbFeatures, CV_32FC1);
	//IplImage *filterFFT = cvCreateImage(cvSize((maxCols + 2) * NbFeatures, maxRows), IPL_DEPTH_32F, 1);
	int numModels = mixture.models.size;
    //filtersFFT.resize(numModels);
	filtersFFT.size = numModels;
	for (int m = 0; m < numModels; m++)
    {
        //filtersFFT[m].create(maxRows, (maxCols + 2) * NbFeatures, CV_32FC1);
		filtersFFT[m] = cvCreateMat(maxRows,(maxCols + 2) * NbFeatures, CV_32FC1);
        memset(filtersFFT[m]->data.ptr, 0, filtersFFT[m]->step * filtersFFT[m]->rows);
	}
#if FFTW_ENABLE
//    const fftwf_plan forwardOutPlace =
//		fftwf_plan_many_dft_r2c(2, dims, NbFeatures, filterMirror->data.fl, 0,
//								NbFeatures, 1,
//								(fftwf_complex*)(filtersFFT[0]->data.ptr), 0,
//								NbFeatures, 1, FFTW_PATIENT);
#endif
	for (int m = 0; m < numModels; m++)
    {
        //filtersFFT[m].create(maxRows, (maxCols + 2) * NbFeatures, CV_32FC1);
        //memset(filtersFFT[m].data, 0, maxRows * (maxCols + 2) * NbFeatures * sizeof(float));
        memset(filterMirror->data.ptr, 0, filterMirror->step * filterMirror->rows);
        int numRows = mixture.models[m].parts[0].filter->height;
        int numCols = mixture.models[m].parts[0].filter->width / NbFeatures;
        CvMat* currFilter = mixture.models[m].parts[0].filter;
        for (int i = 0; i < numRows; i++)
        {
            const float* filterRow = (float*)(currFilter->data.ptr + i * currFilter->step);
            float* transformRow = (float*)(filterMirror->data.ptr + filterMirror->step * ((maxRows - i) % maxRows));//filterMirror.ptr<float>((maxRows - i) % maxRows);
            for (int j = 0; j < numCols; j++)
            {
                memcpy(transformRow + NbFeatures * ((maxCols - j) % maxCols), filterRow + NbFeatures * j, NbFeatures * sizeof(float));
            }
        }
#if FFTW_ENABLE
        //fftwf_execute_dft_r2c(forwardOutPlace, filterMirror->data.fl, (fftwf_complex *)filtersFFT[m]->data.ptr);

		CvMat* result = filtersFFT[m];
		myFFT2 (filterMirror, result, NbFeatures, 0);
#endif
    }
	//for (int m = 0; m < numModels; m++)
	//{
	//	char name[32];
	//	sprintf(name,"filtersFFT%d.txt",m);
	//	fw_matrix_32FC1(filtersFFT[m],name);
	//}
#if FFTW_ENABLE
    //fftwf_destroy_plan(forwardOutPlace);
#endif
	cvReleaseMat(&filterMirror);//#add

    //cv::Mat levelFFT(maxRows, (maxCols + 2) * NbFeatures, CV_32FC1);
	levelFFT = cvCreateMat(maxRows, (maxCols + 2) * NbFeatures, CV_32FC1);
    //cv::Mat prodFFT(maxRows, (maxCols + 2), CV_32FC1);
	prodFFT = cvCreateMat(maxRows, maxCols + 2, CV_32FC1);
    //cv::Mat prod(maxRows, maxCols, CV_32FC1);
	prod = cvCreateMat(maxRows, maxCols, CV_32FC1);

	//cv::Mat levelDFT(maxRows, (maxCols + 2) * NbFeatures, CV_32FC1);
#if FFTW_ENABLE
//    forward =
//		fftwf_plan_many_dft_r2c(2, dims, NbFeatures, levelFFT->data.fl, 0,//levelDFT.data, 0,//
//								NbFeatures, 1,
//								(fftwf_complex*)(levelFFT->data.ptr), 0,//levelDFT.data, 0,//
//								NbFeatures, 1, FFTW_PATIENT);
//	inverse =
//		fftwf_plan_many_dft_c2r(2, dims, 1, (fftwf_complex*)(prodFFT->data.ptr), 0, 1, 1,
//							  prod->data.fl, 0, 1, 1, FFTW_PATIENT);
#endif
	//levelDFT.release();

	//FILE *forwardF = fopen("forward.plan", "wb+");
	//fftwf_fprint_plan(forward, forwardF);
	//fclose(forwardF);
	//fftwf_print_plan(forward);

	//init tmp scores for DPM2DVector<IplImage *> tmp(DPM_MAX_MODELS + 1,DPM_PYRAMID_MAX_LEVELS + 1);
	scores.xSize = numLevels;
	scores.ySize = numModels;
	for (int m = 0; m < numModels; m++)
	{            
		const Model::Part& currPart = mixture.models[m].parts[0];
		for(int j = 0; j < scores.ySize; j++)
		{
			scores.at(m, j) = NULL;
		}
		for (int i = 0; i < mergeRects.size; i++)
		{
            //const Model::Part& currPart = mixture.models[m].parts[0];
			int width = mergeRects[i].first.width - currPart.filter->width / NbFeatures + 1; 
			int height = mergeRects[i].first.height - currPart.filter->height + 1;
			scores.at(m,idxMapRectToLevel[i]) = cvCreateMat(height, width, CV_32FC1);
		}
	}
}

ConvMixtureFastHelper::~ConvMixtureFastHelper(void)
{
#if FFTW_ENABLE
//    if (forward)
//        fftwf_destroy_plan(forward);
//    if (inverse)
//        fftwf_destroy_plan(inverse);
#endif
	cvReleaseMat(&levelFFT);
	cvReleaseMat(&prodFFT);
	cvReleaseMat(&prod);
	for(int m = 0; m < scores.ySize; m++)
	{
		for(int i = 0; i < scores.xSize; i++)
		{
			CvMat *dst = scores.at(m,idxMapRectToLevel[i]);
			cvReleaseMat(&dst);
		}
	}
}


const zftdt::DPM2DVector<CvMat *>& ConvMixtureFastHelper::execute(const HOGPyramid& pyramid, const Mixture& mixture) const
{
	//scores.size();
	//scores are 2D-cv::Mat structures
	//IplImage *levelFFT = cvCreateImage(cvSize((maxCols + 2) * NbFeatures, maxRows), IPL_DEPTH_32F, 1);
    //IplImage *prodFFT = cvCreateImage(cvSize((maxCols + 2), maxRows),  IPL_DEPTH_32F, 1);
    //IplImage *prod = cvCreateImage(cvSize(maxCols, maxRows),  IPL_DEPTH_32F, 1);
    int numLevels = pyramid.levels.size;
    int numModels = mixture.models.size;
    int numRects = mergeRects.size;

	assert(numLevels <= DPM_PYRAMID_MAX_LEVELS);
	assert(numModels <= DPM_MAX_MODELS);
    //scores.resize(numModels);

    //for (int m = 0; m < numModels; m++)
    //{
    //    scores[m].resize(numLevels);
    //}
	//scores.xSize = numLevels;
	//scores.ySize = numModels;
	//debug
	//printf("before execute:\n");
	//for(int i =0 ; i < mergeRects.size; i++)
	//{
	//	printf("(%d	%d	%d	%d)->%d\n",mergeRects[i].first.x,mergeRects[i].first.y,
	//			mergeRects[i].first.width,mergeRects[i].first.height,mergeRects[i].second);
	//}
    for (int k = 0; k < numPlanes; k++)
    {
        memset(levelFFT->data.ptr, 0, levelFFT->step * levelFFT->rows);
        for (int i = 0; i < numRects; i++)
        {
            if (mergeRects[i].second == k)
            {
				//mergeRects.first->cv::Rect(0, 0, pyramid.levels[i]->width / NbFeatures, pyramid.levels[i]->height);

				CvRect rect = cvRect(mergeRects[i].first.x * NbFeatures, mergeRects[i].first.y,
                              mergeRects[i].first.width * NbFeatures, mergeRects[i].first.height);
				//cv::Mat part = levelFFT(rect);
				//cv::Mat tmp(pyramid.levels[idxMapRectToLevel[i]],0);
				//tmp.copyTo(part);
				//printf("rect: %d	%d	%d	%d\n",rect.x,rect.y,rect.width, rect.height);
				//cvSetImageROI(levelFFT,rect);
				//cvCopy(pyramid.levels[idxMapRectToLevel[i]],levelFFT);
				//cvResetImageROI(levelFFT);
				CvMat* &src = pyramid.levels[idxMapRectToLevel[i]];
				copyMatWithRoi(src,levelFFT,cvRect(0,0,src->cols,src->rows),rect);
            }
        }
#if 0
		fw_matrix_32FC1(levelFFT,"forward1.txt");
#endif
#if FFTW_ENABLE
        //fftwf_execute_dft_r2c(forward, levelFFT->data.fl, (fftwf_complex*)levelFFT->data.ptr);
        myFFT2 (levelFFT, levelFFT, NbFeatures, 1);
#endif
#if 0
		fw_matrix_32FC1(levelFFT,"forward2.txt");
#endif
        for (int m = 0; m < numModels; m++)
        {            
            const Model& currModel = mixture.models[m];
            const Model::Part& currPart = currModel.parts[0];

			CvMat* currFilterFFT = filtersFFT[m];
            memset(prodFFT->data.ptr, 0, prodFFT->step * prodFFT->rows);                 
            for (int i = 0; i < maxRows; i++)
            {

                const float* ptrF = (float*)(currFilterFFT->data.ptr + i * currFilterFFT->step);//currFilterFFT.ptr<float>(i);
                const float* ptrL = (float*)(levelFFT->data.ptr + i * levelFFT->step);//levelFFT.ptr<float>(i);
                float* ptrP = (float*)(prodFFT->data.ptr + i * prodFFT->step);//prodFFT.ptr<float>(i);
                for (int j = 0; j < maxCols / 2 + 1; j++)
                {
                    const float* ptrCurrF = ptrF + j * NbFeatures * 2;
                    const float* ptrCurrL = ptrL + j * NbFeatures * 2;
                    float val0 = 0, val1 = 0;
                    for (int u = 0; u < NbFeatures - 1; u++)
                    {
                        val0 += ptrCurrF[u * 2] * ptrCurrL[u * 2] - ptrCurrF[u * 2 + 1] * ptrCurrL[u * 2 + 1];
                        val1 += ptrCurrF[u * 2] * ptrCurrL[u * 2 + 1] + ptrCurrF[u * 2 + 1] * ptrCurrL[u * 2];
                    }
                    ptrP[j * 2] = val0;
                    ptrP[j * 2 + 1] = val1;
                    ptrP[j * 2] *= normVal;
                    ptrP[j * 2 + 1] *= normVal;
                }
            }
#if FFTW_ENABLE
            //fftwf_execute_dft_c2r(inverse, (fftwf_complex*)prodFFT->data.ptr, prod->data.fl);
            myIFFT2 (prodFFT, prod, 1);
#endif
            for (int i = 0; i < numRects; i++)
            {

				if (mergeRects[i].second == k)
                {
					CvMat *dst = scores.at(m,idxMapRectToLevel[i]);
                    CvRect rect = cvRect(mergeRects[i].first.x, mergeRects[i].first.y,
                                  dst->width,//mergeRects[i].first.width - currPart.filter->width / NbFeatures + 1, 
                                  dst->height);//mergeRects[i].first.height - currPart.filter->height + 1);
                    //cv::Mat part = prod(rect);
                    //part.copyTo(scores.at(m,idxMapRectToLevel[i]));
					//cvSetImageROI(prod,rect);
					//cvCopy(prod,dst);
					//cvResetImageROI(prod);
					copyMatWithRoi(prod,dst,rect,cvRect(0,0,dst->cols,dst->rows));
                }
            }
        }
    }
	return scores;
}

const ConvMixtureFastHelper* ConvMixtureFastHelper::getHelper(const HOGPyramid& pyramid, const Mixture& mixture)
{

	//TODO:1.00
	static DPMVector<CvSize> levelSizes(DPM_PYRAMID_MAX_LEVELS + 1);
    getPyramidLevelSizes(pyramid, levelSizes);
    static DPMVector<unsigned char> validLevelMask(DPM_PYRAMID_MAX_LEVELS + 1);
    int minValidLevel = getFullDetectionValidLevels(pyramid, mixture, validLevelMask);

	int cols = pyramid.levels[minValidLevel + pyramid.interval]->width / NbFeatures;
    int rows = pyramid.levels[minValidLevel + pyramid.interval]->height;
    int maxCols = pyramid.levels[minValidLevel + pyramid.interval]->width / NbFeatures; 
    int maxRows = pyramid.levels[minValidLevel + pyramid.interval]->height;
    //maxCols = static_cast<int>(pow(2.0F, int(log(float(maxCols)) / log(2.0F) + 1.0F)));
    //maxRows = static_cast<int>(pow(2.0F, int(log(float(maxRows)) / log(2.0F) + 1.0F)));
	maxCols = getMiniPow2B(maxCols);
	maxRows = getMiniPow2B(maxRows);

    for (HelperArr::iterator itr = arr.begin(); itr != arr.end();)
    {
        ConvMixtureFastHelper& helper = *(*itr);
        if ((helper.ptrMixture == &mixture) && equals(helper.levelSizes, levelSizes) &&
            equals(helper.validLevelMask, validLevelMask))
        {
            if (helper.activeCount < MAX_ACTIVE_COUNT)
                helper.activeCount++;
            std::swap(*itr, arr.front());
            return arr.front();
        }
        else if (helper.activeCount > 0)
            helper.activeCount--;
        if (helper.activeCount <= 0)// && *((*itr).refcount) == 1)//TODO:1.1
            itr = arr.erase(itr);
        else
            ++itr;
    }
    ConvMixtureFastHelper* ptrHelper = new ConvMixtureFastHelper(pyramid, mixture);
    arr.push_front(ptrHelper);
    return arr.front();

}

const zftdt::DPM2DVector<CvMat *>& ConvMixtureFastHelper::run(const HOGPyramid& pyramid, const Mixture& mixture)
{
	const ConvMixtureFastHelper* helper = getHelper(pyramid, mixture);
    return helper->execute(pyramid, mixture);
}

//const zftdt::DPM2DVector<IplImage *>& ConvMixtureFastHelper::getConvMixtureScores(void) const
//{
//	return scores;
//}

}
