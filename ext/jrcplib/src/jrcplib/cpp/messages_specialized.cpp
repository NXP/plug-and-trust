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
#include <jrcplib/messages.h>
#include <jrcplib/errors.h>
#include <jrcplib/device.h>
#include "../include/utils.h"

namespace jrcplib
{
    using namespace utils;

    /**
    * Inline function to parse the sub-MTY from the request payload
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  Two bytes sub-mty value
    *
    */
    inline uint16_t ParseSubMty(BytesVec raw_message)
    {
        /* In order to get the sub-MTY, take the first two bytes of the payload.
        Again, they're in big-endian, so convert to little-endian before returning. */
        uint32_t payload_start = kJrcpMessageHdlIndex + ParseHdl(raw_message) + JRCP_SUB_MTY_OFFSET;
        uint16_t besubmty = *reinterpret_cast<const uint16_t*> (&raw_message[payload_start]);
        return SwapEndian<uint16_t>(besubmty);
    }

    /**
    * Constructor for the wait for card request message object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpWaitForCardReqMessage object
    *
    */
    JrcpWaitForCardReqMessage::JrcpWaitForCardReqMessage(const BytesVec& raw_message)
        : JrcpMessage(raw_message)
    {
        uint32_t bwaitingtime = *reinterpret_cast<const uint32_t*>
            (&raw_message[GetPayloadStartIndex()]);
        waiting_time_ms = SwapEndian<uint32_t>(bwaitingtime);
    }

    /**
    * Constructor for the wait for card request message object
    *
    * @param nad The NAD value
    * @param bwaitingtime The waiting time in milli seconds
    *
    * @return  JrcpWaitForCardReqMessage object
    *
    */
    JrcpWaitForCardReqMessage::JrcpWaitForCardReqMessage(uint8_t nad, uint32_t bwaitingtime)
        : JrcpMessage(kJrcpWaitForCardMty, nad, 0, {}, JRCP_SIZE_OF_PAYLOAD_LENGTH),
        waiting_time_ms(bwaitingtime)
    {

    }

    /* Destructor for the wait for card request message object */
    JrcpWaitForCardReqMessage::~JrcpWaitForCardReqMessage()
    {

    }

    /**
    * Overridden method to get the raw message from the wait for card request object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using wait for card request object
    *
    */
    BytesVec JrcpWaitForCardReqMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, and Header (if available) are written to rawmsg here. */
        AppendPreamble(rawmsg);
        // Length 4 bytes
        rawmsg.push_back(0x00);
        rawmsg.push_back(0x00);
        rawmsg.push_back(0x00);
        rawmsg.push_back(JRCP_WAIT_FOR_CARD_RESPONSE_PAYLOAD_LENGTH);

        /* The payload, consisting of the waiting time during which we wait for
        a target to be detected. */
        BytesVec timevec = Uint32ToBEBytesVec(waiting_time_ms);
        rawmsg.insert(rawmsg.end(), timevec.begin(), timevec.end());

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }


    /**
    *  Constructor for the Wait for card response message containing the ATR value
    *
    * @param nad The NAD value of the device for which the ATR is requested
    * @param atr The ATR response
    *
    * @return  JrcpWaitForCardRespMessage object having the ATR
    *
    */
    JrcpWaitForCardRespMessage::JrcpWaitForCardRespMessage(uint8_t nad, const BytesVec& atr)
        : JrcpMessage(kJrcpWaitForCardMty, nad, 0, {}, (uint32_t)atr.size()), atr_(atr)
    {
    }


    /* Destructor for the Wait for card response message */
    JrcpWaitForCardRespMessage::~JrcpWaitForCardRespMessage()
    {

    }

    /**
    * Overridden method to get the raw message from the wait for card response object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using wait for card response object
    *
    */
    BytesVec JrcpWaitForCardRespMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        rawmsg.insert(rawmsg.end(), atr_.begin(), atr_.end());

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }


    /**
    * Constructor for the JrcpSendDataMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpSendDataMessage object
    *
    */
    JrcpSendDataMessage::JrcpSendDataMessage(BytesVec raw_message)
        : JrcpMessage(raw_message)
    {
        data_.insert(data_.end(),
            raw_message.begin() + GetPayloadStartIndex(),
            raw_message.begin() + GetPayloadStartIndex() + GetLn());
    }

    /* Destructor for the JrcpSendDataMessage class  */
    JrcpSendDataMessage::~JrcpSendDataMessage()
    {
    }

    /**
    *  Constructor to create the JrcpSendDataMessage based on the device nad and payload in the
    *  request message received by the network layer
    *
    * @param nad The NAD value of the device for which the send data is requested
    * @param payload The response payload
    *
    * @return  JrcpSendDataMessage object having the Send Data response
    *
    */
    JrcpSendDataMessage::JrcpSendDataMessage(uint8_t nad, BytesVec payload)
        : JrcpMessage(kJrcpSendDataMty, nad, 0, {}, (uint32_t)payload.size()), data_(payload)
    {
    }

    /**
    * Overridden method to get the raw message from the JrcpSendDataMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpSendDataMessage object
    *
    */
    BytesVec JrcpSendDataMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        rawmsg.insert(rawmsg.end(), data_.begin(), data_.end());

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }

    /**
    * Process the Terminal request MTY.
    *
    * @param instance The JRCP controller instance
    * @param req_msg The terminal info request message instance
    * @param out The pointer which receives the response instance to this MTY
    *
    * @return Error code if something went wrong or OK otherwise
    *
    * @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED
    *           if the MTY of the request message does not match to any class
    *           casting of the controller config MTY fails
    * @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED
    *           iff invalid NAD is received (not registered in the controller instance)
    */
    int32_t TerminalInfoHandler(JrcpController *instance, JrcpMessage * req_msg, JrcpMessage **out)
    {
        BytesVec resp_payload;

        /* Perform the downcast to the Terminal Info Request message class */
        const jrcplib::JrcpTerminalInfoReqMessage *terminal_info = dynamic_cast<const jrcplib::JrcpTerminalInfoReqMessage *>(req_msg);

        /* Check if the message can be meaningfully cast down. */
        if (terminal_info == nullptr)
        {
            /* Return error code if casting failed. Generic message will be send as a response
             * Handling of this is done in the callee wrapper function */
            return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
        }

        /* Get the device registry */
        DeviceRegistry &devreg = instance->GetDeviceRegistry();
        uint8_t nad = req_msg->GetNad();
        /* Check whether the device is registered or not */
        if (nullptr == (devreg)[nad])
        {
            /* Return error code if requested device is not registered. Generic message will be send as a response
             * Handling of this is done in the callee wrapper function */
            return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
        }

        /* Construct the actual response */
        std::string desc = (devreg)[nad]->GetDeviceDescription();
        resp_payload.insert(resp_payload.end(), desc.begin(), desc.end());
        auto terminfomsg = new jrcplib::JrcpTerminalInfoRespMessage(nad, resp_payload);

        /* Check for the JRCP format and set it to the response object */
        terminfomsg->SetJrcpV1Format(terminal_info->IsJrcpV1Format());
        *out = terminfomsg;
        /* Set the device server status */
        (devreg)[nad]->SetDeviceServerStatus(JrcpGenericStatusCodes::GenericOK);
        /* Return with the result  in out. */
        return JRCPLIB_ERR_OK;
    }


    /**
    * Constructor for the JrcpTerminalInfoReqMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpTerminalInfoReqMessage object
    *
    */
    JrcpTerminalInfoReqMessage::JrcpTerminalInfoReqMessage(const BytesVec& raw_message)
        : JrcpMessage(raw_message)
    {
        // Nothing to do
    }

    /**
    * Constructor for the JrcpTerminalInfoReqMessage object
    *
    * @param nad The NAD value
    *
    * @return  JrcpTerminalInfoReqMessage object
    *
    */
    JrcpTerminalInfoReqMessage::JrcpTerminalInfoReqMessage(uint8_t nad)
        : JrcpMessage(kJrcpTerminalInformationMty, nad, 0, {}, 0)
    {
       // Nothing to do
    }

    /* Destructor for JrcpTerminalInfoReqMessage object */
    JrcpTerminalInfoReqMessage::~JrcpTerminalInfoReqMessage()
    {
    }


    /**
    * Overridden method to get the raw message from the JrcpTerminalInfoReqMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpTerminalInfoReqMessage object
    *
    */
    BytesVec JrcpTerminalInfoReqMessage::GetRawMessage() const
    {
        BytesVec rawmsg;

        if (false == is_v1_) {
            /* SOF, MTY, NAD, HDL, and Header (if available) are written to rawmsg here. */
            AppendPreamble(rawmsg);
            // Length 4 bytes
            rawmsg.push_back(0x00);
            rawmsg.push_back(0x00);
            rawmsg.push_back(0x00);
            rawmsg.push_back(0x00);
            /* TIL, TR, TC are written to rawmsg here. */
            AppendTiming(rawmsg);
        }
        else {
            AppendPreambleWithV1Format(rawmsg);
            rawmsg.push_back(0x00);
            rawmsg.push_back(0x00);
        }

        return rawmsg;
    }


    /**
    *  Constructor to create the terminal info response message
    *
    * @param nad The NAD value of the device for which the terminal info is requested
    * @param term_info The response terminal info string
    *
    * @return  JrcpTerminalInfoRespMessage object having the terminal info response
    *
    */
    JrcpTerminalInfoRespMessage::JrcpTerminalInfoRespMessage(uint8_t nad, const BytesVec& term_info)
        : JrcpMessage(kJrcpTerminalInformationMty, nad, 0, {}, (uint32_t)term_info.size()), term_info_(term_info)
    {
    }

    /* Destructor for the JrcpTerminalInfoRespMessage object */
    JrcpTerminalInfoRespMessage::~JrcpTerminalInfoRespMessage()
    {

    }

    /**
    * Overridden method to get the raw message from the JrcpTerminalInfoRespMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpTerminalInfoRespMessage object
    *
    */
    BytesVec JrcpTerminalInfoRespMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* Check for the JRCP version format and frame the message accordingly */
        if (false == is_v1_)
        {
            /* JRCP V2 format */
            /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
            AppendPreambleWithLn(rawmsg);

            rawmsg.insert(rawmsg.end(), term_info_.begin(), term_info_.end());

            /* TIL, TR, TC are written to rawmsg here. */
            AppendTiming(rawmsg);
        }
        else
        {
            /* JRCP V1 format */
            AppendPreambleWithV1Format(rawmsg);
            BytesVec length = Uint16ToBEBytesVec((uint16_t)term_info_.size());
            rawmsg.insert(rawmsg.end(), length.begin(), length.end());
            rawmsg.insert(rawmsg.end(), term_info_.begin(), term_info_.end());
        }
        return rawmsg;
    }


    /**
    * Constructor for theJrcpServerStatusReqMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpServerStatusReqMessage object
    *
    */
    JrcpServerStatusReqMessage::JrcpServerStatusReqMessage(const BytesVec& raw_message)
        : JrcpMessage(raw_message)
    {
        // Nothing to do
    }

    /**
    * Constructor for theJrcpServerStatusReqMessage object
    *
    * @param nad The NAD value
    *
    * @return  JrcpServerStatusReqMessage object
    *
    */
    JrcpServerStatusReqMessage::JrcpServerStatusReqMessage(uint8_t nad)
        : JrcpMessage(kJrcpServerStatusMty, nad, 0, {}, 0)
    {
        // Nothing to do
    }

    /**
    * Overridden method to get the raw message from the JrcpServerStatusReqMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpServerStatusReqMessage object
    *
    */
    BytesVec JrcpServerStatusReqMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, and Header (if available) are written to rawmsg here. */
        AppendPreamble(rawmsg);

        // Length 4 bytes
        rawmsg.push_back(0x00);
        rawmsg.push_back(0x00);
        rawmsg.push_back(0x00);
        rawmsg.push_back(0x00);

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }

    /* Destructor for JrcpServerStatusReqMessage object */
    JrcpServerStatusReqMessage::~JrcpServerStatusReqMessage()
    {
    }

    /**
    *  Contructor to create the server status response message
    *
    * @param nad The NAD value of the device for which the server status is requested
    * @param status The JRCP srever status code for the NAD to create the response
    * @param ascii_msg The associated string description of the srever status
    *
    * @return  JrcpServerStatusRespMessage object having the server status response
    *
    */
    JrcpServerStatusRespMessage::JrcpServerStatusRespMessage(uint8_t nad, JrcpGenericStatusCodes status, BytesVec ascii_msg)
        : JrcpMessage(kJrcpServerStatusMty, nad, 0, {}, JRCP_SIZE_OF_PAYLOAD_LENGTH + (uint32_t)ascii_msg.size())
    {
        if (GenericStatusCodeTypeCheck::is_value(status) == false)
        {
            status = JrcpGenericStatusCodes::GenericError;
        }
        status_code_ = status;
        ascii_msg_ = ascii_msg;
    }

    /**
    * Process the Server status request MTY.
    *
    * @param instance The JRCP controller instance
    * @param req_msg The server status request message instance
    * @param out The pointer which receives the response instance to this MTY
    *
    * @return Error code if something went wrong or OK otherwise
    *
    * @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED
    *           if the MTY of the request message does not match to any class
    *           casting of the controller config MTY fails
    */
    int32_t ServerStatusHandler(JrcpController *instance, JrcpMessage * req_msg, JrcpMessage **out)
    {
        uint8_t nad = 0x00;
        /* Perform the downcast to the Feature Control Request messsage class */
        const jrcplib::JrcpServerStatusReqMessage *server_status = dynamic_cast<const jrcplib::JrcpServerStatusReqMessage *>(req_msg);

        /* Check if the message can be meaningfully cast down. */
        if (server_status == nullptr)
        {
            /* Return error code if casting failed. Generic message will be send as a response
            * Handling of this is done in the callee wrapper function */
            return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
        }

        /* Construct the result and return immediately with OK */
        nad = req_msg->GetNad();

        /* These are just double checks. Using the library implementation as of
        right now this cannot occur. */
        if (instance == nullptr)
        {
            return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
        }

        /* Check that the device even exists before we try to access it. */
        if (instance->GetDeviceRegistry()[nad] == nullptr)
        {
            return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
        }

        /* Do not use the hidden device registry, use the normal one. A device
        that is not listed in ListReaders cannot have a status. */
        *out = new jrcplib::JrcpServerStatusRespMessage(instance->GetDeviceRegistry()[nad]->GetDeviceServerStatus(), nad);
        return JRCPLIB_ERR_OK;
    }

    /**
    *  Contructor to create the server status response message
    *
    * @param status The JRCP srever status code for the NAD to create the response
    * @param nad The NAD value of the device for which the ATR is requested
    *
    * @return  JrcpServerStatusRespMessage object having the server status response
    *
    */
    JrcpServerStatusRespMessage::JrcpServerStatusRespMessage(JrcpGenericStatusCodes status, uint8_t nad)
        : JrcpMessage(kJrcpServerStatusMty, nad, 0, {}, JRCP_SERVER_STATUS_CODE_SIZE)
    {
        if (GenericStatusCodeTypeCheck::is_value(status) == false)
        {
            status = JrcpGenericStatusCodes::GenericError;
        }
        status_code_ = status;
    }


    /**
    * Overridden method to get the raw message from the JrcpServerStatusRespMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpServerStatusRespMessage object
    *
    */
    BytesVec JrcpServerStatusRespMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        BytesVec statusvec = Uint16ToBEBytesVec(status_code_);
        rawmsg.insert(rawmsg.end(), statusvec.cbegin(), statusvec.cend());

        rawmsg.insert(rawmsg.end(), ascii_msg_.cbegin(), ascii_msg_.cend());

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }

    /* Destructor for JrcpServerStatusRespMessage object */
    JrcpServerStatusRespMessage::~JrcpServerStatusRespMessage()
    {
    }

    /**
    * Constructor for theJrcpTimingInfoReqMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpTimingInfoReqMessage object
    *
    */
    JrcpTimingInfoReqMessage::JrcpTimingInfoReqMessage(const BytesVec& raw_message)
        : JrcpMessage(raw_message)
    {

        uint16_t submty = ParseSubMty(raw_message);
        if (TimingInfoTypeCheck::is_value(submty) != true)
        {
            /* Do not initialize */
            SetMessageWellFormed(false);
            return;
        }
        else
        {
            action_ = static_cast<TimingInfoType>(submty);
        }

        if (action_ == TimingInfoType::TimingSetOptions)
        {
            payload_.insert(payload_.end(),
                raw_message.begin() + GetPayloadStartIndex() + JRCP_SIZE_OF_SUB_MTY_LENGTH,
                raw_message.end() - 1); /* submty*/
        }
    }

    /**
    * Constructor for theJrcpTimingInfoReqMessage object
    *
    * @param nad The NAD value
    * @param submty The sub MTY defining timer options
    * @param payload The payload defining actions
    *
    * @return  JrcpTimingInfoReqMessage object
    *
    */
    JrcpTimingInfoReqMessage::JrcpTimingInfoReqMessage(uint8_t nad, TimingInfoType submty, const BytesVec& payload)
        : JrcpMessage(kJrcpTimingInformationMty, nad, 0, {}, JRCP_SIZE_OF_SUB_MTY_LENGTH + (uint32_t)payload.size())
    {
        if (TimingInfoTypeCheck::is_value(submty) != true)
        {
            /* Do not initialize */
            SetMessageWellFormed(false);
            return;
        }
        else
        {
            action_ = static_cast<TimingInfoType>(submty);
        }

        if (action_ == TimingInfoType::TimingSetOptions)
        {
            payload_ = payload;
        }
    }

    /* Destructor for JrcpTimingInfoReqMessage object */
    JrcpTimingInfoReqMessage::~JrcpTimingInfoReqMessage()
    {
    }


    /**
    * Overridden method to get the rew message from the JrcpTimingInfoReqMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpTimingInfoReqMessage object
    *
    */
    BytesVec JrcpTimingInfoReqMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        BytesVec actvec = Uint16ToBEBytesVec(static_cast<uint16_t>(action_));
        rawmsg.insert(rawmsg.end(), actvec.begin(), actvec.end());

        if (!payload_.empty())
        {
            rawmsg.insert(rawmsg.end(), payload_.begin(), payload_.end());
        }

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }

    /**
    *  Contructor to create the timing info response message
    *
    * @param nad The NAD value of the device for which the timing info is requested
    * @param payload The JRCP response paylod
    * @param response_type The JRCP timing info resposne type
    * @param status The JRCP timing status code
    *
    * @return  JrcpTimingInfoRespMessage object having the timing info response
    *
    */
    JrcpTimingInfoRespMessage::JrcpTimingInfoRespMessage(uint8_t nad,
        BytesVec payload,
        TimingInfoType response_type,
        TimingStatusCodes status)
        : JrcpMessage(kJrcpTimingInformationMty, nad, 0, {}, payload.empty() ? JRCP_TIMING_INFO_SIZE : ((uint32_t)payload.size() + JRCP_SIZE_OF_SUB_MTY_LENGTH)),
        payload_(payload)
    {

        status_ = status;
        response_type_ = response_type;

        // Only certain types of combinations of response type and payload length
        // are allowed. Check the parameters and return a default error message on
        // failure.
        bool check = false;
        switch (response_type_)
        {
        case TimingStatusCode:
            check = true;
            break;
        case TimingSetOptions: /* Fall through is intentional */
        case TimingResetTimer:
            /* Doesn't make sense for a response. */
            check = true;
            break;
        case TimingGetCurrentOptions: /* Fall through is intentional */
        case TimingGetAvailableOptions:
            check = (payload.size() != JRCP_TIMING_INFO_SIZE) ? false : true;
            break;
        case TimingQueryTimer:
            check = (payload.size() != (JRCP_TIMING_INFO_SIZE * 2)) ? false : true;
            break;
        default:
            check = false;
        }

        if (check == false)
        {
            response_type_ = TimingStatusCode;
            status_ = TimingError;
        }
    }


    /* Destructor for JrcpTimingInfoRespMessage object */
    JrcpTimingInfoRespMessage::~JrcpTimingInfoRespMessage()
    {
    }


    /**
    * Overridden method to get the rew message from the JrcpTimingInfoRespMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpTimingInfoRespMessage object
    *
    */
    BytesVec JrcpTimingInfoRespMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, and Header (if available) are written to rawmsg here. */
        AppendPreamble(rawmsg);

        uint32_t payLoadSize = payload_.empty() ? JRCP_TIMING_INFO_SIZE : (JRCP_SIZE_OF_SUB_MTY_LENGTH + (uint32_t)payload_.size());

        BytesVec lnvec = Uint32ToBEBytesVec(payLoadSize);
        rawmsg.insert(rawmsg.end(), lnvec.begin(), lnvec.end());

        // A response cannot have SetOptions or ResetTimer as a sub-MTY
        // => Reset to 0 in those cases.
        uint16_t sum_mty = 0;
        switch (response_type_)
        {
        case TimingInfoType::TimingSetOptions: /* Fall through is intentional */
        case TimingInfoType::TimingResetTimer:
            sum_mty = 0;
            break;
        default:
            sum_mty = response_type_;
            break;
        }
        BytesVec submtyvec = Uint16ToBEBytesVec(sum_mty);
        rawmsg.insert(rawmsg.end(), submtyvec.cbegin(), submtyvec.cend());

        if (!payload_.empty())
        {
            rawmsg.insert(rawmsg.end(), payload_.begin(), payload_.end());
        }
        else
        {
            BytesVec stvec = Uint16ToBEBytesVec(static_cast<uint16_t>(status_));
            rawmsg.insert(rawmsg.end(), stvec.begin(), stvec.end());
        }

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }


    /**
    * Contructor for the JrcpPrepareTearingReqMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpPrepareTearingReqMessage object
    *
    */
    JrcpPrepareTearingReqMessage::JrcpPrepareTearingReqMessage(const BytesVec& raw_message)
        : JrcpMessage(raw_message),
        action_(TearingAction::TearingActionUnknown),
        device_specific_action_(0)
    {
        uint16_t submty = ParseSubMty(raw_message);
        TearingAction action = static_cast<TearingAction>(submty);
        if (TearingActionCheck::is_value(submty) != true)
        {
            // Verify is it in range of Device Specific actions
            if (action >= TearingAction::TearingActionDeviceSpecificStart &&
                action <= TearingAction::TearingActionDeviceSpecificEnd)
            {
                device_specific_action_ = submty;
            }
            else
            {
                SetMessageWellFormed(false);
            }
        }
        else
        {
            action_ = action;
        }

        // Contains more than just 2 bytes SubMTY
        if (GetLn() > JRCP_SIZE_OF_SUB_MTY_LENGTH)
        {
            payload_.insert(payload_.end(),
                raw_message.begin() + GetPayloadStartIndex() + JRCP_SIZE_OF_SUB_MTY_LENGTH,
                raw_message.begin() + GetPayloadStartIndex() + GetLn());
        }
    }

    /**
    * Contructor for the JrcpPrepareTearingReqMessage object
    *
    * @param nad The NAD value
    * @param submty The sub MTY defining tearing options
    * @param payload The payload defining actions
    *
    * @return  JrcpPrepareTearingReqMessage object
    *
    */
    JrcpPrepareTearingReqMessage::JrcpPrepareTearingReqMessage(uint8_t nad, TearingAction submty, const BytesVec& payload)
        : JrcpMessage(kJrcpPrepareTearingMty, nad, 0, {}, JRCP_SIZE_OF_SUB_MTY_LENGTH + (uint32_t)payload.size()),
        action_(TearingAction::TearingActionUnknown),
        device_specific_action_(0),
    payload_(payload)
    {
        TearingAction action = static_cast<TearingAction>(submty);
        if (TearingActionCheck::is_value(submty) != true)
        {
            // Verify is it in range of Device Specific actions
            if (action >= TearingAction::TearingActionDeviceSpecificStart &&
                action <= TearingAction::TearingActionDeviceSpecificEnd)
            {
                device_specific_action_ = submty;
            }
            else
            {
                SetMessageWellFormed(false);
            }
        }
        else
        {
            action_ = action;
        }
    }

    /* Destructor for JrcpPrepareTearingReqMessage object */
    JrcpPrepareTearingReqMessage::~JrcpPrepareTearingReqMessage()
    {
    }


    /**
    * Overridden method to get the raw message from the JrcpPrepareTearingReqMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpPrepareTearingReqMessage object
    *
    */
    BytesVec JrcpPrepareTearingReqMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        if (action_ != TearingAction::TearingActionUnknown)
        {
            BytesVec actvec = Uint16ToBEBytesVec(static_cast<uint16_t>(action_));
            rawmsg.insert(rawmsg.end(), actvec.begin(), actvec.end());
        }
        else if (device_specific_action_ != 0)
        {
            BytesVec devactvec = Uint16ToBEBytesVec(device_specific_action_);
            rawmsg.insert(rawmsg.end(), devactvec.begin(), devactvec.end());
        }

        if (!payload_.empty())
        {
            rawmsg.insert(rawmsg.end(), payload_.begin(), payload_.end());
        }

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }

    /**
    *  Contructor to create the prepare tearing response message
    *
    * @param nad The NAD value of the device for which the prepare tearing is requested
    * @param status The JRCP tearing status code
    *
    * @return  JrcpPrepareTearingRespMessage object having the prepare tearing response
    *
    */
    JrcpPrepareTearingRespMessage::JrcpPrepareTearingRespMessage(uint8_t nad,
        TearingStatusCodes status)
        : JrcpMessage(kJrcpPrepareTearingMty, nad, 0, {}, JRCP_SIZE_OF_SUB_MTY_LENGTH + JRCP_TEARING_STATUS_CODE_SIZE /* 2 bytes SubMty + 2 bytes Status code*/)
    {
        submty_ = 0x0000;
        status_ = status;
    }

    /* Destructor for JrcpPrepareTearingRespMessage object */
    JrcpPrepareTearingRespMessage::~JrcpPrepareTearingRespMessage()
    {

    }


    /**
    * Overridden method to get the rew message from the JrcpPrepareTearingRespMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpPrepareTearingRespMessage object
    *
    */
    BytesVec JrcpPrepareTearingRespMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        // SubMTY is always 0x0000, but take it from the member variable.
        BytesVec submtyvec = Uint16ToBEBytesVec(submty_);
        rawmsg.insert(rawmsg.end(), submtyvec.cbegin(), submtyvec.cend());

        // Status
        BytesVec stvec = Uint16ToBEBytesVec(static_cast<uint16_t>(status_));
        rawmsg.insert(rawmsg.end(), stvec.begin(), stvec.end());

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }

    /**
    * Contructor for the JrcpEventHandlingReqMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpEventHandlingReqMessage object
    *
    */
    JrcpEventHandlingReqMessage::JrcpEventHandlingReqMessage(const BytesVec& raw_message)
        : JrcpMessage(raw_message)
    {
        uint16_t submty = ParseSubMty(raw_message);
        if (EventRequestTypeCheck::is_value(submty))
        {
            reqtype_ = static_cast<EventRequestType>(submty);
        }
        else
        {
            SetMessageWellFormed(false);
        }
    }

    /**
    * Contructor for the JrcpEventHandlingReqMessage object
    *
    * @param nad The NAD value
    * @param submty The value that determines request type
    *
    * @return  JrcpEventHandlingReqMessage object
    *
    */
    JrcpEventHandlingReqMessage::JrcpEventHandlingReqMessage(uint8_t nad, EventRequestType submty)
        : JrcpMessage(kJrcpEventHandlingMty, nad, 0, {}, JRCP_SIZE_OF_SUB_MTY_LENGTH)
    {
        if (EventRequestTypeCheck::is_value(submty))
        {
            reqtype_ = static_cast<EventRequestType>(submty);
        }
        else
        {
            SetMessageWellFormed(false);
        }
    }


    /* Destructor for JrcpEventHandlingReqMessage object */
    JrcpEventHandlingReqMessage::~JrcpEventHandlingReqMessage()
    {
    }

    /**
    * Overridden method to get the rew message from the JrcpEventHandlingReqMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpEventHandlingReqMessage object
    *
    */
    BytesVec JrcpEventHandlingReqMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        BytesVec rtvec = Uint16ToBEBytesVec(static_cast<uint16_t>(reqtype_));
        rawmsg.insert(rawmsg.end(), rtvec.begin(), rtvec.end());

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);
        return rawmsg;
    }

    /**
    *  Contructor to create the event handling response message
    *
    * @param nad The NAD value of the device for which the event handling is requested
    * @param resptype The JRCP event response type
    * @param payload The JRCP response payload
    *
    * @return  JrcpEventHandlingRespMessage object having the event handling response
    *
    */
    JrcpEventHandlingRespMessage::JrcpEventHandlingRespMessage(uint8_t nad,
        EventResponseType resptype,
        const BytesVec& payload)
        : JrcpMessage(kJrcpEventHandlingMty, nad, 0, {}, JRCP_SIZE_OF_SUB_MTY_LENGTH + (uint32_t)payload.size() /* submty + payload */),
    resptype_(resptype),
    payload_(payload)
    {
    }

    /* Destructor for JrcpEventHandlingRespMessage object */
    JrcpEventHandlingRespMessage::~JrcpEventHandlingRespMessage()
    {
    }


    /**
    * Overridden method to get the raw message from the JrcpEventHandlingRespMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpEventHandlingRespMessage object
    *
    */
    BytesVec JrcpEventHandlingRespMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        BytesVec rtvec = Uint16ToBEBytesVec(static_cast<uint16_t>(resptype_));
        rawmsg.insert(rawmsg.end(), rtvec.begin(), rtvec.end());

        if (!payload_.empty())
        {
            rawmsg.insert(rawmsg.end(), payload_.begin(), payload_.end());
        }

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }


    /**
    * Contructor for the JrcpSetIoPinReqMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpSetIoPinReqMessage object
    *
    */
    JrcpSetIoPinReqMessage::JrcpSetIoPinReqMessage(const BytesVec& raw_message)
        : JrcpMessage(raw_message)
    {
        payload_.insert(payload_.end(),
            raw_message.begin() + GetPayloadStartIndex(),
            raw_message.begin() + GetLn());
    }

    /**
    * Contructor for the JrcpSetIoPinReqMessage object
    *
    * @param nad The NAD value
    * @param payload The payload with pin ID and VAL
    *
    * @return  JrcpSetIoPinReqMessage object
    *
    */
    JrcpSetIoPinReqMessage::JrcpSetIoPinReqMessage(uint8_t nad, const BytesVec& payload)
        : JrcpMessage(kJrcpSetIoPinMty, nad, 0, {}, JRCP_SET_IO_PIN_PAYLOAD_SIZE),
        payload_(payload)
    {
        if (IoPinValueCheck::is_value(payload_[1]) != true)
        {
            /* Do not initialize */
            SetMessageWellFormed(false);
            return;
        }
    }

    /* Destructor for JrcpSetIoPinReqMessage object */
    JrcpSetIoPinReqMessage::~JrcpSetIoPinReqMessage()
    {
    }


    /**
    * Overridden method to get the raw message from the JrcpSetIoPinReqMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpSetIoPinReqMessage object
    *
    */
    BytesVec JrcpSetIoPinReqMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        if (!payload_.empty())
        {
            rawmsg.insert(rawmsg.end(), payload_.begin(), payload_.end());
        }

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }

    /**
    *  Contructor to create the set IO response message
    *
    * @param nad The NAD value of the device for which the set IO is requested
    * @param payload The JRCP response payload
    *
    * @return  JrcpSetIoPinRespMessage object having the set IO response
    *
    */
    JrcpSetIoPinRespMessage::JrcpSetIoPinRespMessage(uint8_t nad,
        const BytesVec& payload)
        : JrcpMessage(kJrcpEventHandlingMty, nad, 0, {}, (uint32_t)payload.size()), payload_(payload)
    {
    }

    /* Destructor for JrcpSetIoPinRespMessage object */
    JrcpSetIoPinRespMessage::~JrcpSetIoPinRespMessage()
    {
    }


    /**
    * Overridden method to get the raw message from the JrcpSetIoPinRespMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpSetIoPinRespMessage object
    *
    */
    BytesVec JrcpSetIoPinRespMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        if (!payload_.empty())
        {
            rawmsg.insert(rawmsg.end(), payload_.begin(), payload_.end());
        }

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }


    /**
    * Contructor for the JrcpWarmResetReqMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpWarmResetReqMessage object
    *
    */
    JrcpWarmResetReqMessage::JrcpWarmResetReqMessage(const BytesVec& raw_message)
        : JrcpMessage(raw_message)
    {
        uint32_t bwaitingtime = *reinterpret_cast<const uint32_t*>
            (&raw_message[GetPayloadStartIndex()]);
        waiting_time_ms_ = SwapEndian<uint32_t>(bwaitingtime);;

    }

    /**
    * Constructor for the JrcpWarmResetReqMessage object
    *
    * @param nad The NAD value
    * @param waiting_time_ms The reset waiting time in milli seconds
    *
    * @return  JrcpColdResetReqMessage object
    *
    */
    JrcpWarmResetReqMessage::JrcpWarmResetReqMessage(uint8_t nad, uint32_t waiting_time_ms)
        : JrcpMessage(kJrcpWarmResetMty, nad, 0, {}, JRCP_WAITING_TIME_MS_SIZE), waiting_time_ms_(waiting_time_ms)
    {
    }

    /* Destructor for JrcpWarmResetReqMessage object */
    JrcpWarmResetReqMessage::~JrcpWarmResetReqMessage()
    {
    }


    /**
    * Overridden method to get the raw message from the JrcpWarmResetReqMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpWarmResetReqMessage object
    *
    */
    BytesVec JrcpWarmResetReqMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, and Header (if available) are written to rawmsg here. */
        AppendPreamble(rawmsg);
        // Length 4 bytes
        rawmsg.push_back(0x00);
        rawmsg.push_back(0x00);
        rawmsg.push_back(0x00);
        rawmsg.push_back(0x04);

        BytesVec timevec = Uint32ToBEBytesVec(waiting_time_ms_);
        rawmsg.insert(rawmsg.end(), timevec.begin(), timevec.end());

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }

    JrcpWarmResetRespMessage::JrcpWarmResetRespMessage(const BytesVec& raw_message)
        : JrcpMessage(raw_message)
    {
        atr_.assign(
            raw_message.begin() + GetPayloadStartIndex(),
            raw_message.begin() + GetPayloadStartIndex() + GetLn());

    }


    /**
    *  Constructor to create the warm reset response message
    *
    * @param nad The NAD value of the device for which the warm reset is requested
    * @param atr The JRCP response ATR
    *
    * @return  JrcpWarmResetRespMessage object having the warm reset response
    *
    */
    JrcpWarmResetRespMessage::JrcpWarmResetRespMessage(uint8_t nad, const BytesVec& atr)
        : JrcpMessage(kJrcpWarmResetMty, nad, 0, {}, (uint32_t)atr.size()), atr_(atr)
    {
    }

    /* Destructor for JrcpWarmResetRespMessage object */
    JrcpWarmResetRespMessage::~JrcpWarmResetRespMessage()
    {
    }


    /**
    * Overridden method to get the raw message from the JrcpWarmResetRespMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpWarmResetRespMessage object
    *
    */
    BytesVec JrcpWarmResetRespMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        rawmsg.insert(rawmsg.end(), atr_.begin(), atr_.end());

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }

    /**
    * Constructor for the JrcpColdResetReqMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpColdResetReqMessage object
    *
    */
    JrcpColdResetReqMessage::JrcpColdResetReqMessage(const BytesVec& raw_message)
        : JrcpMessage(raw_message)
    {
        uint32_t bwaitingtime = *reinterpret_cast<const uint32_t*>
            (&raw_message[GetPayloadStartIndex()]);
        waiting_time_ms_ = SwapEndian<uint32_t>(bwaitingtime);;

    }

    /**
    * Constructor for the JrcpColdResetReqMessage object
    *
    * @param nad The NAD value
    * @param waiting_time_ms The reset waiting time in milli seconds
    *
    * @return  JrcpColdResetReqMessage object
    *
    */
    JrcpColdResetReqMessage::JrcpColdResetReqMessage(uint8_t nad, uint32_t waiting_time_ms)
        : JrcpMessage(kJrcpColdResetMty, nad, 0, {}, JRCP_WAITING_TIME_MS_SIZE),
        waiting_time_ms_(waiting_time_ms)
    {
    }


    /* Destructor for JrcpColdResetReqMessage object */
    JrcpColdResetReqMessage::~JrcpColdResetReqMessage()
    {
    }


    /**
    * Overridden method to get the raw message from the JrcpColdResetReqMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpColdResetReqMessage object
    *
    */
    BytesVec JrcpColdResetReqMessage::GetRawMessage() const
    {
        BytesVec rawmsg;

        /* SOF, MTY, NAD, HDL, and Header (if available) are written to rawmsg here. */
        AppendPreamble(rawmsg);

        // Length is always 4 bytes
        rawmsg.push_back(0x00);
        rawmsg.push_back(0x00);
        rawmsg.push_back(0x00);
        rawmsg.push_back(JRCP_TIMING_INFO_SIZE);

        BytesVec timevec = Uint32ToBEBytesVec(waiting_time_ms_);
        rawmsg.insert(rawmsg.end(), timevec.begin(), timevec.end());

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);
        return rawmsg;
    }


    JrcpColdResetRespMessage::JrcpColdResetRespMessage(const BytesVec& raw_message)
        : JrcpMessage(raw_message)
    {
        atr_.assign(
            raw_message.begin() + GetPayloadStartIndex(),
            raw_message.begin() + GetPayloadStartIndex() + GetLn());

    }

    /**
    *  Constructor to create the cold reset response message
    *
    * @param nad The NAD value of the device for which the cold reset is requested
    * @param atr The JRCP response ATR
    *
    * @return  JrcpColdResetRespMessage object having the cold reset response
    *
    */
    JrcpColdResetRespMessage::JrcpColdResetRespMessage(uint8_t nad, const BytesVec& atr)
        : JrcpMessage(kJrcpColdResetMty, nad, 0, {}, (uint32_t)atr.size()), atr_(atr)
    {
    }

    /* Destructor for JrcpColdResetRespMessage object */
    JrcpColdResetRespMessage::~JrcpColdResetRespMessage()
    {
    }


    /**
    * Overridden method to get the rew message from the JrcpColdResetRespMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpColdResetRespMessage object
    *
    */
    BytesVec JrcpColdResetRespMessage::GetRawMessage() const
    {
        BytesVec rawmsg;

        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        rawmsg.insert(rawmsg.end(), atr_.begin(), atr_.end());

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }

    /**
    * Contructor for the JrcpHciSendDataReqMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpHciSendDataReqMessage object
    *
    */
    JrcpHciSendDataReqMessage::JrcpHciSendDataReqMessage(const BytesVec& raw_message)
        : JrcpMessage(raw_message)
    {
        BytesVec header = GetHeader();

        if (header.size() != JRCP_HCI_HEADER_SIZE)
        {
            // NGv: Where is error handling?
            // TODO: Error handling
        }
        /* HCI header = Gate Id | Event Type | Frame type */
        uint8_t gate = header[0];
        event_type_ = header[1];
        uint8_t ft = header[2];

        if (HciGateIDCheck::is_value(gate))
        {
            gateID_ = static_cast<HciGateID>(gate);
        }
        else
        {
            SetMessageWellFormed(false);
        }

        if (FrameTypeCheck::is_value(ft))
        {
            frame_type_ = static_cast<FrameType>(ft);
        }
        else
        {
            SetMessageWellFormed(false);
        }
        payload_.insert(payload_.end(), raw_message.begin() + GetPayloadStartIndex(),
            raw_message.begin() + GetPayloadStartIndex() + GetLn());
    }

    /**
    * Contructor for the JrcpHciSendDataReqMessage object
    *
    * @param nad The NAD value
    * @param gate The gate ID
    * @param event_type The event type
    * @param ft The frame type
    * @param payload The event data
    *
    * @return  JrcpHciSendDataReqMessage object
    *
    */
    JrcpHciSendDataReqMessage::JrcpHciSendDataReqMessage(uint8_t nad, HciGateID gate,
        uint8_t event_type, FrameType ft, const BytesVec& payload)
        : JrcpMessage(kJrcpHciSendDataMty, nad, JRCP_HCI_HEADER_SIZE, { (unsigned char)gate, event_type, (unsigned char)ft }, (uint32_t)payload.size()),
        event_type_(event_type), payload_(payload)
    {
        if (true == HciGateIDCheck::is_value(gate))
        {
            gateID_ = static_cast<HciGateID>(gate);
        }
        else
        {
            SetMessageWellFormed(false);
        }


        if (true == FrameTypeCheck::is_value(ft))
        {
            frame_type_ = static_cast<FrameType>(ft);
        }
        else
        {
            SetMessageWellFormed(false);
        }
    }

    /* Destructor to JrcpHciSendDataReqMessage object */
    JrcpHciSendDataReqMessage::~JrcpHciSendDataReqMessage()
    {
    }


    /**
    * Overridden method to get the raw message from the JrcpHciSendDataReqMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpHciSendDataReqMessage object
    *
    */
    BytesVec JrcpHciSendDataReqMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        rawmsg.insert(rawmsg.end(), payload_.begin(), payload_.end());

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }

    /**
    *  Constructor to create the HCI send data response message
    *
    * @param nad The NAD value of the device for which the HCI send data is requested
    * @param status The HCI status code
    *
    * @return  JrcpHciSendDataRespMessage object having the HCI send data response
    *
    */
    JrcpHciSendDataRespMessage::JrcpHciSendDataRespMessage(uint8_t nad, HciStatusCode status)
        : JrcpMessage(kJrcpHciSendDataMty, nad, 0, {}, JRCP_HCI_STATUS_CODE_SIZE /* 2 bytes status */)
    {
        status_ = status;
    }

    /* Destructor to JrcpHciSendDataRespMessage object */
    JrcpHciSendDataRespMessage::~JrcpHciSendDataRespMessage()
    {

    }


    /**
    * Overridden method to get the raw message from the JrcpHciSendDataRespMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpHciSendDataRespMessage object
    *
    */
    BytesVec JrcpHciSendDataRespMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, and Header (if available) are written to rawmsg here. */
        AppendPreamble(rawmsg);

        // Length 2 bytes
        rawmsg.push_back(0x00);
        rawmsg.push_back(0x00);
        rawmsg.push_back(0x00);
        rawmsg.push_back(JRCP_HCI_STATUS_CODE_SIZE);

        BytesVec stvec = Uint16ToBEBytesVec(static_cast<uint16_t>(status_));
        rawmsg.insert(rawmsg.end(), stvec.begin(), stvec.end());

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }


    /* Destructor to JrcpFeatureControlReqMessage object */
    JrcpFeatureControlReqMessage::~JrcpFeatureControlReqMessage()
    {

    }


    /**
    * Overridden method to get the raw message from the JrcpFeatureControlReqMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpFeatureControlReqMessage object
    *
    */
    BytesVec JrcpFeatureControlReqMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        BytesVec feature = Uint16ToBEBytesVec(feature_type_);

        rawmsg.insert(rawmsg.end(), feature.begin(), feature.end());
        rawmsg.insert(rawmsg.end(), payload_.begin(), payload_.end());

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }

    /**
    * Constructor for the JrcpFeatureControlReqMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpFeatureControlReqMessage object
    *
    */
    JrcpFeatureControlReqMessage::JrcpFeatureControlReqMessage(const BytesVec& raw_message)
        : JrcpMessage(raw_message), feature_type_(FeatureType::UnknownFeatureType)
    {
        uint16_t submty = ParseSubMty(raw_message);
        if (FeatureTypeCheck::is_value(submty) == true)
        {
            feature_type_ = static_cast<FeatureType>(submty);
        }
        else
        {
            /* Do not initialize */
            SetMessageWellFormed(false);
            return;
        }
        /* Payload contains the config action as sub-MTY. Therefore remove the first two bytes */
        payload_.insert(payload_.end(), raw_message.begin() + GetPayloadStartIndex() + 2, raw_message.end() - 1);
    }

    /**
    * Constructor for the JrcpFeatureControlReqMessage object
    *
    * @param nad The NAD value
    * @param submty The value that determines request type
    * @param payload The payload containing FTY and optional information
    *
    * @return  JrcpFeatureControlReqMessage object
    *
    */
    JrcpFeatureControlReqMessage::JrcpFeatureControlReqMessage(uint8_t nad, FeatureType submty, const BytesVec& payload)
        : JrcpMessage(kJrcpFeatureControlRequestMty, nad, 0, {}, JRCP_SIZE_OF_SUB_MTY_LENGTH + (uint32_t)payload.size()),
        feature_type_(FeatureType::UnknownFeatureType),
        payload_(payload)
    {
        if (FeatureTypeCheck::is_value(submty) == true)
        {
            feature_type_ = static_cast<FeatureType>(submty);
        }
        else
        {
            /* Do not initialize */
            SetMessageWellFormed(false);
            return;
        }
    }


    /**
    * Function to get the version supported for a requested feature(MTY)
    *
    * @param instance The JRCP controller instance
    * @param nad The NAD value for the device requested
    * @param req_payload The feature control request message payload
    * @param resp_payload The reference to the feature control response payload
    *
    * @return None.
    *
    * @retval None
    */
    static void GetFeatureSubOptions(JrcpController *instance, uint8_t nad, BytesVec req_payload, BytesVec &resp_payload)
    {
        BytesVec status;

        /* TODO: Ngv: Below assumption is wrong. NAD must not be part of payload */

        /*Reading the payload which is in order as below e.g Tearing action */
        /*MTY|NAD|xxxxh(Action)*/
        /* Currently the support is not available for this action. Therefore return error code with empty SUB-MTY */
        resp_payload.clear();
        DeviceRegistry & devreg = instance->GetDeviceRegistry();
        /* The second index the payload will contain the NAD value */
        uint8_t req_nad = req_payload[1];
        if (nullptr == (devreg)[req_nad])
        {
            status = Uint32ToBEBytesVec(JrcpGenericStatusCodes::GenericError);
            resp_payload.insert(resp_payload.begin(), status.begin(), status.end());
            return;
        }
        /* The feature specific actions currently is not supported in the JRCP library.
           Planned to be included in the future */
        status = Uint32ToBEBytesVec(JrcpGenericStatusCodes::GenericUnsupportedCommand);
        resp_payload.insert(resp_payload.begin(), status.begin(), status.end());
        (devreg)[nad]->SetDeviceServerStatus(JrcpGenericStatusCodes::GenericUnsupportedCommand);
        return;
    }

    /**
    * Function to get the version supported for a requested feature(MTY)
    *
    * @param instance The JRCP controller instance
    * @param nad The NAD value for the device requested
    * @param req_payload The feature control request message payload
    * @param resp_payload The reference to the feature control response payload
    *
    * @return None
    *
    * @retval None
    */
    static void GetSupportedVersionForFeature(JrcpController *instance, uint8_t nad, BytesVec req_payload, BytesVec &resp_payload)
    {
        BytesVec status;
        uint8_t mty = req_payload[0];
        /* The first byte in the payload contains the requested MTY */
        switch (mty)
        {
        /* The below cases cover the MTYs which belongs to the legacy format and are not supported anymore */
        case jrcplib::kJrcpStatusMty: /* Fall through is intentional for all below 7 cases*/
        case jrcplib::kJrcpErrorMty:
        case jrcplib::kJrcpInformationTextMty:
        case jrcplib::kJrcpInitializationDataMty:
        case jrcplib::kJrcpDebugMty:
        case jrcplib::kJrcpChangeProtocolMty:
        case jrcplib::kJrcpSetTerminalParametersMty:
        case jrcplib::kJrcpExtendedLength:
            status = Uint16ToBEBytesVec(JrcpProtocolSupported::Unsupported);
            resp_payload.insert(resp_payload.end(), status.begin(), status.end());
            return;

        default:
        {
            if ( mty == jrcplib::kJrcpHciSendDataMty ||
                 mty == jrcplib::kJrcpPrepareTearingMty ||
                 mty == jrcplib::kJrcpTimingInformationMty ||
                (mty >= jrcplib::kJrcpDeviceSpecificExtensionsFirst && mty <= jrcplib::kJrcpDeviceSpecificExtensionsLast))
            {
                /* The following MTYs belongs to the device expecific extension.*/
                /* Get the device registry and check whether the device exists or not */
                const DeviceRegistry &devreg = instance->GetDeviceRegistry();
                std::shared_ptr<const jrcplib::Device> device = (devreg)[nad];
                if (device == nullptr)
                {
                    /* Clear the payload which contains SUB-MTY */
                    resp_payload.clear();
                    status = Uint32ToBEBytesVec(JrcpProtocolSupported::Unsupported);
                    resp_payload.insert(resp_payload.end(), status.begin(), status.end());
                    return;
                }

                /* The first byte in the payload responds to the MTY requested.
                 * Check whether the feature handler exists for the particular NAD for the requested message type */
                const FeatureHandler* fh;
                if (device->GetFeatureHandler(mty, &fh) != JRCPLIB_ERR_NO_HANDLER_REGISTERED)
                {
                    /* Return the version for the supported MTY handler.
                     * The function handler is in form of std::pair which contains the MTY version as the first parameter */
                    status = Uint16ToBEBytesVec(fh->first);
                    resp_payload.insert(resp_payload.end(), status.begin(), status.end());
                }
                else
                {
                    /* Handler not present respond with unsupported protocol */
                    status = Uint16ToBEBytesVec(JrcpProtocolSupported::Unsupported);
                    resp_payload.insert(resp_payload.end(), status.begin(), status.end());
                }
                return;
            }
            /* For any other MTYs return the controller version */
            jrcplib_protocol_supported_t protocol_version = instance->GetControllerVersion();
            status = Uint16ToBEBytesVec(protocol_version);
            resp_payload.insert(resp_payload.end(), status.begin(), status.end());
            return;
        }
        break;
        }
    }

    /**
    * Function to get the session Id response. Also invokes the hanlder to pass the session Id to the client
    *
    * @param instance The JRCP controller instance
    * @param req_msg The feature control request message instance
    * @param req_payload The feature control request message payload
    * @param resp_payload The reference to the response message payload
    *
    * @return None
    *
    * @retval None
    */
    static void GetSessionIdResponse(JrcpController *instance, JrcpMessage * req_msg, BytesVec req_payload, BytesVec &resp_payload)
    {
        BytesVec status;
        uint8_t nad = req_msg->GetNad();
        /* Clear the response payload */
        resp_payload.clear();

        const FeatureHandler* fh;
        /* If the handler is registered it will be called to pass the session Id to the client in order to store it in the
           trace file. In future, the complete implementation will be handled as a part of separate MTY */
        if (instance->GetDeviceRegistry()[nad]->GetFeatureHandler(JRCP_STORE_SESSION_ID, &fh) != JRCPLIB_ERR_NO_HANDLER_REGISTERED)
        {
            /* Call the handler to provide the Session ID. The response message is created as part of the Feature Control Request message
               No need to pass the response object pointer as the main purpose is to only pass the session Id to the client
               In Future, there will be a dedicated handler to fetch the session Id */
            if (JRCPLIB_ERR_OK == fh->second(instance, req_msg, nullptr))
            {
                status = Uint32ToBEBytesVec(JrcpGenericStatusCodes::GenericOK);
                resp_payload.insert(resp_payload.end(), status.begin(), status.end());
                instance->GetDeviceRegistry()[nad]->SetDeviceServerStatus(JrcpGenericStatusCodes::GenericOK);
            }
            else
            {
                /* Session Id is invalid */
                status = Uint32ToBEBytesVec(JrcpGenericStatusCodes::GenericUnsupportedCommand);
                resp_payload.insert(resp_payload.end(), status.begin(), status.end());
                instance->GetDeviceRegistry()[nad]->SetDeviceServerStatus(JrcpGenericStatusCodes::GenericUnsupportedCommand);
            }
            return;
        }
        else
        {
            /* Handler to set the session Id is not registered */
            status = Uint32ToBEBytesVec(JrcpGenericStatusCodes::GenericUnsupportedCommand);
            resp_payload.insert(resp_payload.end(), status.begin(), status.end());
            instance->GetDeviceRegistry()[nad]->SetDeviceServerStatus(JrcpGenericStatusCodes::GenericUnsupportedCommand);
            return;
        }
    }

    /**
    * Process the Feature control request MTY.
    *
    * @param instance The JRCP controller instance
    * @param req_msg The feature control request message instance
    * @param out The pointer which receives the response instance to this MTY
    *
    * @return Error code if something went wrong or OK otherwise
    *
    * @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED
    *           iff the MTY of the request message does not match to any class
    * @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED
    *           iff the requested device is not registered
    */
    int32_t FeatureControlRequestHandler(JrcpController *instance, JrcpMessage * req_msg, JrcpMessage **out)
    {
        BytesVec resp_payload;
        FeatureType feature_type;

        /* Perform the downcast to the Feature Control Request message class */
        const jrcplib::JrcpFeatureControlReqMessage *feature_req = dynamic_cast<const jrcplib::JrcpFeatureControlReqMessage *>(req_msg);

        /* Check if the message can be meaningfully cast down. */
        if (feature_req == nullptr)
        {
            /* Return error code if casting failed. Generic message will be send as a response
             * Handling of this is done in the callee wrapper function */
            return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
        }

        /* Get the device NAD and check whether the device is registered */
        uint8_t nad = feature_req->GetNad();

        /* Check that the device even exists before we try to access it. */
        if (instance->GetDeviceRegistry()[nad] == nullptr)
        {
            /* Return error code no device registered. Generic message will be send as a response
            * Handling of this is done in the callee wrapper function */
            return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
        }

        feature_type = feature_req->GetFeatureType();
        BytesVec feature_type_vec = Uint16ToBEBytesVec(feature_type);
        resp_payload.insert(resp_payload.end(), feature_type_vec.begin(), feature_type_vec.end());
        switch (feature_type)
        {
            /* Provide the version of the supported message handlers */
        case FeatureType::VersionControl:
            GetSupportedVersionForFeature(instance, nad, feature_req->GetPayload(), resp_payload);
            instance->GetDeviceRegistry()[nad]->SetDeviceServerStatus(JrcpGenericStatusCodes::GenericOK);
            break;

            /* Provide the feature options (actions) of the supported message handlers */
        case FeatureType::FeatureOption:
            GetFeatureSubOptions(instance, nad, feature_req->GetPayload(), resp_payload);
            break;

            /* Provide the controller identifier info */
        case FeatureType::ControllerInfo:
        {
            std::string identifier = instance->GetControllerIdentifier();
            resp_payload.insert(resp_payload.end(), identifier.begin(), identifier.end());
        }
            break;

            /* Provide the protocol version of the controller device */
        case FeatureType::ProtocolVersion:
        {
            BytesVec version_vec(Uint16ToBEBytesVec(instance->GetControllerVersion()));
            resp_payload.insert(resp_payload.end(), version_vec.begin(), version_vec.end());
        }
            break;

            /* Provide the session Id */
        case FeatureType::SessionId:
            GetSessionIdResponse(instance, req_msg, feature_req->GetPayload(), resp_payload);
            break;

        default:
        {
            /* Clear the payload */
            resp_payload.clear();
            BytesVec status = Uint32ToBEBytesVec(JrcpGenericStatusCodes::GenericUnsupportedCommand);
            resp_payload.insert(resp_payload.end(), status.begin(), status.end());
            instance->GetDeviceRegistry()[nad]->SetDeviceServerStatus(JrcpGenericStatusCodes::GenericUnsupportedCommand);
        }
            break;
        }

        /* Create the response instance */
        *out = new jrcplib::JrcpFeatureControlRespMessage(nad, resp_payload);
        return JRCPLIB_ERR_OK;
    }


    /**
    *  Contructor to create the feature control response message
    *
    * @param nad The NAD value of the device for which the feature control is requested
    * @param payload The JRCP response payload
    *
    * @return  JrcpFeatureControlRespMessage object having the feature control response
    *
    */
    JrcpFeatureControlRespMessage::JrcpFeatureControlRespMessage(uint8_t nad, const BytesVec& payload)
        : JrcpMessage(kJrcpFeatureControlRequestMty, nad, 0, {}, (uint32_t)payload.size()), payload_(payload)
    {
    }

    /**
    *  Contructor to create the feature control response message
    *
    * @param nad The NAD value of the device for which the feature control is requested
    * @param submty The sub mty containing the feature type requested
    * @param payload The JRCP response payload
    *
    * @return  JrcpFeatureControlRespMessage object having the feature control response
    *
    */
    JrcpFeatureControlRespMessage::JrcpFeatureControlRespMessage(uint8_t nad, jrcplib_feature_type_t submty, const BytesVec& payload)
        : JrcpMessage(kJrcpFeatureControlRequestMty, nad, 0, {}, JRCP_SIZE_OF_SUB_MTY_LENGTH + (uint32_t)payload.size())
    {
        BytesVec submtyvec = Uint16ToBEBytesVec(submty);
        payload_.insert(payload_.end(), submtyvec.begin(), submtyvec.end());
        payload_.insert(payload_.end(), payload.begin(), payload.end());

    }

    /* Destructor to JrcpFeatureControlRespMessage object */
    JrcpFeatureControlRespMessage::~JrcpFeatureControlRespMessage()
    {
    }


    /**
    * Overridden method to get the raw message from the JrcpFeatureControlRespMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpFeatureControlRespMessage object
    *
    */
    BytesVec JrcpFeatureControlRespMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        rawmsg.push_back(kJrcpMessageSofValue); // sof
        rawmsg.push_back(kJrcpFeatureControlRequestMty);
        rawmsg.push_back(GetNad());

        rawmsg.push_back(0x00); // HDL

        BytesVec lnvec = Uint32ToBEBytesVec(GetLn());
        rawmsg.insert(rawmsg.end(), lnvec.begin(), lnvec.end());

        if (true != payload_.empty())
        {
            rawmsg.insert(rawmsg.end(), payload_.begin(), payload_.end());
        }

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);
        return rawmsg;
    }


    /**
    * Contructor for the JrcpGenericMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpGenericMessage object
    *
    */
    JrcpGenericMessage::JrcpGenericMessage(BytesVec raw_message)
        : JrcpMessage(raw_message)
    {
        payload_.insert(payload_.end(),
            raw_message.begin() + GetPayloadStartIndex(),
            raw_message.begin() + GetPayloadStartIndex() + GetLn());
    }

    /**
    *  Contructor to create generic JRCP  message
    *
    * @param mty The MTY requested
    * @param nad The NAD value of the device
    * @param hdl The custom header length
    * @param header The custom header
    * @param ln The payload length
    *
    * @return  JrcpGenericMessage object
    *
    */
    JrcpGenericMessage::JrcpGenericMessage(uint8_t mty, uint8_t nad, uint8_t hdl, BytesVec header, uint32_t ln)
        : JrcpMessage(mty, nad, hdl, header, ln)
    {

    }

    /**
    *  Contructor to create generic JRCP  message
    *
    * @param mty The MTY requested
    * @param nad The NAD value of the device
    * @param hdl The custom header length
    * @param payload The payload data
    * @param header The custom header
    * @param ln The payload length
    *
    * @return  JrcpGenericMessage object
    *
    */
    JrcpGenericMessage::JrcpGenericMessage(uint8_t mty, uint8_t nad, uint8_t hdl, BytesVec header, BytesVec payload, uint32_t ln)
        : JrcpMessage(mty, nad, hdl, header, ln)
    {
        payload_.assign(payload.begin(), payload.end());
    }

    /* Destructor to JrcpGenericMessage object */
    JrcpGenericMessage::~JrcpGenericMessage()
    {

    }


    /**
    * Overridden method to get the raw message from the JrcpGenericMessage object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpGenericMessage object
    *
    */
    BytesVec JrcpGenericMessage::GetRawMessage() const
    {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        if (GetLn() > 0)
        {
            rawmsg.insert(rawmsg.end(), payload_.begin(), payload_.end());
        }

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);

        return rawmsg;
    }

    /**
    * Contructor for the JrcpControllerReqMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpControllerReqMessage object
    *
    */
    JrcpControllerReqMessage::JrcpControllerReqMessage(BytesVec raw_message)
        : JrcpMessage(raw_message),
          controller_config_type_(ConfigurationType::UnknownConfigurationType)
    {
        uint16_t submty = ParseSubMty(raw_message);
        if (ConfigurationTypeCheck::is_value(submty) == true) {
            controller_config_type_ = static_cast<ConfigurationType>(submty);
        }
        else {
            /* Do not initialize */
            return;
        }
        /* Payload contains the config action as sub-MTY. Therefore remove the first two bytes */
        payload_.insert(payload_.end(), raw_message.begin() + GetPayloadStartIndex() + JRCP_SIZE_OF_SUB_MTY_LENGTH, raw_message.end() - 1);
    }


    JrcpControllerReqMessage::JrcpControllerReqMessage(ConfigurationType conf_type, const BytesVec& payload)
        : JrcpMessage(kJrcpControllerConfigurationMty, JRCP_SERVER_NAD, 0, {}, JRCP_SIZE_OF_SUB_MTY_LENGTH + (uint32_t)payload.size()),
        controller_config_type_(ConfigurationType::UnknownConfigurationType),
        payload_(payload)
    {
        if (ConfigurationTypeCheck::is_value(conf_type) == true)
        {
            controller_config_type_ = static_cast<ConfigurationType>(conf_type);
        }
        else
        {
            /* Do not initialize */
            SetMessageWellFormed(false);
            return;
        }
    }


    /* Destructor to JrcpControllerReqMessage object */
    JrcpControllerReqMessage::~JrcpControllerReqMessage() {}

    /**
    * Overridden method to get the raw message from the controller request message object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpControllerReqMessage object
    *
    */
    BytesVec JrcpControllerReqMessage::GetRawMessage() const {
        BytesVec rawmsg;
        rawmsg.push_back(kJrcpMessageSofValue); // sof
        rawmsg.push_back(kJrcpControllerConfigurationMty);
        rawmsg.push_back(GetNad());

        rawmsg.push_back(0x00); // HDL

        BytesVec lnvec = Uint32ToBEBytesVec(JRCP_SIZE_OF_SUB_MTY_LENGTH + (uint32_t)payload_.size());
        rawmsg.insert(rawmsg.end(), lnvec.begin(), lnvec.end());

        BytesVec submtyvec = Uint16ToBEBytesVec(static_cast<uint16_t>(controller_config_type_));
        rawmsg.insert(rawmsg.end(), submtyvec.begin(), submtyvec.end());


        if (true != payload_.empty()) {
            rawmsg.insert(rawmsg.end(), payload_.begin(), payload_.end());
        }

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);
        return rawmsg;
    }

    /**
    * Add the connected device information to the payload
    *
    * @param thisinst The JRCP controller instance
    * @param list Reference to the response payload
    *
    * @return None
    *
    * @retval None.
    *
    */
    static void AppendTlvDeviceListing(const JrcpController *thisinst, BytesVec &list)
    {
        /* Get the Device Registry of the instance */
        const DeviceRegistry &devreg = thisinst->GetDeviceRegistry();
        /* Traverse through the device registry to get the connected devices */
        for (size_t i = 0; i < devreg.GetRegistrySize(); i++)
        {
            if (nullptr != (devreg)[i])
            {
                if (true == (devreg)[i]->IsConnected())
                {
                    /* Device available and connected, store its NAD value */
                    list.push_back((devreg)[i]->GetDeviceNad());
                }
                else
                {
                    /* Device available but not connected, store with controller NAD(0xFF) */
                    list.push_back(JRCP_SERVER_NAD);
                }
                std::string descr = (devreg)[i]->GetDeviceDescription();
                list.push_back((uint8_t)descr.size());
                list.insert(list.end(), descr.begin(), descr.end());
            }
        }
        return;
    }

    /**
    * Provides the index of the first connected device
    *
    * @param thisinst The JRCP controller instance
    *
    * @return Index of the connected device or OK if no device found in connected state
    *
    * @retval JRCPLIB_ERR_OK if no device was found.
    */
    static int32_t FindFirstConnectedDevice(const JrcpController *thisinst)
    {
        const DeviceRegistry &devreg = thisinst->GetDeviceRegistry();
        /* Check if any device assigned by the client(starts from NAD 0x80) is in connected state */
        for (size_t i = JRCP_USER_DEFINED_NAD_START_VALUE; i < devreg.GetRegistrySize(); i++)
        {
            if (nullptr != devreg[i])
            {
                if (true == devreg[i]->IsConnected())
                {
                    return (int32_t)i;
                }
            }
        }
        return JRCPLIB_ERR_OK;
    }


    /**
    * Provides the index of the first connected device
    *
    * @param thisinst The JRCP controller instance
    * @param term_string Device description received in the request message
    *
    * @return Index of the connected device, negative means error code
    *
    * @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED if no device was found.
    */
    static int32_t FindFirstConnectedDeviceMatchingTerminalString(const JrcpController *thisinst, const std::string &term_string)
    {
        const DeviceRegistry &devreg = thisinst->GetDeviceRegistry();
        for (size_t i = 0; i < devreg.GetRegistrySize(); i++)
        {
            if (nullptr != devreg[i])
            {
                if (0 == term_string.compare(devreg[i]->GetDeviceDescription()))
                {
                    /* Matching device found return its index */
                    return (int32_t)i;
                }
            }
        }
        return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
    }


    /**
    * Function to connect the requested device
    *
    * @param instance The JRCP controller instance
    * @param req_payload The controller request message payload
    * @param resp_payload The reference to the response payload
    *
    * @return None.
    *
    * @retval None.
    */
    static void ConnectRequestedDevice(JrcpController *instance, const BytesVec req_payload, BytesVec &resp_payload)
    {
        BytesVec status;
        DeviceRegistry &devreg = instance->GetDeviceRegistry();

        /* Copy the payload which contains the description of the Device. */
        std::string term_string = std::string(req_payload.begin(), req_payload.end());
        /* Check for null character or empty payload. Also according to JRCP spec v 2.8 terminal string cannot exceed 1024 bytes */
        if ((0 == term_string.size()) || (0 == term_string.compare("null")) || (1024 < req_payload.size()))
        {
            /* Invalid message */
            status = Uint16ToBEBytesVec(JrcpGenericStatusCodes::GenericError);
            resp_payload.insert(resp_payload.end(), status.begin(), status.end());
            return;
        }

        /* Get the index of the device from the device registry */
        int32_t index = FindFirstConnectedDeviceMatchingTerminalString(instance, term_string);
        if (index >= 0)
        {
            /* According to JRCP spec, modification in connection status of device with NAD below 0x80 is not permitted */
            if (JRCP_PREDEFINED_NAD_MAX_VALUE >= index)
            {
                status = Uint16ToBEBytesVec(JrcpGenericStatusCodes::GenericUnsupportedCommand);
                resp_payload.insert(resp_payload.end(), status.begin(), status.end());
                return;
            }
            else
            {
                /* Check if any other device is in connected state */
                if (FindFirstConnectedDevice(instance) != JRCPLIB_ERR_OK)
                {
                    status = Uint16ToBEBytesVec(JrcpGenericStatusCodes::GenericError);
                    resp_payload.insert(resp_payload.end(), status.begin(), status.end());
                    return;
                }
                /* All conditions are satisfied, set the device status to connected */
                (devreg)[index]->SetDeviceConnectionStatus(DeviceConnectionStatus::Connected);
                resp_payload.push_back(index);
                (devreg)[index]->SetDeviceServerStatus(JrcpGenericStatusCodes::GenericOK);
                return;
            }
        }
        else
        {
            /* Device with requested NAD not found */
            status = Uint32ToBEBytesVec(JrcpGenericStatusCodes::GenericError);
            resp_payload.insert(resp_payload.begin(), status.begin(), status.end());
            return;
        }
    }

    /**
    * Function to disconnect the requested device
    *
    * @param instance The JRCP controller instance
    * @param req_payload The controller request message payload
    * @param resp_payload The reference to the response payload
    *
    * @return None.
    *
    * @retval None
    */
    static void DisconnectRequestedDevice(JrcpController *instance, const BytesVec req_payload, BytesVec &resp_payload)
    {
        BytesVec status;
        /* The response payload already contains the SUB-MTY. Clear the response payload. The response
           for close terminal contains empty SUB-MTY, therefore clear first two bytes which was added */
        resp_payload.clear();
        DeviceRegistry &devreg = instance->GetDeviceRegistry();
        /* The payload must contain the NAD to disconnect else return unsupported command */
        if (0 == req_payload.size())
        {
            /* Error occured set empty SUB-MTY */
            status = Uint32ToBEBytesVec(JrcpGenericStatusCodes::GenericUnsupportedCommand);
            resp_payload.insert(resp_payload.begin(), status.begin(), status.end());
            return;
        }

        /* The nad value will be present in the first byte of the payload */
        uint8_t nad = req_payload[0];

        /* According to JRCP spec, modification in connection status of device with NAD below 0x80 is not permitted */
        if (JRCP_PREDEFINED_NAD_MAX_VALUE >= nad)
        {
            /* Error occured set empty SUB-MTY*/
            status = Uint32ToBEBytesVec(JrcpGenericStatusCodes::GenericUnsupportedCommand);
            resp_payload.insert(resp_payload.begin(), status.begin(), status.end());
            return;
        }
        /* Check if device is registered */
        if (nullptr != (devreg)[nad])
        {
            if ((devreg)[nad]->IsConnected() == false)
            {
                /* Error occured . Device present but is already in disconnected state */
                status = Uint32ToBEBytesVec(JrcpGenericStatusCodes::GenericError);
                resp_payload.insert(resp_payload.begin(), status.begin(), status.end());
            }
            else
            {
                /* Device found. Set its status as disconnected */
                (devreg)[nad]->SetDeviceConnectionStatus(DeviceConnectionStatus::Disconnected);
                status = Uint32ToBEBytesVec(JrcpGenericStatusCodes::GenericOK);
                resp_payload.insert(resp_payload.begin(), status.begin(), status.end());
                (devreg)[nad]->SetDeviceServerStatus(JrcpGenericStatusCodes::GenericOK);
            }
        }
        else
        {
            /* Error occured. Device not registered set empty SUB-MTY*/
            status = Uint32ToBEBytesVec(JrcpGenericStatusCodes::GenericError);
            resp_payload.insert(resp_payload.begin(), status.begin(), status.end());
        }

        return;
    }


    /**
    * Process the controller configuration request MTY. This handler is specifically for the controller device
    *
    * @param instance The JRCP controller instance
    * @param req_msg The controller request message instance
    * @param out The pointer which receives the response instance to this MTY
    *
    * @return Error code if something went wrong or OK otherwise
    *
    * @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED
    *           iff the MTY of the request message does not match to any class
    * @retval JRCPLIB_ERR_OK in case of success
    */
    int32_t ControllerConfigurationHandler(JrcpController *instance, JrcpMessage *req_msg, JrcpMessage **out)
    {
        BytesVec resp_payload;
        BytesVec status;
        //DeviceRegistry &devreg =
        instance->GetDeviceRegistry();

        /* Try to cast the message to Controller Request Message */
        const jrcplib::JrcpControllerReqMessage *controller_config_msg = dynamic_cast<const jrcplib::JrcpControllerReqMessage *>(req_msg);
        if (controller_config_msg == nullptr)
        {
            /* Return error code if casting failed. Generic message will be send as a response
             * Handling of this is done in the callee wrapper function */
            return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
        }

        uint16_t config_type = controller_config_msg->GetConfigurationType();
        BytesVec config_type_vec = Uint16ToBEBytesVec(config_type);
        resp_payload.insert(resp_payload.end(), config_type_vec.begin(), config_type_vec.end());

        switch (config_type)
        {
            /* List all the interfaces available for the JRCP instance */
        case ConfigurationType::ListReaders:
            AppendTlvDeviceListing(instance, resp_payload);
            break;

            /* Set the requested interface to connected state */
        case ConfigurationType::ConnectTerminal:
            ConnectRequestedDevice(instance, controller_config_msg->GetPayload(), resp_payload);
            break;

            /* Set the requested interface to disconnected state */
        case ConfigurationType::CloseTerminal:
            DisconnectRequestedDevice(instance, controller_config_msg->GetPayload(), resp_payload);
            break;

            /* Freeze and Release mapping is currently not supported */
        case ConfigurationType::FreezeMapping: /* Fall through is intentional */
        case ConfigurationType::ReleaseMapping: /* Fall through is intentional */
        default:
            /* Clear the response payload to send empty sub-MTY in case error occurred */
            resp_payload.clear();
            status = Uint32ToBEBytesVec(JrcpGenericStatusCodes::GenericUnsupportedCommand);
            resp_payload.insert(resp_payload.end(), status.begin(), status.end());
            break;
        }

        *out = new jrcplib::JrcpControllerRespMessage(req_msg->GetNad(),
                                                  controller_config_msg->GetConfigurationType(),
                                                  resp_payload);
        /* Return OK  */
        return JRCPLIB_ERR_OK;
    }


    /**
    * Contructor for the JrcpControllerReqMessage object
    *
    * @param raw_message The jrcp raw input message
    *
    * @return  JrcpControllerReqMessage object
    *
    */
    JrcpControllerRespMessage::JrcpControllerRespMessage(BytesVec raw_message)
        : JrcpMessage(raw_message)
    {
        uint16_t submty = ParseSubMty(raw_message);
        if (ConfigurationTypeCheck::is_value(submty) == true) {
            controller_config_type_ = static_cast<ConfigurationType>(submty);
        }
        else {
            /* Do not initialize */
            return;
        }
        /* Payload contains the config action as sub-MTY. Therefore remove the first two bytes */
        payload_.insert(payload_.end(), raw_message.begin() + GetPayloadStartIndex() + JRCP_SIZE_OF_SUB_MTY_LENGTH, raw_message.end());
    }

    /**
    *  Contructor to create the controller configuration response message
    *
    * @param nad The NAD value of the device for which the controller configuration is requested
    * @param config_type The sub mty containing the configuration type requested
    * @param payload The JRCP response payload
    *
    * @return  JrcpControllerRespMessage object having the controller configuration response
    *
    */
    JrcpControllerRespMessage::JrcpControllerRespMessage(uint8_t nad, ConfigurationType config_type, BytesVec payload)
        : JrcpMessage(kJrcpControllerConfigurationMty, nad, 0, {}, (uint32_t)payload.size()),
        controller_config_type_(config_type),
        payload_(payload) {
    }

    /* Destructor to JrcpControllerRespMessage object */
    JrcpControllerRespMessage::~JrcpControllerRespMessage() {}


    /**
    * Overridden method to get the raw message from the controller response message object
    *
    * @param None
    *
    * @return  unsigned char vector to the raw message generated using JrcpControllerRespMessage object
    *
    */
    BytesVec JrcpControllerRespMessage::GetRawMessage() const {
        BytesVec rawmsg;
        /* SOF, MTY, NAD, HDL, Header (if available) and LN are written to rawmsg here. */
        AppendPreambleWithLn(rawmsg);

        if (true != payload_.empty()) {
            rawmsg.insert(rawmsg.end(), payload_.begin(), payload_.end());
        }

        /* TIL, TR, TC are written to rawmsg here. */
        AppendTiming(rawmsg);
        return rawmsg;
    }
}
