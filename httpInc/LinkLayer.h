/*
 * linkLayer.h
 *
 *  Created on: Sep 7, 2015
 *      Author: spark
 */

#ifndef LINKLAYER_H_
#define LINKLAYER_H_

#ifndef NULL
#define NULL            ((void*)0)
#endif

#define MAX_NODE_NUM (14)
#if 0
typedef struct node * pNode_t;
typedef struct node
{
	unsigned char *pUrl;
	pNode_t pNextNode;
}node;

typedef struct inFifo
{
	pNode_t pFrontNode;
	pNode_t pRearNode;
	int nodeNumber;
}inFifo;
//int g_readFlag,g_writeFlag;

inFifo *initFifo();
int isEmpty(inFifo *pInFifo);
int enFifo(inFifo *pInFifo, unsigned char *pUrl);
int deFifo(inFifo *pInFifo, unsigned char *pUrl);
int feFifo(inFifo *pInFifo);
#endif
typedef struct _tagRegisterTable
{
	// control registers. (4k)
	uint32_t DPUBootControl;
	uint32_t readControl;
	uint32_t writeControl;
	uint32_t getPicNumers;
	uint32_t failPicNumers;
	uint32_t dpmOverControl;
	uint32_t dpmStartControl;
	uint32_t dpmAllOverControl;
	uint32_t reserved0[0x1000 / 4 - 8];

	// status registers. (4k)
	uint32_t DPUBootStatus;
	uint32_t readStatus;
	uint32_t writeStatus;
	uint32_t DSP_urlNumsReg;
	uint32_t DSP_modelType;//1:motor 2:car 3:person
	uint32_t dpmOverStatus;
	uint32_t dpmStartStatus;
	uint32_t reserved1[0x1000 / 4 - 6];
} registerTable; //DSP
#if 0
typedef struct _tagLinkLayerRegisterTable
{
	// status registers. (4k)
	uint32_t DPUBootStatus;
	uint32_t writeStatus;
	uint32_t readStatus;
	uint32_t registerPhyAddrInPc;
	uint32_t reserved0[0x1000 / 4 - 4];

	// control registers. (4k)
	uint32_t DPUBootControl;
	uint32_t writeControl;
	uint32_t readControl;
	uint32_t reserved1[0x1000 / 4 - 3];
}LinkLayerRegisterTable; //PC
#endif
typedef struct _tagLinkLayerHandler
{
	registerTable *pRegisterTable;
	uint32_t *pOutBuffer;
	uint32_t *pInBuffer;
	uint32_t outBufferLength;
	uint32_t inBufferLength;

	// try removing global var.
	uint32_t *pWriteConfirmReg;
	uint32_t *pReadConfirmReg;
} LinkLayerHandler, *LinkLayerHandlerPtr;
/*
void LinkLayer_Open(uint32_t *pRegMem);
int LinkLayer_Read(uint8_t *pBuffer, int length);
int LinkLayer_Write(uint8_t *pBuffer, int length);
*/
#endif /* LINKLAYER_H_ */

