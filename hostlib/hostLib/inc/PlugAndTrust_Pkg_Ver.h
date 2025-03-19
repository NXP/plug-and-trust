/* Copyright 2019-2021,2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 *
 */

#ifndef PLUGANDTRUST_VERSION_INFO_H_INCLUDED
#define PLUGANDTRUST_VERSION_INFO_H_INCLUDED


/* clang-format off */
#define PLUGANDTRUST_PROD_NAME          "PlugAndTrust"
#define PLUGANDTRUST_VER_STRING_NUM     "v04.07.00_20250210"
#define PLUGANDTRUST_PROD_NAME_VER_FULL "PlugAndTrust_v04.07.00_20250210"
#define PLUGANDTRUST_VER_MAJOR          (4u)
#define PLUGANDTRUST_VER_MINOR          (7u)
#define PLUGANDTRUST_VER_DEV            (0u)

/* v04.07 = 40007u */
#define PLUGANDTRUST_VER_MAJOR_MINOR ( 0 \
    | (PLUGANDTRUST_VER_MAJOR * 10000u)    \
    | (PLUGANDTRUST_VER_MINOR))

/* v04.07.00 = 400070000ULL */
#define PLUGANDTRUST_VER_MAJOR_MINOR_DEV ( 0 \
    | (PLUGANDTRUST_VER_MAJOR * 10000*10000u)    \
    | (PLUGANDTRUST_VER_MINOR * 10000u)    \
    | (PLUGANDTRUST_VER_DEV))

/* clang-format on */


/* Version Information:
 * Generated by:
 *     scripts\version_info.py (v2019.01.17_00)
 *
 * Do not edit this file. Update:
 *     ./version_info.txt instead.
 *
 * prod_name = "PlugAndTrust"
 *
 * prod_desc = "Plug And Trust Package"
 *
 * lang_c_prefix = prod_name.upper()
 *
 * lang_namespace = ""
 *
 * v_major  = "04"
 *
 * v_minor  = "07"
 *
 * v_dev    = "00"
 *
 * # Develop Branch
 * v_meta   = ""
 *
 * maturity = "B"
 *
 */

#endif /* PLUGANDTRUST_VERSION_INFO_H_INCLUDED */
