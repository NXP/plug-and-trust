/*
 *
 * Copyright 2021,2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* ************************************************************************** */
/* Includes                                                                   */
/* ************************************************************************** */

#include <errno.h>
#include <ex_sss.h>
#include <ex_sss_boot.h>
#include <fsl_sss_se05x_apis.h>
#include <se05x_ecc_curves_values.h>
#include <nxEnsure.h>
#include <nxLog_App.h>
#include <stdio.h>
#include <string.h>
#include "se05x_APDU.h"
#include "ex_sss_objid.h"
#include "se05x_get_passcode.h"

#define BCD_TO_DEC(x) (x - 6 * ((x) >> 4))
#define SE05X_PASSOCDE_BINARY_FILEID        0X7fff2000
#define SE05X_PASSOCDE_SET_NO               1

ex_sss_boot_ctx_t gex_sss_chip_ctx;

static sss_status_t read_cer_and_get_passcode(uint32_t keyId)
{
    smStatus_t smstatus   = SM_NOT_OK;
    sss_object_t keyObject = { 0 };
    sss_status_t status = kStatus_SSS_Success;
    SE05x_Result_t exists = kSE05x_Result_NA;
    uint8_t buf[1024] = {0};
    uint8_t offset = (SE05X_PASSOCDE_SET_NO - 1) * (32 + 4);
    size_t buflen = sizeof(buf);
    size_t bufbitlen = sizeof(buf) *8;

    ENSURE_OR_RETURN_ON_ERROR(gex_sss_chip_ctx.ks.session != NULL, kStatus_SSS_Fail);

    smstatus = Se05x_API_CheckObjectExists(&((sss_se05x_session_t *) &gex_sss_chip_ctx.session)->s_ctx, keyId, &exists);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    if (exists == kSE05x_Result_SUCCESS)
    {
        status = sss_key_object_init(&keyObject, &gex_sss_chip_ctx.ks);
        ENSURE_OR_RETURN_ON_ERROR(status == kStatus_SSS_Success, status);

        status = sss_key_object_get_handle(&keyObject, keyId);
        ENSURE_OR_RETURN_ON_ERROR(status == kStatus_SSS_Success, status);

        status = sss_key_store_get_key(&gex_sss_chip_ctx.ks, &keyObject, buf, &buflen, &bufbitlen);
        ENSURE_OR_RETURN_ON_ERROR(status == kStatus_SSS_Success, status);

        uint32_t setUpPINCode_se05x = (BCD_TO_DEC(buf[offset+3])) + (100 * BCD_TO_DEC(buf[offset+2])) + (10000 * BCD_TO_DEC(buf[offset+1])) + (1000000 * BCD_TO_DEC(buf[offset]));

        LOG_I("Extracted PIN (BCD to Decimal): %u", setUpPINCode_se05x);
    }
    else
    {
        LOG_W("Pass-code Binary file does not exists");
        status = kStatus_SSS_Fail;
    }

    return status;
}

void se05x_get_passcode()
{
  sss_status_t status = kStatus_SSS_Success;
  const char *portName = nullptr;

  memset(&gex_sss_chip_ctx,  0,  sizeof(gex_sss_chip_ctx));

  status = ex_sss_boot_connectstring(0,  NULL,  (char**)&portName);
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  status = ex_sss_boot_open(&gex_sss_chip_ctx,  portName);
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  status = ex_sss_key_store_and_object_init(&gex_sss_chip_ctx);
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  status = read_cer_and_get_passcode(SE05X_PASSOCDE_BINARY_FILEID);

cleanup:

    if (kStatus_SSS_Success == status) {
        LOG_I("se05x_get_passcode example successful !!!...");
    } else {
        LOG_E("se05x_get_passcode example successful!!!...");
    }

    return;
}
