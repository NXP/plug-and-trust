/*
 *
 * Copyright 2018-2020, 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FSL_SSS_UTIL_RSA_SIGN_H
#define FSL_SSS_UTIL_RSA_SIGN_H

#if SSS_HAVE_APPLET_SE05X_IOT && SSSFTR_RSA
#include <fsl_sss_se05x_apis.h>
#include <fsl_sss_se05x_types.h>
#include <sm_types.h>

smStatus_t pkcs1_v15_encode(
    sss_se05x_asymmetric_t *context, const uint8_t *hash, size_t hash_len, uint8_t *out, size_t *out_len);

smStatus_t pkcs1_v15_encode_no_hash(
    sss_se05x_asymmetric_t *context, const uint8_t *hash, size_t hash_len, uint8_t *out, size_t *out_len);

uint8_t sss_mgf_mask_func(uint8_t *dst, size_t dlen, uint8_t *src, size_t slen, sss_algorithm_t sha_algorithm);

smStatus_t emsa_encode(sss_se05x_asymmetric_t *context, const uint8_t *hash, size_t hash_len, uint8_t *out, size_t *out_len);

smStatus_t emsa_decode_and_compare(
    sss_se05x_asymmetric_t *context, uint8_t *sig, size_t sig_len, const uint8_t *hash, size_t hash_len);

#endif //SSS_HAVE_APPLET_SE05X_IOT && SSSFTR_RSA

#endif
