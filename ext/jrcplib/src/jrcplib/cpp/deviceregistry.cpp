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
#include <jrcplib/deviceregistry.h>
#include <jrcplib/device.h>

#include <cassert>

namespace jrcplib
{
    std::shared_ptr<Device> DeviceRegistry::operator[](std::size_t idx)
    {
        /* Bounds check! */
        if (idx <= 0xFF)
        {
            /* Left hand side of && is sequenced before right hand side!
            See: http://en.cppreference.com/w/cpp/language/eval_order */
            if (registry_[idx] == nullptr)
            {
                /* If the device exists, but is hidden, return NULL. */
                return nullptr;
            }
            /* If the device exists but is not hidden, or does not exist,
            return whatever we have for it in the registry. */
            return registry_[idx];
        }
        /* If the NAD failed the bounds check, return NULL. */
        return nullptr;
    }

    std::shared_ptr<const Device> DeviceRegistry::operator[](std::size_t idx) const
    {
        /* Bounds check! */
        if (idx <= 0xFF)
        {
            /* Left hand side of && is sequenced before right hand side!
            See: http://en.cppreference.com/w/cpp/language/eval_order */
            if (registry_[idx] == nullptr)
            {
                /* If the device exists, but is hidden, return NULL. */
                return nullptr;
            }
            /* If the device exists but is not hidden, or does not exist,
            return whatever we have for it in the registry. */
            return registry_[idx];
        }
        /* If the NAD failed the bounds check, return NULL. */
        return nullptr;
    }

    /* Public method to add a device to the device registry */
    int32_t DeviceRegistry::AddDevice(std::shared_ptr<Device> device)
    {
        uint8_t nad = device->GetDeviceNad();
        assert(device != nullptr);
        if (registry_[nad] != nullptr) {
            return JRCPLIB_ERR_NAD_IN_USE;
        }
        registry_[nad] = device;
        return JRCPLIB_ERR_OK;
    }

    /* Public method to add a device to the device registry along with the default handlers */
    int32_t DeviceRegistry::AddDeviceWithDefaultHandlers(jrcplib_protocol_supported_t protocol_version, std::shared_ptr<Device> device)
    {
        int32_t status = false;

        if (JRCPLIB_ERR_OK == status)
        {
            /* Device object is created. Add the default message handlers */
            std::map<uint8_t, jrcplib::FeatureHandlerFunc>handler_table = {
                { jrcplib::kJrcpFeatureControlRequestMty, jrcplib::FeatureControlRequestHandler },
                { jrcplib::kJrcpServerStatusMty, jrcplib::ServerStatusHandler },
                { jrcplib::kJrcpTerminalInformationMty, jrcplib::TerminalInfoHandler }
            };

            /* Register all the feature handlers from above. */
            for (auto idx = handler_table.begin(); idx != handler_table.end(); ++idx)
            {
                jrcplib::FeatureHandler fh = std::make_pair(protocol_version, idx->second);
                status = device->RegisterFeatureHandler(idx->first, fh);
                if (JRCPLIB_ERR_OK != status)
                {
                    break;
                }
            }
        }
        return (JRCPLIB_ERR_OK == status) ? this->AddDevice(device) : status;
    }

    /* Public method to remove a device from the device registry */
    int32_t DeviceRegistry::RemoveDevice(std::shared_ptr<const Device> device)
    {
        uint8_t nad = device->GetDeviceNad();
        assert(device != nullptr);
        return RemoveDevice(nad);
    }

    /* Public method to remove a device from device registry */
    int32_t DeviceRegistry::RemoveDevice(uint8_t nad) {
        if (registry_[nad] == nullptr) {
            return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
        }
        registry_[nad] = nullptr;
        return JRCPLIB_ERR_OK;
    }
}
