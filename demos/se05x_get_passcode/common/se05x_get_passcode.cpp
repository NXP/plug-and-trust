/*
 *
 * Copyright 2021,2025-2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* ************************************************************************** */
/* Includes                                                                   */
/* ************************************************************************** */

#include "se05x_get_passcode.h"
#include "ex_sss_objid.h"
#include "se05x_APDU.h"
#include "se05x_host_gpio.h"
#include <errno.h>
#include <ex_sss.h>
#include <ex_sss_boot.h>
#include <fsl_sss_se05x_apis.h>
#include <nxEnsure.h>
#include <nxLog_App.h>
#include <se05x_ecc_curves_values.h>
#include <stdio.h>
#include <string.h>

#define BCD_TO_DEC(x) (x - 6 * ((x) >> 4))
#define SE05X_PASSOCDE_BINARY_FILEID 0X7fff2000

ex_sss_boot_ctx_t gex_sss_chip_ctx;

static sss_status_t read_cer_and_get_passcode(uint32_t keyId,
                                              uint8_t passcode_set_no) {
  smStatus_t smstatus = SM_NOT_OK;
  sss_object_t keyObject = {0};
  sss_status_t status = kStatus_SSS_Success;
  SE05x_Result_t exists = kSE05x_Result_NA;
  uint8_t buf[1024] = {0};
  uint8_t offset = 0;
  size_t buflen = sizeof(buf);
  size_t bufbitlen = sizeof(buf) * 8;

  ENSURE_OR_RETURN_ON_ERROR(gex_sss_chip_ctx.ks.session != NULL,
                            kStatus_SSS_Fail);

  ENSURE_OR_RETURN_ON_ERROR(passcode_set_no > 0 && passcode_set_no < 8,
                            kStatus_SSS_Fail);

  offset = (uint8_t)((passcode_set_no - 1) * (32 + 4));

  smstatus = Se05x_API_CheckObjectExists(
      &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx, keyId,
      &exists);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  if (exists == kSE05x_Result_SUCCESS) {
    status = sss_key_object_init(&keyObject, &gex_sss_chip_ctx.ks);
    ENSURE_OR_RETURN_ON_ERROR(status == kStatus_SSS_Success, status);

    status = sss_key_object_get_handle(&keyObject, keyId);
    ENSURE_OR_RETURN_ON_ERROR(status == kStatus_SSS_Success, status);

    status = sss_key_store_get_key(&gex_sss_chip_ctx.ks, &keyObject, buf,
                                   &buflen, &bufbitlen);
    ENSURE_OR_RETURN_ON_ERROR(status == kStatus_SSS_Success, status);

    uint32_t setUpPINCode_se05x = (BCD_TO_DEC(buf[offset + 3])) +
                                  (100 * BCD_TO_DEC(buf[offset + 2])) +
                                  (10000 * BCD_TO_DEC(buf[offset + 1])) +
                                  (1000000 * BCD_TO_DEC(buf[offset]));

    LOG_I("Extracted PIN (BCD to Decimal): %u", setUpPINCode_se05x);
  } else {
    LOG_W("Pass-code Binary file does not exists");
    status = kStatus_SSS_Fail;
  }

  return status;
}

void se05x_get_passcode(ex_sss_boot_ctx_t *pCtx, uint8_t passcode_set_no) {
  sss_status_t status = kStatus_SSS_Success;
  const char *portName = nullptr;
  if (se05x_host_gpio_power_init() != 0) {
    LOG_E("SE05x - Error in se05x_host_gpio_power_init function");
    LOG_E("SE05x - Crypto operations offloaded to secure element will fail");
  }

  LOG_I("SE05x - Turn ON secure Element");
  if (se05x_host_gpio_power_set(1) != 0) {
    LOG_E("SE05x - Error in se05x_host_gpio_power_set(1) function");
  }

  if (pCtx == NULL) {
    memset(&gex_sss_chip_ctx, 0, sizeof(gex_sss_chip_ctx));

    status = ex_sss_boot_connectstring(0, NULL, (char **)&portName);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    status = ex_sss_boot_open(&gex_sss_chip_ctx, portName);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    status = ex_sss_key_store_and_object_init(&gex_sss_chip_ctx);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
  } else {
    memcpy(&gex_sss_chip_ctx, pCtx, sizeof(ex_sss_boot_ctx_t));
  }

  status =
      read_cer_and_get_passcode(SE05X_PASSOCDE_BINARY_FILEID, passcode_set_no);

cleanup:

  LOG_I("SE05x - Turn OFF secure Element");
  if (se05x_host_gpio_power_set(0) != 0) {
    LOG_E("SE05x - Failed to set the GPIO connected to SE05x to low");
  }

  LOG_I("SE05x - De-initialize GPIO");
  if (se05x_host_gpio_power_deinit() != 0) {
    LOG_E("SE05x - Failed to de-initialize GPIO connected to SE05x");
  }

  if (kStatus_SSS_Success == status) {
    LOG_I("se05x_get_passcode example successful !!!...");
  } else {
    LOG_E("se05x_get_passcode example failed!!!...");
  }

  return;
}
