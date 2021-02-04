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
#ifndef JRCPLIB_DATA_TYPES_H_
#define JRCPLIB_DATA_TYPES_H_
/** @file data_types.h */

#include <stdint.h>

#ifndef _WIN32
#    ifndef SOCKET
#        define SOCKET int
#    endif
#    ifndef INVALID_SOCKET
#        define INVALID_SOCKET  (SOCKET)(~0)
#    endif
#    ifndef SOCKET_ERROR
#        define SOCKET_ERROR            (-1)
#    endif
#else
# include <WinSock2.h>
#endif

#ifdef __cplusplus

#include <vector>
#include <stdint.h>

namespace jrcplib {
    typedef std::vector<uint8_t> BytesVec;

    template<typename EnumType, EnumType... Values> class EnumCheck;

    template<typename EnumType> class EnumCheck<EnumType>
    {
  public:
        template<typename IntType>
                static bool const is_value(IntType) { return false; }
    };

    template<typename EnumType, EnumType V, EnumType... Next>
            class EnumCheck<EnumType, V, Next...> : private EnumCheck<EnumType, Next...>
    {
        using super = EnumCheck<EnumType, Next...>;

  public:
        template<typename IntType>
                static bool const is_value(IntType v)
        {
            return v == static_cast<IntType>(V) || super::is_value(v);
        }
    };

} // namespace jrcplib

extern "C" {
#endif

/** The type of a JRCP Event Handling request. */
typedef enum EventRequestType
{
    EventRequestUnknown = -1,
    EventRequestAnyEvent = 0x0000,
    EventRequestNonHciEvent = 0x0001,
    EventRequestOnlyHciEvent = 0x0002
} jrcplib_event_request_type_t;

/** The type of a JRCP Event Handling response. */
typedef enum EventResponseType
{
    EventResponseStatusCode = 0x0000,
    EventResponseNonHciRawEventData = 0x0001,
    EventResponseHciRawEventData = 0x0002
} jrcplib_event_response_type_t;

/** The action of a JRCP Prepare Tearing request. */
typedef enum TearingAction
{
    TearingActionUnknown = -1,
    TearingActionStatusCode = 0x0000,
    TearingActionResetNoPowerCycle = 0x0001,
    TearingActionResetPowerCycle = 0x0002,
    TearingActionDeviceSpecificStart = 0x8000,
    TearingActionDeviceSpecificEnd = 0x80FF
} jrcplib_tearing_action_t;

/** The sub-MTY of a JRCP Timing Information request/response. */
typedef enum TimingInfoType
{
    TimingUnknown = -1,
    TimingStatusCode = 0x0000,
    TimingResetTimer = 0x0001,
    TimingQueryTimer = 0x0002,
    TimingSetOptions = 0x0003,
    TimingGetCurrentOptions = 0x0004,
    TimingGetAvailableOptions = 0x0005
} jrcplib_timing_info_type_t;

/** The Gate ID of an HCI Send Data request. */
typedef enum HciGateID
{
    HciGateUnknown = -1,
    HciGateLoopback = 0x04,
    HciGateTypeBCard = 0x21,
    HciGateTypeBPrimeCard = 0x22,
    HciGateTypeACard = 0x23,
    HciGateTypeFCard = 0x24,
    HciGateTypeBReader = 0x11,
    HciGateTypeAReader = 0x13,
    HciGateConnectivity = 0x41,
    HciGateApdu = 0x30
} jrcplib_hci_gate_id_t;

/** The Frame Type (CLT/IFrame) of an HCI Send Data request. */
typedef enum FrameType
{
    FrameTypeUnknown = -1,
    FrameTypeClt = 0x01,
    FrameTypeIframe = 0x02
} jrcplib_hci_frame_type_t;

/** The status code of an HCI Send Data response. */
typedef enum HciStatusCode
{
    HciSendEventOK = 0x3200,
    HciSendEventError = 0x3201
} jrcplib_hci_status_code_t;

/** The status code in a JRCP Prepare Tearing response. */
typedef enum TearingStatusCodes
{
    TearingOK = 0x0C00,
    TearingError = 0x0C01
} jrcplib_tearing_status_code_t;

/** The status of a JRCP Timing Information response with Sub-MTY `TimingStatusCode`. */
typedef enum TimingStatusCodes
{
    TimingOK = 0x0B00,
    TimingError = 0x0B01
} jrcplib_timing_status_code_t;

/** The status of a JRCP Configuration controller actions. */
typedef enum ConfigurationType {
    ListReaders = 0x0001,
    FreezeMapping = 0x0002,
    ReleaseMapping = 0x0003,
    ConnectTerminal = 0x0004,
    CloseTerminal = 0x0005,
    UnknownConfigurationType = 0xFFFF
} jrcplib_controller_config_type_t;


/** The status of a JRCP Feature Request Type. */
typedef enum FeatureType {
    StatusCode = 0x0000,
    VersionControl = 0x0001,
    FeatureOption = 0x0002,
    ControllerInfo = 0x0003,
    ProtocolVersion = 0x0004,
    SessionId = 0x0005,
    UnknownFeatureType = 0xFFFF
} jrcplib_feature_type_t;

/** The IO Pin value of a JRCP Set IO Pin request. */
typedef enum IoPinValue
{
    PinCleared = 0x00,
    PinSet = 0x01,
} jrcplib_io_pin_value_t;

/** The JRCP Generic Status codes, as defined in section 4.23 of JRCP spec v2.8 */
typedef enum JrcpGenericStatusCodes
{
    GenericOK = 0x0000,
    GenericError = 0x0001,
    GenericTargetRemoved = 0x0002,
    GenericTimeOut = 0x00EE,
    GenericUnsupportedCommand = 0x00FF
} jrcplib_jrcp_generic_status_code_t;

/** The supported JRCP protocol versions. */
typedef enum JrcpProtocolSupported
{
    Unsupported = 0x0000,
    JrcpV2 = 0x0200,
    SimulatorSpecificVersion = 0x0301
} jrcplib_protocol_supported_t;

/**< Device connection statuses */
typedef enum DeviceConnectionStatus
{
    Connected = 1,
    Disconnected = 0
} jrcplib_device_connection_status_t;


/**< Reset type */
typedef enum ResetType
{
    ColdReset = 0,
    WarmReset = 1
} jrcplib_reset_type_t;


/* TCP Client connection statuses */
typedef enum TcpClientConnectionStatus
{
    TcpClientConnected = 1,
    TcpClientDisconnected = 0
} jrcplib_tcpclient_connection_status_t;



typedef struct {
    uint8_t  nad;
    char    *description;
} jrcplib_nad_t;


#ifdef __cplusplus
}
#endif

#endif // JRCPLIB_DATA_TYPES_H_
