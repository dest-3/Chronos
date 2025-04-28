/**
 * @file - hdkif.c
 * lwIP Ethernet interface for Hercules Devices
 *
 */

/**
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/**
 * Copyright (c) 2010 Texas Instruments Incorporated
 *
 * This file is dervied from the "ethernetif.c" skeleton Ethernet network
 * interface driver for lwIP.
 *
 */
#include "src/include/lwip/opt.h"
#include "src/include/lwip/def.h"
#include "src/include/lwip/mem.h"
#include "src/include/lwip/pbuf.h"
#include "src/include/lwip/sys.h"
#include "src/include/lwip/stats.h"
#include "src/include/lwip/snmp.h"
#include "src/include/netif/etharp.h"
#include "src/include/netif/ppp_oe.h"
#include "src/include/lwip/err.h"
#include "ports/hdk/include/netif/hdkif.h"
#include "ports/hdk/include/arch/cc.h"

/* HALCoGen DriverLib Header & PHY files required for this interface driver. */
#if defined(_TMS570LC43x_) || defined(_RM57Lx_)
#include "HL_emac.h"
#include "HL_mdio.h"
#include "HL_phy_dp83640.h"
#include "HL_sci.h"
#else
#include "emac.h"
#include "mdio.h"
#include "phy_dp83640.h"
#include "sci.h"
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* EMAC Control RAM size in bytes */
#ifndef SIZE_EMAC_CTRL_RAM
#define SIZE_EMAC_CTRL_RAM        0x2000
#endif

#define PBUF_LEN_MAX              MAX_TRANSFER_UNIT


/* Define those to better describe the network interface. */
#define IFNAME0                  'e'
#define IFNAME1                  'n'

/* Choosing the SCI module used depending upon the device HDK */
#if defined(_TMS570LC43x_) || defined(_RM57Lx_)
#define sciREGx	sciREG1
#else
#define sciREGx	scilinREG
#endif

/* EMAC TX Buffer descriptor data structure */
struct emac_tx_bdp {
  volatile struct emac_tx_bdp *next;
  volatile uint32 bufptr;
  volatile uint32 bufoff_len;
  volatile uint32 flags_pktlen;
  
  /* helper to know which pbuf this tx bd corresponds to */
  volatile struct pbuf *pbuf;
}emac_tx_bdp;

/* EMAC RX Buffer descriptor data structure */
struct emac_rx_bdp {
  volatile struct emac_rx_bdp *next;
  volatile uint32 bufptr;
  volatile uint32 bufoff_len;
  volatile uint32 flags_pktlen;

  /* helper to know which pbuf this rx bd corresponds to */
  volatile struct pbuf *pbuf;
}emac_rx_bdp;

/**
 * Helper struct to hold the data used to operate on a particular 
 * receive channel 
 */
struct rxch {
  volatile struct emac_rx_bdp *free_head;
  volatile struct emac_rx_bdp *active_head;
  volatile struct emac_rx_bdp *active_tail;
  uint32 freed_pbuf_len;
}rxch;

/**
 * Helper struct to hold the data used to operate on a particular 
 * transmit channel 
 */
struct txch {
  volatile struct emac_tx_bdp *free_head;
  volatile struct emac_tx_bdp *active_tail;
  volatile struct emac_tx_bdp *next_bd_to_process;
}txch;
 
/**
 * Helper struct to hold private data used to operate the ethernet interface.
 */
struct hdkif {
  /* emac instance number */
  uint32 inst_num;

  uint8 mac_addr[6];

  /* emac base address */
  uint32 emac_base;

  /* emac controller base address */
  volatile uint32 emac_ctrl_base;
  volatile uint32 emac_ctrl_ram;

  /* mdio base address */
  volatile uint32 mdio_base;

  /* phy parameters for this instance - for future use */
  uint32 phy_addr;
  boolean (*phy_autoneg)(uint32, uint32, uint16);
  boolean (*phy_partnerability)(uint32, uint32, uint16*);
 
  /* The tx/rx channels for the interface */
  struct txch txch;
  struct rxch rxch;
}hdkif;

/* Defining interface for all the emac instances */
static struct hdkif hdkif_data[MAX_EMAC_INSTANCE];

uint32 hdkif_swizzle_data(uint32 word) {

#if defined(_TMS570LC43x_)
	return
		(((word << 24) & 0xFF000000) |
		((word <<  8) & 0x00FF0000)  |
		((word >>  8) & 0x0000FF00)  |
		((word >> 24) & 0x000000FF));
#else
	return word;
#endif
}

struct emac_tx_bdp *hdkif_swizzle_txp(volatile struct emac_tx_bdp *p) {
	return (struct emac_tx_bdp *)hdkif_swizzle_data((uint32)p);
}

struct emac_rx_bdp *hdkif_swizzle_rxp(volatile struct emac_rx_bdp *p) {
	return (struct emac_rx_bdp *)hdkif_swizzle_data((uint32)p);
}
/**
* Function to set the MAC address to the interface
* @param   inst_num the instance number 
* @return  none. 
*/
void 
hdkif_macaddrset(uint32 inst_num, uint8 *mac_addr) {
  struct hdkif *hdkif;
  uint32 temp;

  hdkif = &hdkif_data[inst_num];
  
  /* set MAC hardware address */
  for(temp = 0; temp < ETHARP_HWADDR_LEN; temp++) {
    hdkif->mac_addr[temp] = mac_addr[(ETHARP_HWADDR_LEN - 1) - temp];
  }
}

/**
* Function to setup the instance parameters inside the interface
* @param   hdkif
* @return  none. 
*/
static void 
hdkif_inst_config(struct hdkif *hdkif) {
  if(hdkif->inst_num == 0) {
    hdkif->emac_base = EMAC_0_BASE;
    hdkif->emac_ctrl_base = EMAC_CTRL_0_BASE;
    hdkif->emac_ctrl_ram = EMAC_CTRL_RAM_0_BASE;
    hdkif->mdio_base = MDIO_0_BASE;
    hdkif->phy_addr = 1;
    hdkif->phy_autoneg = Dp83640AutoNegotiate;
    hdkif->phy_partnerability = Dp83640PartnerAbilityGet;
  }
}

/**
* Function to setup the link. AutoNegotiates with the phy for link
* setup and set the EMAC with the result of autonegotiation. 
* @param  hdkif
* @return ERR_OK if everything passed
*         others if not passed
*/
static err_t 
hdkif_link_setup(struct hdkif *hdkif) {
  err_t linkstat = ERR_CONN;
  uint16 partnr_ablty;
  uint32 phyduplex = EMAC_DUPLEX_HALF;
  volatile unsigned int delay = 0xFFFFF;

  if(hdkif->inst_num == 0) {
    if(Dp83640AutoNegotiate(hdkif->mdio_base, hdkif->phy_addr,
                             (DP83640_100BTX | DP83640_100BTX_FD
                              | DP83640_10BT | DP83640_10BT_FD)) == TRUE) {
      linkstat = ERR_OK;
      Dp83640PartnerAbilityGet(hdkif->mdio_base, hdkif->phy_addr,
                                &partnr_ablty);
      
      /* Check for 100 Mbps and duplex capability */
      if(partnr_ablty & DP83640_100BTX_FD) {
        phyduplex = EMAC_DUPLEX_FULL;
      }
    }
    else {
      linkstat = ERR_CONN;
    }
  }

  else {
    linkstat = ERR_CONN;
  }
 
  /* Set the EMAC with the negotiation results if it is successful */ 
  if(linkstat == ERR_OK) {
    EMACDuplexSet(hdkif->emac_base, phyduplex); 
  }

  /* Wait for the MII to settle down */
  while(delay--);

  return linkstat;
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf might be
 * chained. That is, one pbuf can span more than one tx buffer descriptors
 *
 * @param hdkif the network interface state for this ethernetif
 * @param pbuf  the pbuf which is to be sent over EMAC
 * @return None
 */
static void
hdkif_transmit(struct hdkif *hdkif, struct pbuf *pbuf) {
  struct pbuf *q;
  struct txch *txch;
  volatile struct emac_tx_bdp *curr_bd, *active_head, *bd_end;

  txch = &(hdkif->txch);
 
  /* Get the buffer descriptor which is free to transmit */
  curr_bd = txch->free_head;
  
  active_head = curr_bd;
 
  /* Update the total packet length */
  uint32 flags_pktlen = pbuf->tot_len;
  flags_pktlen |= (EMAC_BUF_DESC_SOP | EMAC_BUF_DESC_OWNER);
  curr_bd->flags_pktlen = hdkif_swizzle_data(flags_pktlen);

  /* Copy pbuf information into TX buffer descriptors */
  for(q = pbuf; q != NULL; q = q->next) {

    /* Intialize the buffer pointer and length */
    curr_bd->bufptr = hdkif_swizzle_data((uint32)(q->payload));
    curr_bd->bufoff_len = hdkif_swizzle_data((q->len) & 0xFFFF);
    bd_end = curr_bd;
    curr_bd->pbuf = pbuf;
    curr_bd = hdkif_swizzle_txp(curr_bd->next);
    //if (curr_bd) curr_bd->flags_pktlen = 0;
  }

  /* Indicate the end of the packet */
  bd_end->next = NULL;
  bd_end->flags_pktlen |= hdkif_swizzle_data(EMAC_BUF_DESC_EOP);
 
  txch->free_head = curr_bd;
 
  /* For the first time, write the HDP with the filled bd */
  if(txch->active_tail == NULL) {
    EMACTxHdrDescPtrWrite(hdkif->emac_base, (unsigned int)(active_head), 0);
  }
 
  /*
   * Chain the bd's. If the DMA engine, already reached the end of the chain, 
   * the EOQ will be set. In that case, the HDP shall be written again.
   */
  else {
    curr_bd = txch->active_tail;
    /* TODO: (This is a workaround) Wait for the EOQ bit is set */
    while (EMAC_BUF_DESC_EOQ != (hdkif_swizzle_data(curr_bd->flags_pktlen) & EMAC_BUF_DESC_EOQ));
    /* TODO: (This is a workaround) Don't write to TXHDP0 until it turns to zero */
  	while (0 != *((uint32 *)0xFCF78600));
    curr_bd->next = hdkif_swizzle_txp(active_head);
    if (EMAC_BUF_DESC_EOQ == (hdkif_swizzle_data(curr_bd->flags_pktlen) & EMAC_BUF_DESC_EOQ)) {
      /* Write the Header Descriptor Pointer and start DMA */
      EMACTxHdrDescPtrWrite(hdkif->emac_base, (unsigned int)(active_head), 0);
    }
  }

  txch->active_tail = bd_end;
}

/**
 * This function will send a packet through the emac if the channel is 
 * available. Otherwise, the packet will be queued in a pbuf queue.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 */
static err_t
hdkif_output(struct netif *netif, struct pbuf *p) {
  SYS_ARCH_DECL_PROTECT(lev);

  /**
   * This entire function must run within a "critical section" to preserve
   * the integrity of the transmit pbuf queue.
   *
   */
  SYS_ARCH_PROTECT(lev);

  /* adjust the packet length if less than minimum required */
  if(p->tot_len < MIN_PKT_LEN) {
     p->tot_len = MIN_PKT_LEN;
     p->len = MIN_PKT_LEN;
  }

  /**
   * Bump the reference count on the pbuf to prevent it from being
   * freed till we are done with it.
   *
   */
  pbuf_ref(p);
   
  /* call the actual transmit function */
  hdkif_transmit(netif->state, p);

  /* Return to prior interrupt state and return. */
  SYS_ARCH_UNPROTECT(lev);
  
  return ERR_OK;
}


extern void 	sciDisplayText		(sciBASE_t *sci, uint8_t *text,uint32_t length);
static uint8_t		txtCRLF2[]			= {'\r', '\n'};
static uint8_t		txtSuccess[]		= {"SUCCESS"};
static uint8_t		txtProgress[]		= {"."};
static uint8_t		txtError[]			= {"!!! ERROR !!!"};
static uint8_t		txtPhyGetId[]		= {"	DEBUG - Getting PHY ID..."};
static uint8_t		txtPhyGetAlSts[]	= {"	DEBUG - Getting PHY Alive Status..."};
static uint8_t		txtPhyGetLnkSts[]	= {"	DEBUG - Getting PHY Link Status..."};
static uint8_t		txtPhySetupLink[]	= {"	DEBUG - Setting up Link..."};
/* End of DEBUG BLOCK - 1 */

/**
 * In this function, the hardware should be initialized.
 * Called from hdkif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static err_t
hdkif_hw_init(struct netif *netif)
{
  uint32 temp, channel;
  uint32 num_bd, pbuf_cnt = 0;
  volatile uint32 phyID=0;
  volatile uint32 delay = 0xfff;
  volatile uint32 phyIdReadCount = 0xFFFF;
  volatile struct emac_tx_bdp *curr_txbd, *last_txbd;
  volatile struct emac_rx_bdp *curr_bd, *last_bd;
  struct hdkif *hdkif;
  struct txch *txch;
  struct rxch *rxch;
  struct pbuf *p, *q;

  hdkif = netif->state;

  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  for(temp = 0; temp < ETHARP_HWADDR_LEN; temp++) {
    netif->hwaddr[temp] = hdkif->mac_addr[(ETHARP_HWADDR_LEN - 1) - temp];
  }

  /* maximum transfer unit */
  netif->mtu = MAX_TRANSFER_UNIT;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

  EMACInit(hdkif->emac_ctrl_base, hdkif->emac_base);

  int i = 0;
  while(i < 100000) {
      i += 1;
  }

  MDIOInit(hdkif->mdio_base, MDIO_FREQ_INPUT, MDIO_FREQ_OUTPUT);


  while(delay--);
  
  EMACRxBroadCastEnable(hdkif->emac_base, 0);

  /* Set the MAC Addresses in EMAC hardware */
  EMACMACSrcAddrSet(hdkif->emac_base, hdkif->mac_addr);
  
  for(channel = 0; channel < 8; channel++) {
       EMACMACAddrSet(hdkif->emac_base, channel, hdkif->mac_addr, EMAC_MACADDR_MATCH);
  }

//  sciDisplayText(sciREGx, txtCRLF2, sizeof(txtCRLF2));
//  sciDisplayText(sciREGx, txtPhyGetId, sizeof(txtPhyGetId));
  printf("%s\n", txtPhyGetId);

  while ((phyID == 0) && (phyIdReadCount > 0)) {
	  phyID = Dp83640IDGet(hdkif->mdio_base, hdkif->phy_addr);
	  phyIdReadCount--;
//	  sciDisplayText(sciREGx, txtProgress, sizeof(txtProgress));
  }

  if (0 != phyID) {
//  	sciDisplayText(sciREGx, txtSuccess, sizeof(txtSuccess));
      printf("%s\n", txtSuccess);
  } else {
      printf("%s\n", txtError);
//  	sciDisplayText(sciREGx, txtError, sizeof(txtError));
  }
//  sciDisplayText(sciREGx, txtCRLF2, sizeof(txtCRLF2));

//  sciDisplayText(sciREGx, txtPhyGetAlSts, sizeof(txtPhyGetAlSts));
  printf("%s\n", txtPhyGetAlSts);

  if(!((MDIOPhyAliveStatusGet(hdkif->mdio_base)
        >> hdkif->phy_addr) & 0x01 )) {
//    sciDisplayText(sciREGx, txtError, sizeof(txtError));
    return ERR_CONN;
  } else {
//  	  sciDisplayText(sciREGx, txtSuccess, sizeof(txtSuccess));
  	  printf("%s\n", txtSuccess);
  }
//  sciDisplayText(sciREGx, txtCRLF2, sizeof(txtCRLF2));

  printf("%s\n", txtPhyGetLnkSts);
//  sciDisplayText(sciREGx, txtPhyGetLnkSts, sizeof(txtPhyGetLnkSts));
  if(!Dp83640LinkStatusGet(hdkif->mdio_base, hdkif->phy_addr, 0xFFFF)) {
//	  sciDisplayText(sciREGx, txtError, sizeof(txtError));
	  printf("%s\n", txtError);
      return ERR_CONN;
  } else {
//  	  sciDisplayText(sciREGx, txtSuccess, sizeof(txtSuccess));
      printf("%s\n", txtSuccess);
  }
//  sciDisplayText(sciREGx, txtCRLF2, sizeof(txtCRLF2));

//  sciDisplayText(sciREGx, txtPhySetupLink, sizeof(txtPhySetupLink));
  printf("%s\n", txtPhySetupLink);
  if(hdkif_link_setup(hdkif) != ERR_OK) {
      printf("%s\n", txtError);
    return ERR_CONN;
  } else {
      printf("%s\n", txtSuccess);
  }

  txch = &(hdkif->txch);  

  /**
  * Initialize the Descriptor Memory For TX and RX 
  * Only Channel 0 is supported for both TX and RX
  */
  txch->free_head = (volatile struct emac_tx_bdp*)(hdkif->emac_ctrl_ram);
  txch->next_bd_to_process = txch->free_head;
  txch->active_tail = NULL;
 
  /* Set the number of descriptors for the channel */ 
  num_bd = (SIZE_EMAC_CTRL_RAM >> 1) / sizeof(emac_tx_bdp);
  
  curr_txbd = txch->free_head;

  /* Initialize all the TX buffer Descriptors */
  while(num_bd--) {
    curr_txbd->next = hdkif_swizzle_txp(curr_txbd + 1);
    curr_txbd->flags_pktlen = 0;
    last_txbd = curr_txbd;
    curr_txbd = hdkif_swizzle_txp(curr_txbd->next);
  } 
  last_txbd->next = hdkif_swizzle_txp(txch->free_head);
  
  /* Initialize the descriptors for the RX channel */
  rxch = &(hdkif->rxch);
  rxch->active_head = (volatile struct emac_rx_bdp*)(curr_txbd + 1);
  
  rxch->free_head = NULL;
  rxch->freed_pbuf_len = 0;
  num_bd = ((SIZE_EMAC_CTRL_RAM >> 1) / sizeof(emac_rx_bdp) - 1);
  curr_bd = rxch->active_head;
  last_bd = curr_bd;

  /*
  ** Allocate the pbufs for the maximum count permitted or till the
  ** number of buffer desceriptors expire, which ever is earlier.
  */
  while(pbuf_cnt < MAX_RX_PBUF_ALLOC) {
    p = pbuf_alloc(PBUF_RAW, PBUF_LEN_MAX, PBUF_POOL);
    pbuf_cnt++;
    
    if(p != NULL) {
      /* write the descriptors if there are enough numbers to hold the pbuf*/
      if(((uint32)pbuf_clen(p)) <= num_bd) {
        for(q = p; q != NULL; q = q->next) {
          curr_bd->bufptr = hdkif_swizzle_data((uint32)(q->payload));
          curr_bd->bufoff_len = hdkif_swizzle_data(q->len);
          curr_bd->next = hdkif_swizzle_rxp(curr_bd + 1);
          curr_bd->flags_pktlen = hdkif_swizzle_data(EMAC_BUF_DESC_OWNER);
          
          /* Save the pbuf */
          curr_bd->pbuf = q;
          last_bd = curr_bd;
          curr_bd = hdkif_swizzle_rxp(curr_bd->next);
          num_bd--;
        }
      }
    
      /* free the allocated pbuf if no free descriptors are left */
      else {
        pbuf_free(p);
        break;
      }
    }
    else {
      break;
    }
  }

  last_bd->next = NULL;
  rxch->active_tail = last_bd;

  /* Acknowledge receive and transmit interrupts for proper interrupt pulsing*/
  EMACCoreIntAck(hdkif->emac_base, EMAC_INT_CORE0_RX);
  EMACCoreIntAck(hdkif->emac_base, EMAC_INT_CORE0_TX);
  
  EMACRxUnicastSet(hdkif->emac_base, 0);
  EMACNumFreeBufSet(hdkif->emac_base, 0, 10);
  EMACTxEnable(hdkif->emac_base);
  EMACRxEnable(hdkif->emac_base);

  /* Write the RX HDP for channel 0 */
  EMACRxHdrDescPtrWrite(hdkif->emac_base, (uint32)rxch->active_head, 0);
 
  EMACMIIEnable(hdkif->emac_base); 

  
  /**
  * Enable the Transmission and reception, enable the interrupts for
  * channel 0 and for control core 0
  */
  EMACTxIntPulseEnable(hdkif->emac_base, hdkif->emac_ctrl_base, 0, 0);
  EMACRxIntPulseEnable(hdkif->emac_base, hdkif->emac_ctrl_base, 0, 0);

  return ERR_OK;
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function hdkif_hw_init() to do the
 * low level initializations.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
hdkif_init(struct netif *netif)
{
  /* Get the instance number first */
  unsigned int inst_num = *(unsigned int*)(netif->state);
  struct hdkif *hdkif;

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  hdkif = &hdkif_data[inst_num];
 
  netif->state = hdkif;
  
  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
  */
  NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 10000000);


  hdkif->inst_num = inst_num;

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;

  netif->num = (uint8)inst_num;

  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) 
   */
  netif->output = etharp_output;
  netif->linkoutput = hdkif_output;

  /* initialize the hardware */
  hdkif_inst_config(hdkif);

  return (hdkif_hw_init(netif));
}

/**
 * Handler for Receive interrupt. Packet processing is done in this
 * interrupt handler itself. 
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return none
 */
void
hdkif_rx_inthandler(struct netif *netif) {
  struct hdkif *hdkif;
  struct rxch *rxch;
  volatile struct emac_rx_bdp *curr_bd, *processed_bd, *curr_tail, *last_bd;
  volatile struct pbuf *pbuf, *q, *new_pbuf;
  uint32 ex_len = 0, len_to_alloc = 0;
  uint16 tot_len;
 
  hdkif = netif->state;
  rxch = &(hdkif->rxch);

  /* Get the bd which contains the earliest filled data */ 
  curr_bd = rxch->active_head;
  last_bd = rxch->active_tail;
  
  /**
   * Process the descriptors as long as data is available
   * when the DMA is receiving data, SOP flag will be set
  */
  while(hdkif_swizzle_data(curr_bd->flags_pktlen) & EMAC_BUF_DESC_SOP) {
    ex_len = 0;
    len_to_alloc = 0;

    /* Start processing once the packet is loaded */
    if((hdkif_swizzle_data(curr_bd->flags_pktlen) & EMAC_BUF_DESC_OWNER)
       != EMAC_BUF_DESC_OWNER) {
 
      if(rxch->free_head == NULL) {
        /* this bd chain will be freed after processing */
        rxch->free_head = curr_bd;     
      }
     
      /* Get the total length of the packet. curr_bd points to the start
       * of the packet.
       */ 
      tot_len = hdkif_swizzle_data(curr_bd->flags_pktlen) & 0xFFFF;

      /* Get the start of the pbuf queue */
      q = curr_bd->pbuf;
 
      do { 
        /* Get the pbuf pointer which is associated with the current bd */
        pbuf = curr_bd->pbuf;
        
        /* If the earlier pbuf ended, update the chain */ 
        if(pbuf->next == NULL) {
          pbuf->next = (struct pbuf*)hdkif_swizzle_rxp(curr_bd->next)->pbuf;
        }  
       
        len_to_alloc += pbuf->len;
        /* Update the len and tot_len fields for the pbuf in the chain*/
        pbuf->len = hdkif_swizzle_data(curr_bd->bufoff_len) & 0xFFFF;
        pbuf->tot_len = tot_len - ex_len ;
        processed_bd = curr_bd;
        ex_len += pbuf->len;
        curr_bd = hdkif_swizzle_rxp(curr_bd->next);
      } while((hdkif_swizzle_data(processed_bd->flags_pktlen) & EMAC_BUF_DESC_EOP)
              != EMAC_BUF_DESC_EOP);

      /**
       * Close the chain for this pbuf. A full packet is received in 
       * this pbuf chain. Now this pbuf can be given to upper layers for 
       * processing. The start of the pbuf chain is now 'q'.
      */
      pbuf->next = NULL;
  
      /* Adjust the link statistics */
      LINK_STATS_INC(link.recv);

      /* Process the packet */
      if(ethernet_input((struct pbuf *)q, netif) != ERR_OK) {
        /* Adjust the link statistics */
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
      }

      /* Acknowledge that this packet is processed */
      EMACRxCPWrite(hdkif->emac_base, 0, (unsigned int)processed_bd);

      rxch->active_head = curr_bd;
   
      /**
       * The earlier pbuf chain is freed from the upper layer. So, we need to
       * allocate a new pbuf chain and update the descriptors with the pbuf info.
       * To support chaining, the total length freed by the upper layer is tracked.
       * Care should be taken even if the allocation fails.
       */   
      /**
       * now len_to_alloc will contain the length of the pbuf which was freed
       * from the upper layer
       */
      rxch->freed_pbuf_len += len_to_alloc;  
      new_pbuf = pbuf_alloc(PBUF_RAW, (rxch->freed_pbuf_len), PBUF_POOL);

      /* Write the descriptors with the pbuf info till either of them expires */
      if(new_pbuf != NULL) {
        curr_bd = rxch->free_head;

        for(q = new_pbuf; (q != NULL) && (curr_bd != rxch->active_head); q = q->next) {
          curr_bd->bufptr = hdkif_swizzle_data((uint32)(q->payload));
          
          /* no support for buf_offset. RXBUFFEROFFEST register is 0 */
          curr_bd->bufoff_len = hdkif_swizzle_data((q->len) & 0xFFFF);
          curr_bd->flags_pktlen = hdkif_swizzle_data(EMAC_BUF_DESC_OWNER);
          
          rxch->freed_pbuf_len -= q->len;
         
          /* Save the pbuf */
          curr_bd->pbuf = q;
          last_bd = curr_bd;
          curr_bd = hdkif_swizzle_rxp(curr_bd->next);
        }
         
        /**
         * At this point either pbuf expired or no rxbd to allocate. If 
         * there are no, enough rx bds to allocate all pbufs in the chain,
         * free the rest of the pbuf
         */
        if(q != NULL) {
          pbuf_free((struct pbuf *)q);
        }
       
        curr_tail = rxch->active_tail; 
        last_bd->next = NULL;
       
        curr_tail->next = hdkif_swizzle_rxp(rxch->free_head);
        
        /**
         * Check if the reception has ended. If the EOQ flag is set, the NULL
         * Pointer is taken by the DMA engine. So we need to write the RX HDP
         * with the next descriptor.
         */
        if(hdkif_swizzle_data(curr_tail->flags_pktlen) & EMAC_BUF_DESC_EOQ) {
          EMACRxHdrDescPtrWrite(hdkif->emac_base, (uint32)(rxch->free_head), 0);
        }

        rxch->free_head  = curr_bd;
        rxch->active_tail = last_bd;
      }
    }
    curr_bd = rxch->active_head;
  }

  EMACCoreIntAck(hdkif->emac_base, EMAC_INT_CORE0_RX);
  EMACCoreIntAck(hdkif->emac_base, EMAC_INT_CORE0_TX);
}

/**
 * Handler for EMAC Transmit interrupt
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return none
 */
void
hdkif_tx_inthandler(struct netif *netif) {
  struct txch *txch;
  struct hdkif *hdkif; 
  volatile struct emac_tx_bdp *curr_bd, *next_bd_to_process;
  
  hdkif = netif->state; 
  txch = &(hdkif->txch);
    
  next_bd_to_process = txch->next_bd_to_process;
  
  curr_bd = next_bd_to_process;
  
  /* Check for correct start of packet */
  while(hdkif_swizzle_data(curr_bd->flags_pktlen) & EMAC_BUF_DESC_SOP) {
    
    /* Make sure that the transmission is over */
    while((hdkif_swizzle_data(curr_bd->flags_pktlen) & EMAC_BUF_DESC_OWNER)
          == EMAC_BUF_DESC_OWNER);
   
    /* Traverse till the end of packet is reached */ 
    while((hdkif_swizzle_data(curr_bd->flags_pktlen) & EMAC_BUF_DESC_EOP) != EMAC_BUF_DESC_EOP) {
       curr_bd = hdkif_swizzle_txp(curr_bd->next);
    }
 
    next_bd_to_process->flags_pktlen &= hdkif_swizzle_data(~(EMAC_BUF_DESC_SOP));
    curr_bd->flags_pktlen &= hdkif_swizzle_data(~(EMAC_BUF_DESC_EOP));

    /**
     * If there are no more data transmitted, the next interrupt 
     * shall happen with the pbuf associated with the free_head
     */
    if(curr_bd->next == NULL) {
      txch->next_bd_to_process = txch->free_head;
    }
  
    else {
      txch->next_bd_to_process = hdkif_swizzle_txp(curr_bd->next);
    }
    
    /* Acknowledge the EMAC and free the corresponding pbuf */
    EMACTxCPWrite(hdkif->emac_base, 0, (uint32)curr_bd);

    pbuf_free((struct pbuf *)curr_bd->pbuf);

    LINK_STATS_INC(link.xmit);
   
    next_bd_to_process = txch->next_bd_to_process;
    curr_bd = next_bd_to_process;
  }

  EMACCoreIntAck(hdkif->emac_base, EMAC_INT_CORE0_RX);
  EMACCoreIntAck(hdkif->emac_base, EMAC_INT_CORE0_TX);
}

/**
 * Gets the netif status
 *
 * @param   the netif whoes status to be checked 
 * @return  the status
 */
uint32
hdkif_netif_status(struct netif *netif) {
  return (netif_is_up(netif));
}

/**
 * returns the link status
 *
 * @param   the netif whoes link to be checked 
 * @return  the status
 */
uint32
hdkif_link_status(struct netif *netif) {

  struct hdkif *hdkif = netif->state;

  return (Dp83640LinkStatusGet(hdkif->mdio_base,
                                hdkif->phy_addr,
                                10000));
}


