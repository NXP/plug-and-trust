/*
 *
 * Copyright 2018-2020 NXP
 * SPDX-License-Identifier: Apache-2.0
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

#include <fsl_sss_api.h>

#if SSS_HAVE_APPLET_A71CH || SSS_HAVE_APPLET_A71CH_SIM
#include <fsl_sscp_a71ch.h>
#endif
#if SSS_HAVE_HOSTCRYPTO_MBEDTLS
#include <fsl_sss_mbedtls_apis.h>
#endif
#if SSS_HAVE_HOSTCRYPTO_OPENSSL
#include <fsl_sss_openssl_apis.h>
#endif

#if SSS_HAVE_SSCP
#include <fsl_sss_sscp.h>
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

/* ************************************************************************** */
/* Global Variables                                                           */
/* ************************************************************************** */

/* ************************************************************************** */
/* Functions                                                                  */
/* ************************************************************************** */

/* Entry point for each individual SSS API Based example */

#endif /* SSS_EX_INC_EX_SSS_H_ */
