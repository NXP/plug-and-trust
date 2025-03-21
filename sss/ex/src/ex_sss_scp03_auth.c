/*
 *
 * Copyright 2019-2020,2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
*
* ex_sss_scp03_auth.c:  *The purpose and scope of this file*
*
* Project:  sss-doc-upstream
*
* $Date: Dec 12, 2019 $
* $Author: nxf42670 $
* $Revision$
*/

/* *****************************************************************************************************************
* Includes
* ***************************************************************************************************************** */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ex_sss_auth.h"
#include "ex_sss_boot_int.h"
#include "ex_sss_scp03_keys.h"
#include "nxLog_App.h"
#include "nxScp03_Types.h"

/* *****************************************************************************************************************
* Internal Definitions
* ***************************************************************************************************************** */

/* *****************************************************************************************************************
* Type Definitions
* ***************************************************************************************************************** */

/* *****************************************************************************************************************
* Global and Static Variables
* Total Size: NNNbytes
* ***************************************************************************************************************** */

/* *****************************************************************************************************************
* Private Functions Prototypes
* ***************************************************************************************************************** */

#ifdef EX_SSS_SCP03_FILE_PATH

static sss_status_t Scp03_KeyString_to_Keybuffer(
    bool hasAuthKey, char *inputKey, uint8_t *auth_key, size_t key_size, size_t *key_len);

static sss_status_t read_platfscp03_keys_from_file(
    const char *scp03_file_path, uint8_t *enc, size_t *enc_len, uint8_t *mac, size_t *mac_len);

#define UNSECURE_LOGGING_OF_SCP_KEYS 0

/* *****************************************************************************************************************
* Public Functions
* ***************************************************************************************************************** */
static sss_status_t read_platfscp03_keys_from_file_temp(uint8_t *penc, size_t *enc_len, uint8_t *pmac, size_t *mac_len)
{
    const char *filename = EX_SSS_SCP03_FILE_PATH;
    return read_platfscp03_keys_from_file(filename, penc, enc_len, pmac, mac_len);
}

sss_status_t scp03_keys_from_path(uint8_t *penc, size_t *enc_len, uint8_t *pmac, size_t *mac_len)
{
    sss_status_t status  = kStatus_SSS_Fail;
    const char *filename = EX_SSS_SCP03_FILE_PATH;
    FILE *fp             = NULL;
    LOG_D("Using File: %s", filename);

    if (strstr(filename, "..") != NULL) {
        LOG_W("Potential directory traversal");
    }

    fp = fopen(filename, "rb");
    if (fp != NULL) {
        // File exists. Get keys from file
        LOG_W("Using SCP03 keys from:'%s' (FILE=%s)", filename, EX_SSS_SCP03_FILE_PATH);
        if (fclose(fp) != 0) {
            LOG_E("fclose Error");
        }
        status = read_platfscp03_keys_from_file_temp(penc, enc_len, pmac, mac_len);
    }
    else {
        // File does not exist. Check env variable
        const char *scp03_path_env = getenv(EX_SSS_BOOT_SCP03_PATH_ENV);
        if (scp03_path_env != NULL) {
            LOG_W("Using SCP03 keys from:'%s' (ENV=%s)", scp03_path_env, EX_SSS_BOOT_SCP03_PATH_ENV);
            status = read_platfscp03_keys_from_file(scp03_path_env, penc, enc_len, pmac, mac_len);
        }
        else {
            //If File does not exist, Use default key length.
            *enc_len = EX_SSS_AUTH_SE05X_KEY_LEN;
            *mac_len = EX_SSS_AUTH_SE05X_KEY_LEN;
            LOG_I(
                "Using default PlatfSCP03 keys. "
                "You can use keys from file using ENV=%s",
                EX_SSS_BOOT_SCP03_PATH_ENV);
        }
    }

    if (status != kStatus_SSS_Success) {
        LOG_D("Using default keys");
    }

    return status;
}

static sss_status_t read_platfscp03_keys_from_file(
    const char *scp03_file_path, uint8_t *enc, size_t *enc_len, uint8_t *mac, size_t *mac_len)
{
    sss_status_t status = kStatus_SSS_Fail;
    FILE *scp_file      = fopen(scp03_file_path, "r");
    char file_data[1024];
    char *pdata = &file_data[0];
    bool hasEnc = false;
    bool hasMac = false;

    if (strstr(scp03_file_path, "..") != NULL) {
        LOG_W("Potential directory traversal");
    }

    if (scp_file == NULL) {
        LOG_E("Cannot open SCP file");
        status = kStatus_SSS_Fail;
        return status;
    }

    while (fgets(pdata, sizeof(file_data), scp_file)) {
        size_t i = 0, j = 0;

        /*Don't need leading spaces*/
        for (i = 0; i < strlen(pdata); i++) {
            int charac = (int)pdata[i];
            if (!isspace(charac)) {
                break;
            }
        }

        /*Lines beginning with '#' are comments*/
        if (pdata[i] == '#') {
            continue;
        }

        /*Remove trailing comments*/
        for (j = 0; j < strlen(pdata); j++) {
            if (pdata[j] == '#') {
                pdata[j] = '\0';
                break;
            }
        }

        if (strncmp(&pdata[i], "ENC ", strlen("ENC ")) == 0) {
#if UNSECURE_LOGGING_OF_SCP_KEYS
            LOG_I("%s", &pdata[i]);
#endif
            status = Scp03_KeyString_to_Keybuffer(hasEnc, &pdata[i], enc, *enc_len, enc_len);
            if (status != kStatus_SSS_Success) {
                if (fclose(scp_file) != 0) {
                    LOG_E("fclose error");
                }
                return status;
            }
            hasEnc = true;
        }

        else if (!strncmp(&pdata[i], "MAC ", strlen("MAC "))) {
#if UNSECURE_LOGGING_OF_SCP_KEYS
            LOG_I("%s", &pdata[i]);
#endif
            status = Scp03_KeyString_to_Keybuffer(hasMac, &pdata[i], mac, *mac_len, mac_len);
            if (status != kStatus_SSS_Success) {
                if (fclose(scp_file) != 0) {
                    LOG_E("fclose error");
                }
                return status;
            }
            hasMac = true;
        }

        else if (!strncmp(&pdata[i], "DEK ", strlen("DEK "))) {
            // #if UNSECURE_LOGGING_OF_SCP_KEYS
            //             LOG_I("%s", &pdata[i]);
            // #endif
            //             status = Scp03_KeyString_to_Keybuffer(hasDek, &pdata[i], dek, dek_len);
            //             if (status != kStatus_SSS_Success) {
            //                 fclose(scp_file);
            //                 return status;
            //             }
            //             hasDek = true;
        }

        else {
            LOG_E("Unknown key type %s", &pdata[i]);
            status = kStatus_SSS_Fail;
            if (fclose(scp_file) != 0) {
                LOG_E("fclose error");
            }
            return status;
        }
    }

    if (fclose(scp_file) != 0) {
        LOG_E("fclose error");
    }

    return kStatus_SSS_Success;
}

static sss_status_t Scp03_KeyString_to_Keybuffer(
    bool hasAuthKey, char *inputKey, uint8_t *auth_key, size_t key_size, size_t *key_len)
{
    sss_status_t status = kStatus_SSS_Success;
    size_t j            = 0;
    int charac          = (int)inputKey[j];
    size_t key_count    = 0;
    if (hasAuthKey) {
        LOG_E("Duplicate Auth key value");
        status = kStatus_SSS_Fail;
        return status;
    }
    while (!isspace(charac)) {
        j++;
        charac = (int)inputKey[j];
    }
    while (isspace(charac)) {
        j++;
        charac = (int)inputKey[j];
    }
    if (inputKey[j] == '\0') {
        LOG_E("Invalid Key");
        status = kStatus_SSS_Fail;
        return status;
    }
    for (size_t count = 0; count < key_size && inputKey[j] != '\0' && !isspace(inputKey[j]); count++) {
        if (sscanf(&inputKey[j], "%2hhx", &auth_key[count]) != 1) {
            LOG_E("Cannot copy data");
            status = kStatus_SSS_Fail;
            return status;
        }
        key_count++;
        j = j + 2;
    }

    *key_len = key_count;

    return status;
}

#endif //EX_SSS_SCP03_FILE_PATH
