#
# Copyright 2025 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
# ****************************************************************
# Device attestation Provisioning with SE05X
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
list(APPEND EXTRA_MCUX_MODULES ${CHIP_ROOT}/third_party/simw-top-mini/repo)

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
    get_filename_component(NXP_SDK_RECONFIG_CMAKE_DIR ${NXP_MATTER_SUPPORT_DIR}/examples/platform/rt REALPATH)
endif()

# Include the nxp_sdk_reconfig.cmake to customize the SDK for Matter application
include(${NXP_SDK_RECONFIG_CMAKE_DIR}/nxp_sdk_reconfig.cmake)

# Define application project name
project(se05x_dev_attest_key_prov)

target_include_directories(app
    PRIVATE
    ${CHIP_ROOT}/third_party/simw-top-mini/repo/demos/se05x_dev_attest_key_prov/common
)

target_sources(app
    PRIVATE
    main.cpp
    ../common/se05x_dev_attest_key_prov.cpp
    ${CHIP_ROOT}/examples/platform/nxp/common/app/support/Memconfig.cpp
)
