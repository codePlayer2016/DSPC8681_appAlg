/******************************************************************************/
/*            Copyright (c) 2011 Texas Instruments, Incorporated.             */
/*                           All Rights Reserved.                             */
/******************************************************************************/

/*!
 ********************************************************************************
 @file     TestAppDecoder.c
 @brief    This is the top level client file that drives the JPEG
 (Progressive Support) Image Decoder Call using XDM Interface
 @author   Multimedia Codecs TI India
 @version  0.1 - July 23,2011    initial version
 ********************************************************************************
 */

/* Standard C header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* JPEG Interface header files */
#include "jpegdec_ti.h"
#include "jpegdec.h"
#include "jpegDecoder.h"
#include "LinkLayer.h"
#include "dpmFunc.h"

/* CSL and DMAN3 header files                                                 */
#include <ti/sysbios/family/c66/Cache.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include "jpegDecoder.h"
#include "ti/platform/platform.h"

void JpegInit();
void JpegDeInit();
void DpmDeInit();
void JpegProcess(int picNum);
void JpegProcess(int picNum);

extern volatile cregister unsigned int DNUM;
typedef struct __tagPicInfor
{
//	uint8_t *picAddr[100];
//	uint8_t picUrls[100][120];
//	uint8_t picName[100][40];
//	uint32_t picLength[100];
//	uint32_t picNums;
	uint8_t *picSrcAddr[10];
	uint32_t picSrcLength[10];
	uint8_t picNums;
} PicInfor;

extern void write_uart(char* msg);


//#define DEBUG_PROCESS1
#ifdef DEBUG_PROCESS1
void debugLog(char* msg)
{
	uint32_t i;
	uint32_t msg_len = strlen(msg);

	/* Write the message to the UART */
	for (i = 0; i < msg_len; i++)
	{
		platform_uart_write(msg[i]);
	}
}
#else
void debugLog(char* msg)
{

}
#endif


uint32_t *g_pSendBuffer = (uint32_t *) (C6678_PCIEDATA_BASE + 4 * 4 * 1024);
// for uart debug
char debugInfor[100];

extern Semaphore_Handle httptodpmSemaphore;
extern Semaphore_Handle gRecvSemaphore;
extern Semaphore_Handle gSendSemaphore;
extern Semaphore_Handle g_dpmProcBg;
extern Semaphore_Handle pcFinishReadSemaphore;
//extern unsigned char *g_outBuffer; //4M

//extern PicInfor gPictureInfor;//
extern PicInfor *p_gPictureInfor;
extern unsigned char *pCore1InBuf;
uint8_t *inputSrc = NULL;

int jpegPicLength = 0;
registerTable *pRegisterTable = (registerTable *) C6678_PCIEDATA_BASE;

XDAS_Int8 outputData[OUTPUT_BUFFER_SIZE];

XDAS_Int8 refData[OUTPUT_BUFFER_SIZE];

//extern void yuv2bmp(unsigned char * YUV, int width, int height, int picNum);
//int dpmProcess(char *rgbBuf, int width, int height, int picNum, int maxNum,int totalNum,registerTable *pRegisterTable);

//JPEGDEC_Handle handle;
JPEGDEC_Params jpegdecParams;
JPEGDEC_DynamicParams jpegdecDynamicParams;
JPEGDEC_Status jpegdecStatus;
JPEGDEC_InArgs jpegdecInArgs;
JPEGDEC_OutArgs jpegdecOutArgs;

XDAS_UInt32 resizeOption, progressiveDecFlag, RGB_Format, alpha_rgb;
XDAS_UInt32 outImgRes, numMCU_row, x_org, y_org, x_length, y_length;

IIMGDEC1_Fxns *IIMGDEC1Fxns;

/* Algorithm specific handle                                                */
IALG_Handle handle;
/* Input/Output Buffer Descriptors                                          */
XDM1_BufDesc inputBufDesc, outputBufDesc;

JPEGDEC_Params *jpegDecParams = (JPEGDEC_Params*) &jpegdecParams;
JPEGDEC_DynamicParams *dynamicParams =
		(JPEGDEC_DynamicParams*) &jpegdecDynamicParams;
JPEGDEC_Status *status = (JPEGDEC_Status*) &jpegdecStatus;
JPEGDEC_InArgs *inArgs = (JPEGDEC_InArgs*) &jpegdecInArgs;
JPEGDEC_OutArgs *outArgs = (JPEGDEC_OutArgs*) &jpegdecOutArgs;

//XDAS_Int32 DecodeTask()
void DPMMain()
{

	//if (DNUM == 1)
	{
		unsigned int picNum = 0;
		unsigned int picMax = 4;
		int retVal = 0;
		int count = 0;
		Semaphore_pend(g_dpmProcBg, BIOS_WAIT_FOREVER);
		debugLog("----dpmMain begin----\n\r");
		/* Init jpeg Decode */
		JpegInit();
		/* Init DPM Algrithm */
		debugLog("----dpmInit begin----\n\r");
		dpmInit();
		//Semaphore_pend(httptodpmSemaphore, BIOS_WAIT_FOREVER);
		debugLog("----dpmInit over----\n\r");

		//write_uart("dpm begin process the picture\n\r");




		debugLog("jpegInit over\n\r");
//	sprintf(debugInfor,
//			"p_gPictureInfor->picNums is %d DSP_DPM_OVERSTATUS is %x pRegisterTable->dpmOverStatus is %x \r\n",
//			p_gPictureInfor->picNums, DSP_DPM_OVERSTATUS,
//			pRegisterTable->dpmOverStatus);
//	write_uart(debugInfor);
//	count = picNum % URLNUM;

//	while (1)
		{
//		write_uart("222222222222before pend pcFinishReadSemaphore\r\n");
//		Semaphore_pend(pcFinishReadSemaphore, BIOS_WAIT_FOREVER);

			//while (count < URLNUM && picNum < p_gPictureInfor->picNums)
			while (picNum < picMax)
			{
				/* jpeg Decode to process one picture */
				JpegProcess(picNum);
			if ((outArgs->imgdecOutArgs.extendedError == JPEGDEC_SUCCESS))
			{
				dpmProcess(outputBufDesc.descs[0].buf,
						status->imgdecStatus.outputWidth,
						status->imgdecStatus.outputHeight, picNum, URLNUM,
						p_gPictureInfor->picNums, pRegisterTable);
				//int dpmProcess(char *rgbBuf, int width, int height, int picNum, int maxNum,int totalNum,registerTable *pRegisterTable);

			}
				picNum++;
				//count++;
			}
#if 0
			JpegDeInit();
			count = 0;
			//all picture dpm over
			if (picNum >= p_gPictureInfor->picNums)
			{
				//set reg
				pRegisterTable->dpmAllOverControl |= DSP_DPM_ALLOVER;
				DEVICE_REG32_W(PCIE_EP_IRQ_SET, 0x1);
				write_uart(
						"dsp have finish all picture dpm process,trigger the host interrupt\r\n");
				picNum = 0;
				break;

			}
			//part picture dpm over
			else
			{
				//check clear dpmOver interrupt reg or not
				if (pRegisterTable->dpmOverStatus & DSP_DPM_OVERSTATUS)
				{
					pRegisterTable->dpmOverControl |= DSP_DPM_OVERCLR;
				}
				// trigger the interrupt to the pc ,
				DEVICE_REG32_W(PCIE_EP_IRQ_SET, 0x1);
				write_uart("trigger the host interrupt\r\n");

				//Semaphore_post(gSendSemaphore);
				//write_uart("post gSendSemaphore,make http loop can continue\r\n");
			}
#endif

		}
	}
}
void JpegInit()
{
	//write_uart("jpegInit\n\r");
	jpegDecParams->imgdecParams.size = sizeof(JPEGDEC_Params);
	// TODO: the two params should be set by parse the jpeg picture.
	jpegDecParams->imgdecParams.maxHeight = 1500;
	jpegDecParams->imgdecParams.maxWidth = 2000;
	jpegDecParams->imgdecParams.maxScans = 15; //?
	//cyx
	jpegDecParams->imgdecParams.forceChromaFormat = XDM_RGB;
	jpegDecParams->imgdecParams.dataEndianness = XDM_BYTE;
	jpegDecParams->outImgRes = 0;
	jpegDecParams->progressiveDecFlag = 0;

	dynamicParams->imgdecDynamicParams.size = sizeof(JPEGDEC_DynamicParams);
	dynamicParams->resizeOption = 2; //2
	dynamicParams->imgdecDynamicParams.displayWidth = 0;
	dynamicParams->imgdecDynamicParams.numAU = 0;
	dynamicParams->numMCU_row = 0;
	dynamicParams->x_org = 0;
	dynamicParams->x_org = 0;
	dynamicParams->x_length = 0;
	dynamicParams->y_length = 0;
	dynamicParams->alpha_rgb = 0;
	dynamicParams->progDisplay = 0;
	dynamicParams->RGB_Format = 0;

	jpegdecStatus.imgdecStatus.size = sizeof(JPEGDEC_Status);
	jpegdecInArgs.imgdecInArgs.size = sizeof(JPEGDEC_InArgs);
	jpegdecOutArgs.imgdecOutArgs.size = sizeof(JPEGDEC_OutArgs);

	//write_uart("dpm alg_create\n\r");
	handle = (IALG_Handle) ALG_create((IALG_Fxns *) &JPEGDEC_TI_IJPEGDEC,
			(IALG_Handle) NULL, (IALG_Params *) jpegDecParams);
	if (handle == NULL)
	{
		write_uart("jpeg init error\n\r");
		return;
	}

}
void JpegDeInit()
{
	/* Delete the Algorithm instance object specified by handle */
	ALG_delete(handle);
}
void DpmDeInit()
{

}
void JpegProcess(int picNum)
{
	XDAS_Int32 retVal;
	XDAS_Int32 ScanCount;
	int byteremain = 0, inputsize = 0;
	XDAS_UInt32 ii;
	XDAS_Int32 validBytes;
	XDAS_UInt32 bytesConsumed;

	uint8_t *inputData = NULL;

	inputSrc = p_gPictureInfor->picSrcAddr[picNum];
	jpegPicLength = p_gPictureInfor->picSrcLength[picNum];
	//inputData=((uint8_t *)inputSrc+4);
	inputData = inputSrc;
	//inputData=((char*)inputSrc+4);
	validBytes = jpegPicLength;

	sprintf(debugInfor, "validBytes=%d,inputData address:%x\r\n", validBytes,
			inputData);
	debugLog(debugInfor);
	debugLog("start the jpeg decode and dpm\r\n");

	ScanCount = 1;

#ifdef ENABLE_CACHE
	/* Cache clean */
#ifndef MSVC
	Cache_wbInvAll();
#endif
#endif

	dynamicParams->imgdecDynamicParams.decodeHeader = XDM_DECODE_AU;

	inputBufDesc.descs[0].buf = inputData;
	outputBufDesc.descs[0].buf = outputData;
	if (inputBufDesc.descs[0].buf == NULL)
	{
		write_uart("input img is error\r\n");
		//return;
	}

	/* Assigning Algorithm handle fxns field to IIMGDEC1fxns                  */
	IIMGDEC1Fxns = (IIMGDEC1_Fxns *) handle->fxns;

	/* Resetting bytesGenerated variable                                     */
	bytesConsumed = 0;

	dynamicParams->imgdecDynamicParams.decodeHeader = XDM_PARSE_HEADER;
	/* Activate the Algorithm                                              */
	handle->fxns->algActivate(handle);

	/* Assign the number of bytes available                                */
	inArgs->imgdecInArgs.numBytes = validBytes;
	dynamicParams->frame_numbytes = inArgs->imgdecInArgs.numBytes;
	inputBufDesc.descs[0].buf = (XDAS_Int8 *) ((XDAS_Int32) inputData
			+ bytesConsumed);

	/* Get Buffer information                                              */
	IIMGDEC1Fxns->control((IIMGDEC1_Handle) handle, XDM_GETBUFINFO,
			(IIMGDEC1_DynamicParams *) dynamicParams,
			(IIMGDEC1_Status *) status);

	write_uart("contrl GETBUFINFO ok\r\n");

	/* Fill up the buffers as required by algorithm                        */
	inputBufDesc.numBufs = status->imgdecStatus.bufInfo.minNumInBufs;
	inputBufDesc.descs[0].bufSize =
			status->imgdecStatus.bufInfo.minInBufSize[0];

	for (ii = 0; ii < (status->imgdecStatus.bufInfo.minNumInBufs - 1); ii++)
	{
		inputBufDesc.descs[ii + 1].buf = inputBufDesc.descs[ii].buf
				+ status->imgdecStatus.bufInfo.minInBufSize[ii];
		inputBufDesc.descs[ii + 1].bufSize =
				status->imgdecStatus.bufInfo.minInBufSize[ii + 1];
	}

	outputBufDesc.numBufs = status->imgdecStatus.bufInfo.minNumOutBufs;
	outputBufDesc.descs[0].bufSize =
			status->imgdecStatus.bufInfo.minOutBufSize[0];
	for (ii = 0; ii < (status->imgdecStatus.bufInfo.minNumOutBufs - 1); ii++)
	{
		outputBufDesc.descs[ii + 1].buf = outputBufDesc.descs[ii].buf
				+ status->imgdecStatus.bufInfo.minOutBufSize[ii];
		outputBufDesc.descs[ii + 1].bufSize =
				status->imgdecStatus.bufInfo.minOutBufSize[ii + 1];
	}

	IIMGDEC1Fxns->control((IIMGDEC1_Handle) handle, XDM_SETPARAMS,
			(IIMGDEC1_DynamicParams *) dynamicParams,
			(IIMGDEC1_Status *) status);

	write_uart("contrl SETPARAMS ok\r\n");

	/* Basic Algorithm process() call                                      */
	retVal = IIMGDEC1Fxns->process((IIMGDEC1_Handle) handle,
			(XDM1_BufDesc *) &inputBufDesc, (XDM1_BufDesc *) &outputBufDesc,
			(IIMGDEC1_InArgs *) inArgs, (IIMGDEC1_OutArgs *) outArgs);

	write_uart("PROCESS ok\r\n");

	IIMGDEC1Fxns->control((IIMGDEC1_Handle) handle, XDM_GETSTATUS,
			(IIMGDEC1_DynamicParams *) dynamicParams,
			(IIMGDEC1_Status *) status);

	write_uart("contrl GETSTATUS ok\r\n");

	dynamicParams->imgdecDynamicParams.decodeHeader = XDM_DECODE_AU;
	IIMGDEC1Fxns->control((IIMGDEC1_Handle) handle, XDM_RESET,
			(IIMGDEC1_DynamicParams *) dynamicParams,
			(IIMGDEC1_Status *) status);

	/* DeActivate the Algorithm                                            */
	handle->fxns->algDeactivate(handle);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bytesConsumed = 0;

	/* Activate the Algorithm                                              */
	handle->fxns->algActivate(handle);

	/* Assign the number of bytes available                                */
	inArgs->imgdecInArgs.numBytes = validBytes;
	inputBufDesc.descs[0].buf = (XDAS_Int8 *) ((XDAS_Int32) inputData);

	/* Get Buffer information                                              */
	IIMGDEC1Fxns->control((IIMGDEC1_Handle) handle, XDM_GETBUFINFO,
			(IIMGDEC1_DynamicParams *) dynamicParams,
			(IIMGDEC1_Status *) status);

	/* Fill up the buffers as required by algorithm                        */
	inputBufDesc.numBufs = status->imgdecStatus.bufInfo.minNumInBufs;
	inputBufDesc.descs[0].bufSize =
			status->imgdecStatus.bufInfo.minInBufSize[0];

	for (ii = 0; ii < (status->imgdecStatus.bufInfo.minNumInBufs - 1); ii++)
	{
		inputBufDesc.descs[ii + 1].buf = inputBufDesc.descs[ii].buf
				+ status->imgdecStatus.bufInfo.minInBufSize[ii];
		inputBufDesc.descs[ii + 1].bufSize =
				status->imgdecStatus.bufInfo.minInBufSize[ii + 1];
	}

	outputBufDesc.numBufs = status->imgdecStatus.bufInfo.minNumOutBufs;
	outputBufDesc.descs[0].bufSize =
			status->imgdecStatus.bufInfo.minOutBufSize[0];
	for (ii = 0; ii < (status->imgdecStatus.bufInfo.minNumOutBufs - 1); ii++)
	{
		outputBufDesc.descs[ii + 1].buf = outputBufDesc.descs[ii].buf
				+ status->imgdecStatus.bufInfo.minOutBufSize[ii];
		outputBufDesc.descs[ii + 1].bufSize =
				status->imgdecStatus.bufInfo.minOutBufSize[ii + 1];
	}

	/* Optional: Set Run time parameters in the Algorithm via control()    */
	IIMGDEC1Fxns->control((IIMGDEC1_Handle) handle, XDM_SETPARAMS,
			(IIMGDEC1_DynamicParams *) dynamicParams,
			(IIMGDEC1_Status *) status);

	/* Do-While Loop for Decode Call                                         */

	do
	{

		byteremain = inArgs->imgdecInArgs.numBytes; // newly added
		inputsize = byteremain;

		if (dynamicParams->numMCU_row != 0) // newly added for sliced
		{
			if (byteremain <= (inputsize * dynamicParams->numMCU_row))
				inArgs->imgdecInArgs.numBytes = byteremain;
			else
				inArgs->imgdecInArgs.numBytes = inputsize
						* dynamicParams->numMCU_row;
			if (inArgs->imgdecInArgs.numBytes > dynamicParams->frame_numbytes)
				inArgs->imgdecInArgs.numBytes = dynamicParams->frame_numbytes;
		}

		// Basic Algorithm process() call
		retVal = IIMGDEC1Fxns->process((IIMGDEC1_Handle) handle,
				(XDM1_BufDesc *) &inputBufDesc, (XDM1_BufDesc *) &outputBufDesc,
				(IIMGDEC1_InArgs *) inArgs, (IIMGDEC1_OutArgs *) outArgs);

		byteremain -= outArgs->imgdecOutArgs.bytesConsumed;

		if (dynamicParams->numMCU_row)
		{
			inputBufDesc.descs[0].buf += outArgs->imgdecOutArgs.bytesConsumed;
			inArgs->imgdecInArgs.numBytes -=
					outArgs->imgdecOutArgs.bytesConsumed;
		}

		bytesConsumed += outArgs->imgdecOutArgs.bytesConsumed;

		if (retVal == XDM_EFAIL)
		{
			printf("\n Process function returned an Error...  ");
			break; /* Error Condition: Application may want to break off         */
		}

		/* Optional: Read status via control()                                 */
		IIMGDEC1Fxns->control((IIMGDEC1_Handle) handle, XDM_GETSTATUS,
				(IIMGDEC1_DynamicParams *) dynamicParams,
				(IIMGDEC1_Status *) status);

		if (status->mode == 0) // sequential baseline
		{
			outputBufDesc.descs[0].buf += status->bytesgenerated[0];
			outputBufDesc.descs[1].buf += status->bytesgenerated[1];
			outputBufDesc.descs[2].buf += status->bytesgenerated[2];
		}
		else if ((status->mode == 1)) // Non interlaeved sequential
		{
			if (jpegDecParams->imgdecParams.forceChromaFormat != 8)
			{
				outputBufDesc.descs[0].buf += status->bytesgenerated[0];
				outputBufDesc.descs[1].buf += status->bytesgenerated[1];
				outputBufDesc.descs[2].buf += status->bytesgenerated[2];
			}
		}
		else if (status->mode != 2)
		{
			outputBufDesc.descs[0].buf += status->bytesgenerated[0];
			outputBufDesc.descs[1].buf += status->bytesgenerated[1];
			outputBufDesc.descs[2].buf += status->bytesgenerated[2];
		}

		if ((outArgs->imgdecOutArgs.extendedError != JPEGDEC_SUCCESS))
		{
			printf("\n Decoder ERROR %0x \n ",
					outArgs->imgdecOutArgs.extendedError);
			break;
		}

		/* Check for frame ready via display buffer pointers                   */
		if (outputBufDesc.descs[0].buf != NULL)
		{
			write_uart("jpeg decode is successful\r\n");

			ScanCount++;
		}

		if (status->end_of_seq == 1)
		{
			break;
		}

	} while (1); /* end of Do-While loop                                    */

	/* DeActivate the Algorithm                                            */
	handle->fxns->algDeactivate(handle);

	IIMGDEC1Fxns->control((IIMGDEC1_Handle) handle, XDM_GETSTATUS,
			(IIMGDEC1_DynamicParams *) dynamicParams,
			(IIMGDEC1_Status *) status);

	for (ii = 0; ii < (status->imgdecStatus.bufInfo.minNumOutBufs); ii++)
	{

		outputBufDesc.descs[ii].bufSize =
				status->imgdecStatus.bufInfo.minOutBufSize[ii];
	}
	sprintf(debugInfor, "width=%d,heigth=%d\r\n",
			(status->imgdecStatus.outputWidth),
			(status->imgdecStatus.outputHeight));
	debugLog(debugInfor);

	if (ScanCount == 0)
	{
		ScanCount = 1; /* To avoid division with zero */
	}
	inputSrc = NULL;
	inputData = NULL;

}

