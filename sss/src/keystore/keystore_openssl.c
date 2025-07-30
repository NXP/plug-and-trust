/*
 *
 * Copyright 2018-2020,2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Key store in PC : For testing */

/* ************************************************************************** */
/* Includes                                                                   */
/* ************************************************************************** */

#include <fsl_sss_keyid_map.h>
#include <fsl_sss_openssl_apis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nxLog_sss.h"
#if SSS_HAVE_HOSTCRYPTO_OPENSSL
#include <openssl/evp.h>

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

/* ************************************************************************** */
/* Static function declarations                                               */
/* ************************************************************************** */

/* ************************************************************************** */
/* Public Functions                                                           */
/* ************************************************************************** */

sss_status_t ks_openssl_load_key(sss_openssl_object_t *sss_key, keyStoreTable_t *keystore_shadow, uint32_t extKeyId)
{
    sss_status_t retval                = kStatus_SSS_Fail;
    char file_name[MAX_FILE_NAME_SIZE] = {0};
    FILE *fp                           = NULL;
    int evp_pkey_bits                  = 0;
    //const char *root_folder = sss_key->keyStore->session->szRootPath;
    size_t size = 0;
    uint32_t i;
    keyIdAndTypeIndexLookup_t *shadowEntry = NULL;
    EVP_PKEY *pkey                         = NULL;

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
    if (retval == kStatus_SSS_Success) {
        fp = fopen(file_name, "rb");
        if (fp == NULL) {
            LOG_E("Can not open file");
            retval = kStatus_SSS_Fail;
        }
        else {
            /*Buffer: max RSA key*/
            uint8_t keyBuf[3000];
            const uint8_t *buf_ptr = keyBuf;
            long signed_size       = 0;
            if ((fseek(fp, 0, SEEK_END)) != 0) {
                LOG_E("fseek failed, hence calling fclose");
                if (fclose(fp) != 0) {
                    LOG_E("fclose error");
                }
                return kStatus_SSS_Fail;
            }
            signed_size = ftell(fp);
            if (signed_size < 0) {
                retval = kStatus_SSS_Fail;
                if (fclose(fp) != 0) {
                    LOG_E("fclose error");
                }
                return retval;
            }
            size = (size_t)signed_size;
            if ((fseek(fp, 0, SEEK_SET)) != 0) {
                LOG_E("fseek failed, hence calling fclose");
                if (fclose(fp) != 0) {
                    LOG_E("fclose error");
                }
                return kStatus_SSS_Fail;
            }
            if (!fread(keyBuf, size, 1, fp)) {
                LOG_E("Error in fread");
            }
            if (fclose(fp) != 0) {
                LOG_E("fclose failed");
            }
            retval = sss_openssl_key_object_allocate(sss_key,
                shadowEntry->extKeyId,
                (sss_key_part_t)(shadowEntry->keyPart & 0x0F),
                (sss_cipher_type_t)(shadowEntry->cipherType),
                size,
                kKeyObject_Mode_Persistent);
            if (retval == kStatus_SSS_Success) {
                switch (sss_key->cipherType) {
                case kSSS_CipherType_RSA:
                case kSSS_CipherType_RSA_CRT: {
                    if (sss_key->contents != NULL) {
                        EVP_PKEY_free((EVP_PKEY *)sss_key->contents);
                    }
                    if (sss_key->objectType == kSSS_KeyPart_Public) {
                        pkey = d2i_PublicKey(EVP_PKEY_RSA, NULL, &buf_ptr, (long)size);
                    }
                    else {
                        pkey = d2i_AutoPrivateKey(NULL, &buf_ptr, (long)size);
                    }

                    if (pkey == NULL) {
                        retval = kStatus_SSS_Fail;
                    }
                    else {
                        sss_key->contents = (void *)pkey;
                    }

                    evp_pkey_bits = EVP_PKEY_bits(pkey);
                    if (evp_pkey_bits < 0) {
                        return kStatus_SSS_Fail;
                    }
                    sss_key->keyBitLen = evp_pkey_bits;
                } break;
                case kSSS_CipherType_EC_NIST_P:
                case kSSS_CipherType_EC_NIST_K:
                case kSSS_CipherType_EC_BRAINPOOL:
                case kSSS_CipherType_EC_MONTGOMERY:
                case kSSS_CipherType_EC_TWISTED_ED: {
                    if (sss_key->contents != NULL) {
                        EVP_PKEY_free((EVP_PKEY *)sss_key->contents);
                    }
                    if (sss_key->objectType == kSSS_KeyPart_Public) {
                        pkey = d2i_PublicKey(EVP_PKEY_EC, NULL, &buf_ptr, (long)size);
                    }
                    else {
                        pkey = d2i_AutoPrivateKey(NULL, &buf_ptr, (long)size);
                    }

                    if (pkey == NULL) {
                        retval = kStatus_SSS_Fail;
                    }
                    else {
                        sss_key->contents = (void *)pkey;
                    }

                    evp_pkey_bits = EVP_PKEY_bits(pkey);
                    if (evp_pkey_bits < 0) {
                        retval = kStatus_SSS_Fail;
                    }
                    sss_key->keyBitLen = evp_pkey_bits;

                } break;
                default: {
                    retval = sss_openssl_key_store_set_key(sss_key->keyStore, sss_key, keyBuf, size, size * 8, NULL, 0);
                } break;
                }
            }
        }
    }
    return retval;
}

sss_status_t ks_openssl_store_key(const sss_openssl_object_t *sss_key)
{
    sss_status_t retval                = kStatus_SSS_Fail;
    char file_name[MAX_FILE_NAME_SIZE] = {0};
    FILE *fp                           = NULL;
    unsigned char *Buffer              = NULL;
    ks_sw_getKeyFileName(
        file_name, sizeof(file_name), (const sss_object_t *)sss_key, sss_key->keyStore->session->szRootPath);
    fp = fopen(file_name, "wb+");
    if (fp == NULL) {
        LOG_E("Can not open file");
        retval = kStatus_SSS_Fail;
    }
    else {
        int len = 0;
        EVP_PKEY *pk;
        pk = (EVP_PKEY *)sss_key->contents;
        switch (sss_key->objectType) {
        case kSSS_KeyPart_Default:
            if (fwrite(sss_key->contents, sss_key->contents_max_size, 1, fp) != 1) {
                LOG_E("fwrite error, hence calling fclose");
                if (fclose(fp) != 0) {
                    LOG_E("fclose error");
                }
                return kStatus_SSS_Fail;
            }
            retval = kStatus_SSS_Success;
            break;
        case kSSS_KeyPart_Pair:
        case kSSS_KeyPart_Private:
            len = i2d_PrivateKey(pk, NULL);
            if (len < 0) {
                goto exit;
            }
            len = i2d_PrivateKey(pk, &Buffer);
            if (len < 0) {
                goto exit;
            }
            break;
        case kSSS_KeyPart_Public:
            len = i2d_PublicKey(pk, NULL);
            if (len < 0) {
                goto exit;
            }
            len = i2d_PublicKey(pk, &Buffer);
            if (len < 0) {
                goto exit;
            }
            break;
        default:
            LOG_E("Invalid objectType");
        }
        if (len > 0 && retval != kStatus_SSS_Success) {
            if ((fwrite(Buffer, len, 1, fp)) != 1) {
                LOG_E("fwrite error, hence calling fclose");
                if (fclose(fp) != 0) {
                    LOG_E("fclose error");
                }
                return kStatus_SSS_Fail;
            }
            retval = kStatus_SSS_Success;
        }
    }
exit:
    if (fp != NULL) {
        if (fclose(fp) != 0) {
            LOG_E("fclose error");
        }
    }
    if (Buffer != NULL) {
        OPENSSL_free(Buffer);
    }
    return retval;
}

#ifdef _MSC_VER
#define UNLINK _unlink
#else
#define UNLINK unlink
#endif

sss_status_t ks_openssl_remove_key(const sss_openssl_object_t *sss_key)
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

/* ************************************************************************** */
/* Private Functions                                                          */
/* ************************************************************************** */

#endif /* OpenSSL */
