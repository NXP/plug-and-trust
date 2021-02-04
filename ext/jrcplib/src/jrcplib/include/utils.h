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

#ifndef UTILS_H_
#define UTILS_H_


#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <jrcplib/messages.h>

namespace jrcplib {
    namespace utils {
        template<typename ... Args>
        extern std::string StrFormat(const std::string& format, Args ... args);

        template <typename T> T SwapEndian(T u);


        /* Short utility to help returning the right error code from a function.

        If `out` is not null, writes value to *out and returns `JRCPLIB_ERR_OK`.
        If `out` is null, returns `JRCPLIB_ERR_INVALID_POINTER_ARGUMENT`.

        This is to help minimize the boilerplate code at the end of API functions.
        */
        template <typename T> inline int32_t TryAssignOutParameter(T *out, T value)
        {
            if (out != nullptr)
            {
                *out = value;
                return JRCPLIB_ERR_OK;
            }
            else
            {
                return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
            }
        }

        template <typename T> inline void SetOutParameterValue(T **out, std::nullptr_t value)
        {
            if (out != nullptr)
            {
                *out = value;
            }
        }

        template <typename T> inline void SetOutParameterValue(T *out, T value)
        {
            if (out != nullptr)
            {
                *out = value;
            }
        }

        std::vector<uint8_t> Uint32ToBytesVec(uint32_t ui);
        std::vector<uint8_t> Uint32ToBEBytesVec(uint32_t ui);

        std::vector<uint8_t> Uint16ToBytesVec(uint16_t us);
        std::vector<uint8_t> Uint16ToBEBytesVec(uint16_t us);

        bool MessageTypeIdCheck(const jrcplib_jrcp_msg_t *msg);

    } // namespace utils
} // namespace jrcplib

#endif // UTILS_H_
