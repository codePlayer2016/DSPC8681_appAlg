/*
 * DPM.cpp
 *
 *  Created on: 2015-3-13
 *      Author: LinZW
 */

#include "HOG.h"
#include "DPM.h"
//#include "DPMDetector.h"
#include <math.h>
#include <float.h>
#include <limits.h>
#include <algorithm>
#include <fstream>
#include <stdio.h>
extern std::ofstream foutDebug;

#define ALLOW_MAX_POS_IN_PADDED_AREA 1
//static const int NbFeatures = 32;


#ifdef _WIN32
const static float M_PI = 3.14159265358979323846;
#endif
const static float EPS = FLT_EPSILON;//std::numeric_limits<float>::epsilon();



struct Less
{
    Less(int num_) : num(num_) {}
    bool operator()(int test) {return test < num;}
    int num;
};

struct Greater
{
    Greater(int num_) : num(num_) {}
    bool operator()(int test) {return test > num;}
    int num;
};

CvRect rectOverlap(const CvRect &a, const CvRect &b)
{
	int xMax = a.x > b.x ? a.x : b.x;
	int yMax = a.y > b.y ? a.y : b.y;
	int wMin = (a.width + a.x) > (b.width + b.x) ? (b.width + b.x) - xMax : (a.width + a.x) - xMax;
	int hMin = (a.height + a.y) > (b.height + b.y) ? (b.height + b.y) - yMax : (a.height + a.y) - yMax;
	if(hMin <= 0 || wMin <= 0)
		return cvRect(0,0,0,0);
	return cvRect(xMax, yMax, wMin, hMin);
}

class Intersector
{
public:
	/// Constructor.
	/// @param[in] reference The reference rectangle.
	/// @param[in] threshold The threshold of the criterion.
	/// @param[in] felzenszwalb Use Felzenszwalb's criterion instead (area of intersection over area
	/// of second rectangle). Useful to remove small detections inside bigger ones.
	Intersector(CvRect reference, double threshold = 0.5, bool felzenszwalb = false) :
	reference_(reference), threshold_(threshold), felzenszwalb_(felzenszwalb)
	{
	}

	/// Tests for the intersection between a given rectangle and the reference.
	/// @param[in] rect The rectangle to intersect with the reference.
	/// @param[out] score The score of the intersection.
	bool operator()(CvRect rect) const
	{
		const int left = std::max(reference_.x, rect.x);
		const int right = std::min(reference_.x + reference_.width, rect.x + rect.width);

		if (right < left)
			return false;

		const int top = std::max(reference_.y, rect.y);
        const int bottom = std::min(reference_.y + reference_.height, rect.y + rect.height);

		if (bottom < top)
			return false;

		const int intersectionArea = (right - left) * (bottom - top);
		const int rectArea = rect.height * rect.width;//rect.area();
        const int refArea = reference_.width * reference_.height;//reference_.area();

		/*if (true)
        {
            if (intersectionArea >= rectArea * threshold_ ||
                intersectionArea >= refArea * threshold_)
                return true;
        }
        else */if (felzenszwalb_)
        {
			if (intersectionArea >= rectArea * threshold_)
				return true;
		}
		else
        {
			const int unionArea = refArea + rectArea - intersectionArea;

			if (intersectionArea >= unionArea * threshold_)
				return true;
		}

		return false;
	}

private:
	CvRect reference_;
	double threshold_;
	bool felzenszwalb_;
};

inline bool lhsIncludeRhs(const zftdt::Detection& lhs, const zftdt::Detection& rhs, double ratioBigSmall, double ratioIntersectSmall)
{
    CvRect lrect = lhs, rrect = rhs;
    CvRect intersect = rectOverlap(lrect,rrect);//lrect & rrect;
    int larea = lrect.width * lrect.height;//lrect.area();
	int rarea = rrect.width * rrect.height;//rrect.area();
	int iarea = intersect.height * intersect.width;
    if (larea > ratioBigSmall * rarea && iarea > ratioIntersectSmall * rarea)//intersect.area()
        return true;
    return false;
}
static void removeIncluding(zftdt::DPMVector<zftdt::Detection>& detections)
{
	static zftdt::Detection tmp;
	int removeNo = 0;
	for (int i = 0; i < detections.size; i++)
    {
        for (int j = 0; j < detections.size; j++)
        {
            if(j == i) continue;
			if (lhsIncludeRhs(detections[i], detections[j], 2, 0.8))//must compare i->j & j->i
            {
				//exchange j and zhe last unremoved element
				if(i != detections.size - 1)
				{
					tmp = detections[detections.size - 1];
					detections[detections.size - 1] = detections[i];
					detections[i] = tmp;
				}
				//decrease element size
				detections.size--;
				i--;//current swapped, back trace this
                break;
            }
        }
	}
}

static void sortDetectionDescend(zftdt::DPMVector<zftdt::Detection>& detections)
{
	static zftdt::Detection tmp;
	int maxIndex = 0;
	//sometimes size equals 0
	if(detections.size <= 0)
		return;
	for(int i = 0; i < detections.size - 1; i++)
	{
		maxIndex = i;
		for(int j = i + 1; j < detections.size; j++)
		{
			if(detections[j] < detections[maxIndex])//sort ascend by score but "<"
			{
				maxIndex = j;
			}
		}
		//exchange max with first
		if(maxIndex != i)
		{
			tmp = detections[i];
			detections[i] = detections[maxIndex];
			detections[maxIndex] = tmp;
		}
	}
}

static void removeDetection(zftdt::DPMVector<zftdt::Detection>& detections,float overlap)
{
	static zftdt::Detection tmpD;
	//record eliminated index of detections, avoid object moving
	static zftdt::DPMVector<int> index(DPM_MAX_MAXIA);
	int vecSize = detections.size;
	int removeNumForeach = 0;
	index.size = vecSize;
	for(int i = 0; i < vecSize; i++)
	{
		index[i] = i;
	}
	for(int i = 0; i < vecSize; i++)
	{
		Intersector tmp(detections[index[i]], overlap, true);
		removeNumForeach = 0;
		for(int j = i + 1; j < vecSize; j++)
		{
			index[j - removeNumForeach] = index[j];
			if(tmp.operator()(detections[index[j]]))
			{
				removeNumForeach++;
				//remove j element->push to end--->error:can't exchange the end to the current, it's already sorted
				//tmpD = detections[j];
				//detections[j] = detections[detections.size - 1];
				//detections[detections.size - 1] = tmpD;
				//detections.size--;
				//j--;//back to j
			}

		}
		vecSize -= removeNumForeach;
	}
	//prepare removed result
	for(int i = 1; i < vecSize; i++)
	{
		if(i != index[i])
		{
			tmpD = detections[index[i]];
			detections[index[i]] = detections[i];
			detections[i] = tmpD;
		}
	}
	detections.size = vecSize;
}

namespace
{
struct Maximum
{
    Maximum(void) : val(0.0f) {};
    Maximum(const CvPoint& pos_, float val_) : pos(pos_), val(val_) {};
    bool operator <(const Maximum& other) const
    {
    	return val < other.val;
    }
    Maximum operator =(const Maximum& other)
    {
    	val = other.val;
    	pos = other.pos;
    	return *this;
    }
//private:
    CvPoint pos;
    float val;
};

inline void localMaxima(const IplImage* src, Maximum *maxima, const CvSize& win, float thresh)
{
	//maxima->1D vector of 2D
	//maxima.clear();
	int size = 0;

    if (!src->imageData)
        return;
    if (src->depth != IPL_DEPTH_32F || src->nChannels != 1)
        return;
    if (win.width < 3 || win.height < 3)
        return;
    if (win.width % 2 == 0 || win.height % 2 == 0)
        return;
    if (win.width > src->width || win.height > src->height)
        return;
    int cols = src->width, rows = src->height;
    int winHalfWidth = win.width / 2, winHalfHeight = win.height / 2;
    //cv::Mat mark(rows, cols, CV_8UC1);
	IplImage *mark = cvCreateImage(cvSize(cols, rows), IPL_DEPTH_8U, 1);
    memset(mark->imageData, 0, mark->imageSize);

    //std::vector<Maximum> initMaxima;
    for (int i = 0; i < rows; i++)
    {
		const float* ptrSrc = (float*)(src->imageData + i * src->widthStep);
        const unsigned char* ptrMark = (unsigned char*)(mark->imageData + i * mark->widthStep);//mark.ptr<unsigned char>(i);
        int top = (i > winHalfHeight) ? -winHalfHeight : -i;
        int bottom = (i + winHalfHeight < rows) ? winHalfHeight + 1 : rows - i;
        int actualWinHeight = bottom - top;
        for (int j = 0; j < cols; j++)
        {
            if (ptrMark[j]) continue;
            float val = ptrSrc[j];
            int count = 0;
            int left = (j > winHalfWidth) ? -winHalfWidth : -j;
            int right = (j + winHalfWidth < cols) ? winHalfWidth + 1 : cols - j;
            int actualWinWidth = right - left;
            for (int u = top; u < bottom; u++)
            {
                const float* ptrCheckSrc = (float*)(src->imageData + (i + u) * src->widthStep);
                for (int v = left; v < right; v++)
                {
                    if (val > ptrCheckSrc[j + v])
                        count++;
                }
            }
            if (count == actualWinHeight * actualWinWidth - 1)
            {
                if (val > thresh)
				{
                    //maxima.push_back(Maximum(cv::Point(j, i), val));
					*(maxima + size) = Maximum(cvPoint(j, i), val);
					size++;
				}
                //initMaxima.push_back(Maximum(cv::Point(j, i), val));
				unsigned char* ptrMarkj = (unsigned char*)(mark->imageData + i * mark->widthStep + j + left);//mark.ptr<unsigned char>(i + u) + j + left
                for (int u = top; u < bottom; u++)
                {
                    //memset(mark.ptr<unsigned char>(i + u) + j + left, 0XFF, actualWinWidth * sizeof(unsigned char));
					memset(ptrMarkj + u * mark->widthStep, 0XFF, actualWinWidth * sizeof(unsigned char));
                }
            }
        }
    }
	maxima[size].val = 0.5f;//add to figure out its size

	cvReleaseImage(&mark);
	assert(size < DPM_MAX_MAXIA);
}

}



void zftdt::detectFast(const Mixture & mixture, int width, int height, const HOGPyramid & pyramid,
    double rootThreshold, double fullThreshold, double overlap, zftdt::DPMVector<Detection> & detections)
{
	//detections.clear();
	detections.size = 0;

    if (pyramid.isEmpty())
        return;

	zftdt::DPMVector<IplImage*> scores = g_DPM_memory->mem_GetScores(pyramid, mixture);
	zftdt::DPMVector<IplImage*> argmaxes = g_DPM_memory->mem_GetArgmaxes(pyramid, mixture);

    mixture.getRootLevelsRootScores(pyramid, scores, argmaxes);

    //std::vector<std::vector<Maximum> > maxima(scores.size);
	//2D-Maximum,每一维的数量不同，通过后置的val值判断
	static zftdt::DPM2DVector<Maximum> maxima(DPM_PYRAMID_MAX_LEVELS + 1,DPM_MAX_MAXIA + 1);
	//initialize maxima as DPM_MAX_MAXIA * DPM_PYRAMID_MAX_LEVELS numbers of elements
	maxima.xSize = DPM_MAX_MAXIA;
	maxima.ySize = DPM_PYRAMID_MAX_LEVELS;

    //const cv::Rect fullRect(0, 0, width, height);
	const CvRect fullRect = cvRect(0, 0, width, height);
    for (int i = pyramid.interval; i < scores.size; i++)
    {

		//cv::FileStorage pScores("scores.xml", cv::FileStorage::WRITE);
		//pScores << "scores" << scores[i];
		//pScores.release();
        localMaxima(scores[i], &(maxima.at(i,0)), cvSize(3, 3), rootThreshold);
        double scale = pow(2.0, static_cast<double>(i) / pyramid.interval + 2.0);
        for (int j = 0; j < maxima.xSize; j++)
        {

			const Maximum &maximaIJ = maxima.at(i,j);
			if(maximaIJ.val < rootThreshold)
				break;
			//std::vector<cv::Point> partPoints;
			static zftdt::DPMVector<CvPoint> partPoints(DPM_MAX_MODEL_PARTS + 1);
			partPoints.size = 0;

			const CvPoint &posT = maximaIJ.pos;
			int modelIndex = *((int*)(argmaxes[i]->imageData + posT.y * argmaxes[i]->widthStep) + posT.x);//argmaxes[i].at<int>(maximaIJ.pos);
            float partScore =
                mixture.models[modelIndex].findBestParts(pyramid, i, maximaIJ.pos, partPoints);
            float totalScore = maximaIJ.val + partScore;
            if (totalScore < fullThreshold)
                continue;
			CvRect rootConvRect = cvRect(maximaIJ.pos.x, maximaIJ.pos.y,
                        mixture.models[modelIndex].parts[0].filter->width / NbFeatures,
                        mixture.models[modelIndex].parts[0].filter->height);
            CvRect rootImageRect = cvRect((rootConvRect.x - pyramid.padx) * scale + 0.5,
                                   (rootConvRect.y - pyramid.pady) * scale + 0.5,
                                   rootConvRect.width * scale + 0.5,
                                   rootConvRect.height * scale + 0.5);
            rootImageRect = rectOverlap(rootImageRect, fullRect);//&= fullRect;
            //printf("index = %d, level = %d, rect = (%d, %d, %d, %d), rootScore = %f\n",
            //    modelIndex, i, rootImageRect.x, rootImageRect.y, rootImageRect.width, rootImageRect.height, maxima[i][j].val);

//			static Detection tmpD;
//			tmpD.assign(modelIndex, totalScore, i, rootConvRect, rootImageRect);
			//detections.push_back(tmpD);//TODO3.1:push_back->no operator "=" matches these operands
			detections.size++;
			assert(detections.size <= detections.maxNum);
			detections[detections.size - 1].assign(modelIndex, totalScore, i, rootConvRect, rootImageRect);

            double partScale = pow(2.0, static_cast<double>(i - pyramid.interval) / pyramid.interval + 2.0);
            for (int k = 0; k < partPoints.size; k++)
            {

                CvRect partConvRect = cvRect(partPoints[k].x, partPoints[k].y,
                                      mixture.models[modelIndex].parts[k + 1].filter->width / NbFeatures,
                                      mixture.models[modelIndex].parts[k + 1].filter->height);

                CvRect partImageRect = cvRect((partConvRect.x - pyramid.padx) * partScale + 0.5,
                                       (partConvRect.y - pyramid.pady) * partScale + 0.5,
                                       partConvRect.width * partScale + 0.5,
                                       partConvRect.height * partScale + 0.5);
                //partImageRect &= fullRect;
				partImageRect = rectOverlap(partImageRect, fullRect);

				zftdt::DPMVector<CvRect>& tLevel = detections[detections.size - 1].levelRects;
				zftdt::DPMVector<CvRect>& tImage = detections[detections.size - 1].imageRects;
				//
//				tLevel.size++;
//				tImage.size++;
//				assert(tLevel.size <= tLevel.maxNum && tImage.size <= tImage.maxNum);
//				tLevel[tLevel.size - 1] = partConvRect;
//				tImage[tImage.size - 1] = partImageRect;
				tLevel.push_back(partConvRect);
				tImage.push_back(partImageRect);
            }
        }
    }
    removeIncluding(detections);

    // Non maxima suppression
	//sort(detections.begin(), detections.end());
	sortDetectionDescend(detections);
	//
	//for (int i = 1; i < detections.size(); ++i)
	//	detections.resize(remove_if(detections.begin() + i, detections.end(),
 //                         Intersector(detections[i - 1], overlap, true)) -
	//					  detections.begin());
	removeDetection(detections,overlap);
    return;
}

int fw_matrix_32FC1(const CvMat *src, const char* name)
{
	FILE *pfile = fopen(name,"wb+");
	//assert(src->depth == 32 && src->nChannels == 1);
	assert(CV_MAT_TYPE(src->type) == CV_32FC1);
	for(int i = 0; i < src->rows; i++)
	{
		float* ptr = (float*)(src->data.ptr + i * src->step);
		for(int j = 0; j < src->cols; j++)
		{
			fprintf(pfile, "%.5f ", ptr[j]);
		}
		fprintf(pfile, "\n");
	}
	fclose(pfile);
	return src->rows * src->cols * CV_ELEM_SIZE(src->type);
}

int fw_matrix_8UC3(const IplImage *src, const char* name)
{
	FILE *pfile = fopen(name,"wb+");
	assert(src->depth == 8 && src->nChannels == 3);
	for(int i = 0; i < src->height; i++)
	{
		unsigned char* ptr = (unsigned char*)(src->imageData + i * src->widthStep);
		for(int j = 0; j < src->width; j++)
		{
			fprintf(pfile, "%03d ", ptr[j]);
		}
		fprintf(pfile, "\n");
	}
	fclose(pfile);
	return src->height * src->width * (src->depth << 3) * src->nChannels;
}

int copyMatWithRoi(const CvMat *src, CvMat *dst, const CvRect &sRoi, const CvRect &dRoi)
{
	if(sRoi.width != dRoi.width || sRoi.height != dRoi.height)
	{
		printf("[%s]Line %d:roi not equal.\n",__FUNCTION__,__LINE__);
		return -1;
	}
	if(sRoi.width + sRoi.x > src->cols || dRoi.height + dRoi.y > dst->rows)
	{
		printf("[%s]Line %d:roi exceed mat size.\n",__FUNCTION__,__LINE__);
		return -1;
	}

	if(src->type != dst->type)
	{
		printf("[%s]Line %d:type not match.\n",__FUNCTION__,__LINE__);
		return -1;
	}
	const int pixSize = CV_ELEM_SIZE(src->type);
	for(int i = 0; i < sRoi.height; i++)
	{
		void *ptrS = src->data.ptr + (i + sRoi.y) * src->step + pixSize * sRoi.x;
		void *ptrD = dst->data.ptr + (i + dRoi.y) * dst->step + pixSize * dRoi.x;
		memcpy(ptrD,ptrS,sRoi.width * pixSize);
	}
	return 0;
}



