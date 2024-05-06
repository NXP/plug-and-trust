/*
 *
 * Copyright 2019-2020,2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Common, Re-Usable main implementation */
/* Include this header file only once in the application */

/*
 *  Applications control the boot flow by defining these macros.
 *
 *
 *  - EX_SSS_BOOT_PCONTEXT : Pointer to ex_sss_boot_ctx_t
 *      This allows that boot framework do not blindly rely on
 *      global variables.
 *
 *  - EX_SSS_BOOT_DO_ERASE : Delete all objects on boot up if 1
 *      Few examples expect the IC is *empty*, and few examples
 *      expect to work with previously provisioned/persisted data.
 *      This variable allows to over-ride that behaviour.
 *
 *  - EX_SSS_BOOT_EXPOSE_ARGC_ARGV : Expose ARGC & ARGV from Command
 *      line to Application.
 *      When running from PC/Linux/OSX, command line arguments allow
 *      to choose extra command line parameters, e.g. Input/Output
 *      certificate or signing/verifying data.
 *      But on embedded platforms, such feature is not possible to
 *      achieve.
 *
 *  Optional variables:
 *
 *  - EX_SSS_BOOT_OPEN_HOST_SESSION : For examples that do not
 *      need host side implementation, his allows to skip opening
 *      the host session. (Host session is needed to either re-verify
 *      test data at host, or for SCP03).
 *      By default this is enabled.
 *
 */


#include "ex_sss_ports.h"
#if defined(__linux__) && defined(T1oI2C)
#if SSS_HAVE_APPLET_SE05X_IOT
#include "ex_sss_main_inc_linux.h"
#endif
#endif
#include <string.h> /* memset */
#include <limits.h>
#include "PlugAndTrust_Pkg_Ver.h"
#include "string.h" /* memset */

#ifdef EX_SSS_BOOT_PCONTEXT
#define PCONTEXT EX_SSS_BOOT_PCONTEXT
#else
#define PCONTEXT (NULL)
#endif

#if !defined(EX_SSS_BOOT_DO_ERASE)
#error EX_SSS_BOOT_DO_ERASE must be set to 0 or 1
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

#if defined(CPU_JN518X)
/* Allocate the memory for the heap. */
uint8_t __attribute__((section(".bss.$SRAM1"))) ucHeap[configTOTAL_HEAP_SIZE];
#endif

int main(int argc, const char *argv[])
{
    int ret;
    sss_status_t status = kStatus_SSS_Fail;
    char *portName;

#if EX_SSS_BOOT_EXPOSE_ARGC_ARGV
    gex_sss_argc = argc;
    gex_sss_argv = argv;
#endif // EX_SSS_BOOT_EXPOSE_ARGC_ARGV

#if defined(__linux__) && defined(T1oI2C) && SSS_HAVE_APPLET_SE05X_IOT
    ex_sss_main_linux_conf();
#endif // defined(__linux__) && defined(T1oI2C) && SSS_HAVE_APPLET_SE05X_IOT

    LOG_I(PLUGANDTRUST_PROD_NAME_VER_FULL);

#ifdef EX_SSS_BOOT_PCONTEXT
    memset((EX_SSS_BOOT_PCONTEXT), 0, sizeof(*(EX_SSS_BOOT_PCONTEXT)));
#endif // EX_SSS_BOOT_PCONTEXT

    status = ex_sss_boot_connectstring(argc, argv, &portName);
    if (kStatus_SSS_Success != status) {
        LOG_E("ex_sss_boot_connectstring Failed");
        goto cleanup;
    }

    /* Initialise Logging locks */
    if (nLog_Init() != 0) {
        LOG_E("Lock initialisation failed");
    }
#if defined(EX_SSS_BOOT_SKIP_SELECT_APPLET) && (EX_SSS_BOOT_SKIP_SELECT_APPLET == 1)
    (PCONTEXT)->se05x_open_ctx.skip_select_applet = 1;
#endif

    if (ex_sss_boot_isHelp(portName)) {
        memset(PCONTEXT, 0, sizeof(*PCONTEXT));
#if EX_SSS_BOOT_EXPOSE_ARGC_ARGV
        /* so that tool can fetchup last value */
        gex_sss_argc++;
#endif // EX_SSS_BOOT_EXPOSE_ARGC_ARGV
        goto before_ex_sss_entry;
    }

    status = ex_sss_boot_open(PCONTEXT, portName);
    if (kStatus_SSS_Success != status) {
        LOG_E("ex_sss_session_open Failed");
        goto cleanup;
    }

#if EX_SSS_BOOT_DO_ERASE
    status = ex_sss_boot_factory_reset((PCONTEXT));
#endif

    if (kType_SSS_SubSystem_NONE == ((PCONTEXT)->session.subsystem)) {
        /* Nothing to do. Device is not opened
         * This is needed for the case when we open a generic communication
         * channel, without being specific to SE05X
         */
    }
    else {
        status = ex_sss_key_store_and_object_init((PCONTEXT));
        if (kStatus_SSS_Success != status) {
            LOG_E("ex_sss_key_store_and_object_init Failed");
            goto cleanup;
        }
    }

#if EX_SSS_BOOT_OPEN_HOST_SESSION && SSS_HAVE_HOSTCRYPTO_ANY
    ex_sss_boot_open_host_session((PCONTEXT));
#endif

before_ex_sss_entry:

    status = ex_sss_entry((PCONTEXT));
    LOG_I("ex_sss Finished");
    if (kStatus_SSS_Success != status) {
        LOG_E("ex_sss_entry Failed");
        goto cleanup;
    }
    // Delete locks for pthreads
    nLog_DeInit();
    goto cleanup;

cleanup:
#ifdef EX_SSS_BOOT_PCONTEXT
    ex_sss_session_close((EX_SSS_BOOT_PCONTEXT));
#endif
    if (kStatus_SSS_Success == status) {
        ret = 0;
#if defined(__linux__) && defined(T1oI2C) && SSS_HAVE_APPLET_SE05X_IOT
        ex_sss_main_linux_unconf();
#endif // defined(__linux__) && defined(T1oI2C) && SSS_HAVE_APPLET_SE05X_IOT
    }
    else {
        LOG_E("!ERROR! ret != 0.");
        ret = 1;
    }
    return ret;
}