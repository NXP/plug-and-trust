CMAKE_MINIMUM_REQUIRED(VERSION 3.5.0)

PROJECT(se05x_get_passcode)

SET(SIMW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
INCLUDE(${SIMW_LIB_DIR}/simw_lib.cmake)

IF(SSS_HAVE_SE05X_AUTH_NONE)
ADD_EXECUTABLE(${PROJECT_NAME} ${SIMW_SE_SOURCES} main.c ../common/se05x_get_passcode.cpp ${SIMW_LIB_DIR}/demos/se05x_host_gpio/se05x_host_gpio.c)
ELSE()
ADD_EXECUTABLE(${PROJECT_NAME} ${SIMW_SE_SOURCES} ${SIMW_SE_AUTH_SOURCES} main.c ../common/se05x_get_passcode.cpp ${SIMW_LIB_DIR}/demos/se05x_host_gpio/se05x_host_gpio.c)
ENDIF()

SIMW_LINK_HOSTCRYPTO(${PROJECT_NAME})

TARGET_INCLUDE_DIRECTORIES(
    ${PROJECT_NAME}
    PUBLIC
    ../common/
    ${SIMW_INC_DIR}/
    ${SIMW_LIB_DIR}/
    ${SIMW_LIB_DIR}/demos/se05x_host_gpio
    )
