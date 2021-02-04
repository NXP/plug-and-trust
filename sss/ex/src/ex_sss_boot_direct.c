/*
 *
 * Copyright 2019,2020 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *
 * ex_sss_boot_direct.c:  *The purpose and scope of this file*
 *
 * Project:  SecureIoTMW-Debug@appboot-top-eclipse_x86
 *
 * $Date: Mar 10, 2019 $
 * $Author: ing05193 $
 * $Revision$
 */

/* *****************************************************************************************************************
 * Includes
 * ***************************************************************************************************************** */
#include <board.h>
#include <ex_sss_boot.h>
#if defined(MBEDTLS)
#include <ksdk_mbedtls.h>
#endif
#include <pin_mux.h>

#include "ax_reset.h"
#include "clock_config.h"

#ifdef CPU_MIMXRT1062DVL6A
#include "fsl_trng.h"
#include "fsl_dcp.h"
#define TRNG0 TRNG
#endif

/* *****************************************************************************************************************
 * Internal Definitions
 * ***************************************************************************************************************** */

/* *****************************************************************************************************************
 * Type Definitions
 * ***************************************************************************************************************** */

#ifdef USE_SERGER_RTT
extern void nInit_segger_Log(void);
#endif

/* *****************************************************************************************************************
 * Global and Static Variables
 * Total Size: NNNbytes
 * ***************************************************************************************************************** */

/* *****************************************************************************************************************
 * Private Functions Prototypes
 * ***************************************************************************************************************** */

static void ex_sss_boot_direct_frdm(void);
static void ex_sss_boot_direct_RT1060(void);
static void ex_sss_boot_direct_lpc54018(void);
static void ex_sss_boot_direct_lpc55s(void);
static void ex_sss_boot_direct_K32W(void);

/* *****************************************************************************************************************
 * Public Functions
 * ***************************************************************************************************************** */
sss_status_t ex_sss_boot_direct()
{
#if defined(FRDM_KW41Z) || defined(FRDM_K64F)
    BOARD_BootClockRUN();
#endif
#if defined(FRDM_K82F)
    BOARD_BootClockHSRUN();
#endif

#if defined(LPC_K32W)
        /* Security code to allow debug access */
    SYSCON->CODESECURITYPROT = 0x87654320;

    /* attach clock for USART(debug console) */
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    /* reset FLEXCOMM for USART */
    RESET_PeripheralReset(kFC0_RST_SHIFT_RSTn);

    BOARD_BootClockRUN();

#endif

    ex_sss_boot_direct_frdm();
    ex_sss_boot_direct_RT1060();
    ex_sss_boot_direct_lpc54018();
    ex_sss_boot_direct_lpc55s();
    ex_sss_boot_direct_K32W();
#ifdef FREEDOM
    LED_BLUE_ON();
#endif
#ifdef FRDM_K64F
    {
        /* For DHCP Ethernet */
        SYSMPU_Type *base = SYSMPU;
        base->CESR &= ~SYSMPU_CESR_VLD_MASK;
    }
#endif

#ifdef USE_SERGER_RTT
    nInit_segger_Log();
#endif

#if !defined(USE_RTOS) || USE_RTOS == 0
    ex_sss_boot_rtos_init();
#endif
    return 0;
}

/* *****************************************************************************************************************
 * Private Functions
 * ***************************************************************************************************************** */

static void ex_sss_boot_direct_frdm()
{
#ifdef FREEDOM
    BOARD_InitPins();
    BOARD_InitDebugConsole();

    LED_BLUE_INIT(1);
    LED_GREEN_INIT(1);
    LED_RED_INIT(1);
#endif
}

static void ex_sss_boot_direct_K32W()
{
#ifdef LPC_K32W
    BOARD_InitPins();
    BOARD_InitDebugConsole();

    LED_GREEN_INIT(1);
    LED_RED_INIT(1);
#endif
}


static void ex_sss_boot_direct_RT1060()
{
#ifdef CPU_MIMXRT1062DVL6A
    dcp_config_t dcpConfig;
    trng_config_t trngConfig;

    BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

#if defined(IMX_RT)
    /* Data cache must be temporarily disabled to be able to use sdram */
    SCB_DisableDCache();
#endif

    /* Initialize DCP */
    DCP_GetDefaultConfig(&dcpConfig);
    DCP_Init(DCP, &dcpConfig);

    /* Initialize TRNG */
    TRNG_GetDefaultConfig(&trngConfig);
    /* Set sample mode of the TRNG ring oscillator to Von Neumann, for better random data.
    * It is optional.*/
    trngConfig.sampleMode = kTRNG_SampleModeVonNeumann;

    /* Initialize TRNG */
    TRNG_Init(TRNG0, &trngConfig);
#endif
}

static void ex_sss_boot_direct_lpc54018()
{
#if defined(CPU_LPC54018)

    while (gDummy--) {
        /* Forcefully use gDummy so that linker does not kick it out */
    }

    /* attach 12 MHz clock to FLEXCOMM0 (debug console) */
    CLOCK_AttachClk(BOARD_DEBUG_USART_CLK_ATTACH);

    /* Enable Clock for RIT */
    CLOCK_EnableClock(kCLOCK_Gpio3);

    /* attach 12 MHz clock to FLEXCOMM2 (I2C master) */
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);

    /* reset FLEXCOMM for I2C */
    RESET_PeripheralReset(kFC2_RST_SHIFT_RSTn);

    BOARD_InitBootPins();
    BOARD_BootClockFROHF96M();
    BOARD_InitDebugConsole();

#endif /* CPU_LPC54018 */
}

static void ex_sss_boot_direct_lpc55s()
{
#if defined(LPC_55x)

    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    /* attach 12 MHz clock to FLEXCOMM8 (I2C master) */
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM4);

    /* reset FLEXCOMM for I2C */
    RESET_PeripheralReset(kFC4_RST_SHIFT_RSTn);

    BOARD_InitPins();
    BOARD_BootClockFROHF96M();
    BOARD_InitDebugConsole();

    LED_BLUE_INIT(1);
    LED_GREEN_INIT(1);
    LED_RED_INIT(1);

    LED_BLUE_ON();

    axReset_HostConfigure();
    axReset_PowerUp();

#endif /* LPC_55x */
}
