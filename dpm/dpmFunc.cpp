/*
 * testAll.cpp
 *
 *  Created on: 2015-8-5
 *      Author: julie
 */

#include "DPM.h"
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
#include "carcyclist.h"
#include "personcyclist.h"
#include "LinkLayer.h"

#define PCIE_EP_IRQ_SET		 (0x21800064)
//#define TRUE (1)
#define EP_IRQ_CLR                   0x68
#define EP_IRQ_STATUS                0x6C
//dpm
#define DSP_DPM_OVER  (0x00aa5500U)
#define DSP_DPM_CLROVER  (0x0055aa00U)

#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)
#define DEVICE_REG32_R(x)    (*(volatile uint32_t *)(x))

typedef struct __tagPicInfor
{
	uint8_t *picAddr[100];
	uint8_t picUrls[100][120];
	uint8_t picName[100][40];
	uint32_t picLength[100];
	uint32_t picNums;
} PicInfor;

#ifdef __cplusplus
extern "C"
{
#endif

#include "ti/platform/platform.h"
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>
extern void debugLog(char* msg);
extern void write_uart(char* msg);

extern Semaphore_Handle gRecvSemaphore;

void dpmInit();
int dpmProcess(char *rgbBuf, int width, int height, int picNum, int maxNum,
		int totalNum, registerTable *pRegisterTable);

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
//extern PicInfor gPictureInfor;
extern PicInfor *p_gPictureInfor;

extern char debugInfor[100];
uint32_t endFlag = 0xffaa;

using namespace std;
using namespace zftdt;

#define RES_RECORD_EN (1)
#define JPEG_DECODE_EN (1)

#define TIME_INIT (TSCH = 0, TSCL = 0)
#define TIME_READ _itoll(TSCH, TSCL)
#define C6678_PCIEDATA_BASE (0x60000000U)
DeformablePartModel *model = NULL;

DeformablePartModel *pMotorModel = NULL;
DeformablePartModel *pCarModel = NULL;
DeformablePartModel *pPersonModel = NULL;
//extern registerTable *pRegisterTable;
#ifdef __cplusplus
extern "C"
{
#endif

extern uint32_t *g_pSendBuffer;

#ifdef __cplusplus
}
#endif

static int getSubPicture(picInfo_t *pSrcPic);

void dpmInit()
{
	write_uart("dpm init begin1\n\r");
	std::string strm0(pMotorParam0);
	std::string strm1(pMotorParam1);
	std::string strm2(pMotorParam2);
	std::string strm3(pMotorParam3);
	std::string strm4(pMotorParam4);
	std::string strm5(pMotorParam5);
	std::string strm6(pMotorParam6);
	std::string strm7(pMotorParam7);
	std::string strm8(pMotorParam8);
	std::string strm9(pMotorParam9);
	std::string strm10(pMotorParam10);
	std::string strm11(pMotorParam11);
	const string modelPathMotor = strm0 + strm1 + strm2 + strm3 + strm4 + strm5
			+ strm6 + strm7 + strm8 + strm9 + strm10 + strm11;
	//write_uart("dpm init begin2\n\r");
	std::string strc0(pCarParam0);
	std::string strc1(pCarParam1);
	std::string strc2(pCarParam2);
	std::string strc3(pCarParam3);
	std::string strc4(pCarParam4);
	std::string strc5(pCarParam5);
	std::string strc6(pCarParam6);
	std::string strc7(pCarParam7);
	std::string strc8(pCarParam8);
	std::string strc9(pCarParam9);
	std::string strc10(pCarParam10);
	std::string strc11(pCarParam11);
	const string modelPathCar = strc0 + strc1 + strc2 + strc3 + strc4 + strc5
			+ strc6 + strc7 + strc8 + strc9 + strc10 + strc11;
#if 1
	std::string strp0(pPersonParam0);
	std::string strp1(pPersonParam1);
	std::string strp2(pPersonParam2);
	std::string strp3(pPersonParam3);
	std::string strp4(pPersonParam4);
	std::string strp5(pPersonParam5);
	std::string strp6(pPersonParam6);
	std::string strp7(pPersonParam7);
	std::string strp8(pPersonParam8);
	std::string strp9(pPersonParam9);
	std::string strp10(pPersonParam10);
	std::string strp11(pPersonParam11);
	const string modelPathPerson = strp0 + strp1 + strp2 + strp3 + strp4 + strp5
			+ strp6 + strp7 + strp8 + strp9 + strp10 + strp11;

#endif
	//write_uart("dpm init begin3\n\r");
	pMotorModel = new DeformablePartModel(modelPathMotor);
	pCarModel = new DeformablePartModel(modelPathCar);
	pPersonModel = new DeformablePartModel(modelPathPerson);

	//write_uart("dpm init begin4\n\r");
	g_DPM_memory = new MemAllocDPM();
	write_uart("dpm init finished\n\r");
}
int dpmProcess(char *rgbBuf, int width, int height, int picNum, int maxNum,
		int totalNum, registerTable *pRegisterTable)
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
	// set for test
	pRegisterTable->DSP_modelType = 2;
	// -m params
	if (pRegisterTable->DSP_modelType == 1)
	{
		write_uart("here coming into motor detection!!\r\n");
		model = pMotorModel;
	}
	if (pRegisterTable->DSP_modelType == 2)
	{
		write_uart("here coming into car detection!!\r\n");
		model = pCarModel;
	}
	if (pRegisterTable->DSP_modelType == 3)
	{
		write_uart("here coming into person detection!!\r\n");
		model = pPersonModel;
	}

	////////////////////////////////////////////////////
	write_uart("dsp wait for being triggerred to start dpm\r\n");

	//Semaphore_pend(gRecvSemaphore, BIOS_WAIT_FOREVER);
	////////////////////////////////////////////////////

	write_uart("after model set");

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
		debugLog(debugInfor);

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

	//store subPic to shared zone
	subPicLen = (pictureInfo.nHeigth * pictureInfo.nWidth) * 3;
	//*************************store original picture*********************************/
//	memcpy(g_pSendBuffer, p_gPictureInfor->picUrls[picNum], 120);
//	g_pSendBuffer = (g_pSendBuffer + 120 / 4);
//
//	memcpy(g_pSendBuffer, p_gPictureInfor->picName[picNum], 40);
//	g_pSendBuffer = (g_pSendBuffer + 40 / 4);
//
//	memcpy(g_pSendBuffer, &(p_gPictureInfor->picLength[picNum]), 4);
//	g_pSendBuffer = (g_pSendBuffer + 4 / 4);
//
//	memcpy(g_pSendBuffer, (p_gPictureInfor->picAddr[picNum] + 4),
//			p_gPictureInfor->picLength[picNum]);
//	g_pSendBuffer = (g_pSendBuffer + (p_gPictureInfor->picLength[picNum] + 4) / 4);

	//*************************store sub picture*********************************/
	memcpy(g_pSendBuffer, &pictureInfo.nWidth, sizeof(int));
	g_pSendBuffer += 1;

	memcpy(g_pSendBuffer, &pictureInfo.nHeigth, sizeof(int));
	g_pSendBuffer += 1;

	memcpy(g_pSendBuffer, &pictureInfo.nXpoint, sizeof(int));
	g_pSendBuffer += 1;

	memcpy(g_pSendBuffer, &pictureInfo.nYpoint, sizeof(int));
	g_pSendBuffer += 1;

	memcpy(g_pSendBuffer, &subPicLen, sizeof(int));
	g_pSendBuffer += 1;

	memcpy(((uint8_t *) (g_pSendBuffer)), pictureInfo.pSubData, subPicLen);
	g_pSendBuffer = (g_pSendBuffer + (subPicLen + 4) / 4);

	if ((picNum % maxNum == maxNum - 1) || (picNum == totalNum - 1))
	{ //every loop last and all of the last pic,we need set end flag
		memcpy(g_pSendBuffer, &endFlag, sizeof(int));
		g_pSendBuffer = (uint32_t *) (C6678_PCIEDATA_BASE + 4 * 4 * 1024);
	}

	free(pictureInfo.pSrcData);
	free(pictureInfo.pSubData);

	sprintf(debugInfor, "timeDetectFast=%d\r\n", timeDetectFast);
	debugLog(debugInfor);

	//cyx add for second picture dpm process
	write_uart("the second picture dpm process start semphore\r\n");
	if ((picNum % maxNum == maxNum - 1) || (picNum == totalNum - 1))
	{
		write_uart("the last picture ,and we did not post semaphore\r\n");
	}
	else
	{
		//Semaphore_post(gRecvSemaphore);
	}

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
