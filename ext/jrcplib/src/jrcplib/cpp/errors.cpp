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
#include <jrcplib/errors.h>
#include <map>

const int32_t JRCPLIB_STATUS_DEVICE_CONNECTED = 5;
const int32_t JRCPLIB_STATUS_DEVICE_DISCONNECTED = 6;

const int32_t JRCPLIB_ERR_OK = 0;
const int32_t JRCPLIB_ERR_NAD_IN_USE = -1;
const int32_t JRCPLIB_ERR_DESCRIPTION_IN_USE = -2;
const int32_t JRCPLIB_ERR_INVALID_DEVICE = -3;
const int32_t JRCPLIB_ERR_DEVICE_IN_USE = -4;
const int32_t JRCPLIB_ERR_INVALID_HANDLER = -5;
const int32_t JRCPLIB_ERR_NO_HANDLER_REGISTERED = -6;
const int32_t JRCPLIB_ERR_INVALID_POINTER_ARGUMENT = -7;
const int32_t JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT = -8;
const int32_t JRCPLIB_ERR_INSUFFICIENT_BUFFER = -9;
const int32_t JRCPLIB_ERR_OPERATION_NOT_PERMITTED = -10;
const int32_t JRCPLIB_ERR_OUT_OF_MEMORY = -11;
const int32_t JRCPLIB_ERR_HANDLER_ALREADY_PRESENT = -12;
const int32_t JRCPLIB_ERR_NO_DEVICE_REGISTERED = -13;
const int32_t JRCPLIB_ERR_PROTOCOL_UNSUPPORTED = -14;
const int32_t JRCPLIB_ERR_INVALID_SOCKET = -15;
const int32_t JRCPLIB_ERR_CLIENT_SOCKET_MISMATCH = -16;
const int32_t JRCPLIB_ERR_MALFORMED_MESSAGE = -17;
const int32_t JRCPLIB_ERR_TIMING_OPTION_UNSUPPORTED = -18;
const int32_t JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE = -19;
const int32_t JRCPLIB_ERR_MESSAGE_HANDLER_NOT_REGISTERED = -20;
const int32_t JRCPLIB_ERR_NO_DEVICE_IN_BUSY_STATE = -21;
const int32_t JRCPLIB_ERR_DESCRIPTION_LENGTH_EXCEED = -22;
const int32_t JRCPLIB_ERR_TARGET_DEVICE_REMOVED = -23;
const int32_t JRCPLIB_ERR_TIMEOUT_OCCURRED = -24;
const int32_t JRCPLIB_ERR_DEVICE_RESET_SOCKET_FAILED = -25;
const int32_t JRCPLIB_ERR_NETWORKING_INIT_FAILED = -26;
const int32_t JRCPLIB_ERR_NETWORKING_CLEANUP_FAILED = -27;
const int32_t JRCPLIB_ERR_NETWORKING_SOCKET_CREATION_FAILED = -28;
const int32_t JRCPLIB_ERR_NETWORKING_SOCKET_CONNECTION_FAILED = -29;
const int32_t JRCPLIB_ERR_NETWORKING_SOCKET_SEND_FAILED = -30;
const int32_t JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED = -31;
const int32_t JRCPLIB_ERR_NETWORKING_NO_ACTIVE_CONNECTION = -32;
const int32_t JRCPLIB_ERR_INVALID_ARGUMENT = -33;
const int32_t JRCPLIB_ERR_NETWORKING_COMMAND_EXECUTION_FAILED = -34;



namespace jrcplib
{
    /* TODO: Put all the error codes here with descriptions. */
    static std::map<int32_t, const char *> error_strings =
    {
        { JRCPLIB_ERR_OK, "No error occurred." },
        { JRCPLIB_ERR_NAD_IN_USE, "The requested node address is already in use." },
        { JRCPLIB_ERR_DESCRIPTION_IN_USE, "The requested description is already in use." },
        { JRCPLIB_ERR_DEVICE_IN_USE, "The requested device is already in use." },
        { JRCPLIB_ERR_NO_HANDLER_REGISTERED, "No handler is registered for the given combination of NAD and MTY." },
        { JRCPLIB_ERR_INVALID_DEVICE, "An invalid device was passed as an argument." },
        { JRCPLIB_ERR_INVALID_HANDLER, "An invalid handler was passed as an argument." },
        { JRCPLIB_ERR_INVALID_POINTER_ARGUMENT, "An invalid pointer was passed as an argument." },
        { JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT, "An invalid message object was passed as an argument." },
        { JRCPLIB_ERR_INSUFFICIENT_BUFFER, "The buffer was too small to hold the requested information." },
        { JRCPLIB_ERR_OPERATION_NOT_PERMITTED, "The requested operation is not applicable to the given arguments." },
        { JRCPLIB_ERR_OUT_OF_MEMORY, "Not enough memory." },
        { JRCPLIB_ERR_HANDLER_ALREADY_PRESENT, "A handler for this combination of NAD and MTY is already present." },
        { JRCPLIB_ERR_NO_DEVICE_REGISTERED, "No device is registered for the given NAD and MTY." },
        { JRCPLIB_ERR_PROTOCOL_UNSUPPORTED, "The passed protocol version in JrcpInit is not supported." },
        { JRCPLIB_ERR_INVALID_SOCKET, "The Socket ID passed as argument us INVALID." },
        { JRCPLIB_ERR_CLIENT_SOCKET_MISMATCH, "The Registered Client Socket ID and passed socket ID are not matching." },
        { JRCPLIB_ERR_MALFORMED_MESSAGE, "The message given to the message processor was malformed and could not be processed." },
        { JRCPLIB_ERR_TIMING_OPTION_UNSUPPORTED, "The Option passed in Timing information command is not supported" },
        { JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE, "The passed library instance was invalid." },
        { JRCPLIB_ERR_MESSAGE_HANDLER_NOT_REGISTERED, "The message handler not registered for the requested NAD." },
        { JRCPLIB_ERR_NO_DEVICE_IN_BUSY_STATE ,"There is currently no device in the JRCP instance which is in busy state"},
        { JRCPLIB_ERR_DESCRIPTION_LENGTH_EXCEED, "The device description length exceed maximum allowed length 0xFF" },
        { JRCPLIB_ERR_TARGET_DEVICE_REMOVED, "The target device is removed" },
        { JRCPLIB_ERR_TIMEOUT_OCCURRED, "Timeout occurred in sending the response to the client" },
        { JRCPLIB_ERR_NETWORKING_INIT_FAILED, "Network subsystem initialization failed" },
        { JRCPLIB_ERR_NETWORKING_CLEANUP_FAILED, "Network subsystem cleanup failed" },
        { JRCPLIB_ERR_NETWORKING_SOCKET_CREATION_FAILED, "The socket creation failed" },
        { JRCPLIB_ERR_NETWORKING_SOCKET_CONNECTION_FAILED, "The connection to server failed" },
        { JRCPLIB_ERR_NETWORKING_SOCKET_SEND_FAILED, "Sending to socket failed" },
        { JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED, "Receiving from socket failed" },
        { JRCPLIB_ERR_NETWORKING_NO_ACTIVE_CONNECTION, "No active TCP connection" },
        { JRCPLIB_ERR_INVALID_ARGUMENT, "Function argument is invalid" },
        { JRCPLIB_ERR_NETWORKING_COMMAND_EXECUTION_FAILED, "Command execution failed. No reason available." },
    };

    /* Internal API to get the meaningful error string by passing the error code */
    const char *GetErrorString(int32_t errorCode)
    {
        auto result = error_strings.find(errorCode);

        /* Return some default string if the error message was not in the list. */
        if (result == error_strings.end())
        {
            return "Unknown Error.";
        }

        return result->second;
    }
}

JRCPLIB_EXPORT_API const char *jrcplib_get_error_string(int32_t errcode)
{
    return jrcplib::GetErrorString(errcode);
}

