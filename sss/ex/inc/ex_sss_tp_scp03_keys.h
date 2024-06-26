/*
 *
 * Copyright 2019,2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SSS_AUTH_SE050_OEF_20191211_1809_
#define SSS_AUTH_SE050_OEF_20191211_1809_

/* ************************************************************************** */
/* Includes                                                                   */
/* ************************************************************************** */

#if defined(SSS_USE_FTR_FILE)
#include "fsl_sss_ftr.h"
#else
#include "fsl_sss_ftr_default.h"
#endif

/* ************************************************************************** */
/* Defines                                                                    */
/* ************************************************************************** */

/* clang-format off */

/* See https://www.nxp.com/docs/en/application-note/AN12436.pdf */

// Variant           ==> OEF ID
// SE050A1           ==> A204
// SE050A2           ==> A205
// SE050B1           ==> A202
// SE050B2           ==> A203
// SE050C1           ==> A200
// SE050C2           ==> A201
// Development Board ==> A1F4 (DEVKIT)

//SE051C             ==>0xA8FA
//SE051A             ==>0xA920
//SE050E              ==>0xA921
//A5000 gen          ==>0xA736
//SE050F2            ==>0xA92A

/* !!! GENERATED FILE. DO NOT MODIFY !!! */

#if (( \
     SSS_PFSCP_ENABLE_SE050A1 + \
     SSS_PFSCP_ENABLE_SE050A2 + \
     SSS_PFSCP_ENABLE_SE050B1 + \
     SSS_PFSCP_ENABLE_SE050B2 + \
     SSS_PFSCP_ENABLE_SE050C1 + \
     SSS_PFSCP_ENABLE_SE050C2 + \
     SSS_PFSCP_ENABLE_SE050_DEVKIT + \
     SSS_PFSCP_ENABLE_SE051A2 + \
     SSS_PFSCP_ENABLE_SE051C2 + \
     SSS_PFSCP_ENABLE_SE050F2 + \
     SSS_PFSCP_ENABLE_SE051C_0005A8FA + \
     SSS_PFSCP_ENABLE_SE051A_0001A920 + \
     SSS_PFSCP_ENABLE_SE050E_0001A921 + \
     SSS_PFSCP_ENABLE_A5000_0004A736 + \
     SSS_PFSCP_ENABLE_SE050F2_0001A92A + \
     SSS_PFSCP_ENABLE_SE052_B501 + \
     SSS_PFSCP_ENABLE_OTHER + \
     0) == 0)
#if defined(SSS_HAVE_SE05X_VER_03_XX) && SSS_HAVE_SE05X_VER_03_XX == 1
    #undef SSS_PFSCP_ENABLE_SE050_DEVKIT
    #define SSS_PFSCP_ENABLE_SE050_DEVKIT 1
#else
    #undef SSS_PFSCP_ENABLE_SE050E_0001A921
    #define SSS_PFSCP_ENABLE_SE050E_0001A921 1
#endif

#endif

#if (( \
     SSS_PFSCP_ENABLE_SE050A1 + \
     SSS_PFSCP_ENABLE_SE050A2 + \
     SSS_PFSCP_ENABLE_SE050B1 + \
     SSS_PFSCP_ENABLE_SE050B2 + \
     SSS_PFSCP_ENABLE_SE050C1 + \
     SSS_PFSCP_ENABLE_SE050C2 + \
     SSS_PFSCP_ENABLE_SE050_DEVKIT + \
     SSS_PFSCP_ENABLE_SE051A2 + \
     SSS_PFSCP_ENABLE_SE051C2 + \
     SSS_PFSCP_ENABLE_SE050F2 + \
     SSS_PFSCP_ENABLE_SE051C_0005A8FA + \
     SSS_PFSCP_ENABLE_SE051A_0001A920 + \
     SSS_PFSCP_ENABLE_SE050E_0001A921 + \
     SSS_PFSCP_ENABLE_A5000_0004A736 + \
     SSS_PFSCP_ENABLE_SE050F2_0001A92A + \
     SSS_PFSCP_ENABLE_SE052_B501 + \
     SSS_PFSCP_ENABLE_OTHER + \
     0) > 1)
#error "Select only one Platform SCP configuration"
#endif

// SSS_PFSCP_ENABLE_SE050A1
#if defined (SSS_PFSCP_ENABLE_SE050A1) && SSS_PFSCP_ENABLE_SE050A1 == 1
#define SSS_AUTH_KEY_ENC \
    { 0x34, 0xAE, 0x09, 0x67, 0xE3, 0x29, 0xE9, 0x51, 0x8E, 0x72, 0x65, 0xD5, 0xAD, 0xCC, 0x01, 0xC2, }
#define SSS_AUTH_KEY_MAC \
    { 0x52, 0xB2, 0x53, 0xCA, 0xDF, 0x47, 0x2B, 0xDB, 0x3D, 0x0F, 0xB3, 0x8E, 0x09, 0x77, 0x00, 0x99, }
#define SSS_AUTH_KEY_DEK \
    { 0xAC, 0xC9, 0x14, 0x31, 0xFE, 0x26, 0x81, 0x1B, 0x5E, 0xCB, 0xC8, 0x45, 0x62, 0x0D, 0x83, 0x44, }
#endif // SSS_PFSCP_ENABLE_SE050A1

// SSS_PFSCP_ENABLE_SE050A2
#if defined (SSS_PFSCP_ENABLE_SE050A2) && SSS_PFSCP_ENABLE_SE050A2 == 1
#define SSS_AUTH_KEY_ENC \
    { 0x46, 0xA9, 0xC4, 0x8C, 0x34, 0xEF, 0xE3, 0x44, 0xA5, 0x22, 0xE6, 0x67, 0x44, 0xF8, 0x99, 0x6A, }
#define SSS_AUTH_KEY_MAC \
    { 0x12, 0x03, 0xFF, 0x61, 0xDF, 0xBC, 0x9C, 0x86, 0x19, 0x6A, 0x22, 0x74, 0xAE, 0xF4, 0xED, 0x28, }
#define SSS_AUTH_KEY_DEK \
    { 0xF7, 0x56, 0x1C, 0x6F, 0x48, 0x33, 0x61, 0x19, 0xEE, 0x39, 0x43, 0x9A, 0xAB, 0x34, 0x09, 0x8E, }
#endif // SSS_PFSCP_ENABLE_SE050A2

// SSS_PFSCP_ENABLE_SE050B1
#if defined (SSS_PFSCP_ENABLE_SE050B1) && SSS_PFSCP_ENABLE_SE050B1 == 1
#define SSS_AUTH_KEY_ENC \
    { 0xD4, 0x99, 0xBC, 0x90, 0xDE, 0xA5, 0x42, 0xCF, 0x78, 0xD2, 0x5E, 0x13, 0xD6, 0x4C, 0xBB, 0x1F, }
#define SSS_AUTH_KEY_MAC \
    { 0x08, 0x15, 0x55, 0x96, 0x43, 0xFB, 0x79, 0xEB, 0x85, 0x01, 0xA0, 0xDC, 0x83, 0x3D, 0x90, 0x1F, }
#define SSS_AUTH_KEY_DEK \
    { 0xBE, 0x7D, 0xDF, 0xB4, 0x06, 0xE8, 0x1A, 0xE4, 0xE9, 0x66, 0x5A, 0x9F, 0xED, 0x64, 0x26, 0x7C, }
#endif // SSS_PFSCP_ENABLE_SE050B1

// SSS_PFSCP_ENABLE_SE050B2
#if defined (SSS_PFSCP_ENABLE_SE050B2) && SSS_PFSCP_ENABLE_SE050B2 == 1
#define SSS_AUTH_KEY_ENC \
    { 0x5F, 0xA4, 0x3D, 0x82, 0x02, 0xD2, 0x5E, 0x9A, 0x85, 0xB1, 0xFE, 0x7E, 0x2D, 0x26, 0x47, 0x8D, }
#define SSS_AUTH_KEY_MAC \
    { 0x10, 0x5C, 0xEA, 0x22, 0x19, 0xF5, 0x2B, 0xD1, 0x67, 0xA0, 0x74, 0x63, 0xC6, 0x93, 0x79, 0xC3, }
#define SSS_AUTH_KEY_DEK \
    { 0xD7, 0x02, 0x81, 0x57, 0xF2, 0xAD, 0x37, 0x2C, 0x74, 0xBE, 0x96, 0x9B, 0xCC, 0x39, 0x06, 0x27, }
#endif // SSS_PFSCP_ENABLE_SE050B2

// SSS_PFSCP_ENABLE_SE050C1
#if defined (SSS_PFSCP_ENABLE_SE050C1) && SSS_PFSCP_ENABLE_SE050C1 == 1
#define SSS_AUTH_KEY_ENC \
    { 0x85, 0x2B, 0x59, 0x62, 0xE9, 0xCC, 0xE5, 0xD0, 0xBE, 0x74, 0x6B, 0x83, 0x3B, 0xCC, 0x62, 0x87, }
#define SSS_AUTH_KEY_MAC \
    { 0xDB, 0x0A, 0xA3, 0x19, 0xA4, 0x08, 0x69, 0x6C, 0x8E, 0x10, 0x7A, 0xB4, 0xE3, 0xC2, 0x6B, 0x47, }
#define SSS_AUTH_KEY_DEK \
    { 0x4C, 0x2F, 0x75, 0xC6, 0xA2, 0x78, 0xA4, 0xAE, 0xE5, 0xC9, 0xAF, 0x7C, 0x50, 0xEE, 0xA8, 0x0C, }
#endif // SSS_PFSCP_ENABLE_SE050C1

// SSS_PFSCP_ENABLE_SE050C2
#if defined (SSS_PFSCP_ENABLE_SE050C2) && SSS_PFSCP_ENABLE_SE050C2 == 1
#define SSS_AUTH_KEY_ENC \
    { 0xBD, 0x1D, 0xE2, 0x0A, 0x81, 0xEA, 0xB2, 0xBF, 0x3B, 0x70, 0x9A, 0x9D, 0x69, 0xA3, 0x12, 0x54, }
#define SSS_AUTH_KEY_MAC \
    { 0x9A, 0x76, 0x1B, 0x8D, 0xBA, 0x6B, 0xED, 0xF2, 0x27, 0x41, 0xE4, 0x5D, 0x8D, 0x42, 0x36, 0xF5, }
#define SSS_AUTH_KEY_DEK \
    { 0x9B, 0x99, 0x3B, 0x60, 0x0F, 0x1C, 0x64, 0xF5, 0xAD, 0xC0, 0x63, 0x19, 0x2A, 0x96, 0xC9, 0x47, }
#endif // SSS_PFSCP_ENABLE_SE050C2

// SSS_PFSCP_ENABLE_SE050_DEVKIT
#if defined (SSS_PFSCP_ENABLE_SE050_DEVKIT) && SSS_PFSCP_ENABLE_SE050_DEVKIT == 1
#define SSS_AUTH_KEY_ENC \
    { 0x35, 0xC2, 0x56, 0x45, 0x89, 0x58, 0xA3, 0x4F, 0x61, 0x36, 0x15, 0x5F, 0x82, 0x09, 0xD6, 0xCD, }
#define SSS_AUTH_KEY_MAC \
    { 0xAF, 0x17, 0x7D, 0x5D, 0xBD, 0xF7, 0xC0, 0xD5, 0xC1, 0x0A, 0x05, 0xB9, 0xF1, 0x60, 0x7F, 0x78, }
#define SSS_AUTH_KEY_DEK \
    { 0xA1, 0xBC, 0x84, 0x38, 0xBF, 0x77, 0x93, 0x5B, 0x36, 0x1A, 0x44, 0x25, 0xFE, 0x79, 0xFA, 0x29, }
#endif // SSS_PFSCP_ENABLE_SE050_DEVKIT

// SSS_PFSCP_ENABLE_SE051A2
#if defined (SSS_PFSCP_ENABLE_SE051A2) && SSS_PFSCP_ENABLE_SE051A2 == 1
#define SSS_AUTH_KEY_ENC \
    { 0x84, 0x0A, 0x5D, 0x51, 0x79, 0x55, 0x11, 0xC9, 0xCE, 0xF0, 0xC9, 0x6F, 0xD2, 0xCB, 0xF0, 0x41, }
#define SSS_AUTH_KEY_MAC \
    { 0x64, 0x6B, 0xC2, 0xB8, 0xC3, 0xA4, 0xD9, 0xC1, 0xFA, 0x8D, 0x71, 0x16, 0xBE, 0x04, 0xFD, 0xFE, }
#define SSS_AUTH_KEY_DEK \
    { 0x03, 0xE6, 0x69, 0x9A, 0xCA, 0x94, 0x26, 0xD9, 0xC3, 0x89, 0x22, 0xF8, 0x91, 0x4C, 0xE5, 0xF7, }
#endif // SSS_PFSCP_ENABLE_SE051A2

// SSS_PFSCP_ENABLE_SE051C2
#if defined (SSS_PFSCP_ENABLE_SE051C2) && SSS_PFSCP_ENABLE_SE051C2 == 1
#define SSS_AUTH_KEY_ENC \
    { 0x88, 0xDB, 0xCD, 0x65, 0x82, 0x0D, 0x2A, 0xA0, 0x6F, 0xFA, 0xB9, 0x2A, 0xA8, 0xE7, 0x93, 0x64, }
#define SSS_AUTH_KEY_MAC \
    { 0xA8, 0x64, 0x4E, 0x2A, 0x04, 0xD9, 0xE9, 0xC8, 0xC0, 0xEA, 0x60, 0x86, 0x68, 0x29, 0x99, 0xE5, }
#define SSS_AUTH_KEY_DEK \
    { 0x8A, 0x38, 0x72, 0x38, 0x99, 0x88, 0x18, 0x44, 0xE2, 0xC1, 0x51, 0x3D, 0xAC, 0xD9, 0xF8, 0x0D, }
#endif // SSS_PFSCP_ENABLE_SE051C2

// SSS_PFSCP_ENABLE_SE050F2
#if defined (SSS_PFSCP_ENABLE_SE050F2) && SSS_PFSCP_ENABLE_SE050F2 == 1
#define SSS_AUTH_KEY_ENC \
    { 0x91, 0x88, 0xDA, 0x8C, 0xF3, 0x69, 0xCF, 0xA9, 0xA0, 0x08, 0x91, 0x62, 0x7B, 0x65, 0x34, 0x5A, }
#define SSS_AUTH_KEY_MAC \
    { 0xCB, 0x20, 0xF8, 0x09, 0xC7, 0xA0, 0x39, 0x32, 0xBC, 0x20, 0x3B, 0x0A, 0x01, 0x81, 0x6C, 0x81, }
#define SSS_AUTH_KEY_DEK \
    { 0x27, 0x8E, 0x61, 0x9D, 0x83, 0x51, 0x8E, 0x14, 0xC6, 0xF1, 0xE4, 0xFA, 0x96, 0x8B, 0xE5, 0x1C, }
#endif // SSS_PFSCP_ENABLE_SE050F2

// SSS_PFSCP_ENABLE_SE051C_0005A8FA
#if defined (SSS_PFSCP_ENABLE_SE051C_0005A8FA) && SSS_PFSCP_ENABLE_SE051C_0005A8FA == 1
#define SSS_AUTH_KEY_ENC \
    { 0xBF, 0xC2, 0xDB, 0xE1, 0x82, 0x8E, 0x03, 0x5D, 0x3E, 0x7F, 0xA3, 0x6B, 0x90, 0x2A, 0x05, 0xC6, }
#define SSS_AUTH_KEY_MAC \
    { 0xBE, 0xF8, 0x5B, 0xD7, 0xBA, 0x04, 0x97, 0xD6, 0x28, 0x78, 0x1C, 0xE4, 0x7B, 0x18, 0x8C, 0x96, }
#define SSS_AUTH_KEY_DEK \
    { 0xD8, 0x73, 0xF3, 0x16, 0xBE, 0x29, 0x7F, 0x2F, 0xC9, 0xC0, 0xE4, 0x5F, 0x54, 0x71, 0x06, 0x99, }
#endif // SSS_PFSCP_ENABLE_SE051C_0005A8FA

// SSS_PFSCP_ENABLE_SE051A_0001A920
#if defined (SSS_PFSCP_ENABLE_SE051A_0001A920) && SSS_PFSCP_ENABLE_SE051A_0001A920 == 1
#define SSS_AUTH_KEY_ENC \
    { 0x88, 0xEA, 0x9F, 0xA6, 0x86, 0xF3, 0xCF, 0x2F, 0xFC, 0xAF, 0x4B, 0x1C, 0xBA, 0x93, 0xE4, 0x42, }
#define SSS_AUTH_KEY_MAC \
    { 0x4F, 0x16, 0x3F, 0x59, 0xF0, 0x74, 0x31, 0xF4, 0x3E, 0xE2, 0xEE, 0x18, 0x34, 0xA5, 0x23, 0x34, }
#define SSS_AUTH_KEY_DEK \
    { 0xD4, 0x76, 0xCF, 0x47, 0xAA, 0x27, 0xB5, 0x4A, 0xB3, 0xDB, 0xEB, 0xE7, 0x65, 0x6D, 0x67, 0x70, }
#endif // SSS_PFSCP_ENABLE_SE051A_0001A920

// SSS_PFSCP_ENABLE_SE050E_0001A921
#if defined (SSS_PFSCP_ENABLE_SE050E_0001A921) && SSS_PFSCP_ENABLE_SE050E_0001A921 == 1
#define SSS_AUTH_KEY_ENC \
    { 0xD2, 0xDB, 0x63, 0xE7, 0xA0, 0xA5, 0xAE, 0xD7, 0x2A, 0x64, 0x60, 0xC4, 0xDF, 0xDC, 0xAF, 0x64, }
#define SSS_AUTH_KEY_MAC \
    { 0x73, 0x8D, 0x5B, 0x79, 0x8E, 0xD2, 0x41, 0xB0, 0xB2, 0x47, 0x68, 0x51, 0x4B, 0xFB, 0xA9, 0x5B, }
#define SSS_AUTH_KEY_DEK \
    { 0x67, 0x02, 0xDA, 0xC3, 0x09, 0x42, 0xB2, 0xC8, 0x5E, 0x7F, 0x47, 0xB4, 0x2C, 0xED, 0x4E, 0x7F, }
#endif // SSS_PFSCP_ENABLE_SE050E_0001A921

// SSS_PFSCP_ENABLE_A5000_0004A736
#if defined (SSS_PFSCP_ENABLE_A5000_0004A736) && SSS_PFSCP_ENABLE_A5000_0004A736 == 1
#define SSS_AUTH_KEY_ENC \
    { 0xC9, 0x11, 0x85, 0x00, 0xB5, 0xFF, 0xA1, 0x43, 0x3A, 0x50, 0x22, 0x6F, 0x48, 0x9A, 0x0A, 0xA5, }
#define SSS_AUTH_KEY_MAC \
    { 0x29, 0xD2, 0xFE, 0x28, 0xF7, 0xFE, 0xEB, 0x15, 0x30, 0x68, 0xBE, 0x38, 0x1F, 0x61, 0xBC, 0x01, }
#define SSS_AUTH_KEY_DEK \
    { 0x61, 0x24, 0xD3, 0x84, 0x02, 0x11, 0x80, 0x60, 0xED, 0x91, 0x03, 0x60, 0xFC, 0x5A, 0x42, 0x78, }
#endif // SSS_PFSCP_ENABLE_A5000_0004A736

// SSS_PFSCP_ENABLE_SE050F2_0001A92A
#if defined (SSS_PFSCP_ENABLE_SE050F2_0001A92A) && SSS_PFSCP_ENABLE_SE050F2_0001A92A == 1
#define SSS_AUTH_KEY_ENC \
    { 0xB5, 0x0E, 0x1F, 0x12, 0xB8, 0x1F, 0xE5, 0x3B, 0x6C, 0x3B, 0x53, 0x87, 0x91, 0x2A, 0x1A, 0x5A, }
#define SSS_AUTH_KEY_MAC \
    { 0x71, 0x93, 0x69, 0x59, 0xD3, 0x7F, 0x2B, 0x22, 0xC5, 0xA0, 0xC3, 0x49, 0x19, 0xA2, 0xBC, 0x1F, }
#define SSS_AUTH_KEY_DEK \
    { 0x86, 0x95, 0x93, 0x23, 0x98, 0x54, 0xDC, 0x0D, 0x86, 0x99, 0x00, 0x50, 0x0C, 0xA7, 0x9C, 0x15, }
#endif // SSS_PFSCP_ENABLE_SE050F2_0001A92A

// SSS_PFSCP_ENABLE_SE052_B501
#if defined (SSS_PFSCP_ENABLE_SE052_B501) && SSS_PFSCP_ENABLE_SE052_B501 == 1
#define SSS_AUTH_KEY_ENC \
    { 0x3A, 0xE4, 0x41, 0xC7, 0x47, 0xE3, 0x2E, 0xBC, 0x16, 0xB3, 0xBB, 0x2D, 0x84, 0x3C, 0x6D, 0xD8, }
#define SSS_AUTH_KEY_MAC \
    { 0x6C, 0x18, 0xF3, 0xD0, 0x8F, 0xEE, 0x1C, 0xB9, 0x6A, 0x3C, 0x8D, 0xE5, 0xD3, 0x53, 0x8A, 0xAA, }
#define SSS_AUTH_KEY_DEK \
    { 0xB0, 0xE6, 0xA5, 0x69, 0x7D, 0xBD, 0x92, 0x92, 0x43, 0xA4, 0x82, 0xCF, 0x9E, 0x4D, 0x65, 0x22, }
#endif // SSS_PFSCP_ENABLE_SE052_B501

// SSS_PFSCP_ENABLE_OTHER
#if defined (SSS_PFSCP_ENABLE_OTHER) && SSS_PFSCP_ENABLE_OTHER == 1
#define SSS_AUTH_KEY_ENC \
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }
#define SSS_AUTH_KEY_MAC \
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }
#define SSS_AUTH_KEY_DEK \
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }
#endif // SSS_PFSCP_ENABLE_OTHER

/* clang-format on */

#endif /* SSS_AUTH_SE050_OEF_20191211_1809_  */