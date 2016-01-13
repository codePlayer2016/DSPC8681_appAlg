#include "DPM.h"
#include <math.h>
#include <limits.h>
#include <assert.h>
#include <algorithm>
#include <fstream>

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


zftdt::Mixture::Mixture() :
		maxSizeOfFilters(cvSize(0, 0)), models(DPM_MAX_MODELS + 1)
{

}
zftdt::Mixture::~Mixture()
{

}

void zftdt::Mixture::getRootLevelsRootScores(const HOGPyramid& pyramid,
		zftdt::DPMVector<IplImage*>& scores,
		zftdt::DPMVector<IplImage*>& argMaxes) const
{
	const int nbModels = models.size;
	const int nbLevels = pyramid.levels.size;

	// Convolve with all the models

	assert( nbModels <= DPM_MAX_MODELS);
	assert( nbLevels <= DPM_PYRAMID_MAX_LEVELS);

	const DPM2DVector<CvMat *>& tmp = ConvMixtureFastHelper::run(pyramid,
			*this);

	// Resize the scores and argmaxes

	for (int i = pyramid.interval; i < nbLevels; ++i)
	{
		if (!pyramid.levels[i]
				|| !pyramid.levels[i - pyramid.interval]/* ||
				 pyramid.levels[i].rows < maxRootRows || pyramid.levels[i].cols / NbFeatures < maxRootCols*/)
			continue;

		bool allowRun = true;
		for (int j = 0; j < nbModels; j++)
		{
			if (tmp.at(j, i) == NULL) // if(tmp.at(j,i)->imageData != NULL)
			{
				allowRun = false;
				break;
			}
		}
		if (!allowRun)
			continue;

		for (int y = 0; y < scores[i]->height; ++y)
		{
			float *pScores = (float*) (scores[i]->imageData
					+ y * scores[i]->widthStep);
			int *pArgMaxes = (int*) (argMaxes[i]->imageData
					+ y * argMaxes[i]->widthStep);
			for (int x = 0; x < scores[i]->width; ++x)
			{
				int argmax = 0;
				CvMat *imageJI = NULL, *imageXI = NULL;
				float dataJI, dataXI;
				imageXI = tmp.at(argmax, i);
				dataXI =
						*((float*) (imageXI->data.ptr + y * imageXI->step) + x);
				for (int j = 1; j < nbModels; ++j)
				{
					imageJI = tmp.at(j, i);
					dataJI = *((float*) (imageJI->data.ptr + y * imageJI->step)
							+ x);
					if (dataJI > dataXI)
						argmax = j;
				}
				//argmax might be changed
				imageXI = tmp.at(argmax, i);
				dataXI =
						*((float*) (imageXI->data.ptr + y * imageXI->step) + x);
				*(pScores + x) = dataXI; //tmp.at(argmax,i).at<float>(y, x);
				*(pArgMaxes + x) = argmax;
			}
		}
	}
}

std::ostream & zftdt::operator<<(std::ostream & os, const Mixture & mixture)
{
	// Save the number of models (mixture components)
	os << mixture.models.size << std::endl;

	// Save the models themselves
	for (unsigned int i = 0; i < mixture.models.size; ++i)
		os << mixture.models[i] << std::endl;

	return os;
}

std::istream & zftdt::operator>>(std::istream & is, Mixture & mixture)
{
	int nbModels;
	is >> nbModels;


	if (!is || (nbModels <= 0))
	{
		mixture = Mixture();
		return is;
	}
	assert( nbModels <= DPM_MAX_MODELS);

	mixture.models.size = nbModels;

	// add by LHS.
	//printf("nbModels=%d,is>>mixture.models[] while loop for %d\n",nbModels,nbModels);
	//std::cout<<"nbModels="<<nbModels<<" mixture.models[] while loop for"<<nbModels<<std::endl;
	sprintf(debugInfor,"nbModels=%d\r\n",nbModels);
	write_uart(debugInfor);

	for (int i = 0; i < nbModels; ++i)
	{
		is >> mixture.models[i];
#if OPT_LEVEL_MAXFLTSIZE	
		mixture.maxSizeOfFilters.width = std::max(
				mixture.models[i].maxSizeOfFilters.width,
				mixture.maxSizeOfFilters.width);
		mixture.maxSizeOfFilters.height = std::max(
				mixture.models[i].maxSizeOfFilters.height,
				mixture.maxSizeOfFilters.height);
#endif
		// add by LHS.
		if(!is)
		{
			//std::cout<<"is error"<<std::endl;
			write_uart("is is error\r\n");
		}
		//std::cout<<"parts.size="<<mixture.models[i].parts.size<<std::endl;
		sprintf(debugInfor,"parts.size=%d\r\n",mixture.models[i].parts.size);
		write_uart(debugInfor);

		if (!is || mixture.models[i].parts.size == 0)
		{
			mixture = Mixture();
			return is;
		}
	}

	return is;
}
