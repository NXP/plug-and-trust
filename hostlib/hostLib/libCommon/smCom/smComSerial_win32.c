/* Copyright 2018,2020,2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */
#include "sm_types.h"
#include "windows.h"
#include <stdlib.h>
#include <stdio.h>
#include "smComSerial.h"
#include "WinDef.h"
#include "WinBase.h"
#include "string.h"
#include <assert.h>
#include <limits.h>
#if defined(SSS_USE_FTR_FILE)
#include "fsl_sss_ftr.h"
#else
#include "fsl_sss_ftr_default.h"
#endif

#define REMOTE_JC_SHELL_HEADER_LEN (4)
#define REMOTE_JC_SHELL_MSG_TYPE_APDU_DATA (0x01)
#include "sm_apdu.h"
#define MAX_BUF_SIZE (MAX_APDU_BUF_LENGTH)

#ifdef FLOW_VERBOSE
#define NX_LOG_ENABLE_SMCOM_DEBUG 1
#endif

#include "nxLog_smCom.h"
#include "nxEnsure.h"

#define MTY_ATR 0x00
#define MTY_CLOSE 0x03
#define NAD 0x00

static U8 Header[2] = {0x01, 0x00};
static U8 sockapdu[MAX_BUF_SIZE];
static U8 response[MAX_BUF_SIZE];
static U8 *pCmd = (U8 *)&sockapdu;
static U8 *pRsp = (U8 *)&response;

static HANDLE gpComHandle = INVALID_HANDLE_VALUE;

static void escapeComPortName(char pOutPortName[20], const char *iPortName)
{
    ENSURE_OR_GO_EXIT(iPortName != NULL);
    ENSURE_OR_GO_EXIT(strlen(iPortName) < 20);

    strncpy(pOutPortName, iPortName, strlen(iPortName));
    if (0 == _strnicmp(iPortName, "COM", 3)) {
        long number = atol(&iPortName[3]);
        if (number > 4) {
            ENSURE_OR_GO_EXIT((_snprintf(pOutPortName, 20, "\\\\.\\%s", iPortName)) >= 0);
        }
    }
    else {
        ENSURE_OR_GO_EXIT((_snprintf(pOutPortName, 20, "%s", iPortName)) >= 0);
    }
exit:
    return;
}

U32 smComVCom_Open(void** vcom_ctx, const char *pComPortString)
{
    U32 status = 0;
    COMMTIMEOUTS cto;
    char escaped_port_name[20] = {0};
    static HANDLE pComHandle = INVALID_HANDLE_VALUE;
    pComHandle = gpComHandle;

#ifdef UNICODE
    wchar_t wPortName[20] = {0};
#endif
    /* Prepare CTO structure */
    cto.ReadTotalTimeoutConstant = 500;
    cto.ReadTotalTimeoutMultiplier = 0;
    cto.ReadIntervalTimeout = 10;
    cto.WriteTotalTimeoutConstant = 0;
    cto.WriteTotalTimeoutMultiplier = 0;

    escapeComPortName(escaped_port_name, pComPortString);

    printf("Opening COM Port '%s'\n", escaped_port_name);
#if SSS_HAVE_APPLET_SE051_UWB
    if (pComHandle != INVALID_HANDLE_VALUE) {
        printf("\n Already  COM Port Open \n ");
        if (vcom_ctx != NULL) {
            *vcom_ctx = pComHandle;
        }
        return SMCOM_COM_ALREADY_OPEN;
    }
#endif
#ifdef UNICODE
    mbstowcs(wPortName, escaped_port_name, sizeof(wPortName) / sizeof(wPortName[0]));
    pComHandle = CreateFile(wPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
#else
    pComHandle = CreateFile(escaped_port_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
#endif

    status = GetLastError();

    if (status == ERROR_SUCCESS) {
        status = smComVCom_SetState(pComHandle);

        if (status == 0) {
            if (SetCommTimeouts(pComHandle, &cto) == false) {
                status = 1;
            }
        }
    }
    else if (ERROR_FILE_NOT_FOUND == status) {
        printf("ERROR! Failed opening '%s'. ERROR=ERROR_FILE_NOT_FOUND\n", pComPortString);
    }
    else if (ERROR_ACCESS_DENIED == status) {
        printf("ERROR! Failed opening '%s'. ERROR=ERROR_ACCESS_DENIED\n", pComPortString);
    }
    else if (pComHandle == INVALID_HANDLE_VALUE) {
        printf("ERROR! Failed opening '%s'. ERROR=%X\n", escaped_port_name, status);
    }

    if (vcom_ctx == NULL) {
        gpComHandle = pComHandle;
    }
    else {
        *vcom_ctx = pComHandle;
        gpComHandle = pComHandle;
    }
    return status;
}

U32 smComVCom_SetState(void* conn_ctx)
{
    DCB dcb;
    memset(&dcb, 0, sizeof(dcb));
    HANDLE pComHandle = (conn_ctx == NULL) ? gpComHandle : (HANDLE)conn_ctx;

    dcb.DCBlength = sizeof(DCB);
    dcb.BaudRate = 115200;
    dcb.fBinary = true;
    dcb.fParity = false;
    dcb.fOutxCtsFlow = false;
    dcb.fOutxDsrFlow = false;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = false;
    dcb.fTXContinueOnXoff = true;
    dcb.fOutX = false;
    dcb.fInX = false;
    dcb.fErrorChar = false;
    dcb.fNull = false;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fAbortOnError = false;
    dcb.XonLim = 0;
    dcb.XoffLim = 0;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    if (SetCommState(pComHandle, &dcb) == false) {
        return 1;
    }
    else {
        EscapeCommFunction(pComHandle, SETDTR);
        smCom_Init(&smComVCom_Transceive, &smComVCom_TransceiveRaw);
        return 0;
    }
}

U32 smComVCom_GetATR(void* conn_ctx, U8 *pAtr, U16 *atrLen)
{
    HANDLE pComHandle = (conn_ctx == NULL) ? gpComHandle : (HANDLE)conn_ctx;
    U32 expectedLength = 0;
    U32 totalReceived = 0;
    U8 lengthReceived = 0;
    DWORD WrittenLen = 0;
    U8 status;
    U32 rc = 1;

    // wait 256 ms
    U8 ATRCmd[4] = {MTY_ATR, NAD, 0, 0};

    ENSURE_OR_GO_EXIT(pAtr != NULL);
    ENSURE_OR_GO_EXIT(atrLen != NULL);

    LOG_MAU8_D("Get ATR", ATRCmd, sizeof(ATRCmd));
    status = WriteFile(pComHandle, ATRCmd, sizeof(ATRCmd), &WrittenLen, NULL);
    if ((status == 0) || (WrittenLen != sizeof(ATRCmd))) {
        return 1;
    }

    expectedLength = REMOTE_JC_SHELL_HEADER_LEN; // remote JC shell header length
    ENSURE_OR_GO_EXIT(expectedLength <= *atrLen);

    while (totalReceived < expectedLength) {
        U32 maxCommLength;
        DWORD numBytesRead = 0;
        if (lengthReceived == 0) {
            maxCommLength = REMOTE_JC_SHELL_HEADER_LEN - totalReceived;
        }
        else {
            maxCommLength = expectedLength - totalReceived;
        }

        ENSURE_OR_GO_EXIT(maxCommLength <= ((*atrLen) - totalReceived) );
        status = ReadFile(pComHandle, (char *)&pAtr[totalReceived], maxCommLength, &numBytesRead, NULL);
        if (status == 0) {
            return 1;
        }
        else {
            ENSURE_OR_GO_EXIT((UINT32_MAX - totalReceived) >= numBytesRead);
            totalReceived += numBytesRead;
        }
        if ((totalReceived >= REMOTE_JC_SHELL_HEADER_LEN) && (lengthReceived == 0)) {
            expectedLength += ((pAtr[2] << 8) | (pAtr[3]));
            lengthReceived = 1;
        }
    }

    totalReceived -= 4; // Remove the 4 bytes of the Remote JC Terminal protocol
    ENSURE_OR_GO_EXIT(*atrLen >= REMOTE_JC_SHELL_HEADER_LEN + totalReceived);
    memmove(pAtr, pAtr + 4, totalReceived);

    *atrLen = (U16)totalReceived;
    rc = 0;
exit:
    return rc;
}

U32 smComVCom_Transceive(void *conn_ctx, apdu_t *pApdu)
{
#if defined(LOG_SOCK)
    int i;
#endif
    U32 txLen = 0;
    U32 expectedLength = 0;
    U32 totalReceived = 0;
    U8 lengthReceived = 0;
    U8 status;
    DWORD WrittenLen = 0;
    U32 rv = SMCOM_SND_FAILED;
    HANDLE pComHandle = (conn_ctx == NULL) ? gpComHandle : (HANDLE)conn_ctx;

    ENSURE_OR_GO_EXIT(pApdu != NULL);

    //pApdu->rxlen = 0;
    memset(sockapdu, 0x00, MAX_BUF_SIZE);
    memset(response, 0x00, MAX_BUF_SIZE);

    // remote JC Terminal header construction
    txLen = pApdu->buflen;
    memcpy(pCmd, Header, sizeof(Header));
    pCmd[2] = (txLen & 0xFF00) >> 8;
    pCmd[3] = txLen & 0xFF;
    memcpy(&pCmd[4], pApdu->pBuf, pApdu->buflen);
    ENSURE_OR_GO_EXIT((UINT16_MAX - 4) >= pApdu->buflen);
    pApdu->buflen += 4; /* header & length */

#ifdef LOG_SOCK
    sm_printf(CONSOLE, "   send: ");
    for (i = 4; i < (txLen + 4); i++) {
        sm_printf(CONSOLE, "%02X", pCmd[i]);
    }
    sm_printf(CONSOLE, "\n");
#endif

    LOG_MAU8_D("H>", pCmd, 4);
    LOG_MAU8_D("Tx>", pCmd + 4, pApdu->buflen - 4);
    status = WriteFile(pComHandle, pCmd, pApdu->buflen, &WrittenLen, NULL);
    if ((status == 0) || (WrittenLen != pApdu->buflen)) {
        fprintf(stderr, "Client: send() failed: error %i.\n", WrittenLen);
        return SMCOM_SND_FAILED;
    }

    expectedLength = REMOTE_JC_SHELL_HEADER_LEN; // remote JC shell header length

    while (totalReceived < expectedLength) {
        DWORD numBytesRead = 0;
        status = ReadFile(pComHandle, (char *)&pRsp[totalReceived], (MAX_BUF_SIZE - totalReceived), &numBytesRead, NULL);
        ENSURE_OR_GO_EXIT(numBytesRead <= INT32_MAX);

        if (status == 0) {
            rv = SMCOM_RCV_FAILED;
            goto exit;
        }
        else {
            totalReceived += numBytesRead;
        }
        if ((totalReceived >= REMOTE_JC_SHELL_HEADER_LEN) && (lengthReceived == 0)) {
            if ((UINT32_MAX - expectedLength) < (U32)((pRsp[2] << 8) | (pRsp[3]))) {
                return 1;
            }
            expectedLength += ((pRsp[2] << 8) | (pRsp[3]));
            lengthReceived = 1;
        }
    }

    totalReceived -= 4; // Remove the 4 bytes of the Remote JC Terminal protocol
    ENSURE_OR_GO_EXIT(totalReceived <= pApdu->rxlen);
    ENSURE_OR_GO_EXIT(totalReceived <= MAX_BUF_SIZE - 4);
    memcpy(pApdu->pBuf, &pRsp[4], totalReceived);
    LOG_MAU8_D("<H", pRsp, 4);
    LOG_MAU8_D("<Rx", pApdu->pBuf, totalReceived);

#ifdef LOG_SOCK
    sm_printf(CONSOLE, "   recv: ");
    for (i = 0; i < totalReceived; i++) {
        sm_printf(CONSOLE, "%02X", pApdu->pBuf[i]);
    }
    sm_printf(CONSOLE, "\n");
#endif

    pApdu->rxlen = (U16)totalReceived;
    // reset offset for subsequent response parsing
    pApdu->offset = 0;
    rv = SMCOM_OK;
exit:
    return rv;
}

U32 smComVCom_TransceiveRaw(void* conn_ctx, U8 *pTx, U16 txLen, U8 *pRx, U32 *pRxLen)
{
    DWORD retval;
    U32 answerReceived = 0;
    U32 len = 0;
    U8 status = 0;
    DWORD WrittenLen = 0;
#if defined(LOG_SOCK) || defined(DBG_LOG_SOCK)
    int i;
#endif
    U32 readOffset = 0;
    U8 headerParsed = 0;
    U8 correctHeader = 0;
    U32 rv = SMCOM_COM_FAILED;
    HANDLE pComHandle = (conn_ctx == NULL) ? gpComHandle : (HANDLE)conn_ctx;

    ENSURE_OR_GO_EXIT(pTx != NULL);
    ENSURE_OR_GO_EXIT(pRx != NULL);
    ENSURE_OR_GO_EXIT(pRxLen != NULL);

    memset(sockapdu, 0x00, MAX_BUF_SIZE);
    memset(response, 0x00, MAX_BUF_SIZE);

    memcpy(pCmd, Header, 2);
    pCmd[2] = (txLen & 0xFF00) >> 8;
    pCmd[3] = (txLen & 0x00FF);
    memcpy(&pCmd[4], pTx, txLen);
    ENSURE_OR_GO_EXIT((UINT16_MAX - 4) >= txLen);
    txLen += 4; /* header + len */

#ifdef DBG_LOG_SOCK
    sm_printf(CONSOLE, "   full send: ");
    for (i = 0; i < txLen; i++) {
        sm_printf(CONSOLE, "%02X", pCmd[i]);
    }
    sm_printf(CONSOLE, "\n");
#endif

    LOG_MAU8_D("H>", pCmd, 4);
    LOG_MAU8_D("Tx>", pCmd + 4, txLen - 4);
    status = WriteFile(pComHandle, pCmd, txLen, &WrittenLen, NULL);
    if ((status == false) || (WrittenLen != txLen)) {
        fprintf(stderr, "Client: send() failed: error %i.\n", WrittenLen);
        return SMCOM_SND_FAILED;
    }
    else {
#ifdef DBG_LOG_SOCK
        sm_printf(CONSOLE, "Client: send() is OK.\n");
#endif
    }

#ifdef LOG_SOCK
    sm_printf(CONSOLE, "   send: ");
    for (i = 4; i < txLen; i++) {
        sm_printf(CONSOLE, "%02X", pCmd[i]);
    }
    sm_printf(CONSOLE, "\n");
#endif

    retval = REMOTE_JC_SHELL_HEADER_LEN; // receive at least the JCTerminal header

    while ((retval > 0) || (answerReceived == 0)) {
        status = ReadFile(pComHandle, (char *)pRsp, MAX_BUF_SIZE, &retval, NULL);

        if (status == 0) {
            return SMCOM_RCV_FAILED;
        }
        else // data received
        {
            if (retval > 4) {
                LOG_MAU8_D("<H", pRsp, 4);
                LOG_MAU8_D("<Rx", pRsp + 4, retval - 4);
            }
            while (retval > 0) // parse all bytes
            {
                if (headerParsed == 1) // header already parsed; get data
                {
                    if (retval >= len) {
                        if (correctHeader == 1) {
                            ENSURE_OR_GO_EXIT(len <= *pRxLen);
                            ENSURE_OR_GO_EXIT(len <= (MAX_BUF_SIZE - readOffset));
                            memcpy(&pRx[0], &pRsp[readOffset], len);
                            answerReceived = 1;
                        }
                        else {
                            // reset header parsed
                            readOffset += len;
                            headerParsed = 0;
                        }
                        retval -= len;

                        if (retval == 0) // no data left, reset readOffset
                        {
                            readOffset = 0;
                        }
                    }
                    else {
                        // data too small according header => Error
                        fprintf(stderr, "Failed reading data %x %x\n", retval, len);
                        return SMCOM_RCV_FAILED;
                    }
                }
                else // parse header
                {
                    len = ((pRsp[readOffset + 2] << 8) | (pRsp[readOffset + 3]));

                    ENSURE_OR_GO_EXIT(readOffset < MAX_BUF_SIZE);
                    if (pRsp[readOffset] == REMOTE_JC_SHELL_MSG_TYPE_APDU_DATA) {
                        // type correct => copy the data
                        retval -= REMOTE_JC_SHELL_HEADER_LEN;
                        if (retval > 0) // data left to read
                        {
                            readOffset += REMOTE_JC_SHELL_HEADER_LEN;
                        }
                        correctHeader = 1;
                    }
                    else {
                        // type incorrect => skip the data as well and try again if data are left
                        readOffset += REMOTE_JC_SHELL_HEADER_LEN;
                        retval -= REMOTE_JC_SHELL_HEADER_LEN;
                        correctHeader = 0;
                    }
                    headerParsed = 1;
                }
            }
        }
    }

#ifdef LOG_SOCK
    sm_printf(CONSOLE, "   recv: ");
    for (i = 0; i < len; i++) {
        sm_printf(CONSOLE, "%02X", pRx[i]);
    }
    sm_printf(CONSOLE, "\n");
#endif

    *pRxLen = len;

    rv = SMCOM_OK;
exit:
    return rv;
}

U32 smComVCom_Close(void* conn_ctx)
{
    U16 status;
    U32 u32status;
    U8 Cmd[4] = {MTY_CLOSE, NAD, 0, 0};
    DWORD WrittenLen = 0;
    U32 totalReceived = 0;
    U8 lengthReceived = 0;
    U32 expectedLength = 0;
    HANDLE pComHandle = (conn_ctx == NULL) ? gpComHandle : (HANDLE)conn_ctx;

    status = WriteFile(pComHandle, Cmd, sizeof(Cmd), &WrittenLen, NULL);
    if ((status == 0) || (WrittenLen != sizeof(Cmd))) {
        return 1;
    }

    expectedLength = REMOTE_JC_SHELL_HEADER_LEN; // remote JC shell header length

    while (totalReceived < expectedLength) {
        U32 maxCommLength;
        DWORD numBytesRead = 0;
        if (lengthReceived == 0) {
            maxCommLength = REMOTE_JC_SHELL_HEADER_LEN - totalReceived;
        }
        else {
            maxCommLength = expectedLength - totalReceived;
        }

        if (maxCommLength > (MAX_BUF_SIZE - totalReceived)) {
            return 1;
        }
        status = ReadFile(pComHandle, (char *)&pRsp[totalReceived], maxCommLength, &numBytesRead, NULL);
        if (status == 0) {
            return 1;
        }
        else {
            if ((UINT32_MAX - totalReceived) < numBytesRead) {
                return 1;
            }
            totalReceived += numBytesRead;
        }
        if ((totalReceived >= REMOTE_JC_SHELL_HEADER_LEN) && (lengthReceived == 0)) {
            if ((UINT32_MAX - expectedLength) < (U32)((pRsp[2] << 8) | (pRsp[3]))) {
                return 1;
            }
            expectedLength += ((pRsp[2] << 8) | (pRsp[3]));
            lengthReceived = 1;
        }
    }
    status = CloseHandle(pComHandle);
    pComHandle = INVALID_HANDLE_VALUE;
    gpComHandle = INVALID_HANDLE_VALUE;
    u32status = GetLastError();
    if (u32status == ERROR_SUCCESS) {
        return SMCOM_OK;
    }
    else {
        LOG_D("GetLastError returned");
        LOG_U32_D(u32status);
        status = (U16)u32status;
        return status;
    }
}
