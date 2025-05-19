/*
 *
 * Copyright 2019,2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ax_reset.h"
#include "se05x_reset_apis.h"

#if defined(SSS_USE_FTR_FILE)
#include "fsl_sss_ftr.h"
#else
#include "fsl_sss_ftr_default.h"
#endif

void ex_sss_main_linux_conf(void);
void ex_sss_main_linux_unconf(void);

void ex_sss_main_linux_conf(void)
{
    axReset_HostConfigure();
    axReset_PowerUp(SE_RESET_LOGIC);
}

void ex_sss_main_linux_unconf(void)
{
    axReset_PowerDown(SE_RESET_LOGIC);
    axReset_HostUnconfigure();
}
