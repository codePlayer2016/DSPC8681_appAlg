#include "DPM.h"
#include <math.h>
#include <limits.h>
#include <float.h>
#include <algorithm>
#include <fstream>
#include <assert.h>
#include <iomanip>
// add by LHS
//#include <stdio>
//#include <limits.h>
//#include <mmintrin.h>//MMX
//#include <xmmintrin.h>//SSE
//#include <emmintrin.h>//SSE2


// add by LHS
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include "ti/platform/platform.h"
extern void write_uart(char* msg);

#ifdef __cplusplus
}
#endif
extern char debugInfor[100];

extern std::ofstream foutDebug;


zftdt::Model::Model() :
		maxSizeOfFilters(cvSize(0, 0)), bias(0), parts(DPM_MAX_MODEL_PARTS + 1)
{
	//parts.maxNum = DPM_MAX_MODEL_PARTS;
	//parts.size = 0;
	//parts.ptr = new Part[DPM_MAX_MODEL_PARTS]();
	//assert(parts.ptr != NULL);
	//parts = DPMVector<Part>(DPM_MAX_MODEL_PARTS);
	parts.size = parts.maxNum;
	for (int i = 0; i < parts.size; i++)
	{
		parts[i].filter = NULL;
	}
	parts.size = 0;
}

zftdt::Model::~Model()
{
	//if(parts.ptr != NULL)
	//{
	//	delete[] parts.ptr;
	//	parts.ptr = NULL;
	//}
	for (int i = 0; i < parts.size; i++)
	{
		if (parts[i].filter != NULL)
		{
			cvReleaseMat(&(parts[i].filter));
			parts[i].filter = NULL;
		}
	}
}

//zftdt::Model::Model(int maxParts)
//	:maxSizeOfFilters(0,0),bias(0)
//{
//assert(maxParts > 0);
//parts.maxNum = maxParts;
//parts.size = 0;
//parts.ptr = new Part[maxParts]();
//assert(parts.ptr != NULL);
//parts = DPMVector<Part>(maxParts);
//}

float zftdt::Model::findBestParts(const HOGPyramid& pyramid, int rootLevel,
		const CvPoint& rootTopLeft, zftdt::DPMVector<CvPoint>& partLocs) const
{
	//partLocs.clear();
	partLocs.size = 0;

	int numLevels = pyramid.levels.size;
	int interval = pyramid.interval;
	if (rootLevel < interval || rootLevel >= numLevels
			|| !pyramid.levels[rootLevel]
			|| !pyramid.levels[rootLevel - interval])
		//!pyramid.levels[rootLevel]->data.ptr || !pyramid.levels[rootLevel - interval]->data.ptr)
		return -FLT_MAX;	//-std::numeric_limits<float>::infinity();

	const CvMat* level = pyramid.levels[rootLevel - interval];
	CvRect levelRect = cvRect(0, 0, level->cols / NbFeatures, level->rows);

	CvPoint partOrigin = cvPoint(2 * rootTopLeft.x - pyramid.padx,
			2 * rootTopLeft.y - pyramid.pady);
	int halfWinSize = Part::halfSize;
	int winSize = 2 * halfWinSize + 1;
	float score = 0;
	for (int p = 1; p < parts.size; p++)
	{
		const Part& part = parts[p];
		CvPoint partTopLeft = cvPoint(partOrigin.x + part.offset.x,
				partOrigin.y + part.offset.y);
		CvRect searchRect = cvRect(partTopLeft.x - halfWinSize,
				partTopLeft.y - halfWinSize, winSize, winSize);
		CvPoint actualRectTopLeft;
		//cv::Mat conv;
		CvMat *conv = NULL;
		//int64 beg = cv::getTickCount();

		::convolve(level, part.filter, &conv, searchRect, actualRectTopLeft,
				NbFeatures - 1, NbFeatures);
		if (conv == NULL)
			return -FLT_MAX;	//-std::numeric_limits<float>::infinity();
		//int64 end = cv::getTickCount();
		//float timeDetect = 1000 * (end - beg) / cv::getTickFrequency();
		//foutDebug << timeDetect << std::endl;

		CvPoint offset = cvPoint(actualRectTopLeft.x - searchRect.x,
				actualRectTopLeft.y - searchRect.y);//actualRectTopLeft - searchRect.tl();
		float maxVal = -FLT_MAX;	//-std::numeric_limits<float>::infinity();
		CvPoint maxLoc;
		for (int i = 0; i < conv->height; i++)
		{
			float* ptrConv = (float*) (conv->data.ptr + i * conv->step);//conv.ptr<float>(i);
			for (int j = 0; j < conv->width; j++)
			{
				float val = ptrConv[j] + part.cost[i + offset.y][j + offset.x];
				if (val > maxVal)
				{
					maxVal = val;
					maxLoc = cvPoint(j, i);
				}
			}
		}
		cvReleaseMat(&conv);

		score += maxVal;
		//maxLoc += offset;
		maxLoc.x += offset.x;
		maxLoc.y += offset.y;
		//maxLoc += partTopLeft;
		maxLoc.x += partTopLeft.x;
		maxLoc.y += partTopLeft.y;

		maxLoc.x -= halfWinSize;
		maxLoc.y -= halfWinSize;
//		partLocs.size++;
//		assert(partLocs.size < partLocs.maxNum);
//		partLocs[partLocs.size - 1] = maxLoc;
		partLocs.push_back(maxLoc);
	}
	score += bias;
	return score;
}

//void zftdt::Model::convolveRootFilterAllLevels(const HOGPyramid& pyramid, std::vector<cv::Mat>& convolutions) const
//{
//	pyramid.convolve(parts[0].filter, convolutions);
//}

std::ostream & zftdt::operator<<(std::ostream & os, const Model & model)
{

	// Save the number of parts and the bias
	os << model.parts.size << ' ' << model.bias << std::endl;

	// Save the parts themselves
	for (int i = 0; i < model.parts.size; ++i)
	{

		os << model.parts[i].filter->height << ' '
				<< model.parts[i].filter->width / NbFeatures << ' '
				<< NbFeatures << ' ' << model.parts[i].offset.x << ' '
				<< model.parts[i].offset.y << ' '
				<< model.parts[i].deformation[0] << ' '
				<< model.parts[i].deformation[1] << ' '
				<< model.parts[i].deformation[2] << ' '
				<< model.parts[i].deformation[3] << std::endl;

		int rows = model.parts[i].filter->height;
		int cols = model.parts[i].filter->width / NbFeatures;
		unsigned char* pBaseAddr = model.parts[i].filter->data.ptr;
		int widthStep = model.parts[i].filter->step;
		for (int y = 0; y < rows; ++y)
		{
			const float* ptrRow = (float*) (pBaseAddr + y * widthStep);
			for (int x = 0; x < cols; ++x)
			{
				const float* ptrCurrPos = ptrRow + x * NbFeatures;
				for (int j = 0; j < NbFeatures; ++j)
					os << ptrCurrPos[j] << ' ';
			}
			os << std::endl;
		}
	}
	return os;
}

std::istream & zftdt::operator>>(std::istream & is, Model & model)
{
	int nbParts;
	float bias;
	is >> nbParts >> bias;


	if (!is)
	{
		model = Model();
		return is;
	}
	assert(nbParts <= DPM_MAX_MODEL_PARTS);
	model.parts.size = nbParts;
	model.bias = bias;

	// add by LHS.
	//printf("nbParts=%d,first loop\n",nbParts);
	//std::cout<<"nbPart="<<nbParts<<" first loop"<<std::endl;
	//sprintf(debugInfor,"nbParts=%d\r\n",nbParts);
	//write_uart(debugInfor);

	for (int i = 0; i < nbParts; ++i)
	{
		int rows, cols, nbFeatures;

		is >> rows >> cols >> nbFeatures >> model.parts[i].offset.x
				>> model.parts[i].offset.y >> model.parts[i].deformation[0]
				>> model.parts[i].deformation[1]
				>> model.parts[i].deformation[2]
				>> model.parts[i].deformation[3];

#if OPT_LEVEL_MAXFLTSIZE		
		//record max size of filters
		model.maxSizeOfFilters.height = std::max(model.maxSizeOfFilters.height,
				rows);
		model.maxSizeOfFilters.width = std::max(model.maxSizeOfFilters.width,
				cols);
#endif
		// Always set the deformation of the root to zero
		if (!i)
		{
			memset(model.parts[0].deformation, 0,
					sizeof(model.parts[0].deformation));
		}
		model.parts[i].filter = cvCreateMat(rows, cols * NbFeatures, CV_32FC1);
		assert(model.parts[i].filter != NULL);
		memset(model.parts[i].filter->data.ptr, 0,
				rows * cols * NbFeatures * sizeof(float));
		unsigned char* pBaseAddr = model.parts[i].filter->data.ptr;
		int widthStep = model.parts[i].filter->step;

		// add by LHS.
		//printf("rows=%d,cols=%d,nbFeatures=%d\n",rows,cols,nbFeatures);
		//std::cout<<"rows="<<rows<<" cols="<<cols<<" nbFeatures="<<nbFeatures<<std::endl;
		//sprintf(debugInfor,"rows=%d,cols=%d,nbFeatures=%d\r\n",rows,cols,nbFeatures);
		//write_uart(debugInfor);

		for (int y = 0; y < rows; ++y)
		{
			float* ptrRow = (float*) (pBaseAddr + y * widthStep);
			for (int x = 0; x < cols; ++x)
			{
				float* ptrCurrPos = ptrRow + x * NbFeatures;
				for (int j = 0; j < nbFeatures; ++j)
				{
					//float f;
					//is >> f;
					//if (j < NbFeatures)
					//	ptrCurrPos[j] = f;
					is >> ptrCurrPos[j];
				}
			}
		}
		float* deformation = model.parts[i].deformation;
		for (int y = Model::Part::halfSize; y >= -Model::Part::halfSize; y--)
		{
			for (int x = Model::Part::halfSize; x >= -Model::Part::halfSize;
					x--)
			{
				model.parts[i].cost[-y + Model::Part::halfSize][-x
						+ Model::Part::halfSize] = x * x * deformation[0]
						+ x * deformation[1] + y * y * deformation[2]
						+ y * deformation[3];
			}
		}
	}
	if (!is)
		model = Model();

	return is;
}

