/*
 *
 * Copyright 2019 NXP
 * SPDX-License-Identifier: Apache-2.0
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
