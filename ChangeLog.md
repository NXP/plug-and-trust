# Plug-And-Trust Mini Package Change Log

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
