Introduction on Plug & Trust Middleware Mini Package
====================================================================

Plug and Trust middleware mini package contains the minimum files required to
connect to SE05x using t1oi2c protocol. The package is tested on
*Raspberry-Pi* with ``T=1 overI2C``.

The complete Plug and Trust middleware package can be downloaded from
https://www.nxp.com/products/:SE050. The package has support for other
platforms.

- iMX6UL, iMX8MQ - Linux
- Freedom K64F, i.MX RT 1060, LPC55S - FreeRTOS/Without RTOS
- Hikey 960 - Android
- Windows PC(Visual Studio)

It also includes other api usage examples, ssscli (command line tool to use
SE05x), cloud connectivity examples, openssl engine, pkcs11 interface, AWS
Greengrass, OPCUA and more. More details regarding SE05x and other detailed
application notes can be found at https://www.nxp.com/products/:SE050,
https://www.nxp.com/products/:SE051 and https://www.nxp.com/products/:SE052F .

To contact NXP or to report issues, please use https://www.nxp.com/support/support:SUPPORTHOME

It is recommended to use the latest version of this software available at https://github.com/NXP/plug-and-trust/

Change Log
-------------------------------------------------------------
Refer ChangeLog.md


Folder structure of the Mini Pacakge
-------------------------------------------------------------

The folder structure of mini package is as under::

    ├───ecc_example
    ├───hostlib
    │   └───hostLib
    │       ├───inc
    │       ├───libCommon
    │       │   ├───infra
    │       │   ├───nxScp
    │       │   └───smCom
    │       │       └───T1oI2C
    │       ├───platform
    │       │   ├───generic
    │       │   ├───inc
    │       │   ├───linux
    │       │   └───rsp
    │       ├───se05x
    │       │   └───src
    │       └───se05x_03_xx_xx
    └───sss
        ├───ex
        │   ├───ecc
        │   ├───inc
        │   └───src
        ├───inc
        ├───port
        │   └───default
        └───src
            ├───keystore
            ├───mbedtls
            ├───openssl
            └───se05x

Important folders are as under:

:ecc_example:  ECC sign and verify example. (Tested on Raspberry Pi)

:hostlib:  This folder contains the common part of host library e.g. ``T=1oI2C`` communication
           protocol stack, SE050 APIs, etc.

:sss:  This folder contains the **SSS APIs** interface to the Application Layer.


Prerequisite
-------------------------------------------------------------
- Linux should be running on the Raspberry Pi development board,
  the release was tested with Raspbian Buster (``4.19.75-v7l+``)
- SE050/SE051/SE052/A5000 connected to i2c-1 port of Raspberry Pi.


ECC example
-------------------------------------------------------------

This example demonstrates Elliptic Curve Cryptography sign and verify
operation using SSS APIs (``/sss/ex/ecc/ex_sss_ecc.c``).

Execute the command below to test the ecc example ::

    cd ecc_example
    mkdir build
    cd build
    cmake ..
    cmake --build .
    ./ex_ecc

Refer **`CMAKE Options` section** to configure the middleware for different applets / session authentication / host crypto.


Build Applications using Mini Package
-------------------------------------------------------------

Before using the SE05x for any crypto operarions, applications are required to open a session.
Use the following APIs (from ``sss\ex\inc\ex_sss_boot.h``) to do a session open.
Refer **`CMAKE Options` section** for more details on Session Authentication).

..  code-block:: c

    sss_status_t ex_sss_boot_connectstring(int argc, const char *argv[], char **pPortName);

    sss_status_t ex_sss_boot_open(ex_sss_boot_ctx_t *pCtx, const char *portName);

    sss_status_t ex_sss_key_store_and_object_init(ex_sss_boot_ctx_t *pCtx);

The above apis will abstract all the necessary actions required to perform a session open to SE05x.

Alternatively, you can simply inlcude the header file ``ex_sss_main_inc.h`` which uses the above APIs to perform the session open to se05x.

Refer ecc example - ``/sss/ex/ecc/ex_sss_ecc.c``.

Applications code can start with function `ex_sss_entry`.

..  code-block:: c

    sss_status_t ex_sss_entry(ex_sss_boot_ctx_t *pCtx)


CMAKE Options
--------------

PTMW_Applet
************
::

    Secure Element Applet

    You can compile middleware library for different Applets listed below.

    ``-DPTMW_Applet=SE05X_A``: SE050 Type A (ECC)

    ``-DPTMW_Applet=SE05X_B``: SE050 Type B (RSA)

    ``-DPTMW_Applet=SE05X_C``: SE050 (Super set of A + B)

    ``-DPTMW_Applet=SE051_H``: SE051 with SPAKE Support

    ``-DPTMW_Applet=AUTH``: AUTH

    ``-DPTMW_Applet=SE050_E``: SE050E

PTMW_SE05X_Ver
**************
::

    SE05X Applet version.

    Selection of Applet version 03_XX enables SE050 features.
    Selection of Applet version 07_00 enables SE051 / SE052 features.

    ``-DPTMW_SE05X_Ver=03_XX``: SE050

    ``-DPTMW_SE05X_Ver=07_02``: SE051 / SE052

PTMW_HostCrypto
***************
::

    Counterpart Crypto on Host

    What is being used as a cryptographic library on the host.
    As of now only OpenSSL is supported

    ``-DPTMW_HostCrypto=OPENSSL``: Use OpenSSL as host crypto

    ``-DPTMW_HostCrypto=None``: NO Host Crypto

PTMW_SE05X_Auth
***************
::

    SE05x Authentication

    This settings is used by examples to connect using various options
    to authenticate with the Applet.
    Make sure you set PTMW_HostCrypto to Openssl to use any Authentication.

    ``-DPTMW_SE05X_Auth=None``: Use the default session (i.e. session less) login

    ``-DPTMW_SE05X_Auth=UserID``: Do User Authentication with UserID

    ``-DPTMW_SE05X_Auth=PlatfSCP03``: Use Platform SCP for connection to SE

    ``-DPTMW_SE05X_Auth=AESKey``: Do User Authentication with AES Key
        Earlier this was called AppletSCP03

    ``-DPTMW_SE05X_Auth=ECKey``: Do User Authentication with EC Key
        Earlier this was called FastSCP

    ``-DPTMW_SE05X_Auth=UserID_PlatfSCP03``: UserID and PlatfSCP03

    ``-DPTMW_SE05X_Auth=AESKey_PlatfSCP03``: AESKey and PlatfSCP03

    ``-DPTMW_SE05X_Auth=ECKey_PlatfSCP03``: ECKey and PlatfSCP03


Port Mini package to different platform
-------------------------------------------------------------

To port the mini package to different platform, the i2c interface needs to be
ported. Exsisting implementation for i2c read/write on Raspberry Pi is in -
``hostlib/hostLib/platform/linux/i2c_a7.c``.

Other file that may require porting is -
``hostlib/hostLib/platform/generic/sm_timer.c``
