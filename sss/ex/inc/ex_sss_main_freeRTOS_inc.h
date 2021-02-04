/*
 *
 * Copyright 2019 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/* Common, Re-Usable main implementation */
/* Include this header file only once in the application */

/*
 *  Applications control the boot flow by defining these varaibles.
 *
 *
 *  - EX_SSS_BOOT_PCONTEXT         : Pointer to ex_sss_boot_ctx_t
 *  - EX_SSS_BOOT_DO_ERASE : Delete all objects on boot up if 1
 *  - EX_SSS_BOOT_EXPOSE_ARGC_ARGV : Expose ARGC & ARGV from Command Line to Application
 *
 *
 *
 */

#include <ex_sss.h>
#include <ex_sss_boot.h>
#include <nxLog_App.h>

#ifdef EX_SSS_BOOT_PCONTEXT
#define PCONTEXT EX_SSS_BOOT_PCONTEXT
#else
#define PCONTEXT (NULL)
#endif

#if !defined(EX_SSS_BOOT_EXPOSE_ARGC_ARGV)
#error EX_SSS_BOOT_EXPOSE_ARGC_ARGV must be set to 0 or 1
#endif

#if EX_SSS_BOOT_EXPOSE_ARGC_ARGV
static int gex_sss_argc;
static const char **gex_sss_argv;
#endif

#if !defined(EX_SSS_BOOT_OPEN_HOST_SESSION)
#define EX_SSS_BOOT_OPEN_HOST_SESSION 1
#endif

int CreateAndRunDemoTask()
{
    int ret = 1;
    sss_status_t status;
    const char *portName;

    status = ex_sss_boot_connectstring(0, NULL, &portName);
    if (kStatus_SSS_Success != status) {
        LOG_E("ex_sss_boot_connectstring Failed");
        goto cleanup;
    }

    status = ex_sss_boot_open((PCONTEXT), portName);
    if (kStatus_SSS_Success != status) {
        LOG_E("ex_sss_session_open Failed");
        goto cleanup;
    }

#if EX_SSS_BOOT_DO_ERASE
    status = ex_sss_boot_factory_reset((PCONTEXT));
#endif

    status = ex_sss_key_store_and_object_init((PCONTEXT));
    if (kStatus_SSS_Success != status) {
        LOG_E("ex_sss_key_store_and_object_init Failed");
        goto cleanup;
    }

#if EX_SSS_BOOT_OPEN_HOST_SESSION && SSS_HAVE_HOSTCRYPTO_ANY
    ex_sss_boot_open_host_session((PCONTEXT));
#endif

    status = ex_sss_entry((PCONTEXT));
    if (kStatus_SSS_Success != status) {
        LOG_E("ex_sss_entry Failed");
        goto cleanup;
    }

    ret = 0;
cleanup:
    return ret;
}
