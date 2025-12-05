/*
 *
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INC_FREERTOS_H /* Header guard of FreeRTOS */
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#endif /* INC_FREERTOS_H */
#include "se05x_get_passcode.h"
#include "task.h"
#include <assert.h>
#if defined(SSS_USE_FTR_FILE)
#include "fsl_sss_ftr.h"
#else
#include "fsl_sss_ftr_default.h"
#endif

#if (SSS_HAVE_HOSTCRYPTO_MBEDTLS) && (SSS_HAVE_MBEDTLS_2_X) && (FRDM_RW612)
#include "els_pkc_mbedtls.h"
#elif (SSS_HAVE_HOSTCRYPTO_MBEDTLS) && (SSS_HAVE_MBEDTLS_2_X) && (IMX_RT)
#include "fsl_common.h"
#include "ksdk_mbedtls.h"
#endif

#define TP_PASSCODE_SET_NO                                                     \
  1 // Get the passcode of the set number specified. (Possible values = 1,2,3)

static TaskHandle_t gSSSExRtosTaskHandle = NULL;

#if (configAPPLICATION_ALLOCATED_HEAP && (!IMX_RT))
uint8_t __attribute__((section(".heap"))) ucHeap[configTOTAL_HEAP_SIZE];
#endif

void se051_get_passcode_task(void *pvParam);

extern "C" void BOARD_InitHardware(void);

int main(int argc, char *argv[]) {
  BOARD_InitHardware();
#if (SSS_HAVE_HOSTCRYPTO_MBEDTLS) && (SSS_HAVE_MBEDTLS_2_X)
  CRYPTO_InitHardware();
#endif
  if (xTaskCreate(&se051_get_passcode_task, "se051_get_passcode_task", 8000,
                  NULL, 2, &gSSSExRtosTaskHandle) != pdPASS) {
    while (1)
      ;
  }
  vTaskStartScheduler();
}

void se051_get_passcode_task(void *pvParam) {
  uint8_t passcode_set_no = TP_PASSCODE_SET_NO;
  se05x_get_passcode(passcode_set_no);
}

#if (defined(configCHECK_FOR_STACK_OVERFLOW) &&                                \
     (configCHECK_FOR_STACK_OVERFLOW > 0))
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  assert(0);
}
#endif

extern "C" void __wrap_exit(int __status) { assert(0); }
