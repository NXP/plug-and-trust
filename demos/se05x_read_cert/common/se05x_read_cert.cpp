/*
 *
 * Copyright 2023 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/* ************************************************************************** */
/* Includes                                                                   */
/* ************************************************************************** */

#include <errno.h>
#include <ex_sss.h>
#include <ex_sss_boot.h>
#include <fsl_sss_se05x_apis.h>
#include <nxEnsure.h>
#include <nxLog_App.h>
#include <stdio.h>
#include <string.h>

#define CERT_BUFFER_LEN 1024

ex_sss_boot_ctx_t gex_sss_chip_ctx;

void se05x_read_cert(void)
{
  sss_status_t status = kStatus_SSS_Success;
  char *portName = nullptr;
  sss_object_t keyObject = { 0 };
  uint8_t buf[CERT_BUFFER_LEN] = {0};
  size_t buflen = sizeof(buf);
  size_t buflen_bits = sizeof(buf) * 8;
  size_t i = 0;
  int keyId = 0x7FFF2000;

  memset(&gex_sss_chip_ctx, 0, sizeof(gex_sss_chip_ctx));

  status = ex_sss_boot_connectstring(0, NULL, &portName);
  if (kStatus_SSS_Success != status) {
    printf("se05x error: %s\n", "ex_sss_boot_connectstring failed");
    goto exit;
  }

  status = ex_sss_boot_open(&gex_sss_chip_ctx, portName);
  if (kStatus_SSS_Success != status) {
    printf("se05x error: %s\n", "ex_sss_boot_open failed");
    goto exit;
  }

  status = ex_sss_key_store_and_object_init(&gex_sss_chip_ctx);
  if (kStatus_SSS_Success != status) {
    printf("se05x error: %s\n", "ex_sss_key_store_and_object_init failed");
    goto exit;
  }


  status = sss_key_object_init(&keyObject, &gex_sss_chip_ctx.ks);
  if (kStatus_SSS_Success != status) {
    printf("se05x error: %s\n", "sss_key_object_init failed");
    goto exit;
  }

  status = sss_key_object_get_handle(&keyObject, keyId);
  if (kStatus_SSS_Success != status) {
    printf("se05x error: %s\n", "sss_key_object_get_handle failed");
    goto exit;
  }

  status = sss_key_store_get_key(&gex_sss_chip_ctx.ks, &keyObject, buf, &buflen, &buflen_bits);
  if (kStatus_SSS_Success != status) {
    printf("se05x error: %s\n", "sss_key_store_get_key failed");
    goto exit;
  }

  printf("Certtificate ==> \n");
  for(i =0; i < buflen; i++){
    printf("%02x ", buf[i]);
  }
  printf("\n");

  printf("Se05x read certificate successful \n");

exit:
  ex_sss_session_close(&gex_sss_chip_ctx);
  return;
}
