/*
 *
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INC_FREERTOS_H /* Header guard of FreeRTOS */
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#endif /* INC_FREERTOS_H */
#include "task.h"
#include <assert.h>
#include "se051h_nfc_comm_prov.h"
#if defined(SSS_USE_FTR_FILE)
#include "fsl_sss_ftr.h"
#else
#include "fsl_sss_ftr_default.h"
#endif

#if (SSS_HAVE_HOSTCRYPTO_MBEDTLS) && (SSS_HAVE_MBEDTLS_2_X)
#include "els_pkc_mbedtls.h"
#endif

static TaskHandle_t gSSSExRtosTaskHandle = NULL;

#if configAPPLICATION_ALLOCATED_HEAP
uint8_t __attribute__((section(".heap"))) ucHeap[configTOTAL_HEAP_SIZE];
#endif

void se051h_nfc_comm_task (void *pvParam);

extern "C" void BOARD_InitHardware(void);

int main (int argc, char * argv[])
{
    BOARD_InitHardware();
#if (SSS_HAVE_HOSTCRYPTO_MBEDTLS) && (SSS_HAVE_MBEDTLS_2_X)
    CRYPTO_InitHardware();
#endif
    if (xTaskCreate(&se051h_nfc_comm_task, "se051h_nfc_comm_task", 8000, NULL, 2, &gSSSExRtosTaskHandle) != pdPASS) {
        while (1)
        ;
    }
    vTaskStartScheduler();
}

void se051h_nfc_comm_task (void *pvParam){
    se051h_nfc_comm_prov(NULL);
}

#if (defined(configCHECK_FOR_STACK_OVERFLOW) && (configCHECK_FOR_STACK_OVERFLOW > 0))
void vApplicationStackOverflowHook(TaskHandle_t xTask, char * pcTaskName)
{
    assert(0);
}
#endif

extern "C" void __wrap_exit(int __status)
{
    assert(0);
}
