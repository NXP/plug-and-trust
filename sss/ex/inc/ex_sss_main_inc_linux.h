/*
 *
 * Copyright 2019,2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ax_reset.h"

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
    axReset_PowerUp();
}

void ex_sss_main_linux_unconf(void)
{
    axReset_PowerDown();
    axReset_HostUnconfigure();
}
