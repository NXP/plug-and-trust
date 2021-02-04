# Copyright 2019,2020 NXP
#
# This software is owned or controlled by NXP and may only be used
# strictly in accordance with the applicable license terms.  By expressly
# accepting such terms or by downloading, installing, activating and/or
# otherwise using the software, you are agreeing that you have read, and
# that you agree to comply with and are bound by, such license terms.  If
# you do not agree to be bound by the applicable license terms, then you
# may not retain, install, activate or otherwise use the software.
#

PROJECT(jrcplib)

FILE(
    GLOB
    jrcplib_sources
    src/jrcplib/cpp/*.cpp
    src/jrcplib/cpp/networking/*.cpp
)

ADD_LIBRARY(${PROJECT_NAME} ${jrcplib_sources})

IF(
    "Linux"
    STREQUAL
    "${CMAKE_SYSTEM_NAME}"
)
    TARGET_COMPILE_OPTIONS(
        ${PROJECT_NAME}
        PRIVATE -DJRCPLIB_BUILD
        PRIVATE -std=c++11
    )
ELSEIF(
    "Darwin"
    STREQUAL
    "${CMAKE_SYSTEM_NAME}"
)
    TARGET_COMPILE_OPTIONS(
        ${PROJECT_NAME}
        PRIVATE -DJRCPLIB_BUILD
        PRIVATE -std=c++11
        PRIVATE -Wno-ignored-qualifiers
        PRIVATE -Wno-error=ignored-qualifiers
    )
ELSEIF(
    "GNU"
    STREQUAL
    "${CMAKE_C_COMPILER_ID}"
)
    TARGET_COMPILE_OPTIONS(
        ${PROJECT_NAME}
        PRIVATE -DJRCPLIB_BUILD
        PRIVATE -std=c++11
    )
ENDIF()

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC inc)

IF(
    "Windows"
    STREQUAL
    "${CMAKE_SYSTEM_NAME}"
)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ws2_32)
ENDIF()

IF(WithHost_LinuxLike)
    INSTALL(TARGETS ${PROJECT_NAME} DESTINATION lib)
ENDIF()
