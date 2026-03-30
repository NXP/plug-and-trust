/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if (SSS_HAVE_APPLET_SE051_H && SSS_HAVE_SE05X_VER_07_02)
/* OK */
#else
#error "Only with SE051_H based build"
#endif

/** Se05x_API_WriteBinary_V2
*
* See @ref Se05x_API_WriteBinary_V2. Also allows to set 2 bytes object id extension
*
*/
smStatus_t Se05x_API_WriteBinary_V2(pSe05xSession_t session_ctx,
    pSe05xPolicy_t policy,
    uint32_t objectID,
    uint16_t offset,
    uint16_t length,
    const uint8_t *inputData,
    size_t inputDataLen,
    uint16_t objectIDExt,
    uint32_t version);

/** Se05x_API_ReadObject_V2
 *
 * Reads the content of a Secure Object.
 *
 *  * If the object is a key pair, the command will return the key
 *    pair's public key.
 *
 *  * If the object is a public key, the command will return the public
 *    key.
 *
 *  * If the object is a private key or a symmetric key or a userID,
 *    the command will return SW_CONDITIONS_NOT_SATISFIED.
 *
 *  * If the object is a binary file, the file content is read, giving
 *    the offset in TLV[TAG_2] and the length to read in
 *    TLV[TAG_3]. Both TLV[TAG_2] and TLV[TAG_3] are bound together;
 *    i.e.. either both tags are present, or both are absent. If both
 *    are absent, the whole file content is returned.
 *
 *  * If the object is a monotonic counter, the counter value is
 *    returned.
 *
 *  * If the object is a PCR, the PCR value is returned.
 *
 *  * If TLV[TAG_4] is filled, only the modulus or public exponent of
 *    an RSA key pair or RSA public key is read. It does not apply to
 *    other Secure Object types.
 *
 * # Command to Applet
 *
 * @rst
 * +-------+------------+----------------------------------------------+
 * | Field | Value      | Description                                  |
 * +=======+============+==============================================+
 * | CLA   | 0x80       |                                              |
 * +-------+------------+----------------------------------------------+
 * | INS   | INS_READ   | See :cpp:type:`SE05x_INS_t`, in addition to  |
 * |       |            | INS_READ, users can set the INS_ATTEST flag. |
 * |       |            | In that case, attestation applies.           |
 * +-------+------------+----------------------------------------------+
 * | P1    | P1_DEFAULT | See :cpp:type:`SE05x_P1_t`                   |
 * +-------+------------+----------------------------------------------+
 * | P2    | P2_DEFAULT | See :cpp:type:`SE05x_P2_t`                   |
 * +-------+------------+----------------------------------------------+
 * | Lc    | #(Payload) | Payload Length.                              |
 * +-------+------------+----------------------------------------------+
 * |       | TLV[TAG_1] | 4-byte object identifier                     |
 * +-------+------------+----------------------------------------------+
 * |       | TLV[TAG_2] | 2-byte offset   [Optional: default 0]        |
 * |       |            | [Conditional: only when the object is a      |
 * |       |            | BinaryFile object]                           |
 * +-------+------------+----------------------------------------------+
 * |       | TLV[TAG_3] | 2-byte length   [Optional: default 0]        |
 * |       |            | [Conditional: only when the object is a      |
 * |       |            | BinaryFile object]                           |
 * +-------+------------+----------------------------------------------+
 * |       | TLV[TAG_8] | 2-byte object identifier extension - prefix  |
 * |       |            | [Optional: if not given, secure object       |
 * |       |            | identifier is considered as 4-byte           |
 * |       |            | identifier]   [Conditional: only for NFC     |
 * |       |            | Commissioning and SecureObjectType =         |
 * |       |            | TYPE_NFC_BINARY_FILE]                        |
 * +-------+------------+----------------------------------------------+
 * | Le    | 0x00       |                                              |
 * +-------+------------+----------------------------------------------+
 * @endrst
 *
 * # R-APDU Body
 *
 * @rst
 * +------------+--------------------------------------------+
 * | Value      | Description                                |
 * +============+============================================+
 * | TLV[TAG_1] | Data read from the secure object.          |
 * +------------+--------------------------------------------+
 * @endrst
 *
 * # R-APDU Trailer
 *
 * @rst
 * +-------------+--------------------------------+
 * | SW          | Description                    |
 * +=============+================================+
 * | SW_NO_ERROR | The read is done successfully. |
 * +-------------+--------------------------------+
 * @endrst
 *
 * @param[in] session_ctx Session Context [0:kSE05x_pSession]
 * @param[in] objectID object id [1:kSE05x_TAG_1]
 * @param[in] offset offset [2:kSE05x_TAG_2]
 * @param[in] length length [3:kSE05x_TAG_3]
 * @param[out] data  [0:kSE05x_TAG_1]
 * @param[in,out] pdataLen Length for data
 * @param[in] objectIDExt object id extension [8:kSE05x_TAG_8]
 */
smStatus_t Se05x_API_ReadObject_V2(
    pSe05xSession_t session_ctx, uint32_t objectID, uint16_t offset, uint16_t length, uint8_t *data, size_t *pdataLen, uint16_t objectIDExt);


/** Se05x_API_ReadObjectAttributes_V2
 *
 * See @ref Se05x_API_ReadObjectAttributes_V2. Also allows to set 2 bytes object id extension
 *
 * @param[in] session_ctx Session Context [0:kSE05x_pSession]
 * @param[in] objectID object id [1:kSE05x_TAG_1]
 * @param[out] data  [0:kSE05x_TAG_2] or [0:kSE05x_TAG_3]
 * @param[in,out] pdataLen Length for data
 * @param[in] objectIDExt object id extension [8:kSE05x_TAG_8]
 */
smStatus_t Se05x_API_ReadObjectAttributes_V2(
    pSe05xSession_t session_ctx, uint32_t objectID, uint8_t *data, size_t *pdataLen, uint16_t objectIDExt);

/** Se05x_API_ReadType_V2
 *
 * See @ref Se05x_API_ReadType_V2. Also allows to set 2 bytes object id extension
 *
 * @param[in] session_ctx Session Context [0:kSE05x_pSession]
 * @param[in] objectID object id [1:kSE05x_TAG_1]
 * @param[out] ptype  [0:kSE05x_TAG_1]
 * @param[out] pisTransient  [2:kSE05x_TAG_2]
 * @param[in] objectIDExt object id extension [8:kSE05x_TAG_8]
 */
smStatus_t Se05x_API_ReadType_V2(
    pSe05xSession_t session_ctx, uint32_t objectID, SE05x_SecureObjectType_t *ptype, uint8_t *pisTransient, uint16_t objectIDExt);

/** Se05x_API_ReadSize_V2
 *
 * See @ref Se05x_API_ReadSize_V2. Also allows to set 2 bytes object id extension
 *
 * @param[in] session_ctx Session Context [0:kSE05x_pSession]
 * @param[in] objectID object id [1:kSE05x_TAG_1]
 * @param[out] psize  [0:kSE05x_TAG_1]
 * @param[in] objectIDExt object id extension [8:kSE05x_TAG_8]
 */
smStatus_t Se05x_API_ReadSize_V2(pSe05xSession_t session_ctx, uint32_t objectID, uint16_t *psize, uint16_t objectIDExt);

/** Se05x_API_CheckObjectExists_V2
 *
 * See @ref Se05x_API_CheckObjectExists_V2. Also allows to set 2 bytes object id extension
 *
 * @param[in] session_ctx Session Context [0:kSE05x_pSession]
 * @param[in] objectID object id [1:kSE05x_TAG_1]
 * @param[out] presult  [0:kSE05x_TAG_1]
 * @param[in] objectIDExt object id extension [2:kSE05x_TAG_2]
 */
smStatus_t Se05x_API_CheckObjectExists_V2(
    pSe05xSession_t session_ctx, uint32_t objectID, SE05x_Result_t *presult, uint16_t objectIDExt);

/** Se05x_API_DeleteSecureObject_V2
 *
 * See @ref Se05x_API_DeleteSecureObject_V2. Also allows to set 2 bytes object id extension
 *
 * @param[in] session_ctx Session Context [0:kSE05x_pSession]
 * @param[in] objectID object id [1:kSE05x_TAG_1]
 * @param[in] objectIDExt object id extension [2:kSE05x_TAG_2]
 */
smStatus_t Se05x_API_DeleteSecureObject_V2(pSe05xSession_t session_ctx, uint32_t objectID, uint16_t objectIDExt);
