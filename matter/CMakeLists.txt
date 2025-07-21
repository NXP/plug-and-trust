#
# Copyright 2025 NXP
# SPDX-License-Identifier: BSD-3-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if(NOT CHIP_ROOT)
    get_filename_component(CHIP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../.. REALPATH)
endif()

if (NOT DEFINED SIMW_LIB_DIR)
    get_filename_component(SIMW_LIB_DIR ${CHIP_ROOT}/third_party/simw-top-mini/repo REALPATH)
endif()

set(CONFIG_SDK_EXTERNAL_TARGET ${MCUX_SDK_PROJECT_NAME})

if (CONFIG_SE05X_LIB)
    add_library(simw_lib)

    get_property(SDK_INCLUDES TARGET ${CONFIG_SDK_EXTERNAL_TARGET} PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    get_property(SDK_DEFINES TARGET ${CONFIG_SDK_EXTERNAL_TARGET} PROPERTY INTERFACE_COMPILE_DEFINITIONS)

    target_sources(simw_lib PUBLIC
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/infra/global_platf.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/infra/sm_apdu.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/infra/sm_connect.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/infra/sm_errors.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/infra/sm_printf.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/log/nxLog.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/smCom.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/se05x/src/se05x_ECC_curves.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/se05x/src/se05x_mw.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/se05x/src/se05x_tlv.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/se05x_03_xx_xx/se05x_APDU.c
        ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_boot.c
        ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_boot_connectstring.c
        ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_se05x.c
        ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_se05x_auth.c
        ${SIMW_LIB_DIR}/sss/src/fsl_sss_apis.c
        ${SIMW_LIB_DIR}/sss/src/fsl_sss_util_asn1_der.c
        ${SIMW_LIB_DIR}/sss/src/fsl_sss_util_rsa_sign_utils.c
        ${SIMW_LIB_DIR}/sss/src/keystore/keystore_cmn.c
        ${SIMW_LIB_DIR}/sss/src/keystore/keystore_pc.c
        ${SIMW_LIB_DIR}/sss/src/mbedtls/fsl_sss_mbedtls_apis.c
        ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_apis.c
        ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_mw.c
        ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_policy.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/T1oI2C/phNxpEsePal_i2c.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/T1oI2C/phNxpEseProto7816_3.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/T1oI2C/phNxpEse_Api.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/smComT1oI2C.c
        ${SIMW_LIB_DIR}/demos/se05x_host_gpio/se05x_host_gpio.c
    )

    target_include_directories(simw_lib PUBLIC
        ${SDK_INCLUDES}
        ${SIMW_LIB_DIR}/sss/inc
        ${SIMW_LIB_DIR}/sss/port/default
        ${SIMW_LIB_DIR}/sss/ex/src
        ${SIMW_LIB_DIR}/hostlib/hostLib/inc
        ${SIMW_LIB_DIR}/sss/ex/inc
        ${SIMW_LIB_DIR}/hostlib/hostLib/platform/inc
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/infra
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/log
        ${SIMW_LIB_DIR}/hostlib/hostLib/se05x_03_xx_xx
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/T1oI2C
        ${SIMW_LIB_DIR}/hostlib/hostLib/accessManager/inc
        ${SIMW_LIB_DIR}/demos/se05x_host_gpio
    )
    target_compile_definitions(simw_lib
        PUBLIC
        ${SDK_DEFINES}
        SSS_USE_FTR_FILE
        T1oI2C
        T1oI2C_UM11225
        SMCOM_T1oI2C
    )
    if (CONFIG_SE05X_SCP03)
        target_sources(simw_lib PUBLIC
            ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_scp03_auth.c
            ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/nxScp/nxScp03_Com.c
            ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_eckey.c
            ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_scp03.c
        )
    endif()

    if (CONFIG_SE05X_SCP03)
        target_include_directories(simw_lib PUBLIC
            ${SIMW_LIB_DIR}/scp03_ftr
        )
    else()
        target_include_directories(simw_lib PUBLIC
            ${SIMW_LIB_DIR}
        )
    endif()

    if (CONFIG_MCUX_HW_DEVICE_RW612 OR CONFIG_MCUX_HW_DEVICE_RW610)
        target_sources(simw_lib PUBLIC
            ${SIMW_LIB_DIR}/hostlib/hostLib/platform/rt/rw61x/i2c_rw612.c
            ${SIMW_LIB_DIR}/hostlib/hostLib/platform/ksdk/timer_kinetis_freertos.c
        )
        target_compile_definitions(simw_lib
            PUBLIC
            AX_EMBEDDED
            SDK_OS_FREE_RTOS=1
            FRDM_RW612
        )
    endif()

endif()
