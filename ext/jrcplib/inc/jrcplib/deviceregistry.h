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
#ifndef JRCPLIB_DEVICEREGISTRY_H_
#define JRCPLIB_DEVICEREGISTRY_H_
/** @file device.h */

#ifdef __cplusplus

#include <jrcplib/data_types.h>
#include <memory>
#include <cstdint>

#define JRCP_SERVER_NAD   0xFF
#define MAX_JRCP_DEVICE_ENTRY 256

namespace jrcplib
{
    class Device;

    class DeviceRegistry
    {
    public:

        /** Initializing the registry with 256 placeholders.
        This is the maximum number of NADs (node addresses) which can be
        encoded in JRCP messages, because NADs are 8-bit unsigned integers. */
        DeviceRegistry()
            : registry_(std::vector<std::shared_ptr<Device>>(MAX_JRCP_DEVICE_ENTRY, nullptr))
        {
        };

        /* Get a pointer to a device in this registry, by the nad.
        If the device is hidden, it will not show up in here.*/
        std::shared_ptr<Device> operator[](std::size_t idx);

        /* Get a pointer to an immutable device in this registry, by the nad.
        If the device is hidden, it will not show up in here.*/
        std::shared_ptr<const Device> operator[](std::size_t idx) const;

        /* Returns whether the given NAD is already occupied. Also accounts for
        the hidden devices. */
        inline bool IsNADInUse(std::size_t idx)
        {
            return idx <= JRCP_SERVER_NAD
                ? (registry_[idx] != nullptr)
                : false;
        }

        /* Get the size of the registry. Actually this is the maximum index! */
        inline size_t GetRegistrySize() const
        {
            return (MAX_JRCP_DEVICE_ENTRY - 1);
        }

        int32_t AddDevice(std::shared_ptr<Device> device);
        int32_t AddDeviceWithDefaultHandlers(jrcplib_protocol_supported_t protocol_version, std::shared_ptr<Device> device);
        int32_t RemoveDevice(std::shared_ptr<const Device> device);
        int32_t RemoveDevice(uint8_t nad);

    private:
        /* An array of smart pointers for the devices, indexed by NAD. */
        std::vector<std::shared_ptr<Device>> registry_;
    };
}
#endif

#endif
