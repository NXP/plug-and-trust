#
# Copyright 2025 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
# ****************************************************************
# Get passcode with SE05X
# ****************************************************************

cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Retrieve CHIP_ROOT if provided
if (DEFINED CHIP_ROOT)
    get_filename_component(CHIP_ROOT ${CHIP_ROOT} ABSOLUTE)
else()
    get_filename_component(CHIP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../ REALPATH)
endif()

# Export CHIP_ROOT as an environment variable to be recognized by Kconfig
set(COMMON_KCONFIG_ENV_SETTINGS CHIP_ROOT=${CHIP_ROOT})

get_filename_component(EXAMPLE_PLATFORM_NXP_COMMON_DIR ${CHIP_ROOT}/examples/platform/nxp/common REALPATH)
get_filename_component(NXP_MATTER_SUPPORT_DIR ${CHIP_ROOT}/third_party/nxp/nxp_matter_support REALPATH)

# Add secure element lib as an external module
list(APPEND EXTRA_MCUX_MODULES ${CHIP_ROOT}/third_party/simw-top-mini/repo/matter)

# If CONF_FILE_NAME is provided, append its full path to CONF_FILE
if (DEFINED CONF_FILE_NAME)
    set(CONF_FILE ${CONF_FILE} ${CHIP_ROOT}/examples/platform/nxp/config/${CONF_FILE_NAME})
endif()

# Add application prj.conf & Kconfig
set(CONF_FILE ${CONF_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/prj.conf)

set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)

# Load the MCUX SDK package
find_package(McuxSDK 3.0.0)

# Get compiler flags and includes and pass them to CHIP target
list(APPEND CONFIG_CHIP_EXTERNAL_TARGETS McuxSDK)

if (NOT DEFINED NXP_SDK_RECONFIG_CMAKE_DIR)
    if (CONFIG_MCUX_HW_DEVICE_MCXW727C)
        get_filename_component(NXP_SDK_RECONFIG_CMAKE_DIR ${NXP_MATTER_SUPPORT_DIR}/examples/platform/mcxw72 REALPATH)
    else()
        get_filename_component(NXP_SDK_RECONFIG_CMAKE_DIR ${NXP_MATTER_SUPPORT_DIR}/examples/platform/rt REALPATH)
    endif()
endif()

# Include the nxp_sdk_reconfig.cmake to customize the SDK for Matter application
include(${NXP_SDK_RECONFIG_CMAKE_DIR}/nxp_sdk_reconfig.cmake)
include(${CHIP_ROOT}/third_party/nxp/nxp_matter_support/cmake/sdk_hook.cmake)

# Define application project name
project(se05x_get_passcode)

target_include_directories(app
    PRIVATE
    ${CHIP_ROOT}/third_party/simw-top-mini/repo/demos/se05x_get_passcode/common
)

target_sources(app
    PRIVATE
    main.cpp
    ../common/se05x_get_passcode.cpp
)

if (CONFIG_MCUX_HW_DEVICE_MCXW727C)
    target_compile_definitions(simw_lib
        PUBLIC
        MBEDTLS_NXP_SSSAPI=1
        RL_ALLOW_CUSTOM_SHMEM_CONFIG=1
    )
endif()


mcux_add_macro(
    MBEDTLS_USER_CONFIG_FILE=\\\"${CHIP_ROOT}/third_party/simw-top-mini/repo/matter/se05x_demos_common/se05x_provision_mbedtls_user_config.h\\\"
)
if (CONFIG_MCUX_HW_DEVICE_MCXW727C)
        mcux_add_macro(
        CHIP_SYSTEM_CONFIG_USE_OPENTHREAD_ENDPOINT=1
    )
        target_sources(app
            PRIVATE
            ${CHIP_ROOT}/examples/platform/nxp/mcxw72/app/support/Memconfig.cpp
            ${CHIP_ROOT}/src/lib/support/logging/TextOnlyLogging.cpp
            ${CHIP_ROOT}/src/platform/nxp/common/Logging.cpp
        )
        target_include_directories(app
            PRIVATE
            ${CHIP_ROOT}
            ${CHIP_ROOT}/src
            ${CHIP_ROOT}/third_party/nlassert/repo/include
        )
else()
    target_sources(app
        PRIVATE
        ${CHIP_ROOT}/examples/platform/nxp/common/app/support/Memconfig.cpp
    )
endif()

# Use MCUX post-build function to convert the executable to binary format
mcux_convert_binary(
    BINARY ${APPLICATION_BINARY_DIR}/app.bin
    TARGET app
    TOOLCHAINS ${CONFIG_TOOLCHAIN}
    EXTRA_ARGS "${CONFIG_REMOVE_SECTIONS_FROM_BIN}"
)
