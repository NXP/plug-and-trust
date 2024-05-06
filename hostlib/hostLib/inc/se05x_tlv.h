/*
*
* Copyright 2019,2020,2024 NXP
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef SE05X_TLV_H_INC
#define SE05X_TLV_H_INC

#include <sm_types.h>
#include <se05x_enums.h>


#include "nxLog.h"
#include "nxScp03_Types.h"
//#include <smCom.h>

#if SSS_HAVE_SCP_SCP03_SSS
#if (defined(USE_RTOS) && (USE_RTOS == 1))
#include "semphr.h"
#elif (__GNUC__ && !AX_EMBEDDED)
#include <errno.h>
#include <pthread.h>
#endif
#endif // SSS_HAVE_SCP_SCP03_SSS

// #define VERBOSE_APDU_LOGS 1


#define kSE05x_CLA 0x80

typedef enum
{
    SM_NOT_OK = 0xFFFF,                         // Error
    SM_OK = 0x9000,                             // No Error
    SM_ERR_WRONG_LENGTH = 0x6700,               // Wrong length (e.g. C-APDU does not fit into APDU buffer)
    SM_ERR_CONDITIONS_NOT_SATISFIED = 0x6985,   // Conditions not satisfied
    SM_ERR_COMMAND_NOT_ALLOWED = 0x6986,        // Command not allowed - access denied based on object policy
    SM_ERR_SECURITY_STATUS = 0x6982,            // Security status not satisfied
    SM_ERR_WRONG_DATA = 0x6A80,                 // Wrong data provided
    SM_ERR_DATA_INVALID = 0x6984,               // Data invalid - policy set invalid for the given object
    SM_ERR_FILE_FULL = 0x6A84,                  // Not enough memory space available (either transient or persistent memory)
    SM_ERR_APDU_THROUGHPUT = 0x66A6,            // APDU Throughput error
} smStatus_t;


typedef enum
{
    CRED_DEFAULT = 0x00,
    CRED_EC = 0x01,
    CRED_RSA = 0x02,
    CRED_AES = 0x03,
    CRED_DES = 0x04,
    CRED_BINARY = 0x05,
    CRED_PIN = 0x06,
    CRED_COUNTER = 0x07,
    CRED_PCR = 0x08,
    CRED_OBJECT = 0x09,

    CRED_PUB_EC,
    CRED_PUB_RSA
} eSE05xType_t;


/** @addtogroup se05x_types
 *
 * @{ */

/** Se05xApdu_t struct representing an APDU for
* SE05x communication.
*/

typedef struct
{
    /** transmit buffer */
    uint8_t *se05xTxBuf;
    /** Length of transmit buffer */
    size_t se05xTxBufLen;
    /** With Session LC */
    size_t ws_LC;
    /** With Session LC Width 1 or 3 bytes */
    size_t ws_LCW;
    /** WithSession SE05X command */
    uint8_t *wsSe05x_cmd;
    /** WithSession SE05X command Length */
    size_t wsSe05x_cmdLen;
    /** WithSession SE05X Tag1 len */
    size_t wsSe05x_tag1Len;
    /** WithSession SE05X Tag1 Width */
    size_t wsSe05x_tag1W;
    /** WithSession SE05X Tag1 Command Data */
    uint8_t *wsSe05x_tag1Cmd;
    /** WithSession SE05X Tag1 Command Data Len */
    size_t wsSe05x_tag1CmdLen;
     /** SE05x Command Header */
    const tlvHeader_t *se05xCmd_hdr;
    /** SE05x Command LC */
    size_t se05xCmdLC;
    /** SE05x Command LC width */
    size_t se05xCmdLCW;
    /** SE05x Command */
    uint8_t *se05xCmd;
    /** SE05x Command Length */
    size_t se05xCmdLen;
    /** Pointer to data for MAC Calculation*/
    uint8_t *dataToMac;
     /** Length of data for MAC Calculation */
    size_t dataToMacLen;
} Se05xApdu_t;
/**
 *
 * @} */

struct Se05xSession;
struct _sss_se05x_tunnel_context;

/** struct Se05xSession represnting a session in SE05x
*
*/
typedef struct Se05xSession
{
    /** Array of 8 bytes represnting session value.*/
    uint8_t value[8];
    /** Indicating session is active*/
    uint8_t hasSession : 1;
    /** Type of authentication for the session*/
    SE_AuthType_t authType;
    /** auth ID associated with session*/
    uint32_t auth_id;
    uint8_t logical_channel;
    /** Meta Funciton
     *
     * Internall first calls fp_Transform
     * Then calls fp_RawTXn
     * Then calls fp_DeCrypt
     */
    smStatus_t(*fp_TXn)(struct Se05xSession * pSession,
        const tlvHeader_t *hdr, uint8_t *cmdBuf, size_t cmdBufLen, uint8_t *rsp, size_t *rspLen, uint8_t hasle);

    /** API called by fp_TXn. Helps handle UserID/Applet/ECKey to transform buffer.
     *
     * But this API never sends any data out over any communication link. */
    smStatus_t(*fp_Transform)(struct Se05xSession * pSession,
        /** IN */
        const tlvHeader_t *inHdr,
        /** IN */
        uint8_t *inCmdBuf,
        /** IN */
        size_t inCmdBufLen,
        /** OUT:
         *  For Session less,
         *      For Platform SCP this will be copy of,  inHDR, with outHdr[0] = outHdr[0] | 0x04
         *      For Plain Session: Same as inHDR
         *
         *  For With Session:
         *      This will be with TLV Header for Wrapped Session Command
         */
        tlvHeader_t *outHdr,
        /** OUT: For Session less, this will be copy of inCmdBuf
         *
         * For session based impelementation, this will have
         * TAG=Session, L=8,V=Session,TAG=TAG1,L=inCmdBufLen,inCmdBuf */
        uint8_t * pTxBuf,
        /** IN,OUT: */
        size_t * pTxBufLen,
        /** IN */
        uint8_t hasle);

    /** API called by fp_TXn. Helps handle Applet/Fast SCP to decrypt buffer.
    *
    * But this API never reads any data */
    smStatus_t(*fp_DeCrypt)(struct Se05xSession * pSession,
        size_t prevCmdBufLen,
        uint8_t *pInRxBuf,
        size_t *pInRxBufLen,
        uint8_t hasle);
#if SSS_HAVE_APPLET_SE05X_IOT
    /** It's either a minimal/single implemntation that calls smCom_TransceiveRaw()
     *
     * if pTunnelCtx is Null, directly call smCom_TransceiveRaw()
     *
     * Or an API part of tunnel ctx that can do PlatformSCP */
    smStatus_t (*fp_RawTXn)(void *conn_ctx,
        struct _sss_se05x_tunnel_context *pChannelCtx,
        SE_AuthType_t currAuth,
        const tlvHeader_t *hdr,
        uint8_t *cmdBuf,
        size_t cmdBufLen,
        uint8_t *rsp,
        size_t *rspLen,
        uint8_t hasle);
    /** pChannelCtx holds the context information for SE05x tunnel communication.
    *
    */
    struct _sss_se05x_tunnel_context * pChannelCtx;
#endif
#if SSS_HAVE_APPLET
    smStatus_t(*fp_Transmit)(
        SE_AuthType_t currAuth,
        const tlvHeader_t *hdr,
        uint8_t *cmdBuf,
        size_t cmdBufLen,
        uint8_t *rsp,
        size_t *rspLen,
        uint8_t hasle);
#endif
    /** pdynScp03Ctx holds the dynamic context information for SCP03 channel */
    NXSCP03_DynCtx_t *pdynScp03Ctx;

    /**Connection data context */
    void *conn_ctx;
    /** applet version*/
    uint32_t applet_version;

/*
#if SSS_HAVE_SCP_SCP03_SSS
#if (defined(USE_RTOS) && (USE_RTOS == 1))
    SemaphoreHandle_t scp03_lock;
    uint8_t scp03_lock_init;
#elif (__GNUC__ && !AX_EMBEDDED)
    pthread_mutex_t scp03_lock;
    uint8_t scp03_lock_init;
#endif
#endif // SSS_HAVE_SCP_SCP03_SSS
*/
} Se05xSession_t;

/** Se05xPolicy_t representing policy in SE05x
* Se05xPolicy_t structure defines a policy in SE05x with a policy value
* and its length
*/
typedef struct
{
    /** policy value */
    uint8_t *value;
    /** Length of Policy value */
    size_t value_len;
} Se05xPolicy_t;

/**SE05x_TimeStamp_t Representing timestamp in SE05x
*/
typedef struct
{
    /** TimeStamp Array */
    uint8_t ts[12];
} SE05x_TimeStamp_t;

/**SE05x_ExtendedFeatures_t Representing Extended feature in SE05x
*/
typedef struct
{
    /** Extended feature array */
    uint8_t features[30];
} SE05x_ExtendedFeatures_t;

/**Se05x_AppletFeatures_t Representing features of SE05x applet
*/
typedef struct
{
    /** Variant of the SE05x applet features. */
    SE05x_Variant_t variant;
    /** Pointer to extended_features. */
    SE05x_ExtendedFeatures_t *extended_features;
} Se05x_AppletFeatures_t;

typedef Se05x_AppletFeatures_t *pSe05xAppletFeatures_t;
typedef Se05xSession_t *pSe05xSession_t;
typedef Se05xPolicy_t *pSe05xPolicy_t;

#if defined(VERBOSE_APDU_LOGS) && (VERBOSE_APDU_LOGS == 1)
#define DO_LOG_V(TAG, DESCRIPTION, VALUE) nLog("APDU", NX_LEVEL_DEBUG, #TAG " [" DESCRIPTION "] = 0x%X", VALUE);
#define DO_LOG_A(TAG, DESCRIPTION, ARRAY, ARRAY_LEN) \
    nLog_au8("APDU", NX_LEVEL_DEBUG, #TAG " [" DESCRIPTION "]", ARRAY, ARRAY_LEN);
#else
#define DO_LOG_V(TAG, DESCRIPTION, VALUE)
#define DO_LOG_A(TAG, DESCRIPTION, ARRAY, ARRAY_LEN)
#endif

/** Creates a TLV set for SE05x Session*/
#define TLVSET_Se05xSession(DESCRIPTION, PBUF, PBUFLEN, TAG, SESSIONID) \
    TLVSET_u8buf(DESCRIPTION, PBUF, PBUFLEN, TAG, SESSIONID->value, sizeof(SESSIONID->value))

/** Creates a TLV set using parameters for SE05xPolicy information*/
#define TLVSET_Se05xPolicy(DESCRIPTION, PBUF, PBUFLEN, TAG, POLICY) \
    tlvSet_Se05xPolicy(DESCRIPTION, PBUF, PBUFLEN, TAG, POLICY)

/** Creates a TLV set with 8-bit unsigned integer value*/
#define TLVSET_U8(DESCRIPTION, PBUF, PBUFLEN, TAG, VALUE) \
    tlvSet_U8(PBUF, PBUFLEN, TAG, VALUE);                 \
    DO_LOG_V(TAG, DESCRIPTION, VALUE)

/** Creates a TLV set with -bit unsigned integer value*/
#define TLVSET_U16(DESCRIPTION, PBUF, PBUFLEN, TAG, VALUE) \
    tlvSet_U16(PBUF, PBUFLEN, TAG, VALUE);                 \
    DO_LOG_V(TAG, DESCRIPTION, VALUE)

/** Creates a TLV set with 16-bit unsigned integer value*/
#define TLVSET_U16Optional(DESCRIPTION, PBUF, PBUFLEN, TAG, VALUE) \
    tlvSet_U16Optional(PBUF, PBUFLEN, TAG, VALUE);                 \
    DO_LOG_V(TAG, DESCRIPTION, VALUE)

/** Creates a TLV set with 32-bit unsigned integer value*/
#define TLVSET_U32(DESCRIPTION, PBUF, PBUFLEN, TAG, VALUE) \
    tlvSet_U32(PBUF, PBUFLEN, TAG, VALUE);                 \
    DO_LOG_V(TAG, DESCRIPTION, VALUE)

/** Creates a TLV set with 64-bit unsigned integer value and a specified size*/
#define TLVSET_U64_SIZE(DESCRIPTION, PBUF, PBUFLEN, TAG, VALUE,SIZE) \
    tlvSet_U64_size(PBUF, PBUFLEN, TAG, VALUE,SIZE);                 \
    DO_LOG_V(TAG, DESCRIPTION, VALUE)

/** Creates a TLV set with KeyID*/
#define TLVSET_KeyID(DESCRIPTION, PBUF, PBUFLEN, TAG, VALUE) \
    tlvSet_KeyID(PBUF, PBUFLEN, TAG, VALUE);                 \
    DO_LOG_V(TAG, DESCRIPTION, VALUE)

/** Creates a TLV set with maximum attempts value*/
#define TLVSET_MaxAttemps(DESCRIPTION, PBUF, PBUFLEN, TAG, VALUE) \
    tlvSet_MaxAttemps(PBUF, PBUFLEN, TAG, VALUE);                 \
    DO_LOG_V(TAG, DESCRIPTION, VALUE)

/** See @ref TLVSET_U8 */
#define TLVSET_AttestationAlgo TLVSET_U8
/** See @ref TLVSET_U8 */
#define TLVSET_CipherMode TLVSET_U8

/** Creates a TLV set with EC Curve value*/
#define TLVSET_ECCurve(DESCRIPTION, PBUF, PBUFLEN, TAG, VALUE) \
    tlvSet_ECCurve(PBUF, PBUFLEN, TAG, VALUE);                 \
    DO_LOG_V(TAG, DESCRIPTION, VALUE)

/** See @ref TLVSET_U8 */
#define TLVSET_ECCurveParam TLVSET_U8
/** See @ref TLVSET_U8 */
#define TLVSET_ECSignatureAlgo TLVSET_U8
/** See @ref TLVSET_U8 */
#define TLVSET_EDSignatureAlgo TLVSET_U8
/** See @ref TLVSET_U8 */
#define TLVSET_MacOperation TLVSET_U8
/** See @ref TLVSET_U8 */
#define TLVSET_RSAEncryptionAlgo TLVSET_U8
/** See @ref TLVSET_U8 */
#define TLVSET_RSAKeyComponent TLVSET_U8
/** See @ref TLVSET_U8 */
#define TLVSET_RSASignatureAlgo TLVSET_U8
/** See @ref TLVSET_U8 */
#define TLVSET_DigestMode TLVSET_U8
/** See @ref TLVSET_U8 */
#define TLVSET_Variant tlvSet_u8buf_features
/** See @ref TLVSET_U8 */
#define TLVSET_RSAPubKeyComp TLVSET_U8
/** See @ref TLVSET_U8 */
#define TLVSET_PlatformSCPRequest TLVSET_U8
/** See @ref TLVSET_U8 */
#define TLVSET_MemoryType TLVSET_U8

/** See @ref TLVSET_U8 */
#define TLVSET_CryptoContext TLVSET_U8
/** See @ref TLVSET_U8 */
#define TLVSET_CryptoModeSubType(DESCRIPTION, PBUF, PBUFLEN, TAG, VALUE) \
    TLVSET_U8(DESCRIPTION, PBUF, PBUFLEN, TAG, ((VALUE).union_8bit))

/** See @ref TLVSET_U16 */
#define TLVSET_CryptoObjectID TLVSET_U16

// #define TLVSET_pVoid(DESCRIPTION, PBUF, PBUFLEN, TAG, VALUE) (0)
// #define tlvGet_pVoid(DESCRIPTION, PBUF, PBUFLEN, TAG, VALUE) (0)

#define TLVSET_u8buf(DESCRIPTION, PBUF, PBUFLEN, TAG, CMD, CMDLEN) \
    tlvSet_u8buf(PBUF, PBUFLEN, TAG, CMD, CMDLEN);                 \
    DO_LOG_A(TAG, DESCRIPTION, CMD, CMDLEN)

#define TLVSET_u8bufOptional(DESCRIPTION, PBUF, PBUFLEN, TAG, CMD, CMDLEN) \
    tlvSet_u8bufOptional(PBUF, PBUFLEN, TAG, CMD, CMDLEN);                 \
    DO_LOG_A(TAG, DESCRIPTION, CMD, CMDLEN)

#define TLVSET_u8bufOptional_ByteShift(DESCRIPTION, PBUF, PBUFLEN, TAG, CMD, CMDLEN) \
    tlvSet_u8bufOptional_ByteShift(PBUF, PBUFLEN, TAG, CMD, CMDLEN);                 \
    DO_LOG_A(TAG, DESCRIPTION, CMD, CMDLEN)


#define TLVSET_u8buf_I2CM(DESCRIPTION, PBUF, PBUFLEN, TAG, CMD, CMDLEN) \
    tlvSet_u8buf_I2CM(PBUF, PBUFLEN, TAG, CMD, CMDLEN);                 \
    DO_LOG_A(TAG, DESCRIPTION, CMD, CMDLEN)


int tlvSet_U8(uint8_t **buf, size_t *bufLen, SE05x_TAG_t tag, uint8_t value);
int tlvSet_U16(uint8_t **buf, size_t *bufLen, SE05x_TAG_t tag, uint16_t value);
int tlvSet_U16Optional(uint8_t **buf, size_t *bufLen, SE05x_TAG_t tag, uint16_t value);
int tlvSet_U32(uint8_t **buf, size_t *bufLen, SE05x_TAG_t tag, uint32_t value);
int tlvSet_U64_size(uint8_t **buf, size_t *bufLen, SE05x_TAG_t tag, uint64_t value,uint16_t size);
int tlvSet_u8buf(uint8_t **buf, size_t *bufLen, SE05x_TAG_t tag, const uint8_t *cmd, size_t cmdLen);
int tlvSet_u8bufOptional(uint8_t **buf, size_t *bufLen, SE05x_TAG_t tag, const uint8_t *cmd, size_t cmdLen);
/* Same as tlvSet_u8bufOptional, but some time, Most Significant Byte needs to be shifted and Plus by 1 */
int tlvSet_u8bufOptional_ByteShift(uint8_t **buf, size_t *bufLen, SE05x_TAG_t tag, const uint8_t *cmd, size_t cmdLen);
int tlvSet_Se05xPolicy(const char *description, uint8_t **buf, size_t *bufLen, SE05x_TAG_t tag, Se05xPolicy_t *policy);
int tlvSet_KeyID(uint8_t **buf, size_t *bufLen, SE05x_TAG_t tag, uint32_t keyID);
int tlvSet_MaxAttemps(uint8_t **buf, size_t *bufLen, SE05x_TAG_t tag, uint16_t maxAttemps);
int tlvSet_ECCurve(uint8_t **buf, size_t *bufLen, SE05x_TAG_t tag, SE05x_ECCurve_t value);
int tlvSet_u8buf_features(uint8_t **buf, size_t *bufLen, SE05x_TAG_t tag, pSe05xAppletFeatures_t appletVariant);

int tlvGet_U8(uint8_t *buf, size_t *pBufIndex, const size_t bufLen, SE05x_TAG_t tag, uint8_t *pRsp);
int tlvGet_U16(uint8_t *buf, size_t *pBufIndex, const size_t bufLen, SE05x_TAG_t tag, uint16_t *pRsp);
int tlvGet_U32(uint8_t *buf, size_t *pBufIndex, const size_t bufLen, SE05x_TAG_t tag, uint32_t *pRsp);

int tlvGet_u8buf(uint8_t *buf, size_t *pBufIndex, const size_t bufLen, SE05x_TAG_t tag, uint8_t *rsp, size_t *pRspLen);
int tlvGet_ValueIndex(uint8_t *buf, size_t *pBufIndex, const size_t bufLen, SE05x_TAG_t tag);
int tlvGet_Se05xSession(
    uint8_t *buf, size_t *pBufIndex, const size_t bufLen, SE05x_TAG_t tag, pSe05xSession_t *pSessionId);
int tlvGet_TimeStamp(uint8_t *buf, size_t *pBufIndex, const size_t bufLen, SE05x_TAG_t tag, SE05x_TimeStamp_t *pTs);

int tlvSet_u8buf_I2CM(uint8_t **buf, size_t *bufLen, SE05x_I2CM_TAG_t tag, const uint8_t *cmd, size_t cmdLen);

int tlvGet_SecureObjectType(uint8_t *buf, size_t *pBufIndex, size_t bufLen, SE05x_TAG_t tag, SE05x_SecObjTyp_t *pType);

int tlvGet_Result(uint8_t *buf, size_t *pBufIndex, size_t bufLen, SE05x_TAG_t tag, SE05x_Result_t *presult);



smStatus_t se05x_Transform(struct Se05xSession *pSession,
    const tlvHeader_t *hdr,
    uint8_t *cmdApduBuf,
    const size_t cmdApduBufLen,
    tlvHeader_t *out_hdr,
    uint8_t *txBuf,
    size_t *ptxBufLen,
    uint8_t hasle);

smStatus_t se05x_Transform_scp(struct Se05xSession *pSession,
    const tlvHeader_t *hdr,
    uint8_t *cmdApduBuf,
    const size_t cmdApduBufLen,
    tlvHeader_t *outhdr,
    uint8_t *txBuf,
    size_t *ptxBufLen,
    uint8_t hasle);

smStatus_t se05x_DeCrypt(struct Se05xSession *pSessionCtx,
    size_t cmd_cmacLen,
    uint8_t *rsp,
    size_t *rspLength,
    uint8_t hasle);

smStatus_t DoAPDUTxRx_s_Case2(Se05xSession_t *pSessionCtx,
    const tlvHeader_t *hdr,
    uint8_t *cmdBuf,
    size_t cmdBufLen,
    uint8_t *rspBuf,
    size_t *pRspBufLen);

smStatus_t DoAPDUTx_s_Case3(Se05xSession_t *pSessionCtx,
    const tlvHeader_t *hdr,
    uint8_t *cmdBuf,
    size_t cmdBufLen);

smStatus_t DoAPDUTxRx_s_Case4(Se05xSession_t *pSessionCtx,
    const tlvHeader_t *hdr,
    uint8_t *cmdBuf,
    size_t cmdBufLen,
    uint8_t *rspBuf,
    size_t *pRspBufLen);

smStatus_t DoAPDUTxRx_s_Case4_ext(Se05xSession_t *pSessionCtx,
    const tlvHeader_t *hdr,
    uint8_t *cmdBuf,
    size_t cmdBufLen,
    uint8_t *rspBuf,
    size_t *pRspBufLen);

smStatus_t DoAPDUTxRx(Se05xSession_t *pSessionCtx,
    uint8_t *cmdBuf,
    size_t cmdBufLen,
    uint8_t *rspBuf,
    size_t *pRspBufLen);

#if SSS_HAVE_APPLET_SE05X_IOT
smStatus_t Se05x_API_I2CM_Send(
    pSe05xSession_t sessionId, const uint8_t *buffer, size_t bufferLen, uint8_t *result, size_t *presultLen);
#endif
#endif // !SE05X_TLV_H_INC
