/*
 *
 * Copyright 2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INC_FREERTOS_H /* Header guard of FreeRTOS */
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#endif /* INC_FREERTOS_H */
#include "se05x_dev_attest_key_prov.h"
#include "task.h"
#include <assert.h>
#if SSS_USE_MBEDTLS_PSA_APIS
#include "nvs_port.h"
#include <settings.h>
#endif
#include <nxLog_App.h>
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

static TaskHandle_t gSSSExRtosTaskHandle = NULL;

void se05x_dev_attest_task(void *pvParam);

extern "C" void BOARD_InitHardware(void);

int main(int argc, char *argv[]) {
  BOARD_InitHardware();
#if (SSS_HAVE_HOSTCRYPTO_MBEDTLS) && (SSS_HAVE_MBEDTLS_2_X)
  CRYPTO_InitHardware();
#endif

#if SSS_USE_MBEDTLS_PSA_APIS
  const struct flash_area *fa = NULL;
  if (flash_area_open(SETTINGS_PARTITION, &fa) != 0) {
    LOG_E("flash_area_open failed");
    vTaskDelete(NULL);
    return 1;
  }

  if (flash_init(fa->fa_dev) != 0) {
    LOG_E("flash_init failed");
    vTaskDelete(NULL);
    return 1;
  }

  if (settings_subsys_init() != 0) {
    LOG_E("settings_subsys_init failed");
    vTaskDelete(NULL);
    return 1;
  }
#endif

  if (xTaskCreate(&se05x_dev_attest_task, "se05x_dev_attest_key_prov_task",
                  8000, NULL, 2, &gSSSExRtosTaskHandle) != pdPASS) {
    while (1)
      ;
  }
  vTaskStartScheduler();
}

void se05x_dev_attest_task(void *pvParam) { se05x_dev_attest_key_prov(NULL); }

#if (defined(configCHECK_FOR_STACK_OVERFLOW) &&                                \
     (configCHECK_FOR_STACK_OVERFLOW > 0))
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  assert(0);
}
#endif

extern "C" void __wrap_exit(int __status) { assert(0); }