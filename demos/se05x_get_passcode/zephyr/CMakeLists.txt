#
# Copyright 2026 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
# ****************************************************************
# Device attestation Provisioning with SE05X using Zephyr
# ****************************************************************

cmake_minimum_required(VERSION 3.13.1)

# Export CHIP_ROOT as an environment variable to be recognized by Kconfig
# Retrieve CHIP_ROOT if provided
if (DEFINED CHIP_ROOT)
    get_filename_component(CHIP_ROOT ${CHIP_ROOT} ABSOLUTE)
else()
    get_filename_component(CHIP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../ REALPATH)
endif()

# Set the board and overlay before finding Zephyr
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
if(NOT DEFINED DTC_OVERLAY_FILE)
    set(DTC_OVERLAY_FILE ${CHIP_ROOT}/third_party/simw-top-mini/repo/matter/zephyr/board/frdm_rw612.overlay)
endif()

get_filename_component(EXAMPLE_PLATFORM_NXP_COMMON_DIR ${CHIP_ROOT}/examples/platform/nxp/common REALPATH)

# Add secure element lib as an external module
list(APPEND ZEPHYR_EXTRA_MODULES ${CHIP_ROOT}/third_party/simw-top-mini/repo/matter)

# Find Zephyr package
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# If CONF_FILE_NAME is provided, append its full path to CONF_FILE
if (DEFINED CONF_FILE_NAME)
    set(CONF_FILE ${CONF_FILE} ${CHIP_ROOT}/examples/platform/nxp/config/${CONF_FILE_NAME})
endif()

# Add application prj.conf & Kconfig
set(CONF_FILE ${CONF_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/prj.conf)

set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)

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
