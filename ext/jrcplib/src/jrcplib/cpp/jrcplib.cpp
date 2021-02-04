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
#include <jrcplib/jrcplib.h>
#include <jrcplib/device.h>
#include "../include/utils.h"

#include <memory>
#include <cassert>


namespace jrcplib {
    using namespace utils;

    static const std::string JRCP_CONTROLLER_IDENTIFIER("JRCP Protocol 2+");

    /**
    * Function to add the controller device with NAD 0xFF.
    *
    * @param instance The JRCP controller instance
    * @param protocol_version The protocol version of the controller
    *
    * @return void
    */
    static void AddControllerDevice(JrcpController *libInstance, jrcplib_protocol_supported_t protocol_version)
    {
        int32_t status = JRCPLIB_ERR_OK;
        std::shared_ptr<jrcplib::Device> dev = std::make_shared<jrcplib::Device>(JRCP_SERVER_NAD, JRCP_CONTROLLER_IDENTIFIER);

        /* Add the message handlers supported by the controller device */
        std::map<uint8_t, jrcplib::FeatureHandlerFunc>controller_handler_table = {
            { kJrcpControllerConfigurationMty, ControllerConfigurationHandler },
            { kJrcpFeatureControlRequestMty, FeatureControlRequestHandler },
            { kJrcpServerStatusMty, ServerStatusHandler },
            { kJrcpTerminalInformationMty, TerminalInfoHandler }
        };

        /* Register all the feature handlers from above. */
        for (auto idx = controller_handler_table.begin(); idx != controller_handler_table.end(); ++idx)
        {
            jrcplib::FeatureHandler fh = std::make_pair(protocol_version, idx->second);
            status = dev->RegisterFeatureHandler(idx->first, fh);
            if (JRCPLIB_ERR_OK != status)
            {
                break;
            }
        }

        /* Check whether the handlers for controller are registered correctly */
        if (JRCPLIB_ERR_OK == status)
        {
            /* Finally  add the controller device */
            jrcplib::DeviceRegistry &devreg = libInstance->GetDeviceRegistry();
            devreg.AddDevice(dev);
        }
    }

    /** Public function to check the protocol version support by the JRCP library
    *
    * @param protocol_version The protocol version of the controller
    *
    * @return boolean type
    * @retval true if protocol version is supported else return false
    */
    bool IsProtocolVersionSupported(jrcplib_protocol_supported_t protocol_version)
    {
        if (true == ProtocolSupportedCheck::is_value(protocol_version))
        {
            return true;
        }
        return false;
    }


    /** Public function to create and return the JRCP instance
    *
    * @param protocol_version The protocol version of the controller
    * @param identifier_length The jrcp controller identifier length
    * @param controller_identifier The jrcp controller identifier description
    * @param out The pointer to the jrcpcontroller
    *
    * @return A JRCP error code
    * @retval JRCPLIB_ERR_OUT_OF_MEMORY iff the creation of instance fails
    * @retval JRCPLIB_ERR_PROTOCOL_UNSUPPORTED iff the protocol version is not supported
    * @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT if the controller identifier or the pointer to out parameter is null
    * @retval JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT iff the identifier length is 0
    * @retval JRCPLIB_ERR_OK iff JRCP instance is created successdully
    */
    int32_t Init(jrcplib_protocol_supported_t protocol_version, size_t identifier_length, const char* controller_identifier, JrcpController **out)
    {
        /*Validating the received Parameters before creating the controller instance*/
        if (true != IsProtocolVersionSupported(protocol_version))
        {
            return JRCPLIB_ERR_PROTOCOL_UNSUPPORTED;
        }
        if (NULL == controller_identifier || NULL == out)
        {
            return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
        }

        if (0 == identifier_length)
        {
            return JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT;
        }

        // The default constructors for Controller and DeviceRegistry are called in here.
        // No need to initialize these objects by themselves.
        JrcpController *libInstance = new jrcplib::JrcpController(protocol_version, std::string(controller_identifier, identifier_length));

        if (nullptr != libInstance)
        {
            /* No error occurred during the creation of Controller. Add the controller device with NAD 0xFF */
            AddControllerDevice(libInstance, protocol_version);

            return TryAssignOutParameter(out, libInstance);
        }

        return JRCPLIB_ERR_OUT_OF_MEMORY;
    }

    /* Default constructor for the JrcpController */
    JrcpController::JrcpController() :
        device_registry_instance_(),
        controller_version_(JrcpV2),
        controller_identifier_(std::string())
    {
    }

    JrcpController::JrcpController(jrcplib_protocol_supported_t version, std::string identifier)
        : device_registry_instance_(),
          controller_version_(version),
          controller_identifier_(identifier)
    {
    }

    /** Public function to process the raw jrcp message
    *
    * @param input_message The input message buffer
    * @param output_response The pointer to the jrcp response object
    * @param error_code the error generated from processMessage
    *
    * @return A JRCP error code
    *
    * @retval JRCPLIB_ERR_OK iff request object is created successfully
    */
    int32_t JrcpController::ProcessErrorCode(uint8_t * input_message, jrcplib_jrcp_msg_t **output_response, int32_t error_code)
    {
        uint8_t mty = input_message[1];
        uint8_t nad = input_message[2];
        /*In Error Response for HCI command where the hdl is non zero need not be same, so directly assigning as zero*/
        uint8_t hdl = 0;
        jrcplib_jrcp_generic_status_code_t status = JrcpGenericStatusCodes::GenericError;
        BytesVec payloadvec;
        BytesVec headervec;

        /* The following JRCP Request messages don't contain SubMTY, so response message should not contain either */
        if ((kJrcpServerStatusMty != mty)
			&& (kJrcpWaitForCardMty != mty)
			&& (kJrcpSendDataMty != mty)
			&& (kJrcpWarmResetMty != mty)
			&& (kJrcpColdResetMty != mty))
        {
            /*Adding Empty Sub-Mty is common across all Error Response*/
            payloadvec.push_back(0x00);
            payloadvec.push_back(0x00);
        }

        if ((error_code == JRCPLIB_ERR_INVALID_DEVICE)
            || (error_code == JRCPLIB_ERR_NO_DEVICE_REGISTERED))
        {
            status = GenericTargetRemoved;
        }
        else if ((error_code == JRCPLIB_ERR_PROTOCOL_UNSUPPORTED)
            || (error_code == JRCPLIB_ERR_OPERATION_NOT_PERMITTED)
            || (error_code == JRCPLIB_ERR_NO_HANDLER_REGISTERED))
        {
            status = GenericUnsupportedCommand;
        }
        else if ((error_code == JRCPLIB_ERR_INVALID_POINTER_ARGUMENT)
            || (error_code == JRCPLIB_ERR_INVALID_MESSAGE_ARGUMENT)
            || (error_code == JRCPLIB_ERR_INSUFFICIENT_BUFFER)
            || (error_code == JRCPLIB_ERR_OUT_OF_MEMORY)
            || (error_code == JRCPLIB_ERR_INVALID_SOCKET)
            || (error_code == JRCPLIB_ERR_CLIENT_SOCKET_MISMATCH)
            || (error_code == JRCPLIB_ERR_DESCRIPTION_IN_USE)
            || (error_code == JRCPLIB_ERR_DEVICE_IN_USE)
            || (error_code == JRCPLIB_ERR_INVALID_HANDLER)
            || (error_code == JRCPLIB_ERR_HANDLER_ALREADY_PRESENT)
            || (error_code == JRCPLIB_ERR_MALFORMED_MESSAGE))
        {
            status = GenericError;
        }

        /*Coping the Status Code in the payload*/
        BytesVec statusCode = Uint16ToBEBytesVec((uint16_t)status);
        payloadvec.insert(payloadvec.end(), statusCode.begin(), statusCode.end());

        /*Generating the Error Response with the payload framed associated with error code*/
        *output_response = new jrcplib::JrcpGenericMessage(mty, nad, hdl, headervec, payloadvec, (uint32_t)payloadvec.size());

        /*As the Response object is created returning ERR_OK is safe, so that the data from object is sent on network interface*/
        return JRCPLIB_ERR_OK;
    }

    /** Public function to process the raw jrcp message
    *
    * @param input_message The input message buffer
    * @param output_response The pointer to the jrcp response object
    * @param is_jrcp_v1 Boolean value for getting the JRCP format, true for JRCP V1 and false for JRCP V2
    *
    * @return A JRCP error code
    *
    * @retval JRCPLIB_ERR_MALFORMED_MESSAGE iff the request message object cannot be created
    * @retval JRCPLIB_ERR_NO_DEVICE_REGISTERED iff message on the requested NAD is not registered
    * @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT if the any null pointer is encountered
    * @retval JRCPLIB_ERR_NO_HANDLER_REGISTERED iff the requested handler is not registered
    * @retval JRCPLIB_ERR_OK iff request object is created successfully
    */
    int32_t JrcpController::ProcessMessage(BytesVec input_message, jrcplib_jrcp_msg_t **output_response)
    {
        if (0 == input_message.size())
        {
            /* Cannot use a NULL buffer */
            return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
        }

        /* In case of Terminal Info message it is coming in format of V1
         * as part of protocol version negotiation. We will convert it to V2 format*/
        bool is_v1_terminfo = ConvertTerminalInfoMsgToV2Format(input_message);

        /* Get the device from the registry and return with error if it is not registered. */
        std::shared_ptr<jrcplib::Device> device = device_registry_instance_[(ParseNad(input_message))];
        if (nullptr == device)
        {
            return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
        }

        /* Get the feature handler from the device and check whether that gave us an error.*/
        const FeatureHandler* fh;
        int32_t retval = device->GetFeatureHandler(ParseMty(input_message), &fh);
        if (JRCPLIB_ERR_OK != retval)
        {
            return retval;
        }

        /* Then construct the message object and execute the found feature handler. */
        if (nullptr != fh)
        {
            std::unique_ptr<JrcpMessage> msg(JrcpMessageFactory::CreateJrcpReqMessage(input_message));
            if (msg == nullptr)
            {
                return JRCPLIB_ERR_MALFORMED_MESSAGE;
            }

            /* Special case for Terminal Info in V1 format*/
            if (true == is_v1_terminfo) {
                dynamic_cast<jrcplib::JrcpTerminalInfoReqMessage *>(msg.get())->SetJrcpV1Format(true);
            }

            /* Get a response and the error code from the feature handler */
            /* The response message is directly written to the out parameter. */
            return fh->second(this, msg.get(), output_response);

        }

        return JRCPLIB_ERR_NO_HANDLER_REGISTERED;
    }


    /** Public function to frame the jrcp raw message specifically used to convert any JRCP v1 format message to V2 format
    *
    * @param input_message The input message buffer
    *
    * @return A boolean value
    *
    * @retval true if the JRCP message is in V1 format, false if JRCP message is in V2 format
    */
    inline bool JrcpController::ConvertTerminalInfoMsgToV2Format(BytesVec &msgvec)
    {
        if (msgvec[0] == kJrcpTerminalInformationMty) {
            /* We will convert v1 message to V2 format*/
            /* add SOF */
            msgvec.insert(msgvec.begin(), kJrcpMessageSofValue);
            /* add HDL */
            msgvec.insert(msgvec.begin() + kJrcpMessageHdlIndex, 0x00);
            /* increase LN length to 4 bytes*/
            msgvec.insert(msgvec.begin() + kJrcpMessageHdlIndex + 1, 0x00);
            msgvec.insert(msgvec.begin() + kJrcpMessageHdlIndex + 1, 0x00);
            /* add TIL as 0x00 */
            msgvec.push_back(0x00);
            return true;
        }
        return false;
    }


    /** Public function to process the raw jrcp message
    *
    * @param input_message_length The length of the input message
    * @param input_message The input message buffer pointer
    * @param socketID The socket ID of the received message request
    * @param output_response The pointer to the jrcp response object
    *
    * @return A JRCP error code
    *
    * @retval JRCPLIB_ERR_INVALID_POINTER_ARGUMENT if the any null pointer is encountered
    * @retval JRCPLIB_ERR_NO_HANDLER_REGISTERED iff the requested handler is not registered
    * @retval JRCPLIB_ERR_OK iff request object is created successfully
    */
    int32_t JrcpController::ProcessMessage(uint32_t buflen, uint8_t* message_buffer, SOCKET socketID, jrcplib_jrcp_msg_t **output_response)
    {
        BytesVec msgvec;

        /* Set to nullptr up front, will get set to right value later.*/
        SetOutParameterValue(output_response, nullptr);

        if (nullptr == message_buffer)
        {
            return JRCPLIB_ERR_INVALID_POINTER_ARGUMENT;
        }
        msgvec.assign(message_buffer, message_buffer + buflen);

        /* In case of Terminal Info message it is coming in format of V1
         * as part of protocol version negotiation hence NAD value will be in different index */
        uint8_t nad = (msgvec[0] == kJrcpTerminalInformationMty) ? msgvec[1] : ParseNad(msgvec);

        /* Get the message to process, which will also return the type of the JRCP message format */
        auto device = (device_registry_instance_)[nad];
        if (nullptr != device)
        {
            /* Update the socket Id */
            int32_t error = device->CheckAndUpdateSocketID(socketID);

            if (JRCPLIB_ERR_OK == error)
            {
                return ProcessMessage(msgvec, output_response);
            }
            /*LastError regarding SocketID is already set in function CheckAndUpdateSocketID, so no need to set again..!!*/
            return error;
        }
        return JRCPLIB_ERR_NO_DEVICE_REGISTERED;
    }

    /* Getter function for the device_registry_instance_ */
    DeviceRegistry &JrcpController::GetDeviceRegistry()
    {
        return device_registry_instance_;
    }

    /* Getter function for the constant device_registry_instance_ */
    const DeviceRegistry &JrcpController::GetDeviceRegistry() const
    {
        return device_registry_instance_;
    }
}

JRCPLIB_EXPORT_API int32_t jrcplib_init(
    jrcplib_protocol_supported_t protocol_version,
    size_t identifier_length,
    const char* controller_identifier,
    jrcplib_controller_t **out
    )
{
    return jrcplib::Init(protocol_version, identifier_length, controller_identifier, out);
}

JRCPLIB_EXPORT_API void jrcplib_cleanup(jrcplib_controller_t *thisinst)
{
    /* Simple, easy, does not fail if thisinst is NULL. */
    delete thisinst;
}


JRCPLIB_EXPORT_API int32_t jrcplib_controller_process_message(
    jrcplib_controller_t *thisinst,
    uint32_t buflen,
    uint8_t *message_buffer,
    jrcplib_jrcp_msg_t **out
    )
{
    jrcplib::BytesVec msgvec;
    if (thisinst == nullptr)
    {
        /* Check the controller instance */
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }
    msgvec.assign(message_buffer, message_buffer + buflen);


    // ProcessMessage is no longer in the MessageProcessor class, because
    // this class does no longer exist. It has been moved to the controller class.
    // This function has been kept here to make it easy to find from its name.
    return thisinst->ProcessMessage(msgvec, out);
}

JRCPLIB_EXPORT_API int32_t jrcplib_controller_process_request_message(
    jrcplib_controller_t *thisinst,
    uint32_t buflen,
    uint8_t *message_buffer,
    SOCKET socket_id,
    jrcplib_jrcp_msg_t **out
    )
{
    int32_t retval = JRCPLIB_ERR_OK;
    if (thisinst == nullptr)
    {
        /* Check the controller instance */
        return JRCPLIB_ERR_INVALID_LIBRARY_INSTANCE;
    }

    // ProcessMessage is no longer in the MessageProcessor class, because
    // this class does no longer exist. It has been moved to the controller class.
    // This function has been kept here to make it easy to find from its name.
    retval = thisinst->ProcessMessage(buflen, message_buffer, socket_id, out);
    if (retval < JRCPLIB_ERR_OK)
    {
        /*Below Function will frame the generic Response from error code received.*/
        return thisinst->ProcessErrorCode(message_buffer, out, retval);
    }
    else
    {
        return retval;
    }
}

