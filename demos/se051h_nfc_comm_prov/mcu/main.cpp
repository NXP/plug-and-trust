/*
 *
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INC_FREERTOS_H /* Header guard of FreeRTOS */
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#endif /* INC_FREERTOS_H */
#include "se051h_nfc_comm_prov.h"
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
#include "ksdk_mbedtls.h"
#endif

#define DO_RESET 0 // Delete all provisioned data (QR code is not deleted).
#define DO_EC_KEY_PROVISION 0
#define DO_USER_ID_PROVISION 0
#define DO_AES_KEY_PROVISION 0
#define ONLY_T4T_PROVISION 0            // Provision only QR code in T4T applet.
#define QRCODE "MT:-24J0SGJ10KA0648G00" // QR code to provisioned in T4T applet.
#define TP_SPAKE_PASSCODE_SET_NO                                               \
  1 // Trust Provisioned pass-code set to be used (Possible values 1,2,3).
#define TP_SPAKE_ITTER_TO_BE_USED                                              \
  1000 // Trust Provisioned iteration count to be used (Possible values 1000,
       // 5000, 10000, 50000, 100000)

#define WIFI_NET_INTERFACE 0x1
#define THREAD_NET_INTERFACE 0x2
#define ETHERNET_NET_INTERFACE 0x4

// By default WiFi network interface will be enabled.
// Change to enable particular interface for NFC commissioning.
#define DEVICE_NETWORK_TYPE  (WIFI_NET_INTERFACE)
#define PROVISION_WITH_POLICY 0

static TaskHandle_t gSSSExRtosTaskHandle = NULL;

#if (configAPPLICATION_ALLOCATED_HEAP && (!IMX_RT)) && (!FRDM_MCXW72)
uint8_t __attribute__((section(".heap"))) ucHeap[configTOTAL_HEAP_SIZE];
#endif

void se051h_nfc_comm_task(void *pvParam);

extern "C" void BOARD_InitHardware(void);

int main(int argc, char *argv[]) {
  BOARD_InitHardware();
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
  uint8_t do_reset = DO_RESET;
  uint8_t do_ec_key_provision = DO_EC_KEY_PROVISION;
  uint8_t do_user_id_provision = DO_USER_ID_PROVISION;
  uint8_t do_aes_key_provision = DO_AES_KEY_PROVISION;
  uint8_t only_t4t_provision = ONLY_T4T_PROVISION;
  uint8_t device_network_type = DEVICE_NETWORK_TYPE;
  uint8_t qrcode[] = QRCODE;
  uint8_t *qrcode_ptr = &qrcode[0];
  size_t qrcodeLen = sizeof(qrcode) - 1;
  uint8_t tp_spake_passcode_set_no = TP_SPAKE_PASSCODE_SET_NO;
  uint32_t tp_spake_itter_to_be_used = TP_SPAKE_ITTER_TO_BE_USED;
  uint8_t provision_with_policy = PROVISION_WITH_POLICY;

  se051h_nfc_comm_prov(NULL, do_reset, only_t4t_provision, qrcode_ptr,
                       qrcodeLen, device_network_type, tp_spake_passcode_set_no,
                       tp_spake_itter_to_be_used, do_ec_key_provision,
                       do_aes_key_provision, do_user_id_provision, provision_with_policy, 
                       NULL, 0,
                       NULL, 0);
}

#if (defined(configCHECK_FOR_STACK_OVERFLOW) &&                                \
     (configCHECK_FOR_STACK_OVERFLOW > 0))
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  assert(0);
}
#endif

extern "C" void __wrap_exit(int __status) { assert(0); }
