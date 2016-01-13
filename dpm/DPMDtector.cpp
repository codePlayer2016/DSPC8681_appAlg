/*
 * DPMDtector.cpp
 *
 *  Created on: 2015-3-13
 *      Author: LinZW
 */

#include "HOG.h"
#include "DPM.h"
#include <time.h>
#include <fstream>
#include <sstream>


static void output(const IplImage* image,
		const zftdt::DPMVector<zftdt::Detection>& detections,
		zftdt::DPMVector<zftdt::Result>& results)
{
	int numDet = detections.size;
	results.size = numDet;
	for (int i = 0; i < numDet; i++)
	{
		results[i].index = detections[i].index;
		results[i].level = detections[i].level;
		results[i].score = detections[i].score;
		results[i].rects = detections[i].imageRects;
	}
}
namespace zftdt
{
DeformablePartModel::DeformablePartModel(const std::string& paramPath)
{
	mixture = new Mixture();
#if 0 // read the paraFile.
	std::ifstream mfstrm;
	mfstrm.open(paramPath.c_str(), std::ios_base::binary);
	if(!mfstrm)
	{
		std::cout << paramPath << "open failed." << std::endl;
		//system("pause");
		exit(-1);
	}
	mfstrm >> *mixture;
	mfstrm.close();
#endif
#if 0 // get the part file.
	int mfstrLength=0;
	mfstrm.seekg(0,std::ios::end);
	mfstrLength=mfstrm.tellg();
	std::cout<<" mfstrLeng= "<<mfstrLength<<std::endl;
	mfstrm.seekg(0,std::ios::beg);

	std::ofstream testFile;
	testFile.open("sramTest.txt",std::ios_base::binary);

	//char charBuffer[1024*20];
	//int readBufferLength=0;
	float paraValue=0;
	int arrayLength=0;
	int arrayNum=0;

	testFile<<std::ios::fixed;
	while(!mfstrm.eof())
	{
		// begin.
		if(arrayLength==0)
		{
			testFile<<"const char *pMotorParam"<<arrayNum<<' '<<'='<<' '<<'"';
		}
		mfstrm>>paraValue;
		testFile<<paraValue<<' ';
		arrayLength++;
		if(arrayLength==(1024*20)/10)
		{
			// end.
			testFile<<'"'<<';'<<std::endl;
			arrayLength=0;
			arrayNum++;
		}
		else
		{}
	}
	testFile.close();
#endif
#if 0 // test the part file.
	std::string stringTest;
	std::string  str0(pMotorParam0);
	std::string  str1(pMotorParam1);
	std::string str2(pMotorParam2);
	std::string str3(pMotorParam3);
	std::string str4(pMotorParam4);
	std::string str5(pMotorParam5);
	std::string str6(pMotorParam6);
	std::string str7(pMotorParam7);
	std::string str8(pMotorParam8);
	std::string str9(pMotorParam9);
	std::string str10(pMotorParam10);
	std::string str11(pMotorParam11);
	stringTest=str0+str1+str2+str3+str4+str5+str6+str7+str8+str9+str10+str11;
	std::stringstream streamTest(stringTest);
	float srcValue=0.0;
	float destValue=0.0;
	int valueIndex=0;
	int checkNum=0;
	while(!mfstrm.eof())
	{
		mfstrm>>srcValue;
		streamTest>>destValue;
		checkNum++;
		if(srcValue!=destValue)
		{
			std::cout<<"srcValue:"<<srcValue<<" "<<"destValue:"<<destValue<<" "<<"valueIndex"<<valueIndex<<std::endl;
			//System("pause");
		}

	}
	std::cout<<checkNum<<std::endl;

#endif

	std::stringstream mfstrm;
	mfstrm<<std::ios_base::binary;
	mfstrm.str(paramPath);
	mfstrm >> *mixture;
}

bool DeformablePartModel::isValid(void) const
{

	if (mixture->models.size == 0)
		return false;
	int numModels = mixture->models.size;
	for (int i = 0; i < numModels; i++)
	{
		if (mixture->models[i].parts.size < 1)
			return false;
	}
	return true;
}

bool DeformablePartModel::detectFast(const IplImage* image, int minSideLen,
		int maxNumLevels, const HOGPyramid& pyramid, double rootThresh,
		double fullThresh, double overlap,
		zftdt::DPMVector<Result>& results) const
{
	if (!isValid())
		return false;

	//std::vector<Detection> detections;
	//static zftdt::DPMVector<Detection> detections(DPM_MAX_MAXIA);
	zftdt::DPMVector<zftdt::Detection> detections =
			g_DPM_memory->mem_GetDetections();
	detections.size = 0;
//    HOGPyramid pyramid;
//#if OPT_LEVEL_MAXFLTSIZE
//    cv::Size size = mixture->maxSizeOfFilters;
//#else
//	cv::Size size = mixture->getMaxSizeOfFilters();
//#endif
//    int maxFilterSideLen = std::max(size.width, size.height);
//    pyramid.build(image, padx, pady, interval, std::max(maxFilterSideLen * DPM_HOG_CELLSIZE, minSideLen), maxNumLevels);//
	if (pyramid.levels.size <= pyramid.interval)
		return false;
	zftdt::detectFast(*mixture, image->width, image->height, pyramid,
			rootThresh, fullThresh, overlap, detections);
	output(image, detections, results);
	return (results.size != 0);
}

CvSize DeformablePartModel::getMaxSizeOfFilters(void) const
{
	return mixture->maxSizeOfFilters;
}

}

