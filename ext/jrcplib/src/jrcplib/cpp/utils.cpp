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
#include "../include/utils.h"

#include <climits>
#include <memory>

namespace jrcplib {
    namespace utils {

        using std::string;

        template<typename ... Args>
        extern string StrFormat(const string& format, Args ... args)
        {
            size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
        std::unique_ptr<char[]> buf(new char[size]);
            snprintf(buf.get(), size, format.c_str(), args ...);
            return string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
        }



        std::vector<uint8_t> Uint32ToBytesVec(uint32_t ui) {
            std::vector<uint8_t> vec;
            /* Little endian: LSB comes first ;) */
            vec.push_back((ui & 0x000000FF));
            vec.push_back((ui & 0x0000FF00) >> 8);
            vec.push_back((ui & 0x00FF0000) >> 16);
            vec.push_back((ui & 0xFF000000) >> 24);
            return vec;
        }

        std::vector<uint8_t> Uint32ToBEBytesVec(uint32_t ui) {
            return Uint32ToBytesVec(SwapEndian<uint32_t>(ui));
        }


        std::vector<uint8_t> Uint16ToBytesVec(uint16_t us) {
            std::vector<uint8_t> vec;
            vec.push_back((us & 0x00FF));
            vec.push_back((us & 0xFF00) >> 8);
            return vec;
        }

        std::vector<uint8_t> Uint16ToBEBytesVec(uint16_t us) {
            return Uint16ToBytesVec(SwapEndian<uint16_t>(us));
        }


        /* The purpose of this function is to check whether we can safely call a
        purely virtual function of msg.

        This function is used in messages_exported.cpp to check whether the MTY
        of a message matches its actual C++ class.

        This is done by using runtime type information (RTTI) in the form of trying to
        dynamic_cast to the class, or classes, allowed for the found MTY.

        As an example, A message with an MTY of 0 can either be of type
        `JrcpWaitForCardReqMessage` or `JrcpWaitForCardRespMessage`.
        Additionally, all messages are allowed to be of type `JrcpGenericMessage`, just
        to be on the safe side.

        If the MTY matches with any of the classes that are allowed for this MTY, such
        as the generic or one of the specialized classes, this function returns true.
        Otherwise, it returns false.
        */
        bool MessageTypeIdCheck(const jrcplib_jrcp_msg_t *msg)
        {
            /* Get the MTY out of the message. */
            uint8_t mty = msg->GetMty();

            /* If we can cast it to JrcpGenericMessage, give a green light.
            NOTE: This is also the only path where a device specific extension MTY can
            survive the check!*/
            if (dynamic_cast<const jrcplib::JrcpGenericMessage *>(msg) != nullptr)
            {
                /* In this case, all MTYs are allowed */
                return true;
            }

            /* If it has a different derived class, check individually in this switch
            statement. */
            switch (mty)
            {
            case jrcplib::kJrcpWaitForCardMty:
                /* Check wait for card request or wait for card response. */
                return dynamic_cast<const jrcplib::JrcpWaitForCardReqMessage *>(msg) != nullptr
                    || dynamic_cast<const jrcplib::JrcpWaitForCardRespMessage *>(msg) != nullptr;
            case jrcplib::kJrcpSendDataMty:
                /* Check SEND DATA (only one message class for both req/resp.) */
                return dynamic_cast<const jrcplib::JrcpSendDataMessage *>(msg) != nullptr;
            case jrcplib::kJrcpStatusMty:               /* Fall through is intentional */
            case jrcplib::kJrcpErrorMty:                /* Fall through is intentional */
            case jrcplib::kJrcpInitializationDataMty:   /* Fall through is intentional */
            case jrcplib::kJrcpInformationTextMty:      /* Fall through is intentional */
            case jrcplib::kJrcpDebugMty:                /* Fall through is intentional */
            case jrcplib::kJrcpChangeProtocolMty:       /* Fall through is intentional */
            case jrcplib::kJrcpSetTerminalParametersMty:
                /* TODO: Unsupported */
                return false;
            case jrcplib::kJrcpTerminalInformationMty:
                /* Check for terminal info request or response. */
                return dynamic_cast<const jrcplib::JrcpTerminalInfoReqMessage *>(msg) != nullptr
                    || dynamic_cast<const jrcplib::JrcpTerminalInfoRespMessage *>(msg) != nullptr;
            case jrcplib::kJrcpServerStatusMty:
                /* Check for server status request or response. */
                return dynamic_cast<const jrcplib::JrcpServerStatusReqMessage *>(msg) != nullptr
                    || dynamic_cast<const jrcplib::JrcpServerStatusRespMessage *>(msg) != nullptr;
            case jrcplib::kJrcpTimingInformationMty:
                /* Check for timing information request or response. */
                return dynamic_cast<const jrcplib::JrcpTimingInfoReqMessage *>(msg) != nullptr
                    || dynamic_cast<const jrcplib::JrcpTimingInfoRespMessage *>(msg) != nullptr;
            case jrcplib::kJrcpPrepareTearingMty:
                /* Check for prepare tearing request or response. */
                return dynamic_cast<const jrcplib::JrcpPrepareTearingReqMessage *>(msg) != nullptr
                    || dynamic_cast<const jrcplib::JrcpPrepareTearingRespMessage *>(msg) != nullptr;
            case jrcplib::kJrcpEventHandlingMty:
                /* Check for event handling request or response. */
                return dynamic_cast<const jrcplib::JrcpEventHandlingReqMessage *>(msg) != nullptr
                    || dynamic_cast<const jrcplib::JrcpEventHandlingRespMessage *>(msg) != nullptr;
            case jrcplib::kJrcpSetIoPinMty:
                /* Check for Set IO pin request or response. */
                return dynamic_cast<const jrcplib::JrcpSetIoPinReqMessage *>(msg) != nullptr
                    || dynamic_cast<const jrcplib::JrcpSetIoPinRespMessage *>(msg) != nullptr;
            case jrcplib::kJrcpWarmResetMty:
                /* Check for warm reset request or response. */
                return dynamic_cast<const jrcplib::JrcpWarmResetReqMessage *>(msg) != nullptr
                    || dynamic_cast<const jrcplib::JrcpWarmResetRespMessage *>(msg) != nullptr;
            case jrcplib::kJrcpColdResetMty:
                /* Check for cold reset request or response. */
                return dynamic_cast<const jrcplib::JrcpColdResetReqMessage *>(msg) != nullptr
                    || dynamic_cast<const jrcplib::JrcpColdResetRespMessage *>(msg) != nullptr;
            case jrcplib::kJrcpHciSendDataMty:
                /* Check for HCI send data request or response. */
                return dynamic_cast<const jrcplib::JrcpHciSendDataReqMessage *>(msg) != nullptr
                    || dynamic_cast<const jrcplib::JrcpHciSendDataRespMessage *>(msg) != nullptr;
            case jrcplib::kJrcpControllerConfigurationMty:
                /* Check for controller configuration request or response. */
                return dynamic_cast<const jrcplib::JrcpControllerReqMessage *>(msg) != nullptr
                    || dynamic_cast<const jrcplib::JrcpControllerRespMessage *>(msg) != nullptr;
            case jrcplib::kJrcpFeatureControlRequestMty:
                /* Check for feature control request or response. */
                return dynamic_cast<const jrcplib::JrcpFeatureControlReqMessage *>(msg) != nullptr
                    || dynamic_cast<const jrcplib::JrcpFeatureControlRespMessage *>(msg) != nullptr;
            default:
                /* By default return false. */
                return false;
            }
        }


    } // namespace utils
} // namespace jrcplib
