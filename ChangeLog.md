# Plug-And-Trust Mini Package Change Log

## Release v03.00.02

- T1oI2C:

  - Fixed: potential null pointer dereference

  - Fixed: RSYNC _ + CRC error results in saving response to uninitialised buffer.

- ``hostlib/hostLib/platform/linux/i2c_a7.c``: A call to `axI2CTerm()` now closes the I2C file descriptor associated with the
  I2C communication channel.


## Release v03.00.00

- Initial commit

- Plug & Trust middleware to use secure element SE050
