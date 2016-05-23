/******************************************************************************/
/*            Copyright (c) 2011 Texas Instruments, Incorporated.             */
/*                           All Rights Reserved.                             */
/******************************************************************************/

/*!
********************************************************************************
  @file     TestAppDecoder.h
  @brief    This is the top level client header file that drives the JPEG
            (Progressive Support) Image Decoder Call using XDM Interface
  @author   Multimedia Codecs TI India
  @version  0.1 - July 23,2011    initial version
********************************************************************************
*/

#ifndef _TESTAPP_DECODER_
#define _TESTAPP_DECODER_

#include <ti/bios/include/std.h>
#include <ti/xdais/xdas.h>
#include <ti/xdais/dm/xdm.h>
#include "ti/xdais/dm/iimgdec1.h"

#define IMAGE_WIDTH            2048      /* Width of the Display Buffer       */      
#define IMAGE_HEIGHT           1600      /* Height of the Display Buffer      */
#define INPUT_BUFFER_SIZE      0x200000  /* Upto 131072 Bytes File size       */
#define OUTPUT_BUFFER_SIZE     IMAGE_WIDTH*IMAGE_HEIGHT*2
#define STRING_SIZE            256
#define NUM_ALGS                 1      /* Number of Algorithms              */


#define PCIE_EP_IRQ_SET		 (0x21800064)
#define DSP_DPM_OVER  (0x00aa5500U)
#define DSP_DPM_CLROVER  (0x0055aa00U)
#define C6678_PCIEDATA_BASE (0x60000000U)
#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)

#define DSP_DPM_STARTSTATUS  (0x0a0a5500U)
#define DSP_DPM_STARTCLR  (0x0505aa00U)
#define DSP_DPM_OVERSTATUS  (0x005a5a00U)
#define DSP_DPM_OVERCLR  (0x00a5a500U)
#define DSP_DPM_ALLOVER  (0x00005a5aU)

#define URLNUM  (4)

/*!
@struct sTokenMapping 
@brief  Token Mapping structure for parsing codec specific configuration file
@param  tokenName : String name in the configuration file
@param  place     : Place Holder for the data
*/

typedef struct _sTokenMapping {
  XDAS_Int8 *tokenName;
  XDAS_Void *place;
} sTokenMapping;




/*!
********************************************************************************
  @fn									      XDAS_Int32 readparamfile(FILE * fname) ;
	@brief							      Parses codec specific parameter file and populates
                            configurable parameters
  @param  fname[IN]         parameter FILE handle
	@return							      NONE
********************************************************************************
*/
XDAS_Int32 readparamfile(FILE * fname) ;

#endif //_TESTAPP_DECODER_

/******************************************************************************/
/*    Copyright (c) 2011 Texas Instruments, Incorporated                      */
/*    All Rights Reserved                                                     */
/******************************************************************************/
