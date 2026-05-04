/*
 *
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "se05x_get_passcode.h"
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

#ifndef CONFIG_SE05X_TP_SPAKE_PASSCODE_SET_NO
#define CONFIG_SE05X_TP_SPAKE_PASSCODE_SET_NO 1
#endif

K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACK_SIZE);

struct k_thread gSSSExRtosTaskHandle;

#if (configAPPLICATION_ALLOCATED_HEAP && (!IMX_RT)) && (!FRDM_MCXW72)
uint8_t __attribute__((section(".heap"))) ucHeap[configTOTAL_HEAP_SIZE];
#endif

void se051_get_passcode_task(void *, void *, void *);

int main(int argc, char *argv[]) {
#if (SSS_HAVE_HOSTCRYPTO_MBEDTLS) && (SSS_HAVE_MBEDTLS_2_X)
  CRYPTO_InitHardware();
#endif
  k_thread_create(&gSSSExRtosTaskHandle, thread_stack, K_THREAD_STACK_SIZEOF(thread_stack),
                      se051_get_passcode_task, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
}

void se051_get_passcode_task(void *, void *, void *)
{
  uint8_t passcode_set_no = CONFIG_SE05X_TP_SPAKE_PASSCODE_SET_NO;
  se05x_get_passcode(passcode_set_no);
}

extern "C" void __wrap_exit(int __status) { assert(0); }