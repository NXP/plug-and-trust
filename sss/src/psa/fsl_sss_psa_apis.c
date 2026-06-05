/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fsl_sss_mbedtls_apis.h>

#if SSS_USE_MBEDTLS_PSA_APIS

#include <fsl_sss_util_asn1_der.h>
#include <limits.h>
#include <nxEnsure.h>
#include <nxLog_sss.h>
#include <psa/crypto.h>
#include <string.h>
#if defined(MBEDTLS_THREADING_C) && defined(MBEDTLS_THREADING_ALT)
#include "mbedtls/threading.h"
#include "threading_alt.h"
#endif
#include "mbedtls/pk.h"

/* ************************************************************************** */
/* Private helper declarations                                                */
/* ************************************************************************** */

static psa_key_type_t sss_to_psa_key_type(se_sss_cipher_type_t cipherType,
                                          sss_key_part_t keyPart,
                                          size_t keyBitLen);
static psa_key_usage_t
sss_to_psa_key_usage(se_sss_access_permission_t accessRights,
                     sss_mode_t purpose, se_sss_cipher_type_t cipherType);
static psa_algorithm_t sss_to_psa_cipher_algorithm(sss_algorithm_t algorithm);
static psa_algorithm_t sss_to_psa_hash_algorithm(sss_algorithm_t algorithm);
static psa_algorithm_t sss_to_psa_mac_algorithm(sss_algorithm_t algorithm);
static psa_algorithm_t sss_to_psa_asym_algorithm(sss_algorithm_t algorithm);
static psa_algorithm_t sss_to_psa_derive_algorithm(sss_algorithm_t algorithm,
                                                   sss_mode_t mode);

static psa_algorithm_t sss_to_psa_mac_algorithm(sss_algorithm_t algorithm) {
  switch (algorithm) {
  case kAlgorithm_SSS_CMAC_AES:
    return PSA_ALG_CMAC; // 0x03c00200

  case kAlgorithm_SSS_HMAC_SHA1:
    return PSA_ALG_HMAC(PSA_ALG_SHA_1);

  case kAlgorithm_SSS_HMAC_SHA224:
    return PSA_ALG_HMAC(PSA_ALG_SHA_224);

  case kAlgorithm_SSS_HMAC_SHA256:
    return PSA_ALG_HMAC(PSA_ALG_SHA_256);

  case kAlgorithm_SSS_HMAC_SHA384:
    return PSA_ALG_HMAC(PSA_ALG_SHA_384);

  case kAlgorithm_SSS_HMAC_SHA512:
    return PSA_ALG_HMAC(PSA_ALG_SHA_512);

  default:
    return PSA_ALG_NONE;
  }
}

/* -------------------------------------------------------------------------
 * Map application key IDs to valid PSA user range [1 .. 1,073,741,823]
 *
 * Formula: psa_id = (sss_key_id % 1,073,741,822) + 1
 *
 * Examples:
 *   0x7FFF2000 → 0x3FFF2003
 *   0xEF000290 → 0x2F000297
 *
 * Collision Handling:
 *   If mapped psa_id already exists, the old key is DESTROYED.
 *   This is acceptable for session keys (current use case) but would
 *   require enhancement for persistent keys with overlapping ID ranges.
 *
 * Note: Application IDs > 0x7FFFFFFF exceed PSA_KEY_ID_VENDOR_MAX and
 *       must be mapped to avoid rejection by PSA Crypto API.
 * ------------------------------------------------------------------------- */
static psa_key_id_t sss_app_key_id_to_psa(uint32_t sss_key_id) {
  psa_key_id_t psa_id = (psa_key_id_t)(
      (sss_key_id % (PSA_KEY_ID_USER_MAX - PSA_KEY_ID_USER_MIN)) +
      PSA_KEY_ID_USER_MIN);

  psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
  if (psa_get_key_attributes(psa_id, &attrs) == PSA_SUCCESS) {
    psa_reset_key_attributes(&attrs);
    /* Destroy the existing key to allow overwrite */
    psa_status_t status = psa_destroy_key(psa_id);
    if (status != PSA_SUCCESS && status != PSA_ERROR_INVALID_HANDLE) {
      LOG_E("Failed to destroy existing key during collision handling: %d",
            status);
      return PSA_KEY_ID_NULL;
    }
  }
  return psa_id;
}

/**
 * Extract raw ECC public key from X.509 SubjectPublicKeyInfo using mbedTLS
 */
static sss_status_t extract_ecc_public_key_from_x509(const uint8_t *x509_data,
                                                     size_t x509_len,
                                                     uint8_t *raw_key,
                                                     size_t *raw_key_len,
                                                     size_t expected_key_bits) {
  int ret;
  mbedtls_pk_context pk;
  mbedtls_ecp_keypair *ecp;
  size_t expected_raw_len =
      (expected_key_bits / 8) * 2 + 1; // 65 for P-256 (0x04 + x + y)

  mbedtls_pk_init(&pk);

  /* Parse the X.509 SubjectPublicKeyInfo */
  ret = mbedtls_pk_parse_public_key(&pk, x509_data, x509_len);
  if (ret != 0) {
    LOG_E("mbedtls_pk_parse_public_key failed: -0x%04x", -ret);
    mbedtls_pk_free(&pk);
    return kStatus_SSS_Fail;
  }

  /* Verify it's an ECC key */
  if (mbedtls_pk_get_type(&pk) != MBEDTLS_PK_ECKEY &&
      mbedtls_pk_get_type(&pk) != MBEDTLS_PK_ECDSA) {
    LOG_E("Not an ECC public key");
    mbedtls_pk_free(&pk);
    return kStatus_SSS_Fail;
  }

  /* Get the ECC key pair structure */
  ecp = mbedtls_pk_ec(pk);
  if (ecp == NULL) {
    LOG_E("Failed to get ECC key pair");
    mbedtls_pk_free(&pk);
    return kStatus_SSS_Fail;
  }

  /* Check buffer size */
  if (*raw_key_len < expected_raw_len) {
    LOG_E("Output buffer too small for raw public key");
    mbedtls_pk_free(&pk);
    return kStatus_SSS_Fail;
  }

  /* Export the public key point in uncompressed format (0x04 || x || y) */
  ret = mbedtls_ecp_point_write_binary(
      &ecp->MBEDTLS_PRIVATE(grp), &ecp->MBEDTLS_PRIVATE(Q),
      MBEDTLS_ECP_PF_UNCOMPRESSED, raw_key_len, raw_key, expected_raw_len);

  if (ret != 0) {
    LOG_E("Failed to write public key: -0x%04x", -ret);
    mbedtls_pk_free(&pk);
    return kStatus_SSS_Fail;
  }

  LOG_D("Extracted %lu-byte raw ECC public key from X.509",
        (unsigned long)*raw_key_len);

  mbedtls_pk_free(&pk);
  return kStatus_SSS_Success;
}

/**
 * Extract raw ECC private key from DER/PKCS#8 format using mbedTLS
 */
static sss_status_t extract_ecc_private_key_from_der(const uint8_t *der_data,
                                                     size_t der_len,
                                                     uint8_t *raw_key,
                                                     size_t *raw_key_len,
                                                     size_t expected_key_bits) {
  int ret;
  mbedtls_pk_context pk;
  mbedtls_ecp_keypair *ecp;
  size_t expected_raw_len = expected_key_bits / 8; // 32 for P-256

  mbedtls_pk_init(&pk);

  /* Parse the DER-encoded private key */
  ret = mbedtls_pk_parse_key(&pk, der_data, der_len, NULL, 0, NULL, NULL);
  if (ret != 0) {
    LOG_E("mbedtls_pk_parse_key failed: -0x%04x", -ret);
    mbedtls_pk_free(&pk);
    return kStatus_SSS_Fail;
  }

  /* Verify it's an ECC key */
  if (mbedtls_pk_get_type(&pk) != MBEDTLS_PK_ECKEY &&
      mbedtls_pk_get_type(&pk) != MBEDTLS_PK_ECDSA) {
    LOG_E("Not an ECC key");
    mbedtls_pk_free(&pk);
    return kStatus_SSS_Fail;
  }

  /* Get the ECC key pair structure */
  ecp = mbedtls_pk_ec(pk);
  if (ecp == NULL) {
    LOG_E("Failed to get ECC key pair");
    mbedtls_pk_free(&pk);
    return kStatus_SSS_Fail;
  }

  /* Check buffer size */
  if (*raw_key_len < expected_raw_len) {
    LOG_E("Output buffer too small for raw private key");
    mbedtls_pk_free(&pk);
    return kStatus_SSS_Fail;
  }

  /* Extract the private key scalar 'd' */
  ret = mbedtls_mpi_write_binary(&ecp->MBEDTLS_PRIVATE(d), raw_key,
                                 expected_raw_len);
  if (ret != 0) {
    LOG_E("Failed to write private key: -0x%04x", -ret);
    mbedtls_pk_free(&pk);
    return kStatus_SSS_Fail;
  }

  *raw_key_len = expected_raw_len;

  LOG_D("Extracted %lu-byte raw ECC private key from DER",
        (unsigned long)expected_raw_len);

  mbedtls_pk_free(&pk);
  return kStatus_SSS_Success;
}

/* Helper function to convert key data to PSA-compatible format */
static sss_status_t
convert_key_to_psa_format(const uint8_t *key_data, size_t key_data_len,
                          psa_key_type_t key_type, size_t key_bits,
                          uint8_t *output_buffer, size_t output_buffer_size,
                          const uint8_t **psa_key_data, size_t *psa_key_len) {
  sss_status_t retval = kStatus_SSS_Fail;

  if (PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(key_type)) {
    /* ECC Public Key - convert from X.509 if needed */
    if (key_data_len > 65 && key_data[0] == 0x30) {
      *psa_key_len = output_buffer_size;
      retval = extract_ecc_public_key_from_x509(
          key_data, key_data_len, output_buffer, psa_key_len, key_bits);
      if (retval == kStatus_SSS_Success) {
        *psa_key_data = output_buffer;
      }
      return retval;
    }
  } else if (PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type)) {
    /* ECC Private Key - convert from DER if needed */
    if (key_data_len > 32 && key_data[0] == 0x30) {
      *psa_key_len = output_buffer_size;
      retval = extract_ecc_private_key_from_der(
          key_data, key_data_len, output_buffer, psa_key_len, key_bits);
      if (retval == kStatus_SSS_Success) {
        *psa_key_data = output_buffer;
      }
      return retval;
    }
  }

  /* Already in correct format or non-ECC key */
  *psa_key_data = key_data;
  *psa_key_len = key_data_len;
  return kStatus_SSS_Success;
}

/**
 * @brief Add DER/X.509 header to ECC public key based on PSA key type
 *
 * @param key Output buffer for DER-encoded key
 * @param keylen Total buffer size available
 * @param key_buf Returns pointer to where raw key data should be copied
 * @param key_buflen Returns offset/length after header
 * @param key_type PSA key type
 * @param key_bits Key size in bits
 * @return sss_status_t Success or failure
 */
static sss_status_t add_ecc_header_psa(uint8_t *key, size_t *keylen,
                                       uint8_t **key_buf, size_t *key_buflen,
                                       psa_key_type_t key_type,
                                       size_t key_bits) {
  sss_status_t status = kStatus_SSS_Fail;
  const uint8_t *header = NULL;
  size_t header_len = 0;
  psa_ecc_family_t curve_family = PSA_KEY_TYPE_ECC_GET_FAMILY(key_type);

  if (key == NULL || key_buf == NULL || key_buflen == NULL) {
    goto exit;
  }

  /* Select appropriate DER header based on curve family and key size */
  switch (curve_family) {
  case PSA_ECC_FAMILY_SECP_R1:
    /* NIST P curves */
    switch (key_bits) {
#if SSS_HAVE_EC_NIST_192
    case 192:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_nistp192_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_nistp192_header_len) >=
                        (*key_buflen));
      header = gecc_der_header_nist192;
      header_len = der_ecc_nistp192_header_len;
      break;
#endif
#if SSS_HAVE_EC_NIST_224
    case 224:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_nistp224_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_nistp224_header_len) >=
                        (*key_buflen));
      header = gecc_der_header_nist224;
      header_len = der_ecc_nistp224_header_len;
      break;
#endif
    case 256:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_nistp256_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_nistp256_header_len) >=
                        (*key_buflen));
      header = gecc_der_header_nist256;
      header_len = der_ecc_nistp256_header_len;
      break;

    case 384:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_nistp384_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_nistp384_header_len) >=
                        (*key_buflen));
      header = gecc_der_header_nist384;
      header_len = der_ecc_nistp384_header_len;
      break;

#if SSS_HAVE_EC_NIST_521
    case 521:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_nistp521_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_nistp521_header_len) >=
                        (*key_buflen));
      header = gecc_der_header_nist521;
      header_len = der_ecc_nistp521_header_len;
      break;
#endif
    default:
      LOG_W("Unsupported NIST P curve size: %zu bits", key_bits);
      goto exit;
    }
    break;

  case PSA_ECC_FAMILY_SECP_K1:
    /* NIST K curves */
#if SSS_HAVE_EC_NIST_K
    switch (key_bits) {
    case 160:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_160k_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_160k_header_len) >= (*key_buflen));
      header = gecc_der_header_160k;
      header_len = der_ecc_160k_header_len;
      break;

    case 192:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_192k_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_192k_header_len) >= (*key_buflen));
      header = gecc_der_header_192k;
      header_len = der_ecc_192k_header_len;
      break;

    case 224:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_224k_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_224k_header_len) >= (*key_buflen));
      header = gecc_der_header_224k;
      header_len = der_ecc_224k_header_len;
      break;

    case 256:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_256k_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_256k_header_len) >= (*key_buflen));
      header = gecc_der_header_256k;
      header_len = der_ecc_256k_header_len;
      break;

    default:
      LOG_W("Unsupported NIST K curve size: %zu bits", key_bits);
      goto exit;
    }
#else
    LOG_W("NIST K curves not supported in this build");
    goto exit;
#endif
    break;

  case PSA_ECC_FAMILY_BRAINPOOL_P_R1:
    /* Brainpool curves */
#if SSS_HAVE_EC_BP
    switch (key_bits) {
    case 160:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_bp160_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_bp160_header_len) >= (*key_buflen));
      header = gecc_der_header_bp160;
      header_len = der_ecc_bp160_header_len;
      break;

    case 192:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_bp192_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_bp192_header_len) >= (*key_buflen));
      header = gecc_der_header_bp192;
      header_len = der_ecc_bp192_header_len;
      break;

    case 224:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_bp224_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_bp224_header_len) >= (*key_buflen));
      header = gecc_der_header_bp224;
      header_len = der_ecc_bp224_header_len;
      break;

    case 256:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_bp256_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_bp256_header_len) >= (*key_buflen));
      header = gecc_der_header_bp256;
      header_len = der_ecc_bp256_header_len;
      break;

    case 320:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_bp320_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_bp320_header_len) >= (*key_buflen));
      header = gecc_der_header_bp320;
      header_len = der_ecc_bp320_header_len;
      break;

    case 384:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_bp384_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_bp384_header_len) >= (*key_buflen));
      header = gecc_der_header_bp384;
      header_len = der_ecc_bp384_header_len;
      break;

    case 512:
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_bp512_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_bp512_header_len) >= (*key_buflen));
      header = gecc_der_header_bp512;
      header_len = der_ecc_bp512_header_len;
      break;

    default:
      LOG_W("Unsupported Brainpool curve size: %zu bits", key_bits);
      goto exit;
    }
#else
    LOG_W("Brainpool curves not supported in this build");
    goto exit;
#endif
    break;

  case PSA_ECC_FAMILY_MONTGOMERY:
    /* Montgomery curves */
#if SSS_HAVE_EC_MONT
    if (key_bits == 255 || key_bits == 256) {
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_mont_dh_25519_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_mont_dh_25519_header_len) >=
                        (*key_buflen));
      header = gecc_der_header_mont_dh_25519;
      header_len = der_ecc_mont_dh_25519_header_len;
    }
#if SSS_HAVE_SE05X_VER_GTE_07_02
    else if (key_bits == 448) {
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_mont_dh_448_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_mont_dh_448_header_len) >=
                        (*key_buflen));
      header = gecc_der_header_mont_dh_448;
      header_len = der_ecc_mont_dh_448_header_len;
    }
#endif
    else {
      LOG_W("Unsupported Montgomery curve size: %zu bits", key_bits);
      goto exit;
    }
#else
    LOG_W("Montgomery curves not supported in this build");
    goto exit;
#endif
    break;

  case PSA_ECC_FAMILY_TWISTED_EDWARDS:
    /* Twisted Edwards curves */
#if SSS_HAVE_EC_ED
    if (key_bits == 255 || key_bits == 256) {
      ENSURE_OR_GO_EXIT((*keylen) > der_ecc_twisted_ed_25519_header_len);
      ENSURE_OR_GO_EXIT((SIZE_MAX - der_ecc_twisted_ed_25519_header_len) >=
                        (*key_buflen));
      header = gecc_der_header_twisted_ed_25519;
      header_len = der_ecc_twisted_ed_25519_header_len;
    } else {
      LOG_W("Unsupported Twisted Edwards curve size: %zu bits", key_bits);
      goto exit;
    }
#else
    LOG_W("Twisted Edwards curves not supported in this build");
    goto exit;
#endif
    break;

  default:
    LOG_W("Unsupported ECC curve family: %d", curve_family);
    goto exit;
  }

  /* Copy header to output buffer */
  if (header != NULL && header_len > 0) {
    memcpy(key, header, header_len);
    *key_buf = key + header_len;
    *key_buflen = header_len;
    status = kStatus_SSS_Success;
  }

exit:
  return status;
}

static int SetASNTLV(const uint8_t tag, const uint8_t *component,
                     const size_t componentLen, uint8_t *key, size_t *keyLen) {
  if (componentLen <= 0) {
    return 1;
  }

  if (*keyLen < componentLen) {
    return 1;
  }

  *keyLen = *keyLen - componentLen;
  memcpy(&key[*keyLen], component, componentLen);

  if (componentLen <= 127) {
    if (*keyLen < 1) {
      return 1;
    }
    *keyLen = *keyLen - 1;
    key[*keyLen] = (uint8_t)componentLen;
  } else if (componentLen <= 255) {
    if (*keyLen < 2) {
      return 1;
    }
    *keyLen = *keyLen - 2;
    key[*keyLen] = 0x81;
    key[*keyLen + 1] = (uint8_t)componentLen;
  } else {
    if (*keyLen < 3) {
      return 1;
    }
    *keyLen = *keyLen - 3;
    key[*keyLen] = 0x82;
    key[*keyLen + 1] = (componentLen & 0x00FF00) >> 8;
    key[*keyLen + 2] = (componentLen & 0x00FF);
  }

  if (*keyLen < 1) {
    return 1;
  }
  *keyLen = *keyLen - 1;

  key[*keyLen] = tag;

  return 0;
}

int EcSignatureToRandS(uint8_t *signature, size_t *sigLen) {
  int result = 1;
  uint8_t rands[128] = {0};
  int index = 0;
  size_t i = 0;
  size_t len = 0;
  if (signature[index++] != 0x30) {
    goto exit;
  }
  if (signature[index++] != (*sigLen - 2)) {
    goto exit;
  }
  if (signature[index++] != 0x02) {
    goto exit;
  }

  /* Parse length, skip initial 0x00 byte if present */
  len = signature[index++];
  if (len & 0x01) {
    len--;
    index++;
  }

  /* Copy R component*/
  for (i = 0; i < len; i++) {
    rands[i] = signature[index++];
  }

  if (signature[index++] != 0x02) {
    goto exit;
  }

  /* Parse length, skip initial 0x00 byte if present */
  len = signature[index++];
  if (len & 0x01) {
    len--;
    index++;
  }

  /* Copy S component*/
  len = len + i;
  for (; i < len; i++) {
    rands[i] = signature[index++];
  }

  /* Copy to output buffer and update length */
  memcpy(&signature[0], &rands[0], i);
  *sigLen = i;

  result = 0;

exit:
  return result;
}

int EcRandSToSignature(const uint8_t *rands, const size_t rands_len,
                       uint8_t *output, size_t *outputLen) {
  int result = 1;
  uint8_t signature[600] = {0};
  size_t signatureLen = sizeof(signature);
  size_t componentLen = (rands_len) / 2;
  uint8_t tag = ASN_TAG_INT;

  result = SetASNTLV(tag, &rands[componentLen], componentLen, signature,
                     &signatureLen);
  if (result != 0) {
    goto exit;
  }

  result = SetASNTLV(tag, &rands[0], componentLen, signature, &signatureLen);
  if (result != 0) {
    goto exit;
  }

  size_t totalLen = sizeof(signature) - signatureLen;

  if (totalLen <= 127) {
    if (signatureLen < 1) {
      result = 1;
      goto exit;
    }
    signatureLen = signatureLen - 1;
    signature[signatureLen] = (uint8_t)totalLen;
  } else if (totalLen <= 255) {
    if (signatureLen < 2) {
      result = 1;
      goto exit;
    }
    signatureLen = signatureLen - 2;
    signature[signatureLen] = 0x81;
    signature[signatureLen + 1] = (uint8_t)totalLen;
  } else {
    if (signatureLen < 3) {
      result = 1;
      goto exit;
    }
    signatureLen = signatureLen - 3;
    signature[signatureLen] = 0x82;
    signature[signatureLen + 1] = (totalLen & 0x00FF00) >> 8;
    signature[signatureLen + 2] = (totalLen & 0x00FF);
  }

  if (signatureLen < 1) {
    return 1;
  }
  signatureLen = signatureLen - 1;

  signature[signatureLen] = ASN_TAG_SEQUENCE;

  totalLen = sizeof(signature) - signatureLen;
  memcpy(&output[0], &signature[signatureLen], totalLen);
  *outputLen = totalLen;

  result = 0;

exit:
  return result;
}

static sss_status_t
sss_mbedtls_import_key_with_algorithm(sss_mbedtls_object_t *keyObject,
                                      psa_algorithm_t algorithm,
                                      psa_key_usage_t additional_usage) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
  psa_key_id_t key_id = PSA_KEY_ID_NULL;
  uint8_t raw_key_buffer[256];
  const uint8_t *key_data_to_import = NULL;
  size_t key_data_len = 0;

  ENSURE_OR_GO_CLEANUP(keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(keyObject->contents != NULL);
  ENSURE_OR_GO_CLEANUP(keyObject->contents_size > 0);

  /* If key already imported with this algorithm, reuse it */
  if (keyObject->psa_key_id != PSA_KEY_ID_NULL) {
    psa_key_attributes_t existing_attrs = PSA_KEY_ATTRIBUTES_INIT;
    psa_status = psa_get_key_attributes(keyObject->psa_key_id, &existing_attrs);

    if (psa_status == PSA_SUCCESS) {
      psa_algorithm_t existing_alg = psa_get_key_algorithm(&existing_attrs);
      psa_reset_key_attributes(&existing_attrs);

      /* If same algorithm, reuse the key */
      if (existing_alg == algorithm) {
        LOG_D("Reusing existing PSA key ID: %lu",
              (unsigned long)keyObject->psa_key_id);
        retval = kStatus_SSS_Success;
        goto cleanup;
      }
    }

    /* Different algorithm - destroy old key and create new one */
    LOG_D("Destroying old key (algorithm mismatch)");
    psa_destroy_key(keyObject->psa_key_id);
    keyObject->psa_key_id = PSA_KEY_ID_NULL;
  }

  /* Determine key type */
  psa_key_type_t key_type = sss_to_psa_key_type(
      (se_sss_cipher_type_t)keyObject->cipherType,
      (sss_key_part_t)keyObject->objectType, keyObject->keyBitLen);

  if (key_type == PSA_KEY_TYPE_NONE) {
    LOG_E("Unsupported cipher type for key import: %d", keyObject->cipherType);
    goto cleanup;
  }

  /* Convert key format if needed */
  retval = convert_key_to_psa_format(
      keyObject->contents, keyObject->contents_size, key_type,
      keyObject->keyBitLen, raw_key_buffer, sizeof(raw_key_buffer),
      &key_data_to_import, &key_data_len);

  if (retval != kStatus_SSS_Success) {
    LOG_E("Failed to convert key format");
    goto cleanup;
  }

  /* Set key attributes */
  psa_set_key_type(&attributes, key_type);
  psa_set_key_bits(&attributes, keyObject->keyBitLen);
  psa_set_key_algorithm(&attributes, algorithm);

  /* Set usage flags */
  psa_key_usage_t usage = additional_usage;
  if (keyObject->accessRights & kAccessPermission_SE_SSS_Use) {
    usage |= PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT |
             PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE;
  }
  if (keyObject->accessRights & kAccessPermission_SE_SSS_Read) {
    usage |= PSA_KEY_USAGE_EXPORT;
  }

  /* Add DERIVE for ECC keys */
  if (PSA_KEY_TYPE_IS_ECC(key_type)) {
    usage |= PSA_KEY_USAGE_DERIVE;
  }
  psa_set_key_usage_flags(&attributes, usage);

  /* Set persistence */
  if (keyObject->keyMode == kKeyObject_Mode_Persistent) {
    psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_PERSISTENT);
    psa_set_key_id(&attributes, sss_app_key_id_to_psa(keyObject->keyId));
  } else {
    psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
  }

  /* Import the key */

  psa_status =
      psa_import_key(&attributes, key_data_to_import, key_data_len, &key_id);

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA import key failed: %d", psa_status);
    goto cleanup;
  }

  keyObject->psa_key_id = key_id;

  retval = kStatus_SSS_Success;
  LOG_D("Successfully imported key with PSA ID: %lu", (unsigned long)key_id);

cleanup:
  psa_reset_key_attributes(&attributes);
  return retval;
}

/* ************************************************************************** */
/* Session Management                                                         */
/* ************************************************************************** */

// LCOV_EXCL_START
sss_status_t
sss_mbedtls_session_create(sss_mbedtls_session_t *session,
                           se_sss_type_t subsystem, uint32_t application_id,
                           se_sss_connection_type_t connection_type,
                           void *connectionData) {
  sss_status_t retval = kStatus_SSS_Success;

  AX_UNUSED_ARG(application_id);
  AX_UNUSED_ARG(connection_type);
  AX_UNUSED_ARG(connectionData);

  /* Nothing special to be handled */
  return retval;
}
// LCOV_EXCL_STOP

sss_status_t sss_mbedtls_session_open(sss_mbedtls_session_t *session,
                                      se_sss_type_t subsystem,
                                      uint32_t application_id,
                                      se_sss_connection_type_t connection_type,
                                      void *connectionData) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;

  ENSURE_OR_GO_EXIT(session != NULL);
  ENSURE_OR_GO_EXIT(connection_type == kSE_SSS_ConnectionType_Plain);

  AX_UNUSED_ARG(application_id);
  AX_UNUSED_ARG(connectionData);

  memset(session, 0, sizeof(*session));
#if defined(MBEDTLS_THREADING_C) && defined(MBEDTLS_THREADING_ALT)
  config_mbedtls_threading_alt();
#endif /* (MBEDTLS_THREADING_C) && defined(MBEDTLS_THREADING_ALT) */
  psa_status = psa_crypto_init();
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA Crypto init failed: %d", psa_status);
    goto exit;
  }

  session->subsystem = subsystem;
  retval = kStatus_SSS_Success;

exit:
  return retval;
}

sss_status_t sss_mbedtls_session_prop_get_u32(sss_mbedtls_session_t *session,
                                              uint32_t property,
                                              uint32_t *pValue) {
  sss_status_t retval = kStatus_SSS_Fail;
  AX_UNUSED_ARG(session);
  AX_UNUSED_ARG(property);
  AX_UNUSED_ARG(pValue);
  /* TBU */
  return retval;
}

sss_status_t sss_mbedtls_session_prop_get_au8(sss_mbedtls_session_t *session,
                                              uint32_t property,
                                              uint8_t *pValue,
                                              size_t *pValueLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  AX_UNUSED_ARG(session);
  AX_UNUSED_ARG(property);
  AX_UNUSED_ARG(pValue);
  AX_UNUSED_ARG(pValueLen);
  /* TBU */
  return retval;
}

void sss_mbedtls_session_close(sss_mbedtls_session_t *session) {
  if (session != NULL) {
    memset(session, 0, sizeof(*session));
  }
}

void sss_mbedtls_session_delete(sss_mbedtls_session_t *session) {
  AX_UNUSED_ARG(session);
}

/* End: psa_session */

/* ************************************************************************** */
/* Key Object Management                                                      */
/* ************************************************************************** */

sss_status_t sss_mbedtls_key_object_init(sss_mbedtls_object_t *keyObject,
                                         sss_mbedtls_key_store_t *keyStore) {
  sss_status_t retval = kStatus_SSS_Fail;

  ENSURE_OR_GO_CLEANUP(keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(keyStore != NULL);

  memset(keyObject, 0, sizeof(*keyObject));
  keyObject->keyStore = keyStore;
  keyObject->psa_key_id = PSA_KEY_ID_NULL;

  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_key_object_allocate_handle(
    sss_mbedtls_object_t *keyObject, uint32_t keyId, sss_key_part_t key_part,
    se_sss_cipher_type_t cipherType, size_t keyByteLenMax, uint32_t options) {
  sss_status_t retval = kStatus_SSS_Fail;

  ENSURE_OR_GO_CLEANUP(keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(keyId != 0);
  ENSURE_OR_GO_CLEANUP(keyId != 0xFFFFFFFFu);

  if (options != kKeyObject_Mode_Persistent &&
      options != kKeyObject_Mode_Transient) {
    LOG_E("sss_mbedtls_key_object_allocate_handle option invalid 0x%X",
          options);
    goto cleanup;
  }

  keyObject->keyId = keyId;
  keyObject->objectType = key_part;
  keyObject->cipherType = cipherType;
  keyObject->contents_max_size = keyByteLenMax;
  keyObject->keyMode = options;
  keyObject->accessRights = kAccessPermission_SE_SSS_All_Permission;
  /* Map once here — all PSA API calls use psa_key_id which is always valid */
  keyObject->psa_key_id = (options == kKeyObject_Mode_Persistent)
                              ? sss_app_key_id_to_psa(keyId)
                              : PSA_KEY_ID_NULL;

  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_key_object_get_handle(sss_mbedtls_object_t *keyObject,
                                               uint32_t keyId) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
  psa_status_t psa_status;

  ENSURE_OR_GO_CLEANUP(keyObject != NULL);

  keyObject->keyId = keyId; /* keep original for app use */

  /* Map once — derive the PSA-valid ID from the application key ID */
  psa_key_id_t mapped_id = sss_app_key_id_to_psa(keyId);

  psa_status = psa_get_key_attributes(mapped_id, &attributes);
  if (psa_status == PSA_SUCCESS) {
    keyObject->psa_key_id = mapped_id;
    retval = kStatus_SSS_Success;
  } else {
    /* Key does not exist yet — record the mapped ID for later import */
    keyObject->psa_key_id = mapped_id;
    retval = kStatus_SSS_Success;
  }

cleanup:
  psa_reset_key_attributes(&attributes);
  return retval;
}

sss_status_t sss_mbedtls_key_object_set_user(sss_mbedtls_object_t *keyObject,
                                             uint32_t user, uint32_t options) {
  sss_status_t retval = kStatus_SSS_Fail;
  AX_UNUSED_ARG(options);

  ENSURE_OR_GO_EXIT(
      (keyObject->accessRights & kAccessPermission_SE_SSS_ChangeAttributes));

  keyObject->user_id = user;
  retval = kStatus_SSS_Success;

exit:
  return retval;
}

sss_status_t sss_mbedtls_key_object_set_purpose(sss_mbedtls_object_t *keyObject,
                                                sss_mode_t purpose,
                                                uint32_t options) {
  sss_status_t retval = kStatus_SSS_Fail;
  AX_UNUSED_ARG(options);

  ENSURE_OR_GO_EXIT(
      (keyObject->accessRights & kAccessPermission_SE_SSS_ChangeAttributes));

  keyObject->purpose = purpose;
  retval = kStatus_SSS_Success;

exit:
  return retval;
}

sss_status_t sss_mbedtls_key_object_set_access(sss_mbedtls_object_t *keyObject,
                                               uint32_t access,
                                               uint32_t options) {
  sss_status_t retval = kStatus_SSS_Fail;
  AX_UNUSED_ARG(options);

  ENSURE_OR_GO_EXIT(
      (keyObject->accessRights & kAccessPermission_SE_SSS_ChangeAttributes));

  keyObject->accessRights = (se_sss_access_permission_t)access;
  retval = kStatus_SSS_Success;

exit:
  return retval;
}

// LCOV_EXCL_START
sss_status_t
sss_mbedtls_key_object_set_eccgfp_group(sss_mbedtls_object_t *keyObject,
                                        se_sss_eccgfp_group_t *group) {
  sss_status_t retval = kStatus_SSS_Success;
  AX_UNUSED_ARG(keyObject);
  AX_UNUSED_ARG(group);
  /* TBU */
  return retval;
}

sss_status_t sss_mbedtls_key_object_get_user(sss_mbedtls_object_t *keyObject,
                                             uint32_t *user) {
  if (user == NULL) {
    return kStatus_SSS_Fail;
  }
  *user = keyObject->user_id;
  return kStatus_SSS_Success;
}

sss_status_t sss_mbedtls_key_object_get_purpose(sss_mbedtls_object_t *keyObject,
                                                sss_mode_t *purpose) {
  if (purpose == NULL) {
    return kStatus_SSS_Fail;
  }
  *purpose = keyObject->purpose;
  return kStatus_SSS_Success;
}

sss_status_t sss_mbedtls_key_object_get_access(sss_mbedtls_object_t *keyObject,
                                               uint32_t *access) {
  if (access == NULL) {
    return kStatus_SSS_Fail;
  }
  *access = keyObject->accessRights;
  return kStatus_SSS_Success;
}
// LCOV_EXCL_STOP

void sss_mbedtls_key_object_free(sss_mbedtls_object_t *keyObject) {
  if (keyObject != NULL) {
    /* Destroy PSA key (both volatile and persistent) */
    if (keyObject->psa_key_id != PSA_KEY_ID_NULL) {
      psa_destroy_key(keyObject->psa_key_id);
      keyObject->psa_key_id = PSA_KEY_ID_NULL;
    }

    /* Free stored raw key data */
    if (keyObject->contents != NULL && keyObject->contents_must_free) {
      memset(keyObject->contents, 0,
             keyObject->contents_size); // Zero before free
      SSS_FREE(keyObject->contents);
      keyObject->contents = NULL;
    }

    memset(keyObject, 0, sizeof(*keyObject));
  }
}

/* End: psa_keyobj */

/* ************************************************************************** */
/* Key Store Management                                                       */
/* ************************************************************************** */

sss_status_t
sss_mbedtls_key_store_context_init(sss_mbedtls_key_store_t *keyStore,
                                   sss_mbedtls_session_t *session) {
  sss_status_t retval = kStatus_SSS_Fail;

  ENSURE_OR_GO_CLEANUP(keyStore != NULL);
  ENSURE_OR_GO_CLEANUP(session != NULL);

  memset(keyStore, 0, sizeof(*keyStore));
  keyStore->session = session;
  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_key_store_allocate(sss_mbedtls_key_store_t *keyStore,
                                            uint32_t keyStoreId) {
  AX_UNUSED_ARG(keyStore);
  AX_UNUSED_ARG(keyStoreId);
  /* PSA manages keys globally — no explicit allocation needed */
  return kStatus_SSS_Success;
}

sss_status_t sss_mbedtls_key_store_save(sss_mbedtls_key_store_t *keyStore) {
  AX_UNUSED_ARG(keyStore);
  /* PSA handles persistence internally */
  return kStatus_SSS_Success;
}

sss_status_t sss_mbedtls_key_store_load(sss_mbedtls_key_store_t *keyStore) {
  AX_UNUSED_ARG(keyStore);
  /* PSA handles loading internally */
  return kStatus_SSS_Success;
}

/* -------------------------------------------------------------------------
 * Private: map SSS cipher type → PSA key type
 * ------------------------------------------------------------------------- */
static psa_key_type_t sss_to_psa_key_type(se_sss_cipher_type_t cipherType,
                                          sss_key_part_t keyPart,
                                          size_t keyBitLen) {
  switch (cipherType) {
  case kSE_SSS_CipherType_AES:
    return PSA_KEY_TYPE_AES;

  case kSE_SSS_CipherType_DES:
    return PSA_KEY_TYPE_DES;

  case kSE_SSS_CipherType_HMAC:
    return PSA_KEY_TYPE_HMAC;

  case kSE_SSS_CipherType_CMAC:
    /* CMAC uses an AES key */
    return PSA_KEY_TYPE_AES;

  case kSE_SSS_CipherType_RSA:
  case kSE_SSS_CipherType_RSA_CRT:
    if (keyPart == kSSS_KeyPart_Public) {
      return PSA_KEY_TYPE_RSA_PUBLIC_KEY;
    }
    return PSA_KEY_TYPE_RSA_KEY_PAIR;

  case kSE_SSS_CipherType_EC_NIST_P:
    if (keyPart == kSSS_KeyPart_Public) {
      return PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1);
    }
    return PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1);

  case kSE_SSS_CipherType_EC_NIST_K:
    if (keyPart == kSSS_KeyPart_Public) {
      return PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_K1);
    }
    return PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_K1);

  case kSE_SSS_CipherType_EC_BRAINPOOL:
    if (keyPart == kSSS_KeyPart_Public) {
      return PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_BRAINPOOL_P_R1);
    }
    return PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_BRAINPOOL_P_R1);

  case kSE_SSS_CipherType_EC_MONTGOMERY:
    /* PSA uses 255-bit curve for Curve25519, 448 for Curve448.
     * keyBitLen == 256 corresponds to Curve25519 (mbedTLS convention). */
    if (keyBitLen == 256 || keyBitLen == 255) {
      if (keyPart == kSSS_KeyPart_Public) {
        return PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_MONTGOMERY);
      }
      return PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY);
    }
    if (keyBitLen == 448) {
      if (keyPart == kSSS_KeyPart_Public) {
        return PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_MONTGOMERY);
      }
      return PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY);
    }
    return PSA_KEY_TYPE_NONE;

  case kSE_SSS_CipherType_EC_TWISTED_ED:
    if (keyPart == kSSS_KeyPart_Public) {
      return PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS);
    }
    return PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS);

  default:
    return PSA_KEY_TYPE_NONE;
  }
}

/* -------------------------------------------------------------------------
 * Private: map SSS access rights → PSA key usage flags
 * ------------------------------------------------------------------------- */
static psa_key_usage_t
sss_to_psa_key_usage(se_sss_access_permission_t accessRights,
                     sss_mode_t purpose, se_sss_cipher_type_t cipherType) {
  psa_key_usage_t usage = 0;

  if (accessRights & kAccessPermission_SE_SSS_Use) {
    switch (purpose) {
    case kMode_SSS_Encrypt:
      usage |= PSA_KEY_USAGE_ENCRYPT;
      break;
    case kMode_SSS_Decrypt:
      usage |= PSA_KEY_USAGE_DECRYPT;
      break;
    case kMode_SSS_Sign:
      usage |= PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_SIGN_MESSAGE;
      break;
    case kMode_SSS_Verify:
      usage |= PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_VERIFY_MESSAGE;
      break;
    case kMode_SSS_ComputeSharedSecret:
      usage |= PSA_KEY_USAGE_DERIVE;
      break;
    case kMode_SSS_Mac:
    case kMode_SSS_Mac_Validate:
      usage |= PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE;
      break;
    default:
      /* Grant broad defaults per type so the key can be used for any
       * operation that makes sense for that cipher type. */
      if (cipherType == kSE_SSS_CipherType_AES ||
          cipherType == kSE_SSS_CipherType_DES) {
        usage |= PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT |
                 PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE;
      } else if (cipherType == kSE_SSS_CipherType_HMAC ||
                 cipherType == kSE_SSS_CipherType_CMAC) {
        usage |= PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE;
      } else if (cipherType == kSE_SSS_CipherType_EC_NIST_P ||
                 cipherType == kSE_SSS_CipherType_EC_NIST_K ||
                 cipherType == kSE_SSS_CipherType_EC_BRAINPOOL ||
                 cipherType == kSE_SSS_CipherType_EC_MONTGOMERY) {
        usage |= PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH |
                 PSA_KEY_USAGE_DERIVE;
      } else if (cipherType == kSE_SSS_CipherType_RSA ||
                 cipherType == kSE_SSS_CipherType_RSA_CRT) {
        usage |= PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH |
                 PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT;
      }
      break;
    }
  }

  if (accessRights & kAccessPermission_SE_SSS_Read) {
    usage |= PSA_KEY_USAGE_EXPORT;
  }

  /* ECDH curves always need DERIVE */
  if (cipherType == kSE_SSS_CipherType_EC_NIST_P ||
      cipherType == kSE_SSS_CipherType_EC_MONTGOMERY ||
      cipherType == kSE_SSS_CipherType_EC_NIST_K) {
    usage |= PSA_KEY_USAGE_DERIVE;
  }

  return usage;
}

sss_status_t sss_mbedtls_key_store_set_key(sss_mbedtls_key_store_t *keyStore,
                                           sss_mbedtls_object_t *keyObject,
                                           const uint8_t *data, size_t dataLen,
                                           size_t keyBitLen, void *options,
                                           size_t optionsLen) {
  sss_status_t retval = kStatus_SSS_Fail;

  AX_UNUSED_ARG(options);
  AX_UNUSED_ARG(optionsLen);
  AX_UNUSED_ARG(keyStore);

  ENSURE_OR_GO_CLEANUP(keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(data != NULL);
  ENSURE_OR_GO_CLEANUP(dataLen > 0);
  ENSURE_OR_GO_CLEANUP(
      (keyObject->accessRights & kAccessPermission_SE_SSS_Write));

  /* Destroy existing key if present */
  if (keyObject->psa_key_id != PSA_KEY_ID_NULL) {
    psa_destroy_key(keyObject->psa_key_id);
  }

  /* Free existing contents if we allocated it */
  if (keyObject->contents != NULL && keyObject->contents_must_free) {
    memset(keyObject->contents, 0, keyObject->contents_size);
    SSS_FREE(keyObject->contents);
    keyObject->contents = NULL;
    keyObject->contents_must_free = 0;
  }
  keyObject->contents = SSS_MALLOC(dataLen);
  ENSURE_OR_GO_CLEANUP(keyObject->contents != NULL);
  keyObject->contents_must_free = 1;

  /* Store the raw key data */
  memcpy(keyObject->contents, data, dataLen);
  keyObject->contents_size = dataLen;
  keyObject->keyBitLen = keyBitLen;

  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t
sss_mbedtls_key_store_generate_key(sss_mbedtls_key_store_t *keyStore,
                                   sss_mbedtls_object_t *keyObject,
                                   size_t keyBitLen, void *options) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
  psa_key_id_t key_id = PSA_KEY_ID_NULL;

  AX_UNUSED_ARG(options);
  AX_UNUSED_ARG(keyStore);

  ENSURE_OR_GO_CLEANUP(keyObject != NULL);

  /* Destroy existing key if present */
  if (keyObject->psa_key_id != PSA_KEY_ID_NULL) {
    psa_destroy_key(keyObject->psa_key_id);
    keyObject->psa_key_id = PSA_KEY_ID_NULL;
  }

  psa_key_type_t key_type =
      sss_to_psa_key_type((se_sss_cipher_type_t)keyObject->cipherType,
                          (sss_key_part_t)keyObject->objectType, keyBitLen);

  if (key_type == PSA_KEY_TYPE_NONE) {
    LOG_E("Unsupported cipher type for generation: %d", keyObject->cipherType);
    goto cleanup;
  }

  psa_set_key_type(&attributes, key_type);
  psa_set_key_bits(&attributes, keyBitLen);

  psa_key_usage_t usage =
      sss_to_psa_key_usage(keyObject->accessRights, keyObject->purpose,
                           (se_sss_cipher_type_t)keyObject->cipherType);
  psa_set_key_usage_flags(&attributes, usage);

  psa_algorithm_t algorithm = PSA_ALG_NONE;
  if (PSA_KEY_TYPE_IS_ECC(key_type)) {
    /* For ECC keys, use ECDH as the primary algorithm */
    algorithm = PSA_ALG_ECDH;
    LOG_D("Setting ECDH algorithm for ECC key generation");
  } else if (PSA_KEY_TYPE_IS_RSA(key_type)) {
    algorithm = PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_ANY_HASH);
    LOG_D("Setting RSA sign algorithm for RSA key generation");
  }
  psa_set_key_algorithm(&attributes, algorithm);

  if (keyObject->keyMode == kKeyObject_Mode_Persistent) {
    psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_PERSISTENT);
    psa_set_key_id(&attributes, sss_app_key_id_to_psa(keyObject->keyId));
  } else {
    psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
  }

  psa_status = psa_generate_key(&attributes, &key_id);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA generate key failed: %d", psa_status);
    goto cleanup;
  }

  keyObject->psa_key_id = key_id;
  keyObject->keyBitLen = keyBitLen;
  retval = kStatus_SSS_Success;

cleanup:
  psa_reset_key_attributes(&attributes);
  return retval;
}

sss_status_t sss_mbedtls_key_store_get_key(sss_mbedtls_key_store_t *keyStore,
                                           sss_mbedtls_object_t *keyObject,
                                           uint8_t *key, size_t *keyLen,
                                           size_t *pKeyBitLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
  size_t exported_length = 0;

  AX_UNUSED_ARG(keyStore);

  ENSURE_OR_GO_CLEANUP(keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(key != NULL);
  ENSURE_OR_GO_CLEANUP(keyLen != NULL);
  ENSURE_OR_GO_CLEANUP(
      (keyObject->accessRights & kAccessPermission_SE_SSS_Read));

  switch (keyObject->cipherType) {
  case kSE_SSS_CipherType_UserID:
    /* For UserID, use stored contents directly */
    ENSURE_OR_GO_CLEANUP(keyObject->contents != NULL);
    ENSURE_OR_GO_CLEANUP(*keyLen >= keyObject->contents_size);
    memcpy(key, keyObject->contents, keyObject->contents_size);
    *keyLen = keyObject->contents_size;
    if (pKeyBitLen != NULL) {
      *pKeyBitLen = keyObject->contents_size * 8;
    }
    retval = kStatus_SSS_Success;
    break;

  default:
    /* For all other cipher types, export from PSA */
    ENSURE_OR_GO_CLEANUP(keyObject->psa_key_id != PSA_KEY_ID_NULL);

    psa_status = psa_get_key_attributes(keyObject->psa_key_id, &attributes);
    if (psa_status != PSA_SUCCESS) {
      LOG_E("Failed to get key attributes: %d", psa_status);
      goto cleanup;
    }

    psa_key_usage_t usage = psa_get_key_usage_flags(&attributes);
    if (!(usage & PSA_KEY_USAGE_EXPORT)) {
      LOG_E("Key is not exportable");
      goto cleanup;
    }

    psa_key_type_t key_type = psa_get_key_type(&attributes);
    /* For ECC public keys or key pairs, export public key in X.509 format */
    if (PSA_KEY_TYPE_IS_ECC(key_type) &&
        (keyObject->objectType == kSSS_KeyPart_Public ||
         keyObject->objectType == kSSS_KeyPart_Pair)) {

      /* Export raw public key */
      uint8_t raw_key[133]; /* Max for P-521: 1 + 2*66 bytes */
      size_t raw_key_len = sizeof(raw_key);

      psa_status = psa_export_public_key(keyObject->psa_key_id, raw_key,
                                         raw_key_len, &exported_length);

      if (psa_status != PSA_SUCCESS) {
        LOG_E("PSA export public key failed: %d", psa_status);
        goto cleanup;
      }

      /* Add DER header using PSA-specific function */
      size_t key_bits = psa_get_key_bits(&attributes);
      uint8_t *key_buf = NULL;
      size_t key_buflen = 0;
      size_t total_keylen = *keyLen;

      retval = add_ecc_header_psa(key, &total_keylen, &key_buf, &key_buflen,
                                  key_type, key_bits);

      if (retval == kStatus_SSS_Success && key_buf != NULL) {
        /* Header was added, copy raw key after header */
        if (total_keylen < key_buflen + exported_length) {
          LOG_E("Buffer too small for DER-encoded key");
          retval = kStatus_SSS_Fail;
          goto cleanup;
        }
        memcpy(key_buf, raw_key, exported_length);
        *keyLen = key_buflen + exported_length;
      } else {
        /* No header added - return raw key */
        LOG_W("Returning raw ECC public key (no DER header)");
        if (*keyLen < exported_length) {
          LOG_E("Buffer too small for raw key");
          goto cleanup;
        }
        memcpy(key, raw_key, exported_length);
        *keyLen = exported_length;
        retval = kStatus_SSS_Success;
      }

    } else {
      /* Export other key types as-is */
      psa_status =
          psa_export_key(keyObject->psa_key_id, key, *keyLen, &exported_length);

      if (psa_status != PSA_SUCCESS) {
        LOG_E("PSA export key failed: %d", psa_status);
        goto cleanup;
      }
      *keyLen = exported_length;
      retval = kStatus_SSS_Success;
    }

    if (pKeyBitLen != NULL) {
      *pKeyBitLen = psa_get_key_bits(&attributes);
    }
    break;
  }

cleanup:
  psa_reset_key_attributes(&attributes);
  return retval;
}

sss_status_t sss_mbedtls_key_store_open_key(sss_mbedtls_key_store_t *keyStore,
                                            sss_mbedtls_object_t *keyObject) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

  ENSURE_OR_GO_CLEANUP(keyStore != NULL);
  ENSURE_OR_GO_CLEANUP(keyObject != NULL);

  if (keyObject->keyMode == kKeyObject_Mode_Persistent) {
    /* Use the already-mapped psa_key_id set during allocate/get_handle */
    psa_key_id_t key_id = keyObject->psa_key_id;

    /* Safety: if psa_key_id was not yet mapped, map it now */
    if (key_id == PSA_KEY_ID_NULL) {
      key_id = sss_app_key_id_to_psa(keyObject->keyId);
      keyObject->psa_key_id = key_id;
    }

    psa_status = psa_get_key_attributes(key_id, &attributes);
    if (psa_status == PSA_SUCCESS) {
      retval = kStatus_SSS_Success;
    } else {
      LOG_W("Persistent key %u (mapped from 0x%08X) not found", key_id,
            keyObject->keyId);
    }
  } else {
    /* Transient keys are not opened from storage */
    retval = kStatus_SSS_Success;
  }

cleanup:
  psa_reset_key_attributes(&attributes);
  return retval;
}

sss_status_t sss_mbedtls_key_store_freeze_key(sss_mbedtls_key_store_t *keyStore,
                                              sss_mbedtls_object_t *keyObject) {
  AX_UNUSED_ARG(keyStore);

  if (keyObject != NULL) {
    keyObject->accessRights &= ~(uint32_t)kAccessPermission_SE_SSS_Write;
    keyObject->accessRights &=
        ~(uint32_t)kAccessPermission_SE_SSS_ChangeAttributes;
  }

  return kStatus_SSS_Success;
}

sss_status_t sss_mbedtls_key_store_erase_key(sss_mbedtls_key_store_t *keyStore,
                                             sss_mbedtls_object_t *keyObject) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;

  ENSURE_OR_GO_CLEANUP(keyStore != NULL);
  ENSURE_OR_GO_CLEANUP(keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(
      (keyObject->accessRights & kAccessPermission_SE_SSS_Delete));

  if (keyObject->psa_key_id != PSA_KEY_ID_NULL) {
    psa_status = psa_destroy_key(keyObject->psa_key_id);
    if (psa_status == PSA_SUCCESS || psa_status == PSA_ERROR_INVALID_HANDLE) {
      keyObject->psa_key_id = PSA_KEY_ID_NULL;
      retval = kStatus_SSS_Success;
    } else {
      LOG_E("Failed to destroy PSA key: %d", psa_status);
    }
  } else {
    retval = kStatus_SSS_Success;
  }

cleanup:
  return retval;
}

void sss_mbedtls_key_store_context_free(sss_mbedtls_key_store_t *keyStore) {
  if (keyStore != NULL) {
    memset(keyStore, 0, sizeof(*keyStore));
  }
}

/* End: psa_keystore */

/* ************************************************************************** */
/* Random Number Generation                                                   */
/* ************************************************************************** */

sss_status_t sss_mbedtls_rng_context_init(sss_mbedtls_rng_context_t *context,
                                          sss_mbedtls_session_t *session) {
  sss_status_t retval = kStatus_SSS_Fail;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(session != NULL);

  context->session = session;
  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_rng_get_random(sss_mbedtls_rng_context_t *context,
                                        uint8_t *random_data, size_t dataLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(random_data != NULL);
  ENSURE_OR_GO_CLEANUP(dataLen > 0);

  psa_status = psa_generate_random(random_data, dataLen);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA generate random failed: %d", psa_status);
    goto cleanup;
  }

  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_rng_context_free(sss_mbedtls_rng_context_t *context) {
  if (context != NULL) {
    memset(context, 0, sizeof(*context));
  }
  return kStatus_SSS_Success;
}

/* End: psa_rng */

/* ************************************************************************** */
/* Symmetric Cipher                                                           */
/* ************************************************************************** */

static psa_algorithm_t sss_to_psa_cipher_algorithm(sss_algorithm_t algorithm) {
  switch (algorithm) {
  case kAlgorithm_SSS_AES_ECB:
    return PSA_ALG_ECB_NO_PADDING;
  case kAlgorithm_SSS_AES_CBC:
    return PSA_ALG_CBC_NO_PADDING;
  case kAlgorithm_SSS_AES_CTR:
    return PSA_ALG_CTR;
  case kAlgorithm_SSS_DES_ECB:
    return PSA_ALG_ECB_NO_PADDING;
  case kAlgorithm_SSS_DES_CBC:
  case kAlgorithm_SSS_DES3_CBC:
    return PSA_ALG_CBC_NO_PADDING;
  case kAlgorithm_SSS_DES3_ECB:
    return PSA_ALG_ECB_NO_PADDING;
  default:
    return PSA_ALG_NONE;
  }
}

sss_status_t
sss_mbedtls_symmetric_context_init(sss_mbedtls_symmetric_t *context,
                                   sss_mbedtls_session_t *session,
                                   sss_mbedtls_object_t *keyObject,
                                   sss_algorithm_t algorithm, sss_mode_t mode) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_algorithm_t psa_alg;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(session != NULL);
  ENSURE_OR_GO_CLEANUP(keyObject != NULL);

  memset(context, 0, sizeof(*context));
  /* Allocate cipher operation */
  context->cipher_op =
      (psa_cipher_operation_t *)SSS_CALLOC(1, sizeof(psa_cipher_operation_t));
  ENSURE_OR_GO_CLEANUP(context->cipher_op != NULL);
  *context->cipher_op = psa_cipher_operation_init();

  context->session = session;
  context->keyObject = keyObject;
  context->algorithm = algorithm;
  context->mode = mode;
  context->operation_initialized = false;

  /* Determine PSA algorithm for cipher operations */
  psa_alg = sss_to_psa_cipher_algorithm(algorithm);
  if (psa_alg == PSA_ALG_NONE) {
    LOG_E("Unsupported cipher algorithm: %d", algorithm);
    goto cleanup;
  }

  /* Import the key with the correct cipher algorithm */
  retval = sss_mbedtls_import_key_with_algorithm(
      keyObject, psa_alg, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
  ENSURE_OR_GO_CLEANUP(retval == kStatus_SSS_Success);

  retval = kStatus_SSS_Success;

cleanup:
  if (retval != kStatus_SSS_Success && context != NULL &&
      context->cipher_op != NULL) {
    SSS_FREE(context->cipher_op);
    context->cipher_op = NULL;
  }
  return retval;
}

sss_status_t sss_mbedtls_cipher_one_go(sss_mbedtls_symmetric_t *context,
                                       uint8_t *iv, size_t ivLen,
                                       const uint8_t *srcData,
                                       uint8_t *destData, size_t dataLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_algorithm_t alg;
  size_t update_out = 0;
  size_t finish_out = 0;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(srcData != NULL);
  ENSURE_OR_GO_CLEANUP(destData != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject->psa_key_id != PSA_KEY_ID_NULL);

  alg = sss_to_psa_cipher_algorithm(context->algorithm);
  if (alg == PSA_ALG_NONE) {
    LOG_E("Unsupported cipher algorithm: %d", context->algorithm);
    goto cleanup;
  }

  psa_cipher_operation_t op = psa_cipher_operation_init();

  /* Setup cipher operation */
  if (context->mode == kMode_SSS_Encrypt) {
    psa_status =
        psa_cipher_encrypt_setup(&op, context->keyObject->psa_key_id, alg);
  } else if (context->mode == kMode_SSS_Decrypt) {
    psa_status =
        psa_cipher_decrypt_setup(&op, context->keyObject->psa_key_id, alg);
  } else {
    LOG_E("Invalid cipher mode: %d", context->mode);
    goto cleanup;
  }

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA cipher setup failed: %d", psa_status);
    goto cleanup;
  }

  /* Set IV if provided */
  if (iv != NULL && ivLen > 0) {
    psa_status = psa_cipher_set_iv(&op, iv, ivLen);
    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA cipher set IV failed: %d", psa_status);
      psa_cipher_abort(&op);
      goto cleanup;
    }
  }

  size_t output_buffer_size = dataLen;

  /* Process data */
  psa_status = psa_cipher_update(&op, srcData, dataLen, destData,
                                 output_buffer_size, &update_out);

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA cipher update failed: %d", psa_status);
    psa_cipher_abort(&op);
    goto cleanup;
  }

  if (update_out > output_buffer_size) {
    LOG_E("cipher_one_go: update output exceeded buffer");
    psa_cipher_abort(&op);
    goto cleanup;
  }
  /* Finish operation - may add padding */
  size_t finish_buffer_size = output_buffer_size - update_out;

  psa_status = psa_cipher_finish(&op, destData + update_out, finish_buffer_size,
                                 &finish_out);

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA cipher finish failed: %d", psa_status);
    psa_cipher_abort(&op);
    goto cleanup;
  }

  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_cipher_init(sss_mbedtls_symmetric_t *context,
                                     uint8_t *iv, size_t ivLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_algorithm_t alg;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->cipher_op != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject->psa_key_id != PSA_KEY_ID_NULL);

  if (context->operation_initialized) {
    psa_cipher_abort(context->cipher_op);
    *context->cipher_op = psa_cipher_operation_init();
    context->operation_initialized = false;
  }

  alg = sss_to_psa_cipher_algorithm(context->algorithm);
  if (alg == PSA_ALG_NONE) {
    LOG_E("Unsupported cipher algorithm: %d", context->algorithm);
    goto cleanup;
  }

  if (context->mode == kMode_SSS_Encrypt) {
    psa_status = psa_cipher_encrypt_setup(context->cipher_op,
                                          context->keyObject->psa_key_id, alg);
  } else if (context->mode == kMode_SSS_Decrypt) {
    psa_status = psa_cipher_decrypt_setup(context->cipher_op,
                                          context->keyObject->psa_key_id, alg);
  } else {
    LOG_E("Invalid cipher mode: %d", context->mode);
    goto cleanup;
  }

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA cipher setup failed: %d", psa_status);
    goto cleanup;
  }

  context->operation_initialized = true;

  if (iv != NULL && ivLen > 0) {
    psa_status = psa_cipher_set_iv(context->cipher_op, iv, ivLen);
    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA cipher set IV failed: %d", psa_status);
      goto cleanup;
    }
  }

  retval = kStatus_SSS_Success;

cleanup:
  if (retval != kStatus_SSS_Success && context->operation_initialized) {
    psa_cipher_abort(context->cipher_op);
    *context->cipher_op = psa_cipher_operation_init();
    context->operation_initialized = false;
  }
  return retval;
}

sss_status_t sss_mbedtls_cipher_update(sss_mbedtls_symmetric_t *context,
                                       const uint8_t *srcData, size_t srcLen,
                                       uint8_t *destData, size_t *destLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  size_t output_length = 0;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(srcData != NULL);
  ENSURE_OR_GO_CLEANUP(destData != NULL);
  ENSURE_OR_GO_CLEANUP(context->cipher_op != NULL);
  ENSURE_OR_GO_CLEANUP(destLen != NULL);
  ENSURE_OR_GO_CLEANUP(context->operation_initialized);

  psa_status = psa_cipher_update(context->cipher_op, srcData, srcLen, destData,
                                 *destLen, &output_length);

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA cipher update failed: %d", psa_status);
    goto cleanup;
  }

  *destLen = output_length;
  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_cipher_finish(sss_mbedtls_symmetric_t *context,
                                       const uint8_t *srcData, size_t srcLen,
                                       uint8_t *destData, size_t *destLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  size_t update_out = 0;
  size_t finish_out = 0;
  size_t total_out = 0;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(destData != NULL);
  ENSURE_OR_GO_CLEANUP(destLen != NULL);
  ENSURE_OR_GO_CLEANUP(context->operation_initialized);
  ENSURE_OR_GO_CLEANUP(context->cipher_op != NULL);

  if (srcData != NULL && srcLen > 0) {
    psa_status = psa_cipher_update(context->cipher_op, srcData, srcLen,
                                   destData, *destLen, &update_out);
    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA cipher update in finish failed: %d", psa_status);
      goto cleanup;
    }
    total_out = update_out;
  }

  psa_status = psa_cipher_finish(context->cipher_op, destData + total_out,
                                 *destLen - total_out, &finish_out);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA cipher finish failed: %d", psa_status);
    goto cleanup;
  }

  total_out += finish_out;
  *destLen = total_out;

  context->operation_initialized = false;
  retval = kStatus_SSS_Success;

cleanup:
  if (context->operation_initialized) {
    psa_cipher_abort(context->cipher_op);
    *context->cipher_op = psa_cipher_operation_init();
    context->operation_initialized = false;
  }
  return retval;
}

sss_status_t sss_mbedtls_cipher_crypt_ctr(sss_mbedtls_symmetric_t *context,
                                          const uint8_t *srcData,
                                          uint8_t *destData, size_t size,
                                          uint8_t *initialCounter,
                                          uint8_t *lastEncryptedCounter,
                                          size_t *szLeft) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_cipher_operation_t op = psa_cipher_operation_init();
  size_t update_out = 0;
  size_t finish_out = 0;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(srcData != NULL);
  ENSURE_OR_GO_CLEANUP(destData != NULL);
  ENSURE_OR_GO_CLEANUP(initialCounter != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject->psa_key_id != PSA_KEY_ID_NULL);
  ENSURE_OR_GO_CLEANUP(context->algorithm == kAlgorithm_SSS_AES_CTR);

  /*
   * CTR mode is identical for encrypt and decrypt.
   * We use the multi-step API so we can supply the caller-provided
   * counter (IV) rather than letting PSA generate a random one.
   */
  if (context->mode == kMode_SSS_Encrypt ||
      context->mode == kMode_SSS_Decrypt) {
    psa_status = psa_cipher_encrypt_setup(&op, context->keyObject->psa_key_id,
                                          PSA_ALG_CTR);
  } else {
    LOG_E("Invalid mode for CTR: %d", context->mode);
    goto cleanup;
  }

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA CTR setup failed: %d", psa_status);
    goto cleanup;
  }

  /* Supply the 16-byte counter block as the IV */
  psa_status = psa_cipher_set_iv(&op, initialCounter, 16);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA CTR set IV failed: %d", psa_status);
    psa_cipher_abort(&op);
    goto cleanup;
  }

  psa_status =
      psa_cipher_update(&op, srcData, size, destData, size + 16, &update_out);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA CTR update failed: %d", psa_status);
    psa_cipher_abort(&op);
    goto cleanup;
  }

  psa_status = psa_cipher_finish(&op, destData + update_out, 16, &finish_out);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA CTR finish failed: %d", psa_status);
    psa_cipher_abort(&op);
    goto cleanup;
  }

  /*
   * PSA manages the counter state internally.  We cannot easily
   * extract the updated counter so we zero szLeft and leave
   * lastEncryptedCounter untouched.
   */
  if (szLeft != NULL) {
    *szLeft = 0;
  }
  (void)lastEncryptedCounter;

  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

void sss_mbedtls_symmetric_context_free(sss_mbedtls_symmetric_t *context) {
  if (context != NULL) {
    if (context->operation_initialized) {
      psa_cipher_abort(context->cipher_op);
      context->operation_initialized = false;
    }
    if (context->cipher_op != NULL) {
      SSS_FREE(context->cipher_op);
      context->cipher_op = NULL;
    }
    memset(context, 0, sizeof(*context));
  }
}

/* End: psa_symm */

/* ************************************************************************** */
/* Digest / Hash                                                              */
/* ************************************************************************** */

static psa_algorithm_t sss_to_psa_hash_algorithm(sss_algorithm_t algorithm) {
  switch (algorithm) {
  case kAlgorithm_SSS_SHA1:
    return PSA_ALG_SHA_1;
  case kAlgorithm_SSS_SHA224:
    return PSA_ALG_SHA_224;
  case kAlgorithm_SSS_SHA256:
    return PSA_ALG_SHA_256;
  case kAlgorithm_SSS_SHA384:
    return PSA_ALG_SHA_384;
  case kAlgorithm_SSS_SHA512:
    return PSA_ALG_SHA_512;
  default:
    return PSA_ALG_NONE;
  }
}

sss_status_t sss_mbedtls_digest_context_init(sss_mbedtls_digest_t *context,
                                             sss_mbedtls_session_t *session,
                                             sss_algorithm_t algorithm,
                                             sss_mode_t mode) {
  sss_status_t retval = kStatus_SSS_Fail;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(session != NULL);

  if (sss_to_psa_hash_algorithm(algorithm) == PSA_ALG_NONE) {
    LOG_E("Unsupported hash algorithm: %d", algorithm);
    goto cleanup;
  }

  memset(context, 0, sizeof(*context));

  /* Allocate hash operation */
  context->hash_op =
      (psa_hash_operation_t *)SSS_CALLOC(1, sizeof(psa_hash_operation_t));
  ENSURE_OR_GO_CLEANUP(context->hash_op != NULL);
  *context->hash_op = psa_hash_operation_init();

  context->session = session;
  context->algorithm = algorithm;
  context->mode = mode;
  context->operation_initialized = false;

  switch (algorithm) {
  case kAlgorithm_SSS_SHA1:
    context->digestFullLen = 20;
    break;
  case kAlgorithm_SSS_SHA224:
    context->digestFullLen = 28;
    break;
  case kAlgorithm_SSS_SHA256:
    context->digestFullLen = 32;
    break;
  case kAlgorithm_SSS_SHA384:
    context->digestFullLen = 48;
    break;
  case kAlgorithm_SSS_SHA512:
    context->digestFullLen = 64;
    break;
  default:
    context->digestFullLen = 0;
    break;
  }

  retval = kStatus_SSS_Success;

cleanup:
  if (retval != kStatus_SSS_Success && context != NULL &&
      context->hash_op != NULL) {
    SSS_FREE(context->hash_op);
    context->hash_op = NULL;
  }
  return retval;
}

sss_status_t sss_mbedtls_digest_one_go(sss_mbedtls_digest_t *context,
                                       const uint8_t *message,
                                       size_t messageLen, uint8_t *digest,
                                       size_t *digestLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_algorithm_t psa_alg;
  size_t hash_length = 0;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(message != NULL);
  ENSURE_OR_GO_CLEANUP(digest != NULL);
  ENSURE_OR_GO_CLEANUP(digestLen != NULL);

  psa_alg = sss_to_psa_hash_algorithm(context->algorithm);
  if (psa_alg == PSA_ALG_NONE) {
    LOG_E("Unsupported hash algorithm: %d", context->algorithm);
    goto cleanup;
  }

  psa_status = psa_hash_compute(psa_alg, message, messageLen, digest,
                                *digestLen, &hash_length);

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA hash compute failed: %d", psa_status);
    goto cleanup;
  }

  *digestLen = hash_length;
  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_digest_init(sss_mbedtls_digest_t *context) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_algorithm_t psa_alg;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(context->hash_op != NULL);

  if (context->operation_initialized) {
    psa_hash_abort(context->hash_op);
    *context->hash_op = psa_hash_operation_init();
    context->operation_initialized = false;
  }

  psa_alg = sss_to_psa_hash_algorithm(context->algorithm);
  if (psa_alg == PSA_ALG_NONE) {
    LOG_E("Unsupported hash algorithm: %d", context->algorithm);
    goto cleanup;
  }

  psa_status = psa_hash_setup(context->hash_op, psa_alg);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA hash setup failed: %d", psa_status);
    goto cleanup;
  }

  context->operation_initialized = true;
  retval = kStatus_SSS_Success;

cleanup:
  if (retval != kStatus_SSS_Success && context->operation_initialized) {
    psa_hash_abort(context->hash_op);
    *context->hash_op = psa_hash_operation_init();
    context->operation_initialized = false;
  }
  return retval;
}

sss_status_t sss_mbedtls_digest_update(sss_mbedtls_digest_t *context,
                                       const uint8_t *message,
                                       size_t messageLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(context->hash_op != NULL);
  ENSURE_OR_GO_CLEANUP(message != NULL);
  ENSURE_OR_GO_CLEANUP(context->operation_initialized);

  psa_status = psa_hash_update(context->hash_op, message, messageLen);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA hash update failed: %d", psa_status);
    goto cleanup;
  }

  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_digest_finish(sss_mbedtls_digest_t *context,
                                       uint8_t *digest, size_t *digestLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  size_t hash_length = 0;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(context->hash_op != NULL);
  ENSURE_OR_GO_CLEANUP(digest != NULL);
  ENSURE_OR_GO_CLEANUP(digestLen != NULL);
  ENSURE_OR_GO_CLEANUP(context->operation_initialized);

  psa_status =
      psa_hash_finish(context->hash_op, digest, *digestLen, &hash_length);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA hash finish failed: %d", psa_status);
    goto cleanup;
  }

  *digestLen = hash_length;
  context->operation_initialized = false;
  retval = kStatus_SSS_Success;

cleanup:
  if (context->operation_initialized) {
    psa_hash_abort(context->hash_op);
    *context->hash_op = psa_hash_operation_init();
    context->operation_initialized = false;
  }
  return retval;
}

void sss_mbedtls_digest_context_free(sss_mbedtls_digest_t *context) {
  if (context != NULL) {
    if (context->operation_initialized && context->hash_op != NULL) {
      psa_hash_abort(context->hash_op);
      context->operation_initialized = false;
    }
    if (context->hash_op != NULL) {
      SSS_FREE(context->hash_op);
      context->hash_op = NULL;
    }
    memset(context, 0, sizeof(*context));
  }
}

/* End: psa_md */

sss_status_t sss_mbedtls_mac_context_init(sss_mbedtls_mac_t *context,
                                          sss_mbedtls_session_t *session,
                                          sss_mbedtls_object_t *keyObject,
                                          sss_algorithm_t algorithm,
                                          sss_mode_t mode) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_algorithm_t psa_alg;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(session != NULL);
  ENSURE_OR_GO_CLEANUP(keyObject != NULL);

  memset(context, 0, sizeof(*context));

  context->mac_op =
      (psa_mac_operation_t *)SSS_CALLOC(1, sizeof(psa_mac_operation_t));
  ENSURE_OR_GO_CLEANUP(context->mac_op != NULL);
  *context->mac_op = psa_mac_operation_init();

  context->session = session;
  context->keyObject = keyObject;
  context->algorithm = algorithm;
  context->mode = mode;
  context->operation_initialized = false;

  /* Determine PSA algorithm for MAC operations */
  psa_alg = sss_to_psa_mac_algorithm(algorithm);
  if (psa_alg == PSA_ALG_NONE) {
    LOG_E("Unsupported MAC algorithm: %d", algorithm);
    goto cleanup;
  }

  /* Import the key with the correct MAC algorithm */
  retval = sss_mbedtls_import_key_with_algorithm(
      keyObject, psa_alg,
      PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
  ENSURE_OR_GO_CLEANUP(retval == kStatus_SSS_Success);

  LOG_D("MAC context initialized with PSA algorithm 0x%08X", psa_alg);

  retval = kStatus_SSS_Success;

cleanup:
  if (retval != kStatus_SSS_Success && context->mac_op != NULL) {
    SSS_FREE(context->mac_op);
    context->mac_op = NULL;
  }
  return retval;
}

sss_status_t sss_mbedtls_mac_one_go(sss_mbedtls_mac_t *context,
                                    const uint8_t *message, size_t messageLen,
                                    uint8_t *mac, size_t *macLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_algorithm_t psa_alg;
  size_t mac_length = 0;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(message != NULL);
  ENSURE_OR_GO_CLEANUP(mac != NULL);
  ENSURE_OR_GO_CLEANUP(macLen != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject->psa_key_id != PSA_KEY_ID_NULL);

  psa_alg = sss_to_psa_mac_algorithm(context->algorithm);
  if (psa_alg == PSA_ALG_NONE) {
    LOG_E("Unsupported MAC algorithm: %d", context->algorithm);
    goto cleanup;
  }

  if (context->mode == kMode_SSS_Mac) {
    psa_status =
        psa_mac_compute(context->keyObject->psa_key_id, psa_alg, message,
                        messageLen, mac, *macLen, &mac_length);

    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA MAC compute failed: %d", psa_status);
      goto cleanup;
    }

    *macLen = mac_length;

  } else if (context->mode == kMode_SSS_Mac_Validate) {
    psa_status = psa_mac_verify(context->keyObject->psa_key_id, psa_alg,
                                message, messageLen, mac, *macLen);

    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA MAC verify failed: %d", psa_status);
      goto cleanup;
    }
  } else {
    LOG_E("Invalid MAC mode: %d", context->mode);
    goto cleanup;
  }

  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_mac_init(sss_mbedtls_mac_t *context) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_algorithm_t psa_alg;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject->psa_key_id != PSA_KEY_ID_NULL);

  if (context->operation_initialized) {
    psa_mac_abort(context->mac_op);
    context->operation_initialized = false;
  }
  *context->mac_op = psa_mac_operation_init();

  psa_alg = sss_to_psa_mac_algorithm(context->algorithm);
  if (psa_alg == PSA_ALG_NONE) {
    LOG_E("Unsupported MAC algorithm: %d", context->algorithm);
    goto cleanup;
  }

  if (context->mode == kMode_SSS_Mac) {
    psa_status = psa_mac_sign_setup(context->mac_op,
                                    context->keyObject->psa_key_id, psa_alg);
  } else if (context->mode == kMode_SSS_Mac_Validate) {
    psa_status = psa_mac_verify_setup(context->mac_op,
                                      context->keyObject->psa_key_id, psa_alg);
  } else {
    LOG_E("Invalid MAC mode: %d", context->mode);
    goto cleanup;
  }

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA MAC setup failed: %d", psa_status);
    goto cleanup;
  }

  context->operation_initialized = true;
  retval = kStatus_SSS_Success;

cleanup:
  if (retval != kStatus_SSS_Success && context != NULL &&
      context->operation_initialized) {
    psa_mac_abort(context->mac_op);
    *context->mac_op = psa_mac_operation_init();
    context->operation_initialized = false;
  }
  return retval;
}

sss_status_t sss_mbedtls_mac_update(sss_mbedtls_mac_t *context,
                                    const uint8_t *message, size_t messageLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(message != NULL);
  ENSURE_OR_GO_CLEANUP(context->mac_op != NULL);
  ENSURE_OR_GO_CLEANUP(context->operation_initialized);
  LOG_AU8_D(message, messageLen);
  psa_status = psa_mac_update(context->mac_op, message, messageLen);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA MAC update failed: %d", psa_status);
    goto cleanup;
  }

  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_mac_finish(sss_mbedtls_mac_t *context, uint8_t *mac,
                                    size_t *macLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  size_t mac_length = 0;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(context->mac_op != NULL);
  ENSURE_OR_GO_CLEANUP(mac != NULL);
  ENSURE_OR_GO_CLEANUP(macLen != NULL);
  ENSURE_OR_GO_CLEANUP(context->operation_initialized);

  if (context->mode == kMode_SSS_Mac) {
    psa_status =
        psa_mac_sign_finish(context->mac_op, mac, *macLen, &mac_length);
    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA MAC sign finish failed: %d", psa_status);
      goto cleanup;
    }
    *macLen = mac_length;
  } else if (context->mode == kMode_SSS_Mac_Validate) {
    psa_status = psa_mac_verify_finish(context->mac_op, mac, *macLen);
    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA MAC verify finish failed: %d", psa_status);
      goto cleanup;
    }
  } else {
    LOG_E("Invalid MAC mode: %d", context->mode);
    goto cleanup;
  }

  context->operation_initialized = false;
  retval = kStatus_SSS_Success;

cleanup:
  if (context->operation_initialized) {
    psa_mac_abort(context->mac_op);
    *context->mac_op = psa_mac_operation_init();
    context->operation_initialized = false;
  }
  return retval;
}

void sss_mbedtls_mac_context_free(sss_mbedtls_mac_t *context) {
  if (context != NULL) {
    if (context->operation_initialized && context->mac_op != NULL) {
      psa_mac_abort(context->mac_op);
      context->operation_initialized = false;
    }
    if (context->mac_op != NULL) {
      SSS_FREE(context->mac_op);
      context->mac_op = NULL;
    }
    memset(context, 0, sizeof(*context));
  }
}

/* End: psa_mac */

/* ************************************************************************** */
/* Asymmetric                                                                 */
/* ************************************************************************** */

static psa_algorithm_t sss_to_psa_asym_algorithm(sss_algorithm_t algorithm) {
  switch (algorithm) {
  /* ECDSA */
  case kAlgorithm_SSS_ECDSA_SHA1:
    return PSA_ALG_ECDSA(PSA_ALG_SHA_1);
  case kAlgorithm_SSS_ECDSA_SHA224:
    return PSA_ALG_ECDSA(PSA_ALG_SHA_224);
  case kAlgorithm_SSS_ECDSA_SHA256:
    return PSA_ALG_ECDSA(PSA_ALG_SHA_256);
  case kAlgorithm_SSS_ECDSA_SHA384:
    return PSA_ALG_ECDSA(PSA_ALG_SHA_384);
  case kAlgorithm_SSS_ECDSA_SHA512:
    return PSA_ALG_ECDSA(PSA_ALG_SHA_512);

  /* RSA PKCS1v1.5 sign */
  case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA1:
    return PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_1);
  case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA224:
    return PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_224);
  case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA256:
    return PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256);
  case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA384:
    return PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_384);
  case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_SHA512:
    return PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_512);
  case kAlgorithm_SSS_RSASSA_PKCS1_V1_5_NO_HASH:
  case kAlgorithm_SSS_RSASSA_NO_PADDING:
    return PSA_ALG_RSA_PKCS1V15_SIGN_RAW;

  /* RSA PSS sign */
  case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA1:
    return PSA_ALG_RSA_PSS(PSA_ALG_SHA_1);
  case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA224:
    return PSA_ALG_RSA_PSS(PSA_ALG_SHA_224);
  case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA256:
    return PSA_ALG_RSA_PSS(PSA_ALG_SHA_256);
  case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA384:
    return PSA_ALG_RSA_PSS(PSA_ALG_SHA_384);
  case kAlgorithm_SSS_RSASSA_PKCS1_PSS_MGF1_SHA512:
    return PSA_ALG_RSA_PSS(PSA_ALG_SHA_512);

  /* RSA OAEP encrypt */
  case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA1:
    return PSA_ALG_RSA_OAEP(PSA_ALG_SHA_1);
  case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA224:
    return PSA_ALG_RSA_OAEP(PSA_ALG_SHA_224);
  case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA256:
    return PSA_ALG_RSA_OAEP(PSA_ALG_SHA_256);
  case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA384:
    return PSA_ALG_RSA_OAEP(PSA_ALG_SHA_384);
  case kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA512:
    return PSA_ALG_RSA_OAEP(PSA_ALG_SHA_512);

  /* RSA PKCS1v1.5 encrypt */
  case kAlgorithm_SSS_RSAES_PKCS1_V1_5:
    return PSA_ALG_RSA_PKCS1V15_CRYPT;

  /* Plain hash algorithms - assume ECDSA with that hash */
  case kAlgorithm_SSS_SHA1:
    return PSA_ALG_ECDSA(PSA_ALG_SHA_1);
  case kAlgorithm_SSS_SHA224:
    return PSA_ALG_ECDSA(PSA_ALG_SHA_224);
  case kAlgorithm_SSS_SHA256:
    return PSA_ALG_ECDSA(PSA_ALG_SHA_256);
  case kAlgorithm_SSS_SHA384:
    return PSA_ALG_ECDSA(PSA_ALG_SHA_384);
  case kAlgorithm_SSS_SHA512:
    return PSA_ALG_ECDSA(PSA_ALG_SHA_512);

  case kAlgorithm_SSS_ECDH:
    return PSA_ALG_ECDH;
    break;

  default:
    return PSA_ALG_NONE;
  }
}

sss_status_t sss_mbedtls_asymmetric_context_init(
    sss_mbedtls_asymmetric_t *context, sss_mbedtls_session_t *session,
    sss_mbedtls_object_t *keyObject, sss_algorithm_t algorithm,
    sss_mode_t mode) {
  sss_status_t retval = kStatus_SSS_Fail;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(session != NULL);
  ENSURE_OR_GO_CLEANUP(keyObject != NULL);

  memset(context, 0, sizeof(*context));

  context->session = session;
  context->keyObject = keyObject;
  context->algorithm = algorithm;
  context->mode = mode;

  /* Check if key already exists in PSA (e.g., generated key) */
  if (keyObject->psa_key_id != PSA_KEY_ID_NULL) {
    psa_status_t psa_status;
    psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;

    psa_status = psa_get_key_attributes(keyObject->psa_key_id, &attrs);
    if (psa_status == PSA_SUCCESS) {
      psa_reset_key_attributes(&attrs);
      LOG_D("Using existing key with PSA ID: %lu",
            (unsigned long)keyObject->psa_key_id);
      retval = kStatus_SSS_Success;
      goto cleanup;
    }
  }

  /* Try to import if we have raw key data */
  if (keyObject->contents != NULL && keyObject->contents_size > 0) {
    /* Determine PSA algorithm */
    psa_algorithm_t psa_alg = sss_to_psa_asym_algorithm(algorithm);
    if (psa_alg == PSA_ALG_NONE) {
      /* Use wildcard algorithm based on key type */
      if (keyObject->cipherType == kSE_SSS_CipherType_EC_NIST_P ||
          keyObject->cipherType == kSE_SSS_CipherType_EC_NIST_K) {
        psa_alg = PSA_ALG_ECDSA(PSA_ALG_ANY_HASH);
      } else if (keyObject->cipherType == kSE_SSS_CipherType_RSA ||
                 keyObject->cipherType == kSE_SSS_CipherType_RSA_CRT) {
        psa_alg = PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_ANY_HASH);
      }
    }

    /* Determine usage flags based on mode */
    psa_key_usage_t usage = 0;
    if (mode == kMode_SSS_Sign) {
      usage = PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_SIGN_MESSAGE;
    } else if (mode == kMode_SSS_Verify) {
      usage = PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_VERIFY_MESSAGE;
    } else if (mode == kMode_SSS_Encrypt) {
      usage = PSA_KEY_USAGE_ENCRYPT;
    } else if (mode == kMode_SSS_Decrypt) {
      usage = PSA_KEY_USAGE_DECRYPT;
    }

    retval = sss_mbedtls_import_key_with_algorithm(keyObject, psa_alg, usage);
    ENSURE_OR_GO_CLEANUP(retval == kStatus_SSS_Success);
  } else if (keyObject->psa_key_id != PSA_KEY_ID_NULL) {
    /* Key exists in PSA but no raw data - this is OK for generated keys */
    LOG_D("Using existing PSA key (generated key, no raw data)");
    retval = kStatus_SSS_Success;
  } else {
    /* No key data and no PSA key */
    LOG_E("No key available for asymmetric operation");
    goto cleanup;
  }

  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_asymmetric_encrypt(sss_mbedtls_asymmetric_t *context,
                                            const uint8_t *srcData,
                                            size_t srcLen, uint8_t *destData,
                                            size_t *destLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_algorithm_t psa_alg;
  size_t output_length = 0;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(srcData != NULL);
  ENSURE_OR_GO_CLEANUP(destData != NULL);
  ENSURE_OR_GO_CLEANUP(destLen != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject->psa_key_id != PSA_KEY_ID_NULL);
  ENSURE_OR_GO_CLEANUP(
      (context->keyObject->accessRights & kAccessPermission_SE_SSS_Use));

  psa_alg = sss_to_psa_asym_algorithm(context->algorithm);
  if (psa_alg == PSA_ALG_NONE) {
    LOG_E("Unsupported asymmetric algorithm: %d", context->algorithm);
    goto cleanup;
  }

  psa_status = psa_asymmetric_encrypt(context->keyObject->psa_key_id, psa_alg,
                                      srcData, srcLen, NULL, 0, destData,
                                      *destLen, &output_length);

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA asymmetric encrypt failed: %d", psa_status);
    goto cleanup;
  }

  *destLen = output_length;
  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_asymmetric_decrypt(sss_mbedtls_asymmetric_t *context,
                                            const uint8_t *srcData,
                                            size_t srcLen, uint8_t *destData,
                                            size_t *destLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_algorithm_t psa_alg;
  size_t output_length = 0;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(srcData != NULL);
  ENSURE_OR_GO_CLEANUP(destData != NULL);
  ENSURE_OR_GO_CLEANUP(destLen != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject->psa_key_id != PSA_KEY_ID_NULL);
  ENSURE_OR_GO_CLEANUP(
      (context->keyObject->accessRights & kAccessPermission_SE_SSS_Use));

  psa_alg = sss_to_psa_asym_algorithm(context->algorithm);
  if (psa_alg == PSA_ALG_NONE) {
    LOG_E("Unsupported asymmetric algorithm: %d", context->algorithm);
    goto cleanup;
  }

  psa_status = psa_asymmetric_decrypt(context->keyObject->psa_key_id, psa_alg,
                                      srcData, srcLen, NULL, 0, destData,
                                      *destLen, &output_length);

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA asymmetric decrypt failed: %d", psa_status);
    goto cleanup;
  }

  *destLen = output_length;
  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t
sss_mbedtls_asymmetric_sign_digest(sss_mbedtls_asymmetric_t *context,
                                   const uint8_t *digest, size_t digestLen,
                                   uint8_t *signature, size_t *signatureLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_algorithm_t psa_alg;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(digest != NULL);
  ENSURE_OR_GO_CLEANUP(signature != NULL);
  ENSURE_OR_GO_CLEANUP(signatureLen != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject->psa_key_id != PSA_KEY_ID_NULL);
  ENSURE_OR_GO_CLEANUP(
      (context->keyObject->accessRights & kAccessPermission_SE_SSS_Use));

  psa_alg = sss_to_psa_asym_algorithm(context->algorithm);
  if (psa_alg == PSA_ALG_NONE) {
    LOG_E("Unsupported asymmetric algorithm: %d", context->algorithm);
    goto cleanup;
  }

  /* Check if we need DER conversion (ECDSA only) */
  const bool is_ecdsa =
      PSA_ALG_IS_ECDSA(psa_alg) || psa_alg == PSA_ALG_ECDSA_ANY;

  if (is_ecdsa) {
    /* ECDSA: PSA produces raw (r||s), we need DER */
    uint8_t raw_sig[64]; /* Max for P-256 */
    size_t raw_sig_len = sizeof(raw_sig);

    psa_status = psa_sign_hash(context->keyObject->psa_key_id, psa_alg, digest,
                               digestLen, raw_sig, raw_sig_len, &raw_sig_len);

    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA sign hash failed: %d", psa_status);
      goto cleanup;
    }

    /* Convert raw to DER */
    int ret = EcRandSToSignature(raw_sig, raw_sig_len, signature, signatureLen);
    if (ret != 0) {
      LOG_E("EcRandSToSignature failed");
      goto cleanup;
    }

    /* Clear sensitive data */
    memset(raw_sig, 0, sizeof(raw_sig));

  } else {
    /* Non-ECDSA: use signature as-is */
    psa_status =
        psa_sign_hash(context->keyObject->psa_key_id, psa_alg, digest,
                      digestLen, signature, *signatureLen, signatureLen);

    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA sign hash failed: %d", psa_status);
      goto cleanup;
    }
  }

  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_asymmetric_verify_digest(
    sss_mbedtls_asymmetric_t *context, const uint8_t *digest, size_t digestLen,
    const uint8_t *signature, size_t signatureLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_algorithm_t psa_alg;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(digest != NULL);
  ENSURE_OR_GO_CLEANUP(signature != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject->psa_key_id != PSA_KEY_ID_NULL);
  ENSURE_OR_GO_CLEANUP(
      (context->keyObject->accessRights & kAccessPermission_SE_SSS_Use));

  psa_alg = sss_to_psa_asym_algorithm(context->algorithm);
  if (psa_alg == PSA_ALG_NONE) {
    LOG_E("Unsupported asymmetric algorithm: %d", context->algorithm);
    goto cleanup;
  }

  /* Check if we need DER conversion (ECDSA only) */
  const bool is_ecdsa =
      PSA_ALG_IS_ECDSA(psa_alg) &&
      context->keyObject->cipherType == kSE_SSS_CipherType_EC_NIST_P;

  if (is_ecdsa) {

    int ret = EcSignatureToRandS((uint8_t *)signature, &signatureLen);

    if (ret != 0) {
      LOG_E("Failed to convert signature from DER format");
      goto cleanup;
    }

    psa_status = psa_verify_hash(context->keyObject->psa_key_id, psa_alg,
                                 digest, digestLen, signature, signatureLen);

  } else {
    /* Non-ECDSA: use signature as-is */
    psa_status = psa_verify_hash(context->keyObject->psa_key_id, psa_alg,
                                 digest, digestLen, signature, signatureLen);
  }

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA verify hash failed: %d", psa_status);
    goto cleanup;
  }

  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

void sss_mbedtls_asymmetric_context_free(sss_mbedtls_asymmetric_t *context) {
  if (context != NULL) {
    memset(context, 0, sizeof(*context));
  }
}

/* End: psa_asym */

/* ************************************************************************** */
/* Key Derivation                                                             */
/* ************************************************************************** */

static psa_algorithm_t sss_to_psa_derive_algorithm(sss_algorithm_t algorithm,
                                                   sss_mode_t mode) {
  AX_UNUSED_ARG(algorithm);

  /* HKDF */
  if (mode == kMode_SSS_HKDF_ExpandOnly) {
    return PSA_ALG_HKDF_EXPAND(PSA_ALG_SHA_256);
  }
  if (mode == kMode_SSS_HKDF_ExtractExpand) {
    return PSA_ALG_HKDF(PSA_ALG_SHA_256);
  }

  /* ECDH falls through to the DH function which uses psa_raw_key_agreement */
  return PSA_ALG_NONE;
}

sss_status_t sss_mbedtls_derive_key_context_init(
    sss_mbedtls_derive_key_t *context, sss_mbedtls_session_t *session,
    sss_mbedtls_object_t *keyObject, sss_algorithm_t algorithm,
    sss_mode_t mode) {
  sss_status_t retval = kStatus_SSS_Fail;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(session != NULL);
  ENSURE_OR_GO_CLEANUP(keyObject != NULL);

  memset(context, 0, sizeof(*context));

  context->session = session;
  context->keyObject = keyObject;
  context->algorithm = algorithm;
  context->mode = mode;
  context->operation_initialized = false;

  /* Check if key already exists in PSA (e.g., generated key) */
  if (keyObject->psa_key_id != PSA_KEY_ID_NULL) {
    psa_status_t psa_status;
    psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;

    psa_status = psa_get_key_attributes(keyObject->psa_key_id, &attrs);
    if (psa_status == PSA_SUCCESS) {
      // psa_algorithm_t existing_alg = psa_get_key_algorithm(&attrs);
      psa_key_usage_t existing_usage = psa_get_key_usage_flags(&attrs);
      psa_reset_key_attributes(&attrs);

      /* Check if key has DERIVE usage */
      if (existing_usage & PSA_KEY_USAGE_DERIVE) {
        LOG_D("Using existing generated key with PSA ID: %lu",
              (unsigned long)keyObject->psa_key_id);
        retval = kStatus_SSS_Success;
        goto cleanup;
      } else {
        LOG_W("Key exists but doesn't have DERIVE usage");
      }
    }
  }

  /* Try to import if we have raw key data */
  if (keyObject->contents != NULL && keyObject->contents_size > 0) {
    psa_algorithm_t psa_alg = PSA_ALG_ECDH;
    psa_key_usage_t usage = PSA_KEY_USAGE_DERIVE;

    retval = sss_mbedtls_import_key_with_algorithm(keyObject, psa_alg, usage);
    ENSURE_OR_GO_CLEANUP(retval == kStatus_SSS_Success);
  } else if (keyObject->psa_key_id != PSA_KEY_ID_NULL) {
    /* Key exists in PSA but no raw data - this is OK for generated keys */
    LOG_D("Using existing PSA key (generated key, no raw data)");
    retval = kStatus_SSS_Success;
  } else {
    /* No key data and no PSA key */
    LOG_E("No key available for derivation");
    goto cleanup;
  }

  retval = kStatus_SSS_Success;

cleanup:
  return retval;
}

sss_status_t sss_mbedtls_derive_key_go(
    sss_mbedtls_derive_key_t *context, const uint8_t *saltData, size_t saltLen,
    const uint8_t *info, size_t infoLen, sss_mbedtls_object_t *derivedKeyObject,
    uint16_t deriveDataLen, uint8_t *hkdfOutput, size_t *hkdfOutputLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_algorithm_t psa_alg;
  psa_key_derivation_operation_t operation = PSA_KEY_DERIVATION_OPERATION_INIT;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject->psa_key_id != PSA_KEY_ID_NULL);

  psa_alg = sss_to_psa_derive_algorithm(context->algorithm, context->mode);
  if (psa_alg == PSA_ALG_NONE) {
    LOG_E("Unsupported key derivation algorithm/mode: %d/%d",
          context->algorithm, context->mode);
    goto cleanup;
  }

  psa_status = psa_key_derivation_setup(&operation, psa_alg);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA key derivation setup failed: %d", psa_status);
    goto cleanup;
  }

  psa_status = psa_key_derivation_set_capacity(&operation, deriveDataLen);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA key derivation set capacity failed: %d", psa_status);
    goto cleanup;
  }

  /* HKDF full (extract + expand) requires a salt */
  if (context->mode == kMode_SSS_HKDF_ExtractExpand) {
    if (saltData != NULL && saltLen > 0) {
      psa_status = psa_key_derivation_input_bytes(
          &operation, PSA_KEY_DERIVATION_INPUT_SALT, saltData, saltLen);
      if (psa_status != PSA_SUCCESS) {
        LOG_E("PSA key derivation input salt failed: %d", psa_status);
        goto cleanup;
      }
    }
  }

  /* Input the PRK / secret key */
  psa_status =
      psa_key_derivation_input_key(&operation, PSA_KEY_DERIVATION_INPUT_SECRET,
                                   context->keyObject->psa_key_id);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA key derivation input key failed: %d", psa_status);
    goto cleanup;
  }

  /* Input info */
  if (info != NULL && infoLen > 0) {
    psa_status = psa_key_derivation_input_bytes(
        &operation, PSA_KEY_DERIVATION_INPUT_INFO, info, infoLen);
    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA key derivation input info failed: %d", psa_status);
      goto cleanup;
    }
  }

  if (derivedKeyObject != NULL) {
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t derived_key_id = PSA_KEY_ID_NULL;

    psa_key_type_t key_type =
        sss_to_psa_key_type((se_sss_cipher_type_t)derivedKeyObject->cipherType,
                            (sss_key_part_t)derivedKeyObject->objectType,
                            (size_t)deriveDataLen * 8u);

    if (key_type == PSA_KEY_TYPE_NONE) {
      /* Fall back to raw data for unknown/unset cipher type */
      key_type = PSA_KEY_TYPE_RAW_DATA;
    }

    psa_set_key_type(&attributes, key_type);
    psa_set_key_bits(&attributes, (size_t)deriveDataLen * 8u);
    psa_set_key_usage_flags(&attributes,
                            PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT |
                                PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
    psa_set_key_algorithm(&attributes, PSA_ALG_NONE);

    if (derivedKeyObject->keyMode == kKeyObject_Mode_Persistent) {
      psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_PERSISTENT);
      psa_set_key_id(&attributes,
                     sss_app_key_id_to_psa(derivedKeyObject->keyId));
    }

    psa_status =
        psa_key_derivation_output_key(&attributes, &operation, &derived_key_id);
    psa_reset_key_attributes(&attributes);

    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA key derivation output key failed: %d", psa_status);
      goto cleanup;
    }

    derivedKeyObject->psa_key_id = derived_key_id;

  } else if (hkdfOutput != NULL && hkdfOutputLen != NULL) {
    psa_status =
        psa_key_derivation_output_bytes(&operation, hkdfOutput, deriveDataLen);
    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA key derivation output bytes failed: %d", psa_status);
      goto cleanup;
    }
    *hkdfOutputLen = deriveDataLen;
  }

  retval = kStatus_SSS_Success;

cleanup:
  psa_key_derivation_abort(&operation);
  return retval;
}

sss_status_t sss_mbedtls_derive_key_one_go(
    sss_mbedtls_derive_key_t *context, const uint8_t *saltData, size_t saltLen,
    const uint8_t *info, size_t infoLen, sss_mbedtls_object_t *derivedKeyObject,
    uint16_t deriveDataLen) {
  size_t adjustedSaltLen = saltLen;

  if (context->mode == kMode_SSS_HKDF_ExpandOnly) {
    adjustedSaltLen = 0;
  }

  return sss_mbedtls_derive_key_go(context, saltData, adjustedSaltLen, info,
                                   infoLen, derivedKeyObject, deriveDataLen,
                                   NULL, NULL);
}

sss_status_t sss_mbedtls_derive_key_sobj_one_go(
    sss_mbedtls_derive_key_t *context, sss_mbedtls_object_t *saltKeyObject,
    const uint8_t *info, size_t infoLen, sss_mbedtls_object_t *derivedKeyObject,
    uint16_t deriveDataLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_algorithm_t psa_alg;
  psa_key_derivation_operation_t operation = PSA_KEY_DERIVATION_OPERATION_INIT;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(derivedKeyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject->psa_key_id != PSA_KEY_ID_NULL);

  psa_alg = sss_to_psa_derive_algorithm(context->algorithm, context->mode);
  if (psa_alg == PSA_ALG_NONE) {
    LOG_E("Unsupported key derivation algorithm: %d", context->algorithm);
    goto cleanup;
  }

  psa_status = psa_key_derivation_setup(&operation, psa_alg);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA key derivation setup failed: %d", psa_status);
    goto cleanup;
  }

  psa_status = psa_key_derivation_set_capacity(&operation, deriveDataLen);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA key derivation set capacity failed: %d", psa_status);
    goto cleanup;
  }

  /* Salt key (only for full HKDF) */
  if (context->mode == kMode_SSS_HKDF_ExtractExpand && saltKeyObject != NULL &&
      saltKeyObject->psa_key_id != PSA_KEY_ID_NULL) {

    psa_status = psa_key_derivation_input_key(
        &operation, PSA_KEY_DERIVATION_INPUT_SALT, saltKeyObject->psa_key_id);
    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA key derivation input salt key failed: %d", psa_status);
      goto cleanup;
    }
  }

  psa_status =
      psa_key_derivation_input_key(&operation, PSA_KEY_DERIVATION_INPUT_SECRET,
                                   context->keyObject->psa_key_id);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA key derivation input secret key failed: %d", psa_status);
    goto cleanup;
  }

  if (info != NULL && infoLen > 0) {
    psa_status = psa_key_derivation_input_bytes(
        &operation, PSA_KEY_DERIVATION_INPUT_INFO, info, infoLen);
    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA key derivation input info failed: %d", psa_status);
      goto cleanup;
    }
  }

  {
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t derived_key_id = PSA_KEY_ID_NULL;

    psa_key_type_t key_type =
        sss_to_psa_key_type((se_sss_cipher_type_t)derivedKeyObject->cipherType,
                            (sss_key_part_t)derivedKeyObject->objectType,
                            (size_t)deriveDataLen * 8u);

    if (key_type == PSA_KEY_TYPE_NONE) {
      key_type = PSA_KEY_TYPE_RAW_DATA;
    }

    psa_set_key_type(&attributes, key_type);
    psa_set_key_bits(&attributes, (size_t)deriveDataLen * 8u);
    psa_set_key_usage_flags(&attributes,
                            PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT |
                                PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
    psa_set_key_algorithm(&attributes, PSA_ALG_NONE);

    if (derivedKeyObject->keyMode == kKeyObject_Mode_Persistent) {
      psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_PERSISTENT);
      psa_set_key_id(&attributes,
                     sss_app_key_id_to_psa(derivedKeyObject->keyId));
    }

    psa_status =
        psa_key_derivation_output_key(&attributes, &operation, &derived_key_id);
    psa_reset_key_attributes(&attributes);

    if (psa_status != PSA_SUCCESS) {
      LOG_E("PSA key derivation output key failed: %d", psa_status);
      goto cleanup;
    }

    derivedKeyObject->psa_key_id = derived_key_id;
  }

  retval = kStatus_SSS_Success;

cleanup:
  psa_key_derivation_abort(&operation);
  return retval;
}

sss_status_t
sss_mbedtls_derive_key_dh(sss_mbedtls_derive_key_t *context,
                          sss_mbedtls_object_t *otherPartyKeyObject,
                          sss_mbedtls_object_t *derivedKeyObject) {
  sss_status_t retval = kStatus_SSS_Fail;
  psa_status_t psa_status;
  psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
  psa_key_id_t derived_key_id = PSA_KEY_ID_NULL;
  uint8_t shared_secret[256];
  size_t shared_secret_len = 0;
  uint8_t peer_pub_key[256];
  size_t peer_pub_key_len = 0;

  ENSURE_OR_GO_CLEANUP(context != NULL);
  ENSURE_OR_GO_CLEANUP(otherPartyKeyObject != NULL);
  ENSURE_OR_GO_CLEANUP(derivedKeyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject != NULL);
  ENSURE_OR_GO_CLEANUP(context->keyObject->psa_key_id != PSA_KEY_ID_NULL);

  /* Import other party's public key if not already imported */
  psa_algorithm_t psa_alg = PSA_ALG_ECDH;
  psa_key_usage_t usage = PSA_KEY_USAGE_DERIVE;

  retval = sss_mbedtls_import_key_with_algorithm(otherPartyKeyObject, psa_alg,
                                                 usage);
  ENSURE_OR_GO_CLEANUP(retval == kStatus_SSS_Success);

  /* Export peer's public key */
  psa_status =
      psa_export_public_key(otherPartyKeyObject->psa_key_id, peer_pub_key,
                            sizeof(peer_pub_key), &peer_pub_key_len);
  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA export peer public key failed: %d", psa_status);
    goto cleanup;
  }

  /* Perform ECDH */
  psa_status =
      psa_raw_key_agreement(PSA_ALG_ECDH, context->keyObject->psa_key_id,
                            peer_pub_key, peer_pub_key_len, shared_secret,
                            sizeof(shared_secret), &shared_secret_len);

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA ECDH key agreement failed: %d", psa_status);
    goto cleanup;
  }

  /* Import shared secret as derived key */
  psa_key_type_t key_type = sss_to_psa_key_type(
      (se_sss_cipher_type_t)derivedKeyObject->cipherType,
      (sss_key_part_t)derivedKeyObject->objectType, shared_secret_len * 8u);

  if (key_type == PSA_KEY_TYPE_NONE) {
    key_type = PSA_KEY_TYPE_RAW_DATA;
  }

  psa_set_key_type(&attrs, key_type);
  psa_set_key_bits(&attrs, shared_secret_len * 8u);
  psa_set_key_usage_flags(&attrs, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_ENCRYPT |
                                      PSA_KEY_USAGE_DECRYPT |
                                      PSA_KEY_USAGE_EXPORT);
  psa_set_key_algorithm(&attrs, PSA_ALG_NONE);

  if (derivedKeyObject->keyMode == kKeyObject_Mode_Persistent) {
    psa_set_key_lifetime(&attrs, PSA_KEY_LIFETIME_PERSISTENT);
    psa_set_key_id(&attrs, sss_app_key_id_to_psa(derivedKeyObject->keyId));
  }

  psa_status =
      psa_import_key(&attrs, shared_secret, shared_secret_len, &derived_key_id);
  psa_reset_key_attributes(&attrs);

  if (psa_status != PSA_SUCCESS) {
    LOG_E("PSA import derived key failed: %d", psa_status);
    goto cleanup;
  }

  derivedKeyObject->psa_key_id = derived_key_id;
  retval = kStatus_SSS_Success;

cleanup:
  /* Always scrub sensitive data */
  memset(shared_secret, 0, sizeof(shared_secret));
  memset(peer_pub_key, 0, sizeof(peer_pub_key));
  return retval;
}

void sss_mbedtls_derive_key_context_free(sss_mbedtls_derive_key_t *context) {
  if (context != NULL) {
    context->operation_initialized = false;
    memset(context, 0, sizeof(*context));
  }
}

/* End: psa_keyderive */

/* ************************************************************************** */
/* AEAD                                                                       */
/* ************************************************************************** */

sss_status_t sss_mbedtls_aead_context_init(sss_mbedtls_aead_t *context,
                                           sss_mbedtls_session_t *session,
                                           sss_mbedtls_object_t *keyObject,
                                           sss_algorithm_t algorithm,
                                           sss_mode_t mode) {
  sss_status_t retval = kStatus_SSS_Fail;
  AX_UNUSED_ARG(context);
  AX_UNUSED_ARG(session);
  AX_UNUSED_ARG(keyObject);
  AX_UNUSED_ARG(algorithm);
  AX_UNUSED_ARG(mode);
  /* TBU */
  return retval;
}

sss_status_t sss_mbedtls_aead_one_go(sss_mbedtls_aead_t *context,
                                     const uint8_t *srcData, uint8_t *destData,
                                     size_t size, uint8_t *nonce,
                                     size_t nonceLen, const uint8_t *aad,
                                     size_t aadLen, uint8_t *tag,
                                     size_t *tagLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  AX_UNUSED_ARG(context);
  AX_UNUSED_ARG(srcData);
  AX_UNUSED_ARG(destData);
  AX_UNUSED_ARG(size);
  AX_UNUSED_ARG(nonce);
  AX_UNUSED_ARG(nonceLen);
  AX_UNUSED_ARG(aad);
  AX_UNUSED_ARG(aadLen);
  AX_UNUSED_ARG(tag);
  AX_UNUSED_ARG(tagLen);
  /* TBU */
  return retval;
}

sss_status_t sss_mbedtls_aead_init(sss_mbedtls_aead_t *context, uint8_t *nonce,
                                   size_t nonceLen, size_t tagLen,
                                   size_t aadLen, size_t payloadLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  AX_UNUSED_ARG(context);
  AX_UNUSED_ARG(nonce);
  AX_UNUSED_ARG(nonceLen);
  AX_UNUSED_ARG(tagLen);
  AX_UNUSED_ARG(aadLen);
  AX_UNUSED_ARG(payloadLen);
  /* TBU */
  return retval;
}

sss_status_t sss_mbedtls_aead_update_aad(sss_mbedtls_aead_t *context,
                                         const uint8_t *aadData,
                                         size_t aadDataLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  AX_UNUSED_ARG(context);
  AX_UNUSED_ARG(aadData);
  AX_UNUSED_ARG(aadDataLen);
  /* TBU */
  return retval;
}

sss_status_t sss_mbedtls_aead_update(sss_mbedtls_aead_t *context,
                                     const uint8_t *srcData, size_t srcLen,
                                     uint8_t *destData, size_t *destLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  AX_UNUSED_ARG(context);
  AX_UNUSED_ARG(srcData);
  AX_UNUSED_ARG(srcLen);
  AX_UNUSED_ARG(destData);
  AX_UNUSED_ARG(destLen);
  /* TBU */
  return retval;
}

sss_status_t sss_mbedtls_aead_finish(sss_mbedtls_aead_t *context,
                                     const uint8_t *srcData, size_t srcLen,
                                     uint8_t *destData, size_t *destLen,
                                     uint8_t *tag, size_t *tagLen) {
  sss_status_t retval = kStatus_SSS_Fail;
  AX_UNUSED_ARG(context);
  AX_UNUSED_ARG(srcData);
  AX_UNUSED_ARG(srcLen);
  AX_UNUSED_ARG(destData);
  AX_UNUSED_ARG(destLen);
  AX_UNUSED_ARG(tag);
  AX_UNUSED_ARG(tagLen);
  /* TBU */
  return retval;
}

void sss_mbedtls_aead_context_free(sss_mbedtls_aead_t *context) {
  AX_UNUSED_ARG(context);
  /* TBU */
}

#endif /* SSS_USE_MBEDTLS_PSA_APIS */
