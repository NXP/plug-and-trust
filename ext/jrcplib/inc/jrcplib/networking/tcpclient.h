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
#ifndef JRCP_TCP_CLIENT_H_
#define JRCP_TCP_CLIENT_H_

#include <jrcplib/api.h>
#include <jrcplib/data_types.h>
#include <jrcplib/errors.h>


#include <stdlib.h>
#include <stdint.h>


#ifdef _WIN32
#    include <WinSock2.h>
#else
#    include <sys/types.h>
#    include <sys/socket.h>
#    include <netinet/in.h>
#    include <netdb.h>
#    include <netinet/in.h>
#    include <arpa/inet.h>
#    include <sys/time.h>
#    include <unistd.h>
#endif /* _WIN32 */


#ifdef __cplusplus

#include <string>
#include <map>
#include <algorithm>



namespace jrcplib {
    namespace networking {
        class JrcpTcpClientContext
        {
        public:
            JrcpTcpClientContext();
            /*** Initialize context */
            int32_t  Init();
            /** Cleanup context */
            int32_t  CleanUp();
            /** Retrieve list of nodes from server */
            int32_t  GetNodesList(std::map<uint8_t, std::string> &nodes);
            /** Connect to default node */
            int32_t  Connect(std::string ip, uint16_t port);
            /** Connect to specific node specified by nodestr*/
            int32_t  Connect(std::string ip, uint16_t port, std::string nodestr);
            /** Disconnect */
            int32_t  Disconnect();
            /** Send datat to socket */
            int32_t  SendToSocket(const jrcplib::BytesVec &data) const;
            /** Receive data from socket */
            int32_t  RecvJrcpMessage(jrcplib::BytesVec &data);

            /** Get current NAD */
            uint8_t  GetCurrentNad() const { return current_nad_; }
            /** Get current ATR */
            BytesVec GetCurrentAtr() const { return current_atr_; }
            /** Set current ATR */
            void     SetCurrentAtr(BytesVec atr) { current_atr_ = atr; }
        private:
            std::string               server_ip_str_;
            uint16_t                  server_port_;
            struct sockaddr_in        serveraddr_;
            SOCKET                    sock_ = INVALID_SOCKET;
            TcpClientConnectionStatus connection_status_ = TcpClientConnectionStatus::TcpClientDisconnected;
            std::map<uint8_t, std::string> nodesmap_;
            uint8_t                   current_nad_;
            BytesVec                  current_atr_;

            int32_t RetrieveTerminalInfo();
            int32_t RetrieveNodesList();
            int32_t ConnectDevice(std::string &nodestr);
            int32_t ConnectSocket(std::string &ip, uint16_t port);


            int32_t GetNad(std::string &nodestr, uint8_t *nad) {
                for (const auto &pair : nodesmap_) {
                    if (nodestr.compare(pair.second) == 0) {
                        *nad = pair.first;
                        return JRCPLIB_ERR_OK;
                    }
                }
                return JRCPLIB_ERR_INVALID_DEVICE;
            }


            static bool compare(std::pair<uint8_t, std::string> i, std::pair<uint8_t, std::string> j)
            {
                return i.first < j.first;
            }

            uint8_t GetMinNad()
            {
                std::pair<uint8_t, std::string> min
                    = *min_element(nodesmap_.begin(), nodesmap_.end(), &JrcpTcpClientContext::compare);
                return min.first;
            }
        };
    } // namespace networking
} // namespace jrcplib

typedef jrcplib::networking::JrcpTcpClientContext jrcplib_tcpclient_ctx_t;

#else

typedef struct JrcpTcpClientContext jrcplib_tcpclient_ctx_t;

#endif /* __cplusplus*/



#ifdef __cplusplus
extern "C" {
#endif
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
    JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_init(jrcplib_tcpclient_ctx_t **outctx);

    /* Counterpart of @ref jrcplib_tcpclient_init */
    JRCPLIB_EXPORT_API void jrcplib_tcpclient_free(jrcplib_tcpclient_ctx_t *ctx);

    /**
    * Cleanup instance of TCP Client and context. It will disconnect active connection if needed.
    *
    * @param[in]  ctx - Pointer to jrcplib_tcpclient_ctx_t.
    *
    * @return An error code.
    * @retval JRCPLIB_ERR_OK Everything went fine.
    * @retval JRCPLIB_ERR_NETWORKING_CLEANUP_FAILED iff cleanup fails (only on windows platform).
    */
    JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_cleanup(jrcplib_tcpclient_ctx_t *ctx);

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
    # @retval JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED - iff receiving from socket fails
    */
    JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_list_nodes(jrcplib_tcpclient_ctx_t *ctx, uint32_t *nodes_count, jrcplib_nad_t **nodes_list);

    JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_list_nodes_cleanup(
        uint32_t nodes_count,
        jrcplib_nad_t *nodes_list
        );


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
    JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_connect_to_default_node(jrcplib_tcpclient_ctx_t *ctx, const char* ip, uint16_t port);

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
    JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_connect_to_node(jrcplib_tcpclient_ctx_t *ctx, const char* ip, uint16_t port, const char* nodestr);

    /**
    * Disconnect from JRCP Server.
    *
    * @param[in] ctx - Pointer to jrcplib_tcpclient_ctx_t.
    *
    * @return An error code.
    * @retval JRCPLIB_ERR_OK Everything went fine.
    */
    JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_disconnect(jrcplib_tcpclient_ctx_t *ctx);

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
    JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_dataexchange(jrcplib_tcpclient_ctx_t *ctx, size_t inbuflen, uint8_t *inbuf, size_t outbuflen, uint8_t *outbuf, size_t *resp_len);

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
    JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_reset(jrcplib_tcpclient_ctx_t *ctx, jrcplib_reset_type_t reset_type);

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
    JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_prepare_reset(jrcplib_tcpclient_ctx_t *ctx, jrcplib_reset_type_t reset_type, uint32_t instruction_bytes);

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
    JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_activate(jrcplib_tcpclient_ctx_t *ctx, size_t atrbuflen, uint8_t *atr);

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
    JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_echo(jrcplib_tcpclient_ctx_t *ctx, char *echostr);

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
    JRCPLIB_EXPORT_API int32_t jrcplib_tcpclient_nvmCount(jrcplib_tcpclient_ctx_t *ctx, size_t *resp_len, uint8_t* outbuf);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* JRCP_TCP_CLIENT_H_ */
