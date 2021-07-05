# Plug-And-Trust Mini Package Change Log

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
