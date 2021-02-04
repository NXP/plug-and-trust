/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if (SSS_HAVE_APPLET_SE051_CHIP && SSS_HAVE_SE05X_VER_GTE_16_01)
/* OK */
#else
#error "Only with SE051_CHIP based build"
#endif

/** Se05x_API_PAKEInitProtocol
*
* See @ref Se05x_API_PAKEInitProtocol.
* Called to Init the PAKE Protocol.
*/
smStatus_t Se05x_API_PAKEInitProtocol(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint32_t objectID_M,
    uint32_t objectID_N );

/** Se05x_API_PAKEConfigDevice
*
* See @ref Se05x_API_PAKEConfigDevice.
* Called to Config the Device type.
*/
#if SSS_HAVE_SE05X_VER_GTE_16_02
smStatus_t  Se05x_API_PAKEConfigDevice(
    pSe05xSession_t session_ctx,
    SE05x_SPAKEDeviceType_t deviceType,
    SE05x_CryptoObjectID_t cryptoObjectID);
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
    size_t labelBLen);
#endif

/** Se05x_API_PAKEReadDeviceType
*
* See @ref Se05x_API_PAKEReadDeviceType.
* Used to read the PAKE device type
*/
smStatus_t  Se05x_API_PAKEReadDeviceType(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    SE05x_SPAKEDeviceType_t * deviceType);



#if SSS_HAVE_SE05X_VER_GTE_16_02
/** Se05x_API_PAKEInitDevice
*
* See @ref Se05x_API_PAKEInitDevice.
* Used to Init  the PAKE device type
*/
smStatus_t  Se05x_API_PAKEInitDevice(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint8_t *pContext,
    size_t contextLen,
    uint8_t *pLabelA,
    size_t labelALen,
    uint8_t *pLabelB,
    size_t labelBLen);

/** Se05x_API_PAKEInitCredentials
*
* See @ref Se05x_API_PAKEInitCredentials.
* Used to Init  the PAKE device type
*/
smStatus_t  Se05x_API_PAKEInitCredentials(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint32_t objectID_w0,
    uint32_t objectID_w1,
    uint32_t objectID_L);

#else
/** Se05x_API_PAKEInitDevice
*
* See @ref Se05x_API_PAKEInitDevice.
* Used to Init  the PAKE device type
*/
smStatus_t  Se05x_API_PAKEInitDevice(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint32_t objectID_w0,
    uint32_t objectID_w1,
    uint32_t objectID_L);
#endif

/** Se05x_API_PAKEComputeKeyShare
*
* See @ref Se05x_API_PAKEComputeKeyShare.
* Used to Compute the Key share of PAKE device type
*/
#if SSS_HAVE_SE05X_VER_GTE_16_03
smStatus_t  Se05x_API_PAKEComputeKeyShare(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint8_t *pInKey,
    size_t inKeyLen,
    uint8_t *pShareKey,
    size_t *pShareKeyLen);
#else
smStatus_t  Se05x_API_PAKEComputeKeyShare(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint8_t *pInKey,
    size_t inKeyLen,
    uint8_t *pShareKey,
    size_t *pShareKeyLen,
    uint8_t *pRandom,
    size_t randomLen);
#endif

/** Se05x_API_PAKEComputeSessionKeys
*
* See @ref Se05x_API_PAKEComputeSessionKeys.
* Used to Compute PAKE device Session Keys
*/
smStatus_t  Se05x_API_PAKEComputeSessionKeys(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint8_t *pInKey,
    size_t inKeyLen,
    uint8_t *pShSecret,
    size_t *pShSecretLen,
    uint8_t *pKeyConfMessage,
    size_t *pkeyConfMessageLen);

/** Se05x_API_PAKEVerifySessionKeys
*
* See @ref Se05x_API_PAKEVerifySessionKeys.
* Used to Verify PAKE device Verify Session Keys
*/
smStatus_t  Se05x_API_PAKEVerifySessionKeys(
    pSe05xSession_t session_ctx,
    SE05x_CryptoObjectID_t cryptoObjectID,
    uint8_t *pKeyConfMessage,
    size_t keyConfMessageLen,
    uint8_t *presult);
