/* ========================================================================== */
/*                                                                            */
/*  TEXAS INSTRUMENTS, INC.                                                   */
/*                                                                            */
/*  NAME                                                                      */
/*     alg_create.c                                                           */
/*                                                                            */
/*  DESCRIPTION                                                               */
/*    This file contains a simple implementation of the ALG_create API        */
/*    operation.                                                              */
/*                                                                            */
/*  COPYRIGHT NOTICES                                                         */
/*   Copyright (C) 1996, MPEG Software Simulation Group. All Rights           */
/*   Reserved.                                                                */
/*                                                                            */
/*   Copyright (c) 2006 Texas Instruments Inc.  All rights reserved.     */
/*   Exclusive property of the Video & Imaging Products, Emerging End         */
/*   Equipment group of Texas Instruments India Limited. Any handling,        */
/*   use, disclosure, reproduction, duplication, transmission, or storage     */
/*   of any part of this work by any means is subject to restrictions and     */
/*   prior written permission set forth in TI's program license agreements    */
/*   and associated software documentation.                                   */
/*                                                                            */
/*   This copyright notice, restricted rights legend, or other proprietary    */
/*   markings must be reproduced without modification in any authorized       */
/*   copies of any part of this work.  Removal or modification of any part    */
/*   of this notice is prohibited.                                            */
/*                                                                            */
/*   U.S. Patent Nos. 5,283,900  5,392,448                                    */
/* -------------------------------------------------------------------------- */
/*            Copyright (c) 2006 Texas Instruments, Incorporated.             */
/*                           All Rights Reserved.                             */
/* ========================================================================== */
/* "@(#) XDAS 2.12 05-21-01 (__imports)" */
//static const char Copyright[] = "Copyright (C) 2003 Texas Instruments "
//                                "Incorporated. All rights Reserved."; 
#ifdef _TMS320C6400
/* -------------------------------------------------------------------------- */
/* Assigning text section to allow better control on placing function in      */
/* memory of our choice and our alignment. The details about placement of     */
/* these section can be obtained from the linker command file "mpeg2enc.cmd". */
/* -------------------------------------------------------------------------- */
#pragma CODE_SECTION(ALG_create, ".text:create")
#pragma CODE_SECTION(ALG_delete, ".text:delete")
#endif

#ifndef XDM1
//#include <std.h>
#include <ialg.h>
#else
#include <ti/bios/include/std.h>
#include <ti/xdais/ialg.h>
#endif

#include <alg.h>
/*
 #include <std.h>
 #include <alg.h>
 #include <ialg.h>
 */

#include <stdlib.h>

#include <_alg.h>
#ifdef SCRATCH_TEST
extern IALG_MemRec *memTab_scratch;
#endif

extern void write_uart(char* msg);
//char debugInfor[100];

IALG_MemRec gmemTab[4];

/*
 *  ======== ALG_create ========
 */
ALG_Handle ALG_create(IALG_Fxns *fxns, IALG_Handle p, IALG_Params *params)
{

	IALG_MemRec *memTab=gmemTab;
	//IALG_MemRec memTab[4];
	Int n;
	ALG_Handle alg;
	IALG_Fxns *fxnsPtr;


	if (fxns != NULL)
	{
		n = fxns->algNumAlloc != NULL ? fxns->algNumAlloc() : IALG_DEFMEMRECS;
	}
	if(n==0)
	{
		write_uart("algNumAlloc error\n\r");
		n=4;
	}

	//memTab= (IALG_MemRec *) Memory_malloc(n * sizeof(IALG_MemRec));
	//memTab = (IALG_MemRec *) malloc(n * sizeof(IALG_MemRec));
	//memTab = (IALG_MemRec *)malloc(n * 20);
	//write_uart("alg_create step2\n\r");
	if (NULL == memTab)
	{
		write_uart("error\n\r");
		return NULL;
	}

	write_uart("alg_create step3\n\r");
	n = fxns->algAlloc(params, &fxnsPtr, memTab);
	if (n <= 0)
	{
		write_uart("error2\n\r");
		return (NULL);
	}

	if (_ALG_allocMemory(memTab, n))
	{
		alg = (IALG_Handle) memTab[0].base;
		alg->fxns = fxns;
		if (fxns->algInit(alg, memTab, p, params) == IALG_EOK)
		{
#if SCRATCH_TEST

			if ((memTab_scratch = (IALG_MemRec *)malloc(n * sizeof (IALG_MemRec))))
			{
				for(i=0;i<n;i++)
				{
					memTab_scratch[i].base = memTab[i].base;
					memTab_scratch[i].size = memTab[i].size;
					memTab_scratch[i].attrs = memTab[i].attrs;
					memTab_scratch[i].alignment = n;
				}
			}

#endif
			write_uart("jpeg init successufl\n\r");
			//free(memTab);
			return (alg);
		}
		fxns->algFree(alg, memTab);
		//_ALG_freeMemory(memTab, n);
	}

	//free(memTab);

	return (NULL);
}

/*
 *  ======== ALG_delete ========
 */Void ALG_delete(ALG_Handle alg)
{
	IALG_MemRec *memTab;
	Int n;
	IALG_Fxns *fxns;

	if (alg != NULL && alg->fxns != NULL)
	{
		fxns = alg->fxns;
		n = fxns->algNumAlloc != NULL ? fxns->algNumAlloc() : IALG_DEFMEMRECS;

		if ((memTab = (IALG_MemRec *) malloc(n * sizeof(IALG_MemRec))))
		{
			memTab[0].base = alg;
			n = fxns->algFree(alg, memTab);
			_ALG_freeMemory(memTab, n);

			free(memTab);
		}
	}
}
/* ========================================================================== */
/* End of file : alg_create.c                                                 */
/* -------------------------------------------------------------------------- */
/*            Copyright (c) 2006 Texas Instruments, Incorporated.             */
/*                           All Rights Reserved.                             */
/* ========================================================================== */

