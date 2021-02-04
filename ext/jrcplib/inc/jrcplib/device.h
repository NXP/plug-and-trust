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
#ifndef JRCPLIB_DEVICE_H_
#define JRCPLIB_DEVICE_H_
/** @file device.h */

#include <jrcplib/api.h>
#include <jrcplib/data_types.h>
#include <jrcplib/messages.h>
#include <jrcplib/jrcplib.h>
#include <jrcplib/deviceregistry.h> // Included for backward compatibility

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus

#include <string>
#include <cstdint>
#include <map>
#include <utility>
#include <functional>
#include <vector>


namespace jrcplib {

    typedef std::function<int32_t(JrcpController*, JrcpMessage*, JrcpMessage**)> FeatureHandlerFunc;
    typedef std::pair<uint16_t, FeatureHandlerFunc> FeatureHandler;
    typedef std::map<uint8_t, FeatureHandler> Handlers;

    class Device {
    public:
        Device(std::uint8_t nad, std::string description);

        /* The connection state of this device. If it is connected, it will
        be listed with its true NAD on the ListReaders command.
        If it is not connected, it is listed with NAD 0xFF. */
        inline bool IsConnected() const { return (constat_ == DeviceConnectionStatus::Connected); }
        /* Whether the device is currently processing a message. */
        inline bool IsBusy()      const { return isbusy_; }

        /* Inline function to get the device NAD */
        inline uint8_t GetDeviceNad() const { return nad_; }

        /* Inline function to set the busy status of a device  */
        inline void SetDeviceBusyStatus(bool status)  {isbusy_ = status; }

        /* Inline function to set the busy status of a device  */
        inline bool GetDeviceBusyStatus(void)  const { return isbusy_; }

        /* Inline function to get the device description */
        inline std::string GetDeviceDescription() const { return description_; }

        /* Inline function to set the device server status */
        inline void SetDeviceServerStatus(JrcpGenericStatusCodes status)  { server_status_ = status; }

        /* Inline function to get the device server status */
        inline JrcpGenericStatusCodes GetDeviceServerStatus() const { return server_status_; }

        /* Function to register the feature handler based on the MTY for a device NAD  */
        int32_t RegisterFeatureHandler(uint8_t mty, FeatureHandler handler);
        /* Function to deregister the feature handler based on the MTY for a device NAD  */
        int32_t DeregisterFeatureHandler(uint8_t mty);
        /* Public method to update the socket ID of the associated device */
        int32_t CheckAndUpdateSocketID(SOCKET socketID);
        /* Public method to clear the socket ID of the associated device */
        int32_t CheckAndClearSocketID(SOCKET socketID, bool *out);

        /* function to get the device feature handler based on the message type */
        int32_t GetFeatureHandler(uint8_t mty, const FeatureHandler** out) const;

        /* Inline function to set the device connection state */
        inline void SetDeviceConnectionStatus(DeviceConnectionStatus status)  { constat_ = status; }

    private:
        /* Whether the device is connected and will be shown with the true
        NAD in ListReaders. */
        DeviceConnectionStatus constat_ = DeviceConnectionStatus::Disconnected;
        /* Whether the device is currently processing a message. */
        bool isbusy_ = false;
        /* Stores the device status queried as part of server status command */
        JrcpGenericStatusCodes server_status_;
        std::uint8_t nad_;
        std::string description_;
        SOCKET socketID_;
        Handlers handlers_;
    };

} // namespace jrcplib

    using  jrcplib_device_t = jrcplib::Device;
#else
typedef struct Device jrcplib_device_t;
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
* Creates a new device in the given controller.
*
* This combination of controller and NAD can then be used to issue further calls
* to other functions beginning with `jrcplib_device_`.
*
* Before the device will appear in the output of ListReaders, the function
* `jrcplib_controller_register_device` must be called. This should be done
* after registering all the feature handlers.
*
* # Example
*
* ```
* // Device is created and added, but is not visible from ListReaders at all
* jrcplib_controller_create_device(instance, nad, strlen("device"), "device");
* // Register message handlers here ...
* jrcplib_controller_register_device(instance, nad);
* // Now the device went live and can be used normally
* ```
*
* @param thisinst The controller instance where the device should be created.
* @param nad The NAD of the device.
* @param description_len The length of the device description string.
* @param description The description of the device. Must not be NULL.
*
* @return An error code.
* @retval JRCPLIB_ERR_OK Everything went fine.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the description parameter
*       is a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff allocation failed.
* @retval JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE iff the controller is a null
*       pointer.
* @retval JRCPLIB_ERR_NAD_IN_USE iff the NAD is already occupied.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_controller_create_device(
    jrcplib_controller_t *thisinst,
    uint32_t nad,
    size_t description_len,
    const char* description
);

/**
* After registering the feature handlers for this NAD, turn the device live.
*
* @param thisinst The controller instance where the device was created.
* @param nad The preliminary NAD of the device.
*
* @return An error code.
* @retval JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE iff the controller is a null
*       pointer.
* @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED iff there was no device created.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_controller_register_device(
    jrcplib_controller_t *thisinst,
    uint32_t nad
);

/**
* Removes a device descriptor obtained from `jrcplib_device_create`.
*
* This function fails if no device was found at the given NAD.
*
* @param thisinst The controller on which the device was created.
* @param nad The NAD of the device.
*
* @return An error code.
* @retval JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE iff thisinst is NULL.
* @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED iff there was no device at the
*       given NAD.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_controller_remove_device(
    jrcplib_controller_t *thisinst,
    uint8_t nad
);

/**
* Register a message handler for the given MTY on a device.
*
* @param thisinst The controller on which the device was created.
* @param nad The NAD of the device.
* @param mty The message type whose handler should be set on the device. One of
*            the `kJrcp...Mty` constants.
* @param version The feature version of the handler.
* @param pmsghandler The message handler function for the message type.
*
* @return An error code.
* @retval JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE iff thisinst is NULL.
* @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED iff there was no device at the
*       given NAD.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the pmsghandler parameter
*       is a NULL pointer.
* @retval JRCPLIB_ERR_HANDLER_ALREADY_PRESENT iff there is already a message
*       handler at this NAD for the given MTY.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_device_register_msg_handler(
    jrcplib_controller_t *thisinst,
    uint8_t nad,
    uint8_t mty,
    uint16_t version,
    int32_t(*pmsghandler)(jrcplib_controller_t*, jrcplib_jrcp_msg_t*, jrcplib_jrcp_msg_t **)
);

/**
* Deregister a message handler from a device.
*
* There must have previously been a message handler registered for the message
* type.
*
* @param thisinst The controller on which the device was created.
* @param nad The NAD of the device.
* @param mty The message type whose message handler should be deregistered.
*
* @return An error code.
* @retval JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE iff thisinst is NULL.
* @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED iff there was no device at the
*       given NAD.
* @retval JRCPLIB_ERR_NO_HANDLER_REGISTERED iff there was no handler for
*       the MTY.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_device_deregister_msg_handler(
    jrcplib_controller_t *thisinst,
    uint8_t nad,
    uint8_t mty
);

/**
* Set the connected state of the device to true. The device will appear with
* its true NAD in the output of ListReaders.
*
* Before doing this, you must have called `jrcplib_controller_register_device`.
*
* @param thisinst The controller to find the device
* @param nad The NAD to find the device
*
* @return An error code.
* @retval JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE iff thisinst is NULL.
* @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED iff there was no device at the
*       given NAD.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_device_connect(
    jrcplib_controller_t *thisinst,
    uint8_t nad
);

/**
* Set the connected state of the device to false. The device will appear with
* NAD 0xFF in the output of ListReaders.
*
* @param thisinst The controller to find the device
* @param nad The NAD to find the device
*
* @return An error code.
* @retval JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE iff thisinst is NULL.
* @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED iff there was no device at the
*       given NAD.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_device_disconnect(
    jrcplib_controller_t *thisinst,
    uint8_t nad
);

/**
* Function to check if the device mapped to the passed nad is in state connected or disconnected
*
* @param thisinst The controller to find the device
* @param nad The NAD to find the device
*
* @return An error code.
* @retval JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE iff thisinst is NULL.
* @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED iff there was no device at the
*       given NAD.
*
* @return JRCPLIB_STATUS_DEVICE_CONNECTED indicate device state connected
* @return JRCPLIB_STATUS_DEVICE_DISCONNECTED indicate device state disconnected

*/
JRCPLIB_EXPORT_API int32_t jrcplib_device_is_connected
(
    jrcplib_controller_t *thisinst,
    uint8_t nad
);

/**
* Sets JRCP Device Server status code.
*
* @param thisinst The instance of this library whose status code should be set.
* @param nad The NAD of device whose server status needs to be set
* @param status The server status code which needs to be set.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED iff the device NAD is not registered
* @retval JRCPLIB_ERR_INVALID_INSTANCE iff thisinst is NULL.
* @retval JRCPLIB_ERR_OK iff the server status is set successfully for the device
*/
JRCPLIB_EXPORT_API int32_t jrcplib_device_set_server_status
(
    jrcplib_controller_t *thisinst,
    uint8_t nad,
    jrcplib_jrcp_generic_status_code_t status
);

/**
* Gets JRCP device Server status code.
*
* @param thisinst The instance of this library whose status code is requested.
* @param nad The device NAD for which the server status is intended.
* @param out Pointer to a variable which receives the status code.
*
* @return An error code
*
* @retval JRCPLIB_ERR_INVALID_INSTANCE iff thisinst is NULL.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff out is a NULL pointer.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_device_get_server_status
(
    jrcplib_controller_t *thisinst,
    uint8_t nad,
    jrcplib_jrcp_generic_status_code_t *out
);

/**
* Sets JRCP device busy status code.
*
* @param thisinst The instance of this library whose status code is requested.
* @param nad The device NAD for which the server status is intended.
* @param busy_status The boolean value to set the busy status
*
* @return An error code
*
* @retval JRCPLIB_ERR_INVALID_INSTANCE iff thisinst is NULL.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff out is a NULL pointer.
* @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED iff out no device with the passed NAD is registered and available.
* @retval JRCPLIB_ERR_OK iff the device busy status is set successfully.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_device_set_device_busy_status
(
    jrcplib_controller_t *thisinst,
    uint8_t nad,
    bool busy_status
);


/**
* Function to clear the socket id of the first available active device
*
* @param thisinst The instance of this library whose status code is requested.
* @param socket_id The socket id
*
* @return An error code
*
* @retval JRCPLIB_ERR_INVALID_INSTANCE iff thisinst is NULL.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff out is a NULL pointer.
* @retval JRCPLIB_ERR_OK iff device is retrieved successfully.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_device_reset_socket(jrcplib_controller_t *thisinst, SOCKET socket_id);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // JRCPLIB_DEVICE_H_
