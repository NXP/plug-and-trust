/*
 *
 * Copyright 2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* ************************************************************************** */
/* Includes                                                                   */
/* ************************************************************************** */

#include <ex_sss.h>
#include <ex_sss_boot.h>
#include <fsl_sss_se05x_apis.h>
#include <nxEnsure.h>
#include <nxLog_App.h>
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

#include "se051h_nfc_comm_prov.h"

static void print_help() {
  printf("\n The tool is used to provision SE051H for NFC commissioning \n");
  printf(" Usage - ./se051h_nfc_comm_prov [OPTIONS] \n");
  printf(" Following are the OPTIONS supported. \n");
  printf(" --help                       ==>    Display this message. \n");
  printf(" --doreset                    ==> Delete all provisioned data (QR "
         "code is not deleted). \n");
  printf(" --only_t4t_provision         ==> Provision only QR code in T4T "
         "applet. \n");
  printf(" --qrcode <QR_CODE_VALUE>     ==> QR code to provisioned in T4T "
         "applet. \n");
  printf(" --tp_spake_passcode_set_no   ==> Trust Provisioned pass-code set to "
         "be used (Possible values 1,2,3). \n");
  printf(" --tp_spake_itter_to_be_used  ==> Trust Provisioned iteration count "
         "to be used (Possible values 1000,5000,10000, 50000, 100000) \n");
  printf(" --wifi_net_interface         ==> Enable only Wi-Fi network "
         "interface for NFC commissioning. \n");
  printf(" --thread_net_interface       ==> Enable only Thread network "
         "interface for NFC commissioning. \n");
  printf(" --ethernet_net_interface     ==> Enable only Ethernet network "
         "interface for NFC commissioning. \n");
  printf(" Note: It is mandatory to pass at-least one network interface.");
  printf(" --ec_key_session_key         ==> Provision Key for EC Key applet "
         "session. \n");
  printf(" --user_id_session_key        ==> Provision Key for User Id applet "
         "session. \n");
  printf(" --aes_key_session_key        ==> Provision key for AES key applet "
         "session. \n");
  printf(" --provision_with_policy      ==> Provision objects with policy. \n");

  return;
}

void se051h_nfc_comm_prov(ex_sss_boot_ctx_t *pCtx, uint8_t do_reset,
                          uint8_t only_t4t_provision, uint8_t *qrcode,
                          size_t qrcodeLen, uint8_t device_network_type,
                          uint8_t tp_spake_passcode_set_no,
                          uint32_t tp_spake_itter_to_be_used,
                          uint8_t do_ec_key_provision,
                          uint8_t do_aes_key_provision,
                          uint8_t do_user_id_provision,
                          uint8_t provision_with_policy,
                          uint8_t *dac_key, size_t dac_key_len,
                          uint8_t *dac_cert, size_t dac_cert_len);

sss_status_t ex_sss_entry(ex_sss_boot_ctx_t *pCtx) {

  int argc = gex_sss_argc;
  const char **argv = gex_sss_argv;

  uint8_t do_reset = 0;
  uint8_t do_ec_key_provision = 0;
  uint8_t do_user_id_provision = 0;
  uint8_t do_aes_key_provision = 0;
  uint8_t only_t4t_provision = 0;
  uint8_t device_network_type = invalidNetworkInterface;
  uint8_t qrcode[1024] = {0};
  uint8_t *qrcode_ptr = NULL;
  size_t qrcodeLen = 0;
  uint8_t tp_spake_passcode_set_no = 1;
  uint32_t tp_spake_itter_to_be_used = 1000;
  uint8_t provision_with_policy = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0) {
      print_help();
      return 0;
    } else if (strcmp(argv[i], "--doreset") == 0) {
      do_reset = 1;
    } else if (strcmp(argv[i], "--only_t4t_provision") == 0) {
      only_t4t_provision = 1;
    } else if (strcmp(argv[i], "--qrcode") == 0) {
      if (argc <= i + 1) {
        printf("No QR code passed \n");
        return 0;
      }
      i++;
      qrcodeLen = strlen(argv[i]);
      if (qrcodeLen > sizeof(qrcode)) {
        printf("Invalid QR code length \n");
        return 0;
      }
      memcpy(qrcode, argv[i], strlen(argv[i]));
      qrcode_ptr = &qrcode[0];
    } else if (strcmp(argv[i], "--wifi_net_interface") == 0) {
      device_network_type |= wiFiNetworkInterface;
    } else if (strcmp(argv[i], "--thread_net_interface") == 0) {
      device_network_type |= threadNetworkInterface;
    } else if (strcmp(argv[i], "--ethernet_net_interface") == 0) {
      device_network_type |= ethernetNetworkInterface;
    } else if (strcmp(argv[i], "--tp_spake_passcode_set_no") == 0) {
      char *value;
      long tmp = 0;

      if (argc <= i + 1) {
        printf("No passcode set number passed \n");
        return 0;
      }
      i++;

      tmp = strtol(argv[i], &value, 10);
      if (tmp <= 0 || tmp > 3) {
        printf("Invalid passcode set number. Valid values are 1, 2, or 3.\n");
        return -1;
      }
      tp_spake_passcode_set_no = (uint8_t)tmp;
    } else if (strcmp(argv[i], "--tp_spake_itter_to_be_used") == 0) {
      char *value;
      long tmp = 0;

      if (argc <= i + 1) {
        printf("No itteration count passed \n");
        return 0;
      }
      i++;

      tmp = strtol(argv[i], &value, 10);
      if (tmp < 0 || tmp > UINT32_MAX) {
        printf("Invalid iteration count value.\n");
        return -1;
      }
      tp_spake_itter_to_be_used = (uint32_t)tmp;
      if (!(tp_spake_itter_to_be_used == 1000 ||
            tp_spake_itter_to_be_used == 5000 ||
            tp_spake_itter_to_be_used == 10000 ||
            tp_spake_itter_to_be_used == 50000 ||
            tp_spake_itter_to_be_used == 100000)) {
        printf("tp_spake_itter_to_be_used value can be only 1000, 5000, 10000, "
               "50000 or 100000 \n");
        return 0;
      }
    } else if (strcmp(argv[i], "--ec_key_session_key") == 0) {
      do_ec_key_provision = 1;
    } else if (strcmp(argv[i], "--user_id_session_key") == 0) {
      do_user_id_provision = 1;
    } else if (strcmp(argv[i], "--aes_key_session_key") == 0) {
      do_aes_key_provision = 1;
    } else if (strcmp(argv[i], "--provision_with_policy") == 0) {
      provision_with_policy = 1;
    } else {
      print_help();
      return 0;
    }
  }

  if (device_network_type == 0) {
    /* Enable WiFi network interface */
    device_network_type = wiFiNetworkInterface;
  }

  se051h_nfc_comm_prov(NULL, do_reset, only_t4t_provision, qrcode_ptr,
                       qrcodeLen, device_network_type, tp_spake_passcode_set_no,
                       tp_spake_itter_to_be_used, do_ec_key_provision,
                       do_aes_key_provision, do_user_id_provision, provision_with_policy,
                       NULL, 0, NULL, 0);

  return kStatus_SSS_Success;
}
