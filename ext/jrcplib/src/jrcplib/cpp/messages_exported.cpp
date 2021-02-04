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
#include "../include/utils.h"
#include <cassert>
#include <cstring>
#include <functional>

using namespace jrcplib::utils;

/** Helper function for all the response message creation functions with _ex to
 *  avoid code duplication. */
static inline int32_t GetExtendedMessageWithHeaderAndTiming(
    jrcplib_jrcp_msg_t *msg,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc)
{
    /* Catch errors. */
    if (msg == nullptr)
    {
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    /* Now set the header and timing information of this message and return afterwards. */
    int32_t error = jrcplib_msg_set_header(msg, header_len, header);
    if (error != JRCPLIB_ERR_OK)
    {
        jrcplib_msg_destroy(msg);
        return error;
    }

    error = jrcplib_msg_set_timing(msg, tr, tc);
    if (error != JRCPLIB_ERR_OK)
    {
        jrcplib_msg_destroy(msg);
        return error;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_sof(const jrcplib_jrcp_msg_t *msg)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the SOF of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    /* Write to our out parameter */
    return (int32_t)msg->GetSof();
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_mty(const jrcplib_jrcp_msg_t *msg)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the MTY of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    return (uint8_t)msg->GetMty();
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_nad(const jrcplib_jrcp_msg_t *msg)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the NAD of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    return (uint8_t)msg->GetNad();
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_hdl(const jrcplib_jrcp_msg_t *msg)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the HDL of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    return (uint8_t)msg->GetHdl();
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_header(const jrcplib_jrcp_msg_t *msg, uint32_t buffer_len, uint8_t *buffer)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the header of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    if (buffer == nullptr)
    {
        /* Cannot write to a NULL buffer, so report an invalid pointer error. */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    if (buffer_len < (uint32_t)msg->GetHdl())
    {
        /* Refuse to write to a buffer that would be too short. */
        return JRCPLIB_ERR_INSUFFICIENT_BUFFER;
    }

    /* Copy the header over to the buffer memory. */
    jrcplib::BytesVec header = msg->GetHeader();
    size_t size = header.size();
    memcpy(buffer, &header[0], size);

    return (int32_t)size;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_ln(const jrcplib_jrcp_msg_t *msg)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the payload length of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    return (int32_t)msg->GetLn();
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_til(const jrcplib_jrcp_msg_t *msg)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the TIL of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    return (int32_t)msg->GetTil();
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_tr(const jrcplib_jrcp_msg_t *msg, uint32_t buffer_len, uint8_t *buffer)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the TR of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    /* Til 0x08 means there is a TR, Til 0x10 means there is TR and TC. */
    if (msg->GetTil() != JRCP_TIMING_INFO_WITH_RESPONSE_TIMESTAMP && msg->GetTil() != JRCP_TIMING_INFO_WITH_CMD_AND_RESPONSE_TIMESTAMP)
    {
        /* If the TIL indicates that there is no TR in `msg`, then we have
        nothing to do. This is misuse of the library. They could have checked
        before! */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    if (buffer == nullptr)
    {
        /* Cannot write to a NULL buffer, so report an invalid pointer error. */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    if (buffer_len < jrcplib::kJrcpMessageTrLen)
    {
        /* If the buffer is too short, then don't bother writing. */
        return JRCPLIB_ERR_INSUFFICIENT_BUFFER;
    }

    /* Copy the TR over to the given buffer. Then return the copied size. */
    jrcplib::BytesVec tr = msg->GetTr();
    size_t size = tr.size();
    memcpy(buffer, &tr[0], size);

    assert(size == jrcplib::kJrcpMessageTrLen);

    /* Since the size of the TR is always 8, it is appropriate to cast this
    number down to int32_t. */
    return (int32_t)size;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_tc(const jrcplib_jrcp_msg_t *msg, uint32_t buffer_len, uint8_t *buffer)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the TC of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    /* Til 0x10 means there is TR and TC. */
    if (msg->GetTil() != JRCP_TIMING_INFO_WITH_CMD_AND_RESPONSE_TIMESTAMP)
    {
        /* If according to the TIL there is no TC in this message, then we have
        nothing to do again. */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    if (buffer == nullptr)
    {
        /* Cannot write to a NULL buffer, so report an invalid pointer error. */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    if (buffer_len < jrcplib::kJrcpMessageTcLen)
    {
        /* If the buffer is too short, then don't bother writing. */
        return JRCPLIB_ERR_INSUFFICIENT_BUFFER;
    }

    /* Copy the TC over to the given buffer. Then return the copied size. */
    jrcplib::BytesVec tcvec = msg->GetTc();
    size_t size = tcvec.size();
    memcpy(buffer, &tcvec[0], size);

    assert(size == jrcplib::kJrcpMessageTcLen);

    /* Since the size of the TR is always 8, it is appropriate to cast this
    number down to int32_t. */
    return (int32_t)size;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_payload_start_idx(const jrcplib_jrcp_msg_t *msg)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the payload start index of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    return (int32_t)msg->GetPayloadStartIndex();
}


JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_raw_message_length(const jrcplib_jrcp_msg_t *msg)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the raw message length of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    if (MessageTypeIdCheck(msg) != true)
    {
        /* Since we need to use the function GetRawMessage(), which is abstract
        in JrcpMessage, we must make sure that we're dealing with a known message
        type that has actually implemented this method! */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    // TODO: Make sure this type fits -- which it probably isn't
    return (int32_t)msg->GetRawMessage().size();
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_raw_message(const jrcplib_jrcp_msg_t *msg, uint32_t buffer_len, uint8_t *buffer)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the raw message of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    if (MessageTypeIdCheck(msg) != true)
    {
        /* Since we need to use the function GetRawMessage(), which is abstract
        in JrcpMessage, we must make sure that we're dealing with a known message
        type that has actually implemented this method! */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    if (buffer == nullptr)
    {
        /* Cannot write to a NULL buffer. */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Get the raw message. */
    jrcplib::BytesVec raw_msg = msg->GetRawMessage();
    size_t size = raw_msg.size();

    /* Check if the buffer is long enough for the raw message. */
    if (buffer_len < size)
    {
        return JRCPLIB_ERR_INSUFFICIENT_BUFFER;
    }

    /* Copy the contents. */
    memcpy(buffer, &raw_msg[0], size);

    return (int32_t)size;
}

JRCPLIB_EXPORT_API void jrcplib_msg_destroy(jrcplib_jrcp_msg_t *msg)
{
    /* Since this does not fail even if msg is nullptr, don't worry about
    anything.*/
    delete msg;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_event_handling_get_event_req_type
(
    const jrcplib_jrcp_msg_t *msg,
    jrcplib_event_request_type_t *out
)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the event request type of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    /* Try to perform the downcast and check that it all went normal. */
    const jrcplib::JrcpEventHandlingReqMessage *event_msg = dynamic_cast<const jrcplib::JrcpEventHandlingReqMessage *>(msg);

    if (event_msg == nullptr)
    {
        /* In case the cast did not work, return an error code here. */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    return TryAssignOutParameter(out, event_msg->GetEventRequestType());
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_timing_info_get_action
(
    const jrcplib_jrcp_msg_t *msg,
    jrcplib_timing_info_type_t *out
)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the action of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    /* Perform the cast and check that it all went normal. */
    const jrcplib::JrcpTimingInfoReqMessage *timing_msg = dynamic_cast<const jrcplib::JrcpTimingInfoReqMessage *>(msg);

    if (timing_msg == nullptr)
    {
        /* In case the cast did not work, return an error code here. */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    return TryAssignOutParameter(out, timing_msg->GetTimingInfoAction());
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_hci_send_data_get_gate_id
(
    const jrcplib_jrcp_msg_t *msg,
    jrcplib_hci_gate_id_t *out
)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the gate ID of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    /* Perform the cast and check that it all went normal. */
    const jrcplib::JrcpHciSendDataReqMessage *hci_msg = dynamic_cast<const jrcplib::JrcpHciSendDataReqMessage *>(msg);

    if (hci_msg == nullptr)
    {
        /* In case the cast did not work, return an error code here. */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    return TryAssignOutParameter(out, hci_msg->GetGateID());
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_hci_send_data_get_event_type(const jrcplib_jrcp_msg_t *msg)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the event type of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    /* Perform the cast and check that it all went normal. */
    const jrcplib::JrcpHciSendDataReqMessage *hci_msg = dynamic_cast<const jrcplib::JrcpHciSendDataReqMessage *>(msg);

    if (hci_msg == nullptr)
    {
        /* In case the cast did not work, return an error code here. */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    return hci_msg->GetEventType();
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_hci_send_data_get_frame_type
(
    const jrcplib_jrcp_msg_t *msg,
    jrcplib_hci_frame_type_t *out
)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the frame type of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    /* Perform the cast and check that it all went normal. */
    const jrcplib::JrcpHciSendDataReqMessage *hci_msg = dynamic_cast<const jrcplib::JrcpHciSendDataReqMessage *>(msg);

    if (hci_msg == nullptr)
    {
        /* In case the cast did not work, return an error code here. */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    return TryAssignOutParameter(out, hci_msg->GetFrameType());
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_prepare_tearing_get_action
(
    const jrcplib_jrcp_msg_t *msg,
    jrcplib_tearing_action_t *out
)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the tearing action of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    /* Perform the cast and check that it all went normal. */
    const jrcplib::JrcpPrepareTearingReqMessage *tearing_msg = dynamic_cast<const jrcplib::JrcpPrepareTearingReqMessage *>(msg);

    if (tearing_msg == nullptr)
    {
        /* In case the cast did not work, return an error code here. */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    return TryAssignOutParameter(out, tearing_msg->GetTearingAction());
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_prepare_tearing_get_device_specific_action(const jrcplib_jrcp_msg_t *msg)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the device specific action of an invalid (NULL) message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    /* Perform the cast and check that it all went normal. */
    const jrcplib::JrcpPrepareTearingReqMessage *tearing_msg = dynamic_cast<const jrcplib::JrcpPrepareTearingReqMessage *>(msg);

    if (tearing_msg == nullptr)
    {
        /* In case the cast did not work, return an error code here. */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    return (int32_t)tearing_msg->GetTearingDeviceSpecificAction();
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_cold_reset_request_create
(
    uint8_t nad,
    uint32_t waiting_time_ms,
    jrcplib_jrcp_msg_t **out
)
{
    if (out == nullptr)
    {
        /* If we can't write to out, there is no sense in trying to create the request.
        Can't recover now */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Asssemble the message */
    *out = new jrcplib::JrcpColdResetReqMessage(nad, waiting_time_ms);

    /* Check for another lovely error condition and make sure to report it. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_cold_reset_response_create
(
    uint8_t nad,
    uint32_t atr_len,
    uint8_t *atr,
    jrcplib_jrcp_msg_t **out
)
{
    if (atr == nullptr || out == nullptr)
    {
        /* In case the ATR is NULL, there is no sense in trying to create the response.
        If we can't write to out, there is also no reason to do that.
        Can't recover now */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Construct the ATR byte vector from the buffer. */
    jrcplib::BytesVec atrvec(atr, atr + atr_len);

    /* Asssemble the message */
    *out = new jrcplib::JrcpColdResetRespMessage(nad, atrvec);

    /* Check for another lovely error condition and make sure to report it. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_cold_reset_request_create_ex
(
    uint8_t nad,
    uint32_t waiting_time_ms,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_cold_reset_request_create(nad, waiting_time_ms, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

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
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_cold_reset_response_create(nad, atr_len, atr, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_event_handling_request_create
(
    uint8_t nad,
    jrcplib_event_request_type_t submty,
    jrcplib_jrcp_msg_t **out
)
{
    if (out == nullptr)
    {
        /* If we can't write to out, there is no sense in trying to create the request.
        Can't recover now */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Asssemble the message */
    *out = new jrcplib::JrcpEventHandlingReqMessage(nad, submty);

    /* Check for another lovely error condition and make sure to report it. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_event_handling_response_create
(
    uint8_t nad,
    jrcplib_event_response_type_t resptype,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
)
{
    if (payload == nullptr || out == nullptr)
    {
        /* Do not create the message if the payload or eventHeader is NULL. */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Create the byte vectors from the data */
    jrcplib::BytesVec payloadvec(payload, payload + payload_len);

    /* Assemble the resulting message, heap-allocated */
    *out = new jrcplib::JrcpEventHandlingRespMessage(nad, resptype, payloadvec);

    if (*out == nullptr)
    {
        /* If the allocation failed, set error and return. */
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_event_handling_request_create_ex
(
    uint8_t nad,
    jrcplib_event_request_type_t submty,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_event_handling_request_create(nad, submty, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

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
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_event_handling_response_create(nad, resptype, payload_len, payload, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_hci_send_data_request_create
(
    uint8_t nad,
    jrcplib_hci_gate_id_t gate,
    uint8_t event_type,
    jrcplib_hci_frame_type_t ft,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
)
{
    jrcplib::BytesVec payload_vec;

    if (out == nullptr)
    {
        /* If we can't write to out, there is no sense in trying to create the request.
        Can't recover now */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    if (payload != nullptr && payload_len != 0)
    {
        /* Put the payload in the payload vector. */
        payload_vec.assign(payload, payload + payload_len);
    }

    /* Asssemble the message */
    *out = new jrcplib::JrcpHciSendDataReqMessage(nad, gate, event_type, ft, payload_vec);

    /* Check for another lovely error condition and make sure to report it. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_hci_send_data_response_create
(
    uint8_t nad,
    jrcplib_hci_status_code_t status,
    jrcplib_jrcp_msg_t **out
)
{
    if (out == nullptr)
    {
        /* Cannot write to out */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Assemble the resulting message, heap-allocated */
    *out = new jrcplib::JrcpHciSendDataRespMessage(nad, status);

    if (*out == nullptr)
    {
        /* If the allocation failed, set error and return. */
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

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
)
{
    uint8_t header[] = { (uint8_t)gate, event_type, (uint8_t)ft };
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_hci_send_data_request_create(nad, gate, event_type, ft, payload_len, payload, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, JRCP_HCI_HEADER_SIZE, header, tr, tc);
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_hci_send_data_response_create_ex
(
    uint8_t nad,
    jrcplib_hci_status_code_t status,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_hci_send_data_response_create(nad, status, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_prepare_tearing_request_create
(
    uint8_t nad,
    jrcplib_tearing_action_t submty,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
)
{
    if (payload == nullptr || out == nullptr)
    {
        /* In case the payload is NULL, there is no sense in trying to create the request.
        If we can't write to out, there is also no reason to do that.
        Can't recover now */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Construct the payload_vec byte vector. */
    jrcplib::BytesVec payload_vec(payload, payload + payload_len);

    /* Asssemble the message */
    *out = new jrcplib::JrcpPrepareTearingReqMessage(nad, submty, payload_vec);

    /* Check for another lovely error condition and make sure to report it. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_prepare_tearing_response_create
(
    uint8_t nad,
    jrcplib_tearing_status_code_t status,
    jrcplib_jrcp_msg_t **out
)
{
    if (out == nullptr)
    {
        /* Cannot write to out */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Assemble the resulting message, heap-allocated */
    *out = new jrcplib::JrcpPrepareTearingRespMessage(nad, status);

    if (*out == nullptr)
    {
        /* If the allocation failed, set error and return. */
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

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
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_prepare_tearing_request_create(nad, submty, payload_len, payload, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_prepare_tearing_response_create_ex
(
    uint8_t nad,
    jrcplib_tearing_status_code_t status,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_prepare_tearing_response_create(nad, status, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}


JRCPLIB_EXPORT_API int32_t jrcplib_msg_send_data_create
(
    uint8_t nad,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
)
{
    if (payload == nullptr || out == nullptr)
    {
        /* Do not create the message if the payload or out is NULL. */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Put the payload in a vector */
    jrcplib::BytesVec payloadvec(payload, payload + payload_len);

    /* Assemble the message, heap-allocated. */
    *out = new jrcplib::JrcpSendDataMessage(nad, payloadvec);

    if (*out == nullptr)
    {
        /* Allocation of the result failed => report error and return nullptr. */
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}


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
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_send_data_create(nad, payload_len, payload, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}


JRCPLIB_EXPORT_API int32_t jrcplib_msg_server_status_response_create_with_helper_message
(
    uint8_t nad,
    jrcplib_jrcp_generic_status_code_t status,
    uint32_t ascii_msg_len,
    uint8_t *ascii_msg,
    jrcplib_jrcp_msg_t **out
)
{
    if ((ascii_msg == nullptr && ascii_msg_len > 0) || out == nullptr)
    {
        /* Ascii message expected but does not exist because it is NULL: fail */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Put the ASCII message in a vector if it exists. */
    jrcplib::BytesVec payloadvec;

    if (ascii_msg_len > 0)
    {
        payloadvec.assign(ascii_msg, ascii_msg + ascii_msg_len);
    }

    /* Assemble the result */
    *out = new jrcplib::JrcpServerStatusRespMessage(nad, status, payloadvec);

    if (*out == nullptr)
    {
        /* Allocation failed => report and return nullptr */
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_server_status_request_create
(
uint8_t nad,
jrcplib_jrcp_msg_t **out
)
{
    if (out == nullptr)
    {
        /* In case we can't write to out, there is no sense in trying to create the request.
        Can't recover now */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Asssemble the message */
    *out = new jrcplib::JrcpServerStatusReqMessage(nad);

    /* Check for another lovely error condition and make sure to report it. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_server_status_response_create
(
    uint8_t nad,
    jrcplib_jrcp_generic_status_code_t status,
    jrcplib_jrcp_msg_t **out
)
{
    if (out == nullptr)
    {
        /* Cannot write to out */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Assemble a status response from the arguments */
    *out = new jrcplib::JrcpServerStatusRespMessage(status, nad);

    if (*out == nullptr)
    {
        /* Return null if new returned null */
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_server_status_request_create_ex
(
uint8_t nad,
uint32_t header_len,
uint8_t *header,
uint8_t *tr,
uint8_t *tc,
jrcplib_jrcp_msg_t **out
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_server_status_request_create(nad, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

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
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_server_status_response_create_with_helper_message(nad, status, ascii_msg_len, ascii_msg, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_set_io_pin_request_create
(
    uint8_t nad,
    uint8_t id,
    jrcplib_io_pin_value_t value,
    jrcplib_jrcp_msg_t **out
)
{
    if (out == nullptr)
    {
        /* If we can't write to out, there is no sense in trying to create the request.
        Can't recover now */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Construct the payload byte vector from given pin id and value. */
    jrcplib::BytesVec pinvec;
    pinvec.reserve(2);
    pinvec.push_back(id);
    pinvec.push_back((uint8_t)value);

    /* Asssemble the message */
    *out = new jrcplib::JrcpSetIoPinReqMessage(nad, pinvec);

    /* Check for another lovely error condition and make sure to report it. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_set_io_pin_response_create
(
    uint8_t nad,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
)
{
    if (payload == nullptr || out == nullptr)
    {
        /* Can't create a message from NULL payload. */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Create the payload vector */
    jrcplib::BytesVec payloadvec(payload, payload + payload_len);

    /* Allocate and construct the result */
    *out = new jrcplib::JrcpSetIoPinRespMessage(nad, payloadvec);

    if (*out == nullptr)
    {
        /* Return nullptr on allocation failure */
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

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
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_set_io_pin_request_create(nad, id, value, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

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
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_set_io_pin_response_create(nad, payload_len, payload, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_terminal_info_request_create
(
    uint8_t nad,
    jrcplib_jrcp_msg_t **out
)
{
    if (out == nullptr)
    {
        /* If we can't write to out,, there is no sense in trying to create the request.
        Can't recover now */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Asssemble the message */
    *out = new jrcplib::JrcpTerminalInfoReqMessage(nad);

    /* Check for another lovely error condition and make sure to report it. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_terminal_info_response_create
(
    uint8_t nad,
    uint16_t terminfo_len,
    uint8_t *terminfo,
    jrcplib_jrcp_msg_t **out
)
{
    if (terminfo == nullptr || out == nullptr)
    {
        /* If terminfo or out is NULL, do not try creating a response object. */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Create a payload vector based on the arguments. */
    jrcplib::BytesVec terminfovec(terminfo, terminfo + terminfo_len);

    /* Create the message on the heap */
    *out = new jrcplib::JrcpTerminalInfoRespMessage(nad, terminfovec);

    if (*out == nullptr)
    {
        /* Return nullptr if heap-allocation failed. */
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_terminal_info_request_create_ex
(
    uint8_t nad,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_terminal_info_request_create(nad, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

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
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_terminal_info_response_create(nad, terminfo_len, terminfo, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_timing_info_request_create
(
    uint8_t nad,
    jrcplib_timing_info_type_t submty,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
)
{
    jrcplib::BytesVec payload_vec;

    if (out == nullptr)
    {
        /* If we can't write to out,, there is no sense in trying to create the request.
        Can't recover now */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    if (payload != nullptr && payload_len != 0)
    {
        /* Put the payload in the payload vector. */
        payload_vec.assign(payload, payload + payload_len);
    }

    /* Asssemble the message */
    *out = new jrcplib::JrcpTimingInfoReqMessage(nad, submty, payload_vec);

    /* Check for another lovely error condition and make sure to report it. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_timing_info_response_create
(
    uint8_t nad,
    jrcplib_timing_info_type_t response_type,
    jrcplib_timing_status_code_t status,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
)
{
    jrcplib::BytesVec payloadvec;

    if (out == nullptr)
    {
        /* Don't create if out is null */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Because the JRCP specification only allows certain Sub-MTYs in Timing Infor-
    mation responses, we need to make sure we're not violating it.
    This is done using a switch statement. */
    bool check = false;
    switch (response_type)
    {
    case TimingStatusCode:
        /* This one is allowed and unchanged. */
        check = true;
        break;
    case TimingSetOptions: /* Fall through is intentional */
    case TimingResetTimer:
        /* Doesn't make sense for a response. Since the expected response to
        one of these two is TimingStatusCode, set it to this value before
        continuing. */
        response_type = TimingStatusCode;
        check = true;
        break;
    case TimingGetCurrentOptions: /* Fall through is intentional */
    case TimingGetAvailableOptions:
        /* We need a payload for these cases which is exactly
        - 4 bytes long
        - not NULL
        So make sure we meet these conditions. */
        if (payload_len != JRCP_SIZE_OF_PAYLOAD_LENGTH)
        {
            /* Error reporting is done down the road in this case. */
            check = false;
        }
        else if (payload == nullptr)
        {
            /* For specific errors we return directly. */
            return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
        }
        else
        {
            /* Put the payload in the payload vector. */
            payloadvec.assign(payload, payload + payload_len);
            check = true;
        }
        break;
    case TimingQueryTimer:
        /* We need a payload for these cases which is exactly
        - 8 bytes long
        - not NULL
        So make sure we meet these conditions. */
        if (payload_len != JRCP_TIMING_INFO_WITH_RESPONSE_TIMESTAMP)
        {
            check = false;
        }
        else if (payload == nullptr)
        {
            /* For specific errors we return directly. */
            return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
        }
        else
        {
            /* Put the payload in the payload vector. */
            payloadvec.assign(payload, payload + payload_len);
            check = true;
        }
        break;
    default:
        /* Fail by default. */
        check = false;
        break;
    }

    if (check == false)
    {
        /* If the check failed in one of the above branches, give a generic
        YOU DID NOT READ THE MANUAL error back to the caller. */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    /* The result message can be assembled now. */
    *out = new jrcplib::JrcpTimingInfoRespMessage(nad, payloadvec, response_type, status);

    /* If allocation failed, set an error and return null. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

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
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_timing_info_request_create(nad, submty, payload_len, payload, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

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
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_timing_info_response_create(nad, response_type, status, payload_len, payload, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_wait_for_card_request_create
(
    uint8_t nad,
    uint32_t waiting_time_ms,
    jrcplib_jrcp_msg_t **out
)
{
    if (out == nullptr)
    {
        /* If we can't write to out, there is no sense in trying to create the request.
        Can't recover now */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Asssemble the message */
    *out = new jrcplib::JrcpWaitForCardReqMessage(nad, waiting_time_ms);

    /* Check for another lovely error condition and make sure to report it. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_wait_for_card_response_create
(
    uint8_t nad,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
)
{
    if (payload == nullptr || out == nullptr)
    {
        /* We need a valid payload and somewhere to write the result, so fail in
        case we don't have that. */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Copy the payload into the vector. */
    jrcplib::BytesVec payloadvec(payload, payload + payload_len);

    /* Create the resulting message object. */
    *out = new jrcplib::JrcpWaitForCardRespMessage(nad, payloadvec);

    if (*out == nullptr)
    {
        /* Fail if allocation went wrong. */
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_wait_for_card_request_create_ex
(
    uint8_t nad,
    uint32_t waiting_time_ms,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_wait_for_card_request_create(nad, waiting_time_ms, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

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
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_wait_for_card_response_create(nad, payload_len, payload, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_warm_reset_request_create
(
    uint8_t nad,
    uint32_t waiting_time_ms,
    jrcplib_jrcp_msg_t **out
)
{
    if (out == nullptr)
    {
        /* If we can't write to out, there is no sense in trying to create the request.
        Can't recover now */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Asssemble the message */
    *out = new jrcplib::JrcpWarmResetReqMessage(nad, waiting_time_ms);

    /* Check for another lovely error condition and make sure to report it. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_warm_reset_response_create
(
    uint8_t nad,
    uint32_t atr_len,
    uint8_t *atr,
    jrcplib_jrcp_msg_t **out
)
{
    if (atr == nullptr || out == nullptr)
    {
        /* We need a valid ATR and output pointer for this message type. */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /*Construct a vector from the ATR buffer*/
    jrcplib::BytesVec atrvec(atr, atr + atr_len);

    /*Put everything into a warm reset response message*/
    *out = new jrcplib::JrcpWarmResetRespMessage(nad, atrvec);

    if (*out == nullptr)
    {
        /* Return if allocation failed. */
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_warm_reset_request_create_ex
(
    uint8_t nad,
    uint32_t waiting_time_ms,
    uint32_t header_len,
    uint8_t *header,
    uint8_t *tr,
    uint8_t *tc,
    jrcplib_jrcp_msg_t **out
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_warm_reset_request_create(nad, waiting_time_ms, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

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
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_warm_reset_response_create(nad, atr_len, atr, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}


JRCPLIB_EXPORT_API int32_t jrcplib_msg_feature_control_get_feature_type(
    const jrcplib_jrcp_msg_t *msg,
    FeatureType *out
)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot get the feature type of a NULL message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    /* Try to perform the downcast to the Feature Control Request messsage class */
    const jrcplib::JrcpFeatureControlReqMessage *feature_req = dynamic_cast<const jrcplib::JrcpFeatureControlReqMessage *>(msg);

    if (feature_req == nullptr)
    {
        /* If allocation of the resuylt failed, return unknown feature type */
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return TryAssignOutParameter(out, feature_req->GetFeatureType());
}



JRCPLIB_EXPORT_API int32_t jrcplib_msg_feature_control_get_payload(
    const jrcplib_jrcp_msg_t *msg,
    uint8_t *payload,
    uint16_t len
)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Need a valid message for this. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    if (payload == nullptr)
    {
        /* If the buffer is null, report an error and return as well. Can't write
        to null*/
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Try to cast down here. */
    const jrcplib::JrcpFeatureControlReqMessage *feature_req = dynamic_cast<const jrcplib::JrcpFeatureControlReqMessage *>(msg);
    if (nullptr == feature_req)
    {
        /* If that did not work, return with error. */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    /* Get the payload and put it in msgvec.*/
    jrcplib::BytesVec msgvec = feature_req->GetPayload();

    if (len < msgvec.size())
    {
        /* Check whether the length of the passed buffer is long enough. */
        return JRCPLIB_ERR_INSUFFICIENT_BUFFER;
    }

    /* Copy the  payload over to the buffer. */
    memcpy(payload, &msgvec[0], msgvec.size());
    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_feature_control_request_create
(
    uint8_t nad,
    jrcplib_feature_type_t submty,
    uint32_t payload_len,
    uint8_t *payload,
    jrcplib_jrcp_msg_t **out
)
{
    jrcplib::BytesVec payload_vec;

    if (out == nullptr)
    {
        /* If we can't write to out, there is no sense in trying to create the request.
        Can't recover now */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    if (payload != nullptr && payload_len != 0)
    {
        /* Put the payload in the payload vector. */
        payload_vec.assign(payload, payload + payload_len);
    }

    /* Asssemble the message */
    *out = new jrcplib::JrcpFeatureControlReqMessage(nad, submty, payload_vec);

    /* Check for another lovely error condition and make sure to report it. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_feature_control_response_create
(
    uint8_t nad,
    uint32_t payload_len,
    uint8_t * payload,
    jrcplib_jrcp_msg_t **out
)
{
    if (nullptr == payload || *out == nullptr)
    {
        /* If the payload is a null pointer, can' t continue.
        If out is null, we have nowhere to put the created message, can't continue
        either. */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Create a response  with this payload vector. */
    jrcplib::BytesVec msgvec(payload, payload + payload_len);
    *out = new jrcplib::JrcpFeatureControlRespMessage(nad, msgvec);

    /* If the allocation failed, report the error and return. */
    if (*out == nullptr)
    {
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}

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
)
{
    /* Create the underlying message using the other API function without _ex. */
    jrcplib_msg_feature_control_request_create(nad, submty, payload_len, payload, out);
    return GetExtendedMessageWithHeaderAndTiming(*out, header_len, header, tr, tc);
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_feature_control_response_create_ex
(
    uint8_t *message_buffer,
    jrcplib_jrcp_msg_t **out
)
{
    // TODO: IMPLEMENT!!!
    return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_set_header
(
    jrcplib_jrcp_msg_t * msg,
    uint32_t header_len,
    uint8_t *header
)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* We cannot set the header of an invalid message. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    if (header == nullptr || header_len == 0)
    {
        /* If the header is a nullptr or it just doesn't have a length, put
        an empty vector in the header of the message. */
        msg->SetHeader(jrcplib::BytesVec());
    }
    else
    {
        /* Put the header in a vector of bytes */
        jrcplib::BytesVec headervec(header, header + header_len);

        /* Check that the length of the vector matches the length that
        was passed to the function, otherwise report the error */
        if (headervec.size() != header_len)
        {
            return JRCPLIB_ERR_OUT_OF_MEMORY;
        }

        /* And now set the header to this value. */
        msg->SetHeader(headervec);
    }

    /* In both cases, return successfully. */
    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_set_timing
(
    jrcplib_jrcp_msg_t * msg,
    uint8_t *tr,
    uint8_t *tc
)
{
    if (msg == JRCPLIB_INVALID_MESSAGE)
    {
        /* Cannot work with an invalid message, so report and return. */
        return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
    }

    /* Initialize two vectors, one for TR, one for TC. */
    jrcplib::BytesVec trvec, tcvec;

    /* This variable starts at 0 and is counted up in the below `if` clauses to
    calculate the value that we must put in the TIL before returning from
    this function.*/
    int8_t til = 0;

    /* If TR exists, we can use it directly. */
    if (tr != nullptr)
    {
        /* Set up trvec to reflect the fact that we have TR info, and set
        TIL to 8, because that is the value that we need to put in the TIL
        if we only have a TR. In case we find we also have a TC, the TIL gets
        incremented to 0x10 one step later. */
        trvec.assign(tr, tr + JRCP_TIMING_INFO_WITH_RESPONSE_TIMESTAMP);
        til = JRCP_TIMING_INFO_WITH_RESPONSE_TIMESTAMP;

        /* The TC can only be used if a TR is also present, so do it in this
        nested if clause. */
        if (tc != nullptr)
        {
            tcvec.assign(tc, tc + JRCP_TIMING_INFO_WITH_RESPONSE_TIMESTAMP);
            til = JRCP_TIMING_INFO_WITH_CMD_AND_RESPONSE_TIMESTAMP;
        }
    }

    /* Now set the timing information. */
    if (msg->SetTiming(til, trvec, tcvec) != true)
    {
        /* If it did not work, report and return. */
        return JRCPLIB_ERR_OPERATION_NOT_PERMITTED;
    }

    return JRCPLIB_ERR_OK;
}

JRCPLIB_EXPORT_API int32_t jrcplib_msg_get_generic_response_with_payload
(
    uint8_t mty,
    uint8_t nad,
    int8_t hdl,
    uint8_t *header,
    uint8_t *payload,
    uint32_t payloadLn,
    jrcplib_jrcp_msg_t **out
)
{
    if ((nullptr == payload && 0 != payloadLn) || (out == nullptr))
    {
        /* If the payload pointer is NULL, we cannot write to it, so report and
        return. Same for out. */
        return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
    }

    /* Create an empty vector for the header of the result. */
    jrcplib::BytesVec headervec;

    /* If a header is actually present, put it in headervec. */
    if (hdl > 0 && nullptr != header)
    {
        headervec.assign(header, header + hdl);
    }

    /* Put the payload in payloadvec. We already checked that it exists. */
    jrcplib::BytesVec payloadvec(payload, payload + payloadLn);

    /* Assemble the resulting generic message. */
    *out = new jrcplib::JrcpGenericMessage(mty, nad, hdl, headervec, payloadvec, payloadLn);

    if (*out == nullptr)
    {
        /* If the allocation of the generic message failed, return null here now. */
        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    return JRCPLIB_ERR_OK;
}
