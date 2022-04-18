/*
 *
 * Copyright 2021 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/* ************************************************************************** */
/* Includes                                                                   */
/* ************************************************************************** */


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ex_sss.h>
#include <ex_sss_boot.h>
#include <fsl_sss_se05x_apis.h>
#include <nxEnsure.h>
#include <nxLog_App.h>


/* clang-format off */
const uint8_t keyPairData[] = {
    0x30, 0x81, 0x87, 0x02, 0x01, 0x00, 0x30, 0x13,
    0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02,
    0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
    0x03, 0x01, 0x07, 0x04, 0x6D, 0x30, 0x6B, 0x02,
    0x01, 0x01, 0x04, 0x20,

    /* Private key */
    0xaa, 0xb6, 0x00, 0xae, 0x8a, 0xe8, 0xaa, 0xb7,
    0xd7, 0x36, 0x27, 0xc2, 0x17, 0xb7, 0xc2, 0x04,
    0x70, 0x9c, 0xa6, 0x94, 0x6a, 0xf5, 0xf2, 0xf7,
    0x53, 0x08, 0x33, 0xa5, 0x2b, 0x44, 0xfb, 0xff,

    0xA1, 0x44, 0x03, 0x42, 0x00,

    /* Public key */
    0x04, 0x46, 0x3a, 0xc6, 0x93, 0x42, 0x91, 0x0a,
    0x0e, 0x55, 0x88, 0xfc, 0x6f, 0xf5, 0x6b, 0xb6,
    0x3e, 0x62, 0xec, 0xce, 0xcb, 0x14, 0x8f, 0x7d,
    0x4e, 0xb0, 0x3e, 0xe5, 0x52, 0x60, 0x14, 0x15,
    0x76, 0x7d, 0x16, 0xa5, 0xc6, 0x63, 0xf7, 0x93,
    0xe4, 0x91, 0x23, 0x26, 0x0b, 0x82, 0x97, 0xa7,
    0xcd, 0x7e, 0x7c, 0xfc, 0x7b, 0x31, 0x6b, 0x39,
    0xd9, 0x8e, 0x90, 0xd2, 0x93, 0x77, 0x73, 0x8e,
    0x82,
};
/* clang-format on */

#define ATTESTATION_KEY_ID 0xDADADADA

ex_sss_boot_ctx_t gex_sss_chip_ctx;

void se05x_dev_attest_key_prov(void)
{
    sss_status_t status = kStatus_SSS_Success;
    sss_object_t keyPair;
    const char * portName = nullptr;

    memset(&gex_sss_chip_ctx, 0, sizeof(gex_sss_chip_ctx));

    status = ex_sss_boot_connectstring(0, NULL, &portName);
    if (kStatus_SSS_Success != status)
    {
        printf("se05x error: %s\n", "ex_sss_boot_connectstring failed");
        return;
    }

    status = ex_sss_boot_open(&gex_sss_chip_ctx, portName);
    if (kStatus_SSS_Success != status)
    {
        printf("se05x error: %s\n","ex_sss_boot_open failed");
        return;
    }

    status = ex_sss_key_store_and_object_init(&gex_sss_chip_ctx);
    if (kStatus_SSS_Success != status)
    {
        printf("se05x error: %s\n","ex_sss_key_store_and_object_init failed");
        return;
    }

    status = sss_key_object_init(&keyPair, &gex_sss_chip_ctx.ks);
    if(status != kStatus_SSS_Success){
        printf("Error in sss_key_object_init \n");
        return;
    }

    status = sss_key_object_allocate_handle(&keyPair,
        ATTESTATION_KEY_ID,
        kSSS_KeyPart_Pair,
        kSSS_CipherType_EC_NIST_P,
        sizeof(keyPairData),
        kKeyObject_Mode_Persistent);
    if(status != kStatus_SSS_Success){
        printf("Error in sss_key_object_allocate_handle \n");
        return;
    }

    status = sss_key_store_set_key(&gex_sss_chip_ctx.ks, &keyPair, keyPairData, sizeof(keyPairData), 256, NULL, 0);
    if(status != kStatus_SSS_Success){
        printf("Error in sss_key_store_set_key \n");
        return;
    }

    printf("Attestation key Provision successful \n");
    return;
}