/* Copyright 2019-2021,2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 *
 */

#ifndef SSS_APIS_VERSION_INFO_H_INCLUDED
#define SSS_APIS_VERSION_INFO_H_INCLUDED

/* clang-format off */
#define SSS_APIS_PROD_NAME          "SSS_APIs"
#define SSS_APIS_VER_STRING_NUM     "v04.07.01_20250519"
#define SSS_APIS_PROD_NAME_VER_FULL "SSS_APIs_v04.07.01_20250519"
#define SSS_APIS_VER_MAJOR          (4u)
#define SSS_APIS_VER_MINOR          (7u)
#define SSS_APIS_VER_DEV            (1u)

/* v04.07 = 40007u */
#define SSS_APIS_VER_MAJOR_MINOR ( 0 \
    | (SSS_APIS_VER_MAJOR * 10000u)    \
    | (SSS_APIS_VER_MINOR))

/* v04.07.01 = 400070001ULL */
#define SSS_APIS_VER_MAJOR_MINOR_DEV ( 0 \
    | (SSS_APIS_VER_MAJOR * 10000*10000u)    \
    | (SSS_APIS_VER_MINOR * 10000u)    \
    | (SSS_APIS_VER_DEV))

/* clang-format on */

/* Version Information:
 * Generated by:
 *     scripts\version_info.py (v2019.01.17_00)
 *
 * Do not edit this file. Update:
 *     sss/version_info.txt instead.
 *
 * prod_name = "SSS_APIs"
 *
 * prod_desc = "SSS APIs"
 *
 * lang_c_prefix = prod_name.upper()
 *
 * lang_namespace = ""
 *
 * v_major  = "04"
 *
 * v_minor  = "07"
 *
 * v_dev    = "01"
 *
 * v_meta   = ""
 *
 * maturity = "B"
 *
 * #
 * # 03.00.00 : Changed Enums
 * #
 */

#endif /* SSS_APIS_VERSION_INFO_H_INCLUDED */
