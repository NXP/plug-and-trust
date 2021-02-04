/* Copyright 2019,2020 NXP
 *
 * This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms.  By expressly
 * accepting such terms or by downloading, installing, activating and/or
 * otherwise using the software, you are agreeing that you have read, and
 * that you agree to comply with and are bound by, such license terms.  If
 * you do not agree to be bound by the applicable license terms, then you
 * may not retain, install, activate or otherwise use the software.
 */
#include <jrcplib/networking/tcpclient.h>
#include <jrcplib/errors.h>
#include "../../include/utils.h"
#include <jrcplib/messages.h>

#include <cstdint>
#include <cstring>


#ifndef _WIN32
#    define closesocket(a) close(a)
#    define SOCKADDR sockaddr
#endif

#define JRCP_PREAMBLE_SIZE    4
#define JRCP_PAYLOAD_LEN_SIZE 4

using namespace jrcplib::utils;


static int receiveFromSocket(SOCKET s, uint8_t *buffer, uint32_t expected_len) {
    int n = -1;
    uint32_t received_len = 0;
    do {
        n = recv(s, (char *)buffer, expected_len, 0);
        if (n <= 0) {
            return n;
        }
        received_len += n;
    } while (received_len < expected_len);
    return received_len;
}


namespace jrcplib {
    namespace networking {
        JrcpTcpClientContext::JrcpTcpClientContext() { }

        int32_t JrcpTcpClientContext::Init()
        {
#ifdef _WIN32
            WSAData wsaData;

            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                return JRCPLIB_ERR_NETWORKING_INIT_FAILED;
            }
#endif

            return JRCPLIB_ERR_OK;
        }


        int32_t JrcpTcpClientContext::CleanUp()
        {
            Disconnect();
#ifdef _WIN32
            if (WSACleanup() != 0) {
                return JRCPLIB_ERR_NETWORKING_CLEANUP_FAILED;
            }
#endif

            return JRCPLIB_ERR_OK;
        }

        int32_t JrcpTcpClientContext::Connect(std::string ip, uint16_t port)
        {
            int32_t ret = ConnectSocket(ip, port);
            if (ret < 0) {
                return ret;
            }
            this->current_nad_ = GetMinNad();
            return JRCPLIB_ERR_OK;
        }


        int32_t JrcpTcpClientContext::Connect(std::string ip, uint16_t port, std::string nodestr)
        {
            int32_t ret = ConnectSocket(ip, port);
            if (ret < 0) {
                return ret;
            }
            ret = ConnectDevice(nodestr);
            if (ret < 0) {
                return ret;
            }

            uint8_t nad = 0;
            ret = GetNad(nodestr, &nad);
            if (ret < 0) {
                return ret;
            }
            this->current_nad_ = nad;
            return JRCPLIB_ERR_OK;
        }

        int32_t JrcpTcpClientContext::Disconnect()
        {
            if (TcpClientConnectionStatus::TcpClientConnected == connection_status_) {
                closesocket(sock_);
                sock_ = INVALID_SOCKET;
                connection_status_ = TcpClientConnectionStatus::TcpClientDisconnected;
            }
            return JRCPLIB_ERR_OK;
        }

        int32_t JrcpTcpClientContext::GetNodesList(std::map<uint8_t, std::string> &nodes)
        {
            int32_t ret = RetrieveNodesList();
            if (JRCPLIB_ERR_OK != ret) {
                return ret;
            }

            nodes = this->nodesmap_;

            return JRCPLIB_ERR_OK;
        }


        int32_t JrcpTcpClientContext::ConnectSocket(std::string &ip, uint16_t port)
        {
            server_ip_str_ = ip;
            server_port_ = port;

            memset(&serveraddr_, 0, sizeof(struct sockaddr_in));
            serveraddr_.sin_family = AF_INET;
#pragma warning(push)
#pragma warning(disable:4996)
            serveraddr_.sin_addr.s_addr = inet_addr(server_ip_str_.c_str());
#pragma warning(pop)
            serveraddr_.sin_port = htons(server_port_);

            if (INVALID_SOCKET != sock_) {
                closesocket(sock_);
                sock_ = INVALID_SOCKET;
            }

            sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (INVALID_SOCKET == sock_) {
                return JRCPLIB_ERR_NETWORKING_SOCKET_CREATION_FAILED;
            }

            if (connect(sock_, (SOCKADDR *)&serveraddr_, sizeof(serveraddr_)) == SOCKET_ERROR) {
                return JRCPLIB_ERR_NETWORKING_SOCKET_CONNECTION_FAILED;
            }

            connection_status_ = TcpClientConnectionStatus::TcpClientConnected;

            if (JRCPLIB_ERR_OK != RetrieveTerminalInfo()) {
                Disconnect();
                return JRCPLIB_ERR_NETWORKING_SOCKET_CONNECTION_FAILED;
            }

            int32_t ret = RetrieveNodesList();
            if (JRCPLIB_ERR_OK != ret) {
                return ret;
            }

            return JRCPLIB_ERR_OK;
        }


        int32_t JrcpTcpClientContext::RetrieveTerminalInfo()
        {
            BytesVec data;
            if (connection_status_ == TcpClientConnectionStatus::TcpClientDisconnected) {
                return JRCPLIB_ERR_NETWORKING_NO_ACTIVE_CONNECTION;
            }
            jrcplib::BytesVec payload;
            JrcpTerminalInfoReqMessage reqmsg(0xFF); // Controller NAD
            reqmsg.SetJrcpV1Format(true);

            int32_t ret = SendToSocket(reqmsg.GetRawMessage());
            if (ret < 0) {
                return ret;
            }

            ret = RecvJrcpMessage(data);
            if (ret < 0) {
                return ret;
            }

           BytesVec v2_server_resp = { 0x04, 0xFF, 0x00, 0x10, 0x4A, 0x52, 0x43, 0x50, 0x20, 0x50,
                                       0x72, 0x6F, 0x74, 0x6F, 0x63, 0x6F, 0x6C, 0x20, 0x32, 0x2B };

           /* V1 server is not supported */
           if (data != v2_server_resp) {
               return JRCPLIB_ERR_MALFORMED_MESSAGE;
           }

            return JRCPLIB_ERR_OK;

        }

        int32_t JrcpTcpClientContext::RetrieveNodesList()
        {
            BytesVec data;
            if (connection_status_ == TcpClientConnectionStatus::TcpClientDisconnected) {
                return JRCPLIB_ERR_NETWORKING_NO_ACTIVE_CONNECTION;
            }
            jrcplib::BytesVec payload;
            JrcpControllerReqMessage ctrlReqMsg(ConfigurationType::ListReaders, payload);

            int32_t ret = SendToSocket(ctrlReqMsg.GetRawMessage());
            if (ret < 0) {
                return ret;
            }

            ret = RecvJrcpMessage(data);
            if (ret < 0) {
                return ret;
            }

            JrcpControllerRespMessage msg(data);

            if (ConfigurationType::ListReaders != msg.GetConfigurationType()) {
                return JRCPLIB_ERR_MALFORMED_MESSAGE;
            }


            nodesmap_.clear();
            payload = msg.GetPayload();
            uint32_t idx = 0;
            while (idx < payload.size()) {
                std::vector<uint8_t>::const_iterator first = payload.begin() + idx + 2;
                std::vector<uint8_t>::const_iterator last  = payload.begin() + idx + 2 + payload[idx + 1];
                std::string nodename(first, last);
                nodesmap_[payload[idx]] = nodename;
                idx += 2 + payload[idx + 1];
            }

            return JRCPLIB_ERR_OK;
        }

        int32_t JrcpTcpClientContext::ConnectDevice(std::string &nodestr)
        {
            BytesVec data;

            if (connection_status_ == TcpClientConnectionStatus::TcpClientDisconnected) {
                return JRCPLIB_ERR_NETWORKING_NO_ACTIVE_CONNECTION;
            }

            RetrieveNodesList();
            // Verify that node value is connectable
            for (const auto &pair : nodesmap_) {
                if (nodestr.compare(pair.second) == 0 && pair.first <= 0x7F) {
                    return JRCPLIB_ERR_INVALID_ARGUMENT;
                }
            }


            jrcplib::BytesVec payload;
            payload.assign(nodestr.begin(), nodestr.end());
            JrcpControllerReqMessage *ctrlReqMsg = new JrcpControllerReqMessage(ConfigurationType::ConnectTerminal, payload);

            int32_t ret = SendToSocket(ctrlReqMsg->GetRawMessage());
            if (ret < 0) {
                return ret;
            }

            ret = RecvJrcpMessage(data);
            if (ret < 0) {
                return ret;
            }

            JrcpControllerRespMessage *msg = new JrcpControllerRespMessage(data);

            if (ConfigurationType::ConnectTerminal != msg->GetConfigurationType()) {
                return JRCPLIB_ERR_MALFORMED_MESSAGE;
            }

            return JRCPLIB_ERR_OK;
        }


        int32_t JrcpTcpClientContext::SendToSocket(const jrcplib::BytesVec& data) const
        {
            if (TcpClientConnectionStatus::TcpClientDisconnected == connection_status_) {
                return JRCPLIB_ERR_NETWORKING_NO_ACTIVE_CONNECTION;
            }

            if (send(sock_, (const char*) data.data(), (int)data.size(), 0) < 0) {
                fprintf(stderr, "Error sending!!!\n");
                return JRCPLIB_ERR_NETWORKING_SOCKET_SEND_FAILED;
            }

            //fprintf(stdout, "=> Sent to socket: ");
            //for (int i = 0; i < data.size(); i++)
            //{
            //    fprintf(stdout, "%02X", data[i]);
            //}
            //fprintf(stdout, "\n");

            return JRCPLIB_ERR_OK;
        }

        int32_t JrcpTcpClientContext::RecvJrcpMessage(BytesVec& data)
        {
            uint32_t offset = 0;
            // 4096 bytes will be sufficient for most cases.
            // If message will be bigger 4096, chaining will be used
            uint8_t  buffer[4096] = { 0 };
            int      ret = 0;

            data.clear();

            /* Receive preamble */
            if ((ret = receiveFromSocket(sock_, buffer + offset, JRCP_PREAMBLE_SIZE)) <= 0) {
                return JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED;
            }
            offset += ret;

            if (buffer[0] != kJrcpMessageSofValue) { // If JRCP v1
                // The only supported V1 message is Terminal Info, hence only LNL is sufficient. Receive remaining part.
                if ((ret = receiveFromSocket(sock_, buffer + offset, buffer[kJrcpV1MessageLnlIndex])) <= 0) {
                    return JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED;
                }
                offset += ret;
                data.assign(buffer, buffer + offset);
            }
            else { // If JRCP v2 message
                /* Receive header */
                if (buffer[kJrcpMessageHdlIndex] != 0) {
                    if ((ret = receiveFromSocket(sock_, buffer + offset, buffer[kJrcpMessageHdlIndex])) <= 0) {
                        return JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED;
                    }
                    offset += ret;
                }

                /* Receive payload lengh */
                uint32_t payload_len = 0;
                if ((ret = receiveFromSocket(sock_, (uint8_t *)&payload_len, JRCP_PAYLOAD_LEN_SIZE)) <= 0) {
                    return JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED;
                }

                memcpy(buffer + offset, (uint8_t *)&payload_len, JRCP_PAYLOAD_LEN_SIZE);
                offset += ret;
                payload_len = SwapEndian<uint32_t>(payload_len);

                /* Receive payload */
                if (payload_len < sizeof(buffer)) {
                    if ((ret = receiveFromSocket(sock_, buffer + offset, payload_len)) <= 0) {
                        return JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED;
                    }
                    offset += ret;
                    data.assign(buffer, buffer + offset);
                }
                else { // Payload is bigger then buffer size, we need to chain
                    // add what we have now
                    data.assign(buffer, buffer + offset);
                    // receive by chunks of sizeof(buffer)
                    for (uint32_t i = 0; i < payload_len / sizeof(buffer); i++) {
                        memset(buffer, 0, sizeof(buffer));
                        if ((ret = receiveFromSocket(sock_, buffer, sizeof(buffer))) <= 0) {
                            return JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED;
                        }
                        data.insert(data.end(), buffer, buffer + sizeof(buffer));
                    }

                    memset(buffer, 0, sizeof(buffer));
                    if ((ret = receiveFromSocket(sock_, buffer, payload_len % sizeof(buffer))) <= 0) {
                        return JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED;
                    }
                    data.insert(data.end(), buffer, buffer + (payload_len % sizeof(buffer)));
                }

                // Receive TIL
                uint8_t  jrcpTIL[20] = { 0 };
                offset = 0;
                if ((ret = receiveFromSocket(sock_, jrcpTIL + offset, 1)) <= 0) { // TIL is always 1 byte in Request message
                    return JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED;
                }
                offset += ret;
                if ( (jrcpTIL[0] + offset) > sizeof(jrcpTIL) ) {
                    // More than what we can read
                    return JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED;
                }
                else if (jrcpTIL[0] > 0) {
                    // Receive timing information trailer
                    if ((ret = receiveFromSocket(sock_, jrcpTIL + offset, jrcpTIL[0])) <= 0) {
                        return JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED;
                    }
                    offset += ret;
                }
            }

            //fprintf(stdout, "<= Received from socket: ");
            //for (int i = 0; i < data.size(); i++)
            //{
            //    fprintf(stdout, "%02X", data[i]);
            //}
            //fprintf(stdout, "\n");

            return JRCPLIB_ERR_OK;
        }
    } // namespace networking
} // namespace jrcplib


  /**
  * Creates instance of TCP Client and initialize context.
  *
  * @param[out]  outctx Pointer to jrcplib_tcpclient_ctx_t. Will be initialized.
  *
  * @return An error code.
  * @retval JRCPLIB_ERR_OK Everything went fine.
  * @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff assignment to output pointer fails.
  * @retval JRCPLIB_ERR_OUT_OF_MEMORY iff allocation failed.
  */
JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_init(jrcplib_tcpclient_ctx_t **outctx)
{
    jrcplib::networking::JrcpTcpClientContext *ctx = new jrcplib::networking::JrcpTcpClientContext();
    if (nullptr != ctx) {
        ctx->Init();
        return TryAssignOutParameter(outctx, ctx);
    }

    return JRCPLIB_ERR_OUT_OF_MEMORY;
}

JRCPLIB_EXPORT_API void jrcplib_tcpclient_free(jrcplib_tcpclient_ctx_t *ctx) {
    if (nullptr != ctx) {
        delete ctx;
    }
}

/**
* Cleanup instance of TCP Client and context. It will disconnect active connection if needed.
*
* @param[in]  ctx - Pointer to jrcplib_tcpclient_ctx_t.
*
* @return An error code.
* @retval JRCPLIB_ERR_OK Everything went fine.
* @retval JRCPLIB_ERR_NETWORKING_CLEANUP_FAILED iff cleanup fails (only on windows platform).
*/
JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_cleanup(jrcplib_tcpclient_ctx_t *ctx)
{
    return ctx->CleanUp();
}


/**
* Retrieve list of nodes from JRCP Server.
*
* @param[in]  ctx - Pointer to jrcplib_tcpclient_ctx_t.
* @param[out] nodes_count - Number of nodes in nodes_list
* @param[out] nodes_list - list of nodes. Each node contains NAD and its ASCII description.
*
* @return An error code.
* @retval JRCPLIB_ERR_OK Everything went fine.
* @retval JRCPLIB_ERR_NETWORKING_NO_ACTIVE_CONNECTION - iff no active connection.
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_SEND_FAILED - iff sending to socket fails
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED - iff receiving from socket fails
*/
JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_list_nodes
(
    jrcplib_tcpclient_ctx_t *ctx,
    uint32_t *nodes_count,
    jrcplib_nad_t **nodes_list
)
{
    std::map<uint8_t, std::string> nodes;
    int32_t ret = ctx->GetNodesList(nodes);
    if (JRCPLIB_ERR_OK != ret) {
        *nodes_count = 0;
        *nodes_list = nullptr;
        return ret;
    }

    *nodes_count = (uint32_t) nodes.size();
    *nodes_list = (jrcplib_nad_t*) calloc(1, nodes.size() * sizeof(jrcplib_nad_t));
    int idx = 0;
    for (const auto &pair : nodes) {
        (*nodes_list)[idx].nad = pair.first;
        (*nodes_list)[idx].description = (char *) calloc(1, pair.second.size() + 1); // +1 for '\0' termination
        strncpy((*nodes_list)[idx].description, pair.second.c_str(), pair.second.size());
        idx++;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_list_nodes_cleanup(
    uint32_t nodes_count,
    jrcplib_nad_t *nodes_list
    )
{
    uint32_t idx;
    for (idx = 0; idx < nodes_count; idx++)
    {
        if (NULL != nodes_list[idx].description)
        {
            free(nodes_list[idx].description);
            nodes_list[idx].description = NULL;
        }
    }
    free(nodes_list);
    nodes_list = nullptr;
    return JRCPLIB_ERR_OK;
}


/**
* Connect to default node on give ip and port.
*
* @param[in] ctx - Pointer to jrcplib_tcpclient_ctx_t.
* @param[in] ip - IP address of JRCP Server
* @param[in] port - Port number of JRCP Server
*
* @return An error code.
* @retval JRCPLIB_ERR_OK Everything went fine.
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_CONNECTION_FAILED - iff socket connection fails
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_SEND_FAILED - iff sending to socket fails
# @retval JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED - iff receiving from socket fails
*/
JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_connect_to_default_node(
    jrcplib_tcpclient_ctx_t *ctx,
    const char* ip,
    uint16_t port)
{
    return ctx->Connect(ip, port);
}

/**
* Connect to given node, ip and port.
*
* @param[in] ctx - Pointer to jrcplib_tcpclient_ctx_t.
* @param[in] ip - IP address of JRCP Server
* @param[in] port - Port number of JRCP Server
* @param[in] nodestr - ASCII string of node
*
* @return An error code.
* @retval JRCPLIB_ERR_OK Everything went fine.
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_CONNECTION_FAILED - iff socket connection fails
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_SEND_FAILED - iff sending to socket fails
# @retval JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED - iff receiving from socket fails
*/
JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_connect_to_node(jrcplib_tcpclient_ctx_t *ctx, const char* ip, uint16_t port, const char* nodestr)
{
    return ctx->Connect(ip, port, nodestr);
}


/**
* Disconnect from JRCP Server.
*
* @param[in] ctx - Pointer to jrcplib_tcpclient_ctx_t.
*
* @return An error code.
* @retval JRCPLIB_ERR_OK Everything went fine.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_disconnect(jrcplib_tcpclient_ctx_t *ctx)
{
    return ctx->Disconnect();
}

/**
* Send data to JRCP Server using SEND_DATA message and retrieve back result.
*
* @param[in] ctx - Pointer to jrcplib_tcpclient_ctx_t.
* @param[in] inbuflen - Length of inbuf
* @param[in] inbuf - Buffer with data to send
* @param[in] outbuflen - Length of outbuf
* @param[in] outbuf - Buffer with response data
* @param[out] resp_len - Length of response data in outbuf
*
* @return An error code.
* @retval JRCPLIB_ERR_OK Everything went fine.
* @retval JRCPLIB_ERR_NETWORKING_NO_ACTIVE_CONNECTION - iff no active connection.
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_SEND_FAILED - iff sending to socket fails
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED - iff receiving from socket fails
* @retval JRCPLIB_ERR_INSUFFICIENT_BUFFER - iff outbuf length is not sufficient to store response
*/
JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_dataexchange(jrcplib_tcpclient_ctx_t *ctx, size_t inbuflen, uint8_t *inbuf, size_t outbuflen, uint8_t *outbuf, size_t *resp_len)
{
    jrcplib::BytesVec payload(inbuf, inbuf + inbuflen);
    jrcplib::JrcpSendDataMessage msg(ctx->GetCurrentNad(), payload);

    int32_t ret = ctx->SendToSocket(msg.GetRawMessage());
    if (ret < 0) { return ret; }

    payload.clear();

    jrcplib::BytesVec data;
    ret = ctx->RecvJrcpMessage(data);
    if (ret < 0) { return ret; }

    jrcplib::JrcpSendDataMessage respmsg(data);
    jrcplib::BytesVec resp_payload = respmsg.GetPayload();


    if (resp_payload.size() > outbuflen)

    {
        return JRCPLIB_ERR_INSUFFICIENT_BUFFER;
    }

    memcpy(outbuf, resp_payload.data(), resp_payload.size());
    *resp_len = resp_payload.size();

    return JRCPLIB_ERR_OK;
}


/**
* Send reset message to JRCP Server.
*
* @param[in] ctx - Pointer to jrcplib_tcpclient_ctx_t.
* @param[in] reset_type - Type of reset (ColdReset or WarmReset)
*
* @return An error code.
* @retval JRCPLIB_ERR_OK Everything went fine.
* @retval JRCPLIB_ERR_NETWORKING_NO_ACTIVE_CONNECTION - iff no active connection.
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_SEND_FAILED - iff sending to socket fails
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED - iff receiving from socket fails
* @retval JRCPLIB_ERR_INVALID_ARGUMENT - iff reset type is unknown
*/
JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_reset(jrcplib_tcpclient_ctx_t *ctx, jrcplib_reset_type_t reset_type)
{
    return jrcplib_tcpclient_prepare_reset(ctx, reset_type, 0); // reset right away
}

/**
* Send prepare for reset message to JRCP Server after number of instruction bytes
*
* @param[in] ctx - Pointer to jrcplib_tcpclient_ctx_t.
* @param[in] reset_type - Type of reset (ColdReset or WarmReset)
* @param[in] instruction_bytes - No of instruction bytes to reset after
*
* @return An error code.
* @retval JRCPLIB_ERR_OK Everything went fine.
* @retval JRCPLIB_ERR_NETWORKING_NO_ACTIVE_CONNECTION - iff no active connection.
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_SEND_FAILED - iff sending to socket fails
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED - iff receiving from socket fails
* @retval JRCPLIB_ERR_INVALID_ARGUMENT - iff reset type is unknown
*/
JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_prepare_reset(jrcplib_tcpclient_ctx_t *ctx, jrcplib_reset_type_t reset_type, uint32_t instruction_bytes)
{
    switch(reset_type) {
    case ResetType::ColdReset:
    {
        jrcplib::JrcpColdResetReqMessage req_msg(ctx->GetCurrentNad(), instruction_bytes);
        int32_t ret = ctx->SendToSocket(req_msg.GetRawMessage());
        if (ret < 0) { return ret; }

        jrcplib::BytesVec rawmsg;
        ret = ctx->RecvJrcpMessage(rawmsg);
        if (ret < 0) { return ret; }

        jrcplib::JrcpColdResetRespMessage resp_msg(rawmsg);
        ctx->SetCurrentAtr(resp_msg.GetAtr());
    }
        break;
    case ResetType::WarmReset:
    {
        jrcplib::JrcpWarmResetReqMessage req_msg(ctx->GetCurrentNad(), instruction_bytes);
        int32_t ret = ctx->SendToSocket(req_msg.GetRawMessage());
        if (ret < 0) { return ret; }

        jrcplib::BytesVec rawmsg;
        ret = ctx->RecvJrcpMessage(rawmsg);
        if (ret < 0) { return ret; }

        jrcplib::JrcpWarmResetRespMessage resp_msg(rawmsg);
        ctx->SetCurrentAtr(resp_msg.GetAtr());
    }
        break;
    default:
        return JRCPLIB_ERR_INVALID_ARGUMENT;
    }

    return JRCPLIB_ERR_OK;
}

/**
* Returns ATR of last reset.
*
* @param[in] ctx - Pointer to jrcplib_tcpclient_ctx_t.
* @param[out] atrbuflen - Length of atr buffer
* @param[out] atr - Buffer to store ATR
*
* @return An error code.
* @retval JRCPLIB_ERR_OK Everything went fine.
* @retval JRCPLIB_ERR_INSUFFICIENT_BUFFER - iff atr buffer length is not sufficient
*/
JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_activate(jrcplib_tcpclient_ctx_t *ctx, size_t atrbuflen, uint8_t *atr)
{
    if (atr == nullptr) {
        return JRCPLIB_ERR_INVALID_ARGUMENT;
    }

    jrcplib::BytesVec cur_atr = ctx->GetCurrentAtr();

    if (atrbuflen < cur_atr.size())
    {
        return JRCPLIB_ERR_INSUFFICIENT_BUFFER;
    }

    memcpy(atr, cur_atr.data(), cur_atr.size());

    return JRCPLIB_ERR_OK;
}


/**
* Send echo message to JRCP Server.
*
* @param[in] ctx - Pointer to jrcplib_tcpclient_ctx_t.
* @param[in] echostr - null-terminated echo string to send
*
* @return An error code.
* @retval JRCPLIB_ERR_OK Everything went fine.
* @retval JRCPLIB_ERR_NETWORKING_NO_ACTIVE_CONNECTION - iff no active connection.
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_SEND_FAILED - iff sending to socket fails
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED - iff receiving from socket fails
* @retval JRCPLIB_ERR_NETWORKING_COMMAND_EXECUTION_FAILED - iff JRCP Server responded error
*/
JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_echo(jrcplib_tcpclient_ctx_t *ctx, char *echostr)
{

    if (NULL == echostr)
    {
        return JRCPLIB_ERR_INVALID_ARGUMENT;
    }

    uint32_t echolen = (uint32_t)strlen(echostr);
    jrcplib::BytesVec payload;

    payload.push_back(0x09); // Regular echo subMTY
    payload.push_back(0x00);

    // Add echo string to payload
    payload.insert(payload.end(), echostr, echostr + echolen);

    // 0xEE is for MTY JCOP Simulator specific extension for Information Trace event
    jrcplib::JrcpGenericMessage req_msg(0xEE, ctx->GetCurrentNad(), 0, {}, payload, (uint32_t)payload.size());
    int32_t ret = ctx->SendToSocket(req_msg.GetRawMessage());
    if (ret < 0) { return ret; }

    jrcplib::BytesVec rawmsg;
    ret = ctx->RecvJrcpMessage(rawmsg);
    if (ret < 0) { return ret; }

    jrcplib::JrcpGenericMessage respmsg(rawmsg);
    if (rawmsg[respmsg.GetPayloadStartIndex()] != 0xEE && rawmsg[respmsg.GetPayloadStartIndex() + 1] != 0x00)
    {
        // Error ocurred.
        return JRCPLIB_ERR_NETWORKING_COMMAND_EXECUTION_FAILED;
    }

	return JRCPLIB_ERR_OK;
}

/**
* Retrieve NVM count by sending request to JRCP Server.
*
* @param[in] ctx - Pointer to jrcplib_tcpclient_ctx_t.
* @param[in] outbuf - Buffer with response data
* @param[out] resp_len - Length of response data in outbuf
*
* @return An error code.
* @retval JRCPLIB_ERR_OK Everything went fine.
* @retval JRCPLIB_ERR_NETWORKING_NO_ACTIVE_CONNECTION - iff no active connection.
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_SEND_FAILED - iff sending to socket fails
* @retval JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED - iff receiving from socket fails
* @retval JRCPLIB_ERR_NETWORKING_COMMAND_EXECUTION_FAILED - iff JRCP Server responded error
*/
JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_nvmCount(jrcplib_tcpclient_ctx_t *ctx, size_t *resp_len, uint8_t* outbuf)
{

    jrcplib::BytesVec payload;

    payload.push_back(0x16); // Regular echo subMTY
    payload.push_back(0x00);

    // 0xEE is for MTY JCOP Simulator specific extension for Information Trace event
    jrcplib::JrcpGenericMessage req_msg(0xEE, ctx->GetCurrentNad(), 0, {}, payload, (uint32_t)payload.size());
    int32_t ret = ctx->SendToSocket(req_msg.GetRawMessage());
    if (ret < 0) { return ret; }

    jrcplib::BytesVec rawmsg;
    ret = ctx->RecvJrcpMessage(rawmsg);
    if (ret < 0) { return ret; }

    jrcplib::JrcpGenericMessage respmsg(rawmsg);
    if (rawmsg[respmsg.GetPayloadStartIndex()] != 0xEE && rawmsg[respmsg.GetPayloadStartIndex() + 1] != 0x00)
    {
        // Error ocurred.
        return JRCPLIB_ERR_NETWORKING_COMMAND_EXECUTION_FAILED;
    }

    jrcplib::BytesVec resp_payload = respmsg.GetPayload();
    memcpy(outbuf, resp_payload.data(), resp_payload.size());
    *resp_len = resp_payload.size();

    return JRCPLIB_ERR_OK;
}
