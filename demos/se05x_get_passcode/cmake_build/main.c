/*
 *
 * Copyright 2025-2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* ************************************************************************** */
/* Includes                                                                   */
/* ************************************************************************** */

#include <ex_sss.h>
#include <ex_sss_boot.h>
#include <nxEnsure.h>
#include <nxLog_App.h>
#include <se05x_get_passcode.h>
#include <stdio.h>
#include <string.h>

static ex_sss_boot_ctx_t gex_nfc_comm_prov_boot_ctx;

/* ************************************************************************** */
/* Static function declarations                                               */
/* ************************************************************************** */

/* ************************************************************************** */
/* Private Functions                                                          */
/* ************************************************************************** */

/* ************************************************************************** */
/* Public Functions                                                           */
/* ************************************************************************** */

#define EX_SSS_BOOT_PCONTEXT (&gex_nfc_comm_prov_boot_ctx)
#define EX_SSS_BOOT_DO_ERASE 0
#define EX_SSS_BOOT_EXPOSE_ARGC_ARGV 1

/* ************************************************************************** */
/* Include "main()" with the platform specific startup code for Plug & Trust  */
/* MW examples which will call ex_sss_entry()                                 */
/* ************************************************************************** */
#include <ex_sss_main_inc.h>

void print_help() {
  printf("\n The tool is used to get the passcode from SE051H. \n");
  printf("\n Usage - ./se05x_get_passcode [OPTIONS] \n");
  printf(" Following are the OPTIONS supported. \n");
  printf(" --help ==>    Display this message. \n");
  printf(" --tp_passcode_set_no <SET_NO> ==> Get the passcode of the set "
         "number specified. (Possible values = 1,2,3). \n");
  printf(" Note: If no passcode set number is passed, default passcode set "
         "number is 1. \n");

  printf("\n\"\"\"\nNOTE: SE051H has a binary file (Binary file-id = "
         "0X7fff2000) which contains 3 set of passcode and salt values. The "
         "format is shown below \n");
  printf("\n [4 byte passcode #1] [16 byte salt #1] \n [4 byte passcode #2] "
         "[16 byte salt #2] \n [4 byte passcode #3] [16 byte salt #3] \n\n");
  printf("Pre-calculated values of verifiers (w0 and L) for above passcode, "
         "salt and 5 different itterations (1000, 5000, 10000, 50000, 100000)\n"
         "are also trust provisioned in SE051H and can be used for spake "
         "connection with commissioner. \n\"\"\" \n");
  printf(" Example: --tp_passcode_set_no  <SET_NO> \"COM5 \"\n");
  return;
}

sss_status_t ex_sss_entry(ex_sss_boot_ctx_t *pCtx) {

  int argc = gex_sss_argc;
  const char **argv = gex_sss_argv;
  int parameter_error = 1;

  uint8_t passcode_set_no = 1;

  if ((argc >= 4)) {
    for (int i = 1; i < argc - 1; i++) {
      if (strcmp(argv[i], "--help") == 0) {
        print_help();
        return 0;
      } else if (strcmp(argv[i], "--tp_passcode_set_no") == 0) {
        char *value;
        long tmp = 0;

        if (argc <= i + 1) {
          printf("No pass-code set number passed \n");
          return 0;
        }

        i++;
        tmp = strtol(argv[i], &value, 10);
        if (tmp <= 0 || tmp > 3) {
          printf("Invalid passcode set number. Valid values are 1, 2, or 3.\n");
          return -1;
        }
        passcode_set_no = (uint8_t)tmp;
        parameter_error = 0;
      } else {
        parameter_error = 1;
        break;
      }
    }
  } else {
    parameter_error = 1;
  }

  if (parameter_error) {
    print_help();
    return 0;
  }
  se05x_get_passcode(pCtx, passcode_set_no);

  return kStatus_SSS_Success;
}
