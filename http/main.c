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
#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/inc/_stack.h>
#include <ti/ndk/inc/tools/console.h>
#include <ti/ndk/inc/tools/servers.h>

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

extern Semaphore_Handle gRecvSemaphore;
extern Semaphore_Handle gSendSemaphore;

extern Semaphore_Handle timeoutSemaphore;

extern Semaphore_Handle g_readSemaphore;
extern Semaphore_Handle g_writeSemaphore;
extern Semaphore_Handle pcFinishReadSemaphore;

#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)
#define DEVICE_REG32_R(x)    (*(volatile uint32_t *)(x))

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
#define PCIE_IRQ_EOI                   0x21800050
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

//extern int g_flag;
//extern int g_flag;
//#pragma DATA_SECTION(g_outBuffer,".WtSpace");
//unsigned char g_outBuffer[0x00600000]; //4M
//#pragma DATA_SECTION(g_inBuffer,".RdSpace");
unsigned char g_inBuffer[0x00100000]; //url value.
//add the SEM mode .    add by LHS

/* Platform Information - we will read it form the Platform Library */
platform_info gPlatformInfo;

//---------------------------------------------------------------------------
// Title String
//
char *VerStr = "\nTCP/IP Stack 'Hello World!' Application\n\n";

// Our NETCTRL callback functions
static void NetworkOpen();
static void NetworkClose();
static void NetworkIPAddr(IPN IPAddr, uint IfIdx, uint fAdd);

extern int http_get();
//extern void DPMMain();
// Fun reporting function
static void ServiceReport(uint Item, uint Status, uint Report, HANDLE hCfgEntry);

// External references
extern int dtask_udp_hello();
extern int g_flag_DMA_finished;

//debug infor
static char debuginfo[100];
int debuginfoLength = 0;
//---------------------------------------------------------------------------
// Configuration
//
char *LocalIPAddr = "192.168.30.100"; // Set to "0.0.0.0" for DHCP client option
char *PCStaticIP = "192.168.30.124"; // Static IP address for host PC
char *EVMStaticIP = "192.168.30.100"; //    "   IP     "   for EVM
char *LocalIPMask = "255.255.255.0"; // Mask for DHCP Server option
char *GatewayIP = "192.168.30.100"; // Not used when using DHCP
char *DomainName = "demo.net"; // Not used when using DHCP
char *HostName = "tidsp";
char *DNSServer = "0.0.0.0";

// Simulator EMAC Switch does not handle ALE_LEARN mode, so please configure the
// MAC address of the PC where you want to launch the webpages and initiate PING to NDK */

Uint8 clientMACAddress[6] =
{ 0x00, 0x15, 0xE9, 0x85, 0x8A, 0x0A }; /* MAC address for my PC */

UINT8 DHCP_OPTIONS[] =
{ DHCPOPT_SERVER_IDENTIFIER, DHCPOPT_ROUTER };

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
#if 1
/////////////////////////////////////////////////////////////////////////////////////////////
static void isrHandler(void* handle)
{
	char debugInfor[100];
	registerTable *pRegisterTable = (registerTable *) C6678_PCIEDATA_BASE;
	CpIntc_disableHostInt(0, 3);

	sprintf(debugInfor,"pRegisterTable->dpmStartStatus is %x \r\n",
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
		Semaphore_post(g_readSemaphore);
	}
	if ((pRegisterTable->writeStatus) & DSP_WT_READY)

	{
		Semaphore_post(g_writeSemaphore);
	}



	//clear PCIE interrupt
			DEVICE_REG32_W (PCIE_LEGACY_A_IRQ_STATUS,0x1);
	DEVICE_REG32_W(PCIE_IRQ_EOI, 0x0);
	CpIntc_clearSysInt(0, PCIEXpress_Legacy_INTA);

	CpIntc_enableHostInt(0, 3);
}
#endif

int main()
{
	write_uart("Debug: BIOS_start\n\r");
	BIOS_start();
}

int StackTest()
{

	int rc;
	int i;

	int EventID_intc;
	Hwi_Params HwiParam_intc;
	registerTable *pRegisterTable = (registerTable *) C6678_PCIEDATA_BASE;

	HANDLE hCfg;

	QMSS_CFG_T qmss_cfg;
	CPPI_CFG_T cppi_cfg;
	/* Initialize the components required to run this application:
	 *  (1) QMSS
	 *  (2) CPPI
	 *  (3) Packet Accelerator
	 */
	/* Initialize QMSS */
	if (platform_get_coreid() == 0)
	{
		qmss_cfg.master_core = 1;
	}
	else
	{
		qmss_cfg.master_core = 0;
	}
	qmss_cfg.max_num_desc = MAX_NUM_DESC;
	qmss_cfg.desc_size = MAX_DESC_SIZE;
	qmss_cfg.mem_region = Qmss_MemRegion_MEMORY_REGION0;
	if (res_mgr_init_qmss(&qmss_cfg) != 0)
	{
		platform_write("Failed to initialize the QMSS subsystem \n");
		goto main_exit;
	}
	else
	{
		platform_write("QMSS successfully initialized \n");
	}

	/* Initialize CPPI */
	if (platform_get_coreid() == 0)
	{
		cppi_cfg.master_core = 1;
	}
	else
	{
		cppi_cfg.master_core = 0;
	}
	cppi_cfg.dma_num = Cppi_CpDma_PASS_CPDMA;
	cppi_cfg.num_tx_queues = NUM_PA_TX_QUEUES;
	cppi_cfg.num_rx_channels = NUM_PA_RX_CHANNELS;
	if (res_mgr_init_cppi(&cppi_cfg) != 0)
	{
		platform_write("Failed to initialize CPPI subsystem \n");
		goto main_exit;
	}
	else
	{
		platform_write("CPPI successfully initialized \n");
	}

	if (res_mgr_init_pass() != 0)
	{
		platform_write("Failed to initialize the Packet Accelerator \n");
		goto main_exit;
	}
	else
	{
		platform_write("PA successfully initialized \n");
	}
///////////////////////////////////////////////////////////////
	//add the TI's interrupt component.   add by LHS
	//Add the interrupt componet.
	/*
	 id -- Cp_Intc number
	 sysInt -- system interrupt number
	 hostInt -- host interrupt number
	 */

	CpIntc_mapSysIntToHostInt(0, PCIEXpress_Legacy_INTA, 3);
	//modify by cyx
	//CpIntc_mapSysIntToHostInt(0, PCIEXpress_Legacy_INTB, 3);
	/*
	 sysInt -- system interrupt number
	 fxn -- function
	 arg -- argument to function
	 unmask -- bool to unmask interrupt
	 */
	CpIntc_dispatchPlug(PCIEXpress_Legacy_INTA, (CpIntc_FuncPtr) isrHandler, 15,
			TRUE);

	/*
	 id -- Cp_Intc number
	 hostInt -- host interrupt number
	 */
	CpIntc_enableHostInt(0, 3);
	//hostInt -- host interrupt number
	EventID_intc = CpIntc_getEventId(3);
	//HwiParam_intc
	Hwi_Params_init(&HwiParam_intc);
	HwiParam_intc.arg = 3;
	HwiParam_intc.eventId = EventID_intc; //eventId
	HwiParam_intc.enableInt = 1;
	/*
	 intNum -- interrupt number
	 hwiFxn -- pointer to ISR function
	 params -- per-instance config params, or NULL to select default values (target-domain only)
	 eb -- active error-handling block, or NULL to select default policy (target-domain only)
	 */
	Hwi_create(4, &CpIntc_dispatch, &HwiParam_intc, NULL);

	//
	// THIS MUST BE THE ABSOLUTE FIRST THING DONE IN AN APPLICATION before
	//  using the stack!!
	//

	//clear interrupt
	pRegisterTable->dpmStartControl = DSP_DPM_STARTCLR;

	rc = NC_SystemOpen(NC_PRIORITY_LOW, NC_OPMODE_INTERRUPT);
	if (rc)
	{
		platform_write("NC_SystemOpen Failed (%d)\n", rc);
		for (;;)
			;
	}

	// Print out our banner
	platform_write(VerStr);

	//
	// Create and build the system configuration from scratch.
	//

	// Create a new configuration
	hCfg = CfgNew();
	if (!hCfg)
	{
		platform_write("Unable to create configuration\n");
		goto main_exit;
	}

	// We better validate the length of the supplied names
	if (strlen(DomainName) >= CFG_DOMAIN_MAX
			|| strlen(HostName) >= CFG_HOSTNAME_MAX)
	{
		platform_write("Names too long\n");
		goto main_exit;
	}

	// Add our global hostname to hCfg (to be claimed in all connected domains)
	CfgAddEntry(hCfg, CFGTAG_SYSINFO, CFGITEM_DHCP_HOSTNAME, 0,
			strlen(HostName), (UINT8 *) HostName, 0);

	// If the IP address is specified, manually configure IP and Gateway
#if defined(_SCBP6618X_) || defined(_EVMTCI6614_)
	/* SCBP6618x & EVMTCI6614 always uses DHCP */
	if (0)
#else
	//    if (!platform_get_switch_state(1))
	if (inet_addr(LocalIPAddr))
#endif
	{
		CI_IPNET NA;
		CI_ROUTE RT;
		IPN IPTmp;

		//sprintf(LocalIPAddr, "192.168.1.%d", get_DSP_id() + 101);
		// Setup manual IP address
		bzero( &NA, sizeof(NA));
		NA.IPAddr = inet_addr(LocalIPAddr);
		NA.IPMask = inet_addr(LocalIPMask);
		strcpy(NA.Domain, DomainName);
		NA.NetType = 0;

		// Add the address to interface 1
		CfgAddEntry(hCfg, CFGTAG_IPNET, 1, 0, sizeof(CI_IPNET), (UINT8 *) &NA,
				0);

		// Add the default gateway. Since it is the default, the
		// destination address and mask are both zero (we go ahead
		// and show the assignment for clarity).
		bzero( &RT, sizeof(RT));
		RT.IPDestAddr = 0;
		RT.IPDestMask = 0;
		RT.IPGateAddr = inet_addr(GatewayIP);

		// Add the route
		CfgAddEntry(hCfg, CFGTAG_ROUTE, 0, 0, sizeof(CI_ROUTE), (UINT8 *) &RT,
				0);
#if 0
		// Manually add the DNS server when specified
		IPTmp = inet_addr(DNSServer);
		if (IPTmp)
		CfgAddEntry(hCfg, CFGTAG_SYSINFO, CFGITEM_DHCP_DOMAINNAMESERVER, 0,
				sizeof(IPTmp), (UINT8 *) &IPTmp, 0);
#endif
	}
#if 0
	// Else we specify DHCP
	else
	{
		CI_SERVICE_DHCPC dhcpc;

		platform_write("Configuring DHCP client\n");

		// Specify DHCP Service on IF-1
		bzero( &dhcpc, sizeof(dhcpc));
		dhcpc.cisargs.Mode = CIS_FLG_IFIDXVALID;
		dhcpc.cisargs.IfIdx = 1;
		dhcpc.cisargs.pCbSrv = &ServiceReport;
		dhcpc.param.pOptions = DHCP_OPTIONS;
		dhcpc.param.len = 2;

		CfgAddEntry(hCfg, CFGTAG_SERVICE, CFGITEM_SERVICE_DHCPCLIENT, 0,
				sizeof(dhcpc), (UINT8 *) &dhcpc, 0);
	}
#endif
#if 0
	// Specify TELNET service for our Console example
	bzero( &telnet, sizeof(telnet));
	telnet.cisargs.IPAddr = INADDR_ANY;
	telnet.cisargs.pCbSrv = &ServiceReport;
	telnet.param.MaxCon = 2;
	telnet.param.Callback = &ConsoleOpen;
	CfgAddEntry(hCfg, CFGTAG_SERVICE, CFGITEM_SERVICE_TELNET, 0, sizeof(telnet),
			(UINT8 *) &telnet, 0);

	// Create RAM based WEB files for HTTP
	//AddWebFiles();

	// HTTP Authentication
	{
		CI_ACCT CA;

		// Name our authentication group for HTTP (Max size = 31)
		// This is the authentication "realm" name returned by the HTTP
		// server when authentication is required on group 1.
		CfgAddEntry(hCfg, CFGTAG_SYSINFO, CFGITEM_SYSINFO_REALM1, 0, 30,
				(UINT8 *) "DSP_CLIENT_DEMO_AUTHENTICATE1", 0);

		// Create a sample user account who is a member of realm 1.
		// The username and password are just "username" and "password"
		strcpy(CA.Username, "username");
		strcpy(CA.Password, "password");
		CA.Flags = CFG_ACCTFLG_CH1;// Make a member of realm 1
		rc = CfgAddEntry(hCfg, CFGTAG_ACCT, CFGITEM_ACCT_REALM, 0,
				sizeof(CI_ACCT), (UINT8 *) &CA, 0);
	}

	// Specify HTTP service
	bzero( &http, sizeof(http));
	http.cisargs.IPAddr = INADDR_ANY;
	http.cisargs.pCbSrv = &ServiceReport;
	CfgAddEntry(hCfg, CFGTAG_SERVICE, CFGITEM_SERVICE_HTTP, 0, sizeof(http),
			(UINT8 *) &http, 0);

	//
	// Configure IPStack/OS Options
	//
#endif
	// We don't want to see debug messages less than WARNINGS
	rc = DBG_INFO;
	CfgAddEntry(hCfg, CFGTAG_OS, CFGITEM_OS_DBGPRINTLEVEL, CFG_ADDMODE_UNIQUE,
			sizeof(uint), (UINT8 *) &rc, 0);

	//
	// This code sets up the TCP and UDP buffer sizes
	// (Note 8192 is actually the default. This code is here to
	// illustrate how the buffer and limit sizes are configured.)
	//

	// TCP Transmit buffer size
	rc = 8192;
	CfgAddEntry(hCfg, CFGTAG_IP, CFGITEM_IP_SOCKTCPTXBUF, CFG_ADDMODE_UNIQUE,
			sizeof(uint), (UINT8 *) &rc, 0);

	// TCP Receive buffer size (copy mode)
	rc = 8192;
	CfgAddEntry(hCfg, CFGTAG_IP, CFGITEM_IP_SOCKTCPRXBUF, CFG_ADDMODE_UNIQUE,
			sizeof(uint), (UINT8 *) &rc, 0);

	// TCP Receive limit (non-copy mode)
	rc = 8192;
	CfgAddEntry(hCfg, CFGTAG_IP, CFGITEM_IP_SOCKTCPRXLIMIT, CFG_ADDMODE_UNIQUE,
			sizeof(uint), (UINT8 *) &rc, 0);

	// UDP Receive limit
	rc = 8192;
	CfgAddEntry(hCfg, CFGTAG_IP, CFGITEM_IP_SOCKUDPRXLIMIT, CFG_ADDMODE_UNIQUE,
			sizeof(uint), (UINT8 *) &rc, 0);

#if 0
	// TCP Keep Idle (10 seconds)
	rc = 100;
	//   This is the time a connection is idle before TCP will probe
	CfgAddEntry( hCfg, CFGTAG_IP, CFGITEM_IP_TCPKEEPIDLE,
			CFG_ADDMODE_UNIQUE, sizeof(uint), (UINT8 *)&rc, 0 );

	// TCP Keep Interval (1 second)
	//   This is the time between TCP KEEP probes
	rc = 10;
	CfgAddEntry( hCfg, CFGTAG_IP, CFGITEM_IP_TCPKEEPINTVL,
			CFG_ADDMODE_UNIQUE, sizeof(uint), (UINT8 *)&rc, 0 );

	// TCP Max Keep Idle (5 seconds)
	//   This is the TCP KEEP will probe before dropping the connection
	rc = 50;
	CfgAddEntry( hCfg, CFGTAG_IP, CFGITEM_IP_TCPKEEPMAXIDLE,
			CFG_ADDMODE_UNIQUE, sizeof(uint), (UINT8 *)&rc, 0 );
#endif

	//
	// Boot the system using this configuration
	//
	// We keep booting until the function returns 0. This allows
	// us to have a "reboot" command.
	//
	do
	{
		rc = NC_NetStart(hCfg, NetworkOpen, NetworkClose, NetworkIPAddr);
	} while (rc > 0);

	// Free the WEB files
	//RemoveWebFiles();

	// Delete Configuration
	CfgFree(hCfg);

	// Close the OS
	main_exit: NC_SystemClose();
	return (0);

#if 0
	int value_retun = 0;
	int rc;
	int i;
	HANDLE hCfg;
	QMSS_CFG_T qmss_cfg;
	CPPI_CFG_T cppi_cfg;
	int EventID_intc;
	Hwi_Params HwiParam_intc;

// (void) platform_write_configure(PLATFORM_WRITE_UART);
	//write_uart("net work task begin\n\r");

	/* Clear the state of the User LEDs to OFF */
	/*
	 for (i=0; i < gPlatformInfo.led[PLATFORM_USER_LED_CLASS].count; i++)
	 {
	 (void) platform_led(i, PLATFORM_LED_OFF, PLATFORM_USER_LED_CLASS);
	 }
	 */
	/* Initialize the components required to run this application:
	 *  (1) QMSS
	 *  (2) CPPI
	 *  (3) Packet Accelerator
	 */
	/* Initialize QMSS */
	if (platform_get_coreid() == 0)
	{
		qmss_cfg.master_core = 1;
	}
	else
	{
		qmss_cfg.master_core = 0;
	}
	qmss_cfg.max_num_desc = MAX_NUM_DESC;
	qmss_cfg.desc_size = MAX_DESC_SIZE;
	qmss_cfg.mem_region = Qmss_MemRegion_MEMORY_REGION0;
	if (res_mgr_init_qmss(&qmss_cfg) != 0)
	{
		platform_write("Failed to initialize the QMSS subsystem \n");
		goto main_exit;
	}
	else
	{
		platform_write("QMSS successfully initialized \n");
	}

	/* Initialize CPPI */
	if (platform_get_coreid() == 0)
	{
		cppi_cfg.master_core = 1;
	}
	else
	{
		cppi_cfg.master_core = 0;
	}
	cppi_cfg.dma_num = Cppi_CpDma_PASS_CPDMA;
	cppi_cfg.num_tx_queues = NUM_PA_TX_QUEUES;
	cppi_cfg.num_rx_channels = NUM_PA_RX_CHANNELS;
	if (res_mgr_init_cppi(&cppi_cfg) != 0)
	{
		platform_write("Failed to initialize CPPI subsystem \n");
		goto main_exit;
	}
	else
	{
		platform_write("CPPI successfully initialized \n");
	}

	if (res_mgr_init_pass() != 0)
	{
		platform_write("Failed to initialize the Packet Accelerator \n");
		goto main_exit;
	}
	else
	{
		platform_write("PA successfully initialized \n");
	}
//	//add the TI's interrupt component.   add by LHS
//	//Add the interrupt componet.
//	/*
//	 id -- Cp_Intc number
//	 sysInt -- system interrupt number
//	 hostInt -- host interrupt number
//	 */
//	CpIntc_mapSysIntToHostInt(0, PCIEXpress_Legacy_INTA, 3);
//	/*
//	 sysInt -- system interrupt number
//	 fxn -- function
//	 arg -- argument to function
//	 unmask -- bool to unmask interrupt
//	 */
//	CpIntc_dispatchPlug(PCIEXpress_Legacy_INTA, (CpIntc_FuncPtr) isrHandler, 15,
//			TRUE);
//	/*
//	 id -- Cp_Intc number
//	 hostInt -- host interrupt number
//	 */
//	CpIntc_enableHostInt(0, 3);
//	//hostInt -- host interrupt number
//	EventID_intc = CpIntc_getEventId(3);
//	//HwiParam_intc
//	Hwi_Params_init(&HwiParam_intc);
//	HwiParam_intc.arg = 3;
//	HwiParam_intc.eventId = EventID_intc; //eventId
//	HwiParam_intc.enableInt = 1;
//	/*
//	 intNum -- interrupt number
//	 hwiFxn -- pointer to ISR function
//	 params -- per-instance config params, or NULL to select default values (target-domain only)
//	 eb -- active error-handling block, or NULL to select default policy (target-domain only)
//	 */
//	Hwi_create(4, &CpIntc_dispatch, &HwiParam_intc, NULL);

	//
	// THIS MUST BE THE ABSOLUTE FIRST THING DONE IN AN APPLICATION before
	//  using the stack!!
	//
	rc = NC_SystemOpen(NC_PRIORITY_LOW, NC_OPMODE_INTERRUPT);
	if (rc)
	{
		platform_write("NC_SystemOpen Failed (%d)\n", rc);
		for (;;)
		;
	}

	// Print out our banner
	platform_write(VerStr);

	//
	// Create and build the system configuration from scratch.
	//

	// Create a new configuration
	hCfg = CfgNew();
	if (!hCfg)
	{
		platform_write("Unable to create configuration\n");
		goto main_exit;
	}

	//
	// THIS MUST BE THE ABSOLUTE FIRST THING DONE IN AN APPLICATION!!
	//
#if 0
	rc = NC_SystemOpen(NC_PRIORITY_LOW, NC_OPMODE_INTERRUPT);
	if (rc)
	{
		//write_uart("NC_SystemOpen Failed \r\n");
		for (;;)
		;
	}

	// Print out our banner
//    write_uart(VerStr);
	//write_uart("NC_SystemOpen successful\r\n");
	//
	// Create and build the system configuration from scratch.
	//

	// Create a new configuration
	hCfg = CfgNew();
	if (!hCfg)
	{
		//write_uart("Unable to create configuration\n");
		goto main_exit;
	}
	//write_uart("CfgNew successful\r\n");
#endif
	/*
	 * Modify the IP Config refer to the hua example in the MCSDK demo
	 */
	CI_IPNET NA;
	CI_ROUTE RT;
	IPN IPTmp;

	/* Setup an IP address to this EVM */
	bzero( &NA, sizeof(NA));
	NA.IPAddr = inet_addr(EVMStaticIP);
	NA.IPMask = inet_addr(LocalIPMask);
	strcpy(NA.Domain, DomainName);

	/* Add the address to interface 1 */
	CfgAddEntry(hCfg, CFGTAG_IPNET, 1, 0, sizeof(CI_IPNET), (uint8_t *) &NA, 0);

	/* Add the default gateway (back to user PC) */
	bzero( &RT, sizeof(RT));
#if 1
	RT.IPDestAddr = inet_addr(PCStaticIP);
	RT.IPDestMask = inet_addr(LocalIPMask);
#else
	RT.IPDestAddr = 0;
	RT.IPDestMask = 0;
#endif
	RT.IPGateAddr = inet_addr(GatewayIP);

	/* Add the route */
	CfgAddEntry(hCfg, CFGTAG_ROUTE, 0, 0, sizeof(CI_ROUTE), (uint8_t *) &RT, 0);

	/* Manually add the DNS server when specified */
	platform_write("EVM in StaticIP mode at %s\n", EVMStaticIP);
	platform_write("Set IP address of PC to %s\n", PCStaticIP);

	//
	// Configure IPStack/OS Options
	//

	// We don't want to see debug messages less than WARNINGS
	rc = DBG_WARN;
	CfgAddEntry(hCfg, CFGTAG_OS, CFGITEM_OS_DBGPRINTLEVEL, CFG_ADDMODE_UNIQUE,
			sizeof(uint), (UINT8 *) &rc, 0);

	//
	// This code sets up the TCP and UDP buffer sizes
	// (Note 8192 is actually the default. This code is here to
	// illustrate how the buffer and limit sizes are configured.)
	//

	// UDP Receive limit
	/*
	 rc = 8192;
	 CfgAddEntry( hCfg, CFGTAG_IP, CFGITEM_IP_SOCKUDPRXLIMIT,
	 CFG_ADDMODE_UNIQUE, sizeof(uint), (UINT8 *)&rc, 0 );
	 */
	//
	// Boot the system using this configuration
	//
	// We keep booting until the function returns 0. This allows
	// us to have a "reboot" command.
	//
	do
	{
		rc = NC_NetStart(hCfg, NetworkOpen, NetworkClose, NetworkIPAddr);
		//write_uart("NC_NetStart\r\n");
	}while (rc > 0);

	// Delete Configuration
	CfgFree(hCfg);

	// Close the OS
	main_exit: NC_SystemClose();
//    return(0);
//    TaskExit();
//    System_exit(0);
//   BIOS_exit(0);
//    DEVICE_REG32_W(PCIE_LEGACY_A_IRQ_SetEnable,1);
//    DEVICE_REG32_W(PCIE_LEGACY_A_IRQ_RAW,1);
//    start_boot();
#endif
}

//
// System Task Code [ Server Daemon Servers ]
//

//
// NetworkOpen
//
// This function is called after the configuration has booted
//

static void NetworkOpen()
{
	// Create our local server
	//hHello = DaemonNew( SOCK_DGRAM, 0, 7, dtask_udp_hello,OS_TASKPRINORM, OS_TASKSTKNORM, 0, 1 );
	//write_uart("NetworkOpen and begin http_get\r\n");
	TaskCreate(http_get, "http_get", OS_TASKPRINORM, 0x1400, 0, 0, 0);
	TaskCreate(DPMMain, "DPMMain", OS_TASKPRINORM, 0x2000, 0, 0, 0);
}

//
// NetworkClose
//
// This function is called when the network is shutting down,
// or when it no longer has any IP addresses assigned to it.
//
static void NetworkClose()
{
	//DaemonFree( hHello );
}

//
// NetworkIPAddr
//
// This function is called whenever an IP address binding is
// added or removed from the system.
//
static void NetworkIPAddr(IPN IPAddr, uint IfIdx, uint fAdd)
{
	IPN IPTmp;
	char ipInfor[16];
	if (fAdd)
	{
		write_uart("Network Added: \r\n");
		IPTmp = ntohl( IPAddr );
		sprintf(ipInfor, "%d.%d.%d.%d", (UINT8) (IPTmp >> 24) & 0xFF,
				(UINT8) (IPTmp >> 16) & 0xFF, (UINT8) (IPTmp >> 8) & 0xFF,
				(UINT8) IPTmp & 0xFF);
		ipInfor[15] = '\0';
		write_uart(ipInfor);
		write_uart("\r\n");
	}
	else
	{
		write_uart("Network Removed: ");
	}

}

//
// Service Status Reports
//
// Here's a quick example of using service status updates
//
static char *TaskName[] =
{ "Telnet", "HTTP", "NAT", "DHCPS", "DHCPC", "DNS" };
static char *ReportStr[] =
{ "", "Running", "Updated", "Complete", "Fault" };
static char *StatusStr[] =
{ "Disabled", "Waiting", "IPTerm", "Failed", "Enabled" };
static void ServiceReport(uint Item, uint Status, uint Report, HANDLE h)
{
	/*    write_uart( "Service Status: %-9s: %-9s: %-9s: %03d\n",
	 TaskName[Item-1], StatusStr[Status],
	 ReportStr[Report/256], Report&0xFF );
	 */
	//
	// Example of adding to the DHCP configuration space
	//
	// When using the DHCP client, the client has full control over access
	// to the first 256 entries in the CFGTAG_SYSINFO space.
	//
	// Note that the DHCP client will erase all CFGTAG_SYSINFO tags except
	// CFGITEM_DHCP_HOSTNAME. If the application needs to keep manual
	// entries in the DHCP tag range, then the code to maintain them should
	// be placed here.
	//
	// Here, we want to manually add a DNS server to the configuration, but
	// we can only do it once DHCP has finished its programming.
	//
	if (Item == CFGITEM_SERVICE_DHCPCLIENT && Status == CIS_SRV_STATUS_ENABLED
			&& (Report == (NETTOOLS_STAT_RUNNING | DHCPCODE_IPADD)
					|| Report == (NETTOOLS_STAT_RUNNING | DHCPCODE_IPRENEW)))
	{
		IPN IPTmp;

		// Manually add the DNS server when specified
		/*        IPTmp = inet_addr(DNSServer);
		 if( IPTmp )
		 CfgAddEntry( 0, CFGTAG_SYSINFO, CFGITEM_DHCP_DOMAINNAMESERVER,
		 0, sizeof(IPTmp), (UINT8 *)&IPTmp, 0 );
		 */
	}
}
