/*
 *
 * Copyright 2018,2019,2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef fsl_sss_user_apis_H
#define fsl_sss_user_apis_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if defined(SSS_USE_FTR_FILE)
#include "fsl_sss_ftr.h"
#else
#include "fsl_sss_ftr_default.h"
#endif

#if SSS_HAVE_HOSTCRYPTO_USER
#include <fsl_sss_user_types.h>

/* ************************************************************************** */
/* Functions                                                                  */
/* ************************************************************************** */
/**
 * @addtogroup sss_user_impl_session
 * @{
 */
/** @copydoc sss_session_create
 *
 */
sss_status_t sss_user_impl_session_create(sss_user_impl_session_t *session,
    sss_type_t subsystem,
    uint32_t application_id,
    sss_connection_type_t connetion_type,
    void *connectionData);

/** @copydoc sss_session_open
 *
 */
sss_status_t sss_user_impl_session_open(sss_user_impl_session_t *session,
    sss_type_t subsystem,
    uint32_t application_id,
    sss_connection_type_t connetion_type,
    void *connectionData);

/** @copydoc sss_session_prop_get_u32
 *
 */
sss_status_t sss_user_impl_session_prop_get_u32(sss_user_impl_session_t *session, uint32_t property, uint32_t *pValue);

/** @copydoc sss_session_prop_get_au8
 *
 */
sss_status_t sss_user_impl_session_prop_get_au8(
    sss_user_impl_session_t *session, uint32_t property, uint8_t *pValue, size_t *pValueLen);

/** @copydoc sss_session_close
 *
 */
void sss_user_impl_session_close(sss_user_impl_session_t *session);

/** @copydoc sss_session_delete
 *
 */
void sss_user_impl_session_delete(sss_user_impl_session_t *session);

/*! @} */ /* end of : sss_user_impl_session */

/**
 * @addtogroup sss_user_impl_keyobj
 * @{
 */
/** @copydoc sss_key_object_init
 *
 */
sss_status_t sss_user_impl_key_object_init(sss_user_impl_object_t *keyObject, sss_user_impl_key_store_t *keyStore);

/** @copydoc sss_key_object_allocate_handle
 *
 */
sss_status_t sss_user_impl_key_object_allocate_handle(sss_user_impl_object_t *keyObject,
    uint32_t keyId,
    sss_key_part_t keyPart,
    sss_cipher_type_t cipherType,
    size_t keyByteLenMax,
    uint32_t options);

/** @copydoc sss_key_object_get_handle
 *
 */
sss_status_t sss_user_impl_key_object_get_handle(sss_user_impl_object_t *keyObject, uint32_t keyId);

/** @copydoc sss_key_object_set_user
 *
 */
sss_status_t sss_user_impl_key_object_set_user(sss_user_impl_object_t *keyObject, uint32_t user, uint32_t options);

/** @copydoc sss_key_object_set_purpose
 *
 */
sss_status_t sss_user_impl_key_object_set_purpose(
    sss_user_impl_object_t *keyObject, sss_mode_t purpose, uint32_t options);

/** @copydoc sss_key_object_set_access
 *
 */
sss_status_t sss_user_impl_key_object_set_access(sss_user_impl_object_t *keyObject, uint32_t access, uint32_t options);

/** @copydoc sss_key_object_set_eccgfp_group
 *
 */
sss_status_t sss_user_impl_key_object_set_eccgfp_group(sss_user_impl_object_t *keyObject, sss_eccgfp_group_t *group);

/** @copydoc sss_key_object_get_user
 *
 */
sss_status_t sss_user_impl_key_object_get_user(sss_user_impl_object_t *keyObject, uint32_t *user);

/** @copydoc sss_key_object_get_purpose
 *
 */
sss_status_t sss_user_impl_key_object_get_purpose(sss_user_impl_object_t *keyObject, sss_mode_t *purpose);

/** @copydoc sss_key_object_get_access
 *
 */
sss_status_t sss_user_impl_key_object_get_access(sss_user_impl_object_t *keyObject, uint32_t *access);

/** @copydoc sss_key_object_free
 *
 */
void sss_user_impl_key_object_free(sss_user_impl_object_t *keyObject);

/*! @} */ /* end of : sss_user_impl_keyobj */

/**
 * @addtogroup sss_user_impl_keyderive
 * @{
 */
/** @copydoc sss_derive_key_context_init
 *
 */
sss_status_t sss_user_impl_derive_key_context_init(sss_user_impl_derive_key_t *context,
    sss_user_impl_session_t *session,
    sss_user_impl_object_t *keyObject,
    sss_algorithm_t algorithm,
    sss_mode_t mode);

/** @copydoc sss_derive_key_go
 *
 */
sss_status_t sss_user_impl_derive_key_go(sss_user_impl_derive_key_t *context,
    const uint8_t *saltData,
    size_t saltLen,
    const uint8_t *info,
    size_t infoLen,
    sss_user_impl_object_t *derivedKeyObject,
    uint16_t deriveDataLen,
    uint8_t *hkdfOutput,
    size_t *hkdfOutputLen);

/** @copydoc sss_derive_key_dh
 *
 */
sss_status_t sss_user_impl_derive_key_dh(sss_user_impl_derive_key_t *context,
    sss_user_impl_object_t *otherPartyKeyObject,
    sss_user_impl_object_t *derivedKeyObject);

/** @copydoc sss_derive_key_context_free
 *
 */
void sss_user_impl_derive_key_context_free(sss_user_impl_derive_key_t *context);

/*! @} */ /* end of : sss_user_impl_keyderive */

/**
 * @addtogroup sss_user_impl_keystore
 * @{
 */
/** @copydoc sss_key_store_context_init
 *
 */
sss_status_t sss_user_impl_key_store_context_init(
    sss_user_impl_key_store_t *keyStore, sss_user_impl_session_t *session);

/** @copydoc sss_key_store_allocate
 *
 */
sss_status_t sss_user_impl_key_store_allocate(sss_user_impl_key_store_t *keyStore, uint32_t keyStoreId);

/** @copydoc sss_key_store_save
 *
 */
sss_status_t sss_user_impl_key_store_save(sss_user_impl_key_store_t *keyStore);

/** @copydoc sss_key_store_load
 *
 */
sss_status_t sss_user_impl_key_store_load(sss_user_impl_key_store_t *keyStore);

/** @copydoc sss_key_store_set_key
 *
 */
sss_status_t sss_user_impl_key_store_set_key(sss_user_impl_key_store_t *keyStore,
    sss_user_impl_object_t *keyObject,
    const uint8_t *data,
    size_t dataLen,
    size_t keyBitLen,
    void *options,
    size_t optionsLen);

/** @copydoc sss_key_store_generate_key
 *
 */
sss_status_t sss_user_impl_key_store_generate_key(
    sss_user_impl_key_store_t *keyStore, sss_user_impl_object_t *keyObject, size_t keyBitLen, void *options);

/** @copydoc sss_key_store_get_key
 *
 */
sss_status_t sss_user_impl_key_store_get_key(sss_user_impl_key_store_t *keyStore,
    sss_user_impl_object_t *keyObject,
    uint8_t *data,
    size_t *dataLen,
    size_t *pKeyBitLen);

/** @copydoc sss_key_store_open_key
 *
 */
sss_status_t sss_user_impl_key_store_open_key(sss_user_impl_key_store_t *keyStore, sss_user_impl_object_t *keyObject);

/** @copydoc sss_key_store_freeze_key
 *
 */
sss_status_t sss_user_impl_key_store_freeze_key(sss_user_impl_key_store_t *keyStore, sss_user_impl_object_t *keyObject);

/** @copydoc sss_key_store_erase_key
 *
 */
sss_status_t sss_user_impl_key_store_erase_key(sss_user_impl_key_store_t *keyStore, sss_user_impl_object_t *keyObject);

/** @copydoc sss_key_store_prop_get_u32
 *
 */
sss_status_t sss_user_impl_key_store_prop_get_u32(
    sss_user_impl_key_store_t *session, uint32_t property, uint32_t *pValue);

/** @copydoc sss_key_store_prop_get_au8
 *
 */
sss_status_t sss_user_impl_key_store_prop_get_au8(
    sss_user_impl_key_store_t *session, uint32_t property, uint8_t *pValue, size_t *pValueLen);

/** @copydoc sss_key_store_context_free
 *
 */
void sss_user_impl_key_store_context_free(sss_user_impl_key_store_t *keyStore);

/*! @} */ /* end of : sss_user_impl_keystore */

/**
 * @addtogroup sss_user_impl_asym
 * @{
 */
/** @copydoc sss_asymmetric_context_init
 *
 */
sss_status_t sss_user_impl_asymmetric_context_init(sss_user_impl_asymmetric_t *context,
    sss_user_impl_session_t *session,
    sss_user_impl_object_t *keyObject,
    sss_algorithm_t algorithm,
    sss_mode_t mode);

/** @copydoc sss_asymmetric_encrypt
 *
 */
sss_status_t sss_user_impl_asymmetric_encrypt(
    sss_user_impl_asymmetric_t *context, const uint8_t *srcData, size_t srcLen, uint8_t *destData, size_t *destLen);

/** @copydoc sss_asymmetric_decrypt
 *
 */
sss_status_t sss_user_impl_asymmetric_decrypt(
    sss_user_impl_asymmetric_t *context, const uint8_t *srcData, size_t srcLen, uint8_t *destData, size_t *destLen);

/** @copydoc sss_asymmetric_sign_digest
 *
 */
sss_status_t sss_user_impl_asymmetric_sign_digest(
    sss_user_impl_asymmetric_t *context, uint8_t *digest, size_t digestLen, uint8_t *signature, size_t *signatureLen);

/** @copydoc sss_asymmetric_verify_digest
 *
 */
sss_status_t sss_user_impl_asymmetric_verify_digest(
    sss_user_impl_asymmetric_t *context, uint8_t *digest, size_t digestLen, uint8_t *signature, size_t signatureLen);

/** @copydoc sss_asymmetric_context_free
 *
 */
void sss_user_impl_asymmetric_context_free(sss_user_impl_asymmetric_t *context);

/*! @} */ /* end of : sss_user_impl_asym */

/**
 * @addtogroup sss_user_impl_symm
 * @{
 */
/** @copydoc sss_symmetric_context_init
 *
 */
sss_status_t sss_user_impl_symmetric_context_init(sss_user_impl_symmetric_t *context,
    sss_user_impl_session_t *session,
    sss_user_impl_object_t *keyObject,
    sss_algorithm_t algorithm,
    sss_mode_t mode);

/** @copydoc sss_cipher_one_go
 *
 */
sss_status_t sss_user_impl_cipher_one_go(sss_user_impl_symmetric_t *context,
    uint8_t *iv,
    size_t ivLen,
    const uint8_t *srcData,
    uint8_t *destData,
    size_t dataLen);

/** @copydoc sss_cipher_init
 *
 */
sss_status_t sss_user_impl_cipher_init(sss_user_impl_symmetric_t *context, uint8_t *iv, size_t ivLen);

/** @copydoc sss_cipher_update
 *
 */
sss_status_t sss_user_impl_cipher_update(
    sss_user_impl_symmetric_t *context, const uint8_t *srcData, size_t srcLen, uint8_t *destData, size_t *destLen);

/** @copydoc sss_cipher_finish
 *
 */
sss_status_t sss_user_impl_cipher_finish(
    sss_user_impl_symmetric_t *context, const uint8_t *srcData, size_t srcLen, uint8_t *destData, size_t *destLen);

/** @copydoc sss_cipher_crypt_ctr
 *
 */
sss_status_t sss_user_impl_cipher_crypt_ctr(sss_user_impl_symmetric_t *context,
    const uint8_t *srcData,
    uint8_t *destData,
    size_t size,
    uint8_t *initialCounter,
    uint8_t *lastEncryptedCounter,
    size_t *szLeft);

/** @copydoc sss_symmetric_context_free
 *
 */
void sss_user_impl_symmetric_context_free(sss_user_impl_symmetric_t *context);

/*! @} */ /* end of : sss_user_impl_symm */

/**
 * @addtogroup sss_user_impl_aead
 * @{
 */

/**
 * @addtogroup sss_user_impl_mac
 * @{
 */
/** @copydoc sss_mac_context_init
 *
 */
sss_status_t sss_user_impl_mac_context_init(sss_user_impl_mac_t *context,
    sss_user_impl_session_t *session,
    sss_user_impl_object_t *keyObject,
    sss_algorithm_t algorithm,
    sss_mode_t mode);

/** @copydoc sss_mac_one_go
 *
 */
sss_status_t sss_user_impl_mac_one_go(
    sss_user_impl_mac_t *context, const uint8_t *message, size_t messageLen, uint8_t *mac, size_t *macLen);

/** @copydoc sss_mac_init
 *
 */
sss_status_t sss_user_impl_mac_init(sss_user_impl_mac_t *context);

/** @copydoc sss_mac_update
 *
 */
sss_status_t sss_user_impl_mac_update(sss_user_impl_mac_t *context, const uint8_t *message, size_t messageLen);

/** @copydoc sss_mac_finish
 *
 */
sss_status_t sss_user_impl_mac_finish(sss_user_impl_mac_t *context, uint8_t *mac, size_t *macLen);

/** @copydoc sss_mac_context_free
 *
 */
void sss_user_impl_mac_context_free(sss_user_impl_mac_t *context);

/*! @} */ /* end of : sss_user_impl_mac */

/**
 * @addtogroup sss_user_impl_md
 * @{
 */
/** @copydoc sss_digest_context_init
 *
 */
sss_status_t sss_user_impl_digest_context_init(
    sss_user_impl_digest_t *context, sss_user_impl_session_t *session, sss_algorithm_t algorithm, sss_mode_t mode);

/** @copydoc sss_digest_one_go
 *
 */
sss_status_t sss_user_impl_digest_one_go(
    sss_user_impl_digest_t *context, const uint8_t *message, size_t messageLen, uint8_t *digest, size_t *digestLen);

/** @copydoc sss_digest_init
 *
 */
sss_status_t sss_user_impl_digest_init(sss_user_impl_digest_t *context);

/** @copydoc sss_digest_update
 *
 */
sss_status_t sss_user_impl_digest_update(sss_user_impl_digest_t *context, const uint8_t *message, size_t messageLen);

/** @copydoc sss_digest_finish
 *
 */
sss_status_t sss_user_impl_digest_finish(sss_user_impl_digest_t *context, uint8_t *digest, size_t *digestLen);

/** @copydoc sss_digest_context_free
 *
 */
void sss_user_impl_digest_context_free(sss_user_impl_digest_t *context);

/*! @} */ /* end of : sss_user_impl_md */

/**
 * @addtogroup sss_user_impl_rng
 * @{
 */
/** @copydoc sss_rng_context_init
 *
 */
sss_status_t sss_user_impl_rng_context_init(sss_user_impl_rng_context_t *context, sss_user_impl_session_t *session);

/** @copydoc sss_rng_get_random
 *
 */
sss_status_t sss_user_impl_rng_get_random(sss_user_impl_rng_context_t *context, uint8_t *random_data, size_t dataLen);

/** @copydoc sss_rng_context_free
 *
 */
sss_status_t sss_user_impl_rng_context_free(sss_user_impl_rng_context_t *context);

/*! @} */ /* end of : sss_user_impl_rng */

/* clang-format off */

        /* Host Call : session */
#       define sss_host_session_create(session,subsystem,application_id,connetion_type,connectionData) \
            sss_user_impl_session_create(((sss_user_impl_session_t * ) session),(subsystem),(application_id),(connetion_type),(connectionData))
#       define sss_host_session_open(session,subsystem,application_id,connetion_type,connectionData) \
            sss_user_impl_session_open(((sss_user_impl_session_t * ) session),(subsystem),(application_id),(connetion_type),(connectionData))
#       define sss_host_session_prop_get_u32(session,property,pValue) \
            sss_user_impl_session_prop_get_u32(((sss_user_impl_session_t * ) session),(property),(pValue))
#       define sss_host_session_prop_get_au8(session,property,pValue,pValueLen) \
            sss_user_impl_session_prop_get_au8(((sss_user_impl_session_t * ) session),(property),(pValue),(pValueLen))
#       define sss_host_session_close(session) \
            sss_user_impl_session_close(((sss_user_impl_session_t * ) session))
#       define sss_host_session_delete(session) \
            sss_user_impl_session_delete(((sss_user_impl_session_t * ) session))
        /* Host Call : keyobj */
#       define sss_host_key_object_init(keyObject,keyStore) \
            sss_user_impl_key_object_init(((sss_user_impl_object_t * ) keyObject),((sss_user_impl_key_store_t * ) keyStore))
#       define sss_host_key_object_allocate_handle(keyObject,keyId,keyPart,cipherType,keyByteLenMax,options) \
            sss_user_impl_key_object_allocate_handle(((sss_user_impl_object_t * ) keyObject),(keyId),(keyPart),(cipherType),(keyByteLenMax),(options))
#       define sss_host_key_object_get_handle(keyObject,keyId) \
            sss_user_impl_key_object_get_handle(((sss_user_impl_object_t * ) keyObject),(keyId))
#       define sss_host_key_object_set_user(keyObject,user,options) \
            sss_user_impl_key_object_set_user(((sss_user_impl_object_t * ) keyObject),(user),(options))
#       define sss_host_key_object_set_purpose(keyObject,purpose,options) \
            sss_user_impl_key_object_set_purpose(((sss_user_impl_object_t * ) keyObject),(purpose),(options))
#       define sss_host_key_object_set_access(keyObject,access,options) \
            sss_user_impl_key_object_set_access(((sss_user_impl_object_t * ) keyObject),(access),(options))
#       define sss_host_key_object_set_eccgfp_group(keyObject,group) \
            sss_user_impl_key_object_set_eccgfp_group(((sss_user_impl_object_t * ) keyObject),(group))
#       define sss_host_key_object_get_user(keyObject,user) \
            sss_user_impl_key_object_get_user(((sss_user_impl_object_t * ) keyObject),(user))
#       define sss_host_key_object_get_purpose(keyObject,purpose) \
            sss_user_impl_key_object_get_purpose(((sss_user_impl_object_t * ) keyObject),(purpose))
#       define sss_host_key_object_get_access(keyObject,access) \
            sss_user_impl_key_object_get_access(((sss_user_impl_object_t * ) keyObject),(access))
#       define sss_host_key_object_free(keyObject) \
            sss_user_impl_key_object_free(((sss_user_impl_object_t * ) keyObject))
        /* Host Call : keyderive */
#       define sss_host_derive_key_context_init(context,session,keyObject,algorithm,mode) \
            sss_user_impl_derive_key_context_init(((sss_user_impl_derive_key_t * ) context),((sss_user_impl_session_t * ) session),((sss_user_impl_object_t * ) keyObject),(algorithm),(mode))
#       define sss_host_derive_key_go(context,saltData,saltLen,info,infoLen,derivedKeyObject,deriveDataLen,hkdfOutput,hkdfOutputLen) \
            sss_user_impl_derive_key_go(((sss_user_impl_derive_key_t * ) context),(saltData),(saltLen),(info),(infoLen),((sss_user_impl_object_t * ) derivedKeyObject),(deriveDataLen),(hkdfOutput),(hkdfOutputLen))
#       define sss_host_derive_key_dh(context,otherPartyKeyObject,derivedKeyObject) \
            sss_user_impl_derive_key_dh(((sss_user_impl_derive_key_t * ) context),((sss_user_impl_object_t * ) otherPartyKeyObject),((sss_user_impl_object_t * ) derivedKeyObject))
#       define sss_host_derive_key_context_free(context) \
            sss_user_impl_derive_key_context_free(((sss_user_impl_derive_key_t * ) context))
        /* Host Call : keystore */
#       define sss_host_key_store_context_init(keyStore,session) \
            sss_user_impl_key_store_context_init(((sss_user_impl_key_store_t * ) keyStore),((sss_user_impl_session_t * ) session))
#       define sss_host_key_store_allocate(keyStore,keyStoreId) \
            sss_user_impl_key_store_allocate(((sss_user_impl_key_store_t * ) keyStore),(keyStoreId))
#       define sss_host_key_store_save(keyStore) \
            sss_user_impl_key_store_save(((sss_user_impl_key_store_t * ) keyStore))
#       define sss_host_key_store_load(keyStore) \
            sss_user_impl_key_store_load(((sss_user_impl_key_store_t * ) keyStore))
#       define sss_host_key_store_set_key(keyStore,keyObject,data,dataLen,keyBitLen,options,optionsLen) \
            sss_user_impl_key_store_set_key(((sss_user_impl_key_store_t * ) keyStore),((sss_user_impl_object_t * ) keyObject),(data),(dataLen),(keyBitLen),(options),(optionsLen))
#       define sss_host_key_store_generate_key(keyStore,keyObject,keyBitLen,options) \
            sss_user_impl_key_store_generate_key(((sss_user_impl_key_store_t * ) keyStore),((sss_user_impl_object_t * ) keyObject),(keyBitLen),(options))
#       define sss_host_key_store_get_key(keyStore,keyObject,data,dataLen,pKeyBitLen) \
            sss_user_impl_key_store_get_key(((sss_user_impl_key_store_t * ) keyStore),((sss_user_impl_object_t * ) keyObject),(data),(dataLen),(pKeyBitLen))
#       define sss_host_key_store_open_key(keyStore,keyObject) \
            sss_user_impl_key_store_open_key(((sss_user_impl_key_store_t * ) keyStore),((sss_user_impl_object_t * ) keyObject))
#       define sss_host_key_store_freeze_key(keyStore,keyObject) \
            sss_user_impl_key_store_freeze_key(((sss_user_impl_key_store_t * ) keyStore),((sss_user_impl_object_t * ) keyObject))
#       define sss_host_key_store_erase_key(keyStore,keyObject) \
            sss_user_impl_key_store_erase_key(((sss_user_impl_key_store_t * ) keyStore),((sss_user_impl_object_t * ) keyObject))
#       define sss_host_key_store_prop_get_u32(session,property,pValue) \
            sss_user_impl_key_store_prop_get_u32(((sss_user_impl_key_store_t * ) session),(property),(pValue))
#       define sss_host_key_store_prop_get_au8(session,property,pValue,pValueLen) \
            sss_user_impl_key_store_prop_get_au8(((sss_user_impl_key_store_t * ) session),(property),(pValue),(pValueLen))
#       define sss_host_key_store_context_free(keyStore) \
            sss_user_impl_key_store_context_free(((sss_user_impl_key_store_t * ) keyStore))
        /* Host Call : asym */
#       define sss_host_asymmetric_context_init(context,session,keyObject,algorithm,mode) \
            sss_user_impl_asymmetric_context_init(((sss_user_impl_asymmetric_t * ) context),((sss_user_impl_session_t * ) session),((sss_user_impl_object_t * ) keyObject),(algorithm),(mode))
#       define sss_host_asymmetric_encrypt(context,srcData,srcLen,destData,destLen) \
            sss_user_impl_asymmetric_encrypt(((sss_user_impl_asymmetric_t * ) context),(srcData),(srcLen),(destData),(destLen))
#       define sss_host_asymmetric_decrypt(context,srcData,srcLen,destData,destLen) \
            sss_user_impl_asymmetric_decrypt(((sss_user_impl_asymmetric_t * ) context),(srcData),(srcLen),(destData),(destLen))
#       define sss_host_asymmetric_sign_digest(context,digest,digestLen,signature,signatureLen) \
            sss_user_impl_asymmetric_sign_digest(((sss_user_impl_asymmetric_t * ) context),(digest),(digestLen),(signature),(signatureLen))
#       define sss_host_asymmetric_verify_digest(context,digest,digestLen,signature,signatureLen) \
            sss_user_impl_asymmetric_verify_digest(((sss_user_impl_asymmetric_t * ) context),(digest),(digestLen),(signature),(signatureLen))
#       define sss_host_asymmetric_context_free(context) \
            sss_user_impl_asymmetric_context_free(((sss_user_impl_asymmetric_t * ) context))
        /* Host Call : symm */
#       define sss_host_symmetric_context_init(context,session,keyObject,algorithm,mode) \
            sss_user_impl_symmetric_context_init(((sss_user_impl_symmetric_t * ) context),((sss_user_impl_session_t * ) session),((sss_user_impl_object_t * ) keyObject),(algorithm),(mode))
#       define sss_host_cipher_one_go(context,iv,ivLen,srcData,destData,dataLen) \
            sss_user_impl_cipher_one_go(((sss_user_impl_symmetric_t * ) context),(iv),(ivLen),(srcData),(destData),(dataLen))
#       define sss_host_cipher_init(context,iv,ivLen) \
            sss_user_impl_cipher_init(((sss_user_impl_symmetric_t * ) context),(iv),(ivLen))
#       define sss_host_cipher_update(context,srcData,srcLen,destData,destLen) \
            sss_user_impl_cipher_update(((sss_user_impl_symmetric_t * ) context),(srcData),(srcLen),(destData),(destLen))
#       define sss_host_cipher_finish(context,srcData,srcLen,destData,destLen) \
            sss_user_impl_cipher_finish(((sss_user_impl_symmetric_t * ) context),(srcData),(srcLen),(destData),(destLen))
#       define sss_host_cipher_crypt_ctr(context,srcData,destData,size,initialCounter,lastEncryptedCounter,szLeft) \
            sss_user_impl_cipher_crypt_ctr(((sss_user_impl_symmetric_t * ) context),(srcData),(destData),(size),(initialCounter),(lastEncryptedCounter),(szLeft))
#       define sss_host_symmetric_context_free(context) \
            sss_user_impl_symmetric_context_free(((sss_user_impl_symmetric_t * ) context))
        /* Host Call : aead */
#       define sss_host_aead_context_init(context,session,keyObject,algorithm,mode) \
            sss_user_impl_aead_context_init(((sss_user_impl_aead_t * ) context),((sss_user_impl_session_t * ) session),((sss_user_impl_object_t * ) keyObject),(algorithm),(mode))
#       define sss_host_aead_one_go(context,srcData,destData,size,nonce,nonceLen,aad,aadLen,tag,tagLen) \
            sss_user_impl_aead_one_go(((sss_user_impl_aead_t * ) context),(srcData),(destData),(size),(nonce),(nonceLen),(aad),(aadLen),(tag),(tagLen))
#       define sss_host_aead_init(context,nonce,nonceLen,tagLen,aadLen,payloadLen) \
            sss_user_impl_aead_init(((sss_user_impl_aead_t * ) context),(nonce),(nonceLen),(tagLen),(aadLen),(payloadLen))
#       define sss_host_aead_update_aad(context,aadData,aadDataLen) \
            sss_user_impl_aead_update_aad(((sss_user_impl_aead_t * ) context),(aadData),(aadDataLen))
#       define sss_host_aead_update(context,srcData,srcLen,destData,destLen) \
            sss_user_impl_aead_update(((sss_user_impl_aead_t * ) context),(srcData),(srcLen),(destData),(destLen))
#       define sss_host_aead_finish(context,srcData,srcLen,destData,destLen,tag,tagLen) \
            sss_user_impl_aead_finish(((sss_user_impl_aead_t * ) context),(srcData),(srcLen),(destData),(destLen),(tag),(tagLen))
#       define sss_host_aead_context_free(context) \
            sss_user_impl_aead_context_free(((sss_user_impl_aead_t * ) context))
        /* Host Call : mac */
#       define sss_host_mac_context_init(context,session,keyObject,algorithm,mode) \
            sss_user_impl_mac_context_init(((sss_user_impl_mac_t * ) context),((sss_user_impl_session_t * ) session),((sss_user_impl_object_t * ) keyObject),(algorithm),(mode))
#       define sss_host_mac_one_go(context,message,messageLen,mac,macLen) \
            sss_user_impl_mac_one_go(((sss_user_impl_mac_t * ) context),(message),(messageLen),(mac),(macLen))
#       define sss_host_mac_init(context) \
            sss_user_impl_mac_init(((sss_user_impl_mac_t * ) context))
#       define sss_host_mac_update(context,message,messageLen) \
            sss_user_impl_mac_update(((sss_user_impl_mac_t * ) context),(message),(messageLen))
#       define sss_host_mac_finish(context,mac,macLen) \
            sss_user_impl_mac_finish(((sss_user_impl_mac_t * ) context),(mac),(macLen))
#       define sss_host_mac_context_free(context) \
            sss_user_impl_mac_context_free(((sss_user_impl_mac_t * ) context))
        /* Host Call : md */
#       define sss_host_digest_context_init(context,session,algorithm,mode) \
            sss_user_impl_digest_context_init(((sss_user_impl_digest_t * ) context),((sss_user_impl_session_t * ) session),(algorithm),(mode))
#       define sss_host_digest_one_go(context,message,messageLen,digest,digestLen) \
            sss_user_impl_digest_one_go(((sss_user_impl_digest_t * ) context),(message),(messageLen),(digest),(digestLen))
#       define sss_host_digest_init(context) \
            sss_user_impl_digest_init(((sss_user_impl_digest_t * ) context))
#       define sss_host_digest_update(context,message,messageLen) \
            sss_user_impl_digest_update(((sss_user_impl_digest_t * ) context),(message),(messageLen))
#       define sss_host_digest_finish(context,digest,digestLen) \
            sss_user_impl_digest_finish(((sss_user_impl_digest_t * ) context),(digest),(digestLen))
#       define sss_host_digest_context_free(context) \
            sss_user_impl_digest_context_free(((sss_user_impl_digest_t * ) context))
        /* Host Call : rng */
#       define sss_host_rng_context_init(context,session) \
            sss_user_impl_rng_context_init(((sss_user_impl_rng_context_t * ) context),((sss_user_impl_session_t * ) session))
#       define sss_host_rng_get_random(context,random_data,dataLen) \
            sss_user_impl_rng_get_random(((sss_user_impl_rng_context_t * ) context),(random_data),(dataLen))
#       define sss_host_rng_context_free(context) \
            sss_user_impl_rng_context_free(((sss_user_impl_rng_context_t * ) context))

/* clang-format on */
#endif /* SSS_HAVE_HOSTCRYPTO_USER */
#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* fsl_sss_user_apis_H */
