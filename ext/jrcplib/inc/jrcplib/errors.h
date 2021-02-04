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
#ifndef JRCPLIB_ERRORS_H_
#define JRCPLIB_ERRORS_H_
/** @file errors.h */

#include <jrcplib/api.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Status to indicate a device is in connected state*/
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_STATUS_DEVICE_CONNECTED;
/** Status to indicate a device is in disconnected state*/
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_STATUS_DEVICE_DISCONNECTED;


/* TODO: Put all the error codes here to assign them a number. */
/** No error. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_OK;
/** A device is assigned to an NAD and currently used, so a new device cannot
be assigned to that NAD. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_NAD_IN_USE;
/** unused */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_DESCRIPTION_IN_USE;
/** Device description length exceeds max allowed */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_DESCRIPTION_LENGTH_EXCEED;
/** (unused) A device was passed to a function, but it was a null pointer.
Since the API has since been reworked to exclude device pointers but instead
use controllers and NADs, this error code is useless. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_INVALID_DEVICE;
/** unused */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_DEVICE_IN_USE;
/** A feature handler function pointer was passed, but it was a null pointer. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_INVALID_HANDLER;
/** Tried to retrieve or deregister a feature handler, but none was present. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_NO_HANDLER_REGISTERED;
/** For reporting general NULL pointers (to buffers etc.) where NULL pointers are
not allowed. For special types of null pointers, we have specialized error codes:
If a message object is null: Use JRCPLIB_ERR_INVALID_MESSAGE.
If a controller object is null: Use JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_INVALID_POINTER_ARGUMENT; /* For reporting NULL pointers where NULL pointers are not allowed */
/** A message object was passed to a function, but it was a null pointer. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
/** A function was invoked to copy something into a given buffer, but the buffer
is too short to hold the data that it is supposed to write. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_INSUFFICIENT_BUFFER;
/** The programmer using the library forgot to perform certain documented checks
or did not adhere to specified invariants before calling an API function. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
/** An allocation failed. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_OUT_OF_MEMORY;
/** Tried to register a new header, but need to deregister another one first. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_HANDLER_ALREADY_PRESENT;
/** There is no device at a given NAD. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_NO_DEVICE_REGISTERED;
/** There is no device in busy state. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_NO_DEVICE_IN_BUSY_STATE;
/** The protocol version specified is unsupported. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_PROTOCOL_UNSUPPORTED;
/** A passed socket does not exist. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_INVALID_SOCKET;
/** A socket ID was passed which did not match a previously registered socket ID. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_CLIENT_SOCKET_MISMATCH;
/** A raw message did not pass basic checks => Unable to construct a message
object from it. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_MALFORMED_MESSAGE;
/** An unsupported timing option was passed. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_TIMING_OPTION_UNSUPPORTED;
/** A controller object was passed to a function, but the controller was a null
pointer. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
/** The message handler(MTY) for the requested NAD is not available */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_MESSAGE_HANDLER_NOT_REGISTERED;
/** The Target device has been removed */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_TARGET_DEVICE_REMOVED;
/** Time out occurred in sending the response */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_TIMEOUT_OCCURRED;
/** Reset of socket failed unexpectedly */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_DEVICE_RESET_SOCKET_FAILED;

JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_INVALID_ARGUMENT;

/**< Networking initialization failed. Only on Windows when WSAStartup fails. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_NETWORKING_INIT_FAILED;
/**< Networking initialization failed. Only on Windows when WSACleanup fails. */
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_NETWORKING_CLEANUP_FAILED;
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_NETWORKING_SOCKET_CREATION_FAILED;
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_NETWORKING_SOCKET_CONNECTION_FAILED;
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_NETWORKING_SOCKET_SEND_FAILED;
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_NETWORKING_SOCKET_RECV_FAILED;
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_NETWORKING_NO_ACTIVE_CONNECTION;
JRCPLIB_EXPORT_API extern const int32_t JRCPLIB_ERR_NETWORKING_COMMAND_EXECUTION_FAILED;

#ifdef __cplusplus
namespace jrcplib
{
    const char *GetErrorString(int32_t errorCode);
}
#endif

/**
* Returns a string representing the given error code.
*
* @param errcode The error code.
*
* @return The human-readable, NULL-terminated error string.
*/
JRCPLIB_EXPORT_API const char *jrcplib_get_error_string(int32_t errcode);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
