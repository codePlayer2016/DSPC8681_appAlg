#include "DPM.h"

zftdt::MemAllocDPM *zftdt::g_DPM_memory = NULL;

zftdt::MemAllocDPM::MemAllocDPM()
	:scores(0),argmaxes(0), detections(DPM_MAX_MAXIA)
{

}

zftdt::MemAllocDPM::~MemAllocDPM()
{
	for (int i = 1; i < scores.size; i++)
    {
		if(scores[i])
		{
			cvReleaseImage(&(scores[i]));
			scores[i] = NULL;
		}
	}
	for (int i = 1; i < argmaxes.size; i++)
    {
		if(argmaxes[i])
		{
			cvReleaseImage(&(argmaxes[i]));
			argmaxes[i] = NULL;
		}
	}
}

zftdt::DPMVector<IplImage *>& zftdt::MemAllocDPM::mem_GetScores(const zftdt::HOGPyramid& pyramid, const zftdt::Mixture& mixture)
{
	static bool initState = false;
	if(initState)
	{
		return scores;
	}
	else
	{
		const int nbModels = mixture.models.size;
		const int nbLevels = pyramid.levels.size;

		scores.reAllocate(nbLevels);
	
		scores.size = nbLevels;
		int maxRootCols = mixture.models[0].parts[0].filter->width;
		int maxRootRows = mixture.models[0].parts[0].filter->height;
		for (int i = 1; i < nbModels; i++)
		{
			maxRootCols = std::max(maxRootCols, mixture.models[i].parts[0].filter->width);
			maxRootRows = std::max(maxRootRows, mixture.models[i].parts[0].filter->height);
		}
		maxRootCols /= NbFeatures;
		for (int i = 1; i < nbLevels; i++)
		{
			scores[i] = NULL;
		}
		for (int i = pyramid.interval; i < nbLevels; i++) 
		{
			//alloc
			scores[i] = cvCreateImage(cvSize(pyramid.levels[i]->width / NbFeatures - maxRootCols + 1, 
				pyramid.levels[i]->height - maxRootRows + 1), IPL_DEPTH_32F, 1);
			assert(scores[i] != NULL);
		}
		initState = true;
	}
	return scores;
}
zftdt::DPMVector<IplImage *>& zftdt::MemAllocDPM::mem_GetArgmaxes(const zftdt::HOGPyramid& pyramid, const zftdt::Mixture& mixture)
{
	static bool initState = false;
	if(initState)
	{
		return argmaxes;
	}
	else
	{
		const int nbModels = mixture.models.size;
		const int nbLevels = pyramid.levels.size;
	
		argmaxes.reAllocate(nbLevels);

		argmaxes.size = nbLevels;
		int maxRootCols = mixture.models[0].parts[0].filter->width;
		int maxRootRows = mixture.models[0].parts[0].filter->height;
		for (int i = 1; i < nbModels; i++)
		{
			maxRootCols = std::max(maxRootCols, mixture.models[i].parts[0].filter->width);
			maxRootRows = std::max(maxRootRows, mixture.models[i].parts[0].filter->height);
		}
		maxRootCols /= NbFeatures;
		for (int i = 1; i < nbLevels; i++)
		{
			argmaxes[i] = NULL;
		}
		for (int i = pyramid.interval; i < nbLevels; i++) 
		{
			//alloc
			argmaxes[i] = cvCreateImage(cvSize(scores[i]->width, scores[i]->height), IPL_DEPTH_32S, 1);
			assert(argmaxes[i] != NULL);
		}
		initState = true;
	}
	return argmaxes;
}

zftdt::DPMVector<zftdt::Detection>& zftdt::MemAllocDPM::mem_GetDetections(void)
{
	return detections;
}
