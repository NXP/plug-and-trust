/*
 *
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "se051h_nfc_comm_prov.h"
#include <assert.h>

#define THREAD_STACK_SIZE       CONFIG_MAIN_STACK_SIZE
#define configMINIMAL_STACK_SIZE CONFIG_MAIN_STACK_SIZE

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

#define PRIORITY 3

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

K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACK_SIZE);

struct k_thread gSSSExRtosTaskHandle;

#if (configAPPLICATION_ALLOCATED_HEAP && (!IMX_RT)) && (!FRDM_MCXW72)
uint8_t __attribute__((section(".heap"))) ucHeap[configTOTAL_HEAP_SIZE];
#endif

void se051h_nfc_comm_task(void *, void *, void *);

int main(int argc, char *argv[]) {
#if (SSS_HAVE_HOSTCRYPTO_MBEDTLS) && (SSS_HAVE_MBEDTLS_2_X)
  CRYPTO_InitHardware();
#endif
  k_thread_create(&gSSSExRtosTaskHandle, thread_stack, K_THREAD_STACK_SIZEOF(thread_stack),
                      se051h_nfc_comm_task, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
}

void se051h_nfc_comm_task(void *, void *, void *)
{
  uint8_t do_reset = CONFIG_SE05X_DO_RESET;
  uint8_t do_ec_key_provision = CONFIG_SE05X_DO_EC_KEY_PROVISION;
  uint8_t do_user_id_provision = CONFIG_SE05X_DO_USER_ID_PROVISION;
  uint8_t do_aes_key_provision = CONFIG_SE05X_DO_AES_KEY_PROVISION;
  uint8_t only_t4t_provision = CONFIG_SE05X_ONLY_T4T_PROVISION;
  uint8_t device_network_type = DEVICE_NETWORK_TYPE;
  uint8_t qrcode[] = QRCODE;
  uint8_t *qrcode_ptr = &qrcode[0];
  size_t qrcodeLen = sizeof(qrcode) - 1;
  size_t is_qr_code = 0;
  uint8_t tp_spake_passcode_set_no = CONFIG_SE05X_TP_SPAKE_PASSCODE_SET_NO;
  uint32_t tp_spake_itter_to_be_used = CONFIG_SE05X_TP_SPAKE_ITER_TO_BE_USED;
  uint8_t provision_with_policy = CONFIG_SE05X_PROVISION_WITH_POLICY;

  se051h_nfc_comm_prov(NULL, do_reset, only_t4t_provision, qrcode_ptr,
                       qrcodeLen, is_qr_code, device_network_type, tp_spake_passcode_set_no,
                       tp_spake_itter_to_be_used, do_ec_key_provision,
                       do_aes_key_provision, do_user_id_provision, provision_with_policy,
                       NULL, 0,
                       NULL, 0);
}

extern "C" void __wrap_exit(int __status) { assert(0); }