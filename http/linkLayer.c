#include <stdlib.h>
#include <stdint.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/family/c66/tci66xx/CpIntc.h>
#include "LinkLayer.h"
//typedef unsigned short Bool; /* boolean */

#define PCIE_EP_IRQ_SET		 (0x21800064)
//#define TRUE (1)
#define FASLE (0)
#define PCIE_BASE_ADDRESS            0x21800000
#define EP_IRQ_CLR                   0x68
#define EP_IRQ_STATUS                0x6C
//(0x180 + PCIE_BASE_ADDRESS)
#define LEGACY_A_IRQ_STATUS_RAW      (0x21800180)
#define LEGACY_A_IRQ_ENABLE_SET      (0x21800188)
#define LEGACY_A_IRQ_ENABLE_CLR      (0x2180018C)
#define POLL_COUNT_MAX (0x0FFFFFFF)

#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)
#define DEVICE_REG32_R(x)    (*(volatile uint32_t *)(x))
#define PAGE_SIZE (0x1000)
//the ddr read address space zone in DSP mapped to the PC.
#define DDR_WRITE_MMAP_START (0x80B00000)
#define DDR_WRITE_MMAP_LENGTH (0x00400000)
#define DDR_WRITE_MMAP_USED_LENGTH (0x00400000)
//the ddr read address space zone in DSP mapped to the PC.
#define DDR_READ_MMAP_START (0x80F00000)
#define DDR_READ_MMAP_LENGTH (0x00100000)
#define DDR_READ_MMAP_USED_LENGTH (0x00100000)
//expand a 4K space at the end of DDR_READ_MMAP zone as read and write flag
#define DDR_REG_PAGE_START (DDR_READ_MMAP_START + DDR_READ_MMAP_LENGTH - PAGE_SIZE)
#define DDR_REG_PAGE_USED_LENGTH (PAGE_SIZE)

#define OUT_REG (0x60000000)
#define IN_REG (0x60000004)
#define WR_REG (0x60000008)

#define WAITTIME (0x0FFFFFFF)

#define RINIT (0xaa55aa55)
#define READABLE (0x0)
#define RFINISH (0xaa55aa55)

#define WINIT (0x0)
#define WRITEABLE (0x0)
#define WFINISH (0x55aa55aa)
#define WRFLAG (0xFFAAFFAA)

extern Semaphore_Handle g_readSemaphore;
extern Semaphore_Handle g_writeSemaphore;
/*
 extern volatile uint8_t *g_pOutBufFlagReg;
 extern volatile uint8_t *g_pInBufFlagReg;
 extern volatile uint8_t *g_pPcReadOrWriteReg;
 extern volatile uint8_t *g_pDspReadOrWriteReg;
 */
#if 0
extern unsigned char g_outBuffer[0x00600000]; //20M

extern unsigned char g_inBuffer[0x00100000]; //1M.
#if 0
inFifo *initFifo()
{
	inFifo *pInFifo = (inFifo *) malloc(sizeof(inFifo));
	if (pInFifo != NULL)
	{
		pInFifo->pFrontNode = NULL;
		pInFifo->pRearNode = NULL;
		pInFifo->nodeNumber = 0;
	}
	else
	{
	}
	return pInFifo;
}

int isEmpty(inFifo *pInFifo)
{
	int retValue = 0;
	if ((pInFifo->pFrontNode == NULL) && (pInFifo->pRearNode == NULL)
			&& (pInFifo->nodeNumber == 0))
	{
		retValue = 1;
	}
	else
	{
	}
	return (retValue);
}

int enFifo(inFifo *pInFifo, unsigned char *pUrl)
{
	int retValue = 0;
	if (pInFifo->nodeNumber < MAX_NODE_NUM)
	{
		pNode_t pNode = (pNode_t) malloc(sizeof(node));
		if (pNode != NULL)
		{
			pNode->pUrl = pUrl;
			pNode->pNextNode = NULL;

			if (isEmpty(pInFifo) == 1)
			{
				pInFifo->pFrontNode = pNode;
			}
			else
			{
				pInFifo->pRearNode = pNode;
			}
			pInFifo->pRearNode = pNode;

			pInFifo->nodeNumber++;
		}
		else
		{
			retValue = -1;
		}
	}
	else
	{
		//FIFO full
		retValue = -2;
	}
	return (retValue);
}

int deFifo(inFifo *pInFifo, unsigned char *pUrl)
{
	int retValue = 0;
	pNode_t pNode = pInFifo->pFrontNode;
	if ((isEmpty(pInFifo) != 1) && (pNode != NULL))
	{
		pUrl = pNode->pUrl;
		pInFifo->nodeNumber--;
		pInFifo->pFrontNode = pNode->pNextNode;
		if (pInFifo->nodeNumber == 0)
		{
			pInFifo->pRearNode = NULL;
		}
		else
		{
		}
	}
	else
	{
		retValue = -1;
	}
	return (retValue);
}
#endif
/*
void LinkLayer_Open(uint32_t *pRegMem,LinkLayerHandler **ppLLHandler)
{
	*ppLLHandler=(LinkLayerHandler *)malloc(sizeof)
}
*/
int LinkLayer_Read(uint8_t *pBuffer, int length)
{
	int retValue = 0;
	int pollCount = 0;
	unsigned char *pDest = NULL;
	Bool bTimeNotOut = TRUE;

	if (pBuffer == NULL)
	{
		//write_uart("the inBuffer is NULL in the DSPread\r\n");
		retValue = -1;
	}
	else
	{
	}

// wait inBuffer is ready.

	if (retValue == 0)
	{
		while ((DEVICE_REG32_R(IN_REG) == 0) && ((pollCount < POLL_COUNT_MAX)))
		{
			pollCount++;
		}
		if (pollCount > POLL_COUNT_MAX)
		{
			//write_uart("the wait inBuffer.time is out in DSPread\r\n");
			retValue = -2;
			Semaphore_pend(g_readSemaphore, BIOS_WAIT_FOREVER);
		}
		else
		{
		}
	}
	else
	{
	}

//	Semaphore_pend(g_readSemaphore,BIOS_WAIT_FOREVER);
//TODO:read the inBuffer
	if (retValue == 0)
	{
		//pDest = (uint8_t *) g_inBuffer + 0x400000 + 0x1000;
		//memcpy(pBuffer, g_inBuffer, length);
	}
	else
	{
	}

//TODO:set the inBufferFlagReg
	if (retValue == 0)
	{
		DEVICE_REG32_W(IN_REG, 0);
		//write_uart("the read function set the inflag to 0\r\n");
		//(*g_pInBufFlagReg) == 0;
		//*((uint8_t *) (0x81100000 + 0x00200000 - 0x1000 + 0x00000001)) = 0;
	}
	else
	{
	}

	//write_uart("DSP read over in DSPread\r\n");
	return retValue;
}

int LinkLayer_Write(uint8_t *pBuffer, int length)
{
	int retValue = 0;
	int pollCount = 0;
	Bool bTimeNotOut = TRUE;
	//DSP read and wait the sema.
	if (pBuffer == NULL)
	{
		//write_uart("the outBuffer is NULL in the DSPwrite\r\n");
		retValue = -1;
	}
	else
	{

	}

//TODO:wait the outBuffer ready.
	if (retValue == 0)
	{
		while ((DEVICE_REG32_R(OUT_REG) == 1) && ((pollCount < POLL_COUNT_MAX)))
		{
			pollCount++;
		}
		if (pollCount > POLL_COUNT_MAX)
		{
			//write_uart("the wait outBuffer.time is out in DSPwrite\r\n");
			retValue = -2;
		}
	}
	else
	{
	}

//TODO:write the outBuffer.
	if (retValue == 0)
	{
		memcpy(g_outBuffer, pBuffer, 0x600000);
	}
	else
	{
	}

//TODO:set the outBufferFlagReg.
	if (retValue == 0)
	{
		DEVICE_REG32_W(OUT_REG, 1);
		//*((uint8_t *) (0x81100000 + 0x00200000 - 0x1000)) = 1;
	}
	else
	{
	}

	//write_uart("DSP write over in DSPwrite\r\n");
	return (retValue);
}
#endif
