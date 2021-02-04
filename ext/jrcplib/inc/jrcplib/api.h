/* Copyright 2019 NXP
 *
 * This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms.  By expressly
 * accepting such terms or by downloading, installing, activating and/or
 * otherwise using the software, you are agreeing that you have read, and
 * that you agree to comply with and are bound by, such license terms.  If
 * you do not agree to be bound by the applicable license terms, then you
 * may not retain, install, activate or otherwise use the software.
 */
#ifndef JRCPLIB_CORE_H_
#define JRCPLIB_CORE_H_
/** @file api.h */

/* TODO: This macro must be ported to all applicable platforms/compilers. */
#ifdef JRCPLIB_BUILD
    #ifdef _MSC_VER
        #define JRCPLIB_EXPORT_API extern "C" __declspec(dllexport)
    #else
        #define JRCPLIB_EXPORT_API __attribute__((visibility("default")))
    #endif
#else
    #ifdef __cplusplus
        #define JRCPLIB_EXPORT_API extern "C"
    #else
        #define JRCPLIB_EXPORT_API
    #endif
#endif

#endif /* End of JRCPLIB_CORE_H_ */
