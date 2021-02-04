/* Copyright 2019,2020 NXP
 *
 * This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms.  By expressly
 * accepting such terms or by downloading, installing, activating and/or
 * otherwise using the software, you are agreeing that you have read, and
 * that you agree to comply with and are bound by, such license terms.  If
 * you do not agree to be bound by the applicable license terms, then you
 * may not retain, install, activate or otherwise use the software.
 */
#ifndef MESSAGES_H_
#define MESSAGES_H_
/** @file messages.h */

#include <jrcplib/api.h>
#include <jrcplib/data_types.h>
#include <jrcplib/jrcplib.h>
#include <stdint.h>
#include <climits>

#ifdef __cplusplus
#include <vector>

#define JRCP_NO_TIMINIG_INFO                             0x00
#define JRCP_TIMING_INFO_WITH_RESPONSE_TIMESTAMP         0x08
#define JRCP_TIMING_INFO_WITH_CMD_AND_RESPONSE_TIMESTAMP 0x10

#define JRCP_PREDEFINED_NAD_MAX_VALUE 0x7F
#define JRCP_USER_DEFINED_NAD_START_VALUE 0x80

#define JRCP_SIZE_OF_PAYLOAD_LENGTH       0x04
#define JRCP_SIZE_OF_SUB_MTY_LENGTH       0x02
#define JRCP_SUB_MTY_OFFSET               0x05 /* 4 bytes Custom header length + 1 bytes custom header length value */
#define JRCP_HCI_STATUS_CODE_SIZE         0x02
#define JRCP_TEARING_STATUS_CODE_SIZE     0x02
#define JRCP_SERVER_STATUS_CODE_SIZE      0x02
#define JRCP_HCI_HEADER_SIZE              0x03
#define JRCP_TIMING_INFO_SIZE             0x04 /* 4 bytes of the timing value */
#define JRCP_WAITING_TIME_MS_SIZE         0x04
#define JRCP_SET_IO_PIN_PAYLOAD_SIZE      0x02

/* Wait for card payload response length will contain the timing information which is 4 bytes in length */
#define JRCP_WAIT_FOR_CARD_RESPONSE_PAYLOAD_LENGTH 0x04

namespace jrcplib
{
    namespace utils {
        template <typename T>
            T SwapEndian(T u)
        {
            static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

            union
            {
                T u;
                unsigned char u8[sizeof(T)];
            } source, dest;

            source.u = u;

            for (size_t k = 0; k < sizeof(T); k++)
                dest.u8[k] = source.u8[sizeof(T) - k - 1];

            return dest.u;
        }
    }

    /* This enum is only used for the argument of `CreateJrcpRespMessage`.

    A word of warning: Do NOT cast the values of this enum to uint8_t in hopes
    of getting the MTY of the message class. It does not work and is not indended
    to work. For actual MTYs, refer to the below integer constants of the form
    `kJrcp...Mty`. */
    enum JrcpMessageType {
        JrcpWaitForCardReqMessageType = 1,
        JrcpWaitForCardRespMessageType,

        JrcpSendDataMessageType,

        JrcpTerminalInfoMessageType,

        JrcpServerStatusReqMessageType,
        JrcpServerStatusRespMessageType,

        JrcpTimingInfoReqMessageType,
        JrcpTimingInfoRespMessageType,

        JrcpPrepareTearingReqMessageType,
        JrcpPrepareTearingRespMessageType,

        JrcpEventHandlingReqMessageType,
        JrcpEventHandlingRespMessageType,

        JrcpSetIoPinReqMessageType,
        JrcpSetIoPinRespMessageType,

        JrcpWarmResetReqMessageType,
        JrcpWarmResetRespMessageType,

        JrcpColdResetReqMessageType,
        JrcpColdResetRespMessageType,

        JrcpHciSendDataReqMessageType,
        JrcpHciSendDataRespMessageType

    };

    const int kJrcpMessageSofIndex = 0; /* SOF index in raw JRCP frame*/
    const int kJrcpMessageMtyIndex = 1; /* MTY index in raw JRCP frame*/
    const int kJrcpMessageNadIndex = 2; /* NAD index in raw JRCP frame*/
    const int kJrcpMessageHdlIndex = 3; /* HDL index in raw JRCP frame*/

    const uint8_t kJrcpMessageSofValue = 0xA5;  /* Expected SOF value. */
    const uint8_t kJrcpServerNad = 0xFF;        /* NAD of a JRCP server. */

    const size_t kJrcpMessageTcLen = 8; /* Length of the TC in a message. */
    const size_t kJrcpMessageTrLen = 8; /* Length of the TR in a message. */

    const int kJrcpV1MessageLnlIndex = 3; /* Index of LNL in JRCP v1 message */

    /* List of available MTYs. Device-specific MTYs are in the range from
    `kJrcpDeviceSpecificExtensionsFirst` to `kJrcpDeviceSpecificExtensionsLast`. */
    const int kJrcpWaitForCardMty                   = 0x00;
    const int kJrcpSendDataMty                      = 0x01;
    const int kJrcpStatusMty                        = 0x02;
    const int kJrcpErrorMty                         = 0x03;
    const int kJrcpTerminalInformationMty           = 0x04;
    const int kJrcpInitializationDataMty            = 0x05;
    const int kJrcpInformationTextMty               = 0x06;
    const int kJrcpDebugMty                         = 0x07;
    const int kJrcpChangeProtocolMty                = 0x08;
    const int kJrcpSetTerminalParametersMty         = 0x09;
    const int kJrcpServerStatusMty                  = 0x0A;
    const int kJrcpTimingInformationMty             = 0x0B;
    const int kJrcpPrepareTearingMty                = 0x0C;
    const int kJrcpEventHandlingMty                 = 0x0D;
    const int kJrcpSetIoPinMty                      = 0x0E;
    const int kJrcpWarmResetMty                     = 0x30;
    const int kJrcpColdResetMty                     = 0x31;
    const int kJrcpHciSendDataMty                   = 0x32;
    const int kJrcpExtendedLength                   = 0xA5;
    const int kJrcpDeviceSpecificExtensionsFirst    = 0xE0;
    const int kJrcpDeviceSpecificExtensionsLast     = 0xFD;
    const int kJrcpControllerConfigurationMty       = 0xFE;
    const int kJrcpFeatureControlRequestMty         = 0xFF;


    /* Function to validate the start of frame (SOF) for a raw jrcp message */
    inline bool ValidateJrcpMessage(BytesVec raw_message) {
        /* Check the SOF: Is it 0xA5? => return true. Otherwise false */
        return (raw_message[kJrcpMessageSofIndex] == kJrcpMessageSofValue);
    }

    /* Helper function to get the MTY from a raw JRCP frame. */
    inline uint8_t ParseMty(BytesVec raw_message) {
        /* Look at the index of the MTY and return its value. */
        return raw_message[kJrcpMessageMtyIndex];
    }

    /* Helper function to get the NAD from a raw JRCP frame. */
    /* Function to parse the NAD from the jrcp raw message */
    inline uint8_t ParseNad(BytesVec raw_message) {
        /* Look at the index of the NAD and return its value. */
        return raw_message[kJrcpMessageNadIndex];
    }

    /* Helper function to get the HDL from a raw JRCP frame. */
    /* Inline function to parse the custom header length */
    inline uint8_t ParseHdl(BytesVec raw_message) {
        /* Look at the index of the HDL and return its value. */
        return raw_message[kJrcpMessageHdlIndex];
    }

    /* Inline function to parse the payload length */
    inline uint32_t ParseLn(BytesVec raw_message) {
        /* Take the length as a uint32_t out of the message. Because it is stored in
        raw_message as big-endian, convert to little-endian before returning it. */
        uint32_t beln = *reinterpret_cast<const uint32_t*>
                       (&raw_message[kJrcpMessageHdlIndex + ParseHdl(raw_message) + 1]);
        return jrcplib::utils::SwapEndian<uint32_t>(beln);
    }

    /* Inline function to parse the TIL(Timing information Length) */
    inline uint8_t ParseTil(BytesVec raw_message) {
        /* Since it is only one byte and endianness does not matter, directly fetch
        the TIL from its index. */
        return raw_message[kJrcpMessageHdlIndex + ParseHdl(raw_message) + ParseLn(raw_message) + 1];
    }

    /** Base class containing data and functions which are shared by all JRCP
    message types.

    This includes:
    Position        Length  Description
    0               1       Start of Frame (sof_)
    1               1       Message Type (mty_)
    2               1       Node Address (nad_)
    3               1       Header Length (hdl_)
    4               hdl_    Header (header_)
    4 + hdl_        4       Payload Length (ln_)
    4+hdl_+4+ln_    1       Timing Info Length (til_)
    4+hdl_+4+ln_+1  8       optional, Response Timing (tr_)
    4+hdl_+4+ln_+2  8       optional, Command Timing (tc_)
    */
    class JrcpMessage {
  public:
        /* Destructor. Must be overloaded in derived classes. */
        virtual ~JrcpMessage() = 0;
        /* Function to get the raw message. Must be overloaded in derived classes. */
        virtual BytesVec GetRawMessage() const = 0;

        inline uint8_t GetSof() const { return sof_; }
        inline uint8_t GetMty() const { return mty_; }
        inline uint8_t GetNad() const { return nad_; }
        inline uint8_t GetHdl() const { return hdl_; }
        BytesVec GetHeader() const;
        inline uint32_t GetLn() const { return ln_; }
        inline uint8_t GetTil() const { return til_; }
        BytesVec GetTr() const;
        BytesVec GetTc() const;
        inline uint32_t GetPayloadStartIndex() const { return payload_start_idx_; }

        /* Makes it possible to set the timing information from outside this
        class. */
        bool SetTiming(uint8_t til, BytesVec tr, BytesVec tc);
        /* Makes it possible to set the header from outside this class. */
        void SetHeader(BytesVec header);

        inline bool IsMessageWellFormed() const { return is_msg_well_formed_; }

  protected:
        JrcpMessage(uint8_t mty,
                    uint8_t nad,
                    uint8_t hdl,
                    BytesVec header,
                    uint32_t ln);

        JrcpMessage(BytesVec raw_message);

        /** Writes binary the SOF, MTY, NAD, Header to the rawmsg. */
        void AppendPreamble(BytesVec &rawmsg) const;
        /** Writes binary the SOF, MTY, NAD, Header, LN to the rawmsg. */
        void AppendPreambleWithLn(BytesVec &rawmsg) const;
        /** Writes binary TIL + TR + TC to the rawmsg. */
        void AppendTiming(BytesVec &rawmsg) const;
        /** Writes the JRCP message header based on the V1 format */
        void AppendPreambleWithV1Format(BytesVec &rawmsg) const;
        /** Recalculates the payload_start_index_ variable from the header length.
        Use this function every time after changing hdl_. */
        inline void RecalculatePayloadStartIndex()
        {
            /* Calculate the payload start index from the header length. We have to add:
            - The index of the HDL, which is kJrcpMessageHdlIndex.
            - The length of the HDL, which is 1.
            - The size of the header, which is hdl_.
            - The length of the LN, which is 4.
            This gives the following formula:
            */
            payload_start_idx_ = kJrcpMessageHdlIndex + 1 + hdl_ + 4;
        }

        inline void SetMessageWellFormed(bool flag) { is_msg_well_formed_ = flag; }

  private:
        uint8_t  sof_;                  /* Start of Frame */
        uint8_t  mty_;                  /* Message Type */
        uint8_t  nad_;                  /* Node Address */
        uint8_t  hdl_;                  /* Header Length */
        BytesVec header_;               /* The header content */
        uint32_t ln_;                   /* Payload length */
        uint32_t payload_start_idx_;    /* Starting index of the payload */
        uint8_t  til_;                  /* Timing Information Length */
        BytesVec tr_;                   /* Response Timing */
        BytesVec tc_;                   /* Command Timing */

        /* This flag required to handle malformed messages.
         * As we parse message in constructor we don't have straightforward way
         * to handle error except throwing exceptions. But exceptions are not
         * easy to manage in C++ hence instead we use flag.
         */
        bool is_msg_well_formed_ = true;
    };

    /* MTY 00h, Request */
    class JrcpWaitForCardReqMessage : public JrcpMessage {
  public:
        JrcpWaitForCardReqMessage(const BytesVec& raw_message);
        JrcpWaitForCardReqMessage(uint8_t nad, uint32_t waiting_time_ms);
        ~JrcpWaitForCardReqMessage() override;

        BytesVec GetRawMessage() const override;

  private:
        /* [JRCP spec v 2.8] The time in milliseconds the device SHALL wait for
        a target being detected. */
        uint32_t waiting_time_ms;
    };

    /* MTY 00h, Response */
    class JrcpWaitForCardRespMessage: public JrcpMessage
    {
  public:
        JrcpWaitForCardRespMessage(uint8_t nad, const BytesVec& atr);
        ~JrcpWaitForCardRespMessage() override;

        BytesVec GetRawMessage() const override;

  private:
        /* [JRCP spec v 2.8] The Answer To Reset (ATR) information of the last
        (protocol) activation applied. */
        BytesVec atr_;
    };

    /* MTY 01h, Request/Response */
    class JrcpSendDataMessage : public JrcpMessage {
  public:
        JrcpSendDataMessage(BytesVec raw_message);
        JrcpSendDataMessage(uint8_t nad, BytesVec payload);
        ~JrcpSendDataMessage() override;

        BytesVec GetRawMessage() const override;
        BytesVec GetPayload() const { return data_; }

  private:
        /* [JRCP spec v 2.8] Plain APDU bytes of request/response */
        BytesVec data_;
    };

    /* MTY 04h, Request */
    class JrcpTerminalInfoReqMessage : public JrcpMessage {
  public:
      JrcpTerminalInfoReqMessage(const BytesVec& raw_message);
      JrcpTerminalInfoReqMessage(uint8_t nad);
      ~JrcpTerminalInfoReqMessage() override;

      void SetJrcpV1Format(bool isinv1) { is_v1_ = isinv1; }
      bool IsJrcpV1Format() const { return is_v1_; }

       BytesVec GetRawMessage() const override;

    private:
        bool is_v1_ = false;
    };

    int32_t TerminalInfoHandler(JrcpController *instance, JrcpMessage * req_msg, JrcpMessage **out);

    /* MTY 04h, Response */
    class JrcpTerminalInfoRespMessage : public JrcpMessage {
    public:
        JrcpTerminalInfoRespMessage(uint8_t nad, const BytesVec& term_info);
        ~JrcpTerminalInfoRespMessage() override;

        BytesVec GetRawMessage() const override;

        void SetJrcpV1Format(bool isinv1) { is_v1_ = isinv1; }
        bool IsJrcpV1Format() const { return is_v1_; }

  private:
      /* [JRCP spec v 2.8] Terminal information string */
      BytesVec term_info_;
      bool is_v1_ = false;
    };

    /* MTY 0Ah, request */
    class JrcpServerStatusReqMessage : public JrcpMessage {
  public:
        JrcpServerStatusReqMessage(const BytesVec& raw_message);
        JrcpServerStatusReqMessage(uint8_t nad);
        ~JrcpServerStatusReqMessage() override;

        BytesVec GetRawMessage() const override;

    };

    int32_t ServerStatusHandler(JrcpController *instance, JrcpMessage * req_msg, JrcpMessage **out);

    /* MTY 0Ah, response */
    class JrcpServerStatusRespMessage : public JrcpMessage {
  public:
        JrcpServerStatusRespMessage(uint8_t nad, JrcpGenericStatusCodes status, BytesVec ascii_msg = BytesVec());
        JrcpServerStatusRespMessage(JrcpGenericStatusCodes status, uint8_t nad);
        ~JrcpServerStatusRespMessage() override;

        BytesVec GetRawMessage() const override;

  private:
      /* [JRCP spec v 2.8, section 4.11] Generic status code */
      JrcpGenericStatusCodes    status_code_;
      /* [JRCP spec v 2.8, section 4.11] Optional ASCII encoded string */
      BytesVec                  ascii_msg_;

      using GenericStatusCodeTypeCheck = EnumCheck<JrcpGenericStatusCodes,
          JrcpGenericStatusCodes::GenericOK,
          JrcpGenericStatusCodes::GenericError,
          JrcpGenericStatusCodes::GenericTargetRemoved,
          JrcpGenericStatusCodes::GenericUnsupportedCommand>;
    };

    /* MTY 0Bh, request */
    class JrcpTimingInfoReqMessage : public JrcpMessage {
  public:
        JrcpTimingInfoReqMessage(const BytesVec& raw_message);
        JrcpTimingInfoReqMessage(uint8_t nad, TimingInfoType submty, const BytesVec& payload);
        ~JrcpTimingInfoReqMessage() override;

        BytesVec GetRawMessage() const override;

        TimingInfoType GetTimingInfoAction() const { return action_; }

  private:
        /* [JRCP spec v 2.8, section 4.12] Sub-MTY */
        TimingInfoType action_;

        using TimingInfoTypeCheck = EnumCheck<TimingInfoType,
                                    TimingInfoType::TimingStatusCode,
                                    TimingInfoType::TimingResetTimer,
                                    TimingInfoType::TimingQueryTimer,
                                    TimingInfoType::TimingSetOptions,
                                    TimingInfoType::TimingGetCurrentOptions,
                                    TimingInfoType::TimingGetAvailableOptions>;

        /* [JRCP spec v 2.8, section 4.12] Payload, depending on Sub-MTY */
        BytesVec payload_;
    };

    /* MTY 0Bh, response */
    class JrcpTimingInfoRespMessage : public JrcpMessage {
  public:

        JrcpTimingInfoRespMessage(uint8_t nad, BytesVec measured_time,
                                  TimingInfoType response_type,
                                  TimingStatusCodes status);
        ~JrcpTimingInfoRespMessage() override;

        BytesVec GetRawMessage() const override;

  private:
        /* [JRCP spec v 2.8, section 4.12] Raw payload, depending on the response
        type */
        BytesVec payload_;
        /* [JRCP spec v 2.8, section 4.12] Status code, depending on the response
        type */
        TimingStatusCodes status_;
        /* [JRCP spec v 2.8, section 4.12] Response type determining format of
        the complete payload */
        TimingInfoType response_type_;
    };

    /* MTY 0Ch, request */
    class JrcpPrepareTearingReqMessage : public JrcpMessage {
  public:
        JrcpPrepareTearingReqMessage(const BytesVec& raw_message);
        JrcpPrepareTearingReqMessage(uint8_t nad, TearingAction submty, const BytesVec& payload);
        ~JrcpPrepareTearingReqMessage() override;

        BytesVec GetRawMessage() const override;

        TearingAction GetTearingAction() const { return action_; }
        uint16_t GetTearingDeviceSpecificAction() const { return device_specific_action_; }

  private:
        /* [JRCP spec v 2.8, section 4.13] Layout of the payload (action definition)
        if it is one of the known values */
        TearingAction action_;
        /* [JRCP spec v 2.8, section 4.13] Layout of the payload (action definition)
        if it is one of the device-specific values */
        uint16_t device_specific_action_;
        /* [JRCP spec v 2.8, section 4.13] Payload of the message */
        BytesVec payload_;

        using TearingActionCheck = EnumCheck<TearingAction,
                                   TearingAction::TearingActionStatusCode,
                                   TearingAction::TearingActionResetNoPowerCycle,
                                   TearingAction::TearingActionResetPowerCycle,
                                   TearingAction::TearingActionDeviceSpecificStart,
                                   TearingAction::TearingActionDeviceSpecificEnd>;

    };

    /* MTY 0Ch, response */
    class JrcpPrepareTearingRespMessage : public JrcpMessage {
  public:

        JrcpPrepareTearingRespMessage(uint8_t nad, TearingStatusCodes status);
        ~JrcpPrepareTearingRespMessage() override;

        BytesVec GetRawMessage() const override;


  private:
        /* [JRCP spec v 2.8, section 4.13] Sub-MTY, always 0 */
        uint16_t    submty_;
        /* [JRCP spec v 2.8, section 4.13] Status code, either of Tearing OK or
        Tearing Error. */
        TearingStatusCodes status_;

    };

    /* MTY 0Dh, request */
    class JrcpEventHandlingReqMessage : public JrcpMessage {
  public:
        JrcpEventHandlingReqMessage(const BytesVec& raw_message);
        JrcpEventHandlingReqMessage(uint8_t nad, EventRequestType submty);
        ~JrcpEventHandlingReqMessage() override;

        BytesVec GetRawMessage() const override;

        inline EventRequestType GetEventRequestType() const { return reqtype_; }

  private:
        /* [JRCP spec v 2.8, section 4.14] Sub-MTY */
        EventRequestType reqtype_;

        using EventRequestTypeCheck = EnumCheck<EventRequestType,
                                                EventRequestType::EventRequestAnyEvent,
                                                EventRequestType::EventRequestNonHciEvent,
                                                EventRequestType::EventRequestOnlyHciEvent>;

    };

    /* MTY 0Dh, response */
    class JrcpEventHandlingRespMessage: public JrcpMessage
    {
  public:
        JrcpEventHandlingRespMessage(uint8_t nad, EventResponseType resptype, const BytesVec& payload);
        ~JrcpEventHandlingRespMessage() override;

        BytesVec GetRawMessage() const override;


  private:
        /* [JRCP spec v 2.8, section 4.14] Sub-MTY */
        EventResponseType resptype_;

        /* [JRCP spec v 2.8, section 4.14] Payload, depending on sub-MTY */
        BytesVec payload_;

    };

    /* MTY 0Eh, request */
    class JrcpSetIoPinReqMessage : public JrcpMessage {
  public:
        JrcpSetIoPinReqMessage(const BytesVec& raw_message);
        JrcpSetIoPinReqMessage(uint8_t nad, const BytesVec& payload);
        ~JrcpSetIoPinReqMessage() override;
        BytesVec GetRawMessage() const override;
  private:
      /* [JRCP spec v 2.8, section 4.15] Pin ID and value */
      BytesVec payload_;
      using IoPinValueCheck = EnumCheck <IoPinValue,
          IoPinValue::PinCleared,
          IoPinValue::PinSet>;
    };

    /* MTY 0Eh, response */
    class JrcpSetIoPinRespMessage: public JrcpMessage
    {
  public:
        JrcpSetIoPinRespMessage(uint8_t nad, const BytesVec& payload);
        ~JrcpSetIoPinRespMessage() override;
        BytesVec GetRawMessage() const override;
  private:
      /* [JRCP spec v 2.8, section 4.15] Value of the pin on readout after
      setting */
      BytesVec payload_;
    };

    /* MTY 30h, request */
    class JrcpWarmResetReqMessage : public JrcpMessage {
  public:
        JrcpWarmResetReqMessage(const BytesVec& raw_message);
        JrcpWarmResetReqMessage(uint8_t nad, uint32_t waiting_time_ms);
        ~JrcpWarmResetReqMessage() override;

        BytesVec GetRawMessage() const override;

  private:
        /* [JRCP spec v 2.8, section 4.16] Waiting time before the reset */
        uint32_t waiting_time_ms_;
    };

    /* MTY 30h, response */
    class JrcpWarmResetRespMessage: public JrcpMessage
    {
  public:
        JrcpWarmResetRespMessage(const BytesVec& raw_message);
        JrcpWarmResetRespMessage(uint8_t nad, const BytesVec& atr);
        ~JrcpWarmResetRespMessage() override;

        BytesVec GetRawMessage() const override;
        BytesVec GetAtr() const { return atr_; }

  private:
        /* [JRCP spec v 2.8, section 4.16] ATR of target */
        BytesVec atr_;
    };

    /* MTY 31h, request */
    class JrcpColdResetReqMessage: public JrcpMessage
    {
  public:
        JrcpColdResetReqMessage(const BytesVec& raw_message);
        JrcpColdResetReqMessage(uint8_t nad, uint32_t waiting_time_ms);
        ~JrcpColdResetReqMessage() override;

        BytesVec GetRawMessage() const override;

  private:
        /* [JRCP spec v 2.8, section 4.17] Waiting time before the reset */
        uint32_t waiting_time_ms_;
    };

    /* MTY 31h, response */
    class JrcpColdResetRespMessage: public JrcpMessage
    {
  public:
        JrcpColdResetRespMessage(const BytesVec& raw_message);
        JrcpColdResetRespMessage(uint8_t nad, const BytesVec& atr);
        ~JrcpColdResetRespMessage() override;

        BytesVec GetRawMessage() const override;
        BytesVec GetAtr() const { return atr_; }

  private:
        /* [JRCP spec v 2.8, section 4.17] ATR of remote target */
        BytesVec atr_;
    };


    /* MTY 32h, request */
    class JrcpHciSendDataReqMessage: public JrcpMessage
    {
  public:
        JrcpHciSendDataReqMessage(const BytesVec& raw_message);
        JrcpHciSendDataReqMessage(uint8_t nad, HciGateID gate, uint8_t event_type, FrameType ft, const BytesVec& payload);
        ~JrcpHciSendDataReqMessage() override;

        BytesVec GetRawMessage() const override;

        HciGateID GetGateID() const { return gateID_; }
        uint8_t GetEventType() const { return event_type_; }
        FrameType GetFrameType() const { return frame_type_; }

  private:
        /* [JRCP spec v 2.8, section 4.18] HCI Gate (G) byte of the message header */
        HciGateID gateID_;
        /* [JRCP spec v 2.8, section 4.18] Event type (ET) byte of the message header */
        uint8_t   event_type_;
        /* [JRCP spec v 2.8, section 4.18] Frame type (FT) byte of the message header.
        Can be either a CLT frame (01h) or an Iframe (02h). */
        FrameType frame_type_;
        /* [JRCP spec v 2.8, section 4.18] Event data */
        BytesVec  payload_;

        using HciGateIDCheck = EnumCheck<HciGateID,
                                         HciGateID::HciGateLoopback,
                                         HciGateID::HciGateTypeBCard,
                                         HciGateID::HciGateTypeBPrimeCard,
                                         HciGateID::HciGateTypeACard,
                                         HciGateID::HciGateTypeFCard,
                                         HciGateID::HciGateTypeBReader,
                                         HciGateID::HciGateTypeAReader,
                                         HciGateID::HciGateConnectivity,
                                         HciGateID::HciGateApdu>;

        using FrameTypeCheck = EnumCheck<FrameType,
                                         FrameType::FrameTypeClt,
                                         FrameType::FrameTypeIframe>;

    };

    /* MTY 32h, response */
    class JrcpHciSendDataRespMessage: public JrcpMessage
    {
  public:


        JrcpHciSendDataRespMessage(uint8_t nad,
                                   HciStatusCode status);
        ~JrcpHciSendDataRespMessage() override;

        BytesVec GetRawMessage() const override;
  private:
        /* [JRCP spec v 2.8, section 4.18] Specific status code for HCI Send Event */
        HciStatusCode status_;
    };

    /* MTY FFh, request */
    class JrcpFeatureControlReqMessage : public JrcpMessage {
    public:
        JrcpFeatureControlReqMessage(const BytesVec& raw_message);
        JrcpFeatureControlReqMessage(uint8_t nad, FeatureType submty, const BytesVec& payload);
        ~JrcpFeatureControlReqMessage() override;

        BytesVec GetRawMessage() const override;

        inline BytesVec GetPayload() const { return payload_; }

        inline void SetPayload(BytesVec payload) { payload_ = payload; }

        inline FeatureType GetFeatureType() const { return feature_type_; }

    private:
        /* [JRCP spec v 2.8, section 4.22] One-byte FTY */
        FeatureType feature_type_;
        /* [JRCP spec v 2.8, section 4.22] Optional payload */
        BytesVec payload_;

        using FeatureTypeCheck = EnumCheck<FeatureType,
                                           FeatureType::VersionControl,
                                           FeatureType::FeatureOption,
                                           FeatureType::ProtocolVersion,
                                           FeatureType::ControllerInfo,
                                           FeatureType::SessionId>;
    };

    int32_t FeatureControlRequestHandler(JrcpController *instance, JrcpMessage * req_msg, JrcpMessage **out);

    /* MTY FFh, response */
    class JrcpFeatureControlRespMessage: public JrcpMessage
    {
    public:
        JrcpFeatureControlRespMessage(uint8_t nad, const BytesVec& payload);
        JrcpFeatureControlRespMessage(uint8_t nad, jrcplib_feature_type_t submty, const BytesVec &payload);
        ~JrcpFeatureControlRespMessage() override;

        BytesVec GetRawMessage() const override;
    private:
        /* [JRCP spec v 2.8, section 4.22] Optional payload according to table 48 */
        BytesVec payload_;
    };

    /* Any MTY, esp. device-specific messages */
    class JrcpGenericMessage : public JrcpMessage {
    public:
        JrcpGenericMessage(BytesVec raw_message);
        JrcpGenericMessage(uint8_t mty, uint8_t nad, uint8_t hdl, BytesVec header, uint32_t ln);
        JrcpGenericMessage(uint8_t mty, uint8_t nad, uint8_t hdl, BytesVec header, BytesVec payload, uint32_t ln);
        ~JrcpGenericMessage() override;

        BytesVec GetRawMessage() const override;
        inline BytesVec GetPayload() const { return payload_ ; }
    private:
        /* Carries a payload in addition to the base class members */
        BytesVec payload_;
    };

    /* MTY FEh, request */
    class JrcpControllerReqMessage: public JrcpMessage
    {
    public:
        JrcpControllerReqMessage(BytesVec raw_message);
        JrcpControllerReqMessage(ConfigurationType conf_type, const BytesVec& payload);
        ~JrcpControllerReqMessage() override;

        BytesVec GetRawMessage() const override;

        inline ConfigurationType GetConfigurationType() const
        {
            return controller_config_type_;
        }

        inline BytesVec GetPayload() const
        {
            return payload_;
        }

        inline void SetPayload(BytesVec payload)
        {
            payload_ = payload;
        }

    private:
        /* [JRCP spec v 2.8, section 4.21] 2 byte sub-MTY */
        ConfigurationType controller_config_type_;
        /* [JRCP spec v 2.8, section 4.21] Optional payload */
        BytesVec payload_;

        using ConfigurationTypeCheck = EnumCheck<ConfigurationType,
            ConfigurationType::ListReaders,
            ConfigurationType::FreezeMapping,
            ConfigurationType::ReleaseMapping,
            ConfigurationType::ConnectTerminal,
            ConfigurationType::CloseTerminal>;
    };

    int32_t ControllerConfigurationHandler(JrcpController *instance, JrcpMessage *req_msg, JrcpMessage **out);

    /* MTY FEh, response */
    class JrcpControllerRespMessage: public JrcpMessage
    {
    public:
        JrcpControllerRespMessage(BytesVec raw_message);
        JrcpControllerRespMessage(uint8_t nad, ConfigurationType config_type, BytesVec payload);
        ~JrcpControllerRespMessage() override;

        BytesVec GetRawMessage() const override;

       inline BytesVec GetPayload() const
        {
            return payload_;
        }

        inline ConfigurationType GetConfigurationType() const
        {
            return controller_config_type_;
        }

    private:
        /* [JRCP spec v 2.8, section 4.21] The sub-MTY shall be repeated in the
        response. */
        ConfigurationType controller_config_type_;
        /* [JRCP spec v 2.8, section 4.21] Optional payload/response */
        BytesVec payload_;

        using ConfigurationTypeCheck = EnumCheck<ConfigurationType,
            ConfigurationType::ListReaders,
            ConfigurationType::FreezeMapping,
            ConfigurationType::ReleaseMapping,
            ConfigurationType::ConnectTerminal,
            ConfigurationType::CloseTerminal>;
    };

    /* Factory class to create request messages from raw JRCP frames. */
    class JrcpMessageFactory
    {
  public:
        static JrcpMessage* CreateJrcpReqMessage(BytesVec raw_message);
        static JrcpMessage* CreateJrcpRespMessage(JrcpMessageType message_type,
                                                  BytesVec payload);
  private:
        // private constructor to prevent instance creation
        JrcpMessageFactory();


    };
} // namespace jrcp

using  jrcplib_jrcp_msg_t = jrcplib::JrcpMessage;
#else
#ifndef JRCPLIB_MESSAGE_FORWARD_DECLARED_
#define JRCPLIB_MESSAGE_FORWARD_DECLARED_
typedef struct JrcpMessage jrcplib_jrcp_msg_t;
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define JRCPLIB_INVALID_MESSAGE ((jrcplib_jrcp_msg_t *)0)

/**
* Gets the Start Of Frame (SOF) byte of a JRCP message.
*
* It is the byte with index 0 in the raw JRCP message content.
*
* @param msg The message.
*
* @return The read SOF, negative return values for an error code.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_sof(
    const jrcplib_jrcp_msg_t *msg
);

/**
* Gets the Message Type (MTY) byte of a JRCP message.
*
* It is the byte with index 1 in the raw JRCP message content.
*
* @param msg The message.
*
* @return The MTY byte of the message. Valid JRCP messages have an MTY listed
*       in `JrcpMessageType` or a device-specific MTY. Negative return values
*       indicate an error.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_mty(
    const jrcplib_jrcp_msg_t *msg
);

/**
* Gets the Node Address (NAD) byte of a JRCP message.
*
* It is the byte with index 2 in the raw JRCP message content.
*
* @param msg The message.
*
* @return The NAD byte of the message. Valid NADs include the NADs of all
*       currently registered devices. Negative return values indicate an error.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_nad(
    const jrcplib_jrcp_msg_t *msg
);

/**
* Gets the Header Length (HDL) byte of a JRCP message.
*
* It is the byte with index 3 in the raw JRCP message content.
*
* @param msg The message.
*
* @return The HDL byte of the message. Negative return values indicate an error.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_hdl(
    const jrcplib_jrcp_msg_t *msg
);

/**
* Gets the Header (HD) of a JRCP message.
*
* It starts at index 4 in the raw JRCP message content.
*
* @param msg The message.
* @param buffer_len The length of the buffer parameter. It must be at least as
*       long as the HDL byte of the message. Please check `jrcplib_msg_get_hdl`
*       before calling this method.
* @param buffer The buffer which receives the header.
*
* @return The length of the header. Negative return values indicate an error.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the buffer parameter is
*       a NULL pointer.
* @retval JRCPLIB_ERR_INSUFFICIENT_BUFFER iff the buffer length is too short.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_header
(
    const jrcplib_jrcp_msg_t *msg,
    uint32_t buffer_len,
    uint8_t *buffer
);

/**
* Gets the Length (LN) of a JRCP message payload.
*
* It starts after the header in the raw JRCP message content.
*
* @param msg The message.
* @param out Pointer to a variable which receives the HDL read from the message.
*
* @return The length of the payload. Negative return values indicate an error.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_ln(const jrcplib_jrcp_msg_t *msg);

/**
* Gets the Timing Information Presence (TIL) byte of a JRCP message.
*
* It starts after the payload in the raw JRCP message content.
*
* @param msg The message.
*
* @return The encoded presence of timing information in the message. Valid
*       values in request messages: 0x00. Valid values in response messages:
*       0x00 (no timing information), 0x08 (only response timestamp), 0x10
*       (command and response timestamps). Negative return values indicate an
*       error.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_til(const jrcplib_jrcp_msg_t *msg);

/**
* Gets the Response Timestamp (TR) of a JRCP message.
*
* It starts after the TIL byte in the raw JRCP message content.
*
* @param msg The message.
* @param buffer_len The length of the buffer parameter. It must be at least eight
*       bytes long.
* @param buffer The buffer which receives the response timestamp.
*
* @return The length of the timing information. Negative return values indicate
*       an error.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the msg parameter is valid
*       but does not contain a TR.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the buffer parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_INSUFFICIENT_BUFFER iff the buffer parameter is a
*       valid pointer but the buffer_len is shorter than eight bytes.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_tr
(
    const jrcplib_jrcp_msg_t *msg,
    uint32_t buffer_len,
    uint8_t *buffer
);

/**
* Gets the Command Timestamp (TC) of a JRCP message.
*
* It starts after the TR in the raw JRCP message content.
*
* @param msg The message.
* @param buffer_len The length of the buffer parameter. It must be at least eight
*       bytes long.
* @param buffer The buffer which receives the command timestamp.
*
* @return The length of the timing information. Negative return values indicate
*       an error.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the msg parameter is valid
*       but does not contain a TC.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the buffer parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_INSUFFICIENT_BUFFER iff the buffer parameter is a
*       valid pointer but the buffer_len is shorter than eight bytes.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_tc
(
    const jrcplib_jrcp_msg_t *msg,
    uint32_t buffer_len,
    uint8_t *buffer
);

/**
* Gets the index of the first byte of the payload in the raw content of a JRCP
* message.
*
* @param msg The message.
*
* @return The payload start index. Negative return values indicate an error.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_payload_start_idx
(
    const jrcplib_jrcp_msg_t *msg
);

/**
* Gets the length of the raw content of a JRCP message as a byte buffer.
*
* @param msg The message.
*
* @return The length of the raw content. Negative return values indicate an
*       error.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the msg parameter is valid
*       but is of the wrong type. (class cast failed)
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_raw_message_length
(
    const jrcplib_jrcp_msg_t *msg
);

/**
* Gets the raw content of a JRCP message as a byte buffer.
*
* @param msg The message.
* @param buffer_len The length of the buffer parameter. It must be long enough
*       to hold the entire message. Check `jrcplib_msg_get_raw_message_length`.
* @param buffer The buffer which receives the raw content of the message.
*
* @return The length of the content. Negative return values indicate an error.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the msg parameter is valid
*       but is of the wrong type. (class cast failed)
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the buffer parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_INSUFFICIENT_BUFFER iff the buffer parameter is a
*       valid pointer but the buffer_len is shorter than the content length.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_raw_message
(
    const jrcplib_jrcp_msg_t *msg,
    uint32_t buffer_len,
    uint8_t *buffer
);

/**
* Deletes a message object after usage. Fails silently.
*
* @param msg The message.
*/
JRCPLIB_EXPORT_API void jrcplib_msg_destroy(jrcplib_jrcp_msg_t *msg);

/**
* Creates a JRCP Cold Reset Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] waiting_time_ms The reset waiting time in milli seconds
* @param[out] Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the atr parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter is NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_cold_reset_request_create
(
    uint8_t nad,
    uint32_t waiting_time_ms,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Cold Reset Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param atr_len The length of the atr parameter.
* @param atr The buffer which contains the Answer To Reset (ATR).
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the atr parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter is NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_cold_reset_response_create
(
    uint8_t nad,
    uint32_t atr_len,
    uint8_t *atr,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Cold Reset Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] waiting_time_ms The reset waiting time in milli seconds
* @param[in] header_len The length of the header.
* @param[in] header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param[in] tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param[in] tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_cold_reset_request_create_ex
(
    uint8_t nad,
    uint32_t waiting_time_ms,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Cold Reset Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param atr_len The length of the atr parameter.
* @param atr The buffer which contains the Answer To Reset (ATR).
* @param header_len The length of the header.
* @param header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_cold_reset_response_create_ex
(
    uint8_t nad,
    uint32_t atr_len,
    uint8_t *atr,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Event Handling Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] submty The value that determines request type
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the atr parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter is NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_event_handling_request_create
(
    uint8_t nad,
    jrcplib_event_request_type_t submty,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Event Handling Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param resptype The event reponse type.
* @param payload_len The length of the payload parameter.
* @param payload The buffer which contains the payload.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the payload parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_event_handling_response_create
(
    uint8_t nad,
    jrcplib_event_response_type_t resptype,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Event Handling Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] submty The value that determines request type
* @param[in] header_len The length of the header.
* @param[in] header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param[in] tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param[in] tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_event_handling_request_create_ex
(
    uint8_t nad,
    jrcplib_event_request_type_t submty,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Event Handling Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param resptype The event reponse type.
* @param payload_len The length of the payload parameter.
* @param payload The buffer which contains the payload.
* @param header_len The length of the header.
* @param header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_event_handling_response_create_ex
(
    uint8_t nad,
    jrcplib_event_response_type_t resptype,
    uint32_t payload_len,
    uint8_t *payload,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP HCI Send Data Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] gate The gate ID
* @param[in] event_type The event type
* @param[in] ft The frame type
* @param[in] payload_len The payload length
* @param[in] payload pointer to The event data
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the atr parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter is NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_hci_send_data_request_create
(
    uint8_t nad,
    jrcplib_hci_gate_id_t gate,
    uint8_t event_type,
    jrcplib_hci_frame_type_t ft,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP HCI Send Data Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param status The status response.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff out is a null pointer.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_hci_send_data_response_create
(
    uint8_t nad,
    jrcplib_hci_status_code_t status,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP HCI Send Data Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] gate The gate ID
* @param[in] event_type The event type
* @param[in] ft The frame type
* @param[in] payload_len The payload length
* @param[in] payload pointer to The event data
* @param[in] tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param[in] tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_hci_send_data_request_create_ex
(
    uint8_t nad,
    jrcplib_hci_gate_id_t gate,
    uint8_t event_type,
    jrcplib_hci_frame_type_t ft,
    uint32_t payload_len,
    uint8_t *payload,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP HCI Send Data Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param status The status response.
* @param header_len The length of the header.
* @param header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_hci_send_data_response_create_ex
(
    uint8_t nad,
    jrcplib_hci_status_code_t status,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Gets the device-specific action of a JRCP Prepare Tearing Request message.
*
* @param out The pointer to a `jrcplib_tearing_action_t` which receives the
*       result of this operation.
*
* @return The sub-MTY of the message. Negative return values indicate an error
*       code.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the msg parameter is valid
*       but is of the wrong type. (Not a Prepare Tearing Request or class cast
*       failed)
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_prepare_tearing_get_device_specific_action
(
    const jrcplib_jrcp_msg_t *msg
);

/**
* Creates a JRCP Prepare Tearing Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] submty The sub MTY defining tearing options
* @param[in] payload_len The payload length
* @param[in] payload Pointer to the payload defining actions
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the atr parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter is NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_prepare_tearing_request_create
(
    uint8_t nad,
    jrcplib_tearing_action_t submty,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Prepare Tearing Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param status The status response.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_INVALID_POINTER_ARGUMENT iff out is a null pointer.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_prepare_tearing_response_create
(
    uint8_t nad,
    jrcplib_tearing_status_code_t status,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Prepare Tearing Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] submty The sub MTY defining tearing options
* @param[in] payload_len The payload length
* @param[in] payload Pointer to the payload defining actions
* @param[in] header_len The length of the header.
* @param[in] header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param[in] tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param[in] tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_prepare_tearing_request_create_ex
(
    uint8_t nad,
    jrcplib_tearing_action_t submty,
    uint32_t payload_len,
    uint8_t *payload,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Prepare Tearing Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param status The status response.
* @param header_len The length of the header.
* @param header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_INVALID_POINTER_ARGUMENT iff out is a null pointer.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_prepare_tearing_response_create_ex
(
    uint8_t nad,
    jrcplib_tearing_status_code_t status,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Send Data Message with the given parameters.
*
* @param nad The NAD of the device.
* @param payload_len The length of the payload parameter.
* @param payload The buffer which contains the payload.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointers is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_send_data_create
(
    uint8_t nad,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Send Data Message with the given parameters.
*
* @param nad The NAD of the device.
* @param payload_len The length of the payload parameter.
* @param payload The buffer which contains the payload.
* @param header_len The length of the header.
* @param header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointers is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_send_data_create_ex
(
    uint8_t nad,
    uint32_t payload_len,
    uint8_t *payload,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Server Status Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param status The Generic JRCP status code.
* @param ascii_msg_len The length of the ascii_msg parameter.
* @param ascii_msg The buffer which contains the human readable, ASCII-encoded
*       status message..
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointers is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_server_status_response_create_with_helper_message
(
    uint8_t nad,
    jrcplib_jrcp_generic_status_code_t status,
    uint32_t ascii_msg_len,
    uint8_t *ascii_msg,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Server Status Request Message with the given parameters.
*
* @param[in] nad The NAD value.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the atr parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter is NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_server_status_request_create
(
    uint8_t nad,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Server Status Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param status The Generic JRCP status code.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff out is a NULL pointer.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_server_status_response_create
(
    uint8_t nad,
    jrcplib_jrcp_generic_status_code_t status,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Server Status Request Message with the given parameters.
*
* @param[in] nad The NAD value.
* @param[in] header_len The length of the header.
* @param[in] header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param[in] tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param[in] tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_server_status_request_create_ex
(
    uint8_t nad,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Send Data Message with the given parameters.
*
* @param nad The NAD of the device.
* @param status The Generic JRCP status code.
* @param ascii_msg_len The length of the ascii_msg parameter.
* @param ascii_msg The buffer which contains the human readable, ASCII-encoded
*       status message.
* @param header_len The length of the header.
* @param header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any pointer parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_server_status_response_create_ex
(
    uint8_t nad,
    jrcplib_jrcp_generic_status_code_t status,
    uint32_t ascii_msg_len,
    uint8_t *ascii_msg,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Set IO Pin Request Message with the given parameters.
*
* @param[in] nad The NAD of the device.
* @param[in] id The pin ID.
* @param[in] value The pin value to set or clear the pin.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the atr parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter is NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_set_io_pin_request_create
(
    uint8_t nad,
    uint8_t id,
    jrcplib_io_pin_value_t value,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Send I/O Pin Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param payload_len The length of the payload parameter.
* @param payload The buffer which contains the payload.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any pointer parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_set_io_pin_response_create
(
    uint8_t nad,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Set IO Pin Request Message with the given parameters.
*
* @param[in] nad The NAD of the device.
* @param[in] id The pin ID.
* @param[in] value The pin value to set or clear the pin.
* @param[in] header_len The length of the header.
* @param[in] header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param[in] tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param[in] tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_set_io_pin_request_create_ex
(
    uint8_t nad,
    uint8_t id,
    jrcplib_io_pin_value_t value,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Send I/O Pin Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param payload_len The length of the payload parameter.
* @param payload The buffer which contains the payload.
* @param header_len The length of the header.
* @param header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any pointer parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_set_io_pin_response_create_ex
(
    uint8_t nad,
    uint32_t payload_len,
    uint8_t *payload,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Terminal Info Request Message with the given parameters.
*
* @param[in] nad The NAD value.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the atr parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter is NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_terminal_info_request_create
(
    uint8_t nad,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Terminal Info Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param terminfo_len The length of the terminfo parameter.
* @param terminfo The buffer which contains the terminal info.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any pointer parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_terminal_info_response_create
(
    uint8_t nad,
    uint16_t terminfo_len,
    uint8_t *terminfo,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Terminal Info Request Message with the given parameters.
*
* @param[in] nad The NAD value.
* @param[in] header_len The length of the header.
* @param[in] header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param[in] tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param[in] tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_terminal_info_request_create_ex
(
    uint8_t nad,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Terminal Info Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param terminfo_len The length of the terminfo parameter.
* @param terminfo The buffer which contains the terminal info.
* @param header_len The length of the header.
* @param header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any pointer parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_terminal_info_response_create_ex
(
    uint8_t nad,
    uint16_t terminfo_len,
    uint8_t *terminfo,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Timing Info Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] submty The sub MTY defining timer options
* @param[in] payload_len The payload length
* @param[in] payload Pointer to the payload defining actions
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the atr parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter is NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_timing_info_request_create
(
    uint8_t nad,
    jrcplib_timing_info_type_t submty,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Timing Info Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param response_type The type of the response, according to the request:
*       - If the request was of type TimingResetTimer or TimingSetOptions, the
*         expected response type is TimingStatusCode. In this case, the payload
*         is ignored.
*       - For all other request types, the response type must be the same as the
*         request type. In this case, a payload must be present and 4 bytes
*         long.
* @param status The timing info status code.
* @param payload_len The length of the payload parameter.
* @param payload The buffer which contains the payload.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any pointer parameter is
*       required but evaluated to a NULL pointer.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the payload was required
*       but not of the right size.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_timing_info_response_create
(
    uint8_t nad,
    jrcplib_timing_info_type_t response_type,
    jrcplib_timing_status_code_t status,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Timing Info Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] submty The sub MTY defining timer options
* @param[in] payload_len The payload length
* @param[in] payload Pointer to the payload defining actions
* @param[in] header_len The length of the header.
* @param[in] header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param[in] tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param[in] tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_timing_info_request_create_ex
(
    uint8_t nad,
    jrcplib_timing_info_type_t submty,
    uint32_t payload_len,
    uint8_t *payload,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Timing Info Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param response_type The type of the response, according to the request:
*       - If the request was of type TimingResetTimer or TimingSetOptions, the
*         expected response type is TimingStatusCode. In this case, the payload
*         is ignored.
*       - For all other request types, the response type must be the same as the
*         request type. In this case, a payload must be present and 4 bytes
*         long.
* @param status The timing info status code.
* @param payload_len The length of the payload parameter.
* @param payload The buffer which contains the payload.
* @param header_len The length of the header.
* @param header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any pointer parameter is
*       required but evaluated to a NULL pointer.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the payload was required
*       but not of the right size.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_timing_info_response_create_ex
(
    uint8_t nad,
    jrcplib_timing_info_type_t response_type,
    jrcplib_timing_status_code_t status,
    uint32_t payload_len,
    uint8_t *payload,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Wait For Card Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] waiting_time_ms The waiting time in milli seconds.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the atr parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter is NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_wait_for_card_request_create
(
    uint8_t nad,
    uint32_t waiting_time_ms,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Wait For Card Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param payload_len The length of the payload parameter.
* @param payload The buffer which contains the payload.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any pointer parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_wait_for_card_response_create
(
    uint8_t nad,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Wait For Card Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] waiting_time_ms The waiting time in milli seconds.
* @param[in] header_len The length of the header.
* @param[in] header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param[in] tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param[in] tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_wait_for_card_request_create_ex
(
    uint8_t nad,
    uint32_t waiting_time_ms,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Wait For Card Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param payload_len The length of the payload parameter.
* @param payload The buffer which contains the payload.
* @param header_len The length of the header.
* @param header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any pointer parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_wait_for_card_response_create_ex
(
    uint8_t nad,
    uint32_t payload_len,
    uint8_t *payload,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Warm Reset Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] waiting_time_ms The reset waiting time in milli seconds
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the atr parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter is NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_warm_reset_request_create
(
    uint8_t nad,
    uint32_t waiting_time_ms,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Warm Reset Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param atr_len The length of the atr parameter.
* @param atr The buffer which contains the Answer To Reset (ATR).
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any pointer parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_warm_reset_response_create
(
    uint8_t nad,
    uint32_t atr_len,
    uint8_t *atr,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Warm Reset Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] waiting_time_ms The reset waiting time in milli seconds
* @param[in] header_len The length of the header.
* @param[in] header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param[in] tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param[in] tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_warm_reset_request_create_ex
(
    uint8_t nad,
    uint32_t waiting_time_ms,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Warm Reset Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param atr_len The length of the atr parameter.
* @param atr The buffer which contains the Answer To Reset (ATR).
* @param header_len The length of the header.
* @param header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_warm_reset_response_create_ex
(
    uint8_t nad,
    uint32_t atr_len,
    uint8_t *atr,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Modifies the given JRCP response message by setting its header information.
*
* @param msg The message whose header should be set.
* @param header_len The length of the header.
* @param header The content of the header. May be NULL, in this case the header
*       will be empty.
*
* @return Zero on success, nonzero iff an error occurred.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg argument is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the header information could not be
*       copied.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_set_header
(
    jrcplib_jrcp_msg_t * msg,
    uint32_t header_len,
    uint8_t *header
);

/**
* Modifies the given JRCP response message by setting its timing information.
*
* @param msg The message whose timing information should be set.
* @param tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
*
* @return Zero on success, nonzero iff an error occurred.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg argument is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the timing information could not be
*       copied.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_set_timing
(
    jrcplib_jrcp_msg_t * msg,
    uint8_t *tr,
    uint8_t *tc
);

/**
* Gets the event request type of a JRCP Event Handling Request message.
*
* @param msg The message.
* @param out The pointer to a `jrcplib_event_request_type_t` which receives the
*       result of this operation.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the msg parameter is valid
*       but is of the wrong type. (Not an Event Handling Request or class cast
*       failed)
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter was NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_event_handling_get_event_req_type
(
    const jrcplib_jrcp_msg_t *msg,
    jrcplib_event_request_type_t *out
);

/**
* Gets the timing action of a JRCP Timing Information Request message.
*
* @param msg The message.
* @param out The pointer to a `jrcplib_timing_info_type_t` which receives the
*       result of this operation.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the msg parameter is valid
*       but is of the wrong type. (Not a Timing Information Request or class cast
*       failed)
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter was NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_timing_info_get_action
(
    const jrcplib_jrcp_msg_t *msg,
    jrcplib_timing_info_type_t *out
);

/**
* Gets the Gate ID of a JRCP HCI Send Data Request message.
*
* @param out The pointer to a `jrcplib_hci_gate_id_t` which receives the
*       result of this operation.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the msg parameter is valid
*       but is of the wrong type. (Not a HCI Send Data Request or class cast
*       failed)
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter was NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_hci_send_data_get_gate_id
(
    const jrcplib_jrcp_msg_t *msg,
    jrcplib_hci_gate_id_t *out
);

/**
* Gets the event type of a JRCP HCI Send Data Request message.
*
* @param msg The message.
*
* @return The event type. Negative return values indicate an error.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the msg parameter is valid
*       but is of the wrong type. (Not a HCI Send Data Request or class cast
*       failed)
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_hci_send_data_get_event_type
(
    const jrcplib_jrcp_msg_t *msg
);

/**
* Gets the frame type of a JRCP HCI Send Data Request message.
*
* @param out The pointer to a `jrcplib_hci_frame_type_t` which receives the
*       result of this operation.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the msg parameter is valid
*       but is of the wrong type. (Not a HCI Send Data Request or class cast
*       failed)
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter was NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_hci_send_data_get_frame_type
(
    const jrcplib_jrcp_msg_t *msg,
    jrcplib_hci_frame_type_t *out
);

/**
* Gets the tearing action of a JRCP Prepare Tearing Request message.
*
* @param out The pointer to a `jrcplib_tearing_action_t` which receives the
*       result of this operation.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the msg parameter is valid
*       but is of the wrong type. (Not a Prepare Tearing Request or class cast
*       failed)
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter was NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_prepare_tearing_get_action
(
    const jrcplib_jrcp_msg_t *msg,
    jrcplib_tearing_action_t *out
);

/**
* Gets the feature type of a JRCP Feature Control Request message.
*
* @param msg The message.
* @param out Pointer to a variable which receives the feature type.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the msg parameter is valid
*       but is of the wrong type. (Not a Feature Control Request or class cast
*       failed)
* @retval JRCPLIB_ERR_OUT_OF_MEMORY(Not a Feature Control request or the class cast failed )
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter is null.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_feature_control_get_feature_type
(
    const jrcplib_jrcp_msg_t *msg,
    jrcplib_feature_type_t *out
);


/**
* Gets the feature type of a JRCP Feature Control Request message.
*
* @param msg     The message.
* @param payload Pointer to buffer where the payload is stored
* @param len     Len of the paylaod
*
* @return 0 indicates payload copied successfully, negative indicates an error.
*
* @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the msg parameter is
*       JRCPLIB_INVALID_MESSAGE.
* @retval JRCPLIB_ERR_OPERATION_NOT_PERMITTED iff the msg parameter is valid
*       but is of the wrong type. (Not a Feature Control Request or class cast
*       failed)
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT(The buffer to store the payload is invalid)
* @retval JRCPLIB_ERR_INSUFFICIENT_BUFFER(Insufficient buffer pointer passed to get the payload )
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_feature_control_get_payload
(
    const jrcplib_jrcp_msg_t *msg,
    uint8_t *payload,
    uint16_t len
);

/**
* Creates a JRCP Feature Control Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] submty The value that determines request type
* @param[in] payload_len The payload length
* @param[in] payload Pointer to the payload containing FTY and optional information
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the atr parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff the out parameter is NULL.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_feature_control_request_create
(
    uint8_t nad,
    jrcplib_feature_type_t submty,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Feature Control Response Message with the given parameters.
*
* @param nad The NAD of the device.
* @param payload_len The length of the response payload.
* @param payload The buffer which contains the response payload.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any pointer parameter is a
*       NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_feature_control_response_create
(
    uint8_t nad,
    uint32_t payload_len,
    uint8_t * payload,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Feature Control Request Message with the given parameters.
*
* @param[in] nad The NAD value
* @param[in] submty The value that determines request type
* @param[in] payload_len The payload length
* @param[in] payload Pointer to the payload containing FTY and optional information
* @param[in] header_len The length of the header.
* @param[in] header The content of the header. May be NULL, in this case the header
*       will be empty.
* @param[in] tr The Reponse Timing (TR) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data.
* @param[in] tc The Command Timing (TC) of the message. May be NULL, otherwise the
*       pointer must have eight bytes of valid data. If tr is not present, this
*       argument is ignored.
* @param[out] out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_feature_control_request_create_ex
(
    uint8_t nad,
    jrcplib_feature_type_t submty,
    uint32_t payload_len,
    uint8_t *payload,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Feature Control Response Message with the given parameters.
* Currently not implemented.
*
* @param message_buffer The message.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_feature_control_response_create_ex
(
    uint8_t *message_buffer,
    jrcplib_jrcp_msg_t **out
);

/**
* Creates a JRCP Generic Response Message with the given parameters.
*
* @param mty The mesage Type of the Response
* @param nad nad to identify the device
* @param hdl  Length of the Custom header
* @param header Pointer to the Custom Header (Optional if hdl > 0)
* @param payload pointer to the payload buffer
* @param payloadLn Length of the given payload.
* @param out Pointer to a variable which receives the created message object.
*
* @return An error code.
*
* @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT iff any of the pointer
*       parameters are a NULL pointer.
* @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the result could not be allocated.
*/
JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_generic_response_with_payload
(
    uint8_t mty,
    uint8_t nad,
    int8_t hdl,
    uint8_t *header,
    uint8_t *payload,
    uint32_t payloadLn,
    jrcplib_jrcp_msg_t **out
);


#ifdef __cplusplus
}
#endif

#endif // MESSAGES_H_
