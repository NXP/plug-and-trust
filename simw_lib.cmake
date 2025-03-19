#
# Copyright 2023-2024 NXP
# SPDX-License-Identifier: BSD-3-Clause
#

FILE(
    GLOB
    SIMW_SE_SOURCES
    ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_boot.c
    ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_boot_connectstring.c
    ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_se05x.c
    ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_se05x_auth.c
    ${SIMW_LIB_DIR}/sss/src/*.c
    ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_apis.c
    ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_mw.c
    ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_policy.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/infra/*.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/log/nxLog.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/smCom.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/platform/rsp/se05x_reset.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/platform/generic/sm_timer.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/se05x/src/se05x_ECC_curves.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/se05x/src/se05x_mw.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/se05x/src/se05x_tlv.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/se05x_03_xx_xx/se05x_APDU.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/smComT1oI2C.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/T1oI2C/*.c
    ${SIMW_LIB_DIR}/hostlib/hostLib/platform/linux/i2c_a7.c
    ${SIMW_LIB_DIR}/sss/src/openssl/fsl_sss_openssl_apis.c
    ${SIMW_LIB_DIR}/sss/src/keystore/keystore_cmn.c
    ${SIMW_LIB_DIR}/sss/src/keystore/keystore_openssl.c
    ${SIMW_LIB_DIR}/sss/src/keystore/keystore_pc.c
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
    SIMW_INC_DIR
    ${SIMW_LIB_DIR}
    ${SIMW_LIB_DIR}/sss/inc
    ${SIMW_LIB_DIR}/sss/port/default
    ${SIMW_LIB_DIR}/sss/ex/src
    ${SIMW_LIB_DIR}/sss/ex/inc
    ${SIMW_LIB_DIR}/hostlib/hostLib/inc
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/infra
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/log
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/T1oI2C
    ${SIMW_LIB_DIR}/hostlib/hostLib/se05x_03_xx_xx
    ${SIMW_LIB_DIR}/hostlib/hostLib/platform/inc
    ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom
)

ADD_DEFINITIONS(-fPIC)
ADD_DEFINITIONS(-DSSS_USE_FTR_FILE)
ADD_DEFINITIONS(-DSMCOM_T1oI2C)
ADD_DEFINITIONS(-DT1oI2C)
ADD_DEFINITIONS(-DT1oI2C_UM11225)
ADD_DEFINITIONS(-DT1OI2C_RETRY_ON_I2C_FAILED)

#ADD_DEFINITIONS(-DFLOW_VERBOSE)

IF(DEFINED SE_RESET_LOGIC)
    add_definitions(-DSE_RESET_LOGIC=${SE_RESET_LOGIC} )
ENDIF()

INCLUDE(${SIMW_LIB_DIR}/simwlib_cmake_options.cmake)
