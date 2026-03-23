/**
 * @file smComSocket_linux.c
 * @author NXP Semiconductors
 * @version 1.0
 * @par License
 *
 * Copyright 2016, 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 *
 * @par Description
 *
 */

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <netdb.h>

#include "smCom.h"
#include "smComSocket.h"
#include "sm_printf.h"
#include "sm_types.h"
#include "nxEnsure.h"
#include "sm_timer.h"

#ifdef FLOW_VERBOSE
#define NX_LOG_ENABLE_SMCOM_DEBUG 1
#define LOG_SOCK 1
// #define LOG_FULL_CMD_RSP
#endif

#include "nxLog_smCom.h"

// Enable define of LOG_SOCK to echo APDU cmd/rsp
// #define LOG_SOCK

// Enable define of CHECK_ON_ATR to enable check on returned ATR (don't enable this when using the Smart Card Server ...)
#define CHECK_ON_ATR

#define REMOTE_JC_SHELL_HEADER_LEN (4)
#define REMOTE_JC_SHELL_MSG_TYPE_APDU_DATA (0x01)

#include "sm_apdu.h"

#define MAX_BUF_SIZE (MAX_APDU_BUF_LENGTH)

static U8 Header[2] = {0x01, 0x00};
static U8 sockapdu[MAX_BUF_SIZE];
static U8 response[MAX_BUF_SIZE];

static U8 *pCmd = (U8 *)&sockapdu;
static U8 *pRsp = (U8 *)&response;

#if defined(__OSX_AVAILABLE) || defined(RJCT_VCOM)
#define READ_RECV( FD, PTR, BUFLEN) \
        read((FD), (PTR), (BUFLEN))
#define READ_RECV_STR "read"
#define WRITE_SEND( FD, PTR, BUFLEN) \
        write((FD), (PTR), (BUFLEN))
#define WRITE_SEND_STR "write"
#else
#define READ_RECV( FD, PTR, BUFLEN) \
        recv((FD), (PTR), (BUFLEN), 0)
#define READ_RECV_STR "recv"
#define WRITE_SEND( FD, PTR, BUFLEN) \
        send((FD), (PTR), (BUFLEN), 0)
#define WRITE_SEND_STR "send"
#endif


U32 smComSocket_CloseFD(int fd)
{
    int retval;
    U8 Cmd[4] = {MTY_CLOSE, MYT_DEFAULT_NAD, 0, 0};
    U32 totalReceived = 0;
    U8 lengthReceived = 0;
    U32 expectedLength = 0;

    LOG_D("Closing()");

    retval = WRITE_SEND( fd, Cmd, sizeof(Cmd));
    if (retval < 0) {
        LOG_W("Client: " WRITE_SEND_STR "() failed: error %i", retval);
        return SMCOM_SND_FAILED;
    }

    expectedLength = REMOTE_JC_SHELL_HEADER_LEN; // remote JC shell header length

    while (totalReceived < expectedLength) {
        U32 maxCommLength;
        if (lengthReceived == 0) {
            maxCommLength = REMOTE_JC_SHELL_HEADER_LEN - totalReceived;
        }
        else {
            maxCommLength = expectedLength - totalReceived;
        }

        if(maxCommLength > MAX_BUF_SIZE - totalReceived){
            close(fd);
            return SMCOM_RCV_FAILED;
        }

        retval = READ_RECV(fd, (char *)&pRsp[totalReceived], maxCommLength);
        if (retval <= 0) {
            if (errno == EINTR) {
                //Retry on EINTR error
                continue;
            }
            LOG_W("Client: " READ_RECV_STR "() failed: error %i", retval);
            close(fd);
            return SMCOM_RCV_FAILED;
        }
        else {
            totalReceived += retval;
        }
        if(expectedLength > (UINT32_MAX - ((pRsp[2] << 8) | (pRsp[3])))) {
            close(fd);
            return SMCOM_RCV_FAILED;
        }
        if ((totalReceived >= REMOTE_JC_SHELL_HEADER_LEN) && (lengthReceived == 0)) {
            expectedLength += ((pRsp[2] << 8) | (pRsp[3]));
            lengthReceived = 1;
        }
    }
    retval = totalReceived;

    close(fd);
    return 0;
}

U32 smComSocket_GetATRFD(int fd, U8 *pAtr, U16 *atrLen)
{
    int retval = 0;
    int read_write_len;

    U32 expectedLength = 0;
    U32 totalReceived = 0;
    U8 lengthReceived = 0;

    // wait 256 ms
    U8 ATRCmd[8] = {MTY_WAIT_FOR_CARD, MYT_DEFAULT_NAD, 0, 4, 0, 0, 1, 0};

    ENSURE_OR_GO_EXIT(pAtr != NULL);
    ENSURE_OR_GO_EXIT(atrLen != NULL);

    LOG_MAU8_D("ATRCmd", ATRCmd, sizeof(ATRCmd));

    read_write_len = WRITE_SEND( fd, (const char *)ATRCmd, sizeof(ATRCmd));

    if (read_write_len < 0) {
        LOG_W("Client: " WRITE_SEND_STR "() failed: error %i", read_write_len);
        return 1;
    }

    expectedLength = REMOTE_JC_SHELL_HEADER_LEN; // remote JC shell header length

    while (totalReceived < expectedLength) {
        // U32 maxCommLength;
        // if (lengthReceived == 0) {
        //     maxCommLength = REMOTE_JC_SHELL_HEADER_LEN - totalReceived;
        // }
        // else {
        //     maxCommLength = expectedLength - totalReceived;
        // }

        LOG_D("Enter: " READ_RECV_STR "() ");
        if(expectedLength >  MAX_BUF_SIZE - totalReceived){
            retval = 0;
            *atrLen = 0;
            goto exit;
        }

        read_write_len = READ_RECV(fd, (char *)&pRsp[totalReceived], expectedLength);
        LOG_D("Exit: " READ_RECV_STR "(). read_write_len=%d", read_write_len);
        if (read_write_len <= 0) {
            if (errno == EINTR) {
                //Retry on EINTR error
                continue;
            }
            LOG_W("Client: " READ_RECV_STR "() failed: error %i", retval);
            close(fd);
            retval = 0;
            ENSURE_OR_GO_EXIT(0);
        }
        else {
            totalReceived += read_write_len;
        }

        if(expectedLength > (UINT32_MAX - ((pRsp[2] << 8) | (pRsp[3])))){
            close(fd);
            return SMCOM_RCV_FAILED;
        }
        if ((totalReceived >= REMOTE_JC_SHELL_HEADER_LEN) && (lengthReceived == 0)) {
            expectedLength += ((pRsp[2] << 8) | (pRsp[3]));
            lengthReceived = 1;
        }
    }
    read_write_len = totalReceived;

#ifdef LOG_FULL_CMD_RSP
    LOG_MAU8_D("Rsp:Hdr", pRsp, 4);
#endif

    read_write_len -= 4; // Remove the 4 bytes of the Remote JC Terminal protocol
    if(read_write_len >  *atrLen){
        retval = 0;
        *atrLen = 0;
        goto exit;
    }
    if(read_write_len >  MAX_BUF_SIZE - 4){
        retval = 0;
        *atrLen = 0;
        goto exit;
    }
    memcpy(pAtr, pRsp + 4, read_write_len);

    LOG_MAU8_D("Atr", pAtr, read_write_len);

    *atrLen = (U16)read_write_len;
exit:
    return retval;
}

U32 smComSocket_TransceiveFD(int fd, apdu_t *pApdu)
{
    int retval;

    U32 txLen = 0;
    U32 expectedLength = 0;
    U32 totalReceived = 0;
    U8 lengthReceived = 0;
    U32 rv = SMCOM_SND_FAILED;

    ENSURE_OR_GO_EXIT(pApdu != NULL);

    //pApdu->rxlen = 0;
    // TODO (?): adjustments on Le and Lc for SCP still to be done
    memset(sockapdu, 0x00, MAX_BUF_SIZE);
    memset(response, 0x00, MAX_BUF_SIZE);

    // remote JC Terminal header construction
    txLen = pApdu->buflen;
    memcpy(pCmd, Header, sizeof(Header));
    pCmd[2] = (txLen & 0xFF00) >> 8;
    pCmd[3] = txLen & 0xFF;


    ENSURE_OR_GO_EXIT(pApdu->buflen <= MAX_BUF_SIZE - 4);
    memcpy(&pCmd[4], pApdu->pBuf, pApdu->buflen);
    pApdu->buflen += 4; /* header & length */

#ifdef LOG_FULL_CMD_RSP
    LOG_MAU8_D("Cmd:Hdr", pCmd, 4);
#endif

    LOG_MAU8_D("Cmd", pCmd + 4, pApdu->buflen - 4);

    retval = WRITE_SEND( fd, (const char *)pCmd, pApdu->buflen);
    if (retval < 0) {
        LOG_W("Client: " WRITE_SEND_STR "() failed: error %i", retval);
        return SMCOM_SND_FAILED;
    }

    expectedLength = REMOTE_JC_SHELL_HEADER_LEN; // remote JC shell header length

    while (totalReceived < expectedLength) {
        retval = READ_RECV(fd, (char *)&pRsp[totalReceived], (MAX_BUF_SIZE - totalReceived));

        if (retval <= 0) {
            if (errno == EINTR) {
                //Retry on EINTR error
                continue;
            }
            LOG_W("Client: " READ_RECV_STR "() failed: error %i", retval);
            close(fd);
            rv = SMCOM_RCV_FAILED;
            ENSURE_OR_GO_EXIT(0);
        }
        else {
            totalReceived += retval;
        }
        if(expectedLength > (UINT32_MAX - ((pRsp[2] << 8) | (pRsp[3])))){
            close(fd);
            return SMCOM_RCV_FAILED;
        }
        if ((totalReceived >= REMOTE_JC_SHELL_HEADER_LEN) && (lengthReceived == 0)) {
            expectedLength += ((pRsp[2] << 8) | (pRsp[3]));
            lengthReceived = 1;
        }
    }
    retval = totalReceived;

    retval -= 4; // Remove the 4 bytes of the Remote JC Terminal protocol
    ENSURE_OR_GO_EXIT(retval <= pApdu->rxlen);
    ENSURE_OR_GO_EXIT(retval <= MAX_BUF_SIZE - 4);
    memcpy(pApdu->pBuf, &pRsp[4], retval);

#ifdef LOG_FULL_CMD_RSP
    LOG_MAU8_D("Rsp:Hdr", pRsp, 4);
#endif
    LOG_MAU8_D("Rsp", pApdu->pBuf, retval);

    pApdu->rxlen = (U16)retval;
    // reset offset for subsequent response parsing
    pApdu->offset = 0;
    rv = SMCOM_OK;
exit:
    return rv;
}

U32 smComSocket_TransceiveRawFD(int fd, U8 *pTx, U16 txLen, U8 *pRx, U32 *pRxLen)
{
    int read_write_len;
    U32 expectedLength = 0;
    int lengthReceived = 0;

    U32 totalReceived = 0;
    U32 rv = SMCOM_COM_FAILED;

    ENSURE_OR_GO_EXIT(pTx != NULL);
    ENSURE_OR_GO_EXIT(pRx != NULL);
    ENSURE_OR_GO_EXIT(pRxLen != NULL);

    memset(sockapdu, 0x00, MAX_BUF_SIZE);
    memset(response, 0x00, MAX_BUF_SIZE);

    memcpy(pCmd, Header, 2);
    pCmd[2] = (txLen & 0xFF00) >> 8;
    pCmd[3] = (txLen & 0x00FF);
    ENSURE_OR_GO_EXIT(txLen <= MAX_BUF_SIZE - 4);
    memcpy(&pCmd[4], pTx, txLen);
    txLen += 4; /* header + len */


#ifdef LOG_FULL_CMD_RSP
    LOG_MAU8_D("Cmd:Hdr", pCmd, REMOTE_JC_SHELL_HEADER_LEN);
#endif

    LOG_MAU8_D("Cmd", pCmd + REMOTE_JC_SHELL_HEADER_LEN, txLen - REMOTE_JC_SHELL_HEADER_LEN);

    read_write_len = WRITE_SEND( fd, (const char *)pCmd, txLen);
    if (read_write_len < 0) {
        LOG_W("Client: " WRITE_SEND_STR "() failed: error %i", read_write_len);
        return SMCOM_SND_FAILED;
    }
    else {
#ifdef DBG_LOG_SOCK
        LOG_D("Client: " WRITE_SEND_STR "() is OK.\r\n");
#endif
    }

    expectedLength = REMOTE_JC_SHELL_HEADER_LEN; // remote JC shell header length
    while (totalReceived < expectedLength) {
        read_write_len = READ_RECV(fd, (char *)&pRsp[totalReceived], MAX_BUF_SIZE - totalReceived);

        if (read_write_len <= 0) {
            if (errno == EINTR) {
                //Retry on EINTR error
                continue;
            }
            LOG_W("Client: " READ_RECV_STR "() failed: error %i", read_write_len);
            close(fd);
            rv = SMCOM_RCV_FAILED;
            ENSURE_OR_GO_EXIT(0);
        }
        else {
            totalReceived += read_write_len;
        }
        if(expectedLength > (UINT32_MAX - ((pRsp[2] << 8) | (pRsp[3])))){
            close(fd);
            return SMCOM_RCV_FAILED;
        }
        if ((totalReceived >= REMOTE_JC_SHELL_HEADER_LEN) && (lengthReceived == 0)) {
            expectedLength += ((pRsp[2] << 8) | (pRsp[3]));
            lengthReceived = 1;
        }
    }


#ifdef LOG_FULL_CMD_RSP
    LOG_MAU8_D("Rsp:Hdr", pRsp, REMOTE_JC_SHELL_HEADER_LEN);
#endif

    ENSURE_OR_GO_EXIT((totalReceived - REMOTE_JC_SHELL_HEADER_LEN) <= *pRxLen);
    ENSURE_OR_GO_EXIT((totalReceived - REMOTE_JC_SHELL_HEADER_LEN) <= (MAX_BUF_SIZE - REMOTE_JC_SHELL_HEADER_LEN));
    memcpy(pRx, &pRsp[REMOTE_JC_SHELL_HEADER_LEN], totalReceived - REMOTE_JC_SHELL_HEADER_LEN);
    *pRxLen = totalReceived - REMOTE_JC_SHELL_HEADER_LEN;

    LOG_MAU8_D("Rsp", pRx, *pRxLen);

    rv = SMCOM_OK;
exit:
    return rv;
}



U32 smComSocket_LockChannelFD(int fd)
{
    int retval = 0;
    U32 expectedLength = 0;
    U32 totalReceived = 0;

    // wait 256 ms
    U8 LockCmd[4] = { MTY_LOCK, 0, 0, 0};
    U8 LockRsp[4] = { 0,};

    retval = WRITE_SEND(fd, (const char*)LockCmd, sizeof(LockCmd));
    if (retval < 0)
    {
        if (fprintf(stderr, "Client: send() failed: error %i.\n", retval) < 0) {
            LOG_E("fprintf error");
        }
        return 0;
    }

    expectedLength = REMOTE_JC_SHELL_HEADER_LEN; // remote JC shell header length

    while (totalReceived < expectedLength)
    {
        U32 maxCommLength;
        maxCommLength = expectedLength - totalReceived;

        ENSURE_OR_GO_EXIT(maxCommLength <= sizeof(LockRsp) - totalReceived);
        retval = READ_RECV(fd, (char*)&LockRsp[totalReceived], maxCommLength);
        if (retval <= 0)
        {
            if (errno == EINTR) {
                //Retry on EINTR error
                continue;
            }
            if (fprintf(stderr, "Client: recv() failed: error %i.\n", retval) < 0) {
                LOG_E("fprintf error");
            }
            close(fd);
            retval = 0;
            goto exit;
        }
        else
        {
            totalReceived += retval;
        }
    }
    retval = LockRsp[3];

exit:
    return retval;
}

U32 smComSocket_UnlockChannelFD(int fd)
{
    int retval = 0;
    U32 expectedLength = 0;
    U32 totalReceived = 0;

    // wait 256 ms
    U8 UnlockCmd[4] = { MTY_UNLOCK, 0, 0, 0};
    U8 UnlockRsp[4] = { 0,};

    retval = WRITE_SEND(fd, (const char*)UnlockCmd, sizeof(UnlockCmd));
    if (retval < 0)
    {
        if (fprintf(stderr, "Client: send() failed: error %i.\n", retval) < 0) {
            LOG_E("fprintf error");
        }
        return 0;
    }

    expectedLength = REMOTE_JC_SHELL_HEADER_LEN; // remote JC shell header length

    while (totalReceived < expectedLength)
    {
        U32 maxCommLength;
        maxCommLength = expectedLength - totalReceived;

        ENSURE_OR_GO_EXIT(maxCommLength <= sizeof(UnlockRsp) - totalReceived);
        retval = READ_RECV(fd, (char*)&UnlockRsp[totalReceived], maxCommLength);
        if (retval <= 0)
        {
            if (errno == EINTR) {
                //Retry on EINTR error
                continue;
            }
            if (fprintf(stderr, "Client: recv() failed: error %i.\n", retval) < 0) {
                LOG_E("fprintf error");
            }
            close(fd);
            retval = 0;
            goto exit;
        }
        else
        {
            totalReceived += retval;
        }
    }
    retval = UnlockRsp[3];

exit:
    return retval;
}
