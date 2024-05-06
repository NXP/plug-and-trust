# Plug-And-Trust Mini Package Change Log


## Release v04.05.03

- IMPORTANT: Package License changed to BSD-3 from Apache-2. Please refer and accept the LICENSE.txt at the root folder before using the software.

- Fixes for static analysis findings


## Release v04.05.01

- SE052 secure element support added.

- Deep power down (T1oI2C command) API added for SE052 - :func:`phNxpEse_deepPwrDown`.

- I2C Wrapper for Linux/RT1060/RT1170 : I2C read back-of-delay extended to additional platforms including Linux.
This leads to consistent timeout of appr 20 seconds across all platforms when there is no response from secure element.

- T=1oI2C : During session open, added logic in T=1oI2C to wait till previous transaction is complete (by sending WTX response to up to 40 WTX requests).
This helps in case the previous transaction was not completed or middleware was re-started.

- I2C Wrapper for RT1060/RT1170 : Add workaround to send I2C Master stop if NAK received in I2C driver (:file:`simw-top/hostlib/hostLib/platform/ksdk/i2c_imxrt.c`)

- NVM write warning log messages are added in SE05x APDU APIs. (in release v04.05.00).

## Release v04.05.00

- Appler version `06_00` removed from `PTMW_SE05X_Ver`

- Private key (ECC and RSA) create disabled at SE05x APDU layer

- BN Curve and ECDAA APIs removed.

- Thread safety when using SCP03 in SSS layer. (Disabled by default. To enable, uncomment `//#define SSS_USE_SCP03_THREAD_SAFETY` in fsl_sss_se05x_apis.c)

- Issue of polices passed to the SSS API being modified - is Fixed.

- Fixes for compilation warnings and static analysis findings.

- Input pointers are updated with const qualifiers for asymmetric_sign APIs
(:cpp:type:`sss_se05x_asymmetric_sign_digest`, :cpp:type:`sss_se05x_asymmetric_sign`, :cpp:type:`sss_se05x_asymmetric_verify_digest`,
:cpp:type:`sss_se05x_asymmetric_verify`, :cpp:type:`sss_asymmetric_sign_digest`, :cpp:type:`sss_asymmetric_verify_digest`)

- New APDUs for RSA sign and verify with salt length added - :cpp:func:`Se05x_API_RSASign_WithSalt` and :cpp:func:`Se05x_API_RSAVerify_WithSalt`.

- Bug fix : failing phNxpEse_chipReset does not return failure

- phNxpEse_open memory leak fix

- APDU throughput error code (SM_ERR_APDU_THROUGHPUT) added in smStatus_t.

- APDU throughput error code (kStatus_SSS_ApduThroughputError) added in sss_status_t. SSS APIs are updated return new throughput error code.


## Release v04.04.00

- Fixes for compilation warnings and static analysis findings.

- Feature configurations via cmake​. Refer readme for more details.

- Memory leak fixes in SSS OpenSSL APIs.

- T1OI2C read retry ON I2C_FAILED error. Also retry count is increased.

- added optional workaround for SE050A/B/C/F for I2C communication. Enabled via define T1OI2C_SEND_SHORT_APDU.

- Mbedtls ALT files and Mbedtls client server example removed.


## Release v04.03.01

**Important Security Updates**

- Added security fixes on 24 Feb 2023 to prevent buffer overflow on the T=IoI2C stack. It is important to use the updated "hostlib\hostLib\libCommon\smCom\T1oI2C\” and smCom module.

## Release v04.03.00

- OpenSSL 3.0 support in SSS OpenSSL layer. Added OpenSSL version selection (1.1 and 3.0) in feature file.

- API :cpp:func:`Se05x_API_ECDAASign`  marked as deprecated. This will be removed in next release.

- Cipher type :cpp:enumerator:`kSSS_CipherType_EC_BARRETO_NAEHRIG` marked as deprecated. This will be removed in next release.

- enum ``SE05x_ECDAASignatureAlgo_t`` marked as deprecated. This will be removed in next release.

- Private key generation marked as deprecated for :cpp:func:`Se05x_API_WriteECKey` and :cpp:func:`Se05x_API_WriteRSAKey`. This will be removed in next release.

- T4T APDUs added in hostlib

- :cpp:func:`sss_se05x_asymmetric_sign_digest` implementation changed to use digest from host crypto for RSA sign / verify. Please enable one of host crypto (`SSS_HAVE_HOSTCRYPTO_OPENSSL` or `SSS_HAVE_HOSTCRYPTO_MBEDTLS` in fsl_sss_ftr.h) and required build changes to use the RSA sign / verify feature.

- Added a check to prevent a potential buffer overflow issue in T=1OI2C stack


## Release v04.02.00

- SE050E applet support added. Default applet in feature file 'fsl_sss_ftr.h' changed to SE050E.

- Extended :cpp:enumerator:`smStatus_t` with new error codes.

- Updated behaviour of :cpp:func:`sss_se05x_key_object_get_handle` to return
  a success and print warning if it is unable to read attributes but the object exists so
  that other operations (like deleting) can proceed if they don't depend
  on object attributes.

- Updated OEF specific SCP keys handling. Added flags to enable OEF specific SCP03 keys in fsl_sss_ftr.h file.

- SE051-H applet support added (Provides PAKE support).

- Bug fix : Memory leak fix on open session with wrong keys.


## Release v04.01.01

- Policy changes for 7.x applet  (Also refer - :ref:`sss_policies`)
    - Below policies removed from :cpp:type:`sss_policy_sym_key_u` for applet version 7.x.
        - Allow key derivation policy (``can_KD``)
        - Allow to write the object policy (``can_Write``)
        - Allow to (re)generate policy (``can_Gen``)
    - Below policies are added for :cpp:type:`sss_policy_sym_key_u` for applet version 7.x.
        - Allow TLS PRF key derivation (``can_TLS_KDF``)
        - Allow TLS PMS key derivation (``can_TLS_PMS_KD``)
        - Allow HKDF (``can_HKDF``)
        - Allow PBKDF (``can_PBKDF``)
        - Allow Desfire key derivation (``can_Desfire_KD``)
        - Forbid External iv (``forbid_external_iv``)
        - Allow usage as hmac pepper (``can_usage_hmac_pepper``)
    - Below policies removed from :cpp:type:`sss_policy_asym_key_u` for applet version 7.x.
        - Allow to read the object policy (``can_Read``)
        - Allow to write the object policy (``can_Write``)
        - Allow key derivation policy (``can_KD``)
        - Allow key wrapping policy (``can_Wrap``)
    - Below policies are added for :cpp:type:`sss_policy_common_u` for applet version 7.x.
        - Allow to read the object policy (``can_Read``)
        - Allow to write the object policy (``can_Write``)
    - Added new policy - ``ALLOW_DESFIRE_CHANGEKEY``, :cpp:type:`sss_policy_desfire_changekey_authId_value_u`
    - Added new policy - ``ALLOW_DERIVED_INPUT``, :cpp:type:`sss_policy_key_drv_master_keyid_value_u`
    - **can_Read** and **can_Write** polices are moved from symmetric and asymmetric object policy to common policy in applet 7.x. **PLEASE UPDATE THE APPLICATIONS ACCORDINGLY**.

- New attestation scheme for applet 7.x
    - Updated API :cpp:func:`Se05x_API_TriggerSelfTest_W_Attst` for applet version 7.x.
    - Updated API :cpp:func:`Se05x_i2c_master_attst_txn` for applet version 7.x.
    - Updated API :cpp:func:`sss_se05x_key_store_get_key_attst` for applet version 7.x.

- New API added for PBKDF2 support: :cpp:func:`Se05x_API_PBKDF2_extended`. Supports optional salt
  object id and optional derived object id.

- New mode :cpp:enumerator:`kMode_SSS_Mac_Validate` added to support MAC validation feature in
  :cpp:func:`sss_mac_one_go` and ``sss_mac_*`` multistep APIs.

- New API added for ECDH calulation with option to select ECDH algorithm:
  :cpp:func:`Se05x_API_ECDHGenerateSharedSecret_InObject_extended`. ECDH algorithms
  supported - ``EC_SVDP_DH`` and ``EC_SVDP_DH_PLAIN``.

- New API added :cpp:func:`sss_cipher_one_go_v2` with different parameters for source
  and destination lengths to support ISO/IEC 9797-M2 padding.

- Internal IV generation supported added for AES CTR, AES CCM, AES GCM modes:
  :cpp:enumerator:`kAlgorithm_SSS_AES_GCM_INT_IV`,
  :cpp:enumerator:`kAlgorithm_SSS_AES_CTR_INT_IV`,
  :cpp:enumerator:`kAlgorithm_SSS_AES_CCM_INT_IV`.

- New MAC algorithm - :cpp:enumerator:`kAlgorithm_SSS_DES_CMAC8` supported.

- New api :cpp:func:`Se05x_API_ECPointMultiply_InputObj` added.

- New api :cpp:func:`Se05x_API_WriteSymmKey_Ver_extended` added to set key with minimun tag length for AEAD operations

- Removed all deprecated defines starting with ``With`` and replaced with ``SSS_HAVE_``

- ECKey authentication is updated to read SE.ECKA public key with attestation using
  :cpp:func:`Se05x_API_ReadObject_W_Attst_V2` or :cpp:func:`Se05x_API_ReadObject_W_Attst` (based on applet version)
  instead of GetData APDU. To authenicate the public key read with attestation, signature verification is performed
  on the data received from SE. See details of :cpp:func:`Se05x_API_ReadObject_W_Attst_V2` / :cpp:func:`Se05x_API_ReadObject_W_Attst`.

- sss_se05x_cipher_update() and sss_se05x_aead_update() APIs modified to use input buffer directly.

- Bugfix: Write of large binary files with policy fails on applet 3.x.


## Release v03.03.00

- sss_openssl_cipher_one_go() api modified to use EVP calls for AES (ECB, CBC, CTR)

- sss_se05x_cipher_update() api modified to use block size of 256 to enhance performance.


## Release v03.01.00

- Extended kSSS_KeyPart_Default for other objectType.

  - Earlier: Object type ``kSSS_KeyPart_Default`` is used for Binary Files,
    Certificates, Symmetric Keys, PCR and HMAC-key.

  - Now: UserID and Counter are added for ``kSSS_KeyPart_Default``.
    This means objectType of UserID and Counter will be ``kSSS_KeyPart_Default`` after
    calling :cpp:type:`sss_key_object_get_handle`.
    Comment for enum ``sss_key_part_t`` is updated accordingly.

- Added new API :cpp:func:`Se05x_API_WritePCR_WithType` with support to
  write transient PCR objects also.

- Deprecated API :cpp:func:`Se05x_API_WritePCR`. Added macro :c:macro:`ENABLE_DEPRECATED_API_WritePCR`
  to enable compilation of deprecated API :cpp:func:`Se05x_API_WritePCR`.
  Support will be removed by Q1 2022.

- Bugfix - Handling of result tag in case of failure in :cpp:func:`Se05x_API_AeadOneShot`,
  :cpp:func:`Se05x_API_AeadFinal` and
  :cpp:func:`Se05x_API_AeadCCMFinal`

- Bugfix - KVN12 key can be used for PlatformSCP authentication now in SE051.

- SE05x APDU - Response length set to 0 in error condition - :cpp:func:`tlvGet_u8buf`.

- Created separate library (``mwlog``) for logging framework. See :numref:`stack-logging`
  :ref:`stack-logging`

- Order of log level reversed. Current log level is - ``{"ERROR", "WARN ", "INFO ", "DEBUG"}``.

- Mbedtls ALT is extended with ECDSA verify operation using ``MBEDTLS_ECDSA_VERIFY_ALT`` define. (Disabled by default).
  Using this all EC public key verify operations can be performed using SE05x.

- Changed files under BSD3 License with NXP Copyright to Apache2 License.

- Changed files under Proprietary license to Apache 2 License.


## Release v03.00.06

- smCom_Init: return type is now *U16* instead of *void*. Return value indicates success/failure to create mutex/semophore.

- The enumerated type **SE05x_EDSignatureAlgo_t** contained a value **kSE05x_EDSignatureAlgo_ED25519PH_SHA_512**.
  The mnemonic name of the value was misleading as it actually corresponded to the `Pure EDDSA algorithm` not the
  `Prehashed (PH) EDDSA algorithm`. This has now been corrected. **This will require corresponding update in the application code.**

  - EDDSA signature algorithm enumerated value **kSE05x_EDSignatureAlgo_ED25519PH_SHA_512** is changed into **kSE05x_EDSignatureAlgo_ED25519PURE_SHA_512**.

  - EDDSA attestation algorithm enumerated value **kSE05x_AttestationAlgo_ED25519PH_SHA_512** is changed into as **kSE05x_AttestationAlgo_ED25519PURE_SHA_512**.

- Fixed typo in example code API: ex_sss_kestore_and_object_init() is now ex_sss_key_store_and_object_init()

- Added support for SE051 type

- Extended SE051 specific APDU command and response buffer size to match SE051's capabilities.

- SSS API blocks SHA512 attestation, signing and verification for RSA512 key

- Bug Fix : Fix for attestation read of symmetric objects which have no read policy.

- Added Platform SCP03 keys for SE051 (Variant A2 and C2).


## Release v03.00.02

- T1oI2C:

  - Fixed: potential null pointer dereference

  - Fixed: RSYNC _ + CRC error results in saving response to uninitialised buffer.

- ``hostlib/hostLib/platform/linux/i2c_a7.c``: A call to `axI2CTerm()` now closes the I2C file descriptor associated with the
  I2C communication channel.


## Release v03.00.00

- Initial commit

- Plug & Trust middleware to use secure element SE050
