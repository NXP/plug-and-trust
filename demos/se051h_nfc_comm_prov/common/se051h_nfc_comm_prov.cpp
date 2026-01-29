/*
 *
 * Copyright 2021,2025-2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* ************************************************************************** */
/* Includes                                                                   */
/* ************************************************************************** */

#include "se051h_nfc_comm_prov.h"
#include "ex_sss_objid.h"
#include "se05x_APDU.h"
#include "se05x_host_gpio.h"
#include <errno.h>
#include <ex_sss.h>
#include <ex_sss_boot.h>
#include <fsl_sss_se05x_apis.h>
#include <nxEnsure.h>
#include <nxLog_App.h>
#include <se05x_ecc_curves_values.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

static ex_sss_boot_ctx_t gex_sss_chip_ctx;

static uint8_t policy = 0;

static sss_status_t se051h_set_key(const uint8_t *buffer, size_t bufferLen,
                                   size_t bitLen, sss_key_part_t keyPart,
                                   sss_cipher_type_t cipherType, uint32_t keyId,
                                   void *options, size_t optionsLen) {
  sss_status_t status = kStatus_SSS_Success;
  sss_object_t keyObj;
  smStatus_t smstatus = SM_NOT_OK;
  Se05xPolicy_t se05x_policy;

  status = sss_key_object_init(&keyObj, &gex_sss_chip_ctx.ks);
  ENSURE_OR_RETURN_ON_ERROR(status == kStatus_SSS_Success, status);

  status =
      sss_key_object_allocate_handle(&keyObj, keyId, keyPart, cipherType,
                                     bufferLen, kKeyObject_Mode_Persistent);
  ENSURE_OR_RETURN_ON_ERROR(status == kStatus_SSS_Success, status);

  if (cipherType == kSSS_CipherType_Binary ||
      cipherType == kSSS_CipherType_Certificate) {
    if (policy) {
        if (keyId == SE051H_WIFI_CRED_ID_APP_8_4 || keyId == SE051H_WIFI_CRED_ID_APP_8_8) {
            uint8_t policies_buff[MAX_POLICY_BUFFER_SIZE] = WIFI_POLICY_BUFF;
            se05x_policy.value = policies_buff;
            se05x_policy.value_len = WIFI_POLICY_BUF_LEN;

        } else {
            uint8_t policies_buff[MAX_POLICY_BUFFER_SIZE] = BINARY_POLICY_BUFF;
            se05x_policy.value = policies_buff;
            se05x_policy.value_len = POLICY_BUF_LEN;
        }
    }
    else {
        se05x_policy.value = NULL;
        se05x_policy.value_len = 0;
    }

    if (bufferLen > UINT16_MAX) {
        LOG_E("Buffer length exceeds maximum allowed size");
        return kStatus_SSS_Fail;
    }
    smstatus = Se05x_API_WriteBinary_Ver(
        &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx, &se05x_policy, keyId,
        0, (uint16_t)bufferLen, buffer, bufferLen, 0);
    if (smstatus != SM_OK) {
      LOG_E("Error in setting hmac key");
      status = kStatus_SSS_Fail;
    }
  } else if (cipherType == kSSS_CipherType_HMAC) {
    SE05x_KeyID_t kekID = SE05x_KeyID_KEK_NONE;
    sss_se05x_key_store_t *se05x_keyStore =
        (sss_se05x_key_store_t *)&gex_sss_chip_ctx.ks;
    SE05x_Result_t exists = kSE05x_Result_NA;

    if (se05x_keyStore->kekKey != NULL) {
      kekID = se05x_keyStore->kekKey->keyId;
    }

    if (bitLen % 8 == 0) {
      smstatus = Se05x_API_CheckObjectExists(
          &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx, keyId,
          &exists);
      if (smstatus == SM_OK) {
        if (exists == kSE05x_Result_SUCCESS) {
          LOG_I("spake2p_verifiers credentials already provisioned");
          return status;
        } else {
          smstatus = Se05x_API_WriteSymmKey_Ver(
              &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx, NULL,
              SE05x_MaxAttemps_NA, keyId, kekID, buffer, bufferLen,
              kSE05x_INS_NA, kSE05x_SymmKeyType_HMAC, 0);
          if (smstatus != SM_OK) {
            LOG_E("Error in setting hmac key");
            status = kStatus_SSS_Fail;
          }
        }
      } else {
        LOG_E("Se05x_API_CheckObjectExists Failed");
        status = kStatus_SSS_Fail;
      }
    } else {
      LOG_E("Error in setting hmac key");
      status = kStatus_SSS_Fail;
    }
  } else {
    if (!policy) {
        options = NULL;
        optionsLen = 0;
    }
    status = sss_key_store_set_key(&gex_sss_chip_ctx.ks, &keyObj, buffer,
                                   bufferLen, bitLen, options, optionsLen);
    if (status != kStatus_SSS_Success) {
      printf("Error in sss_key_store_set_key \n");
    }
  }

  return status;
}

static smStatus_t se05x_delete_key(uint32_t keyid) {

  smStatus_t smstatus = SM_NOT_OK;
  SE05x_Result_t exists = kSE05x_Result_NA;

  if (gex_sss_chip_ctx.ks.session != NULL) {
    smstatus = Se05x_API_CheckObjectExists(
        &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx, keyid,
        &exists);
    if (smstatus == SM_OK) {
      if (exists == kSE05x_Result_SUCCESS) {
        smstatus = Se05x_API_DeleteSecureObject(
            &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx, keyid);
        if (smstatus != SM_OK) {
          LOG_E("Error in deleting key");
        }
      } else {
        LOG_W("Key does not exists");
      }
    } else {
      LOG_E("Error in Se05x_API_CheckObjectExists");
    }
  }
  return smstatus;
}

#if 0
static sss_status_t se051h_provision_passcode_parameters()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;
    uint8_t passcode_buffer[] = {
        PASSCODE_PARAMS
    };
    SE05x_Result_t exists = kSE05x_Result_NA;

    smstatus = Se05x_API_CheckObjectExists(&((sss_se05x_session_t *) &gex_sss_chip_ctx.session)->s_ctx, SE051H_PASSCODE_ID, &exists);
    if (smstatus == SM_OK){
        if (exists == kSE05x_Result_SUCCESS) {
            LOG_I("Passcode credentials already provisioned");
            status = kStatus_SSS_Success;
            return status;
        }
        else {
            LOG_I("Writing Pass code Parameters to SE05x at Key id = %x",  SE051H_PASSCODE_ID);
            status = se051h_set_key(passcode_buffer, sizeof(passcode_buffer), sizeof(passcode_buffer)*8 , kSSS_KeyPart_Default, kSSS_CipherType_Binary, SE051H_PASSCODE_ID, NULL, 0);
            if(status != kStatus_SSS_Success){
                printf("Error in se051h_provision_passcode_parameters\n");
            }
        }
    }

    return status;
}
#endif

static sss_status_t
se051h_provision_pbkdf_parameters(uint8_t tp_spake_passcode_set_no,
                                  uint32_t tp_spake_itter_to_be_used) {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;
  uint8_t pbkdf_Buffer[] = {PBKDF_PARAMS};

  smstatus = se05x_delete_key(SE051H_PBKDF_PARAMS_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  if (tp_spake_passcode_set_no >= 1 && tp_spake_passcode_set_no <= 3) {
    pbkdf_Buffer[PASSCODE_SET_TO_BE_USED_OFFSET] = tp_spake_passcode_set_no;
  } else {
    printf(
        "Invalid pass-code set number. Using the default set number of 0x01 \n");
  }

  if (tp_spake_itter_to_be_used == 1000) {
    pbkdf_Buffer[PASSCODE_SET_TO_BE_USED_OFFSET + 1] = 0x01;
  } else if (tp_spake_itter_to_be_used == 5000) {
    pbkdf_Buffer[PASSCODE_SET_TO_BE_USED_OFFSET + 1] = 0x02;
  } else if (tp_spake_itter_to_be_used == 10000) {
    pbkdf_Buffer[PASSCODE_SET_TO_BE_USED_OFFSET + 1] = 0x03;
  } else if (tp_spake_itter_to_be_used == 50000) {
    pbkdf_Buffer[PASSCODE_SET_TO_BE_USED_OFFSET + 1] = 0x04;
  } else if (tp_spake_itter_to_be_used == 100000) {
    pbkdf_Buffer[PASSCODE_SET_TO_BE_USED_OFFSET + 1] = 0x05;
  } else {
    printf("Invalid iteration count. Using the default value of 0x01 in "
           "buffer - which corresponds to 1000 iterations \n");
  }

  LOG_I("Writing PBKDF Parameters to SE05x at Key id = %x",
        SE051H_PBKDF_PARAMS_ID);
  status =
      se051h_set_key(pbkdf_Buffer, sizeof(pbkdf_Buffer),
                     sizeof(pbkdf_Buffer) * 8, kSSS_KeyPart_Default,
                     kSSS_CipherType_Binary, SE051H_PBKDF_PARAMS_ID, NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_provision_pbkdf_parameters\n");
  }

  return status;
}

#if 0
static sss_status_t se051h_provision_spake2p_verifiers()
{
    sss_status_t status = kStatus_SSS_Fail;
    uint8_t hmac_key_w0[] = {
        HMAC_KEY_W0
    };
    uint8_t hmac_key_L[] = {
        HMAC_KEY_L
    };

    /*Spake2+ verifier(w0)*/
    LOG_I("Writing HMAC Key(w0) to SE05x at Key id = %x",  SE051H_HMAC_KEY_W0_ID);
    status = se051h_set_key(hmac_key_w0, sizeof(hmac_key_w0), sizeof(hmac_key_w0)*8, kSSS_KeyPart_Default, kSSS_CipherType_HMAC, SE051H_HMAC_KEY_W0_ID, NULL, 0);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    /*Spake2+ verifier(L)*/
    LOG_I("Writing HMAC Key(L) to SE05x at Key id = %x",  SE051H_HMAC_KEY_L_ID);
    status = se051h_set_key(hmac_key_L, sizeof(hmac_key_L), sizeof(hmac_key_L)*8, kSSS_KeyPart_Default, kSSS_CipherType_HMAC, SE051H_HMAC_KEY_L_ID, NULL, 0);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

cleanup:
    return status;
}
#endif

static sss_status_t se051h_provision_dac_cert() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t dac_cer[] = {DAC_CERTIFICATE};

  smstatus = se05x_delete_key(SE051H_DAC_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  /*set Device Attestation certificate*/
  LOG_I("Writing Device Attestation Certificate to SE05x at Key id = %x",
        SE051H_DAC_ID);
  status = se051h_set_key(dac_cer, sizeof(dac_cer), sizeof(dac_cer) * 8,
                          kSSS_KeyPart_Default, kSSS_CipherType_Certificate,
                          SE051H_DAC_ID, NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_set_dev_attest_cert\n");
  }

  return status;
}

static sss_status_t se051h_provision_pai_cert() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t pai_cer[] = {PAI_CERTIFICATE};

  smstatus = se05x_delete_key(SE051H_PAI_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  /*set Device Attestation certificate*/
  LOG_I("Writing PAI certificate to SE05x at Key id = %x", SE051H_PAI_ID);
  status = se051h_set_key(pai_cer, sizeof(pai_cer), sizeof(pai_cer) * 8,
                          kSSS_KeyPart_Default, kSSS_CipherType_Certificate,
                          SE051H_PAI_ID, NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_set_pai_cert\n");
  }

  return status;
}

static sss_status_t se051h_provision_da_key() {

  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t privKey[] = {DA_KEY_PAIR_DATA};
  sss_policy_t policy_for_DA_key;

  if (policy) {
      static sss_policy_u assym_pol;
      assym_pol.type = KPolicy_Asym_Key;
      assym_pol.policy.asymmkey.can_Sign = 1;

      static sss_policy_u commonPol;
      commonPol.type = KPolicy_Common;
      commonPol.policy.common.can_Delete = 1;
      commonPol.policy.common.can_Read = 1;

      static sss_policy_u signPol;
      signPol.type = KPolicy_Internal_Sign;
      signPol.policy.tbsItemList.tbsItemList_KeyId=0x7FFF2031;

      policy_for_DA_key.nPolicies = 3;
      policy_for_DA_key.policies[0] = &assym_pol;
      policy_for_DA_key.policies[1] = &commonPol;
      policy_for_DA_key.policies[2] = &signPol;
  }

  smstatus = se05x_delete_key(SE051H_DA_KEY_PAIR_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);


  LOG_I("Writing DA private key to SE05x at Key id = %x", SE051H_DA_KEY_PAIR_ID);
  status = se051h_set_key(privKey, sizeof(privKey), 256, kSSS_KeyPart_Pair,
                          kSSS_CipherType_EC_NIST_P, SE051H_DA_KEY_PAIR_ID,
                          &policy_for_DA_key, sizeof(policy_for_DA_key));
  if (status != kStatus_SSS_Success) {
    printf("Error is set DA private key\n");
  }
  return status;
}

static sss_status_t se051h_provision_attest_tbs() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t attest_tbs[] = {STRUCTURE_START,   CERTIFICATE_DECLARATION,
                          ATTESTATION_NONCE, TIMESTAMP,
                          STRUCTURE_END,     ATTESTATION_CHALLENGE};

  smstatus = se05x_delete_key(SE051H_ATTEST_TBS);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("Writing Attestation TBS at Key id = %x", SE051H_ATTEST_TBS);
  status = se051h_set_key(attest_tbs, sizeof(attest_tbs),
                          sizeof(attest_tbs) * 8, kSSS_KeyPart_Default,
                          kSSS_CipherType_Binary, SE051H_ATTEST_TBS, NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in writing se051h_provision_attest_tbs data\n");
  }

  return status;
}

static sss_status_t se051h_provision_select_response() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t select_response[] = {SE051H_SELECT_RESPONSE};

  smstatus = se05x_delete_key(SE051H_SELECT_RESPONSE_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("Writing select response data at Key id = %x",
        SE051H_SELECT_RESPONSE_ID);
  status = se051h_set_key(select_response, sizeof(select_response),
                          sizeof(select_response) * 8, kSSS_KeyPart_Default,
                          kSSS_CipherType_Binary, SE051H_SELECT_RESPONSE_ID,
                          NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_provision_select_response\n");
  }

  return status;
}

static sss_status_t se051h_provision_node_oper_key() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  sss_policy_t policy_for_NO_key;

  if (policy) {
      static sss_policy_u assym_pol;
      assym_pol.type = KPolicy_Asym_Key;
      assym_pol.policy.asymmkey.can_Sign = 1;
      assym_pol.policy.asymmkey.can_Verify = 1;
      assym_pol.policy.asymmkey.can_Gen = 1;

      static sss_policy_u commonPol;
      commonPol.type = KPolicy_Common;
      commonPol.policy.common.req_Sm = 1;
      commonPol.policy.common.can_Read = 1;
      commonPol.policy.common.can_Delete = 1;

      policy_for_NO_key.nPolicies = 2;
      policy_for_NO_key.policies[0] = &assym_pol;
      policy_for_NO_key.policies[1] = &commonPol;
  }


  uint8_t no_key[] = {NODE_OP_KEY_PAIR_DATA};

  smstatus = se05x_delete_key(SE051H_NODE_OP_KEY_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("Writing Node Operational key to SE05x at Key id = %x",
        SE051H_NODE_OP_KEY_ID);
  status =
      se051h_set_key(no_key, sizeof(no_key), 256, kSSS_KeyPart_Pair,
                     kSSS_CipherType_EC_NIST_P, SE051H_NODE_OP_KEY_ID, &policy_for_NO_key, sizeof(policy_for_NO_key));
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_provision_node_oper_key\n");
  }

  return status;
}

static sss_status_t se051h_provision_node_oper_cert() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t noc[] = {NOC};

  smstatus = se05x_delete_key(SE051H_NOC_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  /* set NOC */
  LOG_I("Writing Node Operational certificate to SE05x at Key id = %x",
        SE051H_NOC_ID);
  status =
      se051h_set_key(noc, sizeof(noc), sizeof(noc) * 8, kSSS_KeyPart_Default,
                     kSSS_CipherType_Certificate, SE051H_NOC_ID, NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_provision_node_oper_cert\n");
  }

  return status;
}

static sss_status_t se051h_provision_root_cert() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t root_ca[] = {ROOT_CERTIFICATE};

  smstatus = se05x_delete_key(SE051H_ROOT_CER_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  /* set Root certificate */
  LOG_I("Writing Root certificate to SE05x at Key id = %x", SE051H_ROOT_CER_ID);
  status = se051h_set_key(root_ca, sizeof(root_ca), sizeof(root_ca) * 8,
                          kSSS_KeyPart_Default, kSSS_CipherType_Certificate,
                          SE051H_ROOT_CER_ID, NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_provision_root_cert\n");
  }

  return status;
}

static sss_status_t se051h_provision_ssid_passcode() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t wifi_cred_data[] = {WIFI_CRED_DATA};

  smstatus = se05x_delete_key(SE051H_WIFI_CRED_ID_APP_8_4);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("Writing WI-FI credentials to SE05x at Key id = %x",
        SE051H_WIFI_CRED_ID_APP_8_4);
  status = se051h_set_key(wifi_cred_data, sizeof(wifi_cred_data),
                          sizeof(wifi_cred_data) * 8, kSSS_KeyPart_Default,
                          kSSS_CipherType_Binary, SE051H_WIFI_CRED_ID_APP_8_4,
                          NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_provision_ssid_passcode\n");
  }

  return status;
}

static sss_status_t se051h_provision_ssid_passcode_app_8_8() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t wifi_cred_data[] = {WIFI_CRED_DATA};

  smstatus = se05x_delete_key(SE051H_WIFI_CRED_ID_APP_8_8);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("Writing WI-FI credentials to SE05x at Key id = %x",
        SE051H_WIFI_CRED_ID_APP_8_8);
  status = se051h_set_key(wifi_cred_data, sizeof(wifi_cred_data),
                          sizeof(wifi_cred_data) * 8, kSSS_KeyPart_Default,
                          kSSS_CipherType_Binary, SE051H_WIFI_CRED_ID_APP_8_8,
                          NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_provision_ssid_passcode_app_8_8\n");
  }

  return status;
}

static sss_status_t se051h_provision_acl() {

  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t acl_data[] = {ACL};

  smstatus = se05x_delete_key(SE051H_ACL_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("Writing acl to SE05x at Key id = %x", SE051H_ACL_ID);
  status = se051h_set_key(acl_data, sizeof(acl_data), sizeof(acl_data) * 8,
                          kSSS_KeyPart_Default, kSSS_CipherType_Binary,
                          SE051H_ACL_ID, NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_provision_acl\n");
  }

  return status;
}

static sss_status_t se051h_provision_identity_protection_key() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t ipk_data[] = {IPK};

  smstatus = se05x_delete_key(SE051H_IPK_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("Writing ipk to SE05x at Key id = %x", SE051H_IPK_ID);
  status = se051h_set_key(ipk_data, sizeof(ipk_data), sizeof(ipk_data) * 8,
                          kSSS_KeyPart_Default, kSSS_CipherType_Binary,
                          SE051H_IPK_ID, NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_provision_identity_protection_key\n");
  }

  return status;
}

static sss_status_t se051h_provision_basic_info_cluster() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t Basic_info_cluster_data[] = {DATA_VERSION_BIC,
                                       CLUSTER_REVISION_BIC,
                                       FEATUREMAP_BIC,
                                       ATTRIBUTE_LIST_BIC,
                                       ACCEPTED_COMMAND_LIST_BIC,
                                       GENERATED_COMMAND_LIST_BIC,
                                       DATA_MODEL_REVISION,
                                       VENDOR_NAME,
                                       VENDOR_NAME_FILLER,
                                       VENDOR_ID,
                                       PRODUCT_NAME,
                                       PRODUCT_NAME_FILLER,
                                       PRODUCT_ID,
                                       NODE_LABEL,
                                       NODE_LABEL_FILLER,
                                       LOCATION,
                                       HARDWARE_VERSION,
                                       HARDWARE_VERSIONING,
                                       HARDWARE_VERSIONING_FILLER,
                                       SOFTWARE_VERSION,
                                       SOFTWARE_VERSIONING,
                                       SOFTWARE_VERSIONING_FILLER,
                                       UNIQUE_ID,
                                       UNIQUE_ID_FILLER,
                                       CAPABILITY_MINIMA,
                                       SPECIFICATION_VERSION,
                                       MAX_PATH_PER_INVOKE,
                                       CONFIGURATION_VERSION};

  smstatus = se05x_delete_key(SE051H_BASIC_INFO_CLUSTER_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("Writing Basic Information cluster data to SE05x at Key id = %x",
        SE051H_BASIC_INFO_CLUSTER_ID);
  status = se051h_set_key(
      Basic_info_cluster_data, sizeof(Basic_info_cluster_data),
      sizeof(Basic_info_cluster_data) * 8, kSSS_KeyPart_Default,
      kSSS_CipherType_Binary, SE051H_BASIC_INFO_CLUSTER_ID, NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_set_basic_info_cluster_data\n");
  }

  return status;
}

static sss_status_t se051h_provision_general_comm_cluster() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t Genaral_comm_cluster_data[] = {
      DATA_VERSION_GCC,
      CLUSTER_REVISION_GCC,
      FEATUREMAP_GCC,
      ATTRIBUTE_LIST_GCC,
      ACCEPTED_COMMAND_LIST_GCC,
      GENERATED_COMMAND_LIST_GCC,
      BREADCRUMB,
      BASIC_COMMISSIONING_INFO,
      REGULATORY_CONFIG,
      LOCATION_CAPABILITY,
      SUPPORTS_CONCURRENT_CONNECTION,
      TC_ACCEPTED_VERSION,
      TC_MIN_REQUIRED_VERSION,
      TC_ACKNOWLEDGEMENTS,
      TC_ACKNOWLEDGEMENTS_REQUIRED,
      TC_UPDATE_DEADLINE,
      IS_COMM_WITHOUT_POWER,
  };

  smstatus = se05x_delete_key(SE051H_GENERAL_COMM_CLUSTER_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("Writing General Commissioning cluster data to SE05x at Key id = %x",
        SE051H_GENERAL_COMM_CLUSTER_ID);
  status = se051h_set_key(
      Genaral_comm_cluster_data, sizeof(Genaral_comm_cluster_data),
      sizeof(Genaral_comm_cluster_data) * 8, kSSS_KeyPart_Default,
      kSSS_CipherType_Binary, SE051H_GENERAL_COMM_CLUSTER_ID, NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_set_Genaral_comm_cluster_data\n");
  }

  return status;
}

static sss_status_t se051h_provision_operational_cred_cluster() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t OCC_cluster_data[] = {OCC
#if 0
        DATA_VERSION_OCC,
        CLUSTER_REVISION_OCC,
        FEATUREMAP_OCC,
        ATTRIBUTE_LIST_OCC,
        ACCEPTED_COMMAND_LIST_OCC,
        GENERATED_COMMAND_LIST_OCC,
        NOCS,
        FABRICS,
        FABRICS_FILLER,
        SUPPORTED_FABRICS,
        COMMISSIONED_FABRICS,
        TRUSTED_ROOT_CERTIFICATES,
        CURRENT_FABRIC_INDEX
#endif
  };

  smstatus = se05x_delete_key(SE051H_OP_CRED_CLUSTER_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("Writing Operational credentials data to SE05x at Key id = %x",
        SE051H_OP_CRED_CLUSTER_ID);
  status = se051h_set_key(OCC_cluster_data, sizeof(OCC_cluster_data),
                          sizeof(OCC_cluster_data) * 8, kSSS_KeyPart_Default,
                          kSSS_CipherType_Binary, SE051H_OP_CRED_CLUSTER_ID,
                          NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_set_OCC_cluster_data\n");
  }

  return status;
}

static sss_status_t se051h_provision_access_control_cluster() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t acl_data[] = {
      DATA_VERSION_ACC,
      CLUSTER_REVISION_ACC,
      FEATUREMAP_ACC,
      ATTRIBUTE_LIST_ACC,
      ACCEPTED_COMMAND_LIST_ACC,
      GENERATED_COMMAND_LIST_ACC,
      ACLS,
      EXTENSION,
      EXTENSION_FILLER,
      SUBJECTS_PER_ACCESS_CONTROL_ENTRY,
      TARGETS_PER_ACCESS_CONTROL_ENTRY,
      ACCESS_CONTROL_ENTRIES_PER_FABRIC,
      COMMISSIONING_ARL,
      COMMISSIONING_ARL_FILLER,
      ARL,
      ARL_FILLER,
  };

  smstatus = se05x_delete_key(SE051H_ACC_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("Writing ACL data to SE05x at Key id = %x", SE051H_ACC_ID);
  status = se051h_set_key(acl_data, sizeof(acl_data), sizeof(acl_data) * 8,
                          kSSS_KeyPart_Default, kSSS_CipherType_Binary,
                          SE051H_ACC_ID, NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_set_acl_data\n");
  }

  return status;
}

static sss_status_t
se051h_provision_network_comm_cluster(uint8_t device_network_type) {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t ncc_buf[] = {
      DATA_VERSION_NCC,
      CLUSTER_REVISION_NCC,
      FEATUREMAP_NCC,
      ATTRIBUTE_LIST_NCC,
      ACCEPTED_COMMAND_LIST_NCC,
      GENERATED_COMMAND_LIST_NCC,
      MAX_NETWORKS,
      NETWORKS,
      NETWORKS_FILLER,
      SCAN_MAX_TIME_SECONDS,
      CONNECT_MAX_TIME_SECONDS,
      INTERFACE_ENABLED,
      LAST_NETWORKING_STATUS,
      LAST_NETWORK_ID,
      LAST_NETWORK_ID_FILLER,
      LAST_CONNECT_ERROR_VALUE_NCC,
      LAST_CONNECT_ERROR_VALUE_FILLER_NCC,
      SUPPORTED_WIFI_BANDS_NCC,
      SUPPORTED_THREAD_FEATURES_NCC,
      THREAD_VERSION_NCC,
  };

  ncc_buf[NETWORK_INTERFACE_BTYE_OFFSET] = device_network_type;

  smstatus = se05x_delete_key(SE051H_NCC_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("Writing ncc data to SE05x at Key id = %x", SE051H_NCC_ID);
  status = se051h_set_key(ncc_buf, sizeof(ncc_buf), sizeof(ncc_buf) * 8,
                          kSSS_KeyPart_Default, kSSS_CipherType_Binary,
                          SE051H_NCC_ID, NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_set_ncc_data\n");
  }

  return status;
}

static sss_status_t se051h_provision_vendor_reserved() {
  sss_status_t status = kStatus_SSS_Fail;
  smStatus_t smstatus = SM_NOT_OK;

  uint8_t vendor_reserved[] = {VENDOR_RESERVED};

  smstatus = se05x_delete_key(SE051H_VR_ID);
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("Writing vendor reserved data to SE05x at Key id = %x", SE051H_VR_ID);
  status = se051h_set_key(vendor_reserved, sizeof(vendor_reserved),
                          sizeof(vendor_reserved) * 8, kSSS_KeyPart_Default,
                          kSSS_CipherType_Binary, SE051H_VR_ID, NULL, 0);
  if (status != kStatus_SSS_Success) {
    printf("Error in se051h_provision_vendor_reserved\n");
  }

  return status;
}

#if SSS_HAVE_APPLET_SE051_H
static sss_status_t se051h_provision_t4t_applet(uint8_t *qrcode,
                                                size_t qrcodeLen) {
  smStatus_t smStatus = SM_NOT_OK;
  uint8_t ndeffileId[2] = NDEF_FILE_ID;
  size_t ndeffileIdLen = sizeof(ndeffileId);
  uint8_t ndefHeader[] = NDEF_HEADER;
  size_t ndefHeaderLen = sizeof(ndefHeader);
  uint8_t ndefData[128] = {0};
  size_t ndefDataLen = sizeof(ndefData);

  ENSURE_OR_RETURN_ON_ERROR(
      qrcodeLen <= (sizeof(ndefData) - sizeof(ndefHeader)), kStatus_SSS_Fail);

  ndefHeader[1] = (uint8_t)qrcodeLen + 5 /* Remaining buffer in NDEF_HEADER */;
  ndefHeader[4] = (uint8_t)qrcodeLen + 1 /* Remaining buffer in NDEF_HEADER */;

  memcpy(ndefData, ndefHeader, ndefHeaderLen);
  memcpy(ndefData + ndefHeaderLen, qrcode, qrcodeLen);

  ndefDataLen = ndefHeaderLen + qrcodeLen;

  smStatus = Se05x_T4T_API_SelectT4TApplet(
      &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx);
  ENSURE_OR_RETURN_ON_ERROR(smStatus == SM_OK, kStatus_SSS_Fail);

  smStatus = Se05x_T4T_API_ConfigureAccessCtrl(
      &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx,
      kSE05x_T4T_Interface_Contactless, kSE05x_T4T_Operation_Write,
      kSE05x_T4T_AccessCtrl_Granted);
  ENSURE_OR_RETURN_ON_ERROR(smStatus == SM_OK, kStatus_SSS_Fail);

  smStatus = Se05x_T4T_API_SelectFile(
      &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx, ndeffileId,
      ndeffileIdLen);
  ENSURE_OR_RETURN_ON_ERROR(smStatus == SM_OK, kStatus_SSS_Fail);

  smStatus = Se05x_T4T_API_UpdateBinary(
      &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx, ndefData,
      ndefDataLen);
  ENSURE_OR_RETURN_ON_ERROR(smStatus == SM_OK, kStatus_SSS_Fail);

  smStatus = Se05x_T4T_API_ConfigureAccessCtrl(
      &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx,
      kSE05x_T4T_Interface_Contactless, kSE05x_T4T_Operation_Write,
      kSE05x_T4T_AccessCtrl_Granted);
  ENSURE_OR_RETURN_ON_ERROR(smStatus == SM_OK, kStatus_SSS_Fail);

  LOG_I("T4T Provision successful \n");
  return kStatus_SSS_Success;
}
#endif

static smStatus_t se051h_key_provision(const char *key_type, uint32_t object_id,
                                       const uint8_t *key_value,
                                       size_t key_value_len,
                                       uint32_t policy_flags) {
  smStatus_t status = SM_NOT_OK;
  SE05x_Result_t exists = kSE05x_Result_NA;
  uint8_t policy_buf[POLICY_BUF] = {0};
  Se05xPolicy_t policy_for_auth_obj = {0};

  policy_buf[0] = POLICY_BUF_POLICY_LEN;
  policy_buf[POLICY_BUF_POLICY_OFFSET] =
      (uint8_t)((policy_flags & 0xFF000000) >> 24);
  policy_buf[POLICY_BUF_POLICY_OFFSET + 1] =
      (uint8_t)((policy_flags & 0x00FF0000) >> 16);
  policy_buf[POLICY_BUF_POLICY_OFFSET + 2] =
      (uint8_t)((policy_flags & 0x0000FF00) >> 8);
  policy_buf[POLICY_BUF_POLICY_OFFSET + 3] =
      (uint8_t)((policy_flags & 0x000000FF));

  policy_for_auth_obj.value = policy_buf;
  policy_for_auth_obj.value_len = POLICY_BUF;

  status = Se05x_API_CheckObjectExists(
      &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx, object_id,
      &exists);
  if (status == SM_OK && exists == kSE05x_Result_SUCCESS) {
    LOG_W("Object 0x%08X already exists", object_id);
    return SM_OK;
  }

  if (status == SM_OK && exists == kSE05x_Result_FAILURE) {
    if (strcmp(key_type, "ECKEY") == 0) {
      status = Se05x_API_WriteECKey(
          &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx,
          &policy_for_auth_obj, SE05x_MaxAttemps_UNLIMITED, object_id,
          kSE05x_ECCurve_NIST_P256, NULL, 0, key_value, key_value_len,
          (SE05x_INS_t)kSE05x_AttestationType_AUTH, kSE05x_KeyPart_Public);
      if (status != SM_OK) {
        return status;
      }
    }
    if (strcmp(key_type, "AESKEY") == 0) {
      status = Se05x_API_WriteSymmKey(
          &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx,
          &policy_for_auth_obj, SE05x_MaxAttemps_UNLIMITED, object_id,
          SE05x_KeyID_KEK_NONE, key_value, key_value_len,
          (SE05x_INS_t)kSE05x_AttestationType_AUTH, kSE05x_SymmKeyType_AES);
      if (status != SM_OK) {
        return status;
      }
    }
    if (strcmp(key_type, "USERID") == 0) {
      status = Se05x_API_WriteUserID(
          &((sss_se05x_session_t *)&gex_sss_chip_ctx.session)->s_ctx,
          &policy_for_auth_obj, SE05x_MaxAttemps_UNLIMITED, object_id,
          key_value, key_value_len, kSE05x_AttestationType_AUTH);
      if (status != SM_OK) {
        return status;
      }
    }
  }
  return status;
}

static sss_status_t se051h_ec_key_provision() {
  smStatus_t status = SM_NOT_OK;
  const uint8_t ECKey_SE_PublicEcdsakey[] = ECKEY_SE_PUBLICECDSA_KEY;
  uint32_t read_delete_policy = POLICY_OBJ_ALLOW_DELETE | POLICY_OBJ_ALLOW_READ;
  status = se051h_key_provision(
      "ECKEY", kEX_SSS_objID_ECKEY_Auth, ECKey_SE_PublicEcdsakey,
      sizeof(ECKey_SE_PublicEcdsakey), read_delete_policy);
  ENSURE_OR_RETURN_ON_ERROR(status == SM_OK, kStatus_SSS_Fail);
  LOG_I("kEX_SSS_objID_ECKEY_Auth provision successful for Object ID 0x%08X\n",
        kEX_SSS_objID_ECKEY_Auth);
  return kStatus_SSS_Success;
}

static sss_status_t se051h_aes_key_provision() {
  smStatus_t status = SM_NOT_OK;
  const uint8_t aes_value[] = EX_SSS_AUTH_SE05X_APPLETSCP_VALUE;
  uint32_t delete_policy = POLICY_OBJ_ALLOW_DELETE;
  status = se051h_key_provision("AESKEY", kEX_SSS_ObjID_APPLETSCP03_Auth,
                                aes_value, sizeof(aes_value), delete_policy);
  ENSURE_OR_RETURN_ON_ERROR(status == SM_OK, kStatus_SSS_Fail);
  LOG_I("kEX_SSS_ObjID_APPLETSCP03_Auth provision successful for Object ID "
        "0x%08X\n",
        kEX_SSS_ObjID_APPLETSCP03_Auth);
  return kStatus_SSS_Success;
}

static sss_status_t se051h_userid_key_provision() {
  smStatus_t status = SM_NOT_OK;
  const uint8_t userid_value[] = EX_SSS_AUTH_SE05X_UserID_VALUE;
  uint32_t delete_policy = POLICY_OBJ_ALLOW_DELETE;
  status =
      se051h_key_provision("USERID", kEX_SSS_ObjID_UserID_Auth, userid_value,
                           sizeof(userid_value), delete_policy);
  ENSURE_OR_RETURN_ON_ERROR(status == SM_OK, kStatus_SSS_Fail);
  LOG_I("kEX_SSS_ObjID_UserID_Auth provision successful for Object ID 0x%08X\n",
        kEX_SSS_ObjID_UserID_Auth);
  return kStatus_SSS_Success;
}

#define SE05X_DELETE_KEY_TEMPLATE(x)                                           \
  smstatus = se05x_delete_key(x);                                              \
  ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

static sss_status_t se051h_do_reset() {
  smStatus_t smstatus = SM_NOT_OK;

  SE05X_DELETE_KEY_TEMPLATE(SE051H_DAC_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_PAI_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_ATTEST_TBS);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_SELECT_RESPONSE_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_DA_KEY_PAIR_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_NODE_OP_KEY_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_NOC_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_ROOT_CER_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_WIFI_CRED_ID_APP_8_4);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_WIFI_CRED_ID_APP_8_8);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_ACL_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_IPK_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_BASIC_INFO_CLUSTER_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_GENERAL_COMM_CLUSTER_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_OP_CRED_CLUSTER_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_ACC_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_NCC_ID);
  SE05X_DELETE_KEY_TEMPLATE(SE051H_VR_ID);
  return kStatus_SSS_Success;
}

void se051h_nfc_comm_prov(ex_sss_boot_ctx_t *pCtx, uint8_t do_reset,
                          uint8_t only_t4t_provision, uint8_t *qrcode,
                          size_t qrcodeLen, uint8_t device_network_type,
                          uint8_t tp_spake_passcode_set_no,
                          uint32_t tp_spake_itter_to_be_used,
                          uint8_t do_ec_key_provision,
                          uint8_t do_aes_key_provision,
                          uint8_t do_user_id_provision,
                          uint8_t provision_with_policy) {
  sss_status_t status = kStatus_SSS_Success;
  const char *portName = nullptr;

  if (se05x_host_gpio_power_init() != 0) {
    LOG_E("SE05x - Error in se05x_host_gpio_power_init function");
    LOG_E("SE05x - Crypto operations offloaded to secure element will fail");
  }

  LOG_I("SE05x - Turn ON secure Element");
  if (se05x_host_gpio_power_set(1) != 0) {
    LOG_E("SE05x - Error in se05x_host_gpio_power_set(1) function");
  }

  if (pCtx == NULL) {
    memset(&gex_sss_chip_ctx, 0, sizeof(gex_sss_chip_ctx));

    status = ex_sss_boot_connectstring(0, NULL, (char **)&portName);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    status = ex_sss_boot_open(&gex_sss_chip_ctx, portName);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    status = ex_sss_key_store_and_object_init(&gex_sss_chip_ctx);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
  } else {
    memcpy(&gex_sss_chip_ctx, pCtx, sizeof(ex_sss_boot_ctx_t));
  }

  if (provision_with_policy) {
    policy = 1;
  }

  if (do_reset) {
    LOG_I("Deleting all keys of NFC Commissioning provison example");
    status = se051h_do_reset();
    if (status != kStatus_SSS_Success) {
      LOG_E("Error in se051h_do_reset function");
    }
    goto cleanup;
  }

  if (only_t4t_provision == 1) {
    goto t4t_provision;
  }

  if (do_ec_key_provision == 1) {
    status = se051h_ec_key_provision();
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
  }

  if (do_aes_key_provision == 1) {
    status = se051h_aes_key_provision();
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
  }

  if (do_user_id_provision == 1) {
    status = se051h_userid_key_provision();
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
  }

  if (do_ec_key_provision || do_aes_key_provision || do_user_id_provision) {
    goto cleanup;
  }

#if 0
    // Pass code credentials (Binary file containing pass code and salt)
    status = se051h_provision_passcode_parameters();
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
#endif

  // PAKE credentials (iteration count, fail safe timer, session parameters)
  status = se051h_provision_pbkdf_parameters(tp_spake_passcode_set_no,
                                             tp_spake_itter_to_be_used);
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

#if 0
    // spake2+ verifier (w0 and L)
    status = se051h_provision_spake2p_verifiers();
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
#endif

  // DAC certificate
  status = se051h_provision_dac_cert();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // PAI certificate
  status = se051h_provision_pai_cert();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // Device attestation key
  status = se051h_provision_da_key();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // Attestation TBS
  status = se051h_provision_attest_tbs();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // Select response
  status = se051h_provision_select_response();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // Node Operational key (Dummy values)
  status = se051h_provision_node_oper_key();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // Node Operational certificate (Dummy values)
  status = se051h_provision_node_oper_cert();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // Root Certificate
  status = se051h_provision_root_cert();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // WiFi SSID and pass-code (for applet 8.6)
  status = se051h_provision_ssid_passcode();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // WiFi SSID and pass-code (for applet 8.8 and after)
  status = se051h_provision_ssid_passcode_app_8_8();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // Access control list
  status = se051h_provision_acl();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // Identity Protection Key (Dummy values)
  status = se051h_provision_identity_protection_key();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // Basic Information Cluster
  status = se051h_provision_basic_info_cluster();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // General Commissioning Cluster
  status = se051h_provision_general_comm_cluster();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // Operational Credential Cluster
  status = se051h_provision_operational_cred_cluster();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // Access Control Cluster
  status = se051h_provision_access_control_cluster();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // Network Commissioning Cluster
  status = se051h_provision_network_comm_cluster(device_network_type);
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

  // Vendor Reserved
  status = se051h_provision_vendor_reserved();
  ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

t4t_provision:
#if SSS_HAVE_APPLET_SE051_H
  if (qrcodeLen != 0) {
    status = se051h_provision_t4t_applet(qrcode, qrcodeLen);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
  } else {
    LOG_I("No QR code passed");
  }
#endif

cleanup:
  LOG_I("SE05x - Turn OFF secure Element");
  if (se05x_host_gpio_power_set(0) != 0) {
    LOG_E("SE05x - Failed to set the GPIO connected to SE05x to low");
  }

  LOG_I("SE05x - De-initialize GPIO");
  if (se05x_host_gpio_power_deinit() != 0) {
    LOG_E("SE05x - Failed to de-initialize GPIO connected to SE05x");
  }

  if (kStatus_SSS_Success == status) {
    LOG_I("se051h_nfc_comm_prov Provision example successful !!!...");
  } else {
    LOG_E("se051h_nfc_comm_prov Provision example failed !!!...");
  }

  return;
}
