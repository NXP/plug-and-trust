/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <se05x_tlv.h>
#include <se05x_const.h>
#include <se05x_enums.h>



#if defined(NONSECURE_WORLD)
#include "veneer_printf_table.h"
#endif

#if (SSS_HAVE_APPLET_SE051_CHIP && SSS_HAVE_SE05X_VER_GTE_16_01)
/* OK */
#else
#error "Only with SE051_CHIP based build"
#endif

#ifndef NEWLINE
#define NEWLINE must be already defined
#endif

smStatus_t Se05x_API_PAKEInitProtocol(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint32_t objectID_M,
    uint32_t objectID_N)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr = { { kSE05x_CLA, kSE05x_INS_CRYPTO, kSE05x_P1_SPAKE, kSE05x_P2_INIT } };
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen = 0;
    uint8_t *pCmdbuf = &cmdbuf[0];
    int tlvRet = 0;

#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "PAKEInitProtocol []");
#endif /* VERBOSE_APDU_LOGS */
    tlvRet = TLVSET_CryptoObjectID("cryptoObjectID", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, cryptoObjectID);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U32("objectID M", &pCmdbuf, &cmdbufLen, kSE05x_TAG_3, objectID_M);
    if (0 != tlvRet) {
        goto cleanup;
    }
    tlvRet = TLVSET_U32("objectID N ", &pCmdbuf, &cmdbufLen, kSE05x_TAG_4, objectID_N);
    if (0 != tlvRet) {
        goto cleanup;
    }
    retStatus = DoAPDUTx_s_Case3(session_ctx, &hdr, cmdbuf, cmdbufLen);

cleanup:
    return retStatus;
}

#if SSS_HAVE_SE05X_VER_GTE_16_02
smStatus_t  Se05x_API_PAKEConfigDevice(
    pSe05xSession_t session_ctx,
    SE05x_SPAKEDeviceType_t deviceType,
    SE05x_CryptoObjectID_t cryptoObjectID)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr = { { kSE05x_CLA, kSE05x_INS_CRYPTO, kSE05x_P1_SPAKE, kSE05x_P2_TYPE } };
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen = 0;
    uint8_t *pCmdbuf = &cmdbuf[0];
    int tlvRet = 0;

#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "PAKEConfigDevice[]");
#endif /* VERBOSE_APDU_LOGS */

    tlvRet = TLVSET_CryptoObjectID("cryptoObjectID", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, cryptoObjectID);
    if (0 != tlvRet) {
        goto cleanup;
    }

    tlvRet = TLVSET_U8("deviceType", &pCmdbuf, &cmdbufLen, kSE05x_TAG_3, deviceType);
    if (0 != tlvRet) {
        goto cleanup;
    }

    retStatus = DoAPDUTx_s_Case3(session_ctx, &hdr, cmdbuf, cmdbufLen);

cleanup:
    return retStatus;

}

#else
smStatus_t  Se05x_API_PAKEConfigDevice(
    pSe05xSession_t session_ctx,
    SE05x_SPAKEDeviceType_t deviceType,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint8_t *pContext,
    size_t contextLen,
    uint8_t *pLabelA,
    size_t labelALen,
    uint8_t *pLabelB,
    size_t labelBLen)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr = { { kSE05x_CLA, kSE05x_INS_CRYPTO, kSE05x_P1_SPAKE, kSE05x_P2_PARAM } };
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen = 0;
    uint8_t *pCmdbuf = &cmdbuf[0];
    int tlvRet = 0;

#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "PAKEConfigDevice[]");
#endif /* VERBOSE_APDU_LOGS */

    tlvRet = TLVSET_U8("deviceType", &pCmdbuf, &cmdbufLen, kSE05x_TAG_1, deviceType);
    if (0 != tlvRet) {
        goto cleanup;
    }

    tlvRet = TLVSET_CryptoObjectID("cryptoObjectID", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, cryptoObjectID);
    if (0 != tlvRet) {
        goto cleanup;
    }

    tlvRet = TLVSET_u8bufOptional("context byteString", &pCmdbuf, &cmdbufLen, kSE05x_TAG_3, pContext, contextLen);
    if (0 != tlvRet) {
        goto cleanup;
    }

    tlvRet = TLVSET_u8bufOptional("Label A byteString", &pCmdbuf, &cmdbufLen, kSE05x_TAG_4, pLabelA, labelALen);
    if (0 != tlvRet) {
        goto cleanup;
    }

    tlvRet = TLVSET_u8bufOptional("Label B byteString", &pCmdbuf, &cmdbufLen, kSE05x_TAG_5, pLabelB, labelBLen);
    if (0 != tlvRet) {
        goto cleanup;
    }

    retStatus = DoAPDUTx_s_Case3(session_ctx, &hdr, cmdbuf, cmdbufLen);

cleanup:
    return retStatus;

}
#endif

smStatus_t  Se05x_API_PAKEReadDeviceType(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    SE05x_SPAKEDeviceType_t *deviceType)

{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr = { { kSE05x_CLA, kSE05x_INS_READ, kSE05x_P1_SPAKE, kSE05x_P2_DEFAULT } };
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen = 0;
    uint8_t *pCmdbuf = &cmdbuf[0];
    int tlvRet = 0;
    uint8_t rspbuf[SE05X_MAX_BUF_SIZE_RSP];
    uint8_t *pRspbuf = &rspbuf[0];
    size_t rspbufLen = ARRAY_SIZE(rspbuf);
    uint8_t devType = 0;
#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "PAKEReadDeviceType []");
#endif /* VERBOSE_APDU_LOGS */
    tlvRet = TLVSET_CryptoObjectID("cryptoObjectID", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, cryptoObjectID);
    if (0 != tlvRet) {
        goto cleanup;
    }

    retStatus = DoAPDUTxRx_s_Case4(session_ctx, &hdr, cmdbuf, cmdbufLen, rspbuf, &rspbufLen);
    if (retStatus == SM_OK) {
        retStatus = SM_NOT_OK;
        size_t rspIndex = 0;
        tlvRet = tlvGet_U8(pRspbuf, &rspIndex, rspbufLen, kSE05x_TAG_1, &devType);
        if (0 != tlvRet) {
            goto cleanup;
        }

        if ((rspIndex + 2) == rspbufLen) {
            retStatus = (pRspbuf[rspIndex] << 8) | (pRspbuf[rspIndex + 1]);
        }
    }
    if (deviceType != NULL)
        *deviceType = devType;
    else
        retStatus = SM_NOT_OK;
cleanup:
    return retStatus;
}

#if SSS_HAVE_SE05X_VER_GTE_16_02

smStatus_t  Se05x_API_PAKEInitDevice(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint8_t *pContext,
    size_t contextLen,
    uint8_t *pLabelA,
    size_t labelALen,
    uint8_t *pLabelB,
    size_t labelBLen)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr = { { kSE05x_CLA, kSE05x_INS_CRYPTO, kSE05x_P1_SPAKE, kSE05x_P2_ID } };
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen = 0;
    uint8_t *pCmdbuf = &cmdbuf[0];
    int tlvRet = 0;

#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "PAKEInitDevice[]");
#endif /* VERBOSE_APDU_LOGS */

    tlvRet = TLVSET_CryptoObjectID("cryptoObjectID", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, cryptoObjectID);
    if (0 != tlvRet) {
        goto cleanup;
    }

    tlvRet = TLVSET_u8bufOptional("context byteString", &pCmdbuf, &cmdbufLen, kSE05x_TAG_3, pContext, contextLen);
    if (0 != tlvRet) {
        goto cleanup;
    }

    tlvRet = TLVSET_u8bufOptional("Label A byteString", &pCmdbuf, &cmdbufLen, kSE05x_TAG_4, pLabelA, labelALen);
    if (0 != tlvRet) {
        goto cleanup;
    }

    tlvRet = TLVSET_u8bufOptional("Label B byteString", &pCmdbuf, &cmdbufLen, kSE05x_TAG_5, pLabelB, labelBLen);
    if (0 != tlvRet) {
        goto cleanup;
    }

    retStatus = DoAPDUTx_s_Case3(session_ctx, &hdr, cmdbuf, cmdbufLen);

cleanup:
    return retStatus;

}

smStatus_t  Se05x_API_PAKEInitCredentials(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint32_t objectID_w0,
    uint32_t objectID_w1,
    uint32_t objectID_L)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr = { { kSE05x_CLA, kSE05x_INS_CRYPTO, kSE05x_P1_SPAKE, kSE05x_P2_PARAM } };
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen = 0;
    uint8_t *pCmdbuf = &cmdbuf[0];
    int tlvRet = 0;

#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "PAKEInitCredentials[]");
#endif /* VERBOSE_APDU_LOGS */

    tlvRet = TLVSET_CryptoObjectID("cryptoObjectID", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, cryptoObjectID);
    if (0 != tlvRet) {
        goto cleanup;
    }

    if (objectID_w0 != 0)
    {
        tlvRet = TLVSET_U32("objectID w0", &pCmdbuf, &cmdbufLen, kSE05x_TAG_3, objectID_w0);
        if (0 != tlvRet) {
            goto cleanup;
        }
    }
    if (objectID_w1 != 0)
    {
        tlvRet = TLVSET_U32("objectID w1", &pCmdbuf, &cmdbufLen, kSE05x_TAG_4, objectID_w1);
        if (0 != tlvRet) {
            goto cleanup;
        }
    }

    if (objectID_L != 0)
    {
        tlvRet = TLVSET_U32("objectID L", &pCmdbuf, &cmdbufLen, kSE05x_TAG_5, objectID_L);
        if (0 != tlvRet) {
            goto cleanup;
        }
    }

    retStatus = DoAPDUTx_s_Case3(session_ctx, &hdr, cmdbuf, cmdbufLen);

cleanup:
    return retStatus;

}

#else
smStatus_t  Se05x_API_PAKEInitDevice(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint32_t objectID_w0,
    uint32_t objectID_w1,
    uint32_t objectID_L)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr = { { kSE05x_CLA, kSE05x_INS_CRYPTO, kSE05x_P1_SPAKE, kSE05x_P2_ID } };
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen = 0;
    uint8_t *pCmdbuf = &cmdbuf[0];
    int tlvRet = 0;

#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "PAKEInitDevice[]");
#endif /* VERBOSE_APDU_LOGS */

    tlvRet = TLVSET_CryptoObjectID("cryptoObjectID", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, cryptoObjectID);
    if (0 != tlvRet) {
        goto cleanup;
    }

    if (objectID_w0 != 0)
    {
        tlvRet = TLVSET_U32("objectID w0", &pCmdbuf, &cmdbufLen, kSE05x_TAG_3, objectID_w0);
        if (0 != tlvRet) {
            goto cleanup;
        }
    }
    if (objectID_w1 != 0)
    {
        tlvRet = TLVSET_U32("objectID w1", &pCmdbuf, &cmdbufLen, kSE05x_TAG_4, objectID_w1);
        if (0 != tlvRet) {
            goto cleanup;
        }
    }

    if (objectID_L != 0)
    {
        tlvRet = TLVSET_U32("objectID L", &pCmdbuf, &cmdbufLen, kSE05x_TAG_5, objectID_L);
        if (0 != tlvRet) {
            goto cleanup;
        }
    }

    retStatus = DoAPDUTx_s_Case3(session_ctx, &hdr, cmdbuf, cmdbufLen);

cleanup:
    return retStatus;

}
#endif

#if SSS_HAVE_SE05X_VER_GTE_16_03
smStatus_t  Se05x_API_PAKEComputeKeyShare(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint8_t *pInKey,
    size_t inKeyLen,
    uint8_t *pShareKey,
    size_t *pShareKeyLen)
#else
smStatus_t  Se05x_API_PAKEComputeKeyShare(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint8_t *pInKey,
    size_t inKeyLen,
    uint8_t *pShareKey,
    size_t *pShareKeyLen,
    uint8_t *pRandom,
    size_t randomLen)
#endif
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr = { { kSE05x_CLA, kSE05x_INS_CRYPTO, kSE05x_P1_SPAKE, kSE05x_P2_UPDATE } };
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen = 0;
    uint8_t *pCmdbuf = &cmdbuf[0];
    int tlvRet = 0;
    uint8_t rspbuf[SE05X_MAX_BUF_SIZE_RSP];
    uint8_t *pRspbuf = &rspbuf[0];
    size_t rspbufLen = ARRAY_SIZE(rspbuf);
#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "PAKEComputeKeyShare []");
#endif /* VERBOSE_APDU_LOGS */

    tlvRet = TLVSET_CryptoObjectID("cryptoObjectID", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, cryptoObjectID);
    if (0 != tlvRet) {
        goto cleanup;
    }

    tlvRet = TLVSET_u8bufOptional("Share key", &pCmdbuf, &cmdbufLen, kSE05x_TAG_3, pInKey, inKeyLen);
    if (0 != tlvRet) {
        goto cleanup;
    }

#if SSS_HAVE_SE05X_VER_GTE_16_03
#else
    tlvRet = TLVSET_u8bufOptional("Random value", &pCmdbuf, &cmdbufLen, kSE05x_TAG_4, pRandom, randomLen);
    if (0 != tlvRet) {
        goto cleanup;
    }
#endif

    retStatus = DoAPDUTxRx_s_Case4(session_ctx, &hdr, cmdbuf, cmdbufLen, rspbuf, &rspbufLen);
    if (retStatus == SM_OK) {
        retStatus       = SM_NOT_OK;
        size_t rspIndex = 0;
        tlvRet          = tlvGet_u8buf(pRspbuf, &rspIndex, rspbufLen, kSE05x_TAG_1, pShareKey, pShareKeyLen);
        if (0 != tlvRet) {
            goto cleanup;
        }
        if ((rspIndex + 2) == rspbufLen) {
            retStatus = (pRspbuf[rspIndex] << 8) | (pRspbuf[rspIndex + 1]);
        }
    }


cleanup:
    return retStatus;
}

smStatus_t  Se05x_API_PAKEComputeSessionKeys(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint8_t *pInKey,
    size_t inKeyLen,
    uint8_t *pShSecret,
    size_t *pShSecretLen,
    uint8_t *pKeyConfMessage,
    size_t *pkeyConfMessageLen)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr = { { kSE05x_CLA, kSE05x_INS_CRYPTO, kSE05x_P1_SPAKE, kSE05x_P2_GENERATE } };
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen = 0;
    uint8_t *pCmdbuf = &cmdbuf[0];
    int tlvRet = 0;
    uint8_t rspbuf[SE05X_MAX_BUF_SIZE_RSP];
    uint8_t *pRspbuf = &rspbuf[0];
    size_t rspbufLen = ARRAY_SIZE(rspbuf);
#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "PAKEComputeSessionKeys []");
#endif /* VERBOSE_APDU_LOGS */

    tlvRet = TLVSET_CryptoObjectID("cryptoObjectID", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, cryptoObjectID);
    if (0 != tlvRet) {
        goto cleanup;
    }

    tlvRet = TLVSET_u8bufOptional("Share key", &pCmdbuf, &cmdbufLen, kSE05x_TAG_3, pInKey, inKeyLen);
    if (0 != tlvRet) {
        goto cleanup;
    }

    retStatus = DoAPDUTxRx_s_Case4(session_ctx, &hdr, cmdbuf, cmdbufLen, rspbuf, &rspbufLen);
    if (retStatus == SM_OK) {
        retStatus = SM_NOT_OK;
        size_t rspIndex = 0;
        tlvRet = tlvGet_u8buf(pRspbuf, &rspIndex, rspbufLen, kSE05x_TAG_1, pShSecret, pShSecretLen);
        if (0 != tlvRet) {
            goto cleanup;
        }

        tlvRet = tlvGet_u8buf(pRspbuf, &rspIndex, rspbufLen, kSE05x_TAG_2, pKeyConfMessage, pkeyConfMessageLen);
        if (0 != tlvRet) {
            goto cleanup;
        }
        if ((rspIndex + 2) == rspbufLen) {
            retStatus = (pRspbuf[rspIndex] << 8) | (pRspbuf[rspIndex + 1]);
        }
    }

cleanup:
    return retStatus;
}


smStatus_t  Se05x_API_PAKEVerifySessionKeys(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint8_t *pKeyConfMessage,
    size_t keyConfMessageLen,
    uint8_t *presult)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr = { { kSE05x_CLA, kSE05x_INS_CRYPTO, kSE05x_P1_SPAKE, kSE05x_P2_VERIFY } };
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen = 0;
    uint8_t *pCmdbuf = &cmdbuf[0];
    int tlvRet = 0;
    uint8_t rspbuf[SE05X_MAX_BUF_SIZE_RSP];
    uint8_t *pRspbuf = &rspbuf[0];
    size_t rspbufLen = ARRAY_SIZE(rspbuf);
#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "PAKEVerifySessionKeys []");
#endif /* VERBOSE_APDU_LOGS */
    tlvRet = TLVSET_CryptoObjectID("cryptoObjectID", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, cryptoObjectID);
    if (0 != tlvRet) {
        goto cleanup;
    }

    tlvRet = TLVSET_u8bufOptional("Key Confirmation message", &pCmdbuf, &cmdbufLen, kSE05x_TAG_3, pKeyConfMessage, keyConfMessageLen);
    if (0 != tlvRet) {
        goto cleanup;
    }

    retStatus = DoAPDUTxRx_s_Case4(session_ctx, &hdr, cmdbuf, cmdbufLen, rspbuf, &rspbufLen);
    if (retStatus == SM_OK) {
        retStatus = SM_NOT_OK;
        size_t rspIndex = 0;
        size_t presultLen = 1 ;
        tlvRet = tlvGet_u8buf(pRspbuf, &rspIndex, rspbufLen, kSE05x_TAG_1, presult, &presultLen);
        if (0 != tlvRet) {
            goto cleanup;
        }
        if ((rspIndex + 2) == rspbufLen) {
            retStatus = (pRspbuf[rspIndex] << 8) | (pRspbuf[rspIndex + 1]);
        }
    }

cleanup:
    return retStatus;
}

#if SSS_HAVE_SE05X_VER_GTE_16_03
smStatus_t  Se05x_API_PAKEReadState(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    SE05x_PAKEState_t *pakeState)
{
    smStatus_t retStatus = SM_NOT_OK;
    tlvHeader_t hdr = { { kSE05x_CLA, kSE05x_INS_READ, kSE05x_P1_SPAKE, kSE05x_P2_READ_STATE } };
    uint8_t cmdbuf[SE05X_MAX_BUF_SIZE_CMD];
    size_t cmdbufLen = 0;
    uint8_t *pCmdbuf = &cmdbuf[0];
    int tlvRet = 0;
    uint8_t rspbuf[SE05X_MAX_BUF_SIZE_RSP];
    uint8_t *pRspbuf = &rspbuf[0];
    size_t rspbufLen = ARRAY_SIZE(rspbuf);
    uint8_t devType = 0;
#if VERBOSE_APDU_LOGS
    NEWLINE();
    nLog("APDU", NX_LEVEL_DEBUG, "PAKEReadDeviceType []");
#endif /* VERBOSE_APDU_LOGS */
    tlvRet = TLVSET_CryptoObjectID("cryptoObjectID", &pCmdbuf, &cmdbufLen, kSE05x_TAG_2, cryptoObjectID);
    if (0 != tlvRet) {
        goto cleanup;
    }

    retStatus = DoAPDUTxRx_s_Case4(session_ctx, &hdr, cmdbuf, cmdbufLen, rspbuf, &rspbufLen);
    if (retStatus == SM_OK) {
        retStatus = SM_NOT_OK;
        size_t rspIndex = 0;
        tlvRet = tlvGet_U8(pRspbuf, &rspIndex, rspbufLen, kSE05x_TAG_1, &devType);
        if (0 != tlvRet) {
            goto cleanup;
        }

        if ((rspIndex + 2) == rspbufLen) {
            retStatus = (pRspbuf[rspIndex] << 8) | (pRspbuf[rspIndex + 1]);
        }
    }
    if (pakeState != NULL)
        *pakeState = devType;
    else
        retStatus = SM_NOT_OK;
cleanup:
    return retStatus;
}
#endif

