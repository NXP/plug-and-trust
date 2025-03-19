/*
* Copyright 2024-2025 NXP
* SPDX-License-Identifier: BSD-3-Clause
*
*/

#if defined(SSS_USE_FTR_FILE)
#include "fsl_sss_ftr.h"
#else
#include "fsl_sss_ftr_default.h"
#endif

#include <nxLog_sss.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if SSS_HAVE_APPLET_SE05X_IOT && SSSFTR_RSA && !SSS_HAVE_HOSTCRYPTO_NONE
#include "fsl_sss_util_rsa_sign_utils.h"

#include "nxEnsure.h"
#include "se05x_APDU.h"

/*
* Return:  SM_OK (0x9000): Success, SM_NOT_OK (0xFFFF): Error, SM_ERR_APDU_THROUGHPUT (0x66A6): ThroughPut error
*/
smStatus_t pkcs1_v15_encode(
    sss_se05x_asymmetric_t *context, const uint8_t *hash, size_t hash_len, uint8_t *out, size_t *out_len)
{
    size_t oid_size        = 0;
    size_t pad_len         = 0;
    unsigned char *out_ptr = out;
    /* clang-format off */
    unsigned char oid[16] = { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, };
    /* clang-format on */
    size_t out_length       = 0;
    uint16_t key_size_bytes = 0;
    smStatus_t ret_val      = SM_NOT_OK;

    /* Constants */
    const uint8_t RSA_Sign          = 0x01;
    const uint8_t ASN1_sequence     = 0x10;
    const uint8_t ASN1_constructed  = 0x20;
    const uint8_t ASN1_oid          = 0x06;
    const uint8_t ASN1_null         = 0x05;
    const uint8_t ASN1_octat_string = 0x04;

    ret_val = Se05x_API_ReadSize(&context->session->s_ctx, context->keyObject->keyId, &key_size_bytes);
    if (ret_val != SM_OK) {
        if (ret_val == SM_ERR_APDU_THROUGHPUT) {
            return SM_ERR_APDU_THROUGHPUT;
        }
        else {
            return SM_NOT_OK;
        }
    }

    out_length = key_size_bytes;
    pad_len    = out_length;

    switch (context->algorithm) {
    case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA1:
        oid[0]   = 0x2B;
        oid[1]   = 0x0E;
        oid[2]   = 0x03;
        oid[3]   = 0x02;
        oid[4]   = 0x1A;
        oid_size = 5;
        break;
    case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA224:
        oid[8]   = 0x04;
        oid_size = 9;
        break;
    case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA256:
        oid[8]   = 0x01;
        oid_size = 9;
        break;
    case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA384:
        oid[8]   = 0x02;
        oid_size = 9;
        break;
    case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA512:
        oid[8]   = 0x03;
        oid_size = 9;
        break;
    default:
        return SM_NOT_OK;
    }

    if (out_length < (hash_len + oid_size + 6 /* DigestInfo TLV overhead */)) {
        LOG_E("Intended encoded message length too short");
        return SM_NOT_OK;
    }

    if (*out_len < out_length) {
        LOG_E("Out buffer memory is less ");
        return SM_NOT_OK;
    }
    *out_len = out_length;

    /* Check if 1-byte ASN.1 length encoding can be used to
        * represent 8 + hash_len + oid_size without overflow. */
    if (8 + hash_len + oid_size >= 0x80) {
        return SM_NOT_OK;
    }

    /*
        * Check if following can be accomodated:
        * - 5 tag-length pairs (5 * 2 = 10 Bytes)
        *   (To protect against variants of Bleichenbacher's forgery attack
        *    against lax PKCS#1v1.5 verification,  1-Byte length encoding is used)
        * - For hash, hash_len number of bytes
        * - For hash algorithm, oid_size number of bytes
        */
    if (hash_len > (SIZE_MAX - (10 + oid_size))) {
        return SM_NOT_OK;
    }
    if (pad_len < 10 + hash_len + oid_size) {
        return SM_NOT_OK;
    }
    pad_len -= 10 + hash_len + oid_size;

    /* 3 Bytes (signature header) + 8 Bytes (minimal padding)*/
    if (pad_len < 3 + 8) {
        return SM_NOT_OK;
    }
    pad_len -= 3;

    /* Now pad_len is the amount of memory to be filled
        * with padding, and at least 8 bytes long. */

    /* Write signature header and padding */
    *out_ptr++ = 0;
    *out_ptr++ = RSA_Sign;
    memset(out_ptr, 0xFF, pad_len);
    out_ptr += pad_len;
    *out_ptr++ = 0;

    /* Signing hashed data, add corresponding ASN.1 structure
        *
        * Reference: PKCS #1: RSA Cryptography Specifications Version 2.2: Section 9.2.  EMSA-PKCS1-v1_5
        *
        * DigestInfo ::= SEQUENCE {
        *   digestAlgorithm DigestAlgorithmIdentifier,
        *   digest Digest }
        * DigestAlgorithmIdentifier ::= AlgorithmIdentifier
        * Digest ::= OCTET STRING
        *
        * Schematic:
        * TAG_SEQ +
        * LEN [ TAG_SEQ + LEN [ TAG_OID  + LEN [ OID  ]
        *                       + TAG_NULL + LEN [ NULL ]
        *                     ]
        *       + TAG_OCTET + LEN [ HASH ]
        *     ]
        */
    *out_ptr++ = ASN1_sequence | ASN1_constructed;
    *out_ptr++ = (unsigned char)(0x08 + oid_size + hash_len);
    *out_ptr++ = ASN1_sequence | ASN1_constructed;
    *out_ptr++ = (unsigned char)(0x04 + oid_size);
    *out_ptr++ = ASN1_oid;
    *out_ptr++ = (unsigned char)oid_size;
    memcpy(out_ptr, oid, oid_size);
    out_ptr += oid_size;
    *out_ptr++ = ASN1_null;
    *out_ptr++ = 0x00;
    *out_ptr++ = ASN1_octat_string;
    *out_ptr++ = (unsigned char)hash_len;
    memcpy(out_ptr, hash, hash_len);
    out_ptr += hash_len;

    /* Sanity check. */
    if (out_ptr != out + out_length) {
        memset(out, 0, out_length);
        return SM_NOT_OK;
    }

    return SM_OK;
}

/*
* Return:  SM_OK (0x9000): Success, SM_NOT_OK (0xFFFF): Error, SM_ERR_APDU_THROUGHPUT (0x66A6): ThroughPut error
*/
smStatus_t pkcs1_v15_encode_no_hash(
    sss_se05x_asymmetric_t *context, const uint8_t *hash, size_t hash_len, uint8_t *out, size_t *outLen)
{
    uint16_t key_size_bytes = 0;
    smStatus_t ret_val      = SM_NOT_OK;

    if ((out == NULL) || (outLen == NULL)) {
        return SM_NOT_OK;
    }
    if (*outLen < 2) {
        return SM_NOT_OK;
    }
    if (*outLen < 2) {
        return 1;
    }

    ret_val = Se05x_API_ReadSize(&context->session->s_ctx, context->keyObject->keyId, &key_size_bytes);
    if ((ret_val != SM_OK) || (key_size_bytes == 0)) {
        if (ret_val == SM_ERR_APDU_THROUGHPUT) {
            return 2;
        }
        else {
            return 1;
        }
    }

    ret_val = Se05x_API_ReadSize(&context->session->s_ctx, context->keyObject->keyId, &key_size_bytes);
    if ((ret_val != SM_OK) || (key_size_bytes == 0)) {
        if (ret_val == SM_ERR_APDU_THROUGHPUT) {
            return SM_ERR_APDU_THROUGHPUT;
        }
        else {
            return SM_NOT_OK;
        }
    }

    if (hash_len > (size_t)(key_size_bytes - 11)) {
        return SM_NOT_OK;
    }

    if (*outLen < key_size_bytes) {
        return SM_NOT_OK;
    }

    memset(out, 0xFF, *outLen);
    out[0] = 0x00;
    out[1] = 0x01;

    if ((key_size_bytes > *outLen) || (key_size_bytes <= hash_len)) {
        return SM_NOT_OK;
    }

    out[key_size_bytes - hash_len - 1] = 0x00;
    memcpy(&out[key_size_bytes - hash_len], hash, hash_len);
    *outLen = key_size_bytes;

    return SM_OK;
}

uint8_t sss_mgf_mask_func(uint8_t *dst, size_t dlen, uint8_t *src, size_t slen, sss_algorithm_t sha_algorithm)
{
    uint8_t mask[64] = {
        0,
    }; /* MAX - SHA512*/
    uint8_t counter[4] = {
        0,
    };
    uint8_t *p = NULL;
    size_t i, use_len;
    uint8_t ret         = 1;
    sss_status_t status = kStatus_SSS_Fail;
    sss_digest_t digest = {
        0,
    };
    size_t digestLen           = 512; /* MAX - SHA512*/
    size_t hash_length         = slen;
    sss_session_t host_session = {0};
#if SSS_HAVE_HOSTCRYPTO_MBEDTLS
    const sss_type_t host_crypto = kType_SSS_mbedTLS;
#elif SSS_HAVE_HOSTCRYPTO_OPENSSL
    const sss_type_t host_crypto = kType_SSS_OpenSSL;
#else
    const sss_type_t host_crypto = kType_SSS_SubSystem_NONE;
#endif

    memset(mask, 0, 64);
    memset(counter, 0, 4);

    status = sss_host_session_open(&host_session, host_crypto, 0, kSSS_ConnectionType_Plain, NULL);
    if (kStatus_SSS_Success != status) {
        goto exit;
    }

    /* Generate and apply dbMask */
    p = dst;

    while (dlen > 0) {
        use_len = hash_length;
        if (dlen < hash_length) {
            use_len = dlen;
        }

        status = sss_digest_context_init(&digest, &host_session, sha_algorithm, kMode_SSS_Digest);
        if (status != kStatus_SSS_Success) {
            goto exit;
        }

        status = sss_digest_init(&digest);
        if (status != kStatus_SSS_Success) {
            goto exit;
        }

        status = sss_digest_update(&digest, src, slen);
        if (status != kStatus_SSS_Success) {
            goto exit;
        }

        status = sss_digest_update(&digest, counter, 4);
        if (status != kStatus_SSS_Success) {
            goto exit;
        }

        status = sss_digest_finish(&digest, mask, &digestLen);
        if (status != kStatus_SSS_Success) {
            goto exit;
        }

        sss_digest_context_free(&digest);

        for (i = 0; i < use_len; ++i) {
            *p++ ^= mask[i];
        }

        counter[3]++;

        dlen -= use_len;
    }

    ret = 0;

exit:
    if (digest.session != NULL) {
        sss_digest_context_free(&digest);
    }
    sss_host_session_close(&host_session);

    return ret;
}

// Note-1: This function does not implement the full EMSA-PSS Encoding Operation operation
//         (refer to RFC 8017 Section 9.1 Figure 2), the caller MUST pass 'mHash' (= Hash(M)) as input
//         via function argument(s) hash / haslen.
//
// Note-2: Any hash value passed as input that does not match (in byte length)
//         the hash requested for the signature (kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHAxxx)
//         will be rejected.
//
// Return:  SM_OK (0x9000): Success, SM_NOT_OK (0xFFFF): Error, SM_ERR_APDU_THROUGHPUT (0x66A6): ThroughPut error
smStatus_t emsa_encode(
    sss_se05x_asymmetric_t *context, const uint8_t *hash, size_t hash_len, uint8_t *out, size_t *outLen)
{
    size_t outlength = 0;
    uint8_t *p       = out;
    uint8_t salt[64] = {
        0,
    };
    uint32_t saltlength  = 0;
    uint32_t hash_length = 0;
    uint32_t offset      = 0;
    size_t msb;
    sss_rng_context_t rng;
    sss_digest_t digest;
    sss_algorithm_t sha_algorithm = kAlgorithm_None;
    size_t digestLen              = 512; /* MAX - SHA512*/
    sss_status_t status           = kStatus_SSS_Fail;
    uint16_t key_size_bytes       = 0;
    smStatus_t ret_val            = SM_NOT_OK;
    sss_session_t host_session    = {0};
#if SSS_HAVE_HOSTCRYPTO_MBEDTLS
    const sss_type_t host_crypto = kType_SSS_mbedTLS;
#elif SSS_HAVE_HOSTCRYPTO_OPENSSL
    const sss_type_t host_crypto = kType_SSS_OpenSSL;
#else
    const sss_type_t host_crypto = kType_SSS_SubSystem_NONE;
#endif

    status = sss_host_session_open(&host_session, host_crypto, 0, kSSS_ConnectionType_Plain, NULL);
    if (kStatus_SSS_Success != status) {
        goto exit;
    }

    ret_val = Se05x_API_ReadSize(&context->session->s_ctx, context->keyObject->keyId, &key_size_bytes);
    if (ret_val != SM_OK) {
        goto exit;
    }

    outlength = key_size_bytes;
    ENSURE_OR_GO_EXIT(*outLen >= outlength);

    switch (context->algorithm) {
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA1:
        hash_length   = 20;
        sha_algorithm = kAlgorithm_SSS_SHA1;
        break;
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA224:
        hash_length   = 28;
        sha_algorithm = kAlgorithm_SSS_SHA224;
        break;
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA256:
        if (key_size_bytes <= 64) { /* RSA Key size = 512 */
            LOG_E("SHA256 not supported with this RSA key");
            goto exit;
        }
        hash_length   = 32;
        sha_algorithm = kAlgorithm_SSS_SHA256;
        break;
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA384:
        if (key_size_bytes <= 64) { /* RSA Key size = 512 */
            LOG_E("SHA384 not supported with this RSA key");
            goto exit;
        }
        hash_length   = 48;
        sha_algorithm = kAlgorithm_SSS_SHA384;
        break;
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA512:
        if (key_size_bytes <= 128) { /* RSA Key size = 1024 and 512 */
            LOG_E("SHA512 not supported with this RSA key");
            goto exit;
        }
        hash_length   = 64;
        sha_algorithm = kAlgorithm_SSS_SHA512;
        break;
    default:
        goto exit;
    }

    if (hash_length != hash_len) {
        ret_val = SM_NOT_OK;
        goto exit;
    }

    saltlength = hash_length;
    *outLen    = outlength;

    /* Generate salt of length saltlength */
    status = sss_rng_context_init(&rng, &host_session /* session */);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    status = sss_rng_get_random(&rng, salt, saltlength);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    msb = (outlength * 8) - 1;
    p += outlength - hash_length * 2 - 2;
    *p++ = 0x01;
    memcpy(p, salt, saltlength);
    p += saltlength;

    status = sss_digest_context_init(&digest, &host_session, sha_algorithm, kMode_SSS_Digest);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    status = sss_digest_init(&digest);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    status = sss_digest_update(&digest, p, 8);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    status = sss_digest_update(&digest, hash, hash_len);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    status = sss_digest_update(&digest, salt, saltlength);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    status = sss_digest_finish(&digest, p, &digestLen);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    sss_digest_context_free(&digest);

    if (msb % 8 == 0) {
        offset = 1;
    }

    /* Apply MGF Mask */
    if (0 != sss_mgf_mask_func(out + offset, outlength - hash_length - 1 - offset, p, hash_length, sha_algorithm)) {
        goto exit;
    }

    out[0] &= 0xFF >> (outlength * 8 - msb);

    p += hash_length;
    *p++ = 0xBC;

    ret_val = SM_OK;

exit:
    sss_host_session_close(&host_session);
    return ret_val;
}

smStatus_t emsa_decode_and_compare(
    sss_se05x_asymmetric_t *context, uint8_t *sig, size_t sig_len, const uint8_t *hash, size_t hash_len)
{
    uint8_t *p;
    uint8_t *hash_start;
    uint8_t result[512] = {0};
    smStatus_t ret      = SM_NOT_OK;
    uint32_t hlen;
    uint8_t zeros[8];
    uint32_t observed_salt_len, msb;
    uint8_t buf[1024];
    sss_algorithm_t sha_algorithm = kAlgorithm_None;
    sss_digest_t digest;
    size_t digestLen           = 512; /* MAX - SHA512*/
    sss_status_t status        = kStatus_SSS_Fail;
    sss_session_t host_session = {0};
#if SSS_HAVE_HOSTCRYPTO_MBEDTLS
    const sss_type_t host_crypto = kType_SSS_mbedTLS;
#elif SSS_HAVE_HOSTCRYPTO_OPENSSL
    const sss_type_t host_crypto = kType_SSS_OpenSSL;
#else
    const sss_type_t host_crypto = kType_SSS_SubSystem_NONE;
#endif

    ENSURE_OR_GO_EXIT(sig != NULL);
    ENSURE_OR_GO_EXIT(sig_len > 0);
    ENSURE_OR_GO_EXIT(hash != NULL);

    memcpy(buf, sig, sig_len);

    status = sss_host_session_open(&host_session, host_crypto, 0, kSSS_ConnectionType_Plain, NULL);
    if (kStatus_SSS_Success != status) {
        goto exit;
    }

    switch (context->algorithm) {
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA1:
        hlen          = 20;
        sha_algorithm = kAlgorithm_SSS_SHA1;
        break;
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA224:
        hlen          = 28;
        sha_algorithm = kAlgorithm_SSS_SHA224;
        break;
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA256:
        hlen          = 32;
        sha_algorithm = kAlgorithm_SSS_SHA256;
        break;
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA384:
        hlen          = 48;
        sha_algorithm = kAlgorithm_SSS_SHA384;
        break;
    case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA512:
        hlen          = 64;
        sha_algorithm = kAlgorithm_SSS_SHA512;
        break;
    default:
        goto exit;
    }

    p = buf;

    ENSURE_OR_GO_EXIT(sig_len <= sizeof(buf));
    if (buf[sig_len - 1] != 0xBC) {
        goto exit;
    }

    memset(zeros, 0, 8);

    msb = (sig_len * 8) - 1;
    /* Check if Most Significant bit is SET/UNSET */
    if (buf[0] >> (8 - 1)) {
        goto exit;
    }

    if (sig_len < hlen + 2) {
        goto exit;
    }
    hash_start = p + sig_len - hlen - 1;

    if (0 != sss_mgf_mask_func(p, sig_len - hlen - 1, hash_start, hlen, sha_algorithm)) {
        goto exit;
    }

    buf[0] &= 0xFF >> ((sig_len * 8 - msb) % 8);

    while (p < hash_start - 1 && *p == 0)
        p++;

    if (*p++ != 0x01) {
        goto exit;
    }

    observed_salt_len = hash_start - p;

    status = sss_digest_context_init(&digest, &host_session, sha_algorithm, kMode_SSS_Digest);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    status = sss_digest_init(&digest);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    status = sss_digest_update(&digest, zeros, 8);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    status = sss_digest_update(&digest, hash, hash_len);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    status = sss_digest_update(&digest, p, observed_salt_len);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    status = sss_digest_finish(&digest, result, &digestLen);
    if (status != kStatus_SSS_Success) {
        goto exit;
    }

    sss_digest_context_free(&digest);

    if (memcmp(hash_start, result, hlen) != 0) {
        goto exit;
    }

    ret = SM_OK;

exit:
    sss_host_session_close(&host_session);

    return ret;
}

#endif // SSS_HAVE_APPLET_SE05X_IOT && SSSFTR_RSA
