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
#include <cstddef>

#include <jrcplib/messages.h>
#include <jrcplib/errors.h>
#include <jrcplib/device.h>
#include <jrcplib/data_types.h>
#include "../include/utils.h"

namespace jrcplib {

    using namespace utils;

    /* JrcpMessage contructor to save the MTY, NAD, Header length, Header and Payload length in the JrcpMessage Instance */
    JrcpMessage::JrcpMessage(uint8_t mty, uint8_t nad, uint8_t hdl, BytesVec header, uint32_t ln)
            :
            sof_(kJrcpMessageSofValue),
            mty_(mty),
            nad_(nad),
            hdl_(hdl),
            header_(header),
            ln_(ln),
            til_(0)
    {
        /* After changing the header, we always need to recalculate the payload
        start index. */
        RecalculatePayloadStartIndex();
    };

    /* JrcpMessage contructor to save the MTY, NAD, Header length, Header and Payload length in the JrcpMessage Instance
       based on the raw message passed */
    JrcpMessage::JrcpMessage(BytesVec raw_message)
        : sof_(kJrcpMessageSofValue) {
        /* These values can be read directly.*/
        mty_ = ParseMty(raw_message);
        nad_ = ParseNad(raw_message);
        hdl_ = ParseHdl(raw_message);
        ln_ = ParseLn(raw_message);
        til_ = ParseTil(raw_message);

        /* Create the header vector from the raw message. */
        if (hdl_ > 0)
        {
            BytesVec::const_iterator hb = raw_message.begin() + kJrcpMessageHdlIndex + 1;
            BytesVec::const_iterator he = raw_message.begin() + kJrcpMessageHdlIndex + 1 + hdl_;
            header_ = BytesVec(hb, he);
        }

        /* Calculate the payload start index now, since we just updated the header length. */
        RecalculatePayloadStartIndex();
    }

    /* Destructor for the JrcpMessage object */
    JrcpMessage::~JrcpMessage() {}

    /* Public function to set teh timing information for a jrcp message */
    bool JrcpMessage::SetTiming(uint8_t til, BytesVec tr, BytesVec tc) {
        switch (til) {
        case JRCP_NO_TIMINIG_INFO:
                // 0x00 Indicates no timing information.
                til_ = til;
                break;
        case JRCP_TIMING_INFO_WITH_RESPONSE_TIMESTAMP:
                // 0x08 Indicates that there is a response timestamp.
                til_ = til;
                if (tr.size() != JRCP_TIMING_INFO_WITH_RESPONSE_TIMESTAMP) { return false; }
                tr_ = tr;
                break;
        case JRCP_TIMING_INFO_WITH_CMD_AND_RESPONSE_TIMESTAMP:
                // 0x10 Indicates that there is a response timestamp and a command timestamp.
                til_ = til;
                if (tr.size() != JRCP_TIMING_INFO_WITH_RESPONSE_TIMESTAMP && tc.size() != JRCP_TIMING_INFO_WITH_RESPONSE_TIMESTAMP) { return false; }
                tr_ = tr;
                tc_ = tc;
                break;
            default:
                return false;
        }

        return true;
    };

    /* Public Function to set the header */
    void JrcpMessage::SetHeader(BytesVec header)
    {
        hdl_ = (uint8_t)header.size();
        header_ = header;

        /* Calculate the payload start index now. */
        RecalculatePayloadStartIndex();
    }

    /* Public function to append the preamble header in the Jrcp raw message */
    void JrcpMessage::AppendPreamble(BytesVec &rawmsg) const
    {
        /* Write SOF, MTY, NAD and HDL. */
        rawmsg.push_back(kJrcpMessageSofValue);
        rawmsg.push_back(mty_);
        rawmsg.push_back(nad_);
        rawmsg.push_back(hdl_);

        /* If the header exists, write it afterwards. This condition is just double
        checking. */
        if (hdl_ > 0 && header_.size() > 0)
        {
            rawmsg.insert(rawmsg.end(), header_.cbegin(), header_.cend());
        }
    }


    /* Public function to append the preamble header in the Jrcp raw message based on V1 format */
    void JrcpMessage::AppendPreambleWithV1Format(BytesVec &rawmsg) const
    {
        /* Write MTY and NAD. */
        rawmsg.push_back(mty_);
        rawmsg.push_back(nad_);
    }

    /* Public function to append the preamble header with payload length in the Jrcp raw message */
    void JrcpMessage::AppendPreambleWithLn(BytesVec &rawmsg) const
    {
        /* First, append the preamble as usually. */
        AppendPreamble(rawmsg);

        /* Additionally also put the LN in there */
        BytesVec lnvec = Uint32ToBEBytesVec(GetLn());
        rawmsg.insert(rawmsg.end(), lnvec.begin(), lnvec.end());
    }

    /* Public function to append the timing info in the Jrcp raw message */
    void JrcpMessage::AppendTiming(BytesVec &rawmsg) const
    {
        /* First write the TIL byte, then depending on its value the TR and/or TC,
        or neither of them.
        This is directly done on the rawmsg, therefore this function returns nothing. */
        rawmsg.push_back(til_);
        if (til_ == JRCP_TIMING_INFO_WITH_RESPONSE_TIMESTAMP)
        {
            // We have only a TR
            rawmsg.insert(rawmsg.end(), tr_.cbegin(), tr_.cend());
        }
        else if (GetTil() == JRCP_TIMING_INFO_WITH_CMD_AND_RESPONSE_TIMESTAMP)
        {
            // We have a TR and a TC, write them in this order.
            rawmsg.insert(rawmsg.end(), tr_.cbegin(), tr_.cend());
            rawmsg.insert(rawmsg.end(), tc_.cbegin(), tc_.cend());
        }
    }

    /* Public accessor for the header_ field. Copies the header for outside use. */
    BytesVec JrcpMessage::GetHeader() const {
        return header_;
    }

    /* Public accessor for the tr_ field. Copies the TR for outside use. */
    BytesVec JrcpMessage::GetTr() const
    {
        return tr_;
    }

    /* Public accessor for the tc_ field. Copies the TC for outside use. */
    BytesVec JrcpMessage::GetTc() const
    {
        return tc_;
    }

    /* Creates a standard JRCP request message based on some raw data.
     * Returns nullptr if the data did not match a known MTY. */
    JrcpMessage* JrcpMessageFactory::CreateJrcpReqMessage(BytesVec raw_message)
    {
        JrcpMessage* msg = nullptr;
        /* Switch based on the MTY and create the respective class instance. */
        uint8_t mty = ParseMty(raw_message);
        switch (mty) {
            case kJrcpWaitForCardMty:
                msg = new JrcpWaitForCardReqMessage(raw_message);
                break;
            case kJrcpSendDataMty:
                msg = new JrcpSendDataMessage(raw_message);
                break;
            case kJrcpStatusMty:                /* Fall through is intentional */
            case kJrcpErrorMty:                 /* Fall through is intentional */
            case kJrcpInitializationDataMty:    /* Fall through is intentional */
            case kJrcpInformationTextMty:       /* Fall through is intentional */
            case kJrcpDebugMty:                 /* Fall through is intentional */
            case kJrcpChangeProtocolMty:        /* Fall through is intentional */
            case kJrcpSetTerminalParametersMty:
                /* Unsupported commands */
                msg = nullptr;
                break;
            case kJrcpTerminalInformationMty:
                msg = new JrcpTerminalInfoReqMessage(raw_message);
                break;
            case kJrcpServerStatusMty:
                msg = new JrcpServerStatusReqMessage(raw_message);
                break;
            case kJrcpTimingInformationMty:
                msg = new JrcpTimingInfoReqMessage(raw_message);
                break;
            case kJrcpPrepareTearingMty:
                msg = new JrcpPrepareTearingReqMessage(raw_message);
                break;
            case kJrcpEventHandlingMty:
                msg = new JrcpEventHandlingReqMessage(raw_message);
                break;
            case kJrcpSetIoPinMty:
                msg = new JrcpSetIoPinReqMessage(raw_message);
                break;
            case kJrcpWarmResetMty:
                msg = new JrcpWarmResetReqMessage(raw_message);
                break;
            case kJrcpColdResetMty:
                msg = new JrcpColdResetReqMessage(raw_message);
                break;
            case kJrcpHciSendDataMty:
                msg = new JrcpHciSendDataReqMessage(raw_message);
                break;
            case kJrcpControllerConfigurationMty:
                msg = new JrcpControllerReqMessage(raw_message);
                break;
            case kJrcpFeatureControlRequestMty:
                msg = new JrcpFeatureControlReqMessage(raw_message);
                break;
        }

        /* The device specific extensions are handled especially since they have an entire
        range of MTYs associated with them.  instead of just one */
        if (mty >= kJrcpDeviceSpecificExtensionsFirst && mty <= kJrcpDeviceSpecificExtensionsLast)
        {
            msg = new JrcpGenericMessage(raw_message);
        }

        if (msg != nullptr && msg->IsMessageWellFormed() == true) {
            return msg;
        }

        /* if message is not well formed we return nullptr
         * and caller will handle error JRCPLIB_ERR_MALFORMED_MESSAGE */
        if (msg != nullptr && msg->IsMessageWellFormed() == false) {
            /* Do cleanup */
            delete msg;
            msg = nullptr;
        }

        return msg;
    };

    /* Factory method to create a JRCP response message. Currently not implemented. */
    JrcpMessage* JrcpMessageFactory::CreateJrcpRespMessage(JrcpMessageType message_type, BytesVec payload)
    {
        return nullptr;
    };

} // namespace jrcp


