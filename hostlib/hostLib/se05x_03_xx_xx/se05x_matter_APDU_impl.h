/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <se05x_tlv.h>
#include <se05x_const.h>
#include <se05x_enums.h>

#if defined(NONSECURE_WORLD)
#include "veneer_printf_table.h"
#endif

#if (SSS_HAVE_APPLET_SE051_H && SSS_HAVE_SE05X_VER_07_02)
/* OK */
#else
#error "Only with SE051_H based build"
#endif

#ifndef NEWLINE
#define NEWLINE must be already defined
#endif

// Check if applet version minor is greater than or equals to 20 (0x14 hex) - for version x.20.x.00
#define SE05X_CHECK_MINOR_GTE_20_VERSION(app_ver) (((app_ver >> 16) & 0xFF) >= 0x14)

smStatus_t Se05x_API_WriteBinary_V2(pSe05xSession_t session_ctx,
    pSe05xPolicy_t policy,
    uint32_t objectID,
    uint16_t offset,
    uint16_t length,
    const uint8_t *inputData,
    size_t inputDataLen,
    uint16_t objectIDExt,
    uint32_t version)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr      = {{kSE05x_CLA, kSE05x_INS_WRITE, kSE05x_P1_BINARY, kSE05x_P2_DEFAULT}};
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen = 0;
    uint8_t *pCmdbuf = &cmdbuf[0];
    int tlvRet       = 0;

    // Check if applet version minor is greater than or equals to 20 (x.20.x)
    if (!SE05X_CHECK_MINOR_GTE_20_VERSION(session_ctx->applet_version)) {
        retStatus = SM_ERR_CONDITIONS_NOT_SATISFIED;
        goto cleanup;
    }

#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "Se05x_API_WriteBinary_V2 []");
#endif /* VERBOSE_APDU_LOGS */
    tlvRet = TLVSET_Se05xPolicy("policy", &pCmdbuf, &cmdbufLen, kSE05x_TAG_POLICY, policy);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U32("object id", &pCmdbuf, &cmdbufLen, kSE05x_TAG_1, objectID);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U16Optional("offset", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, offset);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U16Optional("length", &pCmdbuf, &cmdbufLen, kSE05x_TAG_3, length);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_u8bufOptional("input data", &pCmdbuf, &cmdbufLen, kSE05x_TAG_4, inputData, inputDataLen);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U16Optional("object ID extension", &pCmdbuf, &cmdbufLen, kSE05x_TAG_5, objectIDExt);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U32("version", &pCmdbuf, &cmdbufLen, kSE05x_TAG_11, version);
    if (0 != tlvRet) {
        goto cleanup;
    }
    retStatus = DoAPDUTx_s_Case3(session_ctx, &hdr, cmdbuf, cmdbufLen);

cleanup:
    return retStatus;
}

smStatus_t Se05x_API_ReadObject_V2(
    pSe05xSession_t session_ctx, uint32_t objectID, uint16_t offset, uint16_t length, uint8_t *data, size_t *pdataLen, uint16_t objectIDExt)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr      = {{kSE05x_CLA, kSE05x_INS_READ, kSE05x_P1_DEFAULT, kSE05x_P2_DEFAULT}};
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen                       = 0;
    uint8_t *pCmdbuf                       = &cmdbuf[0];
    int tlvRet                             = 0;
    uint8_t rspbuf[SE05X_MAX_BUF_SIZE_RSP] = {0};
    uint8_t *pRspbuf                       = &rspbuf[0];
    size_t rspbufLen                       = ARRAY_SIZE(rspbuf);
    size_t rspIndex                        = 0;

    // Check if applet version minor is greater than or equals to 20 (x.20.x)
    if (!SE05X_CHECK_MINOR_GTE_20_VERSION(session_ctx->applet_version)) {
        retStatus = SM_ERR_CONDITIONS_NOT_SATISFIED;
        goto cleanup;
    }

#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "ReadObject_V2 []");
#endif /* VERBOSE_APDU_LOGS */
    tlvRet = TLVSET_U32("object id", &pCmdbuf, &cmdbufLen, kSE05x_TAG_1, objectID);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U16Optional("offset", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, offset);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U16Optional("length", &pCmdbuf, &cmdbufLen, kSE05x_TAG_3, length);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U16Optional("object ID extension", &pCmdbuf, &cmdbufLen, kSE05x_TAG_8, objectIDExt);
    if (0 != tlvRet) {
        goto cleanup;
    }
    retStatus = DoAPDUTxRx_s_Case4_ext(session_ctx, &hdr, cmdbuf, cmdbufLen, rspbuf, &rspbufLen);
    if (retStatus == SM_OK) {
        retStatus = SM_NOT_OK;
        tlvRet    = tlvGet_u8buf(pRspbuf, &rspIndex, rspbufLen, kSE05x_TAG_1, data, pdataLen); /*  */
        if (0 != tlvRet) {
            goto cleanup;
        }
        if ((rspIndex + 2) == rspbufLen) {
            retStatus = (smStatus_t)((pRspbuf[rspIndex] << 8) | (pRspbuf[rspIndex + 1]));
        }
    }

    if (retStatus == SM_ERR_COMMAND_NOT_ALLOWED) {
        LOG_W("Denied to read object %08X bases on policy.", objectID);
    }

cleanup:
    return retStatus;
}

smStatus_t Se05x_API_ReadObjectAttributes_V2(
    pSe05xSession_t session_ctx, uint32_t objectID, uint8_t *data, size_t *pdataLen, uint16_t objectIDExt)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr      = {{kSE05x_CLA, kSE05x_INS_READ, kSE05x_P1_DEFAULT, kSE05x_P2_ATTRIBUTES}};
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen                       = 0;
    uint8_t *pCmdbuf                       = &cmdbuf[0];
    int tlvRet                             = 0;
    uint8_t rspbuf[SE05X_MAX_BUF_SIZE_RSP] = {0};
    uint8_t *pRspbuf                       = &rspbuf[0];
    size_t rspbufLen                       = ARRAY_SIZE(rspbuf);
    size_t rspIndex                        = 0;

    // Check if applet version minor is greater than or equals to 20 (x.20.x)
    if (!SE05X_CHECK_MINOR_GTE_20_VERSION(session_ctx->applet_version)) {
        retStatus = SM_ERR_CONDITIONS_NOT_SATISFIED;
        goto cleanup;
    }

#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "ReadObjectAttributes_V2 []");
#endif /* VERBOSE_APDU_LOGS */
    tlvRet = TLVSET_U32("object id", &pCmdbuf, &cmdbufLen, kSE05x_TAG_1, objectID);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U16Optional("object ID extension", &pCmdbuf, &cmdbufLen, kSE05x_TAG_8, objectIDExt);
    if (0 != tlvRet) {
        goto cleanup;
    }
    retStatus = DoAPDUTxRx_s_Case4(session_ctx, &hdr, cmdbuf, cmdbufLen, rspbuf, &rspbufLen);
    if (retStatus == SM_OK) {
        retStatus = SM_NOT_OK;
#if !SSS_HAVE_SE05X_VER_GTE_07_02
        //Backward compataibility
        tlvRet = tlvGet_u8buf(pRspbuf, &rspIndex, rspbufLen, kSE05x_TAG_2, data, pdataLen); /*  */
#else
        tlvRet = tlvGet_u8buf(pRspbuf, &rspIndex, rspbufLen, kSE05x_TAG_3, data, pdataLen); /*  */
#endif
        if (0 != tlvRet) {
            goto cleanup;
        }
        if ((rspIndex + 2) == rspbufLen) {
            retStatus = (smStatus_t)((pRspbuf[rspIndex] << 8) | (pRspbuf[rspIndex + 1]));
        }
    }

cleanup:
    return retStatus;
}

smStatus_t Se05x_API_ReadType_V2(
    pSe05xSession_t session_ctx, uint32_t objectID, SE05x_SecureObjectType_t *ptype, uint8_t *pisTransient, uint16_t objectIDExt)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr      = {{kSE05x_CLA, kSE05x_INS_READ, kSE05x_P1_DEFAULT, kSE05x_P2_TYPE}};
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen                       = 0;
    uint8_t *pCmdbuf                       = &cmdbuf[0];
    int tlvRet                             = 0;
    uint8_t rspbuf[SE05X_MAX_BUF_SIZE_RSP] = {0};
    uint8_t *pRspbuf                       = &rspbuf[0];
    size_t rspbufLen                       = ARRAY_SIZE(rspbuf);
    size_t rspIndex                        = 0;

        // Check if applet version minor is greater than or equals to 20 (x.20.x)
    if (!SE05X_CHECK_MINOR_GTE_20_VERSION(session_ctx->applet_version)) {
        retStatus = SM_ERR_CONDITIONS_NOT_SATISFIED;
        goto cleanup;
    }

#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "ReadType_V2 []");
#endif /* VERBOSE_APDU_LOGS */
    tlvRet = TLVSET_U32("object id", &pCmdbuf, &cmdbufLen, kSE05x_TAG_1, objectID);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U16Optional("object ID extension", &pCmdbuf, &cmdbufLen, kSE05x_TAG_8, objectIDExt);
    if (0 != tlvRet) {
        goto cleanup;
    }
    retStatus = DoAPDUTxRx_s_Case4(session_ctx, &hdr, cmdbuf, cmdbufLen, rspbuf, &rspbufLen);
    if (retStatus == SM_OK) {
        retStatus = SM_NOT_OK;
        if (ptype != NULL) {
            tlvRet = tlvGet_SecureObjectType(pRspbuf, &rspIndex, rspbufLen, kSE05x_TAG_1, ptype); /* - */
            if (0 != tlvRet) {
                goto cleanup;
            }
        }
        tlvRet = tlvGet_U8(pRspbuf, &rspIndex, rspbufLen, kSE05x_TAG_2, pisTransient); /* - */
        if (0 != tlvRet) {
            goto cleanup;
        }
        if ((rspIndex + 2) == rspbufLen) {
            retStatus = (smStatus_t)((pRspbuf[rspIndex] << 8) | (pRspbuf[rspIndex + 1]));
        }
    }

cleanup:
    return retStatus;
}

smStatus_t Se05x_API_ReadSize_V2(pSe05xSession_t session_ctx, uint32_t objectID, uint16_t *psize, uint16_t objectIDExt)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr      = {{kSE05x_CLA, kSE05x_INS_READ, kSE05x_P1_DEFAULT, kSE05x_P2_SIZE}};
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen                       = 0;
    uint8_t *pCmdbuf                       = &cmdbuf[0];
    int tlvRet                             = 0;
    uint8_t rspbuf[SE05X_MAX_BUF_SIZE_RSP] = {0};
    uint8_t *pRspbuf                       = &rspbuf[0];
    size_t rspbufLen                       = ARRAY_SIZE(rspbuf);
    size_t rspIndex                        = 0;

    // Check if applet version minor is greater than or equals to 20 (x.20.x)
    if (!SE05X_CHECK_MINOR_GTE_20_VERSION(session_ctx->applet_version)) {
        retStatus = SM_ERR_CONDITIONS_NOT_SATISFIED;
        goto cleanup;
    }

#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "ReadSize_V2 []");
#endif /* VERBOSE_APDU_LOGS */
    tlvRet = TLVSET_U32("object id", &pCmdbuf, &cmdbufLen, kSE05x_TAG_1, objectID);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U16Optional("object ID extension", &pCmdbuf, &cmdbufLen, kSE05x_TAG_8, objectIDExt);
    if (0 != tlvRet) {
        goto cleanup;
    }
    retStatus = DoAPDUTxRx_s_Case4(session_ctx, &hdr, cmdbuf, cmdbufLen, rspbuf, &rspbufLen);
    if (retStatus == SM_OK) {
        retStatus = SM_NOT_OK;
        tlvRet    = tlvGet_U16(pRspbuf, &rspIndex, rspbufLen, kSE05x_TAG_1, psize); /* - */
        if (0 != tlvRet) {
            goto cleanup;
        }
        if ((rspIndex + 2) == rspbufLen) {
            retStatus = (smStatus_t)((pRspbuf[rspIndex] << 8) | (pRspbuf[rspIndex + 1]));
        }
    }

cleanup:
    return retStatus;
}

smStatus_t Se05x_API_CheckObjectExists_V2(
    pSe05xSession_t session_ctx, uint32_t objectID, SE05x_Result_t *presult, uint16_t objectIDExt)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr      = {{kSE05x_CLA, kSE05x_INS_MGMT, kSE05x_P1_DEFAULT, kSE05x_P2_EXIST}};
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen                       = 0;
    uint8_t *pCmdbuf                       = &cmdbuf[0];
    int tlvRet                             = 0;
    uint8_t rspbuf[SE05X_MAX_BUF_SIZE_RSP] = {0};
    uint8_t *pRspbuf                       = &rspbuf[0];
    size_t rspbufLen                       = ARRAY_SIZE(rspbuf);
    size_t rspIndex                        = 0;

    // Check if applet version minor is greater than or equals to 20 (x.20.x)
    if (!SE05X_CHECK_MINOR_GTE_20_VERSION(session_ctx->applet_version)) {
        retStatus = SM_ERR_CONDITIONS_NOT_SATISFIED;
        goto cleanup;
    }

#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "CheckObjectExists_V2 []");
#endif /* VERBOSE_APDU_LOGS */
    tlvRet = TLVSET_U32("object id", &pCmdbuf, &cmdbufLen, kSE05x_TAG_1, objectID);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U16Optional("object ID extension", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, objectIDExt);
    if (0 != tlvRet) {
        goto cleanup;
    }
    retStatus = DoAPDUTxRx_s_Case4(session_ctx, &hdr, cmdbuf, cmdbufLen, rspbuf, &rspbufLen);
    if (retStatus == SM_OK) {
        retStatus = SM_NOT_OK;
        tlvRet    = tlvGet_Result(pRspbuf, &rspIndex, rspbufLen, kSE05x_TAG_1, presult); /* - */
        if (0 != tlvRet) {
            goto cleanup;
        }
        if ((rspIndex + 2) == rspbufLen) {
            retStatus = (smStatus_t)((pRspbuf[rspIndex] << 8) | (pRspbuf[rspIndex + 1]));
        }
    }

cleanup:
    return retStatus;
}

smStatus_t Se05x_API_DeleteSecureObject_V2(pSe05xSession_t session_ctx, uint32_t objectID, uint16_t objectIDExt)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr      = {{kSE05x_CLA, kSE05x_INS_MGMT, kSE05x_P1_DEFAULT, kSE05x_P2_DELETE_OBJECT}};
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen = 0;
    uint8_t *pCmdbuf = &cmdbuf[0];
    int tlvRet       = 0;

    // Check if applet version minor is greater than or equals to 20 (x.20.x)
    if (!SE05X_CHECK_MINOR_GTE_20_VERSION(session_ctx->applet_version)) {
        retStatus = SM_ERR_CONDITIONS_NOT_SATISFIED;
        goto cleanup;
    }

#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "DeleteSecureObject_V2 []");
#endif /* VERBOSE_APDU_LOGS */
    tlvRet = TLVSET_U32("object id", &pCmdbuf, &cmdbufLen, kSE05x_TAG_1, objectID);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U16Optional("object ID extension", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, objectIDExt);
    if (0 != tlvRet) {
        goto cleanup;
    }
    retStatus = DoAPDUTx_s_Case3(session_ctx, &hdr, cmdbuf, cmdbufLen);

cleanup:
    return retStatus;
}
