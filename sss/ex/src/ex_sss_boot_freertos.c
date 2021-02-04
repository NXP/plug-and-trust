/*
 *
 * Copyright 2019,2020 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *
 * ex_sss_boot_freertos.c:  *The purpose and scope of this file*
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
#include <ex_sss_boot.h>
#if defined(MBEDTLS)
#include <ksdk_mbedtls.h>
#endif
#include <nxLog_App.h>
#include <sm_const.h>
#include <sm_timer.h>

/* *****************************************************************************************************************
 * Internal Definitions
 * ***************************************************************************************************************** */

#ifndef SE_NAME
#define SE_NAME "NO SE"
#endif

/* *****************************************************************************************************************
 * Type Definitions
 * ***************************************************************************************************************** */

/* *****************************************************************************************************************
 * Global and Static Variables
 * Total Size: NNNbytes
 * ***************************************************************************************************************** */

/* *****************************************************************************************************************
 * Private Functions Prototypes
 * ***************************************************************************************************************** */

/* *****************************************************************************************************************
 * Public Functions
 * ***************************************************************************************************************** */

int ex_sss_boot_rtos_init()
{
#if (AX_EMBEDDED) && defined(MBEDTLS)
    CRYPTO_InitHardware();
#if defined(FSL_FEATURE_SOC_SHA_COUNT) && (FSL_FEATURE_SOC_SHA_COUNT > 0)
    CLOCK_EnableClock(kCLOCK_Sha0);
#if defined(CPU_JN518X) || defined(LPC_K32W)
    RESET_PeripheralReset(kHASH_RST_SHIFT_RSTn);
#else
    RESET_PeripheralReset(kSHA_RST_SHIFT_RSTn);
#endif /* CPU_JN518X or LPC_K32W */ 
#endif /* SHA */
#endif /* (AX_EMBEDDED) && defined(MBEDTLS) */

#if (AX_EMBEDDED)
    LOG_I(
        "\r\nWarning: Running this example will issue a debug reset of the "
        "attached " SE_NAME "\r\n");
    LOG_I("  The content of the " SE_NAME " will be erased.\r\n");
    LOG_I("  ****************************************\r\n");
    LOG_I("Press any character to continue.\r\n");
    //GETCHAR();
#endif

    sm_initSleep();

    return 0;
}

/* *****************************************************************************************************************
 * Private Functions
 * ***************************************************************************************************************** */
