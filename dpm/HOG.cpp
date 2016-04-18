/*
 * HOG.cpp
 *
 *  Created on: 2015-3-13
 *      Author: LinZW
 */

#include "HOG.h"
#include <math.h>
#include <float.h>
#include <limits.h>
#include <algorithm>
#include <assert.h>
#include "ti/platform/platform.h"
extern void write_uart(char* msg);

#define ENABLE_SSE 0
//static const int NbFeatures = 32;
#define NbFeatures (32)
static const int NbBins = 18;

static const float M_PI = 3.14159265358979323846;

static const float EPS = FLT_EPSILON;//std::numeric_limits<float>::epsilon();

struct HOGTable
{
	char bins[512][512][2];
	float magnitudes[512][512][2];

	HOGTable()
	{
		for (int dy = -255; dy <= 255; ++dy) {
			for (int dx = -255; dx <= 255; ++dx) {
				// Magnitude in the range [0, 1]
				const float magnitude = sqrt(float(dx * dx + dy * dy)) / 255.0F;

				// Angle in the range [-pi, pi]
				float angle = atan2(static_cast<float>(dy), static_cast<float>(dx));

				// Convert it to the range [9.0, 27.0]
				angle = angle * (9.0F / M_PI) + 18.0F;

				// Convert it to the range [0, 18)
				if (angle >= 18.0F)
					angle -= 18.0F;

				// Bilinear interpolation
				const int bin0 = angle;
				const int bin1 = (bin0 < 17) ? (bin0 + 1) : 0;
				const float alpha = angle - bin0;

				bins[dy + 255][dx + 255][0] = bin0;   //0,1,2,...,17
				bins[dy + 255][dx + 255][1] = bin1;   //1,2,3,...,17,0  ��¼ÿ�����ص��Ӧ�ݶȷ������� bin ���
				magnitudes[dy + 255][dx + 255][0] = magnitude * (1.0 - alpha);   //�ݶȸ�ֵ������ bin ��ӳ�䲿��
				magnitudes[dy + 255][dx + 255][1] = magnitude * alpha;
			}
		}
	}

private:
	HOGTable(const HOGTable &) throw ();
	void operator=(const HOGTable &) throw ();
};

static HOGTable hogTable;

bool zftdt::HOGPyramid::isEmpty(void) const
{

	if (levels.size == 0)
        return true;
//    int numLevels = levels.size;
//    for (int i = 0; i < numLevels; i++)
//    {
//#if !OPT_LEVEL_CVMAT
//        if (levels[i].data)
//            return false;
//#else
//		if (levels[i]->imageData)//always true
//            return false;
//#endif
//    }
    //return true;
	return false;
}

zftdt::HOGPyramid::HOGPyramid(const int imageW, const int imageH, const int padx_,const int pady_, const int interval_, const int maxFilterSideLen)
	:padx(padx_),pady(pady_),interval(interval_),levels(DPM_PYRAMID_MAX_LEVELS + 1),scaledImage(DPM_PYRAMID_MAX_LEVELS + 1)
{
	//levels.size = 0;
	//levels.maxNum = DPM_PYRAMID_MAX_LEVELS;
	//levels.ptr = new cv::Mat[DPM_PYRAMID_MAX_LEVELS]();
	//assert(levels.ptr != NULL);
	#if OPT_LEVEL_CVMAT
	//initial levels,must equal to the deconstructor size
	levels.size = levels.maxNum;
	for(int i = 0; i < levels.size; i++)
	{
		levels[i] = NULL;
	}
	scaledImage.size = scaledImage.maxNum;
	for(int i = 0; i < scaledImage.size; i++)
	{
		scaledImage[i] = NULL;
	}

	//void *ptr_levels = NULL;
	int width =imageW;
	int height = imageH;
	int step = (width * 3 + (DPM_ALLOC_ALIGN - 1) / DPM_ALLOC_ALIGN) * DPM_ALLOC_ALIGN;

	//ptr_image = _aligned_malloc(height * step,DPM_ALLOC_ALIGN);
	//int minDetectStride = std::min(DPM_MAX_IN_WIDTH,DPM_MAX_IN_HEIGHT) / pow(2.0f,static_cast<float>(DPM_PYRAMID_MAX_LEVELS)/interval);
	//if(minDetectStride > DPM_PYRAMID_MIN_DETECT_STRIDE)//levels
	//{
	//	levels.maxNum = DPM_PYRAMID_MAX_LEVELS + 1;
	//}
	//else
	//{
	//	levels.maxNum = interval * (log(std::min(DPM_MAX_IN_WIDTH,DPM_MAX_IN_HEIGHT))/log(static_cast<float>(DPM_PYRAMID_MIN_DETECT_STRIDE))) + 1;
	//}
	int maxScale = std::min(DPM_PYRAMID_MAX_LEVELS,
        (int)ceil(log(std::min(imageW, imageH) / static_cast<float>(std::max(DPM_PYRAMID_MIN_DETECT_STRIDE,maxFilterSideLen * 8))) / log(2.0f) * interval) + interval);

	float scale = 1.0f;
	for(int i = 0; i < interval; i++)
	{
		scale = pow(2.0f, -i / static_cast<float> (interval));
		width = cvRound(imageW * scale);//static_cast<int>(imageW * scale + (imageW * scale >= 0 ? 0.5 : -0.5)) ;
		height = cvRound(imageH * scale);//static_cast<int>(imageH * scale + (imageH * scale >= 0 ? 0.5 : -0.5));
		int _width = (width + (DPM_HOG_CELLSIZE >> 2)) / (DPM_HOG_CELLSIZE >> 1) + (DPM_HOG_PADX << 1);
		_width *= NbFeatures;
		int _height = (height + (DPM_HOG_CELLSIZE >> 2)) / (DPM_HOG_CELLSIZE >> 1) + (DPM_HOG_PADY << 1);
		//step = ((_width * sizeof(float)  + (DPM_ALLOC_ALIGN - 1)) / DPM_ALLOC_ALIGN) * DPM_ALLOC_ALIGN;
		//printf("required size of Level %d is %d bytes\n", i,_height * step);
		//ptr_levels = _aligned_malloc(_height * step, DPM_ALLOC_ALIGN);// malloc(_height * step);
		//printf("malloc size %d bytes of level %d, addr = 0x%08x\n",_height * step,i, (int)ptr_levels);
		//assert(ptr_levels != NULL);
		//levels.data[i] = cv::Mat(_height, _width, CV_32FC1, ptr_levels[i]);//Level0-5, 1.0 sizes, half cell size
		//IplImage &levi = levels[i];
		//cvInitImageHeader(&levi,cvSize(_width , _height), IPL_DEPTH_32F, 1);
		//cvSetImageData(&levi, ptr_levels, step);
		levels[i] = cvCreateMat(_height, _width, CV_32FC1);
		assert(levels[i] != NULL);

		scaledImage[i] = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
		assert(scaledImage[i] != NULL);

		if (i + interval <= maxScale)
		{
			_width = (width + (DPM_HOG_CELLSIZE >> 1)) / (DPM_HOG_CELLSIZE) + (DPM_HOG_PADX << 1);
			_width *= NbFeatures;
			_height = (height + (DPM_HOG_CELLSIZE >> 1)) / (DPM_HOG_CELLSIZE) + (DPM_HOG_PADY << 1);
			//step = ((_width * sizeof(float)  + (DPM_ALLOC_ALIGN - 1)) / DPM_ALLOC_ALIGN) * DPM_ALLOC_ALIGN;
			//ptr_levels[i + interval] = _aligned_malloc(_height * step, DPM_ALLOC_ALIGN);
			//printf("required size of Level %d is %d bytes\n", i + interval,_height * step);
			//ptr_levels = _aligned_malloc(_height * step, DPM_ALLOC_ALIGN);//malloc(_height * step);
			//printf("malloc size %d bytes of level %d, addr = 0x%08x\n",_height * step, i + interval, (int)ptr_levels);
			//assert(ptr_levels != NULL);
			//levels.data[i + interval] = cv::Mat(_height, _width, CV_32FC1, ptr_levels[i + interval]);//Level6-10, 1.0 sizes, cell size
			//IplImage &levii = levels[i + interval];
			//cvInitImageHeader(&levii,cvSize(_width, _height), IPL_DEPTH_32F, 1);
			//cvSetImageData(&levii, ptr_levels, step);
			levels[i + interval] = cvCreateMat(_height, _width, CV_32FC1);
			assert(levels[i + interval] != NULL);
		}

		for (int j = 2; i + j * interval <= maxScale; ++j)
		{
			//width >>= 1;
			//height >>= 1;
			width = cvRound(width * 0.5f);
			height = cvRound(height * 0.5f);
			_width = (width + (DPM_HOG_CELLSIZE >> 1)) / (DPM_HOG_CELLSIZE) + (DPM_HOG_PADX << 1);
			_width *= NbFeatures;
			_height = (height + (DPM_HOG_CELLSIZE >> 1)) / (DPM_HOG_CELLSIZE) + (DPM_HOG_PADY << 1);
			//step = ((_width * sizeof(float)  + (DPM_ALLOC_ALIGN - 1)) / DPM_ALLOC_ALIGN) * DPM_ALLOC_ALIGN;
			//ptr_levels[i + j * interval] = _aligned_malloc(_height * step, DPM_ALLOC_ALIGN);
			//printf("required size of Level %d is %d bytes\n", i + j * interval,_height * step);
			//ptr_levels = _aligned_malloc(_height * step, DPM_ALLOC_ALIGN);//malloc(_height * step);
			//printf("malloc size %d bytes of level %d, addr = 0x%08x\n",_height * step, i + j * interval, (int)ptr_levels);
			//assert(ptr_levels != NULL);
			//levels.data[i + j * interval] = cv::Mat(_height, _width, CV_32FC1, ptr_levels[i + j * interval]);//Level11-20, 2^(-i) sizes, cell size
			//IplImage &levj = levels[i + j * interval];
			//cvInitImageHeader(&levj,cvSize(_width, _height), IPL_DEPTH_32F, 1);
			//cvSetImageData(&levj, ptr_levels, step);
			levels[i + j * interval] = cvCreateMat(_height, _width, CV_32FC1);
			assert(levels[i + j * interval] != NULL);
			scaledImage[i + j * interval] = cvCreateImage(cvSize(width,height),IPL_DEPTH_8U, 3);
			assert(scaledImage[i + j * interval] != NULL);
		}
	}

#endif
}

zftdt::HOGPyramid::~HOGPyramid()
{
	//levels.size = 0;
	//levels.maxNum = 0;
	//delete [] levels.ptr;
#if OPT_LEVEL_CVMAT
	for(int i = 0; i < scaledImage.size; i++)
	{
		IplImage* &src = scaledImage[i];
		if(src != NULL)
		{
			cvReleaseImage(&src);
			src = NULL;
		}
	}

	//levels.size = levels.maxNum;//only for free
	for(int i = 0; i < levels.size; i++)
	{
		//if(ptr_levels[i])
		//{
		//	_aligned_free(ptr_levels[i]);
		//	ptr_levels[i] = NULL;
		//}
		//IplImage& leveli = levels[i];
		//if(leveli.imageData)
		//{
		//	_aligned_free(leveli.imageData);//free(leveli.imageData);
		//	leveli.imageData = NULL;
		//}
		CvMat* &src = levels[i];
		if(src != NULL)
			cvReleaseMat(&src);
	}
#endif
}

void zftdt::HOGPyramid::build(const IplImage* image, const int minSideLen, const int maxNumLevels)
{
	levels.size = 0;
    if (image == NULL || (padx < 1) || (pady < 1) || (interval < 1))
    {
		return;
	}
	int maxScale = std::min(maxNumLevels,
        (int)ceil(log(std::min(image->width, image->height) / double(minSideLen)) / log(2.0) * interval) + interval);

	// Cannot compute the pyramid on images too small
	if (maxScale < interval)
    {
		return;
	}
	//int64 i = getTickCount();
	//padx = padx_;
	//pady = pady_;
	//interval = interval_;
#define DEBUG_DATA 0
	levels.size = maxScale + 1;//
	//printf("building HOG:\n");
	for (int i = 0; i < interval; ++i)
    {
		const double scale = pow(2.0, -static_cast<double>(i) / interval);

		//int scl_w = image.cols * scale;
		//int scl_h = image.rows * scale;
		//cv::Mat scaled = cv::Mat(scl_h,scl_w,image.type,ptr_image);
		//cv::Mat scaled;
		//cv::resize(image, scaled, cv::Size(), scale, scale, cv::INTER_LINEAR);
		cvResize(image,scaledImage[i],CV_INTER_LINEAR);
		// First octave at twice the image resolution
		//printf("image%d-><%d,%d>\n",i,scaledImage[i]->width,scaledImage[i]->height);
#if DEBUG_DATA
		char name[32];
		sprintf(name,"scale_%d.mat",i);
		fw_matrix_8UC3(scaledImage[i],name);
#endif
		HOG(scaledImage[i], levels[i], padx, pady, DPM_HOG_CELLSIZE>>1);

		// Second octave at the original resolution
		if (i + interval <= maxScale)
		{
			HOG(scaledImage[i], levels[i + interval], padx, pady, DPM_HOG_CELLSIZE);
		}
		// Remaining octaves
		IplImage *scaleJ = scaledImage[i];
		for (int j = 2; i + j * interval <= maxScale; ++j)
        {
			//cv::resize(scaled, scaled, cv::Size(), 0.5, 0.5, cv::INTER_LINEAR);

			cvResize(scaleJ,scaledImage[i + j * interval], CV_INTER_LINEAR);
			scaleJ = scaledImage[i + j * interval];
			HOG(scaleJ, levels[i + j * interval], padx, pady, DPM_HOG_CELLSIZE);
#if DEBUG_DATA
			sprintf(name,"HOG_leves%d.mat",i + j * interval);
			fw_matrix_32FC1(levels[i + j * interval],name);
			printf("image%d-><%d,%d>\n",i + j * interval, scaleJ->width, scaleJ->height);
			sprintf(name,"scale_%d.mat",i + j * interval);
			fw_matrix_8UC3(scaleJ,name);
#endif
		}
	}
#ifdef DEBUG_DATA
#undef DEBUG_DATA
#endif
}



inline float invSqrt(float num)
{
    float x = num * 0.5F, y = num;
    int i = *(int*)&y;
    i = 0X5F3759DF - (i >> 1);
    y = *(float*)&i;
    y = y * (1.5F - (x * y * y));
    //y = y * (1.5F - (x * y * y));
    return y;
}

void HOG(const IplImage* image, CvMat* level, const int padX, const int padY, const int cellSize)
{
    const int width = image->width;
	const int height = image->height;
	const int depth = image->nChannels;
	CvMat *curLevel = level;
	// Make sure the image is big enough
	if ((width < cellSize) || (height < cellSize) ||
        (depth < 1) || (padX < 1) || (padY < 1) || (cellSize < 1))
    {
        //level.release();
        return;
	}

	// Resize the feature matrix
    //const int levelHeight = (height + cellSize / 2) / cellSize + 2 * padY;
    //const int levelWidth = (width + cellSize / 2) / cellSize + 2 * padX;

	const int levelHeight = curLevel->rows;
    const int levelWidth = curLevel->cols;

    //level.create(levelHeight, levelWidth * NbFeatures, CV_32FC1);
    //memset(level.data, 0, levelHeight * levelWidth * NbFeatures * sizeof(float));
	//curLevel.width = levelWidth * NbFeatures;
	//curLevel.height = levelHeight;
	memset(curLevel->data.ptr, 0, levelHeight * curLevel->step);

	const float invCellSize = static_cast<float>(1) / cellSize;

    for (int y = 0; y < height; ++y)
    {
		const unsigned char* linem = (unsigned char*)(image->imageData + image->widthStep * std::max(y - 1, 0));//image.ptr<unsigned char>(std::max(y - 1, 0));
		const unsigned char* line =  (unsigned char*)(image->imageData + image->widthStep * y);//image.ptr<unsigned char>(y);
		const unsigned char* linep = (unsigned char*)(image->imageData + image->widthStep * std::min(y + 1, height - 1));//image.ptr<unsigned char>(std::min(y + 1, height - 1));
		for (int x = 0; x < width; ++x)
        {
			// Use the channel with the largest gradient magnitude
			int maxMagnitude = 0;
			int argDx = 255;
			int argDy = 255;

			for (int i = 0; i < depth; ++i)
            {
				const int dx = static_cast<int>(line[std::min(x + 1, width - 1) * depth + i]) -
							   static_cast<int>(line[std::max(x - 1, 0) * depth + i]);
				const int dy = static_cast<int>(linep[x * depth + i]) -
							   static_cast<int>(linem[x * depth + i]);

				int sqrMag = dx * dx + dy * dy;
				if (sqrMag > maxMagnitude)
                {
					maxMagnitude = sqrMag;
					argDx = dx + 255;
					argDy = dy + 255;
				}
			}

            const char bin0 = hogTable.bins[argDy][argDx][0];
			const char bin1 = hogTable.bins[argDy][argDx][1];
			const float magnitude0 = hogTable.magnitudes[argDy][argDx][0];
			const float magnitude1 = hogTable.magnitudes[argDy][argDx][1];

			// Bilinear interpolation
			const float xp = (x + 0.5F) * invCellSize + padX - 0.5F;
			const float yp = (y + 0.5F) * invCellSize + padY - 0.5F;
			const int ixp = xp;
			const int iyp = yp;
			const float xp0 = xp - ixp;
			const float yp0 = yp - iyp;
			const float xp1 = 1 - xp0;
			const float yp1 = 1 - yp0;
            const float w0 = xp1 * yp1;
            const float w1 = xp0 * yp1;
            const float w2 = xp1 * yp0;
            const float w3 = xp0 * yp0;

            float* ptrCell;
			//ptrCell = level.ptr<float>(iyp) + ixp * NbFeatures;
            ptrCell = (float*)(curLevel->data.ptr + (iyp * curLevel->step)) + ixp * NbFeatures;
            ptrCell[bin0] += w0 * magnitude0;
            ptrCell[bin1] += w0 * magnitude1;
            ptrCell += NbFeatures;
            ptrCell[bin0] += w1 * magnitude0;
            ptrCell[bin1] += w1 * magnitude1;
            //ptrCell = curLevel.ptr<float>(iyp + 1) + ixp * NbFeatures;
			ptrCell = (float*)(curLevel->data.ptr + ((iyp + 1) * curLevel->step)) + ixp * NbFeatures;
            ptrCell[bin0] += w2 * magnitude0;
            ptrCell[bin1] += w2 * magnitude1;
            ptrCell += NbFeatures;
            ptrCell[bin0] += w3 * magnitude0;
            ptrCell[bin1] += w3 * magnitude1;
		}
	}

    int rows = curLevel->height, cols = curLevel->width / NbFeatures;
	// Compute the "gradient energy" of each cell, i.e. ||C(i,j)||^2
	for (int y = 0; y < rows; ++y)
    {
        //float* ptrRow = curLevel.ptr<float>(y);
		float* ptrRow = (float*)(curLevel->data.ptr + (y * curLevel->step));
		for (int x = 0; x < cols; ++x)
        {
			float sumSq = 0;
			float* ptrCell = ptrRow + x * NbFeatures;
			for (int i = 0; i < 9; ++i)
            {
                float sum = ptrCell[i] + ptrCell[i + 9];
				sumSq += sum * sum;
            }
			ptrCell[NbFeatures - 1] = sumSq;
		}
	}

	for (int y = padY; y < rows - padY; ++y)
    {
        //float* ptrPrevRow = curLevel.ptr<float>(y - 1);
        //float* ptrCurrRow = curLevel.ptr<float>(y);
        //float* ptrNextRow = curLevel.ptr<float>(y + 1);
		float* ptrPrevRow = (float*)(curLevel->data.ptr + ((y - 1) * curLevel->step));
        float* ptrCurrRow = (float*)(curLevel->data.ptr + (y * curLevel->step));
        float* ptrNextRow = (float*)(curLevel->data.ptr + ((y + 1) * curLevel->step));
		for (int x = padX; x < cols - padX; ++x)
        {
            //int offsetCell = x * NbFeatures;
            //int offset0 = offsetCell - 1, offset1 = offset0 + NbFeatures, offset2 = offset1 + NbFeatures;
            // val0 val1 val2
            // val3 val4 val5
            // val6 val7 val8
            float val0 = ptrPrevRow[x * NbFeatures - 1], val1 = ptrPrevRow[(x + 1) * NbFeatures - 1], val2 = ptrPrevRow[(x + 2) * NbFeatures - 1];
            float val3 = ptrCurrRow[x * NbFeatures - 1], val4 = ptrCurrRow[(x + 1) * NbFeatures - 1], val5 = ptrCurrRow[(x + 2) * NbFeatures - 1];
            float val6 = ptrNextRow[x * NbFeatures - 1], val7 = ptrNextRow[(x + 1) * NbFeatures - 1], val8 = ptrNextRow[(x + 2) * NbFeatures - 1];
            //float val0 = ptrPrevRow[offset0], val1 = ptrPrevRow[offset1], val2 = ptrPrevRow[offset2];
            //float val3 = ptrCurrRow[offset0], val4 = ptrCurrRow[offset1], val5 = ptrCurrRow[offset2];
            //float val6 = ptrNextRow[offset0], val7 = ptrNextRow[offset1], val8 = ptrNextRow[offset2];
            const float n0 = invSqrt(val0 + val1 + val3 + val4 + EPS);
			const float n1 = invSqrt(val1 + val2 + val4 + val5 + EPS);
			const float n2 = invSqrt(val3 + val4 + val6 + val7 + EPS);
			const float n3 = invSqrt(val4 + val5 + val7 + val8 + EPS);

            float* ptrCell = ptrCurrRow + x * NbFeatures;
            //float* ptrCell = ptrCurrRow + offsetCell;
			// Contrast-insensitive features
			for (int i = 0; i < 9; ++i)
            {
				const float sum = ptrCell[i] + ptrCell[i + 9];
				const float h0 = std::min(sum * n0, static_cast<float>(0.2));
				const float h1 = std::min(sum * n1, static_cast<float>(0.2));
				const float h2 = std::min(sum * n2, static_cast<float>(0.2));
				const float h3 = std::min(sum * n3, static_cast<float>(0.2));
				ptrCell[i + 18] = (h0 + h1 + h2 + h3) * static_cast<float>(0.5);
			}

			// Contrast-sensitive features
			float t0 = 0;
			float t1 = 0;
			float t2 = 0;
			float t3 = 0;

			for (int i = 0; i < 18; ++i)
            {
				const float sum = ptrCell[i];
				const float h0 = std::min(sum * n0, static_cast<float>(0.2));
				const float h1 = std::min(sum * n1, static_cast<float>(0.2));
				const float h2 = std::min(sum * n2, static_cast<float>(0.2));
				const float h3 = std::min(sum * n3, static_cast<float>(0.2));
				ptrCell[i] = (h0 + h1 + h2 + h3) * static_cast<float>(0.5);
				t0 += h0;
				t1 += h1;
				t2 += h2;
				t3 += h3;
			}

			// Texture features
			ptrCell[27] = t0 * static_cast<float>(0.2357);
			ptrCell[28] = t1 * static_cast<float>(0.2357);
			ptrCell[29] = t2 * static_cast<float>(0.2357);
			ptrCell[30] = t3 * static_cast<float>(0.2357);
		}
	}

	// Truncation features
	for (int y = 0; y < rows; ++y)
    {
        //float* ptrRow = curLevel.ptr<float>(y);
		float* ptrRow = (float*)(curLevel->data.ptr + (y * curLevel->step));
		for (int x = 0; x < cols; ++x)
        {
			float* ptrCell = ptrRow + x * NbFeatures;
            if ((y < padY) || (y >= rows - padY) ||
                (x < padX) || (x >= cols - padX))
            {
				memset(ptrCell, 0, NbFeatures * sizeof(float));
				ptrCell[NbFeatures - 1] = 1;
			}
			else
            {
				ptrCell[NbFeatures - 1] = 0;
			}
		}
	}
}
#define SSE_INNER_PROD(ptrCurrX, ptrCurrY, sum)               \
__m128 sumVect = _mm_set_ps1(0.0f);                           \
__m128 prod;                                                  \
prod = _mm_mul_ps(*((__m128*)ptrCurrX), *((__m128*)ptrCurrY));\
sumVect = _mm_add_ps(prod, sumVect);                          \
ptrCurrX += 4, ptrCurrY +=4;                                  \
prod = _mm_mul_ps(*((__m128*)ptrCurrX), *((__m128*)ptrCurrY));\
sumVect = _mm_add_ps(prod, sumVect);                          \
ptrCurrX += 4, ptrCurrY +=4;                                  \
prod = _mm_mul_ps(*((__m128*)ptrCurrX), *((__m128*)ptrCurrY));\
sumVect = _mm_add_ps(prod, sumVect);                          \
ptrCurrX += 4, ptrCurrY +=4;                                  \
prod = _mm_mul_ps(*((__m128*)ptrCurrX), *((__m128*)ptrCurrY));\
sumVect = _mm_add_ps(prod, sumVect);                          \
ptrCurrX += 4, ptrCurrY +=4;                                  \
prod = _mm_mul_ps(*((__m128*)ptrCurrX), *((__m128*)ptrCurrY));\
sumVect = _mm_add_ps(prod, sumVect);                          \
ptrCurrX += 4, ptrCurrY +=4;                                  \
prod = _mm_mul_ps(*((__m128*)ptrCurrX), *((__m128*)ptrCurrY));\
sumVect = _mm_add_ps(prod, sumVect);                          \
ptrCurrX += 4, ptrCurrY +=4;                                  \
prod = _mm_mul_ps(*((__m128*)ptrCurrX), *((__m128*)ptrCurrY));\
sumVect = _mm_add_ps(prod, sumVect);                          \
ptrCurrX += 4, ptrCurrY +=4;                                  \
prod = _mm_mul_ps(*((__m128*)ptrCurrX), *((__m128*)ptrCurrY));\
sumVect = _mm_add_ps(prod, sumVect);                          \
const float* ptrSum = (float*)&sumVect;                       \
sum += (ptrSum[0] + ptrSum[1] + ptrSum[2] + ptrSum[3])

void convolve(const CvMat* x, const CvMat* y, CvMat** z, const CvRect& xRect, CvPoint& actualTopLeft, int featCount, int featStep)
{
    //if (!x.data || !y->imageData || x.rows < y->height || x.cols < y->width)
    //{
    //    z.release();
    //    return;
    //}
    if (x == NULL || x->data.fl == NULL || y == NULL || y->data.fl == NULL || x->height < y->height || x->width < y->width)
    {
        //z.release();
		*z = NULL;
        return;
    }
    CvRect xFullRect = cvRect(0, 0, (x->width - y->width) / featStep + 1, x->height - y->height + 1);
    CvRect actualRect = rectOverlap(xRect,xFullRect);//xRect & xFullRect;
    actualTopLeft = cvPoint(actualRect.x, actualRect.y);//actualRect.tl();
    int zRows = actualRect.height, zCols = actualRect.width;
    *z = cvCreateMat(zRows, zCols, CV_32FC1);
    for (int i = 0; i < zRows; i++)
    {
        float* ptrZ = (float*)((*z)->data.ptr + (*z)->step * i);//z.ptr<float>(i);
        for (int j = 0; j < zCols; j++)
        {
            float sum = 0;
            for (int u = 0; u < y->height; u++)
            {
                const float* ptrX = (float*)(x->data.ptr + (u + i + actualTopLeft.y) * x->step) + (j + actualTopLeft.x) * featStep;//x.ptr<float>(u + i + actualTopLeft.y) + (j + actualTopLeft.x) * featStep;
                const float* ptrY = (float*)(y->data.ptr + u * y->step);
                for (int v = 0; v < y->width / featStep; v++)
                {
                    const float* ptrCurrX = ptrX + v * featStep;
                    const float* ptrCurrY = ptrY + v * featStep;
#if ENABLE_SSE
                    SSE_INNER_PROD(ptrCurrX, ptrCurrY, sum);
#else
                    for (int w = 0; w < featCount; w++)
                        sum += ptrCurrX[w] * ptrCurrY[w];
#endif
                }
            }
            ptrZ[j] = sum;
        }
    }
}






