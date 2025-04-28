/** @file sys_main.c 
*   @brief Application main file
*   @date 15.July.2009
*   @version 1.01.000
*
*   This file contains the initialization & control path for the LwIP & EMAC driver
*   and can be called from system main.
*/

/* (c) Texas Instruments 2011, All rights reserved. */

#if defined(_TMS570LC43x_) || defined(_RM57Lx_)
#include "HL_sys_common.h"
#include "HL_system.h"
#include "HL_emac.h"
#include "HL_mdio.h"
#include "HL_phy_dp83640.h"
#include "HL_sci.h"
#else
#include "sys_common.h"
#include "system.h"
#include "emac.h"
#include "mdio.h"
#include "phy_dp83640.h"
#include "sci.h"
#endif

#include "lwipopts.h"
#include "lwiplib.h"
//#include "httpd.h"
#include "lwip\inet.h"
#include "lwip\udp.h"
#include "lwip\pbuf.h"
#include "locator.h"
#include <stdio.h>

#define TMS570_MDIO_BASE_ADDR 	0xFCF78900u /* Same base address for TMS570 & RM48 devices */
#define TMS570_EMAC_BASE_ADDR	0xFCF78000u /* Same base address for TMS570 & RM48 devices */
#define DPS83640_PHYID			0x20005CE1u	/** PHY ID Register 1 & 2 values for DPS83640 (Same for TMS570 & RM devices */
#define PHY_ADDR				1			/** EVM/Hardware dependent & is same for TMS570 & RM48 HDKs */

#ifndef PRIVILEGED_FUNCTION
#define PRIVILEGED_FUNCTION __attribute__((section(".kernelTEXT")))
#endif

/* Choosing the SCI module used depending upon the device HDK */
#if defined(_TMS570LC43x_) || defined(_RM57Lx_)
#define sciREGx	sciREG1
#else
#define sciREGx	scilinREG
#endif

uint8_t		txtEnetInit[]		= {"Initializing ethernet (DHCP)"};
uint8_t		 * txtIPAddrItoA;

void 	IntMasterIRQEnable	(void);
void 	smallDelay			(void);
void 	sciDisplayText		(sciBASE_t *sci, uint8_t *text,uint32_t length);

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */
void smallDelay(void) {
	  static volatile unsigned int delayval;
	  delayval = 10000;   // 100000 are about 10ms
	  while(delayval--);
}
/* USER CODE END */

//PRIVILEGED_FUNCTION void EMAC_LwIP_Main (uint8_t * macAddress)
void EMAC_LwIP_Main (uint8_t * macAddress)
{
    unsigned int 	ipAddr;
    uint8_t 		testChar;
    struct in_addr 	devIPAddress;

	/* Enable the interrupt generation in CPSR register */
	IntMasterIRQEnable();
	_enable_FIQ();

	printf("%s\n", txtEnetInit);
	ipAddr = lwIPInit(0, macAddress, 0, 0, 0, IPADDR_USE_DHCP);
	/* Uncomment the following if you'd like to assign a static IP address. Change address as required, and uncomment the previous statement. */

//	uint8 ip_addr[4] = { 192, 168, 1, 201 };
//	uint8 netmask[4] = { 255, 255, 255, 0 };
//	uint8 gateway[4] = { 192, 168, 1, 1 };
//	ipAddr = lwIPInit(0, macAddress,
//			*((uint32_t *)ip_addr),
//			*((uint32_t *)netmask),
//			*((uint32_t *)gateway),
//			IPADDR_USE_STATIC);
	
	if (0 == ipAddr) {
	    printf("Failed to get IPAddr\n");
	} else { 
		/* Convert IP Address to string */
		devIPAddress.s_addr = ipAddr;
		txtIPAddrItoA = (uint8_t *)inet_ntoa(devIPAddress);
		LocatorConfig(macAddress, "HDK enet_lwip");
        printf("IP: %s\n", txtIPAddrItoA);
	}

}

/*
** Interrupt Handler for Core 0 Receive interrupt
*/
volatile int countEMACCore0RxIsr = 0;
#pragma INTERRUPT(EMACCore0RxIsr, IRQ)
void EMACCore0RxIsr(void)
{
		countEMACCore0RxIsr++;
		lwIPRxIntHandler(0);
}

/*
** Interrupt Handler for Core 0 Transmit interrupt
*/
volatile int countEMACCore0TxIsr = 0;
#pragma INTERRUPT(EMACCore0TxIsr, IRQ)
void EMACCore0TxIsr(void)
{
	countEMACCore0TxIsr++;
    lwIPTxIntHandler(0);
}

void IntMasterIRQEnable(void)
{
	_enable_IRQ();
	return;
}

void IntMasterIRQDisable(void)
{
	_disable_IRQ();
	return;
}

unsigned int IntMasterStatusGet(void)
{
    return (0xC0 & _get_CPSR());
}

void sciDisplayText(sciBASE_t *sci, uint8_t *text,uint32_t length)
{
    while(length--)
    {
        while ((sci->FLR & 0x4) == 4); /* wait until busy */
        sciSendByte(sci,*text++);      /* send out text   */
    };
}

/* sci notification (Not used but must be provided) */
void sciNotification(sciBASE_t *sci, uint32_t flags)
{
	return;
}
