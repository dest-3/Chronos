/** @file system.h
*   @brief System Driver Header File
*   @date 11-Dec-2018
*   @version 04.07.01
*   
*   This file contains:
*   - Definitions
*   - Types
*   .
*   which are relevant for the System driver.
*/

/* 
* Copyright (C) 2009-2018 Texas Instruments Incorporated - www.ti.com 
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


#ifndef __SYS_SYSTEM_H__
#define __SYS_SYSTEM_H__

#include "reg_system.h"
#include "reg_flash.h"
#include "reg_tcram.h"

#ifdef __cplusplus
extern "C" {
#endif

/* USER CODE BEGIN (0) */
/* USER CODE END */


/* System General Definitions */

/** @enum systemClockSource
*   @brief Alias names for clock sources
*
*   This enumeration is used to provide alias names for the clock sources:
*     - Oscillator
*     - Pll1
*     - External1
*     - Low Power Oscillator Low
*     - Low Power Oscillator High
*     - PLL2
*     - External2
*     - Synchronous VCLK1
*/
enum systemClockSource
{
    SYS_OSC             = 0U,  /**< Alias for oscillator clock Source                */
    SYS_PLL1            = 1U,  /**< Alias for Pll1 clock Source                      */
    SYS_EXTERNAL1       = 3U,  /**< Alias for external clock Source                  */
    SYS_LPO_LOW         = 4U,  /**< Alias for low power oscillator low clock Source  */
    SYS_LPO_HIGH        = 5U,  /**< Alias for low power oscillator high clock Source */
    SYS_PLL2            = 6U,  /**< Alias for Pll2 clock Source                      */
    SYS_EXTERNAL2       = 7U,  /**< Alias for external 2 clock Source                */
    SYS_VCLK            = 9U   /**< Alias for synchronous VCLK1 clock Source         */
};

#define SYS_DOZE_MODE        0x000F3F02U
#define SYS_SNOOZE_MODE      0x000F3F03U
#define SYS_SLEEP_MODE       0x000FFFFFU
#define LPO_TRIM_VALUE       (((*(volatile uint32 *)0xF00801B4U) & 0xFFFF0000U)>>16U)
#define SYS_EXCEPTION        (*(volatile uint32 *)0xFFFFFFE4U)

#define POWERON_RESET        0x8000U
#define OSC_FAILURE_RESET    0x4000U
#define WATCHDOG_RESET       0x2000U
#define ICEPICK_RESET        0x2000U
#define CPU_RESET            0x0020U
#define SW_RESET             0x0010U

#define WATCHDOG_STATUS     (*(volatile uint32 *)0xFFFFFC98U)
#define DEVICE_ID_REV       (*(volatile uint32 *)0xFFFFFFF0U)

/** @def OSC_FREQ
*   @brief Oscillator clock source exported from HALCoGen GUI
*
*   Oscillator clock source exported from HALCoGen GUI
*/
#define OSC_FREQ     16.0F

/** @def PLL1_FREQ
*   @brief PLL 1 clock source exported from HALCoGen GUI
*
*   PLL 1 clock source exported from HALCoGen GUI
*/
#define PLL1_FREQ    220.00F

/** @def LPO_LF_FREQ
*   @brief LPO Low Freq Oscillator source exported from HALCoGen GUI
*
*   LPO Low Freq Oscillator source exported from HALCoGen GUI
*/
#define LPO_LF_FREQ  0.080F

/** @def LPO_HF_FREQ
*   @brief LPO High Freq Oscillator source exported from HALCoGen GUI
*
*   LPO High Freq Oscillator source exported from HALCoGen GUI
*/
#define LPO_HF_FREQ  10.000F

/** @def PLL1_FREQ
*   @brief PLL 2 clock source exported from HALCoGen GUI
*
*   PLL 2 clock source exported from HALCoGen GUI
*/
#define PLL2_FREQ    220.00F

/** @def GCLK_FREQ
*   @brief GCLK domain frequency exported from HALCoGen GUI
*
*   GCLK domain frequency exported from HALCoGen GUI
*/
#define GCLK_FREQ    220.000F

/** @def HCLK_FREQ
*   @brief HCLK domain frequency exported from HALCoGen GUI
*
*   HCLK domain frequency exported from HALCoGen GUI
*/
#define HCLK_FREQ    220.000F

/** @def RTI_FREQ
*   @brief RTI Clock frequency exported from HALCoGen GUI
*
*   RTI Clock frequency exported from HALCoGen GUI
*/
#define RTI_FREQ     110.000F

/** @def AVCLK1_FREQ
*   @brief AVCLK1 Domain frequency exported from HALCoGen GUI
*
*   AVCLK Domain frequency exported from HALCoGen GUI
*/
#define AVCLK1_FREQ  110.000F

/** @def AVCLK2_FREQ
*   @brief AVCLK2 Domain frequency exported from HALCoGen GUI
*
*   AVCLK2 Domain frequency exported from HALCoGen GUI
*/
#define AVCLK2_FREQ  110.0F

/** @def AVCLK3_FREQ
*   @brief AVCLK3 Domain frequency exported from HALCoGen GUI
*
*   AVCLK3 Domain frequency exported from HALCoGen GUI
*/
#define AVCLK3_FREQ  110.000F

/** @def AVCLK4_FREQ
*   @brief AVCLK4 Domain frequency exported from HALCoGen GUI
*
*   AVCLK4 Domain frequency exported from HALCoGen GUI
*/
#define AVCLK4_FREQ  110.000F

/** @def VCLK1_FREQ
*   @brief VCLK1 Domain frequency exported from HALCoGen GUI
*
*   VCLK1 Domain frequency exported from HALCoGen GUI
*/
#define VCLK1_FREQ   110.000F

/** @def VCLK2_FREQ
*   @brief VCLK2 Domain frequency exported from HALCoGen GUI
*
*   VCLK2 Domain frequency exported from HALCoGen GUI
*/
#define VCLK2_FREQ   110.000F

/** @def VCLK3_FREQ
*   @brief VCLK3 Domain frequency exported from HALCoGen GUI
*
*   VCLK3 Domain frequency exported from HALCoGen GUI
*/
#define VCLK3_FREQ   110.000F

/** @def VCLK4_FREQ
*   @brief VCLK4 Domain frequency exported from HALCoGen GUI
*
*   VCLK4 Domain frequency exported from HALCoGen GUI
*/
#define VCLK4_FREQ   0.0F


/** @def SYS_PRE1
*   @brief Alias name for RTI1CLK PRE clock source
*
*   This is an alias name for the RTI1CLK pre clock source.
*   This can be either:
*     - Oscillator
*     - Pll
*     - 32 kHz Oscillator
*     - External
*     - Low Power Oscillator Low
*     - Low Power Oscillator High
*     - Flexray Pll
*/
/*SAFETYMCUSW 79 S MR:19.4 <APPROVED> " Value comes from GUI drop down option " */
#define SYS_PRE1 (SYS_PLL1)

/** @def SYS_PRE2
*   @brief Alias name for RTI2CLK pre clock source
*
*   This is an alias name for the RTI2CLK pre clock source.
*   This can be either:
*     - Oscillator
*     - Pll
*     - 32 kHz Oscillator
*     - External
*     - Low Power Oscillator Low
*     - Low Power Oscillator High
*     - Flexray Pll
*/
/*SAFETYMCUSW 79 S MR:19.4 <APPROVED> " Value comes from GUI drop down option " */
#define SYS_PRE2 (SYS_PLL1)

/* Configuration registers */
typedef struct system_config_reg
{
    uint32 CONFIG_SYSPC1;
    uint32 CONFIG_SYSPC2;
    uint32 CONFIG_SYSPC7;
    uint32 CONFIG_SYSPC8;
    uint32 CONFIG_SYSPC9;
    uint32 CONFIG_CSDIS;
    uint32 CONFIG_CDDIS;
    uint32 CONFIG_GHVSRC;
    uint32 CONFIG_VCLKASRC;
    uint32 CONFIG_RCLKSRC;
    uint32 CONFIG_MSTGCR;
    uint32 CONFIG_MINITGCR;
    uint32 CONFIG_MSINENA;
    uint32 CONFIG_PLLCTL1;
    uint32 CONFIG_PLLCTL2;
    uint32 CONFIG_UERFLAG;
    uint32 CONFIG_LPOMONCTL;
    uint32 CONFIG_CLKTEST;
    uint32 CONFIG_DFTCTRLREG1;
    uint32 CONFIG_DFTCTRLREG2;
    uint32 CONFIG_GPREG1;
    uint32 CONFIG_RAMGCR;
    uint32 CONFIG_BMMCR1;
    uint32 CONFIG_MMUGCR;
    uint32 CONFIG_CLKCNTL;
    uint32 CONFIG_ECPCNTL;
    uint32 CONFIG_DEVCR1;
    uint32 CONFIG_SYSECR;
    uint32 CONFIG_PLLCTL3;
    uint32 CONFIG_STCCLKDIV;
    uint32 CONFIG_CLK2CNTL;
    uint32 CONFIG_VCLKACON1;
    uint32 CONFIG_CLKSLIP;
    uint32 CONFIG_EFC_CTLEN;
} system_config_reg_t;

/* Configuration registers initial value */
#define SYS_SYSPC1_CONFIGVALUE  0U

#define SYS_SYSPC2_CONFIGVALUE  1U

#define SYS_SYSPC7_CONFIGVALUE  0U

#define SYS_SYSPC8_CONFIGVALUE  0U

#define SYS_SYSPC9_CONFIGVALUE  1U

#define SYS_CSDIS_CONFIGVALUE   ( 0x00000000U\
                                | 0x00000000U \
                                | 0x00000008U \
                                | 0x00000080U \
                                | 0x00000000U \
                                | 0x00000000U \
                                | 0x00000000U\
                                | 0x4U )
                      
#define SYS_CDDIS_CONFIGVALUE   ( (uint32)((uint32)0U << 4U )\
                                | (uint32)((uint32)1U << 5U )\
                                | (uint32)((uint32)0U << 8U )\
								| (uint32)((uint32)0U << 9U )\
                                | (uint32)((uint32)0U << 10U)\
                                | (uint32)((uint32)0U << 11U) )
                      
#define SYS_GHVSRC_CONFIGVALUE  ( (uint32)((uint32)SYS_OSC << 24U) \
                                | (uint32)((uint32)SYS_OSC << 16U) \
                                | (uint32)((uint32)SYS_PLL1 << 0U) )
                                
#define SYS_VCLKASRC_CONFIGVALUE    ( (uint32)((uint32)SYS_VCLK << 8U)\
                                    | (uint32)((uint32)SYS_VCLK << 0U) )
                                    
#define SYS_RCLKSRC_CONFIGVALUE     ( (uint32)((uint32)1U << 24U)\
                                    | (uint32)((uint32)SYS_VCLK << 16U)\
                                    | (uint32)((uint32)1U << 8U)\
                                    | (uint32)((uint32)SYS_VCLK << 0U) )
                                    
#define SYS_MSTGCR_CONFIGVALUE      0x00000105U

#define SYS_MINITGCR_CONFIGVALUE    0x5U

#define SYS_MSINENA_CONFIGVALUE     0U

#define SYS_PLLCTL1_CONFIGVALUE_1   ( (uint32)0x00000000U \
                                    | (uint32)0x20000000U \
                                    | (uint32)((uint32)0x1FU << 24U) \
                                    | (uint32)0x00000000U \
                                    | (uint32)((uint32)(6U - 1U)<< 16U)\
                                    | (uint32)(0xA400U))
                                    
#define SYS_PLLCTL1_CONFIGVALUE_2   (((SYS_PLLCTL1_CONFIGVALUE_1) & 0xE0FFFFFFU) | (uint32)((uint32)(1U - 1U) << 24U))
                                    
#define SYS_PLLCTL2_CONFIGVALUE     ( (uint32)0x00000000U\
                                    | (uint32)((uint32)255U << 22U)\
                                    | (uint32)((uint32)7U << 12U)\
                                    | (uint32)((uint32)(2U - 1U)<< 9U)\
                                    | (uint32)61U)
                                    
#define SYS_UERFLAG_CONFIGVALUE     0U

#define SYS_LPOMONCTL_CONFIGVALUE_1 ((uint32)((uint32)1U << 24U) | LPO_TRIM_VALUE)
#define SYS_LPOMONCTL_CONFIGVALUE_2 ((uint32)((uint32)1U << 24U) | (uint32)((uint32)16U << 8U) | 16U)

#define SYS_CLKTEST_CONFIGVALUE     0x000A0000U

#define SYS_DFTCTRLREG1_CONFIGVALUE 0x00002205U

#define SYS_DFTCTRLREG2_CONFIGVALUE 0x5U

#define SYS_GPREG1_CONFIGVALUE      0x0005FFFFU
                                    
#define SYS_RAMGCR_CONFIGVALUE      0x00050000U
                                    
#define SYS_BMMCR1_CONFIGVALUE      0xAU
                                    
#define SYS_MMUGCR_CONFIGVALUE      0U

#define SYS_CLKCNTL_CONFIGVALUE     ( 0x00000100U \
                                    | (uint32)((uint32)1U << 16U) \
                                    | (uint32)((uint32)1U << 24U) ) 
                                    
#define SYS_ECPCNTL_CONFIGVALUE     ( (uint32)((uint32)0U << 24U)\
                                    | (uint32)((uint32)0U << 23U)\
                                    | (uint32)((uint32)(8U - 1U) & 0xFFFFU) )
                                    
#define SYS_DEVCR1_CONFIGVALUE      0xAU
                                    
#define SYS_SYSECR_CONFIGVALUE      0x00004000U
#define SYS2_PLLCTL3_CONFIGVALUE_1  ( (uint32)((uint32)(2U - 1U) << 29U)\
                                    | (uint32)((uint32)0x1FU << 24U) \
                                    | (uint32)((uint32)(6U - 1U)<< 16U) \
                                    | (uint32)(0xA400U))
                                    
#define SYS2_PLLCTL3_CONFIGVALUE_2  (((SYS2_PLLCTL3_CONFIGVALUE_1) & 0xE0FFFFFFU) | (uint32)((uint32)(1U - 1U) << 24U))
#define SYS2_STCCLKDIV_CONFIGVALUE  0U
#define SYS2_CLK2CNTL_CONFIGVALUE   (1U | 0x00000100U)
#define SYS2_VCLKACON1_CONFIGVALUE  ( (uint32)((uint32)1U << 24U) \
                                    | (uint32)((uint32)1U << 20U) \
                                    | (uint32)((uint32)SYS_VCLK << 16U)\
                                    | (uint32)((uint32)5U << 8U)\
                                    | (uint32)((uint32)1U << 4U) \
                                    | (uint32)((uint32)SYS_VCLK << 0U) )
#define SYS2_CLKSLIP_CONFIGVALUE    0x5U
#define SYS2_EFC_CTLEN_CONFIGVALUE  0x5U
    
void systemGetConfigValue(system_config_reg_t *config_reg, config_value_type_t type);

/* USER CODE BEGIN (1) */
/* USER CODE END */

/* FlashW General Definitions */


/** @enum flashWPowerModes
*   @brief Alias names for flash bank power modes
*
*   This enumeration is used to provide alias names for the flash bank power modes:
*     - sleep
*     - standby
*     - active
*/
enum flashWPowerModes
{
    SYS_SLEEP   = 0U, /**< Alias for flash bank power mode sleep   */
    SYS_STANDBY = 1U, /**< Alias for flash bank power mode standby */
    SYS_ACTIVE  = 3U  /**< Alias for flash bank power mode active  */
};

/* USER CODE BEGIN (2) */
/* USER CODE END */


#define FSM_WR_ENA_HL       (*(volatile uint32 *)0xFFF87288U)
#define EEPROM_CONFIG_HL    (*(volatile uint32 *)0xFFF872B8U)

/* Configuration registers */
typedef struct tcmflash_config_reg
{
    uint32 CONFIG_FRDCNTL;
    uint32 CONFIG_FEDACCTRL1;
    uint32 CONFIG_FEDACCTRL2;
    uint32 CONFIG_FEDACSDIS;
    uint32 CONFIG_FBPROT;
    uint32 CONFIG_FBSE;
    uint32 CONFIG_FBAC;
    uint32 CONFIG_FBFALLBACK;
    uint32 CONFIG_FPAC1;
    uint32 CONFIG_FPAC2;
    uint32 CONFIG_FMAC;
    uint32 CONFIG_FLOCK;
    uint32 CONFIG_FDIAGCTRL;
    uint32 CONFIG_FEDACSDIS2;
} tcmflash_config_reg_t;

/* Configuration registers initial value */
#define TCMFLASH_FRDCNTL_CONFIGVALUE        (0x00000000U | (uint32)((uint32)3U << 8U) | (uint32)((uint32)1U << 4U) |  1U)
#define TCMFLASH_FEDACCTRL1_CONFIGVALUE     0x000A0005U
#define TCMFLASH_FEDACCTRL2_CONFIGVALUE     0U
#define TCMFLASH_FEDACSDIS_CONFIGVALUE      0U
#define TCMFLASH_FBPROT_CONFIGVALUE         0U
#define TCMFLASH_FBSE_CONFIGVALUE           0U
#define TCMFLASH_FBAC_CONFIGVALUE           0xFU
#define TCMFLASH_FBFALLBACK_CONFIGVALUE     ( (uint32)((uint32)SYS_ACTIVE << 14U) \
                                            | (uint32)((uint32)3U << 12U) \
                                            | (uint32)((uint32)3U << 10U) \
                                            | (uint32)((uint32)3U << 8U) \
                                            | (uint32)((uint32)3U << 6U) \
                                            | (uint32)((uint32)3U << 4U) \
                                            | (uint32)((uint32)SYS_ACTIVE << 2U) \
                                            | (uint32)((uint32)SYS_ACTIVE << 0U) )
                          
#define TCMFLASH_FPAC1_CONFIGVALUE          0x00C80001U
#define TCMFLASH_FPAC2_CONFIGVALUE          0U
#define TCMFLASH_FMAC_CONFIGVALUE           0U
#define TCMFLASH_FLOCK_CONFIGVALUE          0x55AAU
#define TCMFLASH_FDIAGCTRL_CONFIGVALUE      0x000A0000U
#define TCMFLASH_FEDACSDIS2_CONFIGVALUE     0U

void tcmflashGetConfigValue(tcmflash_config_reg_t *config_reg, config_value_type_t type);

/* USER CODE BEGIN (3) */
/* USER CODE END */


/* System Interface Functions */
void setupPLL(void);
void trimLPO(void);
void customTrimLPO(void);
void setupFlash(void);
void periphInit(void);
void mapClocks(void);
void systemInit(void);
void systemPowerDown(uint32 mode);

/*Configuration registers
* index 0: Even RAM
* index 1: Odd RAM
*/
typedef struct sram_config_reg
{
    uint32 CONFIG_RAMCTRL[2U];
    uint32 CONFIG_RAMTHRESHOLD[2U];
    uint32 CONFIG_RAMINTCTRL[2U];
    uint32 CONFIG_RAMTEST[2U];
    uint32 CONFIG_RAMADDRDECVECT[2U];
} sram_config_reg_t;

/* Configuration registers initial value */
#define SRAM_RAMCTRL_CONFIGVALUE        0x0005000AU
#define SRAM_RAMTHRESHOLD_CONFIGVALUE   1U
#define SRAM_RAMINTCTRL_CONFIGVALUE 1U
#define SRAM_RAMTEST_CONFIGVALUE        0x5U
#define SRAM_RAMADDRDECVECT_CONFIGVALUE 0U

void sramGetConfigValue(sram_config_reg_t *config_reg, config_value_type_t type);
#ifdef __cplusplus
}
#endif /*extern "C" */
#endif
