/**
 * @file smComPCSC.c
 * @author NXP Semiconductors
 * @version 1.0
 * @section LICENSE
 * ----------------------------------------------------------------------------
 *
 * Copyright 2016,2019,2020 NXP
 * SPDX-License-Identifier: Apache-2.0
 * ----------------------------------------------------------------------------
 * @section DESCRIPTION
 * This file implements the SmCom PCSC communication layer.
 * ----------------------------------------------------------------------------
 *
 *****************************************************************************/
#include <assert.h>
#include <stddef.h>

#include "sm_apdu.h"
#include "smComPCSC.h"
#include "sm_printf.h"
#include "nxEnsure.h"
#include <string.h>

#ifdef __OSX_AVAILABLE
#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#elif defined(__linux__)
#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#else
#include <winscard.h>
#endif
//#include <wintypes.h>

#ifdef FLOW_VERBOSE
#define NX_LOG_ENABLE_SMCOM_DEBUG 1
#else
//#define NX_LOG_ENABLE_SMCOM_DEBUG 1
#endif
#include "nxLog_smCom.h"

static U32 smComPCSC_Transceive(void* conn_ctx, apdu_t *pApdu);
static U32 smComPCSC_TransceiveRaw(void* conn_ctx, U8 *pTx, U16 txLen, U8 *pRx, U32 *pRxLen);

static SCARDHANDLE hCard;
static SCARD_IO_REQUEST pioSendPci;

#ifdef FLOW_VERBOSE
#define PCSC_APDU_VERBOSE
#endif

#if defined(__linux__)
/* PC SC Lite has it */
#else
#define pcsc_stringify_error(X) "pcsc_stringify_error"
#endif

U16 smComPCSC_Close(U8 mode)
{
    return SW_OK;
}

U16 smComPCSC_Open(const char *reader_name_in)
{
    LONG rv;
    SCARDCONTEXT hContext;
    DWORD dwReaders, dwActiveProtocol;
    LPTSTR mszReaders;
    LPTSTR selectedReader;
    U16 ret = SMCOM_COM_FAILED;

    rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
    if (rv != SCARD_S_SUCCESS) {
        LOG_E("SCardEstablishContext failed: %x (%s)", rv, pcsc_stringify_error(rv));
        goto exit;
    }

#ifdef SCARD_AUTOALLOCATE
    dwReaders = SCARD_AUTOALLOCATE;

    rv = SCardListReaders(hContext, NULL, (LPTSTR)&mszReaders, &dwReaders);
    if (rv != SCARD_S_SUCCESS) {
        LOG_E("SCardListReaders(..SCARD_AUTOALLOCATE..) failed: %x (%s)", rv, pcsc_stringify_error(rv));
        goto exit;
    }
#else
    rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
    if (rv != SCARD_S_SUCCESS) {
        LOG_E("SCardListReaders(..SCARD_AUTOALLOCATE..) failed: %x (%s)", rv, pcsc_stringify_error(rv));
        goto exit;
    }

    dwReaders = dwReaders * 255;

    mszReaders = calloc(dwReaders, sizeof(char));
    rv = SCardListReaders(hContext, NULL, mszReaders, &dwReaders);
    if (rv != SCARD_S_SUCCESS) {
        LOG_E("SCardListReaders(..SCARD_AUTOALLOCATE..) failed: %x (%s)", rv, pcsc_stringify_error(rv));
        goto exit;
    }
#endif

    if (rv == SCARD_E_NO_READERS_AVAILABLE) {
        LOG_E("Reader is not in groups.");
        goto exit;
    }
    else if (rv == SCARD_S_SUCCESS) {
        LPTSTR pReader;
        // Default to First found reader..
        selectedReader = mszReaders;
        pReader = mszReaders;
        while ('\0' != *pReader) {
            // Display the value.
            // But we only connect to the first reader.
            LOG_I("Found Reader: %s", pReader);
            if (NULL != reader_name_in) {
                if (0 != strstr(pReader, reader_name_in)) {
                    selectedReader = pReader;
                }
            }
            // Advance to the next value.
#ifdef UNICODE
            pReader = pReader + wcslen((wchar_t *)pReader) + 1;
#else
            pReader = pReader + strlen(pReader) + 1;
#endif
        }
    }
    else {
        LOG_E("Failed SCardListReaders");
        goto exit;
    }

    LOG_I("Connecting to reader: %s", selectedReader);
    rv = SCardConnect(
        hContext, selectedReader, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
#ifdef SCARD_AUTOALLOCATE
    LONG rvFree = SCardFreeMemory(hContext, mszReaders);
    if (SCARD_S_SUCCESS != rvFree) {
        LOG_E("Failed SCardFreeMemory");
        goto exit;
    }
#else
    free(mszReaders);
#endif

    if (rv != SCARD_S_SUCCESS) {
        LOG_E("SCardConnect() failed: %x (%s)", rv, pcsc_stringify_error(rv));
        goto exit;
    }

    switch (dwActiveProtocol) {
    case SCARD_PROTOCOL_T0:
        memcpy(&pioSendPci, SCARD_PCI_T0, sizeof(pioSendPci));
        LOG_D("T=0 protocol");
        break;

    case SCARD_PROTOCOL_T1:
        memcpy(&pioSendPci, SCARD_PCI_T1, sizeof(pioSendPci));
        LOG_D("T=1 protocol");
        break;
    }

    return smCom_Init(smComPCSC_Transceive, smComPCSC_TransceiveRaw);
exit:
    return ret;
}

static U32 smComPCSC_Transceive(void* conn_ctx, apdu_t *pApdu)
{
    U32 rxLen = MAX_APDU_BUF_LENGTH;
    U32 status = SMCOM_SND_FAILED;

    ENSURE_OR_GO_EXIT(pApdu != NULL);

    status = smComPCSC_TransceiveRaw(conn_ctx, pApdu->pBuf, pApdu->buflen, pApdu->pBuf, &rxLen);

    pApdu->rxlen = rxLen;
    // reset offset for subsequent response parsing
    pApdu->offset = 0;

exit:
    return status;
}

// #define PCSC_APDU_VERBOSE // Define in Makefile if required
static U32 smComPCSC_TransceiveRaw(void* conn_ctx, U8 *pTx, U16 txLen, U8 *pRx, U32 *pRxLen)
{
    DWORD dwRecvLength;
    LONG rv;
    U32 ret = SMCOM_COM_FAILED;

    ENSURE_OR_GO_EXIT(pRxLen != NULL);
    ENSURE_OR_GO_EXIT(pRx != NULL);
    ENSURE_OR_GO_EXIT(pTx != NULL);

    LOG_AU8_D(pTx, txLen);

    //LPBYTE   pbAttr = NULL;
    //DWORD    cByte = 0;
    //rv = SCardGetAttrib(hCard, SCARD_ATTR_CURRENT_BWT, (LPBYTE)&pbAttr, &cByte);
    //cByte += 10;
    //rv = SCardSetAttrib(hCard, SCARD_ATTR_CURRENT_BWT, (LPBYTE)&pbAttr, cByte);

    dwRecvLength = *pRxLen;
    if (0 == ((txLen + 10) % 64)) {
        /*
            USB 64 byte boundary
            ===================================

            If data block is multiple of 64 bytes ccid expects next block of data and gets stuck in the bulk out.
            Workaround to solve this problem: add 1 extra 0x00 byte to the final command.

            For this to work, the incoming buffer must not be from a constant array, and must have one extra byte at
            the end of the buffer.
        */
        pTx[txLen] = 0x00;
        txLen += 1;
    }

    rv = SCardTransmit(hCard, &pioSendPci, pTx, txLen, NULL, pRx, &dwRecvLength);

    if (rv != SCARD_S_SUCCESS) {
        *pRxLen = 0;
        LOG_E("SCardTransmit() failed: %x (%s)", rv, pcsc_stringify_error(rv));
        return SMCOM_COM_FAILED;
    }
    else {
        *pRxLen = dwRecvLength;
        LOG_AU8_D(pRx, dwRecvLength);
    }

    ret = SMCOM_OK;
exit:
    return ret;
}
