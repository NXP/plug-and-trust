/*
 * Copyright 2025-2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "se051h_nfc_comm_prov.h"
#include <stdio.h>
#include <string.h>

#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

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
  printf(" --rawdata <raw_text_file>    ==> Raw data (hex bytes) to be "
         "provisioned in T4T applet."
         " Ensure to pass a valid ndef header also\n");
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
  printf(" Note: It is mandatory to pass at-least one network interface.\n");
  printf(" --ec_key_session_key         ==> Provision Key for EC Key applet "
         "session. \n");
  printf(" --user_id_session_key        ==> Provision Key for User Id applet "
         "session. \n");
  printf(" --aes_key_session_key        ==> Provision key for AES key applet "
         "session. \n");
  printf(" --provision_with_policy      ==> Provision objects with policy. \n");
  printf(" --dac_key                    ==> DA key pair file to provision. \n");
  printf(
      " --dac_cert                   ==> DA certificate file to provision. \n");
  printf(" --provision_verifiers        ==> Provision dynamic verifiers and "
         "passcode. \n");
  printf(" --delete_key <HEX_KEYID>     ==> Delete key with specified hex key "
         "ID (e.g., 0x7FFF3002). \n");
  printf(" --do_readidlist              ==> Read ID list from SE05x. \n");
  printf(" --t4t_enable_read            ==> Enable Contact less Read. (Not to "
         "be used with PCSC interface) \n");
  printf(" --t4t_disable_read           ==> Disable Contact less Read. (Not to "
         "be used with PCSC interface) \n");
  printf(" --t4t_enable_write           ==> Enable Contact less Write. (Not to "
         "be used with PCSC interface) \n");
  printf(" --t4t_disable_write          ==> Disable Contact less Write. (Not "
         "to be used with PCSC interface) \n");
  printf(" --doreset-cryptoobjects      ==> Delete all crypto objects. (Any "
         "other inputs to example will be ignored.) \n");

  return;
}

static int get_dac_key_from_file(char *filename, uint8_t *dac_key,
                                 size_t *dac_key_len) {
  FILE *fp = NULL;
  EVP_PKEY *pkey = NULL;
  EC_KEY *ec_key = NULL;
  int ret = -1;

  fp = fopen(filename, "rb");
  if (fp == NULL) {
    printf("Failed to open file: %s\n", filename);
    return -1;
  }

  // Try PEM format first
  pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
  if (pkey == NULL) {
    // Try DER format
    if (fseek(fp, 0, SEEK_SET) != 0) {
      fclose(fp);
      return -1;
    }
    pkey = d2i_PrivateKey_fp(fp, NULL);
  }

  if (fclose(fp) != 0) {
    return -1;
  }

  if (pkey == NULL) {
    printf("Failed to read key from file\n");
    return -1;
  }

  // Extract EC key and convert to DER format
  ec_key = EVP_PKEY_get1_EC_KEY(pkey);
  if (ec_key != NULL) {
    int der_len = i2d_ECPrivateKey(ec_key, NULL);
    if (der_len > 0 && (size_t)der_len <= *dac_key_len) {
      uint8_t *p = dac_key;
      i2d_ECPrivateKey(ec_key, &p);
      *dac_key_len = der_len;
      ret = 0;
      printf("Successfully read NIST P-256 key (length: %d bytes)\n", der_len);
    } else {
      printf("Buffer too small or invalid key\n");
    }
    EC_KEY_free(ec_key);
  }

  EVP_PKEY_free(pkey);
  return ret;
}

static int get_dac_cert_from_file(char *filename, uint8_t *dac_cert,
                                  size_t *dac_cert_len) {
  FILE *fp = NULL;
  X509 *cert = NULL;
  int ret = -1;

  fp = fopen(filename, "rb");
  if (fp == NULL) {
    printf("Failed to open file: %s\n", filename);
    return -1;
  }

  // Try PEM format first
  cert = PEM_read_X509(fp, NULL, NULL, NULL);
  if (cert == NULL) {
    // Try DER format
    if (fseek(fp, 0, SEEK_SET) != 0) {
      fclose(fp);
      return -1;
    }
    cert = d2i_X509_fp(fp, NULL);
  }

  if (fclose(fp) != 0) {
    return -1;
  }

  if (cert == NULL) {
    printf("Failed to read certificate from file\n");
    return -1;
  }

  // Convert certificate to DER format
  int der_len = i2d_X509(cert, NULL);
  if (der_len > 0 && (size_t)der_len <= *dac_cert_len) {
    uint8_t *p = dac_cert;
    i2d_X509(cert, &p);
    *dac_cert_len = der_len;
    ret = 0;
    printf("Successfully read certificate (length: %d bytes)\n", der_len);
  } else {
    printf("Buffer too small or invalid certificate (required: %d, available: "
           "%zu)\n",
           der_len, *dac_cert_len);
  }

  X509_free(cert);
  return ret;
}

int main(int argc, char *argv[]) {
  uint8_t do_reset = 0;
  uint8_t do_ec_key_provision = 0;
  uint8_t do_user_id_provision = 0;
  uint8_t do_aes_key_provision = 0;
  uint8_t only_t4t_provision = 0;
  uint8_t device_network_type = invalidNetworkInterface;
  uint8_t qrcode[1024] = {0};
  uint8_t *qrcode_ptr = NULL;
  size_t qrcodeLen = 0;
  size_t is_qr_code = 0;
  uint8_t tp_spake_passcode_set_no = 1;
  uint32_t tp_spake_itter_to_be_used = 1000;
  uint8_t provision_with_policy = 0;
  uint8_t provision_verifiers = 0;
  uint8_t se05x_t4t_access_ctrl_option = se05x_t4t_ac_invalid;
  uint8_t doresetcryproobjects = 0;

  uint8_t dac_key[256] = {0};
  uint8_t *dac_key_ptr = NULL;
  size_t dac_key_len = 0;

  uint8_t dac_cert[1024] = {0};
  uint8_t *dac_cert_ptr = NULL;
  size_t dac_cert_len = 0;

  uint8_t do_delete_key = 0;
  uint32_t delete_keyid = 0;

  uint8_t do_readidlist = 0;

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

      if (qrcodeLen >= sizeof(qrcode)) {
        return 0;
      }
      memcpy(qrcode, argv[i], strlen(argv[i]));
      qrcode_ptr = &qrcode[0];
      is_qr_code = 1;
    } else if (strcmp(argv[i], "--rawdata") == 0) {
      if (argc <= i + 1) {
        printf("No raw file name passed\n");
        return 0;
      }
      i++;
      FILE *fp = fopen(argv[i], "r");
      if (fp == NULL) {
        printf("Failed to open file\n");
        return 0;
      }
      char hexstr[1024] = {0};
      if (fgets(hexstr, sizeof(hexstr), fp) == NULL) {
        if (fclose(fp) != 0) {
          return -1;
        }
        return 0;
      }
      if (fclose(fp) != 0) {
        return -1;
      }
      char *p = hexstr;
      qrcodeLen = 0;
      while (*p && *(p + 1) && qrcodeLen < sizeof(qrcode)) {
        unsigned int byte;
        if (sscanf(p, "%02x", &byte) == 1) {
          qrcode[qrcodeLen++] = (uint8_t)byte;
        }
        p += 2;
      }
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
    } else if (strcmp(argv[i], "--dac_key") == 0) {
      if (argc <= i + 1) {
        printf("No DAC key file passed \n");
        return 0;
      }
      i++;
      char *filename = argv[i];
      dac_key_len = sizeof(dac_key);
      if (get_dac_key_from_file(filename, dac_key, &dac_key_len) == -1) {
        printf("Error in reading DAC key file. The example will provision "
               "default keys to SE051H ");
        dac_key_len = 0;
      } else {
        dac_key_ptr = dac_key;
      }
    } else if (strcmp(argv[i], "--dac_cert") == 0) {
      if (argc <= i + 1) {
        printf("No DAC certificate file passed \n");
        return 0;
      }
      i++;
      char *filename = argv[i];
      dac_cert_len = sizeof(dac_cert);

      dac_cert_len = dac_cert_len - 4;

      if (get_dac_cert_from_file(filename, &dac_cert[4], &dac_cert_len) == -1) {
        printf("Error in reading DAC certificate file. The example will "
               "provision default certificate to SE051H\n");
        dac_cert_len = 0;
      } else {
        if (dac_cert_len > sizeof(dac_cert) - 4) {
          return 0;
        }
        dac_cert[0] = 0x31;
        dac_cert[1] = 0x00;
        dac_cert[2] = static_cast<uint8_t>(dac_cert_len & 0xFF);
        dac_cert[3] = static_cast<uint8_t>(dac_cert_len >> 8);
        dac_cert_len = dac_cert_len + 4;
        dac_cert_ptr = dac_cert;
      }
    } else if (strcmp(argv[i], "--provision_verifiers") == 0) {
      provision_verifiers = 1;
    } else if (strcmp(argv[i], "--delete_key") == 0) {
      if (argc <= i + 1) {
        printf("No key ID passed \n");
        return 0;
      }
      i++;
      char *endptr;
      unsigned long tmp = strtoul(argv[i], &endptr, 0);
      if (*endptr != '\0' || tmp > UINT32_MAX) {
        printf("Invalid key ID format. Use hex format (e.g., 0x7FFF3002)\n");
        return -1;
      }
      delete_keyid = (uint32_t)tmp;
      do_delete_key = 1;
    } else if (strcmp(argv[i], "--do_readidlist") == 0) {
      do_readidlist = 1;
    } else if (strcmp(argv[i], "--t4t_enable_read") == 0) {
      se05x_t4t_access_ctrl_option = se05x_t4t_ac_enable_read;
    } else if (strcmp(argv[i], "--t4t_disable_read") == 0) {
      se05x_t4t_access_ctrl_option = se05x_t4t_ac_disable_read;
    } else if (strcmp(argv[i], "--t4t_enable_write") == 0) {
      se05x_t4t_access_ctrl_option = se05x_t4t_ac_enable_write;
    } else if (strcmp(argv[i], "--t4t_disable_write") == 0) {
      se05x_t4t_access_ctrl_option = se05x_t4t_ac_disable_write;
    } else if (strcmp(argv[i], "--doreset-cryptoobjects") == 0) {
      doresetcryproobjects = 1;
    } else {
      print_help();
      return 0;
    }
  }

  if (do_reset == 0 && do_ec_key_provision == 0 && do_user_id_provision == 0 &&
      do_aes_key_provision == 0 && only_t4t_provision == 0 &&
      do_delete_key == 0 && do_readidlist == 0 &&
      se05x_t4t_access_ctrl_option == 0 &&
      doresetcryproobjects == 0) {
    if (device_network_type == invalidNetworkInterface) {
      printf(
          "Specify at-least one network interface type (--wifi_net_interface "
          "or --thread_net_interface or --ethernet_net_interface) ");
      print_help();
      return 0;
    }
  }

  se051h_nfc_comm_prov(
      NULL, do_reset, only_t4t_provision, qrcode_ptr, qrcodeLen, is_qr_code,
      device_network_type, tp_spake_passcode_set_no, tp_spake_itter_to_be_used,
      do_ec_key_provision, do_aes_key_provision, do_user_id_provision,
      provision_with_policy, dac_key_ptr, dac_key_len, dac_cert_ptr,
      dac_cert_len, provision_verifiers, do_delete_key, delete_keyid,
      do_readidlist, se05x_t4t_access_ctrl_option, doresetcryproobjects);

  return 0;
}