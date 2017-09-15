/*
 *  ======== client.c ========
 *
 * TCP/IP Network Client example ported to use BIOS6 OS.
 *
 * Copyright (C) 2007, 2011 Texas Instruments Incorporated - http://www.ti.com/
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
//#include <ti/ndk/inc/netmain.h>
//#include <ti/ndk/inc/_stack.h>
//#include <ti/ndk/inc/tools/console.h>
//#include <ti/ndk/inc/tools/servers.h>

/* BIOS6 include */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/family/c66/tci66xx/CpIntc.h>
#include <ti/sysbios/knl/Semaphore.h>
/* Platform utilities include */
#include "ti/platform/platform.h"

//add the TI's interrupt component.   add by LHS
#include <ti/sysbios/family/c66/tci66xx/CpIntc.h>
#include <ti/sysbios/hal/Hwi.h>

/* Resource manager for QMSS, PA, CPPI */
#include "ti/platform/resource_mgr.h"

#include "http.h"
#include "DPMMain.h"
#include "LinkLayer.h"
#include "jpegDecoder.h"

extern Semaphore_Handle g_getJpegSrc;
extern Semaphore_Handle g_dpmProcBg;
extern Semaphore_Handle gRecvSemaphore;
extern Semaphore_Handle gSendSemaphore;

extern Semaphore_Handle timeoutSemaphore;

extern Semaphore_Handle pcFinishReadSemaphore;

#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)
#define DEVICE_REG32_R(x)    (*(volatile uint32_t *)(x))

#define CHIP_LEVEL_REG (0x02620000)
#define KICK0 (CHIP_LEVEL_REG+0x0038)
#define KICK1 (CHIP_LEVEL_REG+0x003C)

#define DDR_TEST_START                 0x80000000
#define DDR_TEST_END                   0x80400000
#define BOOT_UART_BAUDRATE                 115200

#define PCIEXpress_Legacy_INTA                 50
#define PCIEXpress_Legacy_INTB                 50
/*
 #define PCIE_IRQ_EOI                   0x21800050
 #define PCIE_EP_IRQ_SET		           0x21800064
 #define PCIE_LEGACY_A_IRQ_STATUS       0x21800184
 #define PCIE_LEGACY_A_IRQ_RAW          0x21800180
 #define PCIE_LEGACY_A_IRQ_SetEnable       0x21800188
 */
//#define PCIE_IRQ_EOI                   0x21800050
#define PCIE_LEGACY_A_IRQ_STATUS       0x21800184

#ifdef _EVMC6678L_
#define MAGIC_ADDR     (0x87fffc)
#define INTC0_OUT3     63
#endif

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

//#define MAGIC_ADDR          		(0x0087FFFC)
#define GBOOT_MAGIC_ADDR(coreNum)			((1<<28) + ((coreNum)<<24) + (MAGIC_ADDR))
#define CORE0_MAGIC_ADDR                   0x1087FFFC
#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)
//extern int g_flag;
//extern int g_flag;
//#pragma DATA_SECTION(g_outBuffer,".WtSpace");
//unsigned char g_outBuffer[0x00600000]; //4M
//#pragma DATA_SECTION(g_inBuffer,".RdSpace");
unsigned char g_inBuffer[0x00100000]; //url value.
//add the SEM mode .    add by LHS

/* Platform Information - we will read it form the Platform Library */
platform_info gPlatformInfo;

extern int g_flag_DMA_finished;

//extern cregister unsigned int DNUM;

//debug infor
static char debugBuf[100];
int debuginfoLength = 0;

#define IPC_INT_ADDR(coreNum)				(0x02620240 + ((coreNum)*4))
#define IPC_AR_ADDR(coreNum)				(0x02620280+((coreNum)*4))
//------------------------------------------------------------------
// 512K*4*7=0x00e00000
#define inBufSize (0x00200000)
#pragma DATA_SECTION(coreNInBuf,".coreNInBuf");
unsigned char coreNInBuf[0x00e00000];
unsigned char *pCore1InBuf = coreNInBuf;
unsigned char *pCore2InBuf = coreNInBuf + inBufSize;
unsigned char *pCore3InBuf = coreNInBuf + (inBufSize * 2);
unsigned char *pCore4InBuf = coreNInBuf + (inBufSize * 3);
unsigned char *pCore5InBuf = coreNInBuf + (inBufSize * 4);
unsigned char *pCore6InBuf = coreNInBuf + (inBufSize * 5);
unsigned char *pCore7InBuf = coreNInBuf + (inBufSize * 6);

// 7M*4*7=196M
#define OutBufSize (0x01C00000)
#pragma DATA_SECTION(coreNOutBuf,".coreNOutBuf");
unsigned char coreNOutBuf[0x0c400000];
unsigned char *pCore1OutBuf = coreNOutBuf;
unsigned char *pCore2OutBuf = coreNOutBuf + OutBufSize;
unsigned char *pCore3OutBuf = coreNOutBuf + (OutBufSize * 2);
unsigned char *pCore4OutBuf = coreNOutBuf + (OutBufSize * 3);
unsigned char *pCore5OutBuf = coreNOutBuf + (OutBufSize * 4);
unsigned char *pCore6OutBuf = coreNOutBuf + (OutBufSize * 5);
unsigned char *pCore7OutBuf = coreNOutBuf + (OutBufSize * 6);

PicInfor *p_gPictureInfor;

extern volatile cregister unsigned int DNUM;

//#define DEBUG_PROCESS
#ifdef DEBUG_PROCESS
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
#else
void write_uart(char *msg)
{
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////
static void isrHandler(void* handle)
{
#if 0
	char debugInfor[100];
	registerTable *pRegisterTable = (registerTable *) C6678_PCIEDATA_BASE;
	CpIntc_disableHostInt(0, 3);

	sprintf(debugInfor, "pRegisterTable->dpmStartStatus is %x \r\n",
			pRegisterTable->dpmStartStatus);
	write_uart(debugInfor);
	if ((pRegisterTable->dpmStartStatus) & DSP_DPM_STARTSTATUS)

	{
		Semaphore_post(gRecvSemaphore);
		//clear interrupt reg
		pRegisterTable->dpmStartControl = 0x0;

	}
	if ((pRegisterTable->dpmOverStatus) & DSP_DPM_OVERSTATUS)

	{
		Semaphore_post(pcFinishReadSemaphore);
		pRegisterTable->dpmStartControl = DSP_DPM_STARTCLR;

	}
	if ((pRegisterTable->readStatus) & DSP_RD_READY)
	{
		Semaphore_post(g_getJpegSrc);
	}
	if ((pRegisterTable->writeStatus) & DSP_WT_READY)

	{
		Semaphore_post(g_writeSemaphore);
	}

	//clear PCIE interrupt
	DEVICE_REG32_W(PCIE_LEGACY_A_IRQ_STATUS, 0x1);
	DEVICE_REG32_W(PCIE_IRQ_EOI, 0x0);
	CpIntc_clearSysInt(0, PCIEXpress_Legacy_INTA);

	CpIntc_enableHostInt(0, 3);
#endif
}

void ipcIrqHandler(UArg params);
extern void DPMMain();
static void registeIPCint()
{
	// IPC interrupt set.
	int ipcEventId = 91;
	Hwi_Params hwiParams;
	Hwi_Params_init(&hwiParams);
	hwiParams.arg = ipcEventId;
	hwiParams.eventId = ipcEventId;
	hwiParams.enableInt = TRUE;
	Hwi_create(5, (Hwi_FuncPtr) ipcIrqHandler, &hwiParams, NULL);
	Hwi_enable();
}
// get interrupt from Core0 can be read the picture from CoreN.
void ipcIrqHandler(UArg params)
{
	unsigned int ipcACKregVal = 0;
	unsigned int ipcACKval = 0;
	//read the IPC_AR_ADDR(0)
	ipcACKregVal = DEVICE_REG32_R(IPC_AR_ADDR(DNUM));
	//identify the interrupt source by the SRCCn bit of the IPC_AR_ADDR(0)
	ipcACKval = (ipcACKregVal >> 4);
	//process
	//if (0 != ipcACKval)
	{
		Semaphore_post(g_getJpegSrc);
	}
	//restore the IPC_CG_ADDR(0) by write the IPC_AR_ADDR(0) that read Value.
	//DEVICE_REG32_W(IPC_AR_ADDR(DNUM), ipcACKregVal);
	//write_uart("core1:receive the pic from core0\n\r");
}
//Cache_wbInv();// use after write.
//Cache_wb();
//Cache_inv();  // use before read.
//Cache_wait()
void triggleIPCinterrupt(int destCoreNum, unsigned int srcFlag)
{
	DEVICE_REG32_W(KICK0, 0x83e70b13);
	DEVICE_REG32_W(KICK1, 0x95a4f1e0);
	unsigned int writeValue = ((srcFlag << 1) | 0x01);
	//DEVICE_REG32_W((IPC_INT_ADDR(destCoreNum)), writeValue);
	DEVICE_REG32_W((IPC_INT_ADDR(destCoreNum)), writeValue);
}
int main()
{

//	if (DNUM == 1)
//	{
//		write_uart("core1 running\r\n");
//	}
	DEVICE_REG32_W(KICK0, 0x83e70b13);
	DEVICE_REG32_W(KICK1, 0x95a4f1e0);
	uint32_t L2RAM_MultiCoreBoot = (0x1087ffff - 8 * sizeof(uint32_t));

	*((uint32_t *) (L2RAM_MultiCoreBoot + DNUM * 4)) = 0x00000001;

	//TODO:
	// create the Task ,receive the input picture frome the core0.
	// create the Task ,process the picture.
	// create the Task ,output the picture.

	/**(volatile uint32_t *)(0x0087fffc)=0xBABEFACE;
	 write_uart("hello world!!!\r\n");
	 while(1)
	 {
	 ;
	 }*/
	registeIPCint();
	BIOS_start();
}

int StackTest()
{

	//if (DNUM == 1)
	unsigned char *pSrc = NULL;
	switch (DNUM)
	{
	case 1:
	{
		//todo:
		pSrc = pCore1InBuf;
		break;
	}
	case 2:
	{
		pSrc = pCore2InBuf;
		break;
	}
	case 3:
	{
		pSrc = pCore3InBuf;
		break;
	}
	case 4:
	{
		pSrc = pCore4InBuf;
		break;
	}
	case 5:
	{
		pSrc = pCore5InBuf;
		break;
	}
	case 6:
	{
		pSrc = pCore6InBuf;
		break;
	}
	case 7:
	{
		pSrc = pCore7InBuf;
		break;
	}
	}

	{
		int maxCount = 4; //todo: will be adjust.
		int countIndex = 0;
		unsigned int picLen = 0;

		p_gPictureInfor = (PicInfor *) malloc(sizeof(PicInfor));

		Semaphore_pend(g_getJpegSrc, BIOS_WAIT_FOREVER);

		write_uart("getINTfromCore0\n\r");
		while (countIndex < maxCount)
		{
			picLen = *(int *) pSrc;
			pSrc += 4;
			p_gPictureInfor->picSrcAddr[countIndex] = pSrc;
			p_gPictureInfor->picSrcLength[countIndex] = picLen;

			sprintf(debugBuf, "length=%d-->%d,countIndex=%d\n\r", picLen,
					(p_gPictureInfor->picSrcLength[countIndex]), countIndex);
			write_uart(debugBuf);
			countIndex++;
			pSrc += ((picLen + 3) / 4) * 4;

		}
		//todo Semaphore_post();for the dpm.
		Semaphore_post(g_dpmProcBg);
		triggleIPCinterrupt(0, 4);

	}

}
