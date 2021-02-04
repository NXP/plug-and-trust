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
#include <jrcplib/device.h>
#include <jrcplib/errors.h>
#include <jrcplib/deviceregistry.h>
#include "../include/utils.h"

#include <cstdint>
#include <string>
#include <cassert>

namespace jrcplib {

    /* Constructor for the device class */
    Device::Device(uint8_t dev_nad, std::string dev_description)
        :
    constat_(DeviceConnectionStatus::Disconnected),
    isbusy_(false),
    server_status_(JrcpGenericStatusCodes::GenericOK),
    nad_(dev_nad),
    description_(dev_description),
    socketID_(INVALID_SOCKET)
    {
    }


    /* Public method to register the feature handler for a particular device */
    int32_t Device::RegisterFeatureHandler(uint8_t mty, FeatureHandler handler) {
        // If handler already exist
        if (handlers_.find(mty) != handlers_.end()) {
            return JRCPLIB_ERR_HANDLER_ALREADY_PRESENT;
        }

        handlers_[mty] = handler;
        return JRCPLIB_ERR_OK;
    }

    /* Public method to deregister the feature handler for a particular device */
    int32_t Device::DeregisterFeatureHandler(uint8_t mty) {
        // Nothing to deregister
        if (handlers_.find(mty) == handlers_.end()) {
            return JRCPLIB_ERR_NO_HANDLER_REGISTERED;
        }

        handlers_.erase(mty);
        return JRCPLIB_ERR_OK;
    }

    /* Public method to get the feature handler for a given MTY */
    int32_t Device::GetFeatureHandler(uint8_t mty, const FeatureHandler**out) const
    {

        auto ifh = handlers_.find(mty);
        if ( ifh == handlers_.end()) {
            return JRCPLIB_ERR_NO_HANDLER_REGISTERED;
        }

        return jrcplib::utils::TryAssignOutParameter(out, &(ifh->second));
    }

    /* Public method to update the socket ID of the associated device */
    int32_t Device::CheckAndUpdateSocketID(SOCKET socketID)  {

        if (INVALID_SOCKET == socketID)
        {
            return JRCPLIB_ERR_INVALID_SOCKET;
        }
        /*Socket is invalid means still this device is not associated with a client so no socket id is not updated at*/
        this->socketID_ = socketID;
        return JRCPLIB_ERR_OK;
    }

    /* Public method to clear the socket ID of the associated device */
    int32_t Device::CheckAndClearSocketID(SOCKET socketID, bool *result)  {

        if (INVALID_SOCKET == socketID)
        {
            jrcplib::utils::SetOutParameterValue(result, false);
            return JRCPLIB_ERR_INVALID_SOCKET;
        }

        if (socketID == socketID_)
        {
            jrcplib::utils::SetOutParameterValue(result, true);
            socketID_ = INVALID_SOCKET;
            return JRCPLIB_ERR_OK;
        }
        /*Its not a error the socket which has to be cleared is not found at*/
        jrcplib::utils::SetOutParameterValue(result, false);
        return JRCPLIB_ERR_OK;
    }



} // namespace jrcplib



JRCPLIB_EXPORT_API int32_t jrcplib_controller_create_device(
    jrcplib_controller_t *thisinst,
    uint32_t nad,
    size_t description_len,
    const char* description
)
{
    /* First check whether the controller is valid */
    if (thisinst == nullptr)
    {
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }

    /* Then check whether the description points to valid memory */
    if (description == nullptr)
    {
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Device description max length if 0xFF */
    if (description_len > 0xFF)
    {
        return JRCPLIB_ERR_DESCRIPTION_LENGTH_EXCEED;
    }

    /* Create the new device with the NAD and description */
    std::shared_ptr<jrcplib::Device> newdev = std::make_shared<jrcplib::Device>(nad, std::string(description, description_len));

    /* If allocation failed, report and return. */
    if (newdev == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    /* To support multiple instances, we have this controller instance pointer from
    which we get the device registry. There can be multiple device registries at
    the same time, which are represented by different instance pointers. Then
    add the device to this instance, which has no effect on any of the other
    instances, since DeviceRegistry is not a singleton any more. */
    return thisinst->GetDeviceRegistry().AddDeviceWithDefaultHandlers(thisinst->GetControllerVersion(), newdev);
}

JRCPLIB_EXPORT_API int32_t jrcplib_controller_register_device(
    jrcplib_controller_t *thisinst,
    uint32_t nad
)
{
    /* First check whether the controller is valid */
    if (thisinst == nullptr)
    {
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }

    /* Then check whether the device is registered already. Otherwise this
    cannot work. */
    if (!thisinst->GetDeviceRegistry().IsNADInUse(nad))
    {
        return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
    }


    /* Check if the device NAD is less than 0x80. If yes, make it connected by default */
    if (JRCP_PREDEFINED_NAD_MAX_VALUE >= nad)
    {
        thisinst->GetDeviceRegistry()[nad]->SetDeviceConnectionStatus(DeviceConnectionStatus::Connected);
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_controller_remove_device(
    jrcplib_controller_t *thisinst,
    uint8_t nad
)
{
    /* First check whether the controller is valid */
    if (thisinst == nullptr)
    {
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }

    /* The device is now automatically deleted when calling the destructor of
    std::shared_ptr. */
    return thisinst->GetDeviceRegistry().RemoveDevice(nad);
}

JRCPLIB_EXPORT_API int32_t jrcplib_device_register_msg_handler(
    jrcplib_controller_t *thisinst,
    uint8_t nad,
    uint8_t mty,
    uint16_t version,
    int32_t(*pmsghandler)(jrcplib_controller_t*, jrcplib_jrcp_msg_t*, jrcplib_jrcp_msg_t **)
)
{
    /* First check whether the controller is valid */
    if (thisinst == nullptr)
    {
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }

    if (!thisinst->GetDeviceRegistry().IsNADInUse(nad))
    {
        /* We need to make sure there is a device at this NAD. */
        return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
    }

    if (pmsghandler == nullptr)
    {
        /* Cannot set to a nullptr message handler! If the intention is to remove
        the message handler from the device, call `jrcplib_device_deregister_msg_handler`
        instead. */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Create the feature handler object */
    jrcplib::FeatureHandlerFunc h = pmsghandler;
    jrcplib::FeatureHandler fh = std::make_pair(version, h);

    /* Try to register the feature handler */
    return thisinst->GetDeviceRegistry()[nad]->RegisterFeatureHandler(mty, fh);
}


JRCPLIB_EXPORT_API int32_t jrcplib_device_deregister_msg_handler(
    jrcplib_controller_t *thisinst,
    uint8_t nad,
    uint8_t mty
)
{
    /* First check whether the controller is valid */
    if (thisinst == nullptr)
    {
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }

    if (thisinst->GetDeviceRegistry()[nad] == nullptr)
    {
        /* We need to make sure there is a device at this NAD. */
        return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
    }

    /* Try to deregister the feature handler */
    return thisinst->GetDeviceRegistry()[nad]->DeregisterFeatureHandler(mty);
}

JRCPLIB_EXPORT_API int32_t jrcplib_device_connect(
    jrcplib_controller_t *thisinst,
    uint8_t nad
)
{
    /* Check instance pointer */
    if (thisinst == nullptr)
    {
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }

    /* Get the device registry from the controller and then get the
    device pointer*/
    jrcplib::DeviceRegistry &dev_reg = thisinst->GetDeviceRegistry();
    if (nullptr != (dev_reg)[nad])
    {
        /* In case the device exists, set its status to connected */
        (dev_reg)[nad]->SetDeviceConnectionStatus(DeviceConnectionStatus::Connected);
        return JRCPLIB_ERR_OK;
    }
    else
    {
        /* In case the device does not exist, set the error code to the
        appropriate error. */
        return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
    }
}

JRCPLIB_EXPORT_API int32_t jrcplib_device_disconnect(
    jrcplib_controller_t *thisinst,
    uint8_t nad
)
{
    /* Check instance pointer */
    if (thisinst == nullptr)
    {
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }

    /* Get the device registry from the controller and then get the
    device pointer*/
    jrcplib::DeviceRegistry &dev_reg = thisinst->GetDeviceRegistry();
    if (nullptr != (dev_reg)[nad])
    {
        /* In case the device exists, set its status to connected */
        (dev_reg)[nad]->SetDeviceConnectionStatus(DeviceConnectionStatus::Disconnected);
        return JRCPLIB_ERR_OK;
    }
    else
    {
        /* In case the device does not exist, set the error code to the
        appropriate error. */
        return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
    }
}

JRCPLIB_EXPORT_API int32_t jrcplib_device_is_connected
(
    jrcplib_controller_t *thisinst,
    uint8_t nad
)
{
    /* Check instance pointer */
    if (thisinst == nullptr)
    {
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }

    /* Get the device registry from the controller and then get the
    device pointer*/
    jrcplib::DeviceRegistry &dev_reg = thisinst->GetDeviceRegistry();
    if (nullptr != (dev_reg)[nad])
    {
        return ((dev_reg)[nad]->IsConnected())
            ? JRCPLIB_STATUS_DEVICE_CONNECTED
            : JRCPLIB_STATUS_DEVICE_DISCONNECTED;
    }
    else
    {
        /* In case the device does not exist, set the error code to the
        appropriate error. */
        return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
    }
}

JRCPLIB_EXPORT_API int32_t jrcplib_device_reset_socket(jrcplib_controller_t *thisinst, SOCKET socket_id)
{
    if (thisinst == nullptr)
    {
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }

    jrcplib::DeviceRegistry &dev_reg = thisinst->GetDeviceRegistry();
    for (size_t i = 0x00; i < dev_reg.GetRegistrySize(); i++)
    {
        auto dev = (dev_reg)[i];
        if (dev != nullptr)
        {
            bool status;
            dev->CheckAndClearSocketID(socket_id, &status);
            if (true == status)
            {
                return JRCPLIB_ERR_OK;
            }
        }
    }

    return JRCPLIB_ERR_DEVICE_RESET_SOCKET_FAILED;
}

JRCPLIB_EXPORT_API int32_t jrcplib_device_set_server_status
(
    jrcplib_controller_t *thisinst,
    uint8_t nad,
    jrcplib_jrcp_generic_status_code_t status
)
{
    if (thisinst == nullptr)
    {
        /* Check the controller instance */
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }

    /* Get the device registry from the controller and then get the
    device pointer*/
    jrcplib::DeviceRegistry &dev_reg = thisinst->GetDeviceRegistry();
    if (nullptr != (dev_reg)[nad])
    {
        /* In case the device exists, set its status to connected */
        (dev_reg)[nad]->SetDeviceServerStatus(status);
        return JRCPLIB_ERR_OK;
    }
    else
    {
        return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
    }
}

JRCPLIB_EXPORT_API int32_t jrcplib_device_get_server_status
(
    jrcplib_controller_t *thisinst,
    uint8_t nad,
    jrcplib_jrcp_generic_status_code_t *out
)
{
    if (thisinst == nullptr)
    {
        /* Check the controller instance */
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }

    /* Get the device registry from the controller and then get the
    device pointer*/
    jrcplib::DeviceRegistry &dev_reg = thisinst->GetDeviceRegistry();
    if (nullptr != (dev_reg)[nad])
    {
        /* In case the device exists, return its status */
        return jrcplib::utils::TryAssignOutParameter(out, (dev_reg)[nad]->GetDeviceServerStatus());
    }
    else
    {
        return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
    }
}

JRCPLIB_EXPORT_API int32_t jrcplib_device_set_device_busy_status
(
    jrcplib_controller_t *thisinst,
    uint8_t nad,
    bool busy_status
)
{
    if (thisinst == nullptr)
    {
        /* Check the controller instance */
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }

    /* Get the device registry from the controller and then get the
    device pointer*/
    jrcplib::DeviceRegistry &dev_reg = thisinst->GetDeviceRegistry();
    if (nullptr != (dev_reg)[nad])
    {
        /* In case the device exists, set its busy_status_ status to busy */
        (dev_reg)[nad]->SetDeviceBusyStatus(busy_status);
        return JRCPLIB_ERR_OK;
    }
    else
    {
        return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
    }
}

