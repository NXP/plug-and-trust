/*
 *
 * Copyright 2025-2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INC_FREERTOS_H /* Header guard of FreeRTOS */
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#endif /* INC_FREERTOS_H */
#include "se051h_nfc_comm_prov.h"
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
#include "ksdk_mbedtls.h"
#endif

/* Configuration defaults when Kconfig is not used */
#ifndef CONFIG_SE05X_DO_RESET
#define CONFIG_SE05X_DO_RESET 0
#endif

#ifndef CONFIG_SE05X_DO_EC_KEY_PROVISION
#define CONFIG_SE05X_DO_EC_KEY_PROVISION 0
#endif

#ifndef CONFIG_SE05X_DO_USER_ID_PROVISION
#define CONFIG_SE05X_DO_USER_ID_PROVISION 0
#endif

#ifndef CONFIG_SE05X_DO_AES_KEY_PROVISION
#define CONFIG_SE05X_DO_AES_KEY_PROVISION 0
#endif

#ifndef CONFIG_SE05X_ONLY_T4T_PROVISION
#define CONFIG_SE05X_ONLY_T4T_PROVISION 0
#endif

#ifndef CONFIG_SE05X_TP_SPAKE_PASSCODE_SET_NO
#define CONFIG_SE05X_TP_SPAKE_PASSCODE_SET_NO 1
#endif

#ifndef CONFIG_SE05X_TP_SPAKE_ITER_TO_BE_USED
#define CONFIG_SE05X_TP_SPAKE_ITER_TO_BE_USED 1000
#endif

#ifndef CONFIG_SE05X_PROVISION_WITH_POLICY
#define CONFIG_SE05X_PROVISION_WITH_POLICY 0
#endif

#ifndef CONFIG_SE05X_PROVISION_VERIFIERS
#define CONFIG_SE05X_PROVISION_VERIFIERS 0
#endif

#ifndef CONFIG_SE05X_DO_READIDLIST
#define CONFIG_SE05X_DO_READIDLIST 0
#endif

/* Network interface type configuration */
#define WIFI_NET_INTERFACE 0x1
#define THREAD_NET_INTERFACE 0x2
#define ETHERNET_NET_INTERFACE 0x4

#if defined(CONFIG_SE05X_DEVICE_NETWORK_TYPE_WIFI)
#define DEVICE_NETWORK_TYPE WIFI_NET_INTERFACE
#elif defined(CONFIG_SE05X_DEVICE_NETWORK_TYPE_THREAD)
#define DEVICE_NETWORK_TYPE THREAD_NET_INTERFACE
#elif defined(CONFIG_SE05X_DEVICE_NETWORK_TYPE_ETHERNET)
#define DEVICE_NETWORK_TYPE ETHERNET_NET_INTERFACE
#else
#define DEVICE_NETWORK_TYPE WIFI_NET_INTERFACE
#endif

#define QRCODE "MT:-24J0SGJ10KA0648G00" // QR code to provisioned in T4T applet

static TaskHandle_t gSSSExRtosTaskHandle = NULL;

void se051h_nfc_comm_task(void *pvParam);

extern "C" void BOARD_InitHardware(void);

int main(int argc, char *argv[]) {
  BOARD_InitHardware();
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
#if (SSS_HAVE_HOSTCRYPTO_MBEDTLS) && (SSS_HAVE_MBEDTLS_2_X)
  CRYPTO_InitHardware();
#endif
  if (xTaskCreate(&se051h_nfc_comm_task, "se051h_nfc_comm_task", 8000, NULL, 2,
                  &gSSSExRtosTaskHandle) != pdPASS) {
    while (1)
      ;
  }
  vTaskStartScheduler();
}

void se051h_nfc_comm_task(void *pvParam) {
  uint8_t do_reset = CONFIG_SE05X_DO_RESET;
  uint8_t do_ec_key_provision = CONFIG_SE05X_DO_EC_KEY_PROVISION;
  uint8_t do_user_id_provision = CONFIG_SE05X_DO_USER_ID_PROVISION;
  uint8_t do_aes_key_provision = CONFIG_SE05X_DO_AES_KEY_PROVISION;
  uint8_t only_t4t_provision = CONFIG_SE05X_ONLY_T4T_PROVISION;
  uint8_t device_network_type = DEVICE_NETWORK_TYPE;
  uint8_t qrcode[] = QRCODE;
  uint8_t *qrcode_ptr = &qrcode[0];
  size_t qrcodeLen = sizeof(qrcode) - 1;
  size_t is_qr_code = 1;
  uint8_t tp_spake_passcode_set_no = CONFIG_SE05X_TP_SPAKE_PASSCODE_SET_NO;
  uint32_t tp_spake_itter_to_be_used = CONFIG_SE05X_TP_SPAKE_ITER_TO_BE_USED;
  uint8_t provision_with_policy = CONFIG_SE05X_PROVISION_WITH_POLICY;
  uint8_t provision_verifiers = CONFIG_SE05X_PROVISION_VERIFIERS;
  uint8_t do_delete_key = 0;
  uint32_t delete_keyid = 0;
  uint8_t do_readidlist = CONFIG_SE05X_DO_READIDLIST;
  uint8_t se05x_t4t_access_ctrl_option = 0;
  uint8_t doresetcryproobjects = 0;

  se051h_nfc_comm_prov(
      NULL, do_reset, only_t4t_provision, qrcode_ptr, qrcodeLen, is_qr_code,
      device_network_type, tp_spake_passcode_set_no, tp_spake_itter_to_be_used,
      do_ec_key_provision, do_aes_key_provision, do_user_id_provision,
      provision_with_policy, NULL, 0, NULL, 0, provision_verifiers,
      do_delete_key, delete_keyid, do_readidlist, se05x_t4t_access_ctrl_option,
      doresetcryproobjects);
  vTaskDelete(NULL);
}

#if (defined(configCHECK_FOR_STACK_OVERFLOW) &&                                \
     (configCHECK_FOR_STACK_OVERFLOW > 0))
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  assert(0);
}
#endif

extern "C" void __wrap_exit(int __status) { assert(0); }
