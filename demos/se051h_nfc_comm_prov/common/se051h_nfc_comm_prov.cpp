/*
 *
 * Copyright 2021,2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* ************************************************************************** */
/* Includes                                                                   */
/* ************************************************************************** */

#include <errno.h>
#include <ex_sss.h>
#include <ex_sss_boot.h>
#include <fsl_sss_se05x_apis.h>
#include <se05x_ecc_curves_values.h>
#include <nxEnsure.h>
#include <nxLog_App.h>
#include <stdio.h>
#include <string.h>
#include "se05x_APDU.h"
#include "ex_sss_objid.h"
#include "se051h_nfc_comm_prov.h"

static ex_sss_boot_ctx_t gex_sss_chip_ctx;

static sss_status_t se051h_set_key(const uint8_t *buffer, size_t bufferLen,
                                  size_t bitLen, sss_key_part_t keyPart,
                                  sss_cipher_type_t cipherType, uint32_t keyId,
                                  void *options, size_t optionsLen) {
  sss_status_t status = kStatus_SSS_Success;
  sss_object_t keyObj;
  smStatus_t smstatus = SM_NOT_OK;

  status = sss_key_object_init(&keyObj, &gex_sss_chip_ctx.ks);
  ENSURE_OR_RETURN_ON_ERROR(status == kStatus_SSS_Success, status);

  status =
      sss_key_object_allocate_handle(&keyObj, keyId, keyPart, cipherType,
                                     bufferLen, kKeyObject_Mode_Persistent);
  ENSURE_OR_RETURN_ON_ERROR(status == kStatus_SSS_Success, status);

  if (cipherType == kSSS_CipherType_Binary || cipherType == kSSS_CipherType_Certificate)
  {
      smstatus = Se05x_API_WriteBinary_Ver(&((sss_se05x_session_t*)&gex_sss_chip_ctx.session)->s_ctx,
          NULL,
          keyId,
          0,
          (uint16_t)bufferLen,
          buffer,
          bufferLen,
          0);
      if (smstatus != SM_OK) {
          LOG_E("Error in setting hmac key");
          status = kStatus_SSS_Fail;
      }
  }
  else if (cipherType == kSSS_CipherType_HMAC)
  {
      SE05x_KeyID_t kekID      = SE05x_KeyID_KEK_NONE;
      sss_se05x_key_store_t *se05x_keyStore = (sss_se05x_key_store_t *)&gex_sss_chip_ctx.ks;
      SE05x_Result_t exists = kSE05x_Result_NA;

      if (se05x_keyStore->kekKey != NULL) {
          kekID = se05x_keyStore->kekKey->keyId;
      }

      if (bitLen % 8 == 0) {
          smstatus = Se05x_API_CheckObjectExists(&((sss_se05x_session_t *) &gex_sss_chip_ctx.session)->s_ctx, keyId, &exists);
          if (smstatus == SM_OK){
              if (exists == kSE05x_Result_SUCCESS) {
                  LOG_I("spake2p_verifiers credentials already provisioned");
                  return status;
              }
              else {
                  smstatus = Se05x_API_WriteSymmKey_Ver(&((sss_se05x_session_t *) &gex_sss_chip_ctx.session)->s_ctx,
                  NULL,
                  SE05x_MaxAttemps_NA,
                  keyId,
                  kekID,
                  buffer,
                  bufferLen,
                  kSE05x_INS_NA,
                  kSE05x_SymmKeyType_HMAC,
                  0);
                  if (smstatus != SM_OK) {
                      LOG_E("Error in setting hmac key");
                      status = kStatus_SSS_Fail;
                  }
             }
         }
         else {
            LOG_E("Se05x_API_CheckObjectExists Failed");
            status = kStatus_SSS_Fail;
        }
      }
      else {
          LOG_E("Error in setting hmac key");
          status = kStatus_SSS_Fail;
      }
  }
  else {
      status = sss_key_store_set_key(&gex_sss_chip_ctx.ks, &keyObj, buffer,
                                bufferLen, bitLen, options, optionsLen);
      if (status != kStatus_SSS_Success) {
          printf("Error in sss_key_store_set_key \n");
      }
  }

  return status;
}

static smStatus_t se05x_delete_key(uint32_t keyid) {

    smStatus_t smstatus   = SM_NOT_OK;
    SE05x_Result_t exists = kSE05x_Result_NA;

    if (gex_sss_chip_ctx.ks.session != NULL){
        smstatus = Se05x_API_CheckObjectExists(&((sss_se05x_session_t *) &gex_sss_chip_ctx.session)->s_ctx, keyid, &exists);
        if (smstatus == SM_OK){
            if (exists == kSE05x_Result_SUCCESS){
                smstatus = Se05x_API_DeleteSecureObject(&((sss_se05x_session_t *) &gex_sss_chip_ctx.session)->s_ctx, keyid);
                if (smstatus != SM_OK){
                    LOG_E("Error in deleting key");
                }
            }
            else{
                LOG_W("Key does not exists");
            }
        }
        else{
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

static sss_status_t se051h_provision_pbkdf_parameters()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;
    uint8_t pbkdf_Buffer[] = {
        PBKDF_PARAMS
    };

    smstatus = se05x_delete_key(SE051H_PBKDF_PARAMS_ID);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    LOG_I("Writing PBKDF Parameters to SE05x at Key id = %x",  SE051H_PBKDF_PARAMS_ID);
    status = se051h_set_key(pbkdf_Buffer, sizeof(pbkdf_Buffer), sizeof(pbkdf_Buffer)*8 , kSSS_KeyPart_Default, kSSS_CipherType_Binary, SE051H_PBKDF_PARAMS_ID, NULL, 0);
    if(status != kStatus_SSS_Success){
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

static sss_status_t se051h_provision_dac_cert()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;

    uint8_t dac_cer[] = {
        DAC_CERTIFICATE
    };

    smstatus = se05x_delete_key(SE051H_DAC_ID);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    /*set Device Attestation certificate*/
    LOG_I("Writing Device Attestation Certificate to SE05x at Key id = %x",  SE051H_DAC_ID);
    status = se051h_set_key(dac_cer, sizeof(dac_cer), sizeof(dac_cer)*8, kSSS_KeyPart_Default, kSSS_CipherType_Certificate, SE051H_DAC_ID, NULL, 0);
    if(status != kStatus_SSS_Success) {
        printf("Error in se051h_set_dev_attest_cert\n");
    }

    return status;
}

static sss_status_t se051h_provision_pai_cert()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;

    uint8_t pai_cer[] = {
        PAI_CERTIFICATE
    };

    smstatus = se05x_delete_key(SE051H_PAI_ID);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    /*set Device Attestation certificate*/
    LOG_I("Writing PAI certificate to SE05x at Key id = %x",  SE051H_PAI_ID);
    status = se051h_set_key(pai_cer, sizeof(pai_cer), sizeof(pai_cer)*8, kSSS_KeyPart_Default, kSSS_CipherType_Certificate, SE051H_PAI_ID, NULL, 0);
    if(status != kStatus_SSS_Success) {
        printf("Error in se051h_set_pai_cert\n");
    }

    return status;
}

static sss_status_t se051h_provision_da_key()
{

    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;
    SE05x_Result_t exists = kSE05x_Result_NA;

    uint8_t privKey[] = {
        DA_KEY_PAIR_DATA
    };

    smstatus = Se05x_API_CheckObjectExists(&((sss_se05x_session_t *) &gex_sss_chip_ctx.session)->s_ctx, SE051H_DA_KEY_PAIR_ID, &exists);
    if (smstatus == SM_OK) {
        if (exists == kSE05x_Result_SUCCESS) {
            LOG_I("Device Attestation Key already provisioned");
            status = kStatus_SSS_Success;
        }
        else {
            LOG_I("Writing DA private key to SE05x at Key id = %x",  SE051H_DA_KEY_PAIR_ID);
            status = se051h_set_key(privKey, sizeof(privKey), 256, kSSS_KeyPart_Pair, kSSS_CipherType_EC_NIST_P, SE051H_DA_KEY_PAIR_ID, NULL, 0);
            if(status != kStatus_SSS_Success) {
                printf("Error is set DA private key\n");
            }
        }
    }
    else {
        LOG_E("Se05x_API_CheckObjectExists Failed");
    }

    return status;
}

static sss_status_t se051h_provision_attest_tbs()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;

    uint8_t attest_tbs[] = {
        STRUCTURE_START,
        CERTIFICATE_DECLARATION,
        ATTESTATION_NONCE,
        TIMESTAMP,
        STRUCTURE_END,
        ATTESTATION_CHALLENGE
    };

    smstatus = se05x_delete_key(SE051H_ATTEST_TBS);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    LOG_I("Writing Attestation TBS at Key id = %x", SE051H_ATTEST_TBS);
    status = se051h_set_key(attest_tbs, sizeof(attest_tbs), sizeof(attest_tbs)*8, kSSS_KeyPart_Default, kSSS_CipherType_Binary, SE051H_ATTEST_TBS, NULL, 0);
    if(status != kStatus_SSS_Success) {
        printf("Error in writing se051h_provision_attest_tbs data\n");
    }

    return status;
}

static sss_status_t se051h_provision_select_response()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;

    uint8_t select_response[] = {
        SELECT_RESPONSE
    };

    smstatus = se05x_delete_key(SE01H_SELECT_RESPONSE_ID);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    LOG_I("Writing select response data at Key id = %x", SE01H_SELECT_RESPONSE_ID);
    status = se051h_set_key(select_response, sizeof(select_response), sizeof(select_response)*8, kSSS_KeyPart_Default, kSSS_CipherType_Binary, SE01H_SELECT_RESPONSE_ID, NULL, 0);
    if(status != kStatus_SSS_Success) {
        printf("Error in se051h_provision_select_response\n");
    }

    return status;
}


static sss_status_t se051h_provision_node_oper_key()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;

    uint8_t no_key[] = {
        NODE_OP_KEY_PAIR_DATA
    };

    smstatus = se05x_delete_key(SE051H_NODE_OP_KEY_ID);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    LOG_I("Writing Node Operational key to SE05x at Key id = %x", SE051H_NODE_OP_KEY_ID);
    status = se051h_set_key(no_key, sizeof(no_key), 256, kSSS_KeyPart_Pair, kSSS_CipherType_EC_NIST_P, SE051H_NODE_OP_KEY_ID, NULL, 0);
    if(status != kStatus_SSS_Success) {
        printf("Error in se051h_provision_node_oper_key\n");
    }

    return status;
}


static sss_status_t se051h_provision_node_oper_cert()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;

    uint8_t noc[] = {
        NOC
    };

    smstatus = se05x_delete_key(SE051H_NOC_ID);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    /* set NOC */
    LOG_I("Writing Node Operational certificate to SE05x at Key id = %x",  SE051H_NOC_ID);
    status = se051h_set_key(noc, sizeof(noc), sizeof(noc)*8, kSSS_KeyPart_Default, kSSS_CipherType_Certificate, SE051H_NOC_ID, NULL, 0);
    if(status != kStatus_SSS_Success) {
        printf("Error in se051h_provision_node_oper_cert\n");
    }

    return status;
}

static sss_status_t se051h_provision_root_cert()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;

    uint8_t root_ca[] = {
        ROOT_CERTIFICATE
    };

    smstatus = se05x_delete_key(SE051H_ROOT_CER_ID);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    /* set Root certificate */
    LOG_I("Writing Root certificate to SE05x at Key id = %x",  SE051H_ROOT_CER_ID);
    status = se051h_set_key(root_ca, sizeof(root_ca), sizeof(root_ca)*8, kSSS_KeyPart_Default, kSSS_CipherType_Certificate, SE051H_ROOT_CER_ID, NULL, 0);
    if(status != kStatus_SSS_Success) {
        printf("Error in se051h_provision_root_cert\n");
    }

    return status;
}

static sss_status_t se051h_provision_ssid_passcode()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;

    uint8_t wifi_cred_data[] = {
        WIFI_CRED_DATA
    };

    smstatus = se05x_delete_key(SE051H_WIFI_CRED_ID);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    LOG_I("Writing WI-FI credentials to SE05x at Key id = %x",  SE051H_WIFI_CRED_ID);
    status = se051h_set_key(wifi_cred_data, sizeof(wifi_cred_data), sizeof(wifi_cred_data)*8 , kSSS_KeyPart_Default, kSSS_CipherType_Binary, SE051H_WIFI_CRED_ID, NULL, 0);
    if(status != kStatus_SSS_Success){
        printf("Error in se051h_provision_ssid_passcode\n");
    }

    return status;
}

static sss_status_t se051h_provision_acl() {

    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;

    uint8_t acl_data[] = {
        ACL
    };

    smstatus = se05x_delete_key(SE051H_ACL_ID);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    LOG_I("Writing acl to SE05x at Key id = %x", SE051H_ACL_ID);
    status = se051h_set_key(acl_data, sizeof(acl_data), sizeof(acl_data)*8, kSSS_KeyPart_Default, kSSS_CipherType_Binary, SE051H_ACL_ID, NULL, 0);
    if(status != kStatus_SSS_Success){
        printf("Error in se051h_provision_acl\n");
    }

    return status;
}

static sss_status_t se051h_provision_identity_protection_key()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;

    uint8_t ipk_data[] = {
        IPK
    };

    smstatus = se05x_delete_key(SE051H_IPK_ID);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    LOG_I("Writing ipk to SE05x at Key id = %x", SE051H_IPK_ID);
    status = se051h_set_key(ipk_data, sizeof(ipk_data), sizeof(ipk_data)*8, kSSS_KeyPart_Default, kSSS_CipherType_Binary, SE051H_IPK_ID, NULL, 0);
    if(status != kStatus_SSS_Success) {
        printf("Error in se051h_provision_identity_protection_key\n");
    }

    return status;
}

static sss_status_t se051h_provision_basic_info_cluster()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;

    uint8_t Basic_info_cluster_data[] = {
       DATA_VERSION_BIC,
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
       CONFIGURATION_VERSION
    };

    smstatus = se05x_delete_key(SE051H_BASIC_INFO_CLUSTER_ID);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    LOG_I("Writing Basic Information cluster data to SE05x at Key id = %x",  SE051H_BASIC_INFO_CLUSTER_ID);
    status = se051h_set_key(Basic_info_cluster_data, sizeof(Basic_info_cluster_data), sizeof(Basic_info_cluster_data)*8 , kSSS_KeyPart_Default, kSSS_CipherType_Binary, SE051H_BASIC_INFO_CLUSTER_ID, NULL, 0);
    if(status != kStatus_SSS_Success){
        printf("Error in se051h_set_basic_info_cluster_data\n");
    }

    return status;
}

static sss_status_t se051h_provision_general_comm_cluster()
{
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
        RECOVERY_IDENTIFIER,
    };

    smstatus = se05x_delete_key(SE051H_GENERAL_COMM_CLUSTER_ID);
    if(smstatus != SM_OK){
        printf("Error in se05x_delete_key\n");
        return status;
    }

    LOG_I("Writing General Commissioning cluster data to SE05x at Key id = %x",  SE051H_GENERAL_COMM_CLUSTER_ID);
    status = se051h_set_key(Genaral_comm_cluster_data, sizeof(Genaral_comm_cluster_data), sizeof(Genaral_comm_cluster_data)*8 , kSSS_KeyPart_Default, kSSS_CipherType_Binary, SE051H_GENERAL_COMM_CLUSTER_ID, NULL, 0);
    if(status != kStatus_SSS_Success){
        printf("Error in se051h_set_Genaral_comm_cluster_data\n");
    }

    return status;
}

static sss_status_t se051h_provision_operational_cred_cluster()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;


    uint8_t OCC_cluster_data[] = {
        OCC
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

    LOG_I("Writing Operational credentials data to SE05x at Key id = %x",  SE051H_OP_CRED_CLUSTER_ID);
    status = se051h_set_key(OCC_cluster_data, sizeof(OCC_cluster_data), sizeof(OCC_cluster_data)*8 , kSSS_KeyPart_Default, kSSS_CipherType_Binary, SE051H_OP_CRED_CLUSTER_ID, NULL, 0);
    if(status != kStatus_SSS_Success){
        printf("Error in se051h_set_OCC_cluster_data\n");
    }

    return status;

}

static sss_status_t se051h_provision_access_control_cluster()
{
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

    LOG_I("Writing ACL data to SE05x at Key id = %x",  SE051H_ACC_ID);
    status = se051h_set_key(acl_data, sizeof(acl_data), sizeof(acl_data)*8 , kSSS_KeyPart_Default, kSSS_CipherType_Binary, SE051H_ACC_ID, NULL, 0);
    if(status != kStatus_SSS_Success){
        printf("Error in se051h_set_acl_data\n");
    }

    return status;

}

static sss_status_t se051h_provision_network_comm_cluster()
{
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

    smstatus = se05x_delete_key(SE051H_NCC_ID);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    LOG_I("Writing ncc data to SE05x at Key id = %x",  SE051H_NCC_ID);
    status = se051h_set_key(ncc_buf, sizeof(ncc_buf), sizeof(ncc_buf)*8 , kSSS_KeyPart_Default, kSSS_CipherType_Binary, SE051H_NCC_ID, NULL, 0);
    if(status != kStatus_SSS_Success){
        printf("Error in se051h_set_ncc_data\n");
    }

    return status;

}

static sss_status_t se051h_provision_vendor_reserved()
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t smstatus = SM_NOT_OK;

    uint8_t vendor_reserved[] = {
        VENDOR_RESERVED
    };

    smstatus = se05x_delete_key(SE051H_VR_ID);
    ENSURE_OR_RETURN_ON_ERROR(smstatus == SM_OK, kStatus_SSS_Fail);

    LOG_I("Writing vendor reserved data to SE05x at Key id = %x",  SE051H_VR_ID);
    status = se051h_set_key(vendor_reserved, sizeof(vendor_reserved), sizeof(vendor_reserved)*8, kSSS_KeyPart_Default, kSSS_CipherType_Binary, SE051H_VR_ID, NULL, 0);
    if(status != kStatus_SSS_Success) {
        printf("Error in se051h_provision_vendor_reserved\n");
    }

    return status;
}

void se051h_nfc_comm_prov(ex_sss_boot_ctx_t *pCtx)
{
    sss_status_t status = kStatus_SSS_Success;
    const char *portName = nullptr;

    if (pCtx == NULL) {
        memset(&gex_sss_chip_ctx,  0,  sizeof(gex_sss_chip_ctx));

        status = ex_sss_boot_connectstring(0,  NULL,  (char**)&portName);
        ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

        status = ex_sss_boot_open(&gex_sss_chip_ctx,  portName);
        ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

        status = ex_sss_key_store_and_object_init(&gex_sss_chip_ctx);
        ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
    }
    else {
        memcpy(&gex_sss_chip_ctx, pCtx, sizeof(ex_sss_boot_ctx_t));
    }

#if 0
    // Pass code credentials (Binary file containing pass code and salt)
    status = se051h_provision_passcode_parameters();
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
#endif

    // PAKE credentials (iteration count, fail safe timer, session parameters)
    status = se051h_provision_pbkdf_parameters();
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

    // WiFi SSID and pass-code
    status = se051h_provision_ssid_passcode();
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
    status = se051h_provision_network_comm_cluster();
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    // Vendor Reserved
    status = se051h_provision_vendor_reserved();

cleanup:

    if (kStatus_SSS_Success == status) {
        LOG_I("se051h_nfc_comm_prov Provision example successful !!!...");
    } else {
        LOG_E("se051h_nfc_comm_prov Provision example failed !!!...");
    }

    return;
}
