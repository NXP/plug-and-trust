/*
 *
 * Copyright 2018-2020,2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SSS_EX_INC_EX_SSS_H_
#define SSS_EX_INC_EX_SSS_H_

/* ************************************************************************** */
/* Includes                                                                   */
/* ************************************************************************** */

#if defined(SSS_USE_FTR_FILE)
#include "fsl_sss_ftr.h"
#else
#include "fsl_sss_ftr_default.h"
#endif

#include "inc/fsl_sss_api.h"


#if SSS_HAVE_HOSTCRYPTO_MBEDTLS
#include <fsl_sss_mbedtls_apis.h>
#endif
#if SSS_HAVE_HOSTCRYPTO_OPENSSL
#include <fsl_sss_openssl_apis.h>
#endif

/* ************************************************************************** */
/* Defines                                                                    */
/* ************************************************************************** */

#ifndef MAKE_TEST_ID
#define MAKE_TEST_ID(ID) (0xEF000000u + ID)
#endif /* MAKE_TEST_ID */

/* ************************************************************************** */
/* Structrues and Typedefs                                                    */
/* ************************************************************************** */

#if 0
typedef struct
{
    se_sss_session_t currentSession;

    se_sss_key_store_t ks;

    sss_sscp_session_t *sscp_session;

    sscp_context_t sscp;
    se_sss_asymmetric_t asymVerifyCtx;
    se_sss_asymmetric_t asymm;
    se_sss_object_t keyPair;
    se_sss_object_t extPubkey;

    se_sss_object_t Device_Cert;
    se_sss_object_t Pubkey;
    se_sss_object_t interCaCert;
    se_sss_object_t interkeyPair;
    se_sss_object_t clientCert;
#if SSS_HAVE_APPLET_SE05X_IOT
    se_sss_session_t hostSession;
    se_sss_key_store_t hostKs;
    se_sss_object_t hostKey;
#endif
    se_sss_symmetric_t symm;
    se_sss_rng_context_t rng;
    se_sss_mac_t mac;

} sss_ex_ctx_t;

#endif

/* ************************************************************************** */
/* Global Variables                                                           */
/* ************************************************************************** */

/* ************************************************************************** */
/* Functions                                                                  */
/* ************************************************************************** */

/* Entry point for each individual SSS API Based example */

#endif /* SSS_EX_INC_EX_SSS_H_ */
