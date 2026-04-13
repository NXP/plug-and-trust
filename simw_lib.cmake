#
# Copyright 2023-2024,2026 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
INCLUDE(${SIMW_LIB_DIR}/simwlib_cmake_options.cmake)

FILE(
    GLOB
    SIMW_SE_SOURCES
    ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_boot.c
    ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_boot_connectstring.c
    ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_se05x.c
    ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_se05x_auth.c
    ${SIMW_LIB_DIR}/sss/src/fsl_sss_apis.c
    ${SIMW_LIB_DIR}/sss/src/fsl_sss_util_asn1_der.c
    ${SIMW_LIB_DIR}/sss/src/fsl_sss_util_rsa_sign_utils.c
    ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_apis.c
    ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_mw.c
    ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_policy.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/infra/*.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/log/nxLog.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/smCom.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/platform/generic/sm_timer.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/se05x/src/se05x_ECC_curves.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/se05x/src/se05x_mw.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/se05x/src/se05x_tlv.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/se05x_03_xx_xx/se05x_APDU.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/smComT1oI2C.c
    ${SIMW_LIB_DIR}/sss/src/openssl/fsl_sss_openssl_apis.c
    ${SIMW_LIB_DIR}/sss/src/keystore/keystore_cmn.c
    ${SIMW_LIB_DIR}/sss/src/keystore/keystore_openssl.c
    ${SIMW_LIB_DIR}/sss/src/keystore/keystore_pc.c
)

FILE(
    GLOB
    SIMW_T1OI2C_SOURCES
    ${SIMW_LIB_DIR}/hostlib/hostLib/platform/rsp/se05x_reset.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/T1oI2C/phNxpEse_Api.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/T1oI2C/phNxpEsePal_i2c.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/T1oI2C/phNxpEseProto7816_3.c
)

FILE(
    GLOB
    SIMW_SE_AUTH_SOURCES
    ##### Authenticated session to se05x
    ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_scp03_auth.c
    ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_eckey.c
    ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_scp03.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/nxScp/nxScp03_Com.c
)

FILE(
    GLOB
    SIMW_VCOM_WIN_SOURCES
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/smComSerial_win32.c
)

FILE(
    GLOB
    SIMW_PLATFORM_LINUX_SOURCES
    ${SIMW_LIB_DIR}/hostlib/hostLib/platform/linux/i2c_a7.c
)

FILE(
    GLOB
    SIMW_VCOM_LINUX_SOURCES
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/smComSerial_PCLinux.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/smComSocket_fd.c
)

SET(SIMW_COMMON_INC_DIR
    ${SIMW_LIB_DIR}
    ${SIMW_LIB_DIR}/sss
    ${SIMW_LIB_DIR}/sss/inc
    ${SIMW_LIB_DIR}/sss/port/default
    ${SIMW_LIB_DIR}/sss/ex/src
    ${SIMW_LIB_DIR}/sss/ex/inc
    ${SIMW_LIB_DIR}/hostlib/hostLib/inc
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/infra
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/log
    ${SIMW_LIB_DIR}/hostlib/hostLib/se05x_03_xx_xx
)

SET(SIMW_PLATFORM_INC_DIR
    ${SIMW_LIB_DIR}/hostlib/hostLib/platform/inc
)

SET(SIMW_SMCOM_INC_DIR
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom
)

IF(SSS_HAVE_SMCOM_T1OI2C)
    LIST(APPEND SIMW_SMCOM_INC_DIR
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/T1oI2C
    )
ENDIF()

IF(SSS_HAVE_SMCOM_VCOM)
    LIST(APPEND SIMW_SMCOM_INC_DIR
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom
    )
ENDIF()

IF(SSS_HAVE_HOST_PCWINDOWS)
    IF(SSS_HAVE_HOSTCRYPTO_OPENSSL)
        IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
            # 64-bit MSVC
            MESSAGE(STATUS "64-bit build detected")
            INCLUDE_DIRECTORIES("C:/Program Files/OpenSSL-Win64/include")
            LINK_DIRECTORIES("C:/Program Files/OpenSSL-Win64/lib/VC/x64/MTd")
            SET(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64" CACHE PATH "OpenSSL root directory" FORCE)
            SET(LIB_EAY_RELEASE "C:/Program Files/OpenSSL-Win64/lib/VC/x64/MTd/libcrypto.lib" CACHE FILEPATH "OpenSSL crypto library" FORCE)
            SET(SSL_EAY_RELEASE "C:/Program Files/OpenSSL-Win64/lib/VC/x64/MTd/libssl.lib" CACHE FILEPATH "OpenSSL SSL library" FORCE)
        ELSE()
            # 32-bit MSVC
            MESSAGE(STATUS "32-bit build detected")
            INCLUDE_DIRECTORIES("C:/Program Files/OpenSSL-Win32/include")
            SET(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win32" CACHE PATH "OpenSSL root directory" FORCE)
            SET(LIB_EAY_RELEASE "C:/Program Files/OpenSSL-Win32/lib/VC/x32/MTd/libcrypto.lib" CACHE FILEPATH "OpenSSL crypto library" FORCE)
            SET(SSL_EAY_RELEASE "C:/Program Files/OpenSSL-Win32/lib/VC/x32/MTd/libssl.lib" CACHE FILEPATH "OpenSSL SSL library" FORCE)
        ENDIF()
    ENDIF()
ENDIF()

SET(SIMW_INC_DIR
    ${SIMW_COMMON_INC_DIR}
    ${SIMW_PLATFORM_INC_DIR}
    ${SIMW_SMCOM_INC_DIR}
)

IF(SSS_HAVE_HOST_PCWINDOWS)
    ADD_DEFINITIONS(-DRJCT_VCOM)
    LIST(
        APPEND
        SIMW_SE_SOURCES
        ${SIMW_VCOM_WIN_SOURCES}
    )
ELSE()
    IF(SSS_HAVE_HOST_PCLINUX)
        IF(SSS_HAVE_SMCOM_VCOM)
            LIST(
                APPEND
                SIMW_SE_SOURCES
                ${SIMW_PLATFORM_LINUX_SOURCES}
                ${SIMW_VCOM_LINUX_SOURCES}
            )
            ADD_DEFINITIONS(-DRJCT_VCOM)
        ENDIF()
        IF(SSS_HAVE_SMCOM_T1OI2C)
            LIST(
                APPEND
                SIMW_SE_SOURCES
                ${SIMW_PLATFORM_LINUX_SOURCES}
                ${SIMW_T1OI2C_SOURCES}
            )
        ENDIF()
    ENDIF()
ENDIF()

IF(SSS_HAVE_SMCOM_T1OI2C)
    ADD_DEFINITIONS(-fPIC)
    ADD_DEFINITIONS(-DSMCOM_T1oI2C)
    ADD_DEFINITIONS(-DT1oI2C)
    ADD_DEFINITIONS(-DT1oI2C_UM11225)
    ADD_DEFINITIONS(-DT1OI2C_RETRY_ON_I2C_FAILED)
ENDIF()

ADD_DEFINITIONS(-DSSS_USE_FTR_FILE)
#ADD_DEFINITIONS(-DFLOW_VERBOSE)
ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)

FUNCTION(SIMW_LINK_HOSTCRYPTO target_name)
    # Windows
    IF(SSS_HAVE_HOST_PCWINDOWS)
        IF(SSS_HAVE_HOSTCRYPTO_OPENSSL)
            SET(OPENSSL_LIBS
                libcrypto
                libssl
                Crypt32
                ws2_32
            )
        ENDIF()
        TARGET_COMPILE_OPTIONS(
            ${PROJECT_NAME}
            PRIVATE
                /wd4267   # Size truncation warning
                /wd4244   # Narrowing integer conversion warning
        )
        IF(OPENSSL_LIBS)
            TARGET_LINK_LIBRARIES(${target_name} PRIVATE ${OPENSSL_LIBS})
        ENDIF()
    ENDIF()

    IF(SSS_HAVE_HOST_PCLINUX)
        LINK_DIRECTORIES("/usr/lib/")
        INCLUDE_DIRECTORIES(/usr/include)
        TARGET_LINK_LIBRARIES(${target_name} PRIVATE
            ssl
            crypto
        )
    ENDIF()
ENDFUNCTION()

IF(DEFINED SE_RESET_LOGIC)
    ADD_DEFINITIONS(-DSE_RESET_LOGIC=${SE_RESET_LOGIC} )
ENDIF()

