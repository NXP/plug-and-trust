/*
 *
 * Copyright 2018-2020,2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Key store in PC : For testing */

/* ************************************************************************** */
/* Includes                                                                   */
/* ************************************************************************** */

#if defined(SSS_USE_FTR_FILE)
#include "fsl_sss_ftr.h"
#else
#include "fsl_sss_ftr_default.h"
#endif

#include <fsl_sss_keyid_map.h>

#if SSS_HAVE_HOSTCRYPTO_MBEDTLS
#include <fsl_sss_mbedtls_apis.h>
#endif

#if SSS_HAVE_HOSTCRYPTO_OPENSSL
#include <fsl_sss_openssl_types.h>
#endif

#include <nxEnsure.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nxLog_sss.h"
#include "sm_types.h"

#if (defined(MBEDTLS_FS_IO) && !AX_EMBEDDED) || SSS_HAVE_HOSTCRYPTO_OPENSSL

/* ************************************************************************** */
/* Local Defines                                                              */
/* ************************************************************************** */

/* File allocation table file name */
#define FAT_FILENAME "sss_fat.bin"
#define MAX_FILE_NAME_SIZE 255

/* ************************************************************************** */
/* Structures and Typedefs                                                    */
/* ************************************************************************** */

/* ************************************************************************** */
/* Global Variables                                                           */
/* ************************************************************************** */

// keyStoreTable_t gKeyStoreShadow;
// keyIdAndTypeIndexLookup_t gLookupEntires[KS_N_ENTIRES];

/* ************************************************************************** */
/* Static function declarations                                               */
/* ************************************************************************** */

/* ************************************************************************** */
/* Public Functions                                                           */
/* ************************************************************************** */

/* For the key sss_key, what will the file name look like */
void ks_sw_getKeyFileName(
    char *const file_name, const size_t size, const sss_object_t *sss_key, const char *root_folder)
{
    uint32_t keyId      = sss_key->keyId;
    uint16_t keyType    = 0;
    uint16_t cipherType = 0;

    if (sss_key->objectType > UINT16_MAX) {
        LOG_E("objectType should be 2 Bytes.");
        return;
    }
    if (sss_key->cipherType > UINT16_MAX) {
        LOG_E("cipherType should be 2 Bytes.");
        return;
    }
    if (size < 1) {
        LOG_E("size should be a non-zero positive value");
        return;
    }
    keyType    = sss_key->objectType;
    cipherType = sss_key->cipherType;
    if (SNPRINTF(file_name, size - 1, "%s/sss_%08X_%04d_%04d.bin", root_folder, keyId, keyType, cipherType) < 0) {
        LOG_E("snprintf error");
        return;
    }
}

void ks_sw_fat_allocate(keyStoreTable_t **keystore_shadow)
{
    keyIdAndTypeIndexLookup_t *ppLookupEntires = NULL;
    keyStoreTable_t *pKeyStoreShadow           = SSS_MALLOC(sizeof(keyStoreTable_t));
    if (pKeyStoreShadow == NULL) {
        LOG_E("Error in pKeyStoreShadow mem allocation");
        return;
    }
    ppLookupEntires = SSS_MALLOC(KS_N_ENTIRES * sizeof(keyIdAndTypeIndexLookup_t));
    if (ppLookupEntires == NULL) {
        LOG_E("Error in ppLookupEntires mem allocation");
        SSS_FREE(pKeyStoreShadow);
        return;
    }

    //for (int i = 0; i < KS_N_ENTIRES; i++) {
    //    ppLookupEntires[i] = calloc(1, sizeof(keyIdAndTypeIndexLookup_t));
    //}
    memset(ppLookupEntires, 0, (KS_N_ENTIRES * sizeof(keyIdAndTypeIndexLookup_t)));
    ks_common_init_fat(pKeyStoreShadow, ppLookupEntires, KS_N_ENTIRES);
    *keystore_shadow = pKeyStoreShadow;
}

void ks_sw_fat_free(keyStoreTable_t *keystore_shadow)
{
    if (NULL != keystore_shadow) {
        if (NULL != keystore_shadow->entries) {
            //for (int i = 0; i < keystore_shadow->maxEntries; i++) {
            //    free(keystore_shadow->entries[i]);
            //}
            SSS_FREE(keystore_shadow->entries);
        }
        memset(keystore_shadow, 0, sizeof(*keystore_shadow));
        SSS_FREE(keystore_shadow);
    }
}

static void unlink_file(const char *szRootPath)
{
    char file_name[MAX_FILE_NAME_SIZE] = {0};
    if (SNPRINTF(file_name, sizeof(file_name), "%s/" FAT_FILENAME, szRootPath) < 0) {
        LOG_E("snprintf error");
        return;
    }
#ifdef _WIN32
    _unlink(file_name);
#else
    unlink(file_name);
#endif
}

void ks_sw_fat_remove(const char *szRootPath)
{
    char file_name[MAX_FILE_NAME_SIZE] = {0};
    FILE *fp                           = NULL;
    if (SNPRINTF(file_name, sizeof(file_name), "%s/" FAT_FILENAME, szRootPath) < 0) {
        LOG_E("snprintf error");
        return;
    }
    fp = fopen(file_name, "rb");
    if (fp == NULL) {
        /* OK. File does not exist. */
    }
    else {
        if (fclose(fp)) {
            LOG_E("fclose Error");
        }
        unlink_file(szRootPath);
    }
}

static sss_status_t ks_sw_fat_update(keyStoreTable_t *keystore_shadow, const char *szRootPath)
{
    sss_status_t retval                = kStatus_SSS_Success;
    char file_name[MAX_FILE_NAME_SIZE] = {0};
    FILE *fp                           = NULL;
    if (SNPRINTF(file_name, sizeof(file_name), "%s/" FAT_FILENAME, szRootPath) < 0) {
        LOG_E("snprintf error");
        return kStatus_SSS_Fail;
    }
    fp = fopen(file_name, "wb+");
    if (fp == NULL) {
        LOG_E("Can not open the file");
        retval = kStatus_SSS_Fail;
    }
    else {
        if (fseek(fp, 0, SEEK_SET) != 0) {
            LOG_E("fseek failed,hence calling fclose");
            if (fclose(fp) != 0) {
                LOG_E("fclose error");
            }
            return kStatus_SSS_Fail;
        }
        if (fwrite(keystore_shadow, sizeof(*keystore_shadow), 1, fp) != 1) {
            LOG_E("fwrite error,hence calling fclose");
            if (fclose(fp) != 0) {
                LOG_E("fclose error");
            }
            return kStatus_SSS_Fail;
        }
        if (fwrite(keystore_shadow->entries, sizeof(*keystore_shadow->entries) * keystore_shadow->maxEntries, 1, fp) !=
            1) {
            LOG_E("fwrite failed, hence calling fclose");
            if (fclose(fp)) {
                LOG_E("fclose error");
            }
            return kStatus_SSS_Fail;
        }
        if (fclose(fp) != 0) {
            LOG_E("fclose failed");
            return kStatus_SSS_Fail;
        }
    }
    return retval;
}

#if defined(MBEDTLS_FS_IO)
sss_status_t ks_mbedtls_fat_update(sss_mbedtls_key_store_t *keyStore)
{
    return ks_sw_fat_update(keyStore->keystore_shadow, keyStore->session->szRootPath);
}
#endif

#if SSS_HAVE_HOSTCRYPTO_OPENSSL
sss_status_t ks_openssl_fat_update(sss_openssl_key_store_t *keyStore)
{
    return ks_sw_fat_update(keyStore->keystore_shadow, keyStore->session->szRootPath);
}
#endif

sss_status_t ks_sw_fat_load(const char *szRootPath, keyStoreTable_t *pKeystore_shadow)
{
    sss_status_t retval                = kStatus_SSS_Fail;
    char file_name[MAX_FILE_NAME_SIZE] = {0};
    FILE *fp                           = NULL;
    size_t ret;
    keyStoreTable_t fileShadow;

    ENSURE_OR_GO_CLEANUP(pKeystore_shadow);

    if (SNPRINTF(file_name, sizeof(file_name), "%s/" FAT_FILENAME, szRootPath) < 0) {
        LOG_E("snprintf error");
        goto cleanup;
    }
    fp = fopen(file_name, "rb");
    if (fp == NULL) {
        /* File did not exist, and it's OK most of the time
         * because the test code comes through this path.
         * hence return fail, but do not log any message. */
        return kStatus_SSS_Fail;
    }

    ret = fread(&fileShadow, 1, sizeof(fileShadow), fp);
    if (ret > 0 && fileShadow.maxEntries == pKeystore_shadow->maxEntries &&
        fileShadow.magic == pKeystore_shadow->magic && fileShadow.version == pKeystore_shadow->version) {
        ret =
            fread(pKeystore_shadow->entries, 1, sizeof(*pKeystore_shadow->entries) * pKeystore_shadow->maxEntries, fp);
        if (ret > 0) {
            retval = kStatus_SSS_Success;
        }
    }
    else {
        LOG_E("ERROR! keystore_shadow != pKeystore_shadow");
    }
    if (fclose(fp) != 0) {
        LOG_E("fclose failed");
        retval = kStatus_SSS_Fail;
        goto cleanup;
    }
cleanup:
    return retval;
}

#if defined(MBEDTLS_FS_IO)
sss_status_t ks_mbedtls_load_key(sss_mbedtls_object_t *sss_key, keyStoreTable_t *keystore_shadow, uint32_t extKeyId)
{
    sss_status_t retval                = kStatus_SSS_Fail;
    char file_name[MAX_FILE_NAME_SIZE] = {0};
    FILE *fp                           = NULL;
    size_t size = 0, fread_size = 0;
    uint32_t i                             = 0;
    keyIdAndTypeIndexLookup_t *shadowEntry = NULL;
    uint8_t *keyBuf                        = NULL;
    int fret                               = 0;

    for (i = 0; i < sss_key->keyStore->max_object_count; i++) {
        if (keystore_shadow->entries[i].extKeyId == extKeyId) {
            shadowEntry         = &keystore_shadow->entries[i];
            sss_key->keyId      = shadowEntry->extKeyId;
            sss_key->cipherType = shadowEntry->cipherType;
            sss_key->objectType = (shadowEntry->keyPart & 0x0F);

            ks_sw_getKeyFileName(
                file_name, sizeof(file_name), (const sss_object_t *)sss_key, sss_key->keyStore->session->szRootPath);
            retval = kStatus_SSS_Success;
            break;
        }
    }
    ENSURE_OR_GO_CLEANUP(kStatus_SSS_Success == retval)

    fp = fopen(file_name, "rb");
    if (fp == NULL) {
        LOG_E("Can not open file");
        retval = kStatus_SSS_Fail;
        goto cleanup;
    }
    /* Buffer to hold max RSA Key*/
    if (fseek(fp, 0, SEEK_END) != 0) {
        LOG_E("fseek failed,hence calling fclose");
        if (fclose(fp) != 0) {
            LOG_E("fclose error");
        }
        retval = kStatus_SSS_Fail;
        goto cleanup;
    }
    fret = ftell(fp);
    if (fret < 0) {
        LOG_E("File does not contain any data");
        retval = kStatus_SSS_Fail;
        if (fclose(fp) != 0) {
            LOG_E("fclose error");
        }
        goto cleanup;
    }
    size = (size_t)fret;
    if (fseek(fp, 0, SEEK_SET) != 0) {
        LOG_E("fseek faild,hence calling fclose");
        retval = kStatus_SSS_Fail;
        if (fclose(fp) != 0) {
            LOG_E("fclose error");
        }
        goto cleanup;
    }
    keyBuf = SSS_CALLOC(1, size);
    if (keyBuf == NULL) {
        retval = kStatus_SSS_Fail;
        if (fclose(fp) != 0) {
            LOG_E("fclose error");
        }
        goto cleanup;
    }
    fread_size = fread(keyBuf, size, 1, fp);
    if (fread_size != 1) {
        retval = kStatus_SSS_Fail;
        LOG_E("fread error");
    }
    if (fclose(fp) != 0) {
        retval = kStatus_SSS_Fail;
        LOG_E("fclose error");
        goto cleanup;
    }
    retval = ks_mbedtls_key_object_create(sss_key,
        shadowEntry->extKeyId,
        (sss_key_part_t)(shadowEntry->keyPart & 0x0F),
        (sss_cipher_type_t)(shadowEntry->cipherType),
        size,
        kKeyObject_Mode_Persistent);
    ENSURE_OR_GO_CLEANUP(kStatus_SSS_Success == retval)

    retval = sss_mbedtls_key_store_set_key(sss_key->keyStore, sss_key, keyBuf, size, size * 8 /* FIXME */, NULL, 0);
cleanup:
    if (keyBuf != NULL) {
        SSS_FREE(keyBuf);
    }
    return retval;
}

sss_status_t ks_mbedtls_store_key(const sss_mbedtls_object_t *sss_key)
{
    sss_status_t retval                = kStatus_SSS_Fail;
    int fret                           = -1;
    char file_name[MAX_FILE_NAME_SIZE] = {0};
    FILE *fp                           = NULL;
    /* Buffer to hold max RSA Key*/
    uint8_t key_buf[3000];
    int ret          = 0;
    unsigned char *c = key_buf;
    mbedtls_pk_context *pk;

    ks_sw_getKeyFileName(
        file_name, sizeof(file_name), (const sss_object_t *)sss_key, sss_key->keyStore->session->szRootPath);
    fp = fopen(file_name, "wb+");
    if (fp == NULL) {
        LOG_E(" Can not open the file");
        retval = kStatus_SSS_Fail;
    }
    else {
        memset(key_buf, 0, sizeof(key_buf));
        pk = (mbedtls_pk_context *)sss_key->contents;
        switch (sss_key->objectType) {
        case kSSS_KeyPart_Default:
            if (fwrite(sss_key->contents, sss_key->contents_max_size, 1, fp) != 1) {
                LOG_E("fwrite error, hence calling fclose");
                fret = fclose(fp);
                if (fret != 0) {
                    LOG_E("fclose error");
                }
                goto exit;
            }
            retval = kStatus_SSS_Success; /* Allows to skip writing pem/der files */
            break;
        case kSSS_KeyPart_Pair:
        case kSSS_KeyPart_Private:
            ret = mbedtls_pk_write_key_der(pk, key_buf, sizeof(key_buf));
            break;
        case kSSS_KeyPart_Public:
            ret = mbedtls_pk_write_pubkey_der(pk, key_buf, sizeof(key_buf));
            break;
        default:
            LOG_E("Invalid objectType");
        }
        if (ret > 0 && retval != kStatus_SSS_Success) {
            c = key_buf + sizeof(key_buf) - ret;
            if (fwrite(c, ret, 1, fp) != 1) {
                LOG_E("fwrite error, hence calling fclose");
                fret = fclose(fp);
                if (fret != 0) {
                    LOG_E("fclose error");
                }
                goto exit;
            }
            retval = kStatus_SSS_Success;
        }
        fret = fflush(fp);
        if (fret != 0) {
            LOG_E("fflush error");
        }
        fret = fclose(fp);
        if (fret != 0) {
            LOG_E("fclose error");
        }
    }
exit:
    return retval;
}

#ifdef _MSC_VER
#define UNLINK _unlink
#else
#define UNLINK unlink
#endif

sss_status_t ks_mbedtls_remove_key(const sss_mbedtls_object_t *sss_key)
{
    sss_status_t retval                = kStatus_SSS_Fail;
    char file_name[MAX_FILE_NAME_SIZE] = {0};
    ks_sw_getKeyFileName(
        file_name, sizeof(file_name), (const sss_object_t *)sss_key, sss_key->keyStore->session->szRootPath);
    if (0 == UNLINK(file_name)) {
        retval = kStatus_SSS_Success;
    }
    return retval;
}
#endif

/* ************************************************************************** */
/* Private Functions                                                          */
/* ************************************************************************** */

#endif /* MBEDTLS_FS_IO */
