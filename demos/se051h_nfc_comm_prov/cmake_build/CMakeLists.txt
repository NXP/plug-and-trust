CMAKE_MINIMUM_REQUIRED(VERSION 3.5.0)

project (se051h_nfc_comm_prov)

SET(SIMW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
INCLUDE(${SIMW_LIB_DIR}/simw_lib.cmake)

IF("${PTMW_SE05X_Auth}" STREQUAL "None")
ADD_EXECUTABLE(${PROJECT_NAME} ${SIMW_SE_SOURCES} main.c ../common/se051h_nfc_comm_prov.cpp)
ELSE()
ADD_EXECUTABLE(${PROJECT_NAME} ${SIMW_SE_SOURCES} ${SIMW_SE_AUTH_SOURCES} main.c ../common/se051h_nfc_comm_prov.cpp)
ENDIF()

IF("${PTMW_HostCrypto}" STREQUAL "OPENSSL")
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ssl crypto)
ENDIF()

TARGET_INCLUDE_DIRECTORIES(
    ${PROJECT_NAME}
    PUBLIC
    ../common/
    ${SIMW_INC_DIR}/
    ${SIMW_LIB_DIR}/
    )
