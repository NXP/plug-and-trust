/* Copyright 2019 NXP
 *
 * This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms.  By expressly
 * accepting such terms or by downloading, installing, activating and/or
 * otherwise using the software, you are agreeing that you have read, and
 * that you agree to comply with and are bound by, such license terms.  If
 * you do not agree to be bound by the applicable license terms, then you
 * may not retain, install, activate or otherwise use the software.
 */
#ifndef TCPSERVER_H_
#define TCPSERVER_H_

#include <string>

using std::string;

namespace network {
  class Server {
 public:
    static Server* getInstance();

    bool setup(uint16_t port, bool bindall, const char* bindip);
    string getLastError();

    // Server is neither copyable nor movable.
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

 private:
    static Server *instance;
    static string serverip;
    static string error_message;

    // private constructor
    Server() {

    }


  };
} // namespace network

#endif // TCPSERVER_H_
