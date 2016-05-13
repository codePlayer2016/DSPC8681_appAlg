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
#if 0
void write_uart(char* msg)
{
	uint32_t i;
	uint32_t msg_len = strlen(msg);

	/* Write the message to the UART */
	for (i = 0; i < msg_len; i++)
	{
		platform_uart_write(msg[i]);
	}
}
#endif

typedef struct __tagPicInfor
{
	uint8_t *picAddr[100];
	uint32_t picLength[100];
	uint8_t picUrls[100][120];
	uint8_t picName[100][40];
	uint8_t picNums;
} PicInfor;

uint32_t *g_pSendBuffer = (uint32_t *) (C6678_PCIEDATA_BASE + 4 * 4 * 1024);
// for uart debug
char debugInfor[100];

extern Semaphore_Handle httptodpmSemaphore;
extern Semaphore_Handle gRecvSemaphore;
extern Semaphore_Handle gSendSemaphore;
extern unsigned char g_outBuffer[0x00400000]; //4M

extern PicInfor gPictureInfor;
//extern PicOutInfor gPicOutInfor[40];
// modify by LHS. the inputData is the src picture.
//XDAS_Int8 inputData[INPUT_BUFFER_SIZE];
int8_t *inputSrc = NULL;
int8_t * inputData;
int jpegPicLength = 0;
registerTable *pRegisterTable = (registerTable *) C6678_PCIEDATA_BASE;

XDAS_Int8 outputData[OUTPUT_BUFFER_SIZE];

XDAS_Int8 refData[OUTPUT_BUFFER_SIZE];

extern void yuv2bmp(unsigned char * YUV, int width, int height, int picNum);

//JPEGDEC_Handle handle;
JPEGDEC_Params jpegdecParams;
JPEGDEC_DynamicParams jpegdecDynamicParams;
JPEGDEC_Status jpegdecStatus;
JPEGDEC_InArgs jpegdecInArgs;
JPEGDEC_OutArgs jpegdecOutArgs;

XDAS_UInt32 resizeOption, progressiveDecFlag, RGB_Format, alpha_rgb;
XDAS_UInt32 outImgRes, numMCU_row, x_org, y_org, x_length, y_length;
XDAS_Int32 DecodeTask(void);

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
void JpegInit()
{
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

	if ((handle = (IALG_Handle) ALG_create((IALG_Fxns *) &JPEGDEC_TI_IJPEGDEC,
			(IALG_Handle) NULL, (IALG_Params *) jpegDecParams)) == NULL)
	{

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

	inputSrc = gPictureInfor.picAddr[picNum];
	jpegPicLength = gPictureInfor.picLength[picNum];
	inputData = ((char *) inputSrc + 4);
	validBytes = jpegPicLength;

	sprintf(debugInfor, "validBytes=%d,inputData address:%x\r\n", validBytes,
			inputData);
	write_uart(debugInfor);
	write_uart("start the jpeg decode and dpm\r\n");

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
			//printf("\n Decoded Frame # %d  ", ScanCount);
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
	write_uart(debugInfor);

	if (ScanCount == 0)
	{
		ScanCount = 1; /* To avoid division with zero */
	}
	inputSrc = NULL;
	inputData = NULL;

}
////cyx
//XDAS_Void DpmProcess(XDM1_BufDesc * outputBufDesc, int width, int height,int picNum,int maxNum)
//{
//	write_uart("dpm algorith start\r\n");
//	testlib(outputBufDesc->descs[0].buf, width, height,picNum,maxNum);
//	write_uart("dpm algorith over\r\n");
//	return;
//}
void DPMMain()
{

	unsigned int picNum = 0;
	int retVal = 0;
	/* Init jpeg Decode */
	JpegInit();
	/* Init DPM Algrithm */
	dpmInit();

	while (1)
	{
		// this is promise for dpm being after loadurl

		Semaphore_pend(httptodpmSemaphore, BIOS_WAIT_FOREVER);
		sprintf(debugInfor,
				"gPictureInfor.picNums is %d DSP_DPM_OVERSTATUS is %x pRegisterTable->dpmOverStatus is %x \r\n",
				gPictureInfor.picNums, DSP_DPM_OVERSTATUS,
				pRegisterTable->dpmOverStatus);
		write_uart(debugInfor);

		retVal = pollValue(&(pRegisterTable->dpmOverStatus), DSP_DPM_OVERSTATUS,
				0x07ffffff);
		if (retVal == 0)
		{
			pRegisterTable->dpmStartControl = DSP_DPM_STARTCLR;
			write_uart("poll pc get data success\r\n");

		}
		else
		{
			sprintf(debugInfor, "wait pc read data timeout\r\n");
			write_uart(debugInfor);
		}
		while (picNum < gPictureInfor.picNums)
		{
			/* jpeg Decode to process one picture */
			JpegProcess(picNum);

			if ((outArgs->imgdecOutArgs.extendedError == JPEGDEC_SUCCESS))
			{

				dpmProcess(outputBufDesc.descs[0].buf,
						status->imgdecStatus.outputWidth,
						status->imgdecStatus.outputHeight, picNum,
						gPictureInfor.picNums);

			}

			picNum++;
		}
		JpegDeInit();
		picNum = 0;
		gPictureInfor.picNums = 0;
		//check clear dpmOver interrupt reg or not
		if (pRegisterTable->dpmOverStatus & DSP_DPM_OVERSTATUS)
		{
			pRegisterTable->dpmOverControl |= DSP_DPM_OVERCLR;
		}
		// trigger the interrupt to the pc ,
		DEVICE_REG32_W(PCIE_EP_IRQ_SET, 0x1);
		write_uart("trigger the host interrupt\r\n");
//cyx 2016.5.12
#if 0

		retVal = pollZero(&(pRegisterTable->dpmOverStatus), 0x00000000,
				0x07ffffff);
		if (retVal == 0)
		{
			pRegisterTable->dpmOverControl = 0x00000000;
			write_uart("pRegisterTable->dpmOverControl success\r\n");

		}
		else
		{
			sprintf(debugInfor, "pRegisterTable->dpmOverControl timeout\r\n");
			write_uart(debugInfor);
		}
#endif
		Semaphore_post(gSendSemaphore);
		write_uart("post gSendSemaphore,make http loop can continue\r\n");

	}
} /* main() */

/*
 //============================================================================
 // TestApp_ReadByteStream
 //  Reading Byte Stream from a File
 */

XDAS_Int32 TestApp_ReadByteStream(FILE *finFile)
{
	XDAS_UInt32 BytesRead, BufferSize;

	fseek(finFile, 0, SEEK_END);
	BufferSize = ftell(finFile);
	printf("\nFileSize = %d", BufferSize);
	fseek(finFile, 0, SEEK_SET);

	if (BufferSize > INPUT_BUFFER_SIZE)
	{
		printf(
				"\nWarning : File size exceeds the application input buffer size %d ",
				INPUT_BUFFER_SIZE);
		printf("\nContinuing decoding for %d bytes.\n", INPUT_BUFFER_SIZE);
		BufferSize = INPUT_BUFFER_SIZE;
	}

	/*Read the "BufferSize" number of bytes in the input buffer*/
	BytesRead = fread(inputData, 1, BufferSize, finFile);
#if 0
	/* Cache Invalidate for Input Buffer */
#ifndef MSVC
	Cache_wbInv(inputData, BufferSize, Cache_Type_ALL, TRUE);
#endif
#endif
	if (BytesRead != BufferSize)
	{
		printf("\nFailure in reading the input bitstream! \n");
		return (-1);
	}
	printf("\nInput File read successfully...");
	return (BytesRead);
}

/*
 //============================================================================
 // TestApp_CompareOutputData
 //  Comparing Output Data with Reference File data
 */

XDAS_Int32 TestApp_CompareOutputData(FILE *fRefFile,
		XDM1_BufDesc * outputBufDesc, IIMGDEC1_OutArgs *outArgs)
{
	XDAS_Int32 i, bufferSize, retVal;
	XDAS_UInt8 *outputData;

	retVal = XDM_EOK;

	for (i = 0; i < outputBufDesc->numBufs; i++)
	{
		outputData = (XDAS_UInt8 *) outputBufDesc->descs[i].buf;
		bufferSize = outputBufDesc->descs[i].bufSize;
		fread(refData, 1, bufferSize, fRefFile);
		if (memcmp(outputData, refData, bufferSize))
		{
			printf("\n Decoder compliance test failed for this frame. ");
			retVal = XDM_EFAIL;
			break;
		}
	}
	return retVal;
}

/*
 //============================================================================
 // TestApp_WriteOutputData
 //  Writing Output Data in a File
 */

XDAS_Void TestApp_WriteOutputData(FILE *fOutFile, XDM1_BufDesc * outputBufDesc,
		IIMGDEC1_OutArgs *outArgs, int width, int height)
{

	XDAS_UInt8 *s;
	XDAS_Int32 i;
	int bufsize = outputBufDesc->descs[0].bufSize
			+ outputBufDesc->descs[1].bufSize + outputBufDesc->descs[2].bufSize;
	if (bufsize != width * height * 3 / 2)
	{
		printf("the outbuf size is not righ\n");
		exit(-1);
	}
	unsigned char * yuv = (unsigned char *) malloc(bufsize);
	if (yuv == NULL)
		printf("ERROR yuv\n");
	///////////////////for save yuv file /////////////////////////
	for (i = 0; i < outputBufDesc->numBufs; i++)
	{
		s = (XDAS_UInt8 *) outputBufDesc->descs[i].buf;
		fwrite(s, sizeof(Byte), outputBufDesc->descs[i].bufSize, fOutFile);
	}
	////////////////////////////////////////////////////////////
	memcpy(yuv, outputBufDesc->descs[0].buf, outputBufDesc->descs[0].bufSize);
	memcpy(yuv + outputBufDesc->descs[0].bufSize, outputBufDesc->descs[1].buf,
			outputBufDesc->descs[1].bufSize);
	memcpy(
			yuv + outputBufDesc->descs[0].bufSize
					+ outputBufDesc->descs[1].bufSize,
			outputBufDesc->descs[2].buf, outputBufDesc->descs[2].bufSize);
	//yuv2bmp(yuv, width, height);

	free(yuv);
	fflush(fOutFile);
	return;
}

/*
 //============================================================================
 // TestApp_WriteOutputData
 //  Writing Output Data in a File
 */

#if 0
XDAS_Void dpmProcess(XDM1_BufDesc * outputBufDesc, int width, int height,
		int picNum)
{
	int bufsize = outputBufDesc->descs[0].bufSize
	+ outputBufDesc->descs[1].bufSize + outputBufDesc->descs[2].bufSize;

	if (bufsize != width * height * 3 / 2)
	{
		//printf("the outbuf size is not righ\n");
		write_uart("the outbuf size is not righ\r\n");
		exit(-1);
	}

	unsigned char * yuv = (unsigned char *) malloc(bufsize);
	if (yuv == NULL)
	{
		//printf("ERROR yuv\n");
		write_uart("ERROR yuv\r\n");
	}

	memcpy(yuv, outputBufDesc->descs[0].buf, outputBufDesc->descs[0].bufSize);
	memcpy(yuv + outputBufDesc->descs[0].bufSize, outputBufDesc->descs[1].buf,
			outputBufDesc->descs[1].bufSize);
	memcpy(
			yuv + outputBufDesc->descs[0].bufSize
			+ outputBufDesc->descs[1].bufSize,
			outputBufDesc->descs[2].buf, outputBufDesc->descs[2].bufSize);
	yuv2bmp(yuv, width, height,picNum);

	free(yuv);

	//cyx
#if 0
	unsigned char * rgb = (unsigned char *) malloc(bufsize);
	if (rgb == NULL)
	{
		//printf("ERROR yuv\n");
		write_uart("ERROR yuv\r\n");
	}

	memcpy(rgb, outputBufDesc->descs[0].buf, outputBufDesc->descs[0].bufSize);
	memcpy(rgb + outputBufDesc->descs[0].bufSize, outputBufDesc->descs[1].buf,
			outputBufDesc->descs[1].bufSize);
	memcpy(
			rgb + outputBufDesc->descs[0].bufSize
			+ outputBufDesc->descs[1].bufSize,
			outputBufDesc->descs[2].buf, outputBufDesc->descs[2].bufSize);
	yuv2bmp(yuv, width, height, picNum);

	free(rgb);
#endif
	return;
}
#endif
//  yuv2bmp((unsigned char *)&outputBufDesc);

/*
 XDAS_Void TestApp_WriteOutputData(FILE *fOutFile,
 XDM1_BufDesc * outputBufDesc,
 IIMGDEC1_OutArgs *outArgs)
 {

 XDAS_UInt8 *s;
 XDAS_Int32 i;

 for(i = 0; i < outputBufDesc->numBufs; i++)
 {
 s = (XDAS_UInt8 *)outputBufDesc->descs[i].buf;
 fwrite (s, sizeof (Byte), outputBufDesc->descs[i].bufSize, fOutFile);
 }

 fflush (fOutFile);
 return;
 }
 */
/*
 //==============================================================================
 // TestApp_SetInitParams
 //  setting of creation time parameters
 */

XDAS_Void TestApp_SetInitParams(IIMGDEC1_Params *params1)
{
	IJPEGDEC_Params *params = (IJPEGDEC_Params*) params1;

	params->imgdecParams.dataEndianness = XDM_BYTE;
	params->outImgRes = 1;
	return;
}

/*
 //==============================================================================
 // TestApp_SetDynamicParams
 //  setting of run time parameters
 */

XDAS_Void TestApp_SetDynamicParams(IIMGDEC1_DynamicParams *dynamicParams1)
{
	/* Set IIMGDEC Run time parameters */
	IJPEGDEC_DynamicParams *dynamicParams =
			(IJPEGDEC_DynamicParams*) dynamicParams1;

	/*Do header analysis first to get the decoded image size*/
	dynamicParams->imgdecDynamicParams.decodeHeader = XDM_DECODE_AU;
	dynamicParams->progDisplay = 0;
	dynamicParams->RGB_Format = 0;
	return;
}

XDAS_Void TestApp_SetSizes()
{
	jpegdecParams.imgdecParams.size = sizeof(JPEGDEC_Params);
	jpegdecStatus.imgdecStatus.size = sizeof(JPEGDEC_Status);
	jpegdecDynamicParams.imgdecDynamicParams.size =
			sizeof(JPEGDEC_DynamicParams);
	jpegdecInArgs.imgdecInArgs.size = sizeof(JPEGDEC_InArgs);
	jpegdecOutArgs.imgdecOutArgs.size = sizeof(JPEGDEC_OutArgs);
}

#define EXT_MEM_BASE (0x80000000)
#define EXT_MEM_SIZE (0x07000000)
#define EXT_MEM_NON_CACHE_BASE (0x87000000)
#define EXT_MEM_NON_CACHE_SIZE (0x01000000)

/* Cache Settings */
XDAS_Void TestApp_EnableCache(void)
{
	Cache_Size size;
	UInt32 mar, disablePrefetch, disableCache;

	size.l1pSize = Cache_L1Size_32K; /* L1P cache size */
	size.l1dSize = Cache_L1Size_32K; /* L1D cache size */
	size.l2Size = Cache_L2Size_64K; /* L2 cache size */

	/* Set L1P, L1D and L2 cache sizes */Cache_setSize(&size);

	/* Cache Enable External Memory Space */
	/* BaseAddr, length, MAR enable/disable */
	/* Cache 0x80000000 --- 0x8FFFFFFF   */
	mar =
			Cache_getMar(
					(Ptr *) EXT_MEM_BASE) | ti_sysbios_family_c66_Cache_PC | ti_sysbios_family_c66_Cache_PFX;
	Cache_setMar((Ptr *) EXT_MEM_BASE, EXT_MEM_SIZE, Cache_Mar_ENABLE | mar);

	disablePrefetch = (~ti_sysbios_family_c66_Cache_PFX) & (0xFFFFFFFF);
	disableCache = (~ti_sysbios_family_c66_Cache_PC) & (0xFFFFFFFF);
	mar = Cache_getMar((Ptr *) EXT_MEM_NON_CACHE_BASE) & disableCache
			& disablePrefetch;
	Cache_setMar((Ptr *) EXT_MEM_NON_CACHE_BASE, EXT_MEM_NON_CACHE_SIZE,
			Cache_Mar_DISABLE | mar);
	Cache_wbInvAll();
} /* TestApp_EnableCache */

/******************************************************************************/
/*    Copyright (c) 2011 Texas Instruments, Incorporated                      */
/*    All Rights Reserved                                                     */
/******************************************************************************/
