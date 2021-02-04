/**
 * @file smComSCI2c.c
 * @author NXP Semiconductors
 * @version 1.0
 * @par License
 *
 * Copyright 2016,2020 NXP
 * SPDX-License-Identifier: Apache-2.0
 *
 * @par Description
 * This file implements the SmCom JRCP-V2 communication layer.
 *
 *****************************************************************************/

#include <assert.h>
#include <stdio.h>
#include "smComJRCP.h"
#include "sm_printf.h"
#include "sm_apdu.h"
#include <string.h>
#include "ex_sss_ports.h"

#ifdef FLOW_VERBOSE
#   define NX_LOG_ENABLE_SMCOM_DEBUG 1
#endif
// #define NX_LOG_ENABLE_SMCOM_DEBUG 1

#include <nxLog_smCom.h>
#include "nxEnsure.h"

#if defined(SMCOM_JRCP_V2)
#include <jrcplib/networking/tcpclient.h>

static U32 smComJRCP_Transceive(void* conn_ctx, apdu_t * pApdu);

static U32 smComJRCP_TransceiveRaw(void* conn_ctx, U8 * pTx, U16 txLen, U8 * pRx, U32 * pRxLen);

static  jrcplib_tcpclient_ctx_t* gctx = NULL;

U16 smComJRCP_Close(void* conn_ctx, U8 mode)
{
    jrcplib_tcpclient_ctx_t* ctx = (conn_ctx == NULL) ? gctx : (jrcplib_tcpclient_ctx_t*)conn_ctx;
    if (ctx != NULL)
    {
        if (gctx == ctx) {
            gctx = NULL;
        }
        jrcplib_tcpclient_disconnect(ctx);
        jrcplib_tcpclient_free(ctx);
        ctx = NULL;
    }

    return SW_OK;
}

//
// smComSCI2C_Init is deprecated, please use smComSCI2C_Open
//
#ifndef TGT_A71CH
void smComJRCP_Init()
{
    smCom_Init(&smComJRCP_Transceive, &smComJRCP_TransceiveRaw);
}
#endif

U16 smComJRCP_Open(void** conn_ctx, const char *hostname_in, unsigned int portnum_in)
{
    int32_t ret = 0;

    const char *hostname_env;
    const char *hostname_selected;
    char szHostname[70] = {0};
    unsigned int portnum = 0;

    const char *hostname_default = EX_SSS_BOOT_SSS_SOCKET_HOSTNAME_DEFAULT;
    const unsigned int portnum_default = EX_SSS_BOOT_SSS_SOCKET_PORTNUMBER_DEFAULT;

    jrcplib_tcpclient_ctx_t* ctx = NULL;

    /* First priority Environment variable */
    hostname_env = getenv("JRCP_HOSTNAME");

    if (NULL == hostname_env) {
        /* No ENV */
        if (hostname_in == NULL) {
            /* No value from CLI : use default */
            hostname_selected = hostname_default;
            LOG_I("Hostname taken from default '%s'. Set 'JRCP_HOSTNAME' to override.", hostname_default);
        }
        else {
            /* Take from CLI */
            hostname_selected = hostname_in;
        }
    }
    else
    {
        strncpy(szHostname, hostname_env, sizeof(szHostname) - 1);
        hostname_selected = szHostname;
    }

    /* First priority Environment variable */
    const char *portnum_str_env = getenv("JRCP_PORT");
    if (portnum_str_env != NULL) {
        /* ENV has it, use that*/
        portnum = strtol(portnum_str_env, NULL, 10);
    }

    if (0 == portnum) {
        /* No ENV */
        if (0 == portnum_in) {
            /* CLI didn't have it, env  didn't have it, use default*/
            portnum = portnum_default;
            LOG_I("Port number taken from default. Set 'JRCP_PORT' to override.");
        }
        else {
            portnum = portnum_in;
        }
    }

    LOG_I("Opening connection to JRCP server on %s:%d", hostname_selected, portnum);
    ret = jrcplib_tcpclient_init(&ctx);
    if (ret < 0) {
        LOG_E("%s", jrcplib_get_error_string(ret));
        smComJRCP_Close(ctx, 0);
        return SMCOM_COM_FAILED;
    }

    ret = jrcplib_tcpclient_connect_to_default_node(ctx, hostname_selected, (uint16_t)portnum);
    if (ret < 0) {
        LOG_E("%s", jrcplib_get_error_string(ret));
        smComJRCP_Close(ctx, 0);
        return SMCOM_COM_FAILED;
    }

    uint32_t nodes_count;
    jrcplib_nad_t *nodes = NULL;
    ret = jrcplib_tcpclient_list_nodes(ctx, &nodes_count, &nodes);
    if (ret < 0) {
        LOG_E("%s", jrcplib_get_error_string(ret));
        smComJRCP_Close(ctx, 0);
        return SMCOM_COM_FAILED;
    }

    /*int i = 0;
    for (; i < nodes_count; i++) {
        printf("Node: NAD: %d, Name: %s\n", nodes[i].nad, nodes[i].description);
    }*/

    ret = jrcplib_tcpclient_list_nodes_cleanup(nodes_count, nodes);
    if (ret < 0) {
        LOG_E("%s", jrcplib_get_error_string(ret));
        smComJRCP_Close(ctx, 0);
        return SMCOM_COM_FAILED;
    }

    ret = jrcplib_tcpclient_reset(ctx, ColdReset);
    if (ret < 0) {
        LOG_E("%s", jrcplib_get_error_string(ret));
        smComJRCP_Close(ctx, 0);
        return SMCOM_COM_FAILED;
    }

    LOG_D("Opening connection to JRCP successful");
    if (conn_ctx == NULL) {
        gctx = ctx;
    }
    else {
        gctx = ctx;
        *conn_ctx = ctx;
    }
    return smCom_Init(&smComJRCP_Transceive, &smComJRCP_TransceiveRaw);
}

U16 smComJRCP_Reset(void* conn_ctx, uint32_t instruction_bytes)
{
    int32_t ret = 0;
    jrcplib_tcpclient_ctx_t* ctx = (conn_ctx == NULL) ? gctx : (jrcplib_tcpclient_ctx_t*)conn_ctx;
    ret = jrcplib_tcpclient_prepare_reset(ctx, ColdReset, instruction_bytes);
    if (ret < 0) {
        LOG_E("%s", jrcplib_get_error_string(ret));
        smComJRCP_Close(ctx, 0);
        return SMCOM_COM_FAILED;
    }

    LOG_D("Resetting JRCP successful");
    return SMCOM_OK;
}

static U32 smComJRCP_Transceive(void* conn_ctx, apdu_t * pApdu)
{
    U32 respLen = MAX_APDU_BUF_LENGTH;
    U32 retCode = SMCOM_COM_FAILED;
    jrcplib_tcpclient_ctx_t* ctx = (conn_ctx == NULL) ? gctx : (jrcplib_tcpclient_ctx_t*)conn_ctx;

    ENSURE_OR_GO_EXIT(pApdu != NULL);

    retCode = smComJRCP_TransceiveRaw(ctx, (U8 *)pApdu->pBuf, pApdu->buflen, pApdu->pBuf, &respLen);

    pApdu->rxlen = (U16)respLen;
exit:
    return retCode;
}

static U32 smComJRCP_TransceiveRaw(void* conn_ctx, U8 * pTx, U16 txLen, U8 * pRx, U32 * pRxLen)
{
    size_t  resp_len = *pRxLen;
    jrcplib_tcpclient_ctx_t* ctx = (conn_ctx == NULL) ? gctx : (jrcplib_tcpclient_ctx_t*)conn_ctx;

    if (ctx == NULL) {
        LOG_W("ctx==NULL");
        return SMCOM_COM_FAILED;
    }

    LOG_MAU8_D("Tx>", pTx, txLen);
    int32_t ret = jrcplib_tcpclient_dataexchange(ctx, txLen, pTx, *pRxLen, pRx, &resp_len);
    *pRxLen = (U32)resp_len;
    if (ret < 0) {
        LOG_E("%s", jrcplib_get_error_string(ret));
        return SMCOM_RCV_FAILED;
    }

    LOG_MAU8_D("<Rx", pRx, resp_len);

    return SMCOM_OK;
}

void smComJRCP_Echo(void* conn_ctx, const char *comp, const char *level, const char *buffer)
{
    char print_buffer[1024];
    jrcplib_tcpclient_ctx_t* ctx = (conn_ctx == NULL) ? gctx : (jrcplib_tcpclient_ctx_t*)conn_ctx;

    if (ctx == NULL) {
        return;
    }
    snprintf(print_buffer, sizeof(print_buffer) / sizeof(print_buffer[0]) - 1, "%s:%s:%s", comp, level, buffer);

    jrcplib_tcpclient_echo(ctx, print_buffer);
}

U32 smComJRCP_NvmCount(void* conn_ctx, U32 * pCount)
{
    size_t resp_len = 0;
    uint8_t rxBuf[MAX_APDU_BUF_LENGTH];
    jrcplib_tcpclient_ctx_t* ctx = (conn_ctx == NULL) ? gctx : (jrcplib_tcpclient_ctx_t*)conn_ctx;

    if(pCount == NULL){
        LOG_W("pCount==NULL");
        return SMCOM_RCV_FAILED;
    }
    if (ctx == NULL) {
        LOG_W("ctx==NULL");
        return SMCOM_COM_FAILED;
    }
    memset(rxBuf, 0x00, sizeof(rxBuf));
    int32_t ret = jrcplib_tcpclient_nvmCount(ctx, &resp_len, rxBuf);
    if (ret < 0) {
        LOG_E("%s", jrcplib_get_error_string(ret));
        return SMCOM_RCV_FAILED;
    }

    *pCount = ((rxBuf[resp_len - 4] << 8 * 3) | (rxBuf[resp_len - 3] << 8 * 2) | (rxBuf[resp_len - 2] << 8 * 1)
                    | (rxBuf[resp_len - 1]));

    LOG_MAU8_D("<Rx", rxBuf, resp_len);

    return SMCOM_OK;
}

#endif // defined(SMCOM_JRCP_V2)
