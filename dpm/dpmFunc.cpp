/*
 * testAll.cpp
 *
 *  Created on: 2015-8-5
 *      Author: julie
 */

#include "DPM.h"
#include "dpmFunc.h"

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
//#include "LinkLayer.h"

#define PCIE_EP_IRQ_SET		 (0x21800064)
//#define TRUE (1)
#define EP_IRQ_CLR                   0x68
#define EP_IRQ_STATUS                0x6C
//dpm
#define DSP_DPM_OVER  (0x00aa5500U)
#define DSP_DPM_CLROVER  (0x0055aa00U)

#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)
#define DEVICE_REG32_R(x)    (*(volatile uint32_t *)(x))

#ifdef __cplusplus
extern "C"
{
#endif

#include "ti/platform/platform.h"
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>
extern void write_uart(char* msg);
extern Semaphore_Handle gRecvSemaphore;
//extern Semaphore_Handle gSendSemaphore;

#ifdef __cplusplus
}
#endif

typedef struct _tagPicInfo
{
	unsigned char *pSrcData;
	unsigned char *pSubData;
	unsigned int srcPicWidth;
	unsigned int srcPicHeigth;
	unsigned int nWidth;
	unsigned int nHeigth;
	int nXpoint;
	int nYpoint;
	unsigned int pictureType; //0:YUV,1:RGB
} picInfo_t;

picInfo_t pictureInfo;

extern char debugInfor[100];
uint32_t endFlag = 0x55ff;

using namespace std;
using namespace zftdt;

#define RES_RECORD_EN (1)
#define JPEG_DECODE_EN (1)

#define TIME_INIT (TSCH = 0, TSCL = 0)
#define TIME_READ _itoll(TSCH, TSCL)
#define C6678_PCIEDATA_BASE (0x60000000U)
DeformablePartModel *model = NULL;
uint32_t *g_pSendBuffer = (uint32_t *) (C6678_PCIEDATA_BASE + 4 * 4 * 1024);

//extern PicInfor gPictureInfor;

static int getSubPicture(picInfo_t *pSrcPic);

void dpmInit()
{

	std::string str0(pMotorParam0);
	std::string str1(pMotorParam1);
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
	const string modelPath = str0 + str1 + str2 + str3 + str4 + str5 + str6
			+ str7 + str8 + str9 + str10 + str11;
	model = new DeformablePartModel(modelPath);
	g_DPM_memory = new MemAllocDPM();
}
//int testlib(char *rgbBuf,int width,int height,int picNum,int maxNum)
int dpmProcess(char *rgbBuf, int width, int height, int picNum, int maxNum)
{

	long long beg, end;
	TIME_INIT;

	long long timeDetectFast;
	int procCount = 0;
	int subPicLen = 0;

	const string prefix = "";
	double threshold = -0.6;
	double rootThreshold = 0.9;
	double overlap = 0.4;
	int padx = DPM_HOG_PADX;
	int pady = DPM_HOG_PADY;
	int interval = DPM_PYRAMID_LAMBDA;
	int maxLevels = DPM_PYRAMID_MAX_LEVELS;
	int minSideLen = DPM_PYRAMID_MIN_DETECT_STRIDE;

	IplImage * origImage = cvCreateImage(cvSize(width, height), 8, 3);
	origImage->imageData = rgbBuf;

	unsigned char *pRGBdata = (unsigned char *) malloc(width * height * 3);
	memcpy(pRGBdata, rgbBuf, (width * height * 3));

	IplImage * normImage = cvCreateImage(
			cvSize(origImage->width, origImage->height), origImage->depth,
			origImage->nChannels);

	////////////////////////////////////////////////////
	write_uart("dsp wait for being triggerred to start dpm\r\n");
	Semaphore_pend(gRecvSemaphore, BIOS_WAIT_FOREVER);
	////////////////////////////////////////////////////

	CvSize filterSize = model->getMaxSizeOfFilters();
	HOGPyramid pyramid = HOGPyramid(normImage->width, normImage->height, padx,
			pady, interval, std::max(filterSize.width, filterSize.height));

	/**************************************************
	 output root&DPM result
	 ***************************************************/
	procCount++;
	cvCopy(origImage, normImage);
	static zftdt::DPMVector<Result> fastResults(DPM_MAX_MAXIA);
	fastResults.size = 0;

	for (int i = 0; i < 1; ++i)
	{
		beg = TIME_READ;
		CvSize size = model->getMaxSizeOfFilters();

		int maxFilterSideLen = std::max(size.width, size.height);
		pyramid.build(normImage,
				std::max(maxFilterSideLen * DPM_HOG_CELLSIZE, minSideLen),
				maxLevels); //

		model->detectFast(normImage, minSideLen, maxLevels, pyramid,
				rootThreshold, threshold, overlap, fastResults);

		end = TIME_READ;

		timeDetectFast = (end - beg);
	}
	sprintf(debugInfor, "fastResults.size=%d\r\n", fastResults.size);
	write_uart(debugInfor);
	for (int i = 0; i < fastResults.size; i++)
	{
		sprintf(debugInfor, "%d,%d,%d,%d\r\n", fastResults[i].rects[0].x,
				fastResults[i].rects[0].y, fastResults[i].rects[0].width,
				fastResults[i].rects[0].height);
		write_uart(debugInfor);

		pictureInfo.nHeigth = ((fastResults[i].rects[0].height + 1) / 2) * 2;
		pictureInfo.nWidth = ((fastResults[i].rects[0].width + 1) / 2) * 2;
		pictureInfo.nXpoint = ((fastResults[i].rects[0].x - 1) / 2) * 2;
		pictureInfo.nYpoint = ((fastResults[i].rects[0].y - 1) / 2) * 2;
	}

	pictureInfo.pSubData = (unsigned char *) malloc(
			(pictureInfo.nHeigth * pictureInfo.nWidth) * 3);

	memset(pictureInfo.pSubData, 0xff,
			(pictureInfo.nHeigth * pictureInfo.nWidth) * 3);

	pictureInfo.pictureType = 1; //rgb
	pictureInfo.pSrcData = pRGBdata;
	pictureInfo.srcPicWidth = width;
	pictureInfo.srcPicHeigth = height;

	getSubPicture(&pictureInfo);

//	memcpy(g_pSendBuffer, (char *) &pictureInfo.nWidth, sizeof(int));
//	g_pSendBuffer = (uint32_t *) ((uint8_t *) (g_pSendBuffer) + sizeof(int));


	//store subPic to shared zone
	subPicLen = (pictureInfo.nHeigth * pictureInfo.nWidth) * 3;
	memcpy(g_pSendBuffer, (char *) &pictureInfo.nWidth, sizeof(int));
	g_pSendBuffer = (uint32_t *) ((uint8_t *) (g_pSendBuffer) + sizeof(int));

	memcpy(g_pSendBuffer, (char *) &pictureInfo.nHeigth, sizeof(int));
	g_pSendBuffer = (uint32_t *) ((uint8_t *) (g_pSendBuffer) + sizeof(int));

	memcpy(g_pSendBuffer, (char *) &subPicLen, sizeof(int));
	g_pSendBuffer = (uint32_t *) ((uint8_t *) (g_pSendBuffer) + sizeof(int));

	memcpy(((uint8_t *) (g_pSendBuffer)), pictureInfo.pSubData, subPicLen);
	g_pSendBuffer = (uint32_t *) ((uint8_t *) (g_pSendBuffer) + subPicLen);

	if (picNum == maxNum - 1)
	{ //set end flag
		memcpy(g_pSendBuffer, (char *) &endFlag, sizeof(int));
	}

	free(pictureInfo.pSrcData);
	free(pictureInfo.pSubData);

	sprintf(debugInfor, "timeDetectFast=%d\r\n", timeDetectFast);
	write_uart(debugInfor);

	//cyx add for second picture dpm process
	write_uart("the second picture dpm process start semphore\r\n");
	Semaphore_post(gRecvSemaphore);

	cvReleaseImage(&normImage);
	cvReleaseImage(&origImage);
	origImage = NULL;
	normImage = NULL;

	return 0;
}

int getSubPicture(picInfo_t *pSrcPic)
{

	int retVal = 0;

	unsigned int subPicStartPointX = 0;
	unsigned int subPicStartPointY = 0;
	unsigned int subPicWidth = 0;
	unsigned int subPicHeigth = 0;

	unsigned int srcPicWidth = 0;
	unsigned int srcPicHeigth = 0;

	unsigned char *pSrcRGBStart = NULL;
	unsigned char *pDestRGBStart = NULL;

	int nHeigthIndex = 0;

	if (pSrcPic != NULL)
	{
		subPicStartPointX = pSrcPic->nXpoint;
		subPicStartPointY = pSrcPic->nYpoint;
		subPicWidth = pSrcPic->nWidth;
		subPicHeigth = pSrcPic->nHeigth;

		srcPicWidth = pSrcPic->srcPicWidth;
		srcPicHeigth = pSrcPic->srcPicHeigth;

		pSrcRGBStart = pSrcPic->pSrcData
				+ (srcPicWidth * subPicStartPointY + subPicStartPointX) * 3;
		pDestRGBStart = pSrcPic->pSubData;

		retVal = ((pSrcPic->pictureType == 1) ? 0 : -2);

	}
	else
	{
		retVal = -1;
		//System_printf(
		//		"error: the input or output buffer in getSubPicture function is NULL\n");
		return (retVal);
	}

	if (retVal == 0)
	{
		for (nHeigthIndex = 0; nHeigthIndex < subPicHeigth; nHeigthIndex++)
		{

			memcpy(pDestRGBStart, pSrcRGBStart, subPicWidth * 3);

			pSrcRGBStart += (srcPicWidth * 3);
			pDestRGBStart += (subPicWidth * 3);
		}
		//rgb2bmp((char *) pSrcPic->pSubData, subPicWidth, subPicHeigth);
	}
	else
	{
		retVal = -2;
		//System_printf(
		//		"error: the input picture format to the dpm is not RGB\n");
		return (retVal);
	}

	return (retVal);
}