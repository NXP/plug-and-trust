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
#ifndef JRCPLIB_H_
#define JRCPLIB_H_
/** @file jrcplib.h */

#include <jrcplib/api.h>
#include <jrcplib/data_types.h>
#include <jrcplib/errors.h>
#include <jrcplib/deviceregistry.h>

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus

#include <iostream>
#include <sstream>
#include <string>

#define JRCP_PROTOCOL_SESSIONID_LENGTH 16
#define JRCP_STORE_SESSION_ID          0x0F

// Forward declare JrcpMessage and jrcplib_jrcp_msg_t because of a cyclic dependency.
namespace jrcplib
{
    class JrcpMessage;
}

using jrcplib_jrcp_msg_t = jrcplib::JrcpMessage;

namespace jrcplib
{
    class JrcpController
    {
    public:
        JrcpController();
        JrcpController(jrcplib_protocol_supported_t version, std::string identifier);

        int32_t                 ProcessErrorCode(uint8_t * input_message, jrcplib_jrcp_msg_t **output_response, int32_t error_code);
        int32_t                 ProcessMessage(BytesVec msg_buffer, jrcplib_jrcp_msg_t **out);
        int32_t                 ProcessMessage(uint32_t buflen, uint8_t* message_buffer, SOCKET socket_id, jrcplib_jrcp_msg_t **out);

        bool                    IsProtocolVersionSupported(jrcplib_protocol_supported_t protocol_version);
        DeviceRegistry &        GetDeviceRegistry();
        const DeviceRegistry &  GetDeviceRegistry() const;

        const std::string &          GetControllerIdentifier() const { return controller_identifier_; }
        jrcplib_protocol_supported_t GetControllerVersion()    const { return controller_version_; }

        bool                    ConvertTerminalInfoMsgToV2Format(BytesVec &out_message);
    private:
        DeviceRegistry          device_registry_instance_;

        jrcplib_protocol_supported_t controller_version_;
        std::string             controller_identifier_;
    };

    using ProtocolSupportedCheck = EnumCheck<JrcpProtocolSupported,
                                             JrcpProtocolSupported::JrcpV2,
                                             JrcpProtocolSupported::SimulatorSpecificVersion>;

    int32_t Init(jrcplib_protocol_supported_t protocol_version, size_t identifier_length, const char* controller_identifier, JrcpController **out);
}

typedef jrcplib::JrcpController jrcplib_controller_t;

#else
/**
* Type which describes an instance of the JRCP library.
*
* Each instance, described by a jrcplib_controller_t, has its own controller and device
* registry. In real-world use, this design is meant to support multiple JRCP
* ports for various type groups of device interfaces (wired, contactless, ISO,
* ...)
*
* An instance of the library can be obtained by calling jrcplib_init.
*/
#ifndef JRCPLIB_MESSAGE_FORWARD_DECLARED_
#define JRCPLIB_MESSAGE_FORWARD_DECLARED_
typedef struct JrcpController jrcplib_controller_t;
typedef struct JrcpMessage jrcplib_jrcp_msg_t;
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
* Creates an instance of the JRCP library to be used with further function calls.
*
* This "instance" is the same as a "controller". It can contain up to 255 user-
* defined devices. By default it already has a server device which is mapped to
* the NAD 0xFF, with some default MTY handlers for the following standard MTYs:
*
*     - Controller Configuration (MTY FEh)
*     - Feature Control (MTY FFh)
*     - Server Status (MTY 0Ah)
*     - Terminal Information (MTY 04h)
*
* If you want to override the behaviour of these standard feature handlers, you
* must first deregister them and then register new handlers.
*
* The controller structure must as of currently not be shared across threads
* without taking care of synchronization yourself; doing so could lead to
* undefined behaviour. Note: This may change in the future.
*
* # Usage example
*
* To initialize a new controller, add a device without any handlers, and then
* get a device list from the predefined Feature Control MTY handler, do the
* following:
*
* @code
* const char *controller_identifier = "My fancy controller!";
* jrcplib_controller_t *instance = NULL;
*
* // Initialization
* int32_t errcode = jrcplib_init(
*     JrcpV2,
*     strlen(controller_identifier),
*     controller_identifier,
*     &instance);
*
* // Check
* if(errcode != JRCPLIB_ERR_OK || instance == NULL) {
*     printf("jrcplib_init failed: %s\n", jrcplib_get_error_string(errcode));
*     return EXIT_FAILURE;
* }
*
* // Device adding with NAD 0x80 and no handlers
* const char *device_identifier = "dev1";
* errcode = jrcplib_controller_create_device(
*     instance,
*     0x80,
*     strlen(device_identifier),
*     device_identifier);
*
* // Check
* if(errcode != JRCPLIB_ERR_OK || instance == NULL) {
*     printf("jrcplib_controller_create_device failed: %s\n",
*         jrcplib_get_error_string(errcode));
*     return EXIT_FAILURE;
* }
*
* errcode = jrcplib_controller_register_device(instance, 0x80);
*
* // Check
* if(errcode != JRCPLIB_ERR_OK || instance == NULL) {
*     printf("jrcplib_controller_register_device failed: %s\n",
*         jrcplib_get_error_string(errcode));
*     return EXIT_FAILURE;
* }
*
* // The request and response messages
* jrcplib_jrcp_msg_t *msg = NULL;
* uint8_t list_reader_command[] = { (uint8_t)0xA5, // SOF
*     (uint8_t)0xFE, (uint8_t)0xFF,                // MTY, NAD
*     0x00,                                        // HDL
*     0x00, 0x00, 0x00, 0x02,                      // LN
*     0x00, 0x01,                                  // List reader command payload
*     0x00                                         // TIL
* };
*
* // Process list_reader_command and put the response into msg.
* errcode = jrcplib_controller_process_message(
*     instance,
*     11,
*     list_reader_command,
*     &msg);
*
* // Check
* if(errcode != JRCPLIB_ERR_OK || instance == NULL) {
*     printf("jrcplib_controller_process_message failed: %s\n",
*         jrcplib_get_error_string(errcode));
*     return EXIT_FAILURE;
* }
*
* // Now the device list is in msg, use e.g. `jrpclib_msg_get_raw_message` and
* // `jrcplib_msg_get_payload_start_idx` to analyze the result.
* @endcode
*
* @param protocol_version The protocol version to be used.
* @param identifier_length The length of the identifier parameter
* @param controller_identifier The identifier of the controller, ASCII.
* @param out The pointer which will receive the instance.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_PROTOCOL_UNSUPPORTED if the protocol_version parameter
*       indicated an unsupported version.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT if null pointer is encountered for controller
*         identifier and the controller instance out parameter
*/
JRCPLIB_EXPORT_API int32_t jrcplib_init(jrcplib_protocol_supported_t protocol_version,
    size_t identifier_length,
    const char* controller_identifier,
    jrcplib_controller_t **out);

/**
* Destroys a JRCP library instance previously created by jrcplib_init.
*
* If an instance of this library should not be used any longer, this function
* should be called to avoid memory leaks.
*
* This function does not fail nor return anything.
*
* @param thisinst The instance which should be cleaned up.
*/
JRCPLIB_EXPORT_API void jrcplib_cleanup(jrcplib_controller_t *thisinst);

/**
* Process the given message in the controller and get the appropriate response.
*
* For request messages, use `jrcplib_controller_process_request_message`.
*
* @param thisinst The controller with the devices to process this message.
* @param buflen The length of the message_buffer argument.
* @param message_buffer The raw JRCP frame.
* @param out The pointer to a message object pointer which receives the response.
*
* @return An error code.
* @retval JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE iff thisinst is a NULL pointer.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff message_buffer or out is a
*       NULL pointer.
* @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED iff the device referenced by the
*       NAD in the message is not registered.
* @retval JRCPLIB_ERR_MALFORMED_MESSAGE iff the message object could not be
*       created from the raw buffer.
* @retval JRCPLIB_ERR_NO_HANDLER_REGISTERED iff there was no message handler
*       in the controller for the given combination of NAD and MTY.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_controller_process_message(
    jrcplib_controller_t *thisinst,
    uint32_t buflen,
    uint8_t *message_buffer,
    jrcplib_jrcp_msg_t **out
    );

/**
* Process the given message in the controller and get the appropriate response.
*
* @param thisinst The controller with the devices to process this message.
* @param buflen The length of the message_buffer argument.
* @param message_buffer The raw JRCP frame.
* @param socket_id The socket ID which is used to identify a connection.
* @param out The pointer to a message object pointer which receives the response.
*
* @return The response message, or NULL if an error occurred.
*
* @retval JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE iff thisinst is a NULL pointer.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff message_buffer or out is a
*       NULL pointer.
* @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED iff the device referenced by the
*       NAD in the message is not registered.
* @retval JRCPLIB_ERR_MALFORMED_MESSAGE iff the message object could not be
*       created from the raw buffer.
* @retval JRCPLIB_ERR_NO_HANDLER_REGISTERED iff there was no message handler
*       in the controller for the given combination of NAD and MTY.
* @retval JRCPLIB_ERR_INVALID_SOCKET iff the socket ID was INVALID_SOCKET.
* @retval JRCPLIB_ERR_CLIENT_SOCKET_MISMATCH iff the socket ID was not associated
*       to the device.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_controller_process_request_message(
    jrcplib_controller_t *thisinst,
    uint32_t buflen,
    uint8_t *message_buffer,
    SOCKET socket_id,
    jrcplib_jrcp_msg_t **out
    );


#ifdef __cplusplus
}
#endif

#endif // JRCPLIB_H_
