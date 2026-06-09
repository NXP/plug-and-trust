#
# Copyright 2026 NXP
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
    zephyr_library_named(simw_lib)
    get_property(SDK_INCLUDES TARGET app PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    get_property(SDK_DEFINES TARGET app PROPERTY INTERFACE_COMPILE_DEFINITIONS)

    zephyr_include_directories($<TARGET_PROPERTY:simw_lib,INTERFACE_INCLUDE_DIRECTORIES>)
    zephyr_compile_definitions($<TARGET_PROPERTY:simw_lib,INTERFACE_COMPILE_DEFINITIONS>)

    zephyr_include_directories("$<TARGET_PROPERTY:app,INTERFACE_INCLUDE_DIRECTORIES>")
    zephyr_compile_definitions($<TARGET_PROPERTY:app,INTERFACE_COMPILE_DEFINITIONS>)

    zephyr_library_sources(
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
        ${SIMW_LIB_DIR}/sss/src/psa/fsl_sss_psa_apis.c
        ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_apis.c
        ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_mw.c
        ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_policy.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/T1oI2C/phNxpEsePal_i2c.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/T1oI2C/phNxpEseProto7816_3.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/T1oI2C/phNxpEse_Api.c
        ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/smCom/smComT1oI2C.c
        ${SIMW_LIB_DIR}/demos/se05x_host_gpio/se05x_host_gpio.c
    )

    zephyr_include_directories(
        ${SDK_INCLUDES}
        ${SIMW_LIB_DIR}/sss/
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
        ${SIMW_LIB_DIR}/hostlib/hostLib/platform/zephyr
        ${SIMW_LIB_DIR}/demos/se05x_host_gpio
        ${SIMW_LIB_DIR}/demos/se051h_nfc_comm_prov/common
    )

    zephyr_compile_definitions(
        ${SDK_DEFINES}
        SSS_USE_FTR_FILE
        T1oI2C
        T1oI2C_UM11225
        SMCOM_T1oI2C
    )

    if (CONFIG_SE05X_SCP03 OR CONFIG_SE05X_USER_ID OR CONFIG_SE05X_AES_KEY OR CONFIG_SE05X_EC_KEY OR CONFIG_SE05X_SCP03_USER_ID OR CONFIG_SE05X_SCP03_AES_KEY OR CONFIG_SE05X_SCP03_EC_KEY)
        zephyr_library_sources(
            ${SIMW_LIB_DIR}/sss/ex/src/ex_sss_scp03_auth.c
            ${SIMW_LIB_DIR}/hostlib/hostLib/libCommon/nxScp/nxScp03_Com.c
            ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_eckey.c
            ${SIMW_LIB_DIR}/sss/src/se05x/fsl_sss_se05x_scp03.c
        )

    endif()

    if (CONFIG_SE05X_SCP03)
        zephyr_include_directories(
            ${SIMW_LIB_DIR}/feature_file/scp03_ftr
        )
    elseif (CONFIG_SE05X_USER_ID)
        zephyr_include_directories(
            ${SIMW_LIB_DIR}/feature_file/user_id_ftr
        )
    elseif (CONFIG_SE05X_AES_KEY)
        zephyr_include_directories(
            ${SIMW_LIB_DIR}/feature_file/aes_key_ftr
        )
    elseif (CONFIG_SE05X_EC_KEY)
        zephyr_include_directories(
            ${SIMW_LIB_DIR}/feature_file/ec_key_ftr
        )
    elseif (CONFIG_SE05X_SCP03_USER_ID)
        zephyr_include_directories(
            ${SIMW_LIB_DIR}/feature_file/scp03_user_id_ftr
        )
    elseif (CONFIG_SE05X_SCP03_AES_KEY)
        zephyr_include_directories(
            ${SIMW_LIB_DIR}/feature_file/scp03_aes_key_ftr
        )
    elseif (CONFIG_SE05X_SCP03_EC_KEY)
        zephyr_include_directories(
            ${SIMW_LIB_DIR}/feature_file/scp03_ec_key_ftr
        )
    else()
        zephyr_include_directories(
            ${SIMW_LIB_DIR}
        )
    endif()

    if (CONFIG_BOARD_FRDM_RW612)
        zephyr_library_sources(
            ${SIMW_LIB_DIR}/hostlib/hostLib/platform/zephyr/i2c_zephyr.c
            ${SIMW_LIB_DIR}/hostlib/hostLib/platform/ksdk/timer_kinetis_bm.c
        )
        zephyr_compile_definitions(
            AX_EMBEDDED
            FRDM_RW612
        )
    endif()

    set_property(GLOBAL APPEND PROPERTY ZEPHYR_INTERFACE_LIBS simw_lib)
endif()