/*
 *
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */
#if defined(SSS_USE_FTR_FILE)
#include "fsl_sss_ftr.h"
#else
#include "fsl_sss_ftr_default.h"
#endif

#if SSS_USE_MBEDTLS_PSA_APIS
#define PSA_WANT_ALG_CMAC 1
#define PSA_WANT_ALG_CBC_NO_PADDING 1
#define PSA_WANT_ALG_CBC_PKCS7 1
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_OID_C
#define MBEDTLS_PK_C
#define PSA_WANT_KEY_TYPE_AES 1
#define PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY 1
#define PSA_WANT_ECC_SECP_R1_256 1
#define MBEDTLS_ECP_C
#ifndef MBEDTLS_BIGNUM_C
#define MBEDTLS_BIGNUM_C
#endif
#ifndef MBEDTLS_PK_HAVE_ECC_KEYS
#define MBEDTLS_PK_HAVE_ECC_KEYS
#endif
#endif