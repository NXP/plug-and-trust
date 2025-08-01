/*
 *
 * Copyright 2018-2020,2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fsl_sss_mbedtls_apis.h>

#define MBEDTLS_DO_LITTLE_ENDIAN

#if SSS_HAVE_HOSTCRYPTO_MBEDTLS

#include <mbedtls/version.h>
#include <stdlib.h>
#ifdef MBEDTLS_FS_IO
#include <memory.h>
#endif
#include <inttypes.h>
#include <mbedtls/aes.h>
#include <mbedtls/base64.h>
#include <mbedtls/cmac.h>
#include <mbedtls/des.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/md.h>
#include <nxEnsure.h>
#include <nxLog_sss.h>
#include <sm_types.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <fsl_sss_util_asn1_der.h>

// #include "../../ex/inc/ex_sss_objid.h" // Enable to test SIMW-656

#define MAX_KEY_OBJ_COUNT KS_N_ENTIRES
#define MAX_FILE_NAME_SIZE 255
#define MAX_SHARED_SECRET_DERIVED_DATA 255
#define BEGIN_PRIVATE "-----BEGIN PRIVATE KEY-----\n"
#define END_PRIVATE "\n-----END PRIVATE KEY-----"
#define BEGIN_PUBLIC "-----BEGIN PUBLIC KEY-----\n"
#define END_PUBLIC "\n-----END PUBLIC KEY-----"

#define CIPHER_BLOCK_SIZE 16
#define DES_BLOCK_SIZE (MBEDTLS_KEY_LENGTH_DES / 8)

#if ((defined(SSS_HAVE_SE05X_AUTH_PLATFSCP03) && (SSS_HAVE_SE05X_AUTH_PLATFSCP03 == 1)) && \
     !defined(MBEDTLS_CIPHER_MODE_CBC))
#error "PlatformSCP03 requires MBEDTLS_CIPHER_MODE_CBC to be enabled"
#endif

/* ************************************************************************** */
/* Functions : Private sss mbedtls delceration                                */
/* ************************************************************************** */
static sss_status_t sss_mbedtls_drbg_seed(sss_mbedtls_session_t *pSession, const char *pers, size_t persLen);

#if SSS_HAVE_MBEDTLS_3_X
int mbedtls_entropy_func_3_X(void *data, unsigned char *output, size_t len);
#endif

#if SSSFTR_SW_ECC && SSS_HAVE_TESTCOUNTERPART && defined(MBEDTLS_ECP_C)
static sss_status_t sss_mbedtls_generate_ecp_key(
    mbedtls_pk_context *pkey, sss_mbedtls_session_t *pSession, size_t keyBitLen, sss_cipher_type_t key_typ);
#endif

#if SSSFTR_SW_RSA && SSS_HAVE_TESTCOUNTERPART && defined(MBEDTLS_RSA_C)
static sss_status_t sss_mbedtls_generate_rsa_key(
    mbedtls_pk_context *pkey, sss_mbedtls_session_t *pSession, size_t keyBitLen);
#endif

#if SSSFTR_SW_TESTCOUNTERPART
static sss_status_t sss_mbedtls_hkdf_extract(const mbedtls_md_info_t *md,
    const uint8_t *salt,
    size_t salt_len,
    const uint8_t *ikm,
    size_t ikm_len,
    uint8_t *prk);

static sss_status_t sss_mbedtls_hkdf_expand(const mbedtls_md_info_t *md,
    const uint8_t *prk,
    size_t prk_len,
    const uint8_t *info,
    size_t info_len,
    uint8_t *okm,
    size_t okm_len);
#endif

static sss_status_t sss_mbedtls_set_key(
    sss_mbedtls_object_t *keyObject, const uint8_t *data, size_t dataLen, size_t keyBitLen);

#if SSS_HAVE_TESTCOUNTERPART
static sss_status_t sss_mbedtls_aead_ccm_finish(
    sss_mbedtls_aead_t *context, uint8_t *destData, size_t *destLen, uint8_t *tag, size_t *tagLen);
static sss_status_t sss_mbedtls_aead_ccm_update(sss_mbedtls_aead_t *context, const uint8_t *srcData, size_t srcLen);
#endif
/* ************************************************************************** */
/* Functions : sss_mbedtls_session                                            */
/* ************************************************************************** */

#ifndef MBEDTLS_CTR_DRBG_C
#error Need MBEDTLS_CTR_DRBG_C defined
#endif

// LCOV_EXCL_START
sss_status_t sss_mbedtls_session_create(sss_mbedtls_session_t *session,
    sss_type_t subsystem,
    uint32_t application_id,
    sss_connection_type_t connection_type,
    void *connectionData)
{
    sss_status_t retval = kStatus_SSS_Success;
    AX_UNUSED_ARG(session);
    AX_UNUSED_ARG(subsystem);
    AX_UNUSED_ARG(application_id);
    AX_UNUSED_ARG(connection_type);
    AX_UNUSED_ARG(connectionData);
    /* Nothing special to be handled */
    return retval;
}
// LCOV_EXCL_STOP

sss_status_t sss_mbedtls_session_open(sss_mbedtls_session_t *session,
    sss_type_t subsystem,
    uint32_t application_id,
    sss_connection_type_t connection_type,
    void *connectionData)
{
    sss_status_t retval      = kStatus_SSS_InvalidArgument;
    static const char pers[] = "mbedtls_session";

    ENSURE_OR_GO_EXIT(session != NULL);
    memset(session, 0, sizeof(*session));
    AX_UNUSED_ARG(application_id);
    ENSURE_OR_GO_EXIT(connection_type == kSSS_ConnectionType_Plain);

#ifdef MBEDTLS_FS_IO
    if (connectionData == NULL) {
        /* Nothing */
    }
    else {
        const char *szRootPath = (const char *)connectionData;
        session->szRootPath    = szRootPath;
    }
#else
    if (connectionData != NULL) {
        /* Can't support connectionData  != NULL for mbedTLS without
        * MBEDTLS_FS_IO */
        retval = kStatus_SSS_InvalidArgument;
        goto exit;
    }
#endif
    retval            = kStatus_SSS_Fail;
    session->ctr_drbg = SSS_CALLOC(1, sizeof(*session->ctr_drbg));
    ENSURE_OR_GO_EXIT(session->ctr_drbg != NULL);

    session->entropy = SSS_CALLOC(1, sizeof(*session->entropy));
    ENSURE_OR_GO_EXIT(session->entropy != NULL);
    retval = kStatus_SSS_InvalidArgument;

    mbedtls_ctr_drbg_init((session->ctr_drbg));
    mbedtls_entropy_init((session->entropy));
    retval = sss_mbedtls_drbg_seed(session, pers, sizeof(pers) - 1);
    if (retval != kStatus_SSS_Success) {
        LOG_E("MbedTLS:DRBG Failed");
        goto exit;
    }
    /* Success */
    session->subsystem = subsystem;

exit:
    return retval;
}

sss_status_t sss_mbedtls_session_prop_get_u32(sss_mbedtls_session_t *session, uint32_t property, uint32_t *pValue)
{
    sss_status_t retval = kStatus_SSS_Fail;
    AX_UNUSED_ARG(session);
    AX_UNUSED_ARG(property);
    AX_UNUSED_ARG(pValue);
    /* TBU */
    return retval;
}

sss_status_t sss_mbedtls_session_prop_get_au8(
    sss_mbedtls_session_t *session, uint32_t property, uint8_t *pValue, size_t *pValueLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
    AX_UNUSED_ARG(session);
    AX_UNUSED_ARG(property);
    AX_UNUSED_ARG(pValue);
    AX_UNUSED_ARG(pValueLen);
    /* TBU */
    return retval;
}

void sss_mbedtls_session_close(sss_mbedtls_session_t *session)
{
    if (session->ctr_drbg != NULL) {
        mbedtls_ctr_drbg_free(session->ctr_drbg);
        SSS_FREE(session->ctr_drbg);
    }
    if (session->entropy != NULL) {
        mbedtls_entropy_free(session->entropy);
        SSS_FREE(session->entropy);
    }
    memset(session, 0, sizeof(*session));
}

void sss_mbedtls_session_delete(sss_mbedtls_session_t *session)
{
    AX_UNUSED_ARG(session);
}

/* End: mbedtls_session */

/* ************************************************************************** */
/* Functions : sss_mbedtls_keyobj                                             */
/* ************************************************************************** */

sss_status_t sss_mbedtls_key_object_init(sss_mbedtls_object_t *keyObject, sss_mbedtls_key_store_t *keyStore)
{
    sss_status_t retval = kStatus_SSS_Fail;
    ENSURE_OR_GO_CLEANUP(keyObject);
    ENSURE_OR_GO_CLEANUP(keyStore);
    memset(keyObject, 0, sizeof(*keyObject));
    keyObject->keyStore = keyStore;
    retval              = kStatus_SSS_Success;
cleanup:
    return retval;
}

sss_status_t sss_mbedtls_key_object_allocate_handle(sss_mbedtls_object_t *keyObject,
    uint32_t keyId,
    sss_key_part_t key_part,
    sss_cipher_type_t cipherType,
    size_t keyByteLenMax,
    uint32_t options)
{
    sss_status_t retval = kStatus_SSS_Fail;
    ENSURE_OR_GO_CLEANUP(keyObject);
    ENSURE_OR_GO_CLEANUP(keyId != 0);
    ENSURE_OR_GO_CLEANUP(keyId != 0xFFFFFFFFu);

#ifdef EX_SSS_OBJID_TEST_START
    if (keyId < EX_SSS_OBJID_TEST_START) {
        return kStatus_SSS_Fail;
    }
    if (keyId > EX_SSS_OBJID_TEST_END) {
        return kStatus_SSS_Fail;
    }
#endif

    if (options != kKeyObject_Mode_Persistent && options != kKeyObject_Mode_Transient) {
        LOG_E("sss_mbedtls_key_object_allocate_handle option invalid 0x%X", options);
        retval = kStatus_SSS_Fail;
        goto cleanup;
    }
    {
        /* to avoid error -Werror=type-limit */
        unsigned int uikey_part = ((unsigned int)key_part);
        if (uikey_part > UINT8_MAX) {
            LOG_E(" Only objectType 8 bits wide supported");
            retval = kStatus_SSS_Fail;
            goto cleanup;
        }
    }
#if defined(MBEDTLS_FS_IO) && !AX_EMBEDDED
    if (options == kKeyObject_Mode_Persistent) {
        uint32_t i;
        sss_mbedtls_object_t **ks;
        ENSURE_OR_GO_CLEANUP(keyObject->keyStore);
        ENSURE_OR_GO_CLEANUP(keyObject->keyStore->max_object_count != 0);
        ENSURE_OR_GO_CLEANUP(keyByteLenMax < UINT16_MAX);
        retval = ks_common_update_fat(
            keyObject->keyStore->keystore_shadow, keyId, key_part, cipherType, 0, 0, (uint16_t)keyByteLenMax);
        ENSURE_OR_GO_CLEANUP(retval == kStatus_SSS_Success);
        ks     = keyObject->keyStore->objects;
        retval = kStatus_SSS_Fail;
        for (i = 0; i < keyObject->keyStore->max_object_count; i++) {
            if (ks[i] == NULL) {
                ks[i]  = keyObject;
                retval = ks_mbedtls_key_object_create(keyObject, keyId, key_part, cipherType, keyByteLenMax, options);
                break;
            }
        }
    }
    else
#endif
    {
        retval = ks_mbedtls_key_object_create(keyObject, keyId, key_part, cipherType, keyByteLenMax, options);
    }
cleanup:
    return retval;
}

sss_status_t sss_mbedtls_key_object_get_handle(sss_mbedtls_object_t *keyObject, uint32_t keyId)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if defined(MBEDTLS_FS_IO) && !AX_EMBEDDED
    uint32_t i;
    ENSURE_OR_GO_CLEANUP(keyObject);
    ENSURE_OR_GO_CLEANUP(keyObject->keyStore);
    retval = kStatus_SSS_Success;
    /* If key store already has loaded this and shared this - fail */
    for (i = 0; i < keyObject->keyStore->max_object_count; i++) {
        if (keyObject->keyStore->objects[i] != NULL && keyObject->keyStore->objects[i]->keyId == keyId) {
            /* Key Object already loaded and shared in another instance */
            LOG_E("KeyID 0x%X already loaded / shared", keyId);
            retval = kStatus_SSS_Fail;
            break;
        }
    }
    if (retval == kStatus_SSS_Success) {
        for (i = 0; i < keyObject->keyStore->max_object_count; i++) {
            if (keyObject->keyStore->objects[i] == NULL) {
                retval = ks_mbedtls_load_key(keyObject, keyObject->keyStore->keystore_shadow, keyId);
                if (retval == kStatus_SSS_Success) {
                    keyObject->keyStore->objects[i] = keyObject;
                }
                break;
            }
        }
    }
cleanup:
#endif
    return retval;
}

sss_status_t sss_mbedtls_key_object_set_user(sss_mbedtls_object_t *keyObject, uint32_t user, uint32_t options)
{
    sss_status_t retval = kStatus_SSS_Fail;
    AX_UNUSED_ARG(options);
    ENSURE_OR_GO_EXIT((keyObject->accessRights & kAccessPermission_SSS_ChangeAttributes));
    retval             = kStatus_SSS_Success;
    keyObject->user_id = user;
exit:
    return retval;
}

sss_status_t sss_mbedtls_key_object_set_purpose(sss_mbedtls_object_t *keyObject, sss_mode_t purpose, uint32_t options)
{
    sss_status_t retval = kStatus_SSS_Fail;
    AX_UNUSED_ARG(options);
    ENSURE_OR_GO_EXIT((keyObject->accessRights & kAccessPermission_SSS_ChangeAttributes));
    retval             = kStatus_SSS_Success;
    keyObject->purpose = purpose;
exit:
    return retval;
}

sss_status_t sss_mbedtls_key_object_set_access(sss_mbedtls_object_t *keyObject, uint32_t access, uint32_t options)
{
    sss_status_t retval = kStatus_SSS_Fail;
    AX_UNUSED_ARG(options);
    ENSURE_OR_GO_EXIT((keyObject->accessRights & kAccessPermission_SSS_ChangeAttributes));
    retval                  = kStatus_SSS_Success;
    keyObject->accessRights = (sss_access_permission_t)access;
exit:
    return retval;
}

// LCOV_EXCL_START
sss_status_t sss_mbedtls_key_object_set_eccgfp_group(sss_mbedtls_object_t *keyObject, sss_eccgfp_group_t *group)
{
    sss_status_t retval = kStatus_SSS_Success;
    AX_UNUSED_ARG(keyObject);
    AX_UNUSED_ARG(group);
    /* TBU */
    return retval;
}

sss_status_t sss_mbedtls_key_object_get_user(sss_mbedtls_object_t *keyObject, uint32_t *user)
{
    sss_status_t retval = kStatus_SSS_Success;

    if (user == NULL) {
        return kStatus_SSS_Fail;
    }
    *user = keyObject->user_id;
    return retval;
}

sss_status_t sss_mbedtls_key_object_get_purpose(sss_mbedtls_object_t *keyObject, sss_mode_t *purpose)
{
    sss_status_t retval = kStatus_SSS_Success;

    if (purpose == NULL) {
        return kStatus_SSS_Fail;
    }
    *purpose = keyObject->purpose;
    return retval;
}

sss_status_t sss_mbedtls_key_object_get_access(sss_mbedtls_object_t *keyObject, uint32_t *access)
{
    sss_status_t retval = kStatus_SSS_Success;

    if (access == NULL) {
        return kStatus_SSS_Fail;
    }
    *access = keyObject->accessRights;
    return retval;
}
// LCOV_EXCL_STOP

void sss_mbedtls_key_object_free(sss_mbedtls_object_t *keyObject)
{
    if (keyObject != NULL) {
#ifdef MBEDTLS_FS_IO
        if (keyObject->keyStore != NULL && keyObject->objectType != 0) {
            unsigned int i = 0;
            for (i = 0; i < keyObject->keyStore->max_object_count; i++) {
                if (keyObject->keyStore->objects[i] == keyObject) {
                    keyObject->keyStore->objects[i] = NULL;
                    break;
                }
            }
        }
#endif
        if (keyObject->contents != NULL && keyObject->contents_must_free) {
            switch (keyObject->objectType) {
            case kSSS_KeyPart_Public:
            case kSSS_KeyPart_Pair:
            case kSSS_KeyPart_Private: {
                mbedtls_pk_context *pk;
                pk = (mbedtls_pk_context *)keyObject->contents;
                mbedtls_pk_free(pk);
                SSS_FREE(pk);
                break;
            }
            default:
                SSS_FREE(keyObject->contents);
            }
        }
        memset(keyObject, 0, sizeof(*keyObject));
    } /* if (keyObject != NULL) */
}

/* End: mbedtls_keyobj */

/* ************************************************************************** */
/* Functions : sss_mbedtls_keyderive                                          */
/* ************************************************************************** */

sss_status_t sss_mbedtls_derive_key_context_init(sss_mbedtls_derive_key_t *context,
    sss_mbedtls_session_t *session,
    sss_mbedtls_object_t *keyObject,
    sss_algorithm_t algorithm,
    sss_mode_t mode)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSSFTR_SW_ECC && defined(MBEDTLS_ECP_C)
    ENSURE_OR_GO_CLEANUP(context);
    ENSURE_OR_GO_CLEANUP(session);
    ENSURE_OR_GO_CLEANUP(keyObject);
    ENSURE_OR_GO_CLEANUP(keyObject->contents);

    context->session   = session;
    context->keyObject = keyObject;
    context->algorithm = algorithm;
    context->mode      = mode;
    retval             = kStatus_SSS_Success;
cleanup:
#endif
    return retval;
}

sss_status_t sss_mbedtls_derive_key_one_go(sss_mbedtls_derive_key_t *context,
    const uint8_t *saltData,
    size_t saltLen,
    const uint8_t *info,
    size_t infoLen,
    sss_mbedtls_object_t *derivedKeyObject,
    uint16_t deriveDataLen)
{
    size_t adjustedSaltLen = saltLen;

    if (context->mode == kMode_SSS_HKDF_ExpandOnly) {
        adjustedSaltLen = 0;
    }

    // The actual implementation (also used by legacy SSS API) decides
    // on the saltLen parameter to apply either HKDF_EE or HKDK_ExpandOnly (saltLen == 0)
    return sss_mbedtls_derive_key_go(
        context, saltData, adjustedSaltLen, info, infoLen, derivedKeyObject, deriveDataLen, NULL, NULL);
}

sss_status_t sss_mbedtls_derive_key_sobj_one_go(sss_mbedtls_derive_key_t *context,
    sss_mbedtls_object_t *saltKeyObject,
    const uint8_t *info,
    size_t infoLen,
    sss_mbedtls_object_t *derivedKeyObject,
    uint16_t deriveDataLen)
{
    uint8_t saltData[1024] = {0};
    size_t saltLen         = sizeof(saltData);
    size_t dummySize;
    sss_status_t status;

    // The actual implementation (also used by legacy SSS API) decides
    // on the saltLen parameter to apply either HKDF_EE or HKDK_ExpandOnly (saltLen == 0)
    if (context->mode != kMode_SSS_HKDF_ExpandOnly) {
        if (saltKeyObject == NULL) {
            return kStatus_SSS_Fail;
        }
        status = sss_mbedtls_key_store_get_key(saltKeyObject->keyStore, saltKeyObject, saltData, &saltLen, &dummySize);
        if (status != kStatus_SSS_Success) {
            return kStatus_SSS_Fail;
        }
    }
    else {
        saltLen = 0;
    }

    return sss_mbedtls_derive_key_go(
        context, saltData, saltLen, info, infoLen, derivedKeyObject, deriveDataLen, NULL, NULL);
}

// In HKDF Expand only mode PRK is unbounded, we set a maximum of 256 byte
// RFC5869 Section 2.3
#define HKDF_PRK_MAX 256
sss_status_t sss_mbedtls_derive_key_go(sss_mbedtls_derive_key_t *context,
    const uint8_t *saltData,
    size_t saltLen,
    const uint8_t *info,
    size_t infoLen,
    sss_mbedtls_object_t *derivedKeyObject,
    uint16_t deriveDataLen,
    uint8_t *hkdfOutput,
    size_t *hkdfOutputLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSSFTR_SW_TESTCOUNTERPART
    const mbedtls_md_info_t *md = NULL;
    uint8_t *secret;
    size_t secretLen;
    uint8_t prk[HKDF_PRK_MAX] = {
        0,
    };
    size_t prk_len           = 0;
    mbedtls_md_type_t md_alg = MBEDTLS_MD_NONE;

    secret    = context->keyObject->contents;
    secretLen = context->keyObject->contents_size;

    AX_UNUSED_ARG(hkdfOutput);
    AX_UNUSED_ARG(hkdfOutputLen);

    switch (context->algorithm) {
    case kAlgorithm_SSS_SHA1:
    case kAlgorithm_SSS_HMAC_SHA1:
        md_alg = MBEDTLS_MD_SHA1;
        break;
    case kAlgorithm_SSS_SHA256:
    case kAlgorithm_SSS_HMAC_SHA256:
        md_alg = MBEDTLS_MD_SHA256;
        break;
    case kAlgorithm_SSS_SHA384:
    case kAlgorithm_SSS_HMAC_SHA384:
        md_alg = MBEDTLS_MD_SHA384;
        break;
    case kAlgorithm_SSS_SHA512:
    case kAlgorithm_SSS_HMAC_SHA512:
        md_alg = MBEDTLS_MD_SHA512;
        break;
    default:
        return kStatus_SSS_Fail;
    }

    md = mbedtls_md_info_from_type(md_alg);

    if (saltLen == 0) {
        /* Copy key as is */
        if (HKDF_PRK_MAX >= secretLen) {
            memcpy(prk, secret, secretLen);
            prk_len = secretLen;
        }
        else {
            LOG_E("HKDF Expand only (mbedTLS implementation): buffer too small");
            return kStatus_SSS_Fail;
        }
    }
    else {
        retval  = sss_mbedtls_hkdf_extract(md, saltData, saltLen, secret, secretLen, prk);
        prk_len = mbedtls_md_get_size(md);
        if (retval != kStatus_SSS_Success) {
            return kStatus_SSS_Fail;
        }
    }

    retval = sss_mbedtls_hkdf_expand(md, prk, prk_len, info, infoLen, derivedKeyObject->contents, deriveDataLen);
    if (retval == kStatus_SSS_Success) {
        derivedKeyObject->contents_size = deriveDataLen;
    }

#endif
    return retval;
}

sss_status_t sss_mbedtls_derive_key_dh(sss_mbedtls_derive_key_t *context,
    sss_mbedtls_object_t *otherPartyKeyObject,
    sss_mbedtls_object_t *derivedKeyObject)
{
#if SSSFTR_SW_ECC && defined(MBEDTLS_ECP_C)
    sss_status_t retval = kStatus_SSS_Success;
    int ret             = -1;
    mbedtls_pk_context *pKeyPrv;
    mbedtls_ecp_keypair *pEcpPrv;

#if defined(MBEDTLS_ECDH_C)
    mbedtls_pk_context *pKeyExt;
    mbedtls_ecp_keypair *pEcpExt;
#endif
    size_t keyLen = 0;
    size_t sharedSecretLen;
    size_t sharedSecretLen_Derived;
    const mbedtls_ecp_curve_info *p_curve_info = NULL;
    mbedtls_mpi rawSharedData;

    ENSURE_OR_GO_EXIT(otherPartyKeyObject);
    ENSURE_OR_GO_EXIT(derivedKeyObject);

    pKeyPrv = (mbedtls_pk_context *)context->keyObject->contents;
    pEcpPrv = mbedtls_pk_ec(*pKeyPrv);
    ENSURE_OR_GO_EXIT(pEcpPrv);

#if defined(MBEDTLS_ECDH_C)
    pKeyExt = (mbedtls_pk_context *)otherPartyKeyObject->contents;
    pEcpExt = mbedtls_pk_ec(*pKeyExt);
    ENSURE_OR_GO_EXIT(pEcpExt);
#endif

    mbedtls_mpi_init(&rawSharedData);

    /* Compute the size of the shared secret */
    if (otherPartyKeyObject->cipherType == kSSS_CipherType_EC_MONTGOMERY) {
#if SSS_HAVE_MBEDTLS_3_X
        if (pEcpPrv->MBEDTLS_PRIVATE(grp).id == MBEDTLS_ECP_DP_CURVE448) {
#else
        if (pEcpPrv->grp.id == MBEDTLS_ECP_DP_CURVE448) {
#endif
            keyLen = 56;
        }
        else {
            keyLen = 32;
        }
    }
    else {
#if SSS_HAVE_MBEDTLS_3_X
        p_curve_info = mbedtls_ecp_curve_info_from_grp_id(pEcpPrv->MBEDTLS_PRIVATE(grp).id);
#else
        p_curve_info = mbedtls_ecp_curve_info_from_grp_id(pEcpPrv->grp.id);
#endif
        if (p_curve_info != NULL) {
            keyLen = (size_t)(((p_curve_info->bit_size + 7)) / 8);
        }
        else {
            retval = kStatus_SSS_Fail;
            goto exit;
        }
    }

    sharedSecretLen = (size_t)(keyLen);
#if defined(MBEDTLS_ECDH_C)
#if SSS_HAVE_MBEDTLS_3_X
    ret = mbedtls_ecdh_compute_shared(&pEcpPrv->MBEDTLS_PRIVATE(grp),
        &rawSharedData,
        &(pEcpExt->MBEDTLS_PRIVATE(Q)),
        &(pEcpPrv->MBEDTLS_PRIVATE(d)),
        mbedtls_ctr_drbg_random,
        context->session->ctr_drbg);
#else
    ret = mbedtls_ecdh_compute_shared(&pEcpPrv->grp,
        &rawSharedData,
        &(pEcpExt->Q),
        &(pEcpPrv->d),
        mbedtls_ctr_drbg_random,
        context->session->ctr_drbg);
#endif
#endif
    if (ret != 0) {
        LOG_E("mbedtls_ecdh_compute_shared returned -0x%04x", -ret);
        retval = kStatus_SSS_Fail;
        goto exit;
    }
    sharedSecretLen_Derived = mbedtls_mpi_size(&rawSharedData);
    if (sharedSecretLen_Derived > sharedSecretLen) {
        LOG_E("Failed: Incorrect shared key length");
        mbedtls_mpi_free(&rawSharedData);
        retval = kStatus_SSS_Fail;
        goto exit;
    }

    derivedKeyObject->contents_size = keyLen;
    ret = mbedtls_mpi_write_binary(&rawSharedData, derivedKeyObject->contents, derivedKeyObject->contents_size);
    if (ret != 0) {
        LOG_E("Failed: unable to write shared key");
        retval = kStatus_SSS_Fail;
        goto exit;
    }
    mbedtls_mpi_free(&rawSharedData);
#ifdef MBEDTLS_DO_LITTLE_ENDIAN
    if (otherPartyKeyObject->cipherType == kSSS_CipherType_EC_MONTGOMERY) {
        // Change Endianness Shared Secret in case of Montgomery Curve
        uint8_t *pVal = (uint8_t *)derivedKeyObject->contents;
        for (size_t keyValueIdx = 0; keyValueIdx < (derivedKeyObject->contents_size >> 1); keyValueIdx++) {
            uint8_t swapByte  = pVal[keyValueIdx];
            pVal[keyValueIdx] = pVal[derivedKeyObject->contents_size - 1 - keyValueIdx];
            pVal[derivedKeyObject->contents_size - 1 - keyValueIdx] = swapByte;
        }
    }
#endif
exit:
    return retval;
#else
    return kStatus_SSS_Fail;
#endif
}

void sss_mbedtls_derive_key_context_free(sss_mbedtls_derive_key_t *context)
{
    memset(context, 0, sizeof(*context));
}

/* End: mbedtls_keyderive */

/* ************************************************************************** */
/* Functions : sss_mbedtls_keystore                                           */
/* ************************************************************************** */

sss_status_t sss_mbedtls_key_store_context_init(sss_mbedtls_key_store_t *keyStore, sss_mbedtls_session_t *session)
{
    sss_status_t retval = kStatus_SSS_Fail;
    ENSURE_OR_GO_CLEANUP(keyStore);
    ENSURE_OR_GO_CLEANUP(session);

    memset(keyStore, 0, sizeof(*keyStore));
    keyStore->session = session;
    retval            = kStatus_SSS_Success;
cleanup:
    return retval;
}

sss_status_t sss_mbedtls_key_store_allocate(sss_mbedtls_key_store_t *keyStore, uint32_t keyStoreId)
{
    sss_status_t retval = kStatus_SSS_Fail;
    AX_UNUSED_ARG(keyStoreId);
    ENSURE_OR_GO_CLEANUP(keyStore);
    ENSURE_OR_GO_CLEANUP(keyStore->session);

#if defined(MBEDTLS_FS_IO) && !AX_EMBEDDED
    /* This function is called once per session so keystore
    object and shadow objects Should be equal to Null */
    ENSURE_OR_GO_CLEANUP(keyStore->objects == NULL);
    ENSURE_OR_GO_CLEANUP(keyStore->keystore_shadow == NULL);

    keyStore->max_object_count = MAX_KEY_OBJ_COUNT;
    keyStore->objects = (sss_mbedtls_object_t **)SSS_MALLOC(MAX_KEY_OBJ_COUNT * sizeof(sss_mbedtls_object_t *));
    ENSURE_OR_GO_CLEANUP(keyStore->objects != NULL);
    memset(keyStore->objects, 0, (MAX_KEY_OBJ_COUNT * sizeof(sss_mbedtls_object_t *)));
    ks_sw_fat_allocate(&keyStore->keystore_shadow);
    if (keyStore->session->szRootPath != NULL) {
        ks_sw_fat_load(keyStore->session->szRootPath, keyStore->keystore_shadow);
    }
    retval = kStatus_SSS_Success;

#else
    retval = kStatus_SSS_Success;
#endif
cleanup:
    return retval;
}

sss_status_t sss_mbedtls_key_store_save(sss_mbedtls_key_store_t *keyStore)
{
    sss_status_t retval = kStatus_SSS_Fail;

#if defined(MBEDTLS_FS_IO) && !AX_EMBEDDED
    uint32_t i;
    ENSURE_OR_GO_CLEANUP(keyStore);
    ENSURE_OR_GO_CLEANUP(keyStore->session);
    ENSURE_OR_GO_CLEANUP(keyStore->session->szRootPath)
    ENSURE_OR_GO_CLEANUP(keyStore->objects)
    for (i = 0; i < keyStore->max_object_count; i++) {
        if (NULL != keyStore->objects[i]) {
            ks_mbedtls_store_key(keyStore->objects[i]);
        }
    }
    retval = ks_mbedtls_fat_update(keyStore);
cleanup:
#endif
    return retval;
}

sss_status_t sss_mbedtls_key_store_load(sss_mbedtls_key_store_t *keyStore)
{
    sss_status_t retval = kStatus_SSS_Fail;
    ENSURE_OR_GO_CLEANUP(keyStore);
    ENSURE_OR_GO_CLEANUP(keyStore->session);
#if defined(MBEDTLS_FS_IO) && !AX_EMBEDDED
    if (keyStore->objects == NULL) {
        retval = sss_mbedtls_key_store_allocate(keyStore, 0);
        if (retval != kStatus_SSS_Success) {
            return kStatus_SSS_Fail;
        }
    }
    if (keyStore->session->szRootPath) {
        if (NULL == keyStore->keystore_shadow) {
            ks_sw_fat_allocate(&keyStore->keystore_shadow);
        }
        retval                     = ks_sw_fat_load(keyStore->session->szRootPath, keyStore->keystore_shadow);
        keyStore->max_object_count = keyStore->keystore_shadow->maxEntries;
    }
#endif
cleanup:
    return retval;
}

sss_status_t sss_mbedtls_key_store_set_key(sss_mbedtls_key_store_t *keyStore,
    sss_mbedtls_object_t *keyObject,
    const uint8_t *data,
    size_t dataLen,
    size_t keyBitLen,
    void *options,
    size_t optionsLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
    AX_UNUSED_ARG(keyStore);
    AX_UNUSED_ARG(options);
    AX_UNUSED_ARG(optionsLen);
    //mbedtls_pk_context *pk = NULL;
    //size_t keyByteLen = keyBitLen / 8;
    ENSURE_OR_GO_CLEANUP(keyObject);
    ENSURE_OR_GO_CLEANUP(keyObject->contents);

    ENSURE_OR_GO_CLEANUP((keyObject->accessRights & kAccessPermission_SSS_Write));
    //pk = (mbedtls_pk_context *)keyObject->contents;
    retval = sss_mbedtls_set_key(keyObject, data, dataLen, keyBitLen);
cleanup:
    return retval;
}

sss_status_t sss_mbedtls_key_store_generate_key(
    sss_mbedtls_key_store_t *keyStore, sss_mbedtls_object_t *keyObject, size_t keyBitLen, void *options)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSS_HAVE_TESTCOUNTERPART && (SSSFTR_SW_ECC || SSSFTR_SW_RSA)
    sss_mbedtls_session_t *pS = NULL;
    mbedtls_pk_context *pkey;
    sss_key_part_t key_part       = kSSS_KeyPart_NONE;
    sss_cipher_type_t cipher_type = kSSS_CipherType_NONE;
    ENSURE_OR_GO_CLEANUP(keyStore);
    ENSURE_OR_GO_CLEANUP(keyObject);
    ENSURE_OR_GO_CLEANUP(keyObject->contents); /* Must be allocated in allocate handle */

    AX_UNUSED_ARG(options);

    pS          = keyStore->session;
    key_part    = (sss_key_part_t)keyObject->objectType;
    cipher_type = (sss_cipher_type_t)keyObject->cipherType;

    pkey = (mbedtls_pk_context *)keyObject->contents;
    if (key_part != kSSS_KeyPart_Pair) {
        retval = kStatus_SSS_Success;
        goto cleanup;
    }

    mbedtls_pk_init(pkey);
    switch (cipher_type) {
#if SSSFTR_SW_ECC && defined(MBEDTLS_ECP_C)
    case kSSS_CipherType_EC_NIST_P:
    case kSSS_CipherType_EC_NIST_K:
    case kSSS_CipherType_EC_BRAINPOOL:
    case kSSS_CipherType_EC_MONTGOMERY:
        retval = sss_mbedtls_generate_ecp_key(pkey, pS, keyBitLen, cipher_type);
        break;
#endif
#if SSSFTR_SW_RSA && defined(MBEDTLS_RSA_C)
    case kSSS_CipherType_RSA:
        retval = sss_mbedtls_generate_rsa_key(pkey, pS, keyBitLen);
        break;
#endif
    default:
        break;
    }
cleanup:
#endif
    return retval;
}

sss_status_t sss_mbedtls_key_store_get_key(sss_mbedtls_key_store_t *keyStore,
    sss_mbedtls_object_t *keyObject,
    uint8_t *data,
    size_t *dataLen,
    size_t *pKeyBitLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSSFTR_SW_RSA || SSSFTR_SW_ECC
    mbedtls_pk_context *pk = NULL;
    int ret                = -1;
    uint8_t output[1600]   = {0};
    unsigned char *c       = output;
#endif

    AX_UNUSED_ARG(keyStore);

    ENSURE_OR_GO_CLEANUP(keyObject);
    ENSURE_OR_GO_CLEANUP((keyObject->accessRights & kAccessPermission_SSS_Read));
    ENSURE_OR_GO_CLEANUP(data);
    ENSURE_OR_GO_CLEANUP(dataLen);

    switch (keyObject->objectType) {
    case kSSS_KeyPart_Default:
        ENSURE_OR_GO_CLEANUP(*dataLen >= keyObject->contents_size);
        memcpy(data, keyObject->contents, keyObject->contents_size);
        *dataLen = keyObject->contents_size;
        if (pKeyBitLen != NULL) {
            *pKeyBitLen = keyObject->contents_size * 8;
        }
        retval = kStatus_SSS_Success;
        break;
#if SSSFTR_SW_RSA || SSSFTR_SW_ECC
    case kSSS_KeyPart_Public:
    case kSSS_KeyPart_Pair:
        pk = (mbedtls_pk_context *)keyObject->contents;
        if (keyObject->cipherType == kSSS_CipherType_EC_MONTGOMERY) {
#if defined(MBEDTLS_ECP_C)
            mbedtls_ecp_keypair *pEcpPub = mbedtls_pk_ec(*pk);
            ENSURE_OR_GO_CLEANUP(pEcpPub);
            size_t pubKey_size = 0;
            size_t header_size = 0;

#if SSS_HAVE_MBEDTLS_3_X
            if (pEcpPub->MBEDTLS_PRIVATE(grp).id == MBEDTLS_ECP_DP_CURVE25519) {
#else
            if (pEcpPub->grp.id == MBEDTLS_ECP_DP_CURVE25519) {
#endif
                pubKey_size = 32;
                if (pKeyBitLen != NULL) {
                    *pKeyBitLen = 256;
                }
                header_size = der_ecc_mont_dh_25519_header_len;
                ENSURE_OR_GO_CLEANUP(*dataLen >= (pubKey_size + header_size));
                memcpy(data, gecc_der_header_mont_dh_25519, header_size);
            }
#if SSS_HAVE_MBEDTLS_3_X
            else if (pEcpPub->MBEDTLS_PRIVATE(grp).id == MBEDTLS_ECP_DP_CURVE448) {
#else
            else if (pEcpPub->grp.id == MBEDTLS_ECP_DP_CURVE448) {
#endif
                pubKey_size = 56;
                if (pKeyBitLen != NULL) {
                    *pKeyBitLen = 448;
                }
                header_size = der_ecc_mont_dh_448_header_len;
                ENSURE_OR_GO_CLEANUP((SIZE_MAX - pubKey_size) >= header_size);
                ENSURE_OR_GO_CLEANUP(*dataLen >= (pubKey_size + header_size));
                memcpy(data, gecc_der_header_mont_dh_448, header_size);
            }
            else {
                LOG_E(
                    "Only mont_dh_25519 (bit length 256) and mont_dh_448 (bit "
                    "length 448)");
                goto cleanup;
            }
#if SSS_HAVE_MBEDTLS_3_X
            ret = mbedtls_mpi_write_binary(&pEcpPub->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(X), output, pubKey_size);
#else
            ret = mbedtls_mpi_write_binary(&pEcpPub->Q.X, output, pubKey_size);
#endif
            ENSURE_OR_GO_CLEANUP(0 == ret);
            *dataLen = pubKey_size + header_size;
#ifdef MBEDTLS_DO_LITTLE_ENDIAN
            /* Reverse the public key */
            {
                size_t i = 0;
                while (i < pubKey_size) {
                    data[i + header_size] = output[pubKey_size - i - 1];
                    i++;
                }
            }
#else
            memcpy(data, output, pubKey_size);
#endif
            retval = kStatus_SSS_Success;
#else
            retval = kStatus_SSS_Fail;
#endif
        }
        else {
            ret = mbedtls_pk_write_pubkey_der(pk, output, sizeof(output));
            if (ret > 0) {
                if ((*dataLen) >= (size_t)ret) {
                    if (pKeyBitLen != NULL) {
                        *pKeyBitLen = mbedtls_pk_get_bitlen(pk);
                    }
                    *dataLen = ret;
                    /* Data is put at end, so copy it to front of output buffer */
                    c = output + sizeof(output) - ret;
                    memcpy(data, c, ret);
                    retval = kStatus_SSS_Success;
                }
            }
        }
        break;
#endif // SSSFTR_SW_RSA || SSSFTR_SW_ECC
    default:
        break;
    }
cleanup:
    return retval;
}

sss_status_t sss_mbedtls_key_store_open_key(sss_mbedtls_key_store_t *keyStore, sss_mbedtls_object_t *keyObject)
{
    sss_status_t retval = kStatus_SSS_Success;
    AX_UNUSED_ARG(keyStore);
    AX_UNUSED_ARG(keyObject);
    return retval;
}

sss_status_t sss_mbedtls_key_store_freeze_key(sss_mbedtls_key_store_t *keyStore, sss_mbedtls_object_t *keyObject)
{
    sss_status_t retval = kStatus_SSS_Success;
    AX_UNUSED_ARG(keyStore);
    AX_UNUSED_ARG(keyObject);
    return retval;
}

sss_status_t sss_mbedtls_key_store_erase_key(sss_mbedtls_key_store_t *keyStore, sss_mbedtls_object_t *keyObject)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSS_HAVE_TESTCOUNTERPART
    ENSURE_OR_GO_EXIT(keyStore);
    ENSURE_OR_GO_EXIT(keyObject);
    ENSURE_OR_GO_EXIT(keyObject->keyStore);

    ENSURE_OR_GO_EXIT((keyObject->accessRights & kAccessPermission_SSS_Delete));

    if (keyObject->keyMode == kKeyObject_Mode_Persistent) {
#if defined(MBEDTLS_FS_IO) && !AX_EMBEDDED
        unsigned int i = 0;
        /* first check if key exists delete key from shadow KS*/
        retval = ks_common_remove_fat(keyObject->keyStore->keystore_shadow, keyObject->keyId);
        ENSURE_OR_GO_CLEANUP(retval == kStatus_SSS_Success);

        /* Update shadow keystore in file system*/
        retval = ks_mbedtls_fat_update(keyObject->keyStore);
        ENSURE_OR_GO_CLEANUP(retval == kStatus_SSS_Success);

        /*Clear key object from file*/
        retval = ks_mbedtls_remove_key(keyObject);

        for (i = 0; i < keyObject->keyStore->max_object_count; i++) {
            if (keyObject->keyStore->objects[i] == keyObject) {
                keyObject->keyStore->objects[i] = NULL;
                break;
            }
        }
#endif
    }
    else {
        retval = kStatus_SSS_Success;
    }

#if defined(MBEDTLS_FS_IO) && !AX_EMBEDDED
cleanup:
#endif
exit:
#endif
    return retval;
}

void sss_mbedtls_key_store_context_free(sss_mbedtls_key_store_t *keyStore)
{
#if defined(MBEDTLS_FS_IO) && !AX_EMBEDDED
    if (NULL != keyStore->objects) {
        uint32_t i;
        for (i = 0; i < keyStore->max_object_count; i++) {
            if (keyStore->objects[i] != NULL) {
                //sss_mbedtls_key_object_free(keyStore->objects[i]);
                keyStore->objects[i] = NULL;
            }
        }
        SSS_FREE(keyStore->objects);
        keyStore->objects = NULL;
    }
    if (NULL != keyStore->keystore_shadow) {
        ks_sw_fat_free(keyStore->keystore_shadow);
    }
#endif
    memset(keyStore, 0, sizeof(*keyStore));
}

/* End: mbedtls_keystore */

/* ************************************************************************** */
/* Functions : sss_mbedtls_asym                                               */
/* ************************************************************************** */

sss_status_t sss_mbedtls_asymmetric_context_init(sss_mbedtls_asymmetric_t *context,
    sss_mbedtls_session_t *session,
    sss_mbedtls_object_t *keyObject,
    sss_algorithm_t algorithm,
    sss_mode_t mode)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSSFTR_SW_ECC || SSSFTR_SW_RSA
    ENSURE_OR_GO_CLEANUP(context);
    ENSURE_OR_GO_CLEANUP(keyObject);
    ENSURE_OR_GO_CLEANUP(keyObject->keyStore->session->subsystem == kType_SSS_mbedTLS);

    context->session   = session;
    context->keyObject = keyObject;
    context->algorithm = algorithm;
    context->mode      = mode;
    retval             = kStatus_SSS_Success;
cleanup:
#endif
    return retval;
}

sss_status_t sss_mbedtls_asymmetric_encrypt(
    sss_mbedtls_asymmetric_t *context, const uint8_t *srcData, size_t srcLen, uint8_t *destData, size_t *destLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSSFTR_SW_RSA && defined(MBEDTLS_RSA_C)
    int ret;
    sss_mbedtls_object_t *keyObj = context->keyObject;
    sss_mbedtls_session_t *pS    = context->session;
    mbedtls_pk_context *pKey     = (mbedtls_pk_context *)keyObj->contents;
    sss_algorithm_t algo         = context->algorithm;
    mbedtls_rsa_context *rsa;
    ENSURE_OR_GO_EXIT((context->keyObject->accessRights & kAccessPermission_SSS_Use));
    retval = kStatus_SSS_Success;

    switch (algo) {
    case kAlgorithm_SSS_RSAES_PKCS1_V1_5:
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V15, 0);
        break;
    case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA1:
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA1);
        break;
    case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA224:
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA224);
        break;
    case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA256:
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
        break;
    case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA384:
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA384);
        break;
    case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA512:
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA512);
        break;
    default:
        retval = kStatus_SSS_Fail;
        goto exit;
    }
    ret = mbedtls_pk_encrypt(pKey, srcData, srcLen, destData, destLen, *destLen, mbedtls_ctr_drbg_random, pS->ctr_drbg);
    retval = kStatus_SSS_Fail;
    ENSURE_OR_GO_EXIT(ret == 0);
    rsa = mbedtls_pk_rsa(*pKey);
    ENSURE_OR_GO_EXIT(rsa);
    retval = kStatus_SSS_Success;
#if SSS_HAVE_MBEDTLS_3_X
    *destLen = rsa->MBEDTLS_PRIVATE(len);
#else
    *destLen = rsa->len;
#endif
exit:
#endif
    return retval;
}

sss_status_t sss_mbedtls_asymmetric_decrypt(
    sss_mbedtls_asymmetric_t *context, const uint8_t *srcData, size_t srcLen, uint8_t *destData, size_t *destLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSSFTR_SW_RSA && defined(MBEDTLS_RSA_C)
    int ret;
    sss_mbedtls_object_t *keyObj = context->keyObject;
    sss_mbedtls_session_t *pS    = context->session;
    mbedtls_pk_context *pKey;
    sss_algorithm_t algo = context->algorithm;
    retval               = kStatus_SSS_Success;
    ENSURE_OR_GO_EXIT((context->keyObject->accessRights & kAccessPermission_SSS_Use));

    pKey = (mbedtls_pk_context *)keyObj->contents;

    switch (algo) {
    case kAlgorithm_SSS_RSAES_PKCS1_V1_5:
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V15, 0);
        break;
    case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA1:
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA1);
        break;
    case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA224:
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA224);
        break;
    case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA256:
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
        break;
    case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA384:
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA384);
        break;
    case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA512:
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA512);
        break;
    default:
        retval = kStatus_SSS_Fail;
        goto exit;
    }

    ret = mbedtls_pk_decrypt(pKey, srcData, srcLen, destData, destLen, *destLen, mbedtls_ctr_drbg_random, pS->ctr_drbg);

    retval = kStatus_SSS_Fail;
    ENSURE_OR_GO_EXIT(ret == 0);
    retval = kStatus_SSS_Success;

exit:
#endif
    return retval;
}

#if SSSFTR_SW_ECC || SSSFTR_SW_RSA
static mbedtls_md_type_t sss_mbedtls_set_padding_get_hash(sss_algorithm_t algorithm, mbedtls_pk_context *pKey)
{
    mbedtls_md_type_t md_alg = MBEDTLS_MD_NONE;
    switch (algorithm) {
    case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA1:
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA1:
    case kAlgorithm_SSS_ECDSA_SHA1:
    case kAlgorithm_SSS_SHA1: {
        md_alg = MBEDTLS_MD_SHA1;
    } break;
    case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA224:
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA224:
    case kAlgorithm_SSS_ECDSA_SHA224:
    case kAlgorithm_SSS_SHA224: {
        md_alg = MBEDTLS_MD_SHA224;
    } break;
    case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA256:
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA256:
    case kAlgorithm_SSS_ECDSA_SHA256:
    case kAlgorithm_SSS_SHA256: {
        md_alg = MBEDTLS_MD_SHA256;
    } break;
    case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA384:
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA384:
    case kAlgorithm_SSS_ECDSA_SHA384:
    case kAlgorithm_SSS_SHA384: {
        md_alg = MBEDTLS_MD_SHA384;
    } break;
    case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA512:
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA512:
    case kAlgorithm_SSS_ECDSA_SHA512:
    case kAlgorithm_SSS_SHA512: {
        md_alg = MBEDTLS_MD_SHA512;
    } break;
    default:
        md_alg = MBEDTLS_MD_NONE;
        break;
    }

#if defined(MBEDTLS_RSA_C)
    if (algorithm >= kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA1 &&
        algorithm <= kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA512) {
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V21, md_alg);
    }
    else if ((algorithm >= kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA1 &&
                 algorithm <= kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA512) ||
             algorithm == kAlgorithm_SSS_RSASSA_PKCS1_V1_5_NO_HASH) {
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*pKey), MBEDTLS_RSA_PKCS_V15, md_alg);
    }
#endif

    return md_alg;
}
#endif

sss_status_t sss_mbedtls_asymmetric_sign_digest(sss_mbedtls_asymmetric_t *context,
    const uint8_t *digest,
    size_t digestLen,
    uint8_t *signature,
    size_t *signatureLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSSFTR_SW_ECC || SSSFTR_SW_RSA
    int ret                  = 1;
    mbedtls_md_type_t md_alg = MBEDTLS_MD_NONE;
    sss_mbedtls_session_t *pS;
    mbedtls_pk_context *pKey;

    ENSURE_OR_GO_EXIT((context->keyObject->accessRights & kAccessPermission_SSS_Use));

    pS   = context->session;
    pKey = (mbedtls_pk_context *)context->keyObject->contents;

    md_alg = sss_mbedtls_set_padding_get_hash(context->algorithm, pKey);

#if SSS_HAVE_MBEDTLS_3_X
    ret = mbedtls_pk_sign(
        pKey, md_alg, digest, digestLen, signature, *signatureLen, signatureLen, mbedtls_ctr_drbg_random, pS->ctr_drbg);
#else
    ret = mbedtls_pk_sign(
        pKey, md_alg, digest, digestLen, signature, signatureLen, mbedtls_ctr_drbg_random, pS->ctr_drbg);
#endif

    ENSURE_OR_GO_EXIT(ret == 0);

    retval = kStatus_SSS_Success;
exit:
#endif
    return retval;
}

sss_status_t sss_mbedtls_asymmetric_verify_digest(sss_mbedtls_asymmetric_t *context,
    const uint8_t *digest,
    size_t digestLen,
    const uint8_t *signature,
    size_t signatureLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSSFTR_SW_ECC || SSSFTR_SW_RSA
    int ret                  = 1;
    mbedtls_md_type_t md_alg = MBEDTLS_MD_NONE;
    mbedtls_pk_context *pKey;

    ENSURE_OR_GO_EXIT((context->keyObject->accessRights & kAccessPermission_SSS_Use));

    pKey = (mbedtls_pk_context *)context->keyObject->contents;

    md_alg = sss_mbedtls_set_padding_get_hash(context->algorithm, pKey);

    ret = mbedtls_pk_verify(pKey, md_alg, digest, digestLen, signature, signatureLen);

    ENSURE_OR_GO_EXIT(ret == 0);

    retval = kStatus_SSS_Success;
exit:
#endif
    return retval;
}

void sss_mbedtls_asymmetric_context_free(sss_mbedtls_asymmetric_t *context)
{
    memset(context, 0, sizeof(*context));
}

/* End: mbedtls_asym */

/* ************************************************************************** */
/* Functions : sss_mbedtls_symm                                               */
/* ************************************************************************** */

sss_status_t sss_mbedtls_symmetric_context_init(sss_mbedtls_symmetric_t *context,
    sss_mbedtls_session_t *session,
    sss_mbedtls_object_t *keyObject,
    sss_algorithm_t algorithm,
    sss_mode_t mode)
{
    sss_status_t retval = kStatus_SSS_Success;

    if (context == NULL) {
        return kStatus_SSS_Fail;
    }
    context->session   = session;
    context->keyObject = keyObject;
    context->algorithm = algorithm;
    context->mode      = mode;

    return retval;
}

sss_status_t sss_mbedtls_cipher_one_go(sss_mbedtls_symmetric_t *context,
    uint8_t *iv,
    size_t ivLen,
    const uint8_t *srcData,
    uint8_t *destData,
    size_t dataLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
    mbedtls_aes_context aes_ctx;
#if SSS_HAVE_TESTCOUNTERPART
    size_t i = 0;
#endif
#if defined(MBEDTLS_DES_C)
    mbedtls_des_context des_ctx;
    mbedtls_des3_context des3_ctx;
#endif
    int mbedtls_ret = 1; /* Fail by default */

    /* Use the copy of IV, as it gets overwritten in the mbedtls_aes_crypt_* functions */
    uint8_t iv_copy[16] = {0};
    if (iv != NULL) {
        ENSURE_OR_GO_EXIT(ivLen <= 16);
        memcpy(iv_copy, iv, ivLen);
    }
    AX_UNUSED_ARG(ivLen);

    switch (context->algorithm) {
#if SSS_HAVE_TESTCOUNTERPART
    case kAlgorithm_SSS_AES_ECB:
#endif //SSS_HAVE_TESTCOUNTERPART
    case kAlgorithm_SSS_AES_CBC:
        mbedtls_aes_init(&aes_ctx);
        ENSURE_OR_GO_EXIT(context->keyObject->contents_size <= (SIZE_MAX / 8));
        if (context->mode == kMode_SSS_Encrypt) {
            mbedtls_ret = mbedtls_aes_setkey_enc(
                &aes_ctx, context->keyObject->contents, (unsigned int)(context->keyObject->contents_size * 8));
        }
        else if (context->mode == kMode_SSS_Decrypt) {
            mbedtls_ret = mbedtls_aes_setkey_dec(
                &aes_ctx, context->keyObject->contents, (unsigned int)(context->keyObject->contents_size * 8));
        }
        break;
#if SSS_HAVE_TESTCOUNTERPART
    case kAlgorithm_SSS_AES_CTR: {
        mbedtls_aes_init(&aes_ctx);
        ENSURE_OR_GO_EXIT(context->keyObject->contents_size <= (SIZE_MAX / 8));
        mbedtls_ret = mbedtls_aes_setkey_enc(
            &aes_ctx, context->keyObject->contents, (unsigned int)(context->keyObject->contents_size * 8));
    } break;
#if defined(MBEDTLS_DES_C)
    case kAlgorithm_SSS_DES_CBC:
    case kAlgorithm_SSS_DES_ECB:
        mbedtls_des_init(&des_ctx);
        if (context->mode == kMode_SSS_Encrypt) {
            mbedtls_ret = mbedtls_des_setkey_enc(&des_ctx, context->keyObject->contents);
        }
        else if (context->mode == kMode_SSS_Decrypt) {
            mbedtls_ret = mbedtls_des_setkey_dec(&des_ctx, context->keyObject->contents);
        }
        break;
    case kAlgorithm_SSS_DES3_CBC:
    case kAlgorithm_SSS_DES3_ECB: {
        mbedtls_des3_init(&des3_ctx);
        if (context->mode == kMode_SSS_Encrypt) {
            mbedtls_ret = mbedtls_des3_set3key_enc(&des3_ctx, context->keyObject->contents);
        }
        else if (context->mode == kMode_SSS_Decrypt) {
            mbedtls_ret = mbedtls_des3_set3key_dec(&des3_ctx, context->keyObject->contents);
        }
        break;
    }
#endif // MBEDTLS_DES_C
#endif //SSS_HAVE_TESTCOUNTERPART
    default:
        goto exit;
    }

    ENSURE_OR_GO_EXIT(mbedtls_ret == 0);
    mbedtls_ret = 1; // Reinitializing

    if (context->mode == kMode_SSS_Encrypt) {
        switch (context->algorithm) {
#if SSS_HAVE_TESTCOUNTERPART
        case kAlgorithm_SSS_AES_ECB:
            do {
                mbedtls_ret = mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, srcData + i, destData + i);
                ENSURE_OR_GO_EXIT(i <= SIZE_MAX - CIPHER_BLOCK_SIZE);
                i += CIPHER_BLOCK_SIZE;
            } while (i < dataLen);
            break;
#endif //SSS_HAVE_TESTCOUNTERPART
#if defined(MBEDTLS_CIPHER_MODE_CBC)
        case kAlgorithm_SSS_AES_CBC:
            mbedtls_ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, dataLen, iv_copy, srcData, destData);
            break;
#endif // MBEDTLS_CIPHER_MODE_CBC
#if defined(MBEDTLS_CIPHER_MODE_CTR)
        case kAlgorithm_SSS_AES_CTR: {
            uint8_t stream_block[16] = {
                0,
            };
            size_t size_left = 0;
            mbedtls_ret =
                mbedtls_aes_crypt_ctr(&aes_ctx, dataLen, &size_left, iv_copy, stream_block, srcData, destData);
        } break;
#endif // MBEDTLS_CIPHER_MODE_CTR
#if defined(MBEDTLS_DES_C)
        case kAlgorithm_SSS_DES_ECB:
            mbedtls_ret = mbedtls_des_crypt_ecb(&des_ctx, srcData, destData);
            break;
#if defined(MBEDTLS_CIPHER_MODE_CBC)
        case kAlgorithm_SSS_DES_CBC:
            mbedtls_ret = mbedtls_des_crypt_cbc(&des_ctx, MBEDTLS_DES_ENCRYPT, dataLen, iv_copy, srcData, destData);
            break;
#endif // MBEDTLS_CIPHER_MODE_CBC
        case kAlgorithm_SSS_DES3_ECB:
            mbedtls_ret = mbedtls_des3_crypt_ecb(&des3_ctx, srcData, destData);
            break;
#if defined(MBEDTLS_CIPHER_MODE_CBC)
        case kAlgorithm_SSS_DES3_CBC:
            mbedtls_ret = mbedtls_des3_crypt_cbc(&des3_ctx, MBEDTLS_DES_ENCRYPT, dataLen, iv_copy, srcData, destData);
            break;
#endif // MBEDTLS_CIPHER_MODE_CBC
#endif
        default:
            break;
        }
    }
    else if (context->mode == kMode_SSS_Decrypt) {
        switch (context->algorithm) {
#if defined(MBEDTLS_CIPHER_MODE_CBC)
        case kAlgorithm_SSS_AES_CBC:
            mbedtls_ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, dataLen, iv_copy, srcData, destData);
            break;
#endif // MBEDTLS_CIPHER_MODE_CBC
#if SSS_HAVE_TESTCOUNTERPART
        case kAlgorithm_SSS_AES_ECB:
            do {
                mbedtls_ret = mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, srcData + i, destData + i);
                ENSURE_OR_GO_EXIT(i <= SIZE_MAX - CIPHER_BLOCK_SIZE);
                i += CIPHER_BLOCK_SIZE;
            } while (i < dataLen);
            break;
#if defined(MBEDTLS_CIPHER_MODE_CTR)
        case kAlgorithm_SSS_AES_CTR: {
            uint8_t stream_block[16] = {
                0,
            };
            size_t size_left = 0;
            mbedtls_ret =
                mbedtls_aes_crypt_ctr(&aes_ctx, dataLen, &size_left, iv_copy, stream_block, srcData, destData);
        } break;
#endif //MBEDTLS_CIPHER_MODE_CTR
#endif //SSS_HAVE_TESTCOUNTERPART
#if defined(MBEDTLS_DES_C)
        case kAlgorithm_SSS_DES_ECB:
            mbedtls_ret = mbedtls_des_crypt_ecb(&des_ctx, srcData, destData);
            break;
#if defined(MBEDTLS_CIPHER_MODE_CBC)
        case kAlgorithm_SSS_DES_CBC:
            mbedtls_ret = mbedtls_des_crypt_cbc(&des_ctx, MBEDTLS_DES_DECRYPT, dataLen, iv_copy, srcData, destData);
            break;
#endif // MBEDTLS_CIPHER_MODE_CBC
        case kAlgorithm_SSS_DES3_ECB:
            mbedtls_ret = mbedtls_des3_crypt_ecb(&des3_ctx, srcData, destData);
            break;
#if defined(MBEDTLS_CIPHER_MODE_CBC)
        case kAlgorithm_SSS_DES3_CBC:
            mbedtls_ret = mbedtls_des3_crypt_cbc(&des3_ctx, MBEDTLS_DES_DECRYPT, dataLen, iv_copy, srcData, destData);
            break;
#endif // MBEDTLS_CIPHER_MODE_CBC
#endif
        default:
            break;
        }
    }
    else {
        goto exit;
    }

    ENSURE_OR_GO_EXIT(mbedtls_ret == 0);

    switch (context->algorithm) {
#if SSS_HAVE_TESTCOUNTERPART
    case kAlgorithm_SSS_AES_ECB:
    case kAlgorithm_SSS_AES_CTR:
#endif //SSS_HAVE_TESTCOUNTERPART
    case kAlgorithm_SSS_AES_CBC:
        mbedtls_aes_free(&aes_ctx);
        break;
#if SSS_HAVE_TESTCOUNTERPART
#if defined(MBEDTLS_DES_C)
    case kAlgorithm_SSS_DES_CBC:
    case kAlgorithm_SSS_DES_ECB:
        mbedtls_des_free(&des_ctx);
        break;
    case kAlgorithm_SSS_DES3_CBC:
    case kAlgorithm_SSS_DES3_ECB: {
        mbedtls_des3_free(&des3_ctx);
        break;
    }
#endif // MBEDTLS_DES_C
#endif //SSS_HAVE_TESTCOUNTERPART
    default:
        goto exit;
    }

    retval = kStatus_SSS_Success;
exit:
    return retval;
}

sss_status_t sss_mbedtls_cipher_one_go_v2(sss_mbedtls_symmetric_t *context,
    uint8_t *iv,
    size_t ivLen,
    const uint8_t *srcData,
    const size_t srcLen,
    uint8_t *destData,
    size_t *pDataLen)
{
    if (pDataLen == NULL) {
        return kStatus_SSS_Fail;
    }
    if (*pDataLen < srcLen) {
        return kStatus_SSS_Fail;
    }
    *pDataLen = srcLen;
    return sss_mbedtls_cipher_one_go(context, iv, ivLen, srcData, destData, *pDataLen);
}

sss_status_t sss_mbedtls_cipher_init(sss_mbedtls_symmetric_t *context, uint8_t *iv, size_t ivLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSS_HAVE_TESTCOUNTERPART
    const mbedtls_cipher_info_t *cipher_info = NULL;
    mbedtls_cipher_type_t cipher_type        = MBEDTLS_CIPHER_NONE;
    int ret                                  = -1;
    retval                                   = kStatus_SSS_Success;
    context->cipher_ctx = (mbedtls_cipher_context_t *)SSS_CALLOC(1, sizeof(mbedtls_cipher_context_t));
    ENSURE_OR_GO_EXIT(context->cipher_ctx != NULL);

    if (context->algorithm == kAlgorithm_SSS_AES_ECB) {
        switch (context->keyObject->keyBitLen) {
        case 128:
            cipher_type = MBEDTLS_CIPHER_AES_128_ECB;
            break;
        case 192:
            cipher_type = MBEDTLS_CIPHER_AES_192_ECB;
            break;
        case 256:
            cipher_type = MBEDTLS_CIPHER_AES_256_ECB;
            break;
        default:
            LOG_E("Invalid kAlgorithm_SSS_AES_ECB keyBitLen");
            retval = kStatus_SSS_Fail;
            goto exit;
        }
    }
    else if (context->algorithm == kAlgorithm_SSS_AES_CBC) {
        switch (context->keyObject->keyBitLen) {
        case 128:
            cipher_type = MBEDTLS_CIPHER_AES_128_CBC;
            break;
        case 192:
            cipher_type = MBEDTLS_CIPHER_AES_192_CBC;
            break;
        case 256:
            cipher_type = MBEDTLS_CIPHER_AES_256_CBC;
            break;
        default:
            LOG_E("Invalid kAlgorithm_SSS_AES_CBC keyBitLen");
            retval = kStatus_SSS_Fail;
            goto exit;
        }
    }
    else if (context->algorithm == kAlgorithm_SSS_AES_CTR) {
        switch (context->keyObject->keyBitLen) {
        case 128:
            cipher_type = MBEDTLS_CIPHER_AES_128_CTR;
            break;
        case 192:
            cipher_type = MBEDTLS_CIPHER_AES_192_CTR;
            break;
        case 256:
            cipher_type = MBEDTLS_CIPHER_AES_256_CTR;
            break;
        default:
            LOG_E("Invalid kAlgorithm_SSS_AES_CTR keyBitLen");
            retval = kStatus_SSS_Fail;
            goto exit;
        }
    }
    else if (context->algorithm == kAlgorithm_SSS_DES_ECB) {
        cipher_type = MBEDTLS_CIPHER_DES_ECB;
    }
    else if (context->algorithm == kAlgorithm_SSS_DES3_ECB) {
        cipher_type = MBEDTLS_CIPHER_DES_EDE3_ECB;
    }
    else if (context->algorithm == kAlgorithm_SSS_DES_CBC) {
        cipher_type = MBEDTLS_CIPHER_DES_CBC;
    }
    else if (context->algorithm == kAlgorithm_SSS_DES3_CBC) {
        cipher_type = MBEDTLS_CIPHER_DES_EDE3_CBC;
    }
    else {
        retval = kStatus_SSS_InvalidArgument;
        SSS_FREE(context->cipher_ctx);
        goto exit;
    }

    if (cipher_type != MBEDTLS_CIPHER_NONE) {
        cipher_info = mbedtls_cipher_info_from_type(cipher_type);
    }

    mbedtls_cipher_init(context->cipher_ctx);

    if (0 == mbedtls_cipher_setup(context->cipher_ctx, cipher_info)) {
        if (context->mode == kMode_SSS_Encrypt) {
            if ((context->keyObject->contents_size * 8) > INT_MAX) {
                retval = kStatus_SSS_Fail;
                goto exit;
            }
            if (mbedtls_cipher_setkey(context->cipher_ctx,
                    context->keyObject->contents,
                    context->keyObject->contents_size * 8,
                    MBEDTLS_ENCRYPT) != 0) {
                retval = kStatus_SSS_InvalidArgument;
            }
        }
        else if (context->mode == kMode_SSS_Decrypt) {
            if ((context->keyObject->contents_size * 8) > INT_MAX) {
                retval = kStatus_SSS_Fail;
                goto exit;
            }
            if (mbedtls_cipher_setkey(context->cipher_ctx,
                    context->keyObject->contents,
                    context->keyObject->contents_size * 8,
                    MBEDTLS_DECRYPT) != 0) {
                retval = kStatus_SSS_InvalidArgument;
            }
        }
        else {
            retval = kStatus_SSS_InvalidArgument;
        }
        if (retval == kStatus_SSS_Success) {
            if (ivLen != 0) {
                /* This should be only called when iv is initialized */
                ret = mbedtls_cipher_set_iv(context->cipher_ctx, iv, ivLen);
                if (ret < 0) {
                    SSS_FREE(context->cipher_ctx);
                    retval = kStatus_SSS_Fail;
                    goto exit;
                }
            }
            ret = mbedtls_cipher_reset(context->cipher_ctx);
            if (ret < 0) {
                SSS_FREE(context->cipher_ctx);
                retval = kStatus_SSS_Fail;
                goto exit;
            }
        }
    }

exit:
#endif
    return retval;
}

sss_status_t sss_mbedtls_cipher_update(
    sss_mbedtls_symmetric_t *context, const uint8_t *srcData, size_t srcLen, uint8_t *destData, size_t *destLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSS_HAVE_TESTCOUNTERPART
    uint8_t inputData[CIPHER_BLOCK_SIZE] = {
        0,
    };
    size_t inputData_len = 0;
    size_t src_offset    = 0;
    size_t output_offset = 0;
    size_t outBuffSize   = 0;
    size_t blockoutLen   = 0;
    int retMbedtlsVal;
    size_t cipherBlockSize = CIPHER_BLOCK_SIZE;

    ENSURE_OR_GO_EXIT(context != NULL);
    ENSURE_OR_GO_EXIT(destLen != NULL);
    ENSURE_OR_GO_EXIT(srcData != NULL);
    outBuffSize = *destLen;

    if (context->algorithm == kAlgorithm_SSS_DES_ECB || context->algorithm == kAlgorithm_SSS_DES_CBC ||
        context->algorithm == kAlgorithm_SSS_DES3_ECB || context->algorithm == kAlgorithm_SSS_DES3_CBC) {
        cipherBlockSize = DES_BLOCK_SIZE;
    }
    ENSURE_OR_GO_EXIT((SIZE_MAX - context->cache_data_len) >= srcLen);
    if ((context->cache_data_len + srcLen) < cipherBlockSize) {
        /* Insufficinet data to process . Cache the data */
        memcpy((context->cache_data + context->cache_data_len), srcData, srcLen);
        context->cache_data_len = context->cache_data_len + srcLen;
        *destLen                = 0;
        return kStatus_SSS_Success;
    }
    else {
        /* Concatenate the unprocessed and current input data*/
        memcpy(inputData, context->cache_data, context->cache_data_len);
        inputData_len = context->cache_data_len;
        memcpy((inputData + inputData_len), srcData, (cipherBlockSize - context->cache_data_len));
        inputData_len += (cipherBlockSize - context->cache_data_len);
        src_offset += (cipherBlockSize - context->cache_data_len);
        context->cache_data_len = 0;

        blockoutLen = outBuffSize;
        ENSURE_OR_GO_EXIT(blockoutLen >= inputData_len);
        retMbedtlsVal = mbedtls_cipher_update(
            context->cipher_ctx, inputData, inputData_len, (destData + output_offset), &blockoutLen);
        ENSURE_OR_GO_EXIT(retMbedtlsVal == 0);

        ENSURE_OR_GO_EXIT(outBuffSize >= blockoutLen);
        outBuffSize -= blockoutLen;
        output_offset += blockoutLen;
        if (srcLen < src_offset) {
            goto exit;
        }
        while (srcLen - src_offset >= cipherBlockSize) {
            memcpy(inputData, (srcData + src_offset), cipherBlockSize);
            ENSURE_OR_GO_EXIT((SIZE_MAX - src_offset) >= cipherBlockSize);
            src_offset += cipherBlockSize;

            blockoutLen   = outBuffSize;
            inputData_len = cipherBlockSize;
            ENSURE_OR_GO_EXIT(blockoutLen >= inputData_len);
            retMbedtlsVal = mbedtls_cipher_update(
                context->cipher_ctx, inputData, inputData_len, (destData + output_offset), &blockoutLen);
            ENSURE_OR_GO_EXIT(retMbedtlsVal == 0);
            if (outBuffSize < blockoutLen) {
                goto exit;
            }
            outBuffSize -= blockoutLen;
            ENSURE_OR_GO_EXIT((SIZE_MAX - output_offset) >= blockoutLen);
            output_offset += blockoutLen;
        }

        *destLen = output_offset;

        /* Copy unprocessed data to cache */
        if ((srcLen - src_offset) > 0) {
            memcpy(context->cache_data, (srcData + src_offset), (srcLen - src_offset));
            context->cache_data_len = (srcLen - src_offset);
        }
    }

    retval = kStatus_SSS_Success;
exit:
    if (retval == kStatus_SSS_Fail) {
        if (destLen != NULL) {
            *destLen = 0;
        }
    }
#endif
    return retval;
}

sss_status_t sss_mbedtls_cipher_finish(
    sss_mbedtls_symmetric_t *context, const uint8_t *srcData, size_t srcLen, uint8_t *destData, size_t *destLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSS_HAVE_TESTCOUNTERPART
    uint8_t srcdata_updated[2 * CIPHER_BLOCK_SIZE] = {
        0,
    };
    size_t srcdata_updated_len = 0;
    size_t outBuffSize         = 0;
    size_t blockoutLen         = 0;
    size_t output_offset       = 0;
    int retMbedtlsVal;
    uint8_t temp[16] = {
        0,
    };
    size_t temp_len        = sizeof(temp);
    size_t cipherBlockSize = CIPHER_BLOCK_SIZE;

    ENSURE_OR_GO_EXIT(context);
    ENSURE_OR_GO_EXIT(destLen != NULL);
    outBuffSize = *destLen;

    if (context->algorithm == kAlgorithm_SSS_DES_ECB || context->algorithm == kAlgorithm_SSS_DES_CBC ||
        context->algorithm == kAlgorithm_SSS_DES3_ECB || context->algorithm == kAlgorithm_SSS_DES3_CBC) {
        cipherBlockSize = DES_BLOCK_SIZE;
    }

    if (srcLen > cipherBlockSize) {
        LOG_E("srcLen cannot be grater than %zu bytes. Call update function ", cipherBlockSize);
        *destLen = 0;
        goto exit;
    }

    if (context->cache_data_len != 0) {
        memcpy(srcdata_updated, context->cache_data, context->cache_data_len);
        srcdata_updated_len     = context->cache_data_len;
        context->cache_data_len = 0;
    }
    if (srcLen != 0) {
        ENSURE_OR_GO_EXIT(srcData != NULL);
        memcpy((srcdata_updated + srcdata_updated_len), srcData, srcLen);
        srcdata_updated_len += srcLen;
    }

    /* Align buffer to cipherBlockSize */
    if (srcdata_updated_len > 0 && (srcdata_updated_len % cipherBlockSize != 0)) {
        srcdata_updated_len = srcdata_updated_len + (cipherBlockSize - (srcdata_updated_len % cipherBlockSize));
    }

    if (*destLen < srcdata_updated_len) {
        LOG_E("Output buffer not sufficient");
        goto exit;
    }

    *destLen = 0;

    if (srcdata_updated_len > 0) {
        blockoutLen = outBuffSize;
        ENSURE_OR_GO_EXIT(blockoutLen >= cipherBlockSize);
        retMbedtlsVal = mbedtls_cipher_update(
            context->cipher_ctx, srcdata_updated, cipherBlockSize, (destData + output_offset), &blockoutLen);
        ENSURE_OR_GO_EXIT(retMbedtlsVal == 0);
        *destLen = blockoutLen;
        if (outBuffSize < blockoutLen) {
            goto exit;
        }
        outBuffSize -= blockoutLen;
        ENSURE_OR_GO_EXIT((SIZE_MAX - output_offset) >= blockoutLen);
        output_offset += blockoutLen;
    }

    if (srcdata_updated_len > cipherBlockSize) {
        blockoutLen = outBuffSize;
        ENSURE_OR_GO_EXIT(blockoutLen >= cipherBlockSize);
        retMbedtlsVal = mbedtls_cipher_update(context->cipher_ctx,
            srcdata_updated + cipherBlockSize,
            cipherBlockSize,
            destData + output_offset,
            &blockoutLen);
        ENSURE_OR_GO_EXIT(retMbedtlsVal == 0);
        ENSURE_OR_GO_EXIT((SIZE_MAX - *destLen) >= blockoutLen);
        *destLen += blockoutLen;
        ENSURE_OR_GO_EXIT(outBuffSize >= blockoutLen);
        outBuffSize -= blockoutLen;
        ENSURE_OR_GO_EXIT((SIZE_MAX - output_offset) >= blockoutLen);
        output_offset += blockoutLen;
    }

    /*
    * For decrypt operation, on passing 16 bytes of buffer data to mbedtls_cipher_update, data is cached
    * and in next updated call it is processed.
    * So call another update api with zero dummy data
    */
    if (context->mode == kMode_SSS_Decrypt) {
        blockoutLen = outBuffSize;
        retMbedtlsVal =
            mbedtls_cipher_update(context->cipher_ctx, temp, temp_len, destData + output_offset, &blockoutLen);
        ENSURE_OR_GO_EXIT(retMbedtlsVal == 0);
        ENSURE_OR_GO_EXIT((SIZE_MAX - *destLen) >= blockoutLen);
        *destLen += blockoutLen;
    }
    ENSURE_OR_GO_EXIT(context->cipher_ctx != NULL);
    mbedtls_cipher_finish(context->cipher_ctx, temp, &temp_len);
    mbedtls_cipher_free(context->cipher_ctx);
    memset(context->cipher_ctx, 0, sizeof(*(context->cipher_ctx)));
    SSS_FREE(context->cipher_ctx);

    retval = kStatus_SSS_Success;
exit:
#endif
    return retval;
}

sss_status_t sss_mbedtls_cipher_crypt_ctr(sss_mbedtls_symmetric_t *context,
    const uint8_t *srcData,
    uint8_t *destData,
    size_t size,
    uint8_t *initialCounter,
    uint8_t *lastEncryptedCounter,
    size_t *szLeft)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if defined(MBEDTLS_CIPHER_MODE_CTR)
    mbedtls_aes_context ctx;
    int mbedtls_ret;

    mbedtls_aes_init(&ctx);

    switch (context->mode) {
    case kMode_SSS_Encrypt:
    case kMode_SSS_Decrypt:
        ENSURE_OR_GO_EXIT(context->algorithm == kAlgorithm_SSS_AES_CTR);
        ENSURE_OR_GO_EXIT(context->keyObject != NULL);
        mbedtls_ret = mbedtls_aes_setkey_enc(
            &ctx, context->keyObject->contents, (unsigned int)(context->keyObject->contents_size * 8));
        ENSURE_OR_GO_EXIT(mbedtls_ret == 0);

        mbedtls_ret =
            mbedtls_aes_crypt_ctr(&ctx, size, szLeft, initialCounter, lastEncryptedCounter, srcData, destData);
        ENSURE_OR_GO_EXIT(mbedtls_ret == 0);
        break;
    default:
        //retval = MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
        goto exit;
    }

    mbedtls_aes_free(&ctx);

    retval = kStatus_SSS_Success;
exit:
#endif // MBEDTLS_CIPHER_MODE_CTR
    return retval;
}

void sss_mbedtls_symmetric_context_free(sss_mbedtls_symmetric_t *context)
{
    memset(context, 0, sizeof(*context));
}

/* End: mbedtls_symm */

/* ************************************************************************** */
/* Functions : sss_mbedtls_aead                                               */
/* ************************************************************************** */

sss_status_t sss_mbedtls_aead_context_init(sss_mbedtls_aead_t *context,
    sss_mbedtls_session_t *session,
    sss_mbedtls_object_t *keyObject,
    sss_algorithm_t algorithm,
    sss_mode_t mode)
{
    sss_status_t retval = kStatus_SSS_Fail;
    ENSURE_OR_GO_CLEANUP(context);
    ENSURE_OR_GO_CLEANUP(session);
    ENSURE_OR_GO_CLEANUP(keyObject);

    context->session   = session;
    context->keyObject = keyObject;
    context->algorithm = algorithm;
    context->mode      = mode;

    if (algorithm == kAlgorithm_SSS_AES_GCM) {
        context->gcm_ctx = (mbedtls_gcm_context *)SSS_CALLOC(1, sizeof(mbedtls_gcm_context));
        ENSURE_OR_GO_CLEANUP(context->gcm_ctx);
    }
    else if (algorithm == kAlgorithm_SSS_AES_CCM) {
        context->ccm_ctx = (mbedtls_ccm_context *)SSS_CALLOC(1, sizeof(mbedtls_ccm_context));
        ENSURE_OR_GO_CLEANUP(context->ccm_ctx);
    }
    else {
        LOG_E("Improper Algorithm passed!");
        goto cleanup;
    }
    context->pCcm_aad  = NULL;
    context->pCcm_data = NULL;
    context->pNonce    = NULL;
    retval             = kStatus_SSS_Success;
cleanup:
    return retval;
}

sss_status_t sss_mbedtls_aead_one_go(sss_mbedtls_aead_t *context,
    const uint8_t *srcData,
    uint8_t *destData,
    size_t size,
    uint8_t *nonce,
    size_t nonceLen,
    const uint8_t *aad,
    size_t aadLen,
    uint8_t *tag,
    size_t *tagLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
    int ret             = 1;
    size_t stagLength   = 0;

    ENSURE_OR_GO_CLEANUP(tagLen != NULL);
    stagLength = *tagLen;
    if (context->algorithm == kAlgorithm_SSS_AES_GCM) {
        /* Initialize gcm context */
        mbedtls_gcm_init(context->gcm_ctx);

        /* Set key to the context */
        ret = mbedtls_gcm_setkey(context->gcm_ctx,
            MBEDTLS_CIPHER_ID_AES,
            context->keyObject->contents,
            (unsigned int)(context->keyObject->contents_size * 8));
        ENSURE_OR_GO_CLEANUP(ret == 0);

        /* Check the mode and perform requested operation */
        if (context->mode == kMode_SSS_Encrypt) {
            ret = mbedtls_gcm_crypt_and_tag(context->gcm_ctx,
                MBEDTLS_GCM_ENCRYPT,
                size,
                nonce,
                nonceLen,
                aad,
                aadLen,
                srcData,
                destData,
                stagLength,
                tag);
        }
        else {
            ret = mbedtls_gcm_auth_decrypt(
                context->gcm_ctx, size, nonce, nonceLen, aad, aadLen, tag, stagLength, srcData, destData);
        }
    }

    ENSURE_OR_GO_CLEANUP(ret == 0);
    *tagLen = stagLength;
    retval  = kStatus_SSS_Success;
cleanup:
    return retval;
}

sss_status_t sss_mbedtls_aead_init(
    sss_mbedtls_aead_t *context, uint8_t *nonce, size_t nonceLen, size_t tagLen, size_t aadLen, size_t payloadLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
    ENSURE_OR_GO_CLEANUP(context);
    ENSURE_OR_GO_CLEANUP(nonce);
    AX_UNUSED_ARG(tagLen);
    /* Save the nonce and its length in context */
    context->pNonce           = nonce;
    context->nonceLen         = nonceLen;
    context->ccm_aadLen       = aadLen;
    context->ccm_dataTotalLen = payloadLen;
    if (context->algorithm == kAlgorithm_SSS_AES_CCM) {
        if (context->ccm_dataTotalLen) {
            context->pCcm_data = SSS_MALLOC(payloadLen);
            if (context->pCcm_data) {
                memset(context->pCcm_data, 0, payloadLen);
                context->ccm_dataoffset = 0;
            }
            else {
                LOG_E("malloc failed");
                goto cleanup;
            }
        }
    }
    context->cache_data_len = 0;
    memset(context->cache_data, 0x00, sizeof(context->cache_data));
    retval = kStatus_SSS_Success;

cleanup:
    return retval;
}

sss_status_t sss_mbedtls_aead_update_aad(sss_mbedtls_aead_t *context, const uint8_t *aadData, size_t aadDataLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
    int ret             = 1;
    int mode            = (context->mode == kMode_SSS_Encrypt) ? MBEDTLS_GCM_ENCRYPT : MBEDTLS_GCM_DECRYPT;
    ENSURE_OR_GO_CLEANUP(context);
    if (aadDataLen > 0) {
        ENSURE_OR_GO_CLEANUP(aadData);
    }
    if (context->algorithm == kAlgorithm_SSS_AES_GCM) {
        /* Initialize gcm context */
        mbedtls_gcm_init(context->gcm_ctx);

        /* Set key to the context */
        ret = mbedtls_gcm_setkey(context->gcm_ctx,
            MBEDTLS_CIPHER_ID_AES,
            context->keyObject->contents,
            (unsigned int)(context->keyObject->contents_size * 8));
        ENSURE_OR_GO_CLEANUP(ret == 0);

/* Add aad Data */
#if SSS_HAVE_MBEDTLS_3_X
        ret = mbedtls_gcm_starts(context->gcm_ctx, mode, context->pNonce, context->nonceLen);
        ENSURE_OR_GO_CLEANUP(ret == 0);
        ret = mbedtls_gcm_update_ad(context->gcm_ctx, (unsigned char *)aadData, aadDataLen);
#else
        ret = mbedtls_gcm_starts(context->gcm_ctx, mode, context->pNonce, context->nonceLen, aadData, aadDataLen);
#endif
        ENSURE_OR_GO_CLEANUP(ret == 0);
    }
    else if (context->algorithm == kAlgorithm_SSS_AES_CCM) {
        /* Initialize ccm context */
        mbedtls_ccm_init(context->ccm_ctx);
        /* Set key to the context */
        ret = mbedtls_ccm_setkey(context->ccm_ctx,
            MBEDTLS_CIPHER_ID_AES,
            context->keyObject->contents,
            (unsigned int)(context->keyObject->contents_size * 8));
        ENSURE_OR_GO_CLEANUP(ret == 0);
        context->pCcm_aad   = aadData;
        context->ccm_aadLen = aadDataLen;
    }
    retval = kStatus_SSS_Success;
cleanup:
    return retval;
}

sss_status_t sss_mbedtls_aead_update(
    sss_mbedtls_aead_t *context, const uint8_t *srcData, size_t srcLen, uint8_t *destData, size_t *destLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSS_HAVE_TESTCOUNTERPART
    uint8_t inputData[CIPHER_BLOCK_SIZE] = {
        0,
    };
    size_t inputData_len = 0;
    size_t src_offset    = 0;
    size_t output_offset = 0;
    size_t outBuffSize   = 0;
    size_t blockoutLen   = 0;
#if SSS_HAVE_MBEDTLS_3_X
    size_t outputLen = 0;
#endif
    int ret = 1;

    ENSURE_OR_GO_CLEANUP(destLen != NULL);
    ENSURE_OR_GO_CLEANUP(srcData != NULL);
    outBuffSize = *destLen;

    ENSURE_OR_GO_CLEANUP((SIZE_MAX - (context->cache_data_len)) >= srcLen);

    if (context->algorithm == kAlgorithm_SSS_AES_CCM) {
        if ((srcData != NULL) && (srcLen > 0)) {
            retval = sss_mbedtls_aead_ccm_update(context, srcData, srcLen);
        }
        ENSURE_OR_GO_CLEANUP(retval == kStatus_SSS_Success);
        *destLen = 0;
    }
    else {
        if ((context->cache_data_len + srcLen) < CIPHER_BLOCK_SIZE) {
            /* Insufficinet data to process . Cache the data */
            memcpy((context->cache_data + context->cache_data_len), srcData, srcLen);
            context->cache_data_len = context->cache_data_len + srcLen;
            *destLen                = 0;
            return kStatus_SSS_Success;
        }
        else {
            /* Concatenate the unprocessed and current input data*/
            memcpy(inputData, context->cache_data, context->cache_data_len);
            inputData_len = context->cache_data_len;
            memcpy((inputData + inputData_len), srcData, (CIPHER_BLOCK_SIZE - context->cache_data_len));
            inputData_len += (CIPHER_BLOCK_SIZE - context->cache_data_len);
            src_offset += (CIPHER_BLOCK_SIZE - context->cache_data_len);
            blockoutLen = outBuffSize;

            /* Add Source Data */
            if (context->gcm_ctx == NULL) {
                return kStatus_SSS_Fail;
            }

#if SSS_HAVE_MBEDTLS_3_X
            outputLen = *destLen - output_offset;
            ret       = mbedtls_gcm_update(
                context->gcm_ctx, inputData, inputData_len, (destData + output_offset), outputLen, &outputLen);
#else
            ret = mbedtls_gcm_update(context->gcm_ctx, inputData_len, inputData, (destData + output_offset));
#endif

            ENSURE_OR_GO_CLEANUP(ret == 0);
            blockoutLen = inputData_len;

            ENSURE_OR_GO_CLEANUP(outBuffSize >= blockoutLen);
            outBuffSize -= blockoutLen;
            output_offset += blockoutLen;
            if (srcLen < src_offset) {
                goto cleanup;
            }
            while (srcLen - src_offset >= CIPHER_BLOCK_SIZE) {
                memcpy(inputData, (srcData + src_offset), 16);
                ENSURE_OR_GO_CLEANUP((SIZE_MAX - src_offset) >= CIPHER_BLOCK_SIZE);
                src_offset += CIPHER_BLOCK_SIZE;

                blockoutLen = outBuffSize;

                /* Add Source Data */
                if (context->gcm_ctx == NULL) {
                    return kStatus_SSS_Fail;
                }
#if SSS_HAVE_MBEDTLS_3_X
                ENSURE_OR_GO_CLEANUP(*destLen >= output_offset);
                outputLen = *destLen - output_offset;
                ret       = mbedtls_gcm_update(
                    context->gcm_ctx, inputData, inputData_len, (destData + output_offset), outputLen, &outputLen);
#else
                ret = mbedtls_gcm_update(context->gcm_ctx, inputData_len, inputData, (destData + output_offset));
#endif
                ENSURE_OR_GO_CLEANUP(ret == 0);
                blockoutLen = inputData_len;
                outBuffSize -= blockoutLen;
                ENSURE_OR_GO_CLEANUP((SIZE_MAX - output_offset) >= blockoutLen);
                output_offset += blockoutLen;
            }
            *destLen = output_offset;
            /* Copy unprocessed data to cache */
            memcpy(context->cache_data, (srcData + src_offset), (srcLen - src_offset));
            context->cache_data_len = (srcLen - src_offset);
        }
    }
    retval = kStatus_SSS_Success;
cleanup:
    if (retval == kStatus_SSS_Fail) {
        if (destLen != NULL) {
            *destLen = 0;
        }
    }
#endif /*End of SSS_HAVE_TESTCOUNTERPART*/
    return retval;
}

#if SSS_HAVE_TESTCOUNTERPART
static sss_status_t sss_mbedtls_aead_ccm_update(sss_mbedtls_aead_t *context, const uint8_t *srcData, size_t srcLen)
{
    sss_status_t retval = kStatus_SSS_Fail;

    ENSURE_OR_GO_EXIT((SIZE_MAX - context->ccm_dataoffset) >= srcLen);

    if ((context->ccm_dataoffset + srcLen) <= (context->ccm_dataTotalLen)) {
        memcpy(context->pCcm_data + context->ccm_dataoffset, srcData, srcLen);
        ENSURE_OR_GO_EXIT((SIZE_MAX - context->ccm_dataoffset) >= srcLen);
        context->ccm_dataoffset = context->ccm_dataoffset + srcLen;
        retval                  = kStatus_SSS_Success;
    }
    else {
        /*Free the allocated memory in init*/
        if (context->pCcm_data != NULL) {
            SSS_FREE(context->pCcm_data);
            context->pCcm_data = NULL;
        }
    }
exit:
    return retval;
}
#endif //#if SSS_HAVE_TESTCOUNTERPART

sss_status_t sss_mbedtls_aead_finish(sss_mbedtls_aead_t *context,
    const uint8_t *srcData,
    size_t srcLen,
    uint8_t *destData,
    size_t *destLen,
    uint8_t *tag,
    size_t *tagLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSS_HAVE_TESTCOUNTERPART
    size_t stagLen                                 = 0;
    int ret                                        = 1;
    uint8_t srcdata_updated[2 * CIPHER_BLOCK_SIZE] = {
        0,
    };
    size_t srcdata_updated_len = 0;
    uint8_t *pTag              = NULL;
#if SSS_HAVE_MBEDTLS_3_X
    size_t totalDestLen = 0;
    size_t outDestLen   = 0;
#endif

    ENSURE_OR_GO_EXIT(context);
    if (srcLen) {
        ENSURE_OR_GO_EXIT(srcData);
    }
    ENSURE_OR_GO_EXIT(destData);
    ENSURE_OR_GO_EXIT(tag);
    ENSURE_OR_GO_EXIT(tagLen);
    stagLen = *tagLen;
    if (context->algorithm == kAlgorithm_SSS_AES_CCM) { /* Check if finish has got source data */
        if ((srcData != NULL) && (srcLen > 0)) {
            retval = sss_mbedtls_aead_ccm_update(context, srcData, srcLen);
            ENSURE_OR_GO_EXIT(retval == kStatus_SSS_Success);
        }
        retval = sss_mbedtls_aead_ccm_finish(context, destData, destLen, tag, tagLen);
        ENSURE_OR_GO_EXIT(retval == kStatus_SSS_Success);
    }
    else {
        if (srcLen > CIPHER_BLOCK_SIZE) {
            LOG_E("srcLen cannot be grater than 16 bytes. Call update function ");
            if (destLen != NULL) {
                *destLen = 0;
            }
            goto exit;
        }

        if (context->cache_data_len != 0) {
            memcpy(srcdata_updated, context->cache_data, context->cache_data_len);
            srcdata_updated_len = context->cache_data_len;
        }

        ENSURE_OR_GO_EXIT(srcdata_updated_len <= SIZE_MAX - srcLen);

        if (srcLen != 0) {
            memcpy((srcdata_updated + srcdata_updated_len), srcData, srcLen);
            srcdata_updated_len += srcLen;
        }

        /* Add Source Data */
        if (context->gcm_ctx == NULL) {
            return kStatus_SSS_Fail;
        }

#if SSS_HAVE_MBEDTLS_3_X
        if (destLen != NULL) {
            totalDestLen = *destLen;
        }
        outDestLen = 0;
        ret        = mbedtls_gcm_update(
            context->gcm_ctx, srcdata_updated, srcdata_updated_len, destData, totalDestLen, &outDestLen);
        totalDestLen = totalDestLen - outDestLen;
        if (destLen != NULL) {
            *destLen = outDestLen;
        }
#else
        ret = mbedtls_gcm_update(context->gcm_ctx, srcdata_updated_len, srcdata_updated, destData);
        if (destLen != NULL) {
            *destLen = srcdata_updated_len;
        }
#endif
        ENSURE_OR_GO_EXIT(ret == 0);

        pTag = (uint8_t *)SSS_MALLOC(*tagLen);
        ENSURE_OR_GO_EXIT(pTag);
        memset(pTag, 0, *tagLen);

/* Get Tag for Enc*/
#if SSS_HAVE_MBEDTLS_3_X
        outDestLen = 0;
        ret        = mbedtls_gcm_finish(context->gcm_ctx, destData, totalDestLen, &outDestLen, pTag, stagLen);
        if (destLen != NULL) {
            ENSURE_OR_GO_EXIT(*destLen <= SIZE_MAX - outDestLen);
            *destLen += outDestLen;
        }
#else
        ret = mbedtls_gcm_finish(context->gcm_ctx, pTag, stagLen);
#endif

        ENSURE_OR_GO_EXIT(ret == 0);
        if (context->mode == kMode_SSS_Encrypt) {
            memcpy(tag, pTag, stagLen);
        }
        else {
            if (0 != memcmp(pTag, tag, stagLen)) {
                goto exit;
            }
        }

        *tagLen = stagLen;
    }
    retval = kStatus_SSS_Success;

exit:
    if (pTag) {
        SSS_FREE(pTag);
    }
#endif
    return retval;
}
#if SSS_HAVE_TESTCOUNTERPART
static sss_status_t sss_mbedtls_aead_ccm_finish(
    sss_mbedtls_aead_t *context, uint8_t *destData, size_t *destLen, uint8_t *tag, size_t *tagLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
    size_t stagLen      = *tagLen;
    int ret             = 1;
    /* Check the mode and perform requested operation */
    if (context->mode == kMode_SSS_Encrypt) {
        ret = mbedtls_ccm_encrypt_and_tag(context->ccm_ctx,
            context->ccm_dataTotalLen,
            context->pNonce,
            context->nonceLen,
            context->pCcm_aad,
            context->ccm_aadLen,
            context->pCcm_data,
            destData,
            tag,
            stagLen);
    }
    else {
        ret = mbedtls_ccm_auth_decrypt(context->ccm_ctx,
            context->ccm_dataTotalLen,
            context->pNonce,
            context->nonceLen,
            context->pCcm_aad,
            context->ccm_aadLen,
            context->pCcm_data,
            destData,
            tag,
            stagLen);
    }
    ENSURE_OR_GO_EXIT(ret == 0);
    *destLen = context->ccm_dataTotalLen;
    retval   = kStatus_SSS_Success;

exit:
    return retval;
}
#endif //if SSS_HAVE_TESTCOUNTERPART

void sss_mbedtls_aead_context_free(sss_mbedtls_aead_t *context)
{
    if (context != NULL) {
        if (context->algorithm == kAlgorithm_SSS_AES_GCM) {
            if (context->gcm_ctx != NULL) {
                mbedtls_gcm_free(context->gcm_ctx);
                SSS_FREE(context->gcm_ctx);
            }
        }
        else if (context->algorithm == kAlgorithm_SSS_AES_CCM) {
            if (context->ccm_ctx != NULL) {
                mbedtls_ccm_free(context->ccm_ctx);
                SSS_FREE(context->ccm_ctx);
                if (context->pCcm_data != NULL) {
                    SSS_FREE(context->pCcm_data);
                    context->pCcm_data = NULL;
                }
            }
        }
        if (context->pCcm_aad != NULL) {
            context->pCcm_aad = NULL;
        }
        if (context->pNonce != NULL) {
            context->pNonce = NULL;
        }
        memset(context, 0, sizeof(*context));
    }
}

/* End: mbedtls_aead */

/* ************************************************************************** */
/* Functions : sss_mbedtls_mac                                               */
/* ************************************************************************** */
sss_status_t sss_mbedtls_mac_context_init(sss_mbedtls_mac_t *context,
    sss_mbedtls_session_t *session,
    sss_mbedtls_object_t *keyObject,
    sss_algorithm_t algorithm,
    sss_mode_t mode)
{
    sss_status_t status = kStatus_SSS_Fail;
    ENSURE_OR_GO_CLEANUP(context);
    ENSURE_OR_GO_CLEANUP(session);
    ENSURE_OR_GO_CLEANUP(keyObject);

    context->session    = session;
    context->keyObject  = keyObject;
    context->algorithm  = algorithm;
    context->mode       = mode;
    context->cipher_ctx = NULL;

    if (context->algorithm == kAlgorithm_SSS_CMAC_AES) {
        context->cipher_ctx = (mbedtls_cipher_context_t *)SSS_CALLOC(1, sizeof(mbedtls_cipher_context_t));
        ENSURE_OR_GO_CLEANUP(context->cipher_ctx);
    }
#if SSSFTR_SW_TESTCOUNTERPART
    if (algorithm == kAlgorithm_SSS_HMAC_SHA1 || algorithm == kAlgorithm_SSS_HMAC_SHA224 ||
        algorithm == kAlgorithm_SSS_HMAC_SHA256 || algorithm == kAlgorithm_SSS_HMAC_SHA384 ||
        algorithm == kAlgorithm_SSS_HMAC_SHA512) {
        context->HmacCtx = (mbedtls_md_context_t *)SSS_CALLOC(1, sizeof(mbedtls_md_context_t));
        ENSURE_OR_GO_CLEANUP(context->HmacCtx);
    }
#endif
    status = kStatus_SSS_Success;
cleanup:
    return status;
}

sss_status_t sss_mbedtls_mac_one_go(
    sss_mbedtls_mac_t *context, const uint8_t *message, size_t messageLen, uint8_t *mac, size_t *macLen)
{
    sss_status_t status = kStatus_SSS_Fail;
    int ret;
    const mbedtls_cipher_info_t *cipher_info;
#if SSS_HAVE_TESTCOUNTERPART
    const mbedtls_md_info_t *md_info = NULL;
#endif
    uint8_t *key;
    size_t keylen;

    ENSURE_OR_GO_CLEANUP(context);
    ENSURE_OR_GO_CLEANUP(context->keyObject->contents);
    key    = context->keyObject->contents;
    keylen = context->keyObject->contents_size;

    if (context->algorithm == kAlgorithm_SSS_CMAC_AES) {
        mbedtls_cipher_type_t cipher_type = MBEDTLS_CIPHER_NONE;

        switch (keylen * 8) {
        case 128:
            cipher_type = MBEDTLS_CIPHER_AES_128_ECB;
            break;
#if SSS_HAVE_TESTCOUNTERPART
        case 192:
            cipher_type = MBEDTLS_CIPHER_AES_192_ECB;
            break;
        case 256:
            cipher_type = MBEDTLS_CIPHER_AES_256_ECB;
            break;
#endif
        default:
            LOG_E("key bit not supported");
            goto cleanup;
        }

        cipher_info = mbedtls_cipher_info_from_type(cipher_type);
        if (cipher_info != NULL) {
            mbedtls_cipher_init(context->cipher_ctx);
            ret = mbedtls_cipher_setup(context->cipher_ctx, cipher_info);
            if (ret == 0) {
                if (ret == 0) {
#ifdef MBEDTLS_CMAC_C
                    ret = mbedtls_cipher_cmac_starts(context->cipher_ctx, key, (keylen * 8));
                    if (ret == 0) {
                        ret = mbedtls_cipher_cmac_update(context->cipher_ctx, message, messageLen);
                        if (ret == 0) {
                            if (context->mode == kMode_SSS_Mac) {
                                ret = mbedtls_cipher_cmac_finish(context->cipher_ctx, mac);
                                if (ret == 0) {
#if SSS_HAVE_MBEDTLS_3_X
                                    *macLen =
                                        context->cipher_ctx->MBEDTLS_PRIVATE(cipher_info)->MBEDTLS_PRIVATE(block_size);
#else
                                    *macLen = context->cipher_ctx->cipher_info->block_size;
#endif
                                    status = kStatus_SSS_Success;
                                }
                            }
                            else if (context->mode == kMode_SSS_Mac_Validate) {
                                /* validate MAC*/
                                uint8_t macLocal[64] = {
                                    0,
                                };
                                size_t macLocalLen;
                                status = kStatus_SSS_Fail;
                                ret    = mbedtls_cipher_cmac_finish(context->cipher_ctx, macLocal);
                                if (ret == 0) {
#if SSS_HAVE_MBEDTLS_3_X
                                    macLocalLen =
                                        context->cipher_ctx->MBEDTLS_PRIVATE(cipher_info)->MBEDTLS_PRIVATE(block_size);
#else
                                    macLocalLen = context->cipher_ctx->cipher_info->block_size;
#endif
                                    if (macLocalLen == *macLen) {
                                        if (!memcmp(macLocal, mac, macLocalLen)) {
                                            status = kStatus_SSS_Success;
                                        }
                                    }
                                }
                            }
                            else {
                                LOG_E("Unknown mode");
                                status = kStatus_SSS_Fail;
                            }
                        }
                    }
#endif
                }
            }
        }
    }
#if SSS_HAVE_TESTCOUNTERPART
    else if (context->algorithm == kAlgorithm_SSS_HMAC_SHA1 || context->algorithm == kAlgorithm_SSS_HMAC_SHA224 ||
             context->algorithm == kAlgorithm_SSS_HMAC_SHA256 || context->algorithm == kAlgorithm_SSS_HMAC_SHA384 ||
             context->algorithm == kAlgorithm_SSS_HMAC_SHA512) {
        /*For HMAC any Key length is supported*/
        switch (context->algorithm) {
        case kAlgorithm_SSS_HMAC_SHA1:
            md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
            break;
        case kAlgorithm_SSS_HMAC_SHA224:
            md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA224);
            break;
        case kAlgorithm_SSS_HMAC_SHA256:
            md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
            break;
        case kAlgorithm_SSS_HMAC_SHA384:
            md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA384);
            break;
        case kAlgorithm_SSS_HMAC_SHA512:
            md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
            break;
        default:
            LOG_E("Invalid HMAC algorithm");
            status = kStatus_SSS_Fail;
            goto cleanup;
        }

        if (md_info != NULL) {
            if (context->mode == kMode_SSS_Mac) {
                ret = mbedtls_md_hmac(md_info, key, keylen, message, messageLen, mac);
                if (ret == 0) {
                    *macLen = mbedtls_md_get_size(md_info);
                    status  = kStatus_SSS_Success;
                }
            }
            else if (context->mode == kMode_SSS_Mac_Validate) {
                /* validate MAC*/
                uint8_t macLocal[64] = {
                    0,
                };
                size_t macLocalLen = sizeof(macLocal);
                status             = kStatus_SSS_Fail;
                ret                = mbedtls_md_hmac(md_info, key, keylen, message, messageLen, macLocal);
                if (ret == 0) {
                    macLocalLen = mbedtls_md_get_size(md_info);
                    if (macLocalLen == *macLen) {
                        if (!memcmp(macLocal, mac, macLocalLen)) {
                            status = kStatus_SSS_Success;
                        }
                    }
                }
            }
            else {
                LOG_E("Unknown mode");
                status = kStatus_SSS_Fail;
            }
        }
    }
#endif //SSS_HAVE_TESTCOUNTERPART
    else {
        LOG_E("Invalid algorithm type");
    }
cleanup:
    return status;
}

sss_status_t sss_mbedtls_mac_init(sss_mbedtls_mac_t *context)
{
    sss_status_t status = kStatus_SSS_Fail;
    int ret;
    uint8_t *key;
    size_t keylen;
    mbedtls_cipher_type_t cipher_type = MBEDTLS_CIPHER_NONE;

    ENSURE_OR_GO_CLEANUP(context->keyObject->contents);
    key    = context->keyObject->contents;
    keylen = context->keyObject->contents_size;

    if (context->algorithm == kAlgorithm_SSS_CMAC_AES) {
        const mbedtls_cipher_info_t *cipher_info = NULL;

        switch (context->keyObject->keyBitLen) {
        case 128:
            cipher_type = MBEDTLS_CIPHER_AES_128_ECB;
            break;
#if SSS_HAVE_TESTCOUNTERPART
        case 192:
            cipher_type = MBEDTLS_CIPHER_AES_192_ECB;
            break;
        case 256:
            cipher_type = MBEDTLS_CIPHER_AES_256_ECB;
            break;
#endif
        default:
            LOG_E("key bit not supported");
            goto cleanup;
        }

        if (cipher_type != MBEDTLS_CIPHER_NONE) {
            cipher_info = mbedtls_cipher_info_from_type(cipher_type);
        }

        if (cipher_info != NULL) {
            mbedtls_cipher_init(context->cipher_ctx);
            ret = mbedtls_cipher_setup(context->cipher_ctx, cipher_info);
            if (ret == 0) {
#ifdef MBEDTLS_CMAC_C
                ret = mbedtls_cipher_cmac_starts(context->cipher_ctx, key, (keylen * 8));
#endif
                if (ret == 0) {
                    status = kStatus_SSS_Success;
                }
            }
        }
    }
#if SSS_HAVE_TESTCOUNTERPART
    else if (context->algorithm == kAlgorithm_SSS_HMAC_SHA1 || context->algorithm == kAlgorithm_SSS_HMAC_SHA224 ||
             context->algorithm == kAlgorithm_SSS_HMAC_SHA256 || context->algorithm == kAlgorithm_SSS_HMAC_SHA384 ||
             context->algorithm == kAlgorithm_SSS_HMAC_SHA512) {
        /* for HMAC any key length is supported */

        const mbedtls_md_info_t *md_info = NULL;
        mbedtls_md_context_t *hmac_ctx;
        hmac_ctx = context->HmacCtx;
        mbedtls_md_init(hmac_ctx);

        switch (context->algorithm) {
        case kAlgorithm_SSS_HMAC_SHA1:
            md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
            break;
        case kAlgorithm_SSS_HMAC_SHA224:
            md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA224);
            break;
        case kAlgorithm_SSS_HMAC_SHA256:
            md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
            break;
        case kAlgorithm_SSS_HMAC_SHA384:
            md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA384);
            break;
        case kAlgorithm_SSS_HMAC_SHA512:
            md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
            break;
        default:
            status = kStatus_SSS_Fail;
            goto cleanup;
        }

        if (md_info != NULL) {
            /* Below, third parameter '1' indicates that HMAC is to be setup*/
            ret = mbedtls_md_setup(hmac_ctx, md_info, 1);
            if (ret == 0) {
                ret = mbedtls_md_hmac_starts(hmac_ctx, key, (keylen));

                if (ret == 0) {
                    status = kStatus_SSS_Success;
                }
            }
        }
    }
#endif //SSS_HAVE_TESTCOUNTERPART
    else {
        LOG_E("invalid algorithm mode for sss_mbedtls_mac_context_init ");
    }

cleanup:
    return status;
}

sss_status_t sss_mbedtls_mac_update(sss_mbedtls_mac_t *context, const uint8_t *message, size_t messageLen)
{
    int ret             = 1;
    sss_status_t status = kStatus_SSS_InvalidArgument;
    ENSURE_OR_GO_EXIT(message != NULL);

    status = kStatus_SSS_Fail;
    LOG_AU8_D(message, messageLen);
    if (context->algorithm == kAlgorithm_SSS_CMAC_AES) {
#ifdef MBEDTLS_CMAC_C
        mbedtls_cipher_context_t *ctx;
        ctx = context->cipher_ctx;
        ret = mbedtls_cipher_cmac_update(ctx, message, messageLen);
#endif
        if (ret == 0) {
            status = kStatus_SSS_Success;
        }
    }
#if SSSFTR_SW_TESTCOUNTERPART
    else if (context->algorithm == kAlgorithm_SSS_HMAC_SHA1 || context->algorithm == kAlgorithm_SSS_HMAC_SHA224 ||
             context->algorithm == kAlgorithm_SSS_HMAC_SHA256 || context->algorithm == kAlgorithm_SSS_HMAC_SHA384 ||
             context->algorithm == kAlgorithm_SSS_HMAC_SHA512) {
        mbedtls_md_context_t *hmac_ctx;
        hmac_ctx = context->HmacCtx;
        ret      = mbedtls_md_hmac_update(hmac_ctx, message, messageLen);

        if (ret == 0) {
            status = kStatus_SSS_Success;
        }
    }
#endif
    else {
        LOG_E("invalid algorithm mode for sss_mbedtls_mac_update");
    }
exit:
    return status;
}

sss_status_t sss_mbedtls_mac_finish(sss_mbedtls_mac_t *context, uint8_t *mac, size_t *macLen)
{
    int ret             = 1;
    sss_status_t status = kStatus_SSS_InvalidArgument;
    ENSURE_OR_GO_EXIT((mac != NULL) && (macLen != NULL));

    status = kStatus_SSS_Fail;

    if (context->algorithm == kAlgorithm_SSS_CMAC_AES) {
        mbedtls_cipher_context_t *ctx;
        ctx = context->cipher_ctx;

        if (context->mode == kMode_SSS_Mac) {
#ifdef MBEDTLS_CMAC_C
            ret = mbedtls_cipher_cmac_finish(ctx, mac);
#endif
            if (ret == 0) {
#if SSS_HAVE_MBEDTLS_3_X
                *macLen = ctx->MBEDTLS_PRIVATE(cipher_info)->MBEDTLS_PRIVATE(block_size);
#else
                *macLen = ctx->cipher_info->block_size;
#endif
                status = kStatus_SSS_Success;
            }
        }
        else if (context->mode == kMode_SSS_Mac_Validate) {
            /* validate MAC*/
            uint8_t macLocal[64] = {
                0,
            };
            size_t macLocalLen;
            status = kStatus_SSS_Fail;
#ifdef MBEDTLS_CMAC_C
            ret = mbedtls_cipher_cmac_finish(ctx, macLocal);
#endif
            if (ret == 0) {
#if SSS_HAVE_MBEDTLS_3_X
                macLocalLen = ctx->MBEDTLS_PRIVATE(cipher_info)->MBEDTLS_PRIVATE(block_size);
#else
                macLocalLen = ctx->cipher_info->block_size;
#endif
                if (macLocalLen == *macLen) {
                    if (!memcmp(macLocal, mac, macLocalLen)) {
                        status = kStatus_SSS_Success;
                    }
                }
            }
        }
        else {
            LOG_E("Unknown mode");
            status = kStatus_SSS_Fail;
        }
    }
#if SSS_HAVE_TESTCOUNTERPART
    else if (context->algorithm == kAlgorithm_SSS_HMAC_SHA1 || context->algorithm == kAlgorithm_SSS_HMAC_SHA224 ||
             context->algorithm == kAlgorithm_SSS_HMAC_SHA256 || context->algorithm == kAlgorithm_SSS_HMAC_SHA384 ||
             context->algorithm == kAlgorithm_SSS_HMAC_SHA512) {
        mbedtls_md_context_t *hmacctx;
        hmacctx = context->HmacCtx;

        if (context->mode == kMode_SSS_Mac) {
            ret = mbedtls_md_hmac_finish(hmacctx, mac);
            if (ret == 0) {
#if SSS_HAVE_MBEDTLS_3_X
                *macLen = mbedtls_md_get_size(hmacctx->MBEDTLS_PRIVATE(md_info));
#else
                *macLen = mbedtls_md_get_size(hmacctx->md_info);
#endif
                status = kStatus_SSS_Success;
            }
        }
        else if (context->mode == kMode_SSS_Mac_Validate) {
            /* validate MAC*/
            uint8_t macLocal[64] = {
                0,
            };
            size_t macLocalLen = sizeof(macLocal);
            status             = kStatus_SSS_Fail;
            ret                = mbedtls_md_hmac_finish(hmacctx, macLocal);
            if (ret == 0) {
#if SSS_HAVE_MBEDTLS_3_X
                macLocalLen = mbedtls_md_get_size(hmacctx->MBEDTLS_PRIVATE(md_info));
#else
                macLocalLen = mbedtls_md_get_size(hmacctx->md_info);
#endif
                if (macLocalLen == *macLen) {
                    if (!memcmp(macLocal, mac, macLocalLen)) {
                        status = kStatus_SSS_Success;
                    }
                }
            }
        }
        else {
            LOG_E("Unknown mode");
            status = kStatus_SSS_Fail;
        }
    }
#endif //SSS_HAVE_TESTCOUNTERPART
    else {
        LOG_E("Invalid algorithm type for sss_mbedtls_mac_finish");
    }
exit:
    return status;
}

void sss_mbedtls_mac_context_free(sss_mbedtls_mac_t *context)
{
    if (context != NULL) {
        if (context->cipher_ctx != NULL) {
            mbedtls_cipher_free(context->cipher_ctx);
            SSS_FREE(context->cipher_ctx);
        }
#if SSSFTR_SW_TESTCOUNTERPART
        if (context->algorithm == kAlgorithm_SSS_HMAC_SHA1 || context->algorithm == kAlgorithm_SSS_HMAC_SHA224 ||
            context->algorithm == kAlgorithm_SSS_HMAC_SHA256 || context->algorithm == kAlgorithm_SSS_HMAC_SHA384 ||
            context->algorithm == kAlgorithm_SSS_HMAC_SHA512) {
            mbedtls_md_free(context->HmacCtx);
            SSS_FREE(context->HmacCtx);
        }
#endif
        memset(context, 0, sizeof(*context));
    }
}

/* ************************************************************************** */
/* Functions : sss_mbedtls_md                                                 */
/* ************************************************************************** */

sss_status_t sss_mbedtls_digest_context_init(
    sss_mbedtls_digest_t *context, sss_mbedtls_session_t *session, sss_algorithm_t algorithm, sss_mode_t mode)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSS_HAVE_TESTCOUNTERPART
    ENSURE_OR_GO_CLEANUP(context);
    memset(context, 0, sizeof(*context));
    context->session   = session;
    context->algorithm = algorithm;
    context->mode      = mode;
    retval             = kStatus_SSS_Success;
cleanup:
#endif //SSS_HAVE_TESTCOUNTERPART
    return retval;
}

sss_status_t sss_mbedtls_digest_one_go(
    sss_mbedtls_digest_t *context, const uint8_t *message, size_t messageLen, uint8_t *digest, size_t *digestLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSS_HAVE_TESTCOUNTERPART
    int ret;
    const mbedtls_md_info_t *mdinfo = NULL;
    mbedtls_md_type_t md_type       = MBEDTLS_MD_NONE;

    switch (context->algorithm) {
    case kAlgorithm_SSS_SHA1:
        md_type    = MBEDTLS_MD_SHA1;
        *digestLen = 20;
        break;
    case kAlgorithm_SSS_SHA224:
        md_type    = MBEDTLS_MD_SHA224;
        *digestLen = 28;
        break;
    case kAlgorithm_SSS_SHA256:
        md_type    = MBEDTLS_MD_SHA256;
        *digestLen = 32;
        break;
    case kAlgorithm_SSS_SHA384:
        md_type    = MBEDTLS_MD_SHA384;
        *digestLen = 48;
        break;
    case kAlgorithm_SSS_SHA512:
        md_type    = MBEDTLS_MD_SHA512;
        *digestLen = 64;
        break;
    default: {
        LOG_E("Algorithm mode not suported");
        goto exit;
    }
    }

    mdinfo = mbedtls_md_info_from_type(md_type);

    ret = mbedtls_md(mdinfo, message, messageLen, digest);

    if (ret != 0) {
        LOG_E("mbedtls_md failed");
        *digestLen = 0;
        goto exit;
    }

    retval = kStatus_SSS_Success;
exit:
#endif //SSS_HAVE_TESTCOUNTERPART
    return retval;
}

sss_status_t sss_mbedtls_digest_init(sss_mbedtls_digest_t *context)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSS_HAVE_TESTCOUNTERPART
    const mbedtls_md_info_t *mdinfo = NULL;
    mbedtls_md_type_t md_type       = MBEDTLS_MD_NONE;
    int ret;

    ENSURE_OR_GO_EXIT(context != NULL);

    switch (context->algorithm) {
    case kAlgorithm_SSS_SHA1:
        md_type = MBEDTLS_MD_SHA1;
        break;
    case kAlgorithm_SSS_SHA224:
        md_type = MBEDTLS_MD_SHA224;
        break;
    case kAlgorithm_SSS_SHA256:
        md_type = MBEDTLS_MD_SHA256;
        break;
    case kAlgorithm_SSS_SHA384:
        md_type = MBEDTLS_MD_SHA384;
        break;
    case kAlgorithm_SSS_SHA512:
        md_type = MBEDTLS_MD_SHA512;
        break;
    default:
        LOG_E("Algorithm mode not suported");
        goto exit;
    }

    mbedtls_md_init(&context->md_ctx);

    mdinfo = mbedtls_md_info_from_type(md_type);

#if SSS_HAVE_MBEDTLS_3_X
    ret = mbedtls_md_setup(&context->md_ctx, mdinfo, 0);
#else
    ret = mbedtls_md_init_ctx(&context->md_ctx, mdinfo);
#endif
    if (ret != 0) {
        mbedtls_md_free(&context->md_ctx);
        goto exit;
    }

    ret = mbedtls_md_starts(&context->md_ctx);
    if (ret != 0) {
        mbedtls_md_free(&context->md_ctx);
        goto exit;
    }

    retval = kStatus_SSS_Success;
exit:
#endif //SSS_HAVE_TESTCOUNTERPART
    return retval;
}

sss_status_t sss_mbedtls_digest_update(sss_mbedtls_digest_t *context, const uint8_t *message, size_t messageLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSS_HAVE_TESTCOUNTERPART

    int ret = mbedtls_md_update(&context->md_ctx, message, messageLen);
    ENSURE_OR_GO_EXIT(ret == 0);

    retval = kStatus_SSS_Success;
exit:
#endif //SSS_HAVE_TESTCOUNTERPART
    return retval;
}

sss_status_t sss_mbedtls_digest_finish(sss_mbedtls_digest_t *context, uint8_t *digest, size_t *digestLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSS_HAVE_TESTCOUNTERPART
    int ret;

    ENSURE_OR_GO_EXIT(context != 0);

    switch (context->algorithm) {
    case kAlgorithm_SSS_SHA1:
        *digestLen = 20;
        break;
    case kAlgorithm_SSS_SHA224:
        *digestLen = 28;
        break;
    case kAlgorithm_SSS_SHA256:
        *digestLen = 32;
        break;
    case kAlgorithm_SSS_SHA384:
        *digestLen = 48;
        break;
    case kAlgorithm_SSS_SHA512:
        *digestLen = 64;
        break;
    default: {
        LOG_E("Algorithm mode not suported");
        goto exit;
    }
    }

    ret = mbedtls_md_finish(&context->md_ctx, digest);
    if (ret != 0) {
        LOG_E("mbedtls_md_update failed");
        *digestLen = 0;
        goto exit;
    }

    retval = kStatus_SSS_Success;
exit:
    if (context != NULL) {
        mbedtls_md_free(&context->md_ctx);
    }
#endif //SSS_HAVE_TESTCOUNTERPART
    return retval;
}

void sss_mbedtls_digest_context_free(sss_mbedtls_digest_t *context)
{
    memset(context, 0, sizeof(*context));
}

/* End: mbedtls_md */

/* ************************************************************************** */
/* Functions : sss_mbedtls_rng                                                */
/* ************************************************************************** */

sss_status_t sss_mbedtls_rng_context_init(sss_mbedtls_rng_context_t *context, sss_mbedtls_session_t *session)
{
    sss_status_t retval = kStatus_SSS_Fail;

    ENSURE_OR_GO_EXIT(context);
    ENSURE_OR_GO_EXIT(session);

    context->session = session;

    if (session->ctr_drbg == NULL) {
        session->ctr_drbg = SSS_CALLOC(1, sizeof(*session->ctr_drbg));
        ENSURE_OR_GO_EXIT(session->ctr_drbg != NULL);
        mbedtls_ctr_drbg_init((session->ctr_drbg));
    }

    if (session->entropy == NULL) {
        session->entropy = SSS_CALLOC(1, sizeof(*session->entropy));
        ENSURE_OR_GO_EXIT(session->entropy != NULL);
        mbedtls_entropy_init((session->entropy));
    }

    retval = kStatus_SSS_Success;
exit:
    return retval;
}

sss_status_t sss_mbedtls_rng_get_random(sss_mbedtls_rng_context_t *context, uint8_t *random_data, size_t dataLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
    size_t chunk        = 0;
    size_t offset       = 0;
    int ret             = -1;

    while (dataLen > 0) {
        if (dataLen > MBEDTLS_CTR_DRBG_MAX_REQUEST) {
            chunk = MBEDTLS_CTR_DRBG_MAX_REQUEST;
        }
        else {
            chunk = dataLen;
        }

        ret = mbedtls_ctr_drbg_random(context->session->ctr_drbg, (random_data + offset), chunk);
        ENSURE_OR_GO_EXIT(ret == 0);
        ENSURE_OR_GO_EXIT((SIZE_MAX - offset) >= chunk);
        offset += chunk;
        dataLen -= chunk;
    }

    retval = kStatus_SSS_Success;
exit:
    return retval;
}

sss_status_t sss_mbedtls_rng_context_free(sss_mbedtls_rng_context_t *context)
{
    sss_status_t retval = kStatus_SSS_Success;
    memset(context, 0, sizeof(*context));
    return retval;
}

/* End: mbedtls_rng */

/* ************************************************************************** */
/* Functions : Private sss mbedtls functions                                  */
/* ************************************************************************** */

// FIXME: Handle data/dataLen
static sss_status_t sss_mbedtls_set_key(
    sss_mbedtls_object_t *keyObject, const uint8_t *data, size_t dataLen, size_t keyBitLen)
{
    sss_status_t retval = kStatus_SSS_Fail;
#if SSSFTR_SW_ECC || SSSFTR_SW_RSA
    size_t base64_olen;
    int ret;
    char pem_format[2048];
#endif
    switch (keyObject->objectType) {
    case kSSS_KeyPart_Default:
        ENSURE_OR_GO_EXIT(dataLen <= keyObject->contents_max_size);
        if (data != NULL) /* For empty certificate */
            memcpy(keyObject->contents, data, dataLen);
        keyObject->contents_size = dataLen;
        keyObject->keyBitLen     = keyBitLen;
        retval                   = kStatus_SSS_Success;
        break;
#if SSSFTR_SW_ECC || SSSFTR_SW_RSA
    case kSSS_KeyPart_Private:
    case kSSS_KeyPart_Pair: {
        mbedtls_pk_context *pk = (mbedtls_pk_context *)keyObject->contents;
        if (keyObject->cipherType == kSSS_CipherType_EC_MONTGOMERY) {
#if defined(MBEDTLS_ECP_C)
            mbedtls_ecp_keypair *pEcpPrv = NULL;
            sss_status_t asn_retval      = kStatus_SSS_Fail;
            ret                          = mbedtls_pk_setup(pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
            ENSURE_OR_GO_EXIT(ret == 0);

            pEcpPrv = mbedtls_pk_ec(*pk);
            ENSURE_OR_GO_EXIT(pEcpPrv);

#if SSS_HAVE_MBEDTLS_3_X
            if (keyBitLen == 256) {
                ret = mbedtls_ecp_group_load(&pEcpPrv->MBEDTLS_PRIVATE(grp), MBEDTLS_ECP_DP_CURVE25519);
            }
            else if (keyBitLen == 448) {
                ret = mbedtls_ecp_group_load(&pEcpPrv->MBEDTLS_PRIVATE(grp), MBEDTLS_ECP_DP_CURVE448);
            }
#else
            if (keyBitLen == 256) {
                ret = mbedtls_ecp_group_load(&pEcpPrv->grp, MBEDTLS_ECP_DP_CURVE25519);
            }
            else if (keyBitLen == 448) {
                ret = mbedtls_ecp_group_load(&pEcpPrv->grp, MBEDTLS_ECP_DP_CURVE448);
            }
#endif

            else {
                ret = 1;
            }
            ENSURE_OR_GO_EXIT(ret == 0);

#ifdef MBEDTLS_DO_LITTLE_ENDIAN // Reverse Endianness
            {
                size_t i                   = 0;
                uint16_t publicKeyIndex    = 0;
                size_t publicKeyLen        = 0;
                uint16_t privateKeyIndex   = 0;
                size_t privateKeyLen       = 0;
                uint8_t pubKeyReversed[64] = {
                    0,
                };
                const uint8_t *pPublicKey  = NULL;
                uint8_t prvKeyReversed[64] = {
                    0,
                };
                const uint8_t *pPrivateKey = NULL;

                asn_retval = sss_util_rfc8410_asn1_get_ec_pair_key_index(
                    data, dataLen, &publicKeyIndex, &publicKeyLen, &privateKeyIndex, &privateKeyLen);
                if (asn_retval != kStatus_SSS_Success) {
                    LOG_W("error in sss_util_rfc8410_asn1_get_ec_pair_key_index");
                    goto exit;
                }

                while (i < publicKeyLen) {
                    ENSURE_OR_GO_EXIT((SIZE_MAX - publicKeyIndex) >= publicKeyLen);
                    pubKeyReversed[i] = data[publicKeyIndex + publicKeyLen - i - 1];
                    i++;
                }
                pPublicKey = &pubKeyReversed[0];

                i = 0;
                while (i < privateKeyLen) {
                    ENSURE_OR_GO_EXIT((SIZE_MAX - publicKeyIndex) >= publicKeyLen);
                    prvKeyReversed[i] = data[privateKeyIndex + privateKeyLen - i - 1];
                    i++;
                }

                /* RFC 7748, Sec 5 Par 5*/
                if (keyBitLen == 256) {
                    prvKeyReversed[privateKeyLen - 1] = prvKeyReversed[privateKeyLen - 1] & 0xF8;
                    prvKeyReversed[0]                 = prvKeyReversed[0] & 0x7F;
                    prvKeyReversed[0]                 = prvKeyReversed[0] | 0x40;
                }
                else {
                    prvKeyReversed[privateKeyLen - 1] = prvKeyReversed[privateKeyLen - 1] & 0xFC;
                    prvKeyReversed[0]                 = prvKeyReversed[0] | 0x80;
                }

                pPrivateKey = &prvKeyReversed[0];

#if SSS_HAVE_MBEDTLS_3_X
                ret = mbedtls_mpi_read_binary(&pEcpPrv->MBEDTLS_PRIVATE(d), pPrivateKey, privateKeyLen);
#else
                ret = mbedtls_mpi_read_binary(&pEcpPrv->d, pPrivateKey, privateKeyLen);
#endif

                ENSURE_OR_GO_EXIT(ret == 0);

#if SSS_HAVE_MBEDTLS_3_X
                ret =
                    mbedtls_mpi_read_binary(&pEcpPrv->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(X), pPublicKey, publicKeyLen);
#else
                ret = mbedtls_mpi_read_binary(&pEcpPrv->Q.X, pPublicKey, publicKeyLen);
#endif

                ENSURE_OR_GO_EXIT(ret == 0);

#if SSS_HAVE_MBEDTLS_3_X
                ret = mbedtls_mpi_lset(&pEcpPrv->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(Z), 1);
#else
                ret = mbedtls_mpi_lset(&pEcpPrv->Q.Z, 1);
#endif
                ENSURE_OR_GO_EXIT(ret == 0);

                retval = kStatus_SSS_Success;
            }
#else
            ret = mbedtls_mpi_read_binary(&pEcpPrv->d, data, dataLen);
            ENSURE_OR_GO_EXIT(ret == 0);
            retval = kStatus_SSS_Success;
#endif
#else
            retval = kStatus_SSS_Fail;
#endif
        }
        else {
#if SSS_HAVE_MBEDTLS_3_X
            ret = mbedtls_pk_parse_key(pk, data, dataLen, NULL, 0, NULL, NULL);
#else
            ret = mbedtls_pk_parse_key(pk, data, dataLen, NULL, 0);
#endif
            (ret == 0) ? (retval = kStatus_SSS_Success) : (retval = kStatus_SSS_Fail);
        }
    } break;
    case kSSS_KeyPart_Public: {
        // Sizeof base64_format should be limited to sizeof(pem_format) minus BEGIN_PUBLIC and END_PUBLIC
        // SIMW-2696.
        uint8_t base64_format[1996] = {0};
        mbedtls_pk_context *pk      = (mbedtls_pk_context *)keyObject->contents;
        if (keyObject->cipherType == kSSS_CipherType_EC_MONTGOMERY) {
#if defined(MBEDTLS_ECP_C)
            mbedtls_ecp_keypair *pEcpPub = NULL;

            ret = mbedtls_pk_setup(pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
            ENSURE_OR_GO_EXIT(ret == 0);

            pEcpPub = mbedtls_pk_ec(*pk);
            ENSURE_OR_GO_EXIT(pEcpPub);
            if (keyBitLen == 256) {
#if SSS_HAVE_MBEDTLS_3_X
                ret = mbedtls_ecp_group_load(&pEcpPub->MBEDTLS_PRIVATE(grp), MBEDTLS_ECP_DP_CURVE25519);
#else
                ret = mbedtls_ecp_group_load(&pEcpPub->grp, MBEDTLS_ECP_DP_CURVE25519);
#endif
            }
            else if (keyBitLen == 448) {
#if SSS_HAVE_MBEDTLS_3_X
                ret = mbedtls_ecp_group_load(&pEcpPub->MBEDTLS_PRIVATE(grp), MBEDTLS_ECP_DP_CURVE448);
#else
                ret = mbedtls_ecp_group_load(&pEcpPub->grp, MBEDTLS_ECP_DP_CURVE448);
#endif
            }
            else {
                ret = 1;
            }
            ENSURE_OR_GO_EXIT(ret == 0);

#ifdef MBEDTLS_DO_LITTLE_ENDIAN // Reverse Endianness
            {
                size_t i                   = 0;
                size_t publicKeyIndex      = 0;
                size_t publicKeyLen        = dataLen;
                size_t nByteKey            = 32; // Corresponds to kSE05x_ECCurve_ECC_MONT_DH_25519
                uint8_t pubKeyReversed[64] = {
                    0,
                };
                const uint8_t *pPublicKey = NULL;
// #define TMP_ENDIAN_VERBOSE
#ifdef TMP_ENDIAN_VERBOSE
                printf("Pub Key Before Reverse & header strip:\n");
                ENSURE_OR_GO_EXIT(dataLen >= (publicKeyIndex + publicKeyLen));
                for (size_t z = 0; z < publicKeyLen; z++) {
                    printf("%02X.", data[publicKeyIndex + z]);
                }
                printf("\n");
                printf("keyBitLen = %d\n", (int)keyBitLen);
#endif
                if (keyBitLen == 256) {
                    publicKeyIndex = der_ecc_mont_dh_25519_header_len;
                    ENSURE_OR_GO_EXIT(publicKeyLen >= der_ecc_mont_dh_25519_header_len);
                    publicKeyLen -= der_ecc_mont_dh_25519_header_len;
                }
                else {
                    nByteKey       = 56;
                    publicKeyIndex = der_ecc_mont_dh_448_header_len;
                    publicKeyLen -= der_ecc_mont_dh_448_header_len;
                }
                ENSURE_OR_GO_EXIT((SIZE_MAX - publicKeyIndex) >= publicKeyLen);
                ENSURE_OR_GO_EXIT(dataLen >= (publicKeyIndex + publicKeyLen));
                while (i < nByteKey) {
                    ENSURE_OR_GO_EXIT((publicKeyIndex + publicKeyLen) > i);
                    pubKeyReversed[i] = data[publicKeyIndex + publicKeyLen - i - 1];
                    i++;
                }
                pPublicKey = &pubKeyReversed[0];

#ifdef TMP_ENDIAN_VERBOSE
                printf("Pub Key After Reverse:\n");
                for (size_t z = 0; z < publicKeyLen; z++) {
                    printf("%02X.", pPublicKey[z]);
                }
                printf("\n");
#endif
#if SSS_HAVE_MBEDTLS_3_X
                ret =
                    mbedtls_mpi_read_binary(&pEcpPub->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(X), pPublicKey, publicKeyLen);
#else
                ret = mbedtls_mpi_read_binary(&pEcpPub->Q.X, pPublicKey, publicKeyLen);
#endif
            }
#else
            ret = mbedtls_mpi_read_binary(&pEcpPub->Q.X, data, dataLen);
#endif // Reverse Endianess

            (ret == 0) ? (retval = kStatus_SSS_Success) : (retval = kStatus_SSS_Fail);

            if (retval == kStatus_SSS_Success) {
#if SSS_HAVE_MBEDTLS_3_X
                ret = mbedtls_mpi_lset(&pEcpPub->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(Z), 1);
#else
                ret = mbedtls_mpi_lset(&pEcpPub->Q.Z, 1);
#endif
                (ret == 0) ? (retval = kStatus_SSS_Success) : (retval = kStatus_SSS_Fail);
            }
#else
            retval = kStatus_SSS_Fail;
#endif
        }
        else {
            ret = mbedtls_base64_encode(base64_format, sizeof(base64_format), &base64_olen, data, dataLen);
            if (SNPRINTF(pem_format, sizeof(pem_format), BEGIN_PUBLIC "%s" END_PUBLIC, base64_format) < 0) {
                retval = kStatus_SSS_Fail;
                goto exit;
            }
            ret = mbedtls_pk_parse_public_key(pk, (const uint8_t *)pem_format, strlen(pem_format) + 1);
            (ret == 0) ? (retval = kStatus_SSS_Success) : (retval = kStatus_SSS_Fail);
        }
    } break;
#endif // SSSFTR_SW_ECC || SSSFTR_SW_RSA
    default:
        retval = kStatus_SSS_Fail;
        LOG_E("Key type not supported");
        break;
    }
exit:
    return retval;
}

#if SSS_HAVE_MBEDTLS_3_X
int mbedtls_entropy_func_3_X(void *data, unsigned char *output, size_t len)
{
    LOG_W(
        "mbedtls_entropy_func_3_X is a dummy implementation with hardcoded entropy. Mandatory to port it to the Micro "
        "Controller being used.");
    unsigned char buf[MBEDTLS_ENTROPY_BLOCK_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    if (len > MBEDTLS_ENTROPY_BLOCK_SIZE) {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }
    memcpy(output, buf, len);
    return 0;
}
#endif

static sss_status_t sss_mbedtls_drbg_seed(sss_mbedtls_session_t *pSession, const char *pers, size_t persLen)
{
    int ret;
    sss_status_t retval = kStatus_SSS_Fail;
#if AX_EMBEDDED && SSS_HAVE_MBEDTLS_3_X
    ret = mbedtls_ctr_drbg_seed(
        pSession->ctr_drbg, &mbedtls_entropy_func_3_X, pSession->entropy, (const unsigned char *)pers, persLen);
#else
    ret = mbedtls_ctr_drbg_seed(
        pSession->ctr_drbg, &mbedtls_entropy_func, pSession->entropy, (const unsigned char *)pers, persLen);
#endif
    ENSURE_OR_GO_EXIT(ret == 0);
    retval = kStatus_SSS_Success;
exit:
    return (retval);
}

#if SSSFTR_SW_ECC && SSS_HAVE_TESTCOUNTERPART && defined(MBEDTLS_ECP_C)
static mbedtls_ecp_group_id get_nist_p_group_id(size_t keyBitLen)
{
    mbedtls_ecp_group_id groupId = MBEDTLS_ECP_DP_NONE;
    switch (keyBitLen) {
    case 192:
        groupId = MBEDTLS_ECP_DP_SECP192R1;
        break;
    case 224:
        groupId = MBEDTLS_ECP_DP_SECP224R1;
        break;
    case 256:
        groupId = MBEDTLS_ECP_DP_SECP256R1;
        break;
    case 384:
        groupId = MBEDTLS_ECP_DP_SECP384R1;
        break;
    case 521:
        groupId = MBEDTLS_ECP_DP_SECP521R1;
        break;
    default:
        break;
    }
    return groupId;
}

static mbedtls_ecp_group_id get_bp_group_id(size_t keyBitLen)
{
    mbedtls_ecp_group_id groupId = MBEDTLS_ECP_DP_NONE;
    switch (keyBitLen) {
    case 256:
        groupId = MBEDTLS_ECP_DP_BP256R1;
        break;
    case 384:
        groupId = MBEDTLS_ECP_DP_BP384R1;
        break;
    case 512:
        groupId = MBEDTLS_ECP_DP_BP512R1;
        break;
    default:
        break;
    }
    return groupId;
}

static mbedtls_ecp_group_id get_nist_k_group_id(size_t keyBitLen)
{
    mbedtls_ecp_group_id groupId = MBEDTLS_ECP_DP_NONE;
    switch (keyBitLen) {
    case 192:
        groupId = MBEDTLS_ECP_DP_SECP192K1;
        break;
    case 224:
        groupId = MBEDTLS_ECP_DP_SECP224K1;
        break;
    case 256:
        groupId = MBEDTLS_ECP_DP_SECP256K1;
        break;
    default:
        break;
    }
    return groupId;
}

static mbedtls_ecp_group_id get_mont_group_id(size_t keyBitLen)
{
    mbedtls_ecp_group_id groupId = MBEDTLS_ECP_DP_NONE;
    switch (keyBitLen) {
    case 256:
        groupId = MBEDTLS_ECP_DP_CURVE25519;
        break;
    case 448:
        groupId = MBEDTLS_ECP_DP_CURVE448;
        break;
    default:
        break;
    }
    return groupId;
}

static sss_status_t sss_mbedtls_generate_ecp_key(
    mbedtls_pk_context *pkey, sss_mbedtls_session_t *pSession, size_t keyBitLen, sss_cipher_type_t cipher_typ)
{
    sss_status_t retval = kStatus_SSS_Fail;
    int ret;
    mbedtls_ecp_group_id groupId = MBEDTLS_ECP_DP_NONE;

    ret = mbedtls_pk_setup(pkey, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    ENSURE_OR_GO_EXIT(ret == 0);
    retval = kStatus_SSS_Success;

    if (cipher_typ == kSSS_CipherType_EC_NIST_P) {
        groupId = get_nist_p_group_id(keyBitLen);
    }
    else if (cipher_typ == kSSS_CipherType_EC_BRAINPOOL) {
        groupId = get_bp_group_id(keyBitLen);
    }
    else if (cipher_typ == kSSS_CipherType_EC_NIST_K) {
        groupId = get_nist_k_group_id(keyBitLen);
    }
    else if (cipher_typ == kSSS_CipherType_EC_MONTGOMERY) {
        groupId = get_mont_group_id(keyBitLen);
    }
    else {
        LOG_E(" sss_openssl_generate_ecp_key: Invalid key type ");
    }

    if (groupId != MBEDTLS_ECP_DP_NONE) {
        ret = mbedtls_ecp_gen_key(groupId, mbedtls_pk_ec(*pkey), mbedtls_ctr_drbg_random, pSession->ctr_drbg);
    }
    else {
        LOG_E(" Don't have support for this keyBitLen");
        ret = 1;
    }

    if (ret != 0) {
        LOG_E(" mbedtls_ecp_gen_key returned -0x%04x", -ret);
        retval = kStatus_SSS_Fail;
        goto exit;
    }
exit:
    return retval;
}
#endif // SSSFTR_SW_ECC && defined(MBEDTLS_ECP_C)

#if SSSFTR_SW_RSA && SSS_HAVE_TESTCOUNTERPART && defined(MBEDTLS_RSA_C)
static sss_status_t sss_mbedtls_generate_rsa_key(
    mbedtls_pk_context *pkey, sss_mbedtls_session_t *pSession, size_t keyBitLen)
{
    int ret;
    sss_status_t retval = kStatus_SSS_Fail;

    ret = mbedtls_pk_setup(pkey, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    ENSURE_OR_GO_EXIT(ret == 0);
    ENSURE_OR_GO_EXIT(keyBitLen == 512 || keyBitLen == 1024 || keyBitLen == 1152 || keyBitLen == 2048 ||
                      keyBitLen == 3072 || keyBitLen == 4096);

    ret = mbedtls_rsa_gen_key(
        mbedtls_pk_rsa(*pkey), mbedtls_ctr_drbg_random, (pSession->ctr_drbg), (unsigned int)keyBitLen, 65537);

    ENSURE_OR_GO_EXIT(ret == 0);
    retval = kStatus_SSS_Success;
exit:
    return retval;
}
#endif

#if SSSFTR_SW_TESTCOUNTERPART
static sss_status_t sss_mbedtls_hkdf_expand(const mbedtls_md_info_t *md,
    const uint8_t *prk,
    size_t prk_len,
    const uint8_t *info,
    size_t info_len,
    uint8_t *okm,
    size_t okm_len)
{
    size_t hash_len;
    size_t N;
    size_t T_len = 0, where = 0, i;
    int ret;
    mbedtls_md_context_t ctx;
    unsigned char T[MBEDTLS_MD_MAX_SIZE] = {0};
    sss_status_t retval                  = kStatus_SSS_Success;

    if (okm == NULL) {
        retval = kStatus_SSS_InvalidArgument;
        goto exit;
    }

    hash_len = mbedtls_md_get_size(md);

    if (hash_len == 0) {
        retval = kStatus_SSS_Fail;
        goto exit;
    }

    if (info == NULL) {
        info = (const unsigned char *)"";
    }

    N = okm_len / hash_len;

    if ((okm_len % hash_len) != 0) {
        if ((SIZE_MAX - N) < 1) {
            retval = kStatus_SSS_Fail;
            goto exit;
        }
        N++;
    }

    if (N > 255) {
        retval = kStatus_SSS_InvalidArgument;
        goto exit;
    }

    mbedtls_md_init(&ctx);

    if ((ret = mbedtls_md_setup(&ctx, md, 1)) != 0) {
        mbedtls_md_free(&ctx);
        retval = kStatus_SSS_Fail;
        goto exit;
    }

    /* Section 2.3. */
    for (i = 1; i <= N; i++) {
        unsigned char c = (unsigned char)i;

        ret = mbedtls_md_hmac_starts(&ctx, prk, prk_len) || mbedtls_md_hmac_update(&ctx, T, T_len) ||
              mbedtls_md_hmac_update(&ctx, info, info_len) ||
              /* The constant concatenated to the end of each T(n) is a single
            octet. */
              mbedtls_md_hmac_update(&ctx, &c, 1) || mbedtls_md_hmac_finish(&ctx, T);

        if (ret != 0) {
            mbedtls_md_free(&ctx);
            retval = kStatus_SSS_Fail;
            goto exit;
        }

        memcpy(okm + where, T, (i != N) ? hash_len : (okm_len - where));
        where += hash_len;
        T_len = hash_len;
    }

    mbedtls_md_free(&ctx);
exit:
    return retval;
}

static sss_status_t sss_mbedtls_hkdf_extract(
    const mbedtls_md_info_t *md, const uint8_t *salt, size_t salt_len, const uint8_t *ikm, size_t ikm_len, uint8_t *prk)
{
    int hash_len;
    int ret;
    unsigned char null_salt[MBEDTLS_MD_MAX_SIZE] = {'\0'};
    sss_status_t retval                          = kStatus_SSS_Success;

    hash_len = mbedtls_md_get_size(md);

    if (salt == NULL) {
        salt     = null_salt;
        salt_len = hash_len;
    }

    ret = mbedtls_md_hmac(md, salt, salt_len, ikm, ikm_len, prk);
    if (ret != 0) {
        retval = kStatus_SSS_Fail;
    }
    return retval;
}
#endif // SSSFTR_SW_TESTCOUNTERPART

/* Low level implementation for sss_mbedtls_key_object_allocate_handle */
sss_status_t ks_mbedtls_key_object_create(sss_mbedtls_object_t *keyObject,
    uint32_t keyId,
    sss_key_part_t keyPart,
    sss_cipher_type_t cipherType,
    size_t keyByteLenMax,
    uint32_t keyMode)
{
    size_t size         = 0;
    sss_status_t retval = kStatus_SSS_Fail;
    ENSURE_OR_GO_CLEANUP(keyObject);

    keyObject->keyId              = keyId;
    keyObject->objectType         = keyPart;
    keyObject->cipherType         = cipherType;
    keyObject->contents_max_size  = keyByteLenMax;
    keyObject->contents_must_free = 1;
    keyObject->keyMode            = keyMode;
    /* Bitwise OR of all sss_access_permission. */
    keyObject->accessRights = kAccessPermission_SSS_All_Permission;
    switch (keyPart) {
    case kSSS_KeyPart_Default:
        size = keyByteLenMax;
        break;
#if SSSFTR_SW_ECC || SSSFTR_SW_RSA
    case kSSS_KeyPart_Pair:
    case kSSS_KeyPart_Private:
    case kSSS_KeyPart_Public:
        size = sizeof(mbedtls_pk_context);
        break;
#endif // SSSFTR_SW_ECC || SSSFTR_SW_RSA
    default:
        break;
    }
    if (size != 0) {
        keyObject->contents           = SSS_MALLOC(size);
        keyObject->contents_must_free = 1;
        ENSURE_OR_GO_CLEANUP(keyObject->contents);
        memset(keyObject->contents, 0, size);
        retval = kStatus_SSS_Success;
    }

cleanup:
    return retval;
}

#endif /* SSS_HAVE_HOSTCRYPTO_MBEDTLS */
