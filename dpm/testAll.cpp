/*
 * testAll.cpp
 *
 *  Created on: 2015-8-5
 *      Author: julie
 */

#include "DPM.h"
#include "testAll.h"

#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <iostream>
#include <stdio.h>
#include <time.h>
#include <c6x.h>
#include "DPMDetector.h"
#include "motorcyclist.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "ti/platform/platform.h"
extern void write_uart(char* msg);

#ifdef __cplusplus
}
#endif
extern char debugInfor[100];

using namespace std;
using namespace zftdt;

#define RES_RECORD_EN (1)
#define JPEG_DECODE_EN (1)

#define TIME_INIT (TSCH = 0, TSCL = 0)
#define TIME_READ _itoll(TSCH, TSCL)



int testlib(char *bmpfilebuf)
{
	long long beg, end;
	TIME_INIT;

	long long timeDetectFast;
	int procCount = 0;

	const string prefix = "";


#if 1
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
	const string modelPath=str0+str1+str2+str3+str4+str5+str6+str7+str8+str9+str10+str11;
#endif

	double threshold = -0.6;
	double rootThreshold= 0.9;
	double overlap=	0.4;
	int padx= DPM_HOG_PADX, pady = DPM_HOG_PADY, interval = DPM_PYRAMID_LAMBDA;
	int maxLevels = DPM_PYRAMID_MAX_LEVELS, minSideLen =DPM_PYRAMID_MIN_DETECT_STRIDE;
    IplImage* origImage = cvLoadImageFromArray(bmpfilebuf,0);

    DeformablePartModel model(modelPath); //haoshi


	IplImage *normImage = cvCreateImage(
			cvSize(origImage->width, origImage->height), origImage->depth,
			origImage->nChannels);

	CvSize filterSize = model.getMaxSizeOfFilters();
	HOGPyramid pyramid = HOGPyramid(normImage->width, normImage->height, padx,
			pady, interval, std::max(filterSize.width, filterSize.height));
	//alloc memory
	g_DPM_memory = new MemAllocDPM();
	/**************************************************
	 output root&DPM result
	 ***************************************************/
	procCount++;
	//std::cout << buf << std::endl;
	//foutFastTime << buf << ",";

	cvCopy(origImage, normImage);

	static zftdt::DPMVector<Result> fastResults(DPM_MAX_MAXIA);
	fastResults.size = 0;

	//for (int i = 0; i < 100; ++i)
	for (int i = 0; i < 1; ++i)
	{
		beg = TIME_READ;
		CvSize size = model.getMaxSizeOfFilters();

		int maxFilterSideLen = std::max(size.width, size.height);
		pyramid.build(normImage,
				std::max(maxFilterSideLen * DPM_HOG_CELLSIZE, minSideLen),
				maxLevels); //
		model.detectFast(normImage, minSideLen, maxLevels, pyramid,
				rootThreshold, threshold, overlap, fastResults);
		end = TIME_READ;

		timeDetectFast = (end - beg);
		//std::cout << timeDetectFast << ",";
		//foutFastTime << timeDetectFast << ",";
	}

	//foutDetectFast << buf << ";";
	for (int i = 0; i < fastResults.size; i++)
	{
		//printf("%d,%d,%d,%d\n",fastResults[i].rects[0].x,fastResults[i].rects[0].y,fastResults[i].rects[0].width,fastResults[i].rects[0].height);
#if 0
		foutDetectFast << fastResults[i].rects[0].x << ","
				<< fastResults[i].rects[0].y << ","
				<< fastResults[i].rects[0].width << ","
				<< fastResults[i].rects[0].height << " ";
#endif

		sprintf(debugInfor,"%d,%d,%d,%d\r\n",fastResults[i].rects[0].x,fastResults[i].rects[0].y,fastResults[i].rects[0].width,fastResults[i].rects[0].height);
		write_uart(debugInfor);
	}

	sprintf(debugInfor,"timeDetectFast=%d",timeDetectFast);
	write_uart(debugInfor);
	//foutDetectFast << std::endl;
	//std::cout << std::endl;
	//foutFastTime << std::endl;

	//foutDetectFast.close();
	//foutFastTime.close();

	cvReleaseImage(&normImage);
	cvReleaseImage(&origImage);
	origImage = NULL;
	normImage = NULL;
	delete g_DPM_memory;
	return 0;
}


#if	1
int TestCase_measureDPM()
{
	const char *imagepath ="D:/DPM/LIHUOSHENGgiveChenglonghu/DMP_DSP/test/bmpImage400/101.bmp";
	FILE *ImageFile=fopen(imagepath,"rb");
	if(ImageFile==NULL)
	{
		printf("ERROR: can not open the imagefile\n");
		exit(-1);
	}
    fseek (ImageFile, 0, SEEK_END);
    int ImageFileSize =ftell (ImageFile);
	fseek(ImageFile, 0, SEEK_SET);
    char *ImageFileBuf=(char *)malloc(ImageFileSize);
    int tmp=fread(ImageFileBuf,1,ImageFileSize,ImageFile);
    fclose(ImageFile);
    testlib(ImageFileBuf);

}

#else
int TestCase_measureDPM()
{
	long long beg, end;
	TIME_INIT;

	long long timeDetectFast;
	int procCount = 0;
	const string filePath = "D:/DPM/LIHUOSHENGgiveChenglonghu/DMP_DSP/test/bmpPic1.txt";
	const char *imagepath ="D:/DPM/LIHUOSHENGgiveChenglonghu/DMP_DSP/test/bmpImage400/101.bmp";
	const string prefix = "";
	string buf;
//	fstream ffstrm(filePath.c_str());

	const string modelPath = "D:/DPM/LIHUOSHENGgiveChenglonghu/DMP_DSP/DPM/src/motorcyclist.txt";

//	DeformablePartModel model(modelPath); //haoshi
	double threshold = -0.6,
	rootThreshold= 0.9;
	double overlap=	0.4;
	int padx= DPM_HOG_PADX, pady = DPM_HOG_PADY, interval = DPM_PYRAMID_LAMBDA;
	int maxLevels = DPM_PYRAMID_MAX_LEVELS, minSideLen =DPM_PYRAMID_MIN_DETECT_STRIDE;

	/*************************************************
	 define HOG pyramid to build
	 *************************************************/
#if	JPEG_DECODE_EN
	FILE *ImageFile=fopen(imagepath,"rb");
	if(ImageFile==NULL)
	{
		printf("ERROR: can not open the imagefile\n");
		exit(-1);
	}
    fseek (ImageFile, 0, SEEK_END);
    int ImageFileSize =ftell (ImageFile);
	fseek(ImageFile, 0, SEEK_SET);
    char *ImageFileBuf=(char *)malloc(ImageFileSize);
    int tmp=fread(ImageFileBuf,1,ImageFileSize,ImageFile);
    IplImage* origImage = cvLoadImageFromArray(ImageFileBuf,0);
    fclose(ImageFile);
#else
	fstream ffstrm(filePath.c_str());
	if (!ffstrm.is_open())
	{
		std::cout << "open " + filePath + " failed." << std::endl;
		exit(-1);
	}
	std::getline(ffstrm, buf);
	if (buf.empty())
	{
		std::cout << "file " + filePath + " is empty." << std::endl;
		exit(-1);
	}
	IplImage *origImage = cvLoadImageFromFile(buf.c_str());
	if (origImage == NULL)
	{
		std::cout << "image " << buf << " to initialize HOG is not found."
				<< std::endl;
		exit(-1);
	}
#endif

    DeformablePartModel model(modelPath); //haoshi

	ofstream foutDetectFast("dectectFastRegion.txt");
	ofstream foutFastTime("dectectFastTime.csv");

	//�����ͼ���Ǿ������ŵ�
	IplImage *normImage = cvCreateImage(
			cvSize(origImage->width, origImage->height), origImage->depth,
			origImage->nChannels);
	//��ʼ��Hog����������ʱ�ĳ�ʼ��ͼ���߾�����HOG�������㷨�����ڴ�����С
	CvSize filterSize = model.getMaxSizeOfFilters();
	HOGPyramid pyramid = HOGPyramid(normImage->width, normImage->height, padx,
			pady, interval, std::max(filterSize.width, filterSize.height));
	//alloc memory
	g_DPM_memory = new MemAllocDPM();
	/**************************************************
	 output root&DPM result
	 ***************************************************/

//	do
//	{
		procCount++;
		std::cout << buf << std::endl;
		foutFastTime << buf << ",";
//		if (origImage == NULL)
//			continue;

		cvCopy(origImage, normImage);

		static zftdt::DPMVector<Result> fastResults(DPM_MAX_MAXIA);
		fastResults.size = 0;

		for (int i = 0; i < 100; ++i)
		{
			beg = TIME_READ;
			CvSize size = model.getMaxSizeOfFilters();

			int maxFilterSideLen = std::max(size.width, size.height);
			pyramid.build(normImage,
					std::max(maxFilterSideLen * DPM_HOG_CELLSIZE, minSideLen),
					maxLevels); //
			model.detectFast(normImage, minSideLen, maxLevels, pyramid,
					rootThreshold, threshold, overlap, fastResults);
			end = TIME_READ;

			timeDetectFast = (end - beg);
			//std::cout << timeDetectFast << ",";
			foutFastTime << timeDetectFast << ",";
		}

		foutDetectFast << buf << ";";
		for (int i = 0; i < fastResults.size; i++)
		{
			foutDetectFast << fastResults[i].rects[0].x << ","
					<< fastResults[i].rects[0].y << ","
					<< fastResults[i].rects[0].width << ","
					<< fastResults[i].rects[0].height << " ";
		}
		foutDetectFast << std::endl;
		//std::cout << std::endl;
		foutFastTime << std::endl;
		cvReleaseImage(&origImage);
		origImage = NULL;
/*
		getline(ffstrm, buf);
		if (buf.empty())
		{
			continue;
		}
		origImage = cvLoadImageFromFile(buf.c_str());
*/
//	} while (!ffstrm.eof());

	foutDetectFast.close();
	foutFastTime.close();

	cvReleaseImage(&normImage);
	cvReleaseImage(&origImage);
//	ffstrm.close();
	delete g_DPM_memory;
	return 0;
}
#endif