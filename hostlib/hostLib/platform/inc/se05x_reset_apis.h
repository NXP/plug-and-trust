/*
 *
 * Copyright 2018-2019,2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _SE05X_API_H
#define _SE05X_API_H

#include <stdint.h>
#include "sm_types.h"

#if defined(SSS_USE_FTR_FILE)
#include "fsl_sss_ftr.h"
#else
#include "fsl_sss_ftr_default.h"
#endif

#if defined(T1oI2C)
U16 smComT1oI2C_ComReset(void* conn_ctx);
#endif


/*
 * Define Reset logic for power-on / reset pin on SE
 * Active high for SE050
 * Change SE_RESET_LOGIC to 0 for SE052F. (Rebuild the cmake options as - "cmake -DSE_RESET_LOGIC=0 .")
 */
#ifndef SE_RESET_LOGIC
#define SE_RESET_LOGIC SSS_HAVE_SE_RESET_LOGIC_1
#endif


/*
 * Based on the applet version (part of sss_session context), the reset logic is decided.
 * In case if the applet version is unknown, pass value 0. The reset logic will be based on SE_RESET_LOGIC.
 * Default value of SE_RESET_LOGIC is 1 (For SE050/SE051). Value can be changed as "cmake -DSE_RESET_LOGIC=0 .".
 *
 * For SE052F - applet version is 7021600. The reset logic is set to active low.
 * Other values of applet version - The reset logic is set to active high.
 */
void se05x_ic_reset(uint32_t applet_version);

#endif // _SE05X_API_H
