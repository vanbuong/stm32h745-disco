/*
 * zb_zcl.h
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_ZCL_H_
#define ZB_ZCL_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "af/zb_af.h"
#include "common/zb_common.h"

/*********************************************************************
 * CONSTANTS
 */
// Zigbee Manufacturer Specific Profile Identification
#define ZCL_MANU_ID_NONE                                    0x0000
#define ZCL_MANU_ID_ANY                                     0xFFFF
#define ZCL_MANU_ID_AQARA                                   0x1037
#define ZCL_MANU_ID_LUMI_UNITED_TECH                        0x115F
#define ZCL_MANU_ID_EMBER                                   0x1002
#define ZLC_MANU_ID_SILICON_LABS                            0x1049
#define ZCL_MANU_ID_PHILIPS                                 0x100B


// Zigbee Home Automation Profile Identification
#define ZCL_HA_PROFILE_ID                                   0x0104
// Zigbee Light Link Profile Identification
#define ZCL_LL_PROFILE_ID                                   0xC05E
// Zigbee Smart Energy Profile Identification
#define ZCL_SE_PROFILE_ID                                   0x0109
// Zigbee Green Power Profile Identification
#define ZCL_GP_PROFILE_ID                                   0xA1E0

// Zigbee Home Automation Device Identification
// Generic Device
#define ZCL_DEVICEID_GENERIC                                0xFFFE

// Generic Device IDs
#define ZCL_DEVICEID_ON_OFF_SWITCH                          0x0000
#define ZCL_DEVICEID_LEVEL_CONTROL_SWITCH                   0x0001
#define ZCL_DEVICEID_ON_OFF_OUTPUT                          0x0002
#define ZCL_DEVICEID_LEVEL_CONTROLLABLE_OUTPUT              0x0003
#define ZCL_DEVICEID_SCENE_SELECTOR                         0x0004
#define ZCL_DEVICEID_CONFIGURATION_TOOL                     0x0005
#define ZCL_DEVICEID_REMOTE_CONTROL                         0x0006
#define ZCL_DEVICEID_COMBINED_INTERFACE                     0x0007
#define ZCL_DEVICEID_RANGE_EXTENDER                         0x0008
#define ZCL_DEVICEID_MAINS_POWER_OUTLET                     0x0009
#define ZCL_DEVICEID_DOOR_LOCK                              0x000A
#define ZCL_DEVICEID_DOOR_LOCK_CONTROLLER                   0x000B
#define ZCL_DEVICEID_SIMPLE_SENSOR                          0x000C
#define ZCL_DEVICEID_CONSUMPTION_AWARENESS                  0x000D
#define ZCL_DEVICEID_HOME_GATEWAY                           0x0050
#define ZCL_DEVICEID_SMART_PLUG                             0x0051
#define ZCL_DEVICEID_WHITE_GOODS                            0x0052
#define ZCL_DEVICEID_METER_INTERFACE                        0x0053

#define ZCL_DEVICEID_TEST_DEVICE                            0x00FF

// Lighting Device IDs
#define ZCL_DEVICEID_ON_OFF_LIGHT                           0x0100
#define ZCL_DEVICEID_DIMMABLE_LIGHT                         0x0101
#define ZCL_DEVICEID_COLOR_DIMMABLE_LIGHT                   0x0102
#define ZCL_DEVICEID_ON_OFF_LIGHT_SWITCH                    0x0103
#define ZCL_DEVICEID_DIMMER_SWITCH                          0x0104
#define ZCL_DEVICEID_COLOR_DIMMER_SWITCH                    0x0105
#define ZCL_DEVICEID_LIGHT_SENSOR                           0x0106
#define ZCL_DEVICEID_OCCUPANCY_SENSOR                       0x0107

// Basic Lighting Device IDs
#define ZCL_DEVICEID_ON_OFF_BALLAST                         0x0108
#define ZCL_DEVICEID_DIMMABLE_BALLAST                       0x0109
#define ZCL_DEVICEID_ON_OFF_PLUG_IN_UNIT                    0x010A
#define ZCL_DEVICEID_DIMMABLE_PLUG_IN_UNIT                  0x010B
#define ZCL_DEVICEID_COLOR_TEMPERATURE_LIGHT                0x010C
#define ZCL_DEVICEID_EXTENDED_COLOR_LIGHT                   0x010D
#define ZCL_DEVICEID_LIGHT_LEVEL_SENSOR                     0x010E

// Lighting Controllers Device IDs
#define ZCL_DEVICEID_COLOR_CONTROLLER                       0x0800
#define ZCL_DEVICEID_COLOR_SCENE_CONTROLLER                 0x0810
#define ZCL_DEVICEID_NON_COLOR_CONTROLLER                   0x0820
#define ZCL_DEVICEID_NON_COLOR_SCENE_CONTROLLER             0x0830
#define ZCL_DEVICEID_CONTROL_BRIDGE                         0x0840
#define ZCL_DEVICEID_ON_OFF_SENSOR                          0x0850

// Closures Device IDs
#define ZCL_DEVICEID_SHADE                                  0x0200
#define ZCL_DEVICEID_SHADE_CONTROLLER                       0x0201
#define ZCL_DEVICEID_WINDOW_COVERING                        0x0202
#define ZCL_DEVICEID_WINDOW_COVERING_CONTROLLER             0x0203

// HVAC Device IDs
#define ZCL_DEVICEID_HEATING_COOLING_UNIT                   0x0300
#define ZCL_DEVICEID_THERMOSTAT                             0x0301
#define ZCL_DEVICEID_TEMPERATURE_SENSOR                     0x0302
#define ZCL_DEVICEID_PUMP                                   0x0303
#define ZCL_DEVICEID_PUMP_CONTROLLER                        0x0304
#define ZCL_DEVICEID_PRESSURE_SENSOR                        0x0305
#define ZCL_DEVICEID_FLOW_SENSOR                            0x0306
#define ZCL_DEVICEID_MINI_SPLIT_AC                          0x0307

// Intruder Alarm Systems (IAS) Device IDs
#define ZCL_DEVICEID_IAS_CIE                                0x0400
#define ZCL_DEVICEID_IAS_ACE                                0x0401
#define ZCL_DEVICEID_IAS_ZONE                               0x0402
#define ZCL_DEVICEID_IAS_WARNING                            0x0403

// Manufacturer Specific Device IDs
#define ZCL_DEVICEID_MANU_AQARA_LIGHT                       0x010C
#define ZCL_DEVICEID_MANU_IKEA_LIGHT                        0x010C
#define ZCL_DEVICEID_MANU_PHILIPS_LIGHT                     0x010D
#define ZCL_DEVICEID_MANU_AQARA_DOOR_SENSOR                 0x5F01

// Zigbee Light Link Device IDs
#define ZCL_DEVICEID_ZLL_ON_OFF_LIGHT                       0x0000
#define ZCL_DEVICEID_ZLL_ON_OFF_PLUGIN_UNIT                 0x0010
#define ZCL_DEVICEID_ZLL_DIMMABLE_LIGHT                     0x0100
#define ZCL_DEVICEID_ZLL_DIMMABLE_PLUGIN_UNIT               0x0110
#define ZCL_DEVICEID_ZLL_COLOR_LIGHT                        0x0200
#define ZCL_DEVICEID_ZLL_EXTENDED_COLOR_LIGHT               0x0210
#define ZCL_DEVICEID_ZLL_COLOR_TEMPERATURE_LIGHT            0x0220

// Green Power Proxy Device IDs
#define ZCL_DEVICEID_GP_PROXY_BASIC                         0x0061

// SE Device IDs
#define ZCL_DEVICEID_SE_RANGE_EXTENDER                      0x0008
#define ZCL_DEVICEID_SE_ESI                                 0x0500
#define ZCL_DEVICEID_SE_METERING                            0x0501
#define ZCL_DEVICEID_SE_IHD                                 0x0502
#define ZCL_DEVICEID_SE_PCT                                 0x0503
#define ZCL_DEVICEID_SE_LOAD_CTRL                           0x0504
#define ZCL_DEVICEID_SE_SMART_APPLIANCE                     0x0505
#define ZCL_DEVICEID_SE_PREPAYMENT_TERMINAL                 0x0506
#define ZCL_DEVICEID_SE_PHYSICAL                            0x0507

// Invalid Device IDs
#define ZCL_DEVICEID_NONE                                   0xFFFF

// General Clusters
#define ZCL_CLUSTER_ID_GENERAL_BASIC                        0x0000
#define ZCL_CLUSTER_ID_GENERAL_POWER_CONFIG                 0x0001
#define ZCL_CLUSTER_ID_GENERAL_DEVICE_TEMP_CONFIG           0x0002
#define ZCL_CLUSTER_ID_GENERAL_IDENTIFY                     0x0003
#define ZCL_CLUSTER_ID_GENERAL_GROUPS                       0x0004
#define ZCL_CLUSTER_ID_GENERAL_SCENES                       0x0005
#define ZCL_CLUSTER_ID_GENERAL_ON_OFF                       0x0006
#define ZCL_CLUSTER_ID_GENERAL_ON_OFF_SWITCH_CONFIG         0x0007
#define ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL                0x0008
#define ZCL_CLUSTER_ID_GENERAL_ALARMS                       0x0009
#define ZCL_CLUSTER_ID_GENERAL_TIME                         0x000A
#define ZCL_CLUSTER_ID_GENERAL_LOCATION                     0x000B
#define ZCL_CLUSTER_ID_GENERAL_ANALOG_INPUT_BASIC           0x000C
#define ZCL_CLUSTER_ID_GENERAL_ANALOG_OUTPUT_BASIC          0x000D
#define ZCL_CLUSTER_ID_GENERAL_ANALOG_VALUE_BASIC           0x000E
#define ZCL_CLUSTER_ID_GENERAL_BINARY_INPUT_BASIC           0x000F
#define ZCL_CLUSTER_ID_GENERAL_BINARY_OUTPUT_BASIC          0x0010
#define ZCL_CLUSTER_ID_GENERAL_BINARY_VALUE_BASIC           0x0011
#define ZCL_CLUSTER_ID_GENERAL_MULTISTATE_INPUT_BASIC       0x0012
#define ZCL_CLUSTER_ID_GENERAL_MULTISTATE_OUTPUT_BASIC      0x0013
#define ZCL_CLUSTER_ID_GENERAL_MULTISTATE_VALUE_BASIC       0x0014
#define ZCL_CLUSTER_ID_GENERAL_COMMISSIONING                0x0015
#define ZCL_CLUSTER_ID_GENERAL_PARTITION                    0x0016
#define ZCL_CLUSTER_ID_OTA                                  0x0019
#define ZCL_CLUSTER_ID_GENERAL_POWER_PROFILE                0x001A
#define ZCL_CLUSTER_ID_GENERAL_APPLIANCE_CONTROL            0x001B
#define ZCL_CLUSTER_ID_GENERAL_POLL_CONTROL                 0x0020
#define ZCL_CLUSTER_ID_GREEN_POWER_PROXY                    0x0021
#define ZCL_CLUSTER_ID_MOBILE_DEVICE_CONFIGURATION          0x0022
#define ZCL_CLUSTER_ID_NEIGHBOR_CLEANING                    0x0023
#define ZCL_CLUSTER_ID_NEAREST_GATEWAY                      0x0024

// Closures Clusters
#define ZCL_CLUSTER_ID_CLOSURES_SHADE_CONFIG                0x0100
#define ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK                   0x0101
#define ZCL_CLUSTER_ID_CLOSURES_WINDOW_COVERING             0x0102

// HVAC Clusters
#define ZCL_CLUSTER_ID_HVAC_PUMP_CONFIG_CONTROL             0x0200
#define ZCL_CLUSTER_ID_HVAC_THERMOSTAT                      0x0201
#define ZCL_CLUSTER_ID_HVAC_FAN_CONTROL                     0x0202
#define ZCL_CLUSTER_ID_HVAC_DIHUMIDIFICATION_CONTROL        0x0203
#define ZCL_CLUSTER_ID_HVAC_USER_INTERFACE_CONFIG           0x0204

// Lighting Clusters
#define ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL               0x0300
#define ZCL_CLUSTER_ID_LIGHTING_BALLAST_CONFIG              0x0301

// Measurement and Sensing Clusters
#define ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT           0x0400
#define ZCL_CLUSTER_ID_MS_ILLUMINANCE_LEVEL_SENSING_CONFIG  0x0401
#define ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT           0x0402
#define ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT              0x0403
#define ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT                  0x0404
#define ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY                 0x0405
#define ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING                 0x0406
#define ZCL_CLUSTER_ID_MS_CO2_MEASUREMENT                   0x040D
#define ZCL_CLUSTER_ID_MS_PM25_MEASUREMENT                  0x042A
#define ZCL_CLUSTER_ID_MS_FORMALDEHYDE_MEASUREMENT          0x042B
#define ZCL_CLUSTER_ID_MS_PM10_MEASUREMENT                  0x042C
#define ZCL_CLUSTER_ID_MS_PM1_MEASUREMENT                   0x042D
#define ZCL_CLUSTER_ID_MS_TVOC_MEASUREMENT                  0x042F
#define ZCL_CLUSTER_ID_MS_IAQ_MEASUREMENT                   0x0430
#define ZCL_CLUSTER_ID_MS_ECO2_MEASUREMENT                  0x0431
#define ZCL_CLUSTER_ID_MS_ELECTRICAL_MEASUREMENT            0x0B04

// Manufacturer-specific cluster and attribute ids are NOT defined here.
// Each vendor owns its own identifiers under zigbee_driver/manu/<vendor>/ -
// see manu/zb_manu.h, which pulls them all in. Cluster ids above 0xFC00 mean
// different things on different vendors' devices, so they only make sense
// paired with a manufacturer code.

// Security and Safety (SS) Clusters
#define ZCL_CLUSTER_ID_SS_IAS_ZONE                          0x0500
#define ZCL_CLUSTER_ID_SS_IAS_ACE                           0x0501
#define ZCL_CLUSTER_ID_SS_IAS_WD                            0x0502

// Protocol Interfaces
#define ZCL_CLUSTER_ID_PI_GENERIC_TUNNEL                    0x0600
#define ZCL_CLUSTER_ID_PI_BACNET_PROTOCOL_TUNNEL            0x0601
#define ZCL_CLUSTER_ID_PI_ANALOG_INPUT_BACNET_REG           0x0602
#define ZCL_CLUSTER_ID_PI_ANALOG_INPUT_BACNET_EXT           0x0603
#define ZCL_CLUSTER_ID_PI_ANALOG_OUTPUT_BACNET_REG          0x0604
#define ZCL_CLUSTER_ID_PI_ANALOG_OUTPUT_BACNET_EXT          0x0605
#define ZCL_CLUSTER_ID_PI_ANALOG_VALUE_BACNET_REG           0x0606
#define ZCL_CLUSTER_ID_PI_ANALOG_VALUE_BACNET_EXT           0x0607
#define ZCL_CLUSTER_ID_PI_BINARY_INPUT_BACNET_REG           0x0608
#define ZCL_CLUSTER_ID_PI_BINARY_INPUT_BACNET_EXT           0x0609
#define ZCL_CLUSTER_ID_PI_BINARY_OUTPUT_BACNET_REG          0x060A
#define ZCL_CLUSTER_ID_PI_BINARY_OUTPUT_BACNET_EXT          0x060B
#define ZCL_CLUSTER_ID_PI_BINARY_VALUE_BACNET_REG           0x060C
#define ZCL_CLUSTER_ID_PI_BINARY_VALUE_BACNET_EXT           0x060D
#define ZCL_CLUSTER_ID_PI_MULTISTATE_INPUT_BACNET_REG       0x060E
#define ZCL_CLUSTER_ID_PI_MULTISTATE_INPUT_BACNET_EXT       0x060F
#define ZCL_CLUSTER_ID_PI_MULTISTATE_OUTPUT_BACNET_REG      0x0610
#define ZCL_CLUSTER_ID_PI_MULTISTATE_OUTPUT_BACNET_EXT      0x0611
#define ZCL_CLUSTER_ID_PI_MULTISTATE_VALUE_BACNET_REG       0x0612
#define ZCL_CLUSTER_ID_PI_MULTISTATE_VALUE_BACNET_EXT       0x0613
#define ZCL_CLUSTER_ID_PI_11073_PROTOCOL_TUNNEL             0x0614
#define ZCL_CLUSTER_ID_PI_ISO7818_PROTOCOL_TUNNEL           0x0615
#define ZCL_CLUSTER_ID_PI_RETAIL_TUNNEL                     0x0617

// Advanced Metering Initiative (SE) Clusters
#define ZCL_CLUSTER_ID_SE_PRICE                             0x0700
#define ZCL_CLUSTER_ID_SE_LOAD_CONTROL                      0x0701
#define ZCL_CLUSTER_ID_SE_METERING                          0x0702
#define ZCL_CLUSTER_ID_SE_MESSAGE                           0x0703
#define ZCL_CLUSTER_ID_SE_SE_TUNNELING                      0x0704
#define ZCL_CLUSTER_ID_SE_PREPAYMENT                        0x0705
#define ZCL_CLUSTER_ID_SE_ENERGY_MGMT                       0x0706
#define ZCL_CLUSTER_ID_SE_TOU_CALENDAR                      0x0707
#define ZCL_CLUSTER_ID_SE_DEVICE_MGMT                       0x0708
#define ZCL_CLUSTER_ID_SE_EVENTS                            0x0709
#define ZCL_CLUSTER_ID_SE_MDU_PAIRING                       0x070A

#define ZCL_CLUSTER_ID_GENERAL_KEY_ESTABLISHMENT            0x0800

#define ZCL_CLUSTER_ID_TELECOMMUNICATIONS_INFOMATION        0x0900
#define ZCL_CLUSTER_ID_TELECOMMUNICATIONS_VOICE_OVER_ZIGBEE 0x0904
#define ZCL_CLUSTER_ID_TELECOMMUNICATIONS_CHATTING          0x0905

#define ZCL_CLUSTER_ID_HA_APPLIANCE_IDENTIFICATION          0x0B00
#define ZCL_CLUSTER_ID_HA_METER_IDENTIFICATION              0x0B01
#define ZCL_CLUSTER_ID_HA_APPLIANCE_EVENTS_ALERTS           0x0B02
#define ZCL_CLUSTER_ID_HA_APPLIANCE_STATISTICS              0x0B03
#define ZCL_CLUSTER_ID_HA_DIAGNOSTIC                        0x0B05

// Light Link Clusters
#define ZCL_CLUSTER_ID_TOUCHLINK                            0x1000

#define ZCL_CLUSTER_ID_NONE                                 0xFFFF

/*** Frame Control bit mask ***/
#define ZCL_FRAME_CONTROL_TYPE                          0x03
#define ZCL_FRAME_CONTROL_MANU_SPECIFIC                 0x04
#define ZCL_FRAME_CONTROL_DIRECTION                     0x08
#define ZCL_FRAME_CONTROL_DISABLE_DEFAULT_RSP           0x10

/*** Frame Types ***/
#define ZCL_FRAME_TYPE_PROFILE_CMD                      0x00
#define ZCL_FRAME_TYPE_SPECIFIC_CMD                     0x01

/*** Frame Client/Server Directions ***/
#define ZCL_FRAME_CLIENT_SERVER_DIR                     0x00
#define ZCL_FRAME_SERVER_CLIENT_DIR                     0x01

/*** Frame Header Size ***/
#define ZCL_FRAME_HEADER_FRAME_CTRL_SIZE                1
#define ZCL_FRAME_HEADER_MANUF_CODE_SIZE                2
#define ZCL_FRAME_HEADER_TRANS_SEQ_NUM_SIZE             1
#define ZCL_FRAME_HEADER_COMMAND_ID_SIZE                1

/*** Foundation Command IDs ***/
#define ZCL_CMD_READ                                    0x00
#define ZCL_CMD_READ_RSP                                0x01
#define ZCL_CMD_WRITE                                   0x02
#define ZCL_CMD_WRITE_UNDIVIDED                         0x03
#define ZCL_CMD_WRITE_RSP                               0x04
#define ZCL_CMD_WRITE_NO_RSP                            0x05
#define ZCL_CMD_CONFIG_REPORT                           0x06
#define ZCL_CMD_CONFIG_REPORT_RSP                       0x07
#define ZCL_CMD_READ_REPORT_CFG                         0x08
#define ZCL_CMD_READ_REPORT_CFG_RSP                     0x09
#define ZCL_CMD_REPORT                                  0x0a
#define ZCL_CMD_DEFAULT_RSP                             0x0b
#define ZCL_CMD_DISCOVER_ATTRS                          0x0c
#define ZCL_CMD_DISCOVER_ATTRS_RSP                      0x0d
#define ZCL_CMD_DISCOVER_CMDS_RECEIVED                  0x11
#define ZCL_CMD_DISCOVER_CMDS_RECEIVED_RSP              0x12
#define ZCL_CMD_DISCOVER_CMDS_GEN                       0x13
#define ZCL_CMD_DISCOVER_CMDS_GEN_RSP                   0x14
#define ZCL_CMD_DISCOVER_ATTRS_EXT                      0x15
#define ZCL_CMD_DISCOVER_ATTRS_EXT_RSP                  0x16

#define ZCL_CMD_MAX           ZCL_CMD_DISCOVER_ATTRS_EXT_RSP

/*** Data Types ***/
#define ZCL_DATATYPE_NO_DATA                            0x00
#define ZCL_DATATYPE_DATA8                              0x08
#define ZCL_DATATYPE_DATA16                             0x09
#define ZCL_DATATYPE_DATA24                             0x0a
#define ZCL_DATATYPE_DATA32                             0x0b
#define ZCL_DATATYPE_DATA40                             0x0c
#define ZCL_DATATYPE_DATA48                             0x0d
#define ZCL_DATATYPE_DATA56                             0x0e
#define ZCL_DATATYPE_DATA64                             0x0f
#define ZCL_DATATYPE_BOOLEAN                            0x10
#define ZCL_DATATYPE_BITMAP8                            0x18
#define ZCL_DATATYPE_BITMAP16                           0x19
#define ZCL_DATATYPE_BITMAP24                           0x1a
#define ZCL_DATATYPE_BITMAP32                           0x1b
#define ZCL_DATATYPE_BITMAP40                           0x1c
#define ZCL_DATATYPE_BITMAP48                           0x1d
#define ZCL_DATATYPE_BITMAP56                           0x1e
#define ZCL_DATATYPE_BITMAP64                           0x1f
#define ZCL_DATATYPE_UINT8                              0x20
#define ZCL_DATATYPE_UINT16                             0x21
#define ZCL_DATATYPE_UINT24                             0x22
#define ZCL_DATATYPE_UINT32                             0x23
#define ZCL_DATATYPE_UINT40                             0x24
#define ZCL_DATATYPE_UINT48                             0x25
#define ZCL_DATATYPE_UINT56                             0x26
#define ZCL_DATATYPE_UINT64                             0x27
#define ZCL_DATATYPE_INT8                               0x28
#define ZCL_DATATYPE_INT16                              0x29
#define ZCL_DATATYPE_INT24                              0x2a
#define ZCL_DATATYPE_INT32                              0x2b
#define ZCL_DATATYPE_INT40                              0x2c
#define ZCL_DATATYPE_INT48                              0x2d
#define ZCL_DATATYPE_INT56                              0x2e
#define ZCL_DATATYPE_INT64                              0x2f
#define ZCL_DATATYPE_ENUM8                              0x30
#define ZCL_DATATYPE_ENUM16                             0x31
#define ZCL_DATATYPE_SEMI_PREC                          0x38
#define ZCL_DATATYPE_SINGLE_PREC                        0x39
#define ZCL_DATATYPE_DOUBLE_PREC                        0x3a
#define ZCL_DATATYPE_OCTET_STR                          0x41
#define ZCL_DATATYPE_CHAR_STR                           0x42
#define ZCL_DATATYPE_LONG_OCTET_STR                     0x43
#define ZCL_DATATYPE_LONG_CHAR_STR                      0x44
#define ZCL_DATATYPE_ARRAY                              0x48
#define ZCL_DATATYPE_STRUCT                             0x4c
#define ZCL_DATATYPE_SET                                0x50
#define ZCL_DATATYPE_BAG                                0x51
#define ZCL_DATATYPE_TOD                                0xe0
#define ZCL_DATATYPE_DATE                               0xe1
#define ZCL_DATATYPE_UTC                                0xe2
#define ZCL_DATATYPE_CLUSTER_ID                         0xe8
#define ZCL_DATATYPE_ATTR_ID                            0xe9
#define ZCL_DATATYPE_BAC_OID                            0xea
#define ZCL_DATATYPE_IEEE_ADDR                          0xf0
#define ZCL_DATATYPE_128_BIT_SEC_KEY                    0xf1
#define ZCL_DATATYPE_UNKNOWN                            0xff

/*** Error Status Codes ***/
#define ZCL_STATUS_SUCCESS                              0x00
#define ZCL_STATUS_FAILURE                              0x01
// 0x02-0x7D are reserved.
#define ZCL_STATUS_NOT_AUTHORIZED                       0x7E
#define ZCL_STATUS_MALFORMED_COMMAND                    0x80
#define ZCL_STATUS_UNSUP_CLUSTER_COMMAND                0x81
#define ZCL_STATUS_UNSUP_GENERAL_COMMAND                0x82
#define ZCL_STATUS_UNSUP_MANU_CLUSTER_COMMAND           0x83
#define ZCL_STATUS_UNSUP_MANU_GENERAL_COMMAND           0x84
#define ZCL_STATUS_INVALID_FIELD                        0x85
#define ZCL_STATUS_UNSUPPORTED_ATTRIBUTE                0x86
#define ZCL_STATUS_INVALID_VALUE                        0x87
#define ZCL_STATUS_READ_ONLY                            0x88
#define ZCL_STATUS_INSUFFICIENT_SPACE                   0x89
#define ZCL_STATUS_DUPLICATE_EXISTS                     0x8a
#define ZCL_STATUS_NOT_FOUND                            0x8b
#define ZCL_STATUS_UNREPORTABLE_ATTRIBUTE               0x8c
#define ZCL_STATUS_INVALID_DATA_TYPE                    0x8d
#define ZCL_STATUS_INVALID_SELECTOR                     0x8e
#define ZCL_STATUS_WRITE_ONLY                           0x8f
#define ZCL_STATUS_INCONSISTENT_STARTUP_STATE           0x90
#define ZCL_STATUS_DEFINED_OUT_OF_BAND                  0x91
#define ZCL_STATUS_INCONSISTENT                         0x92
#define ZCL_STATUS_ACTION_DENIED                        0x93
#define ZCL_STATUS_TIMEOUT                              0x94
#define ZCL_STATUS_ABORT                                0x95
#define ZCL_STATUS_INVALID_IMAGE                        0x96
#define ZCL_STATUS_WAIT_FOR_DATA                        0x97
#define ZCL_STATUS_NO_IMAGE_AVAILABLE                   0x98
#define ZCL_STATUS_REQUIRE_MORE_IMAGE                   0x99

// 0xbd-bf are reserved.
#define ZCL_STATUS_HARDWARE_FAILURE                     0xc0
#define ZCL_STATUS_SOFTWARE_FAILURE                     0xc1
#define ZCL_STATUS_CALIBRATION_ERROR                    0xc2
// 0xc3-0xff are reserved.
#define ZCL_STATUS_CMD_HAS_RSP                          0xff // Non-standard status (used for Default response)

/** Attribute Access Control - bit masks */
#define ACCESS_CONTROL_NONE                             0x00 // attribute can be neither read nor written
#define ACCESS_CONTROL_READ                             0x01 // attribute can be read
#define ACCESS_CONTROL_WRITE                            0x02 // attribute can be written
#define ACCESS_REPORTABLE                               0x04 // indicate attribute is reportable
#define ACCESS_CONTROL_COMMAND                          0x08
#define ACCESS_CONTROL_AUTH_READ                        0x10
#define ACCESS_CONTROL_AUTH_WRITE                       0x20
#define ACCESS_GLOBAL                                   0x40 // TI unique to indicate attributes that arr in both, client and server side of the cluster in the endpoint
#define ACCESS_CLIENT                                   0x80 // TI unique, indicate client side attrib
// NOTE: If no global or client access is defined, then server side of the attribute is assumed
// Access Control for client
#define ACCESS_CONTROL_MASK                             0x07
// Access Control as reported OTA via DiscoveryAttributesExtended
#define ACCESS_CONTROLEXT_MASK                          0x07 // read/write/reportable bits same as above

// Used by Configure Reporting Command
#define ZCL_SEND_ATTR_REPORTS                           0x00
#define ZCL_EXPECT_ATTR_REPORTS                         0x01

// Used by ZCL Read/Write callback functions
#define ZCL_OPER_LEN                                    0x00 // Get length of attribute value to be read
#define ZCL_OPER_READ                                   0x01 // Read attribute value
#define ZCL_OPER_WRITE                                  0x02 // Write new attribute value

#define ATTRID_CLUSTER_REVISION                         0xFFFD // The ClusterRevision global attribute is mandatory for all cluster instances, client and server, conforming to ZCL revision 6 (ZCL6) and later ZCL revisions.
#define ATTRID_ATTRIBUTE_REPORTING_STATUS               0xFFFE // The ClusterRevision global attribute is mandatory for all cluster instances, client and server, conforming to ZCL revision 6 (ZCL6) and later ZCL revisions.

/*************************************************************************************************
 * MACROS
 */
#define ZCL_PROFILE_CMD(a)              ((a) == ZCL_FRAME_TYPE_PROFILE_CMD)
#define ZCL_CLUSTER_CMD(a)              ((a) == ZCL_FRAME_TYPE_SPECIFIC_CMD)
#define ZCL_SERVER_CMD(a)               ((a) == ZCL_FRAME_CLIENT_SERVER_DIR)
#define ZCL_CLIENT_CMD(a)               ((a) == ZCL_FRAME_SERVER_CLIENT_DIR)

#define UNICAST_MSG(msg)                ((msg)->was_broadcast == false && (msg)->group_id == 0)

// Check for Cluster IDs
#define ZCL_CLUSTER_ID_GENERAL(id)      (/*(id) >= ZCL_CLUSTER_ID_GENERAL_BASIC &&*/ \
                                            (id) <= ZCL_CLUSTER_ID_GENERAL_COMMISSIONING)
#define ZCL_CLUSTER_ID_CLOSURES(id)     ((id) <= ZCL_CLUSTER_ID_CLOSURES_SHADE_CONFIG && \
                                            (id) >= ZCL_CLUSTER_ID_CLOSURES_WINDOW_COVERING)
#define ZCL_CLUSTER_ID_HVAC(id)         ((id) <= ZCL_CLUSTER_ID_HVAC_PUMP_CONFIG_CONTROL && \
                                            (id) >= ZCL_CLUSTER_ID_HVAC_USER_INTERFACE_CONFIG)
#define ZCL_CLUSTER_ID_LIGHTING(id)     ((id) <= ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL && \
                                            (id) >= ZCL_CLUSTER_ID_LIGHTING_BALLAST_CONFIG)
#define ZCL_CLUSTER_ID_MS(id)           ((id) >= ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT && \
                                            (id) <= ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING)
#define ZCL_CLUSTER_ID_SS(id)           ((id) >= ZCL_CLUSTER_ID_SS_IAS_ZONE && \
                                            (id) <= ZCL_CLUSTER_ID_SS_IAS_WD)
#define ZCL_CLUSTER_ID_KEY(id)          ((id) == ZCL_CLUSTER_ID_GENERAL_KEY_ESTABLISHMENT)
#define ZCL_CLUSTER_ID_LL(id)           ((id) == ZCL_CLUSTER_ID_TOUCHLINK)
#define ZCL_CLUSTER_ID_TL(id)           ((id) == ZCL_CLUSTER_ID_TOUCHLINK)
#define ZCL_CLUSTER_ID_PC(id)           ((id) == ZCL_CLUSTER_ID_GENERAL_POLL_CONTROL)
#define ZCL_CLUSTER_ID_EM(id)           ((id) == ZCL_CLUSTER_ID_MS_ELECTRICAL_MEASUREMENT)
#define ZCL_CLUSTER_ID_DIAG(id)         ((id) == ZCL_CLUSTER_ID_HA_DIAGNOSTIC)
#define ZCL_CLUSTER_ID_MI(id)           ((id) == ZCL_CLUSTER_ID_HA_METER_IDENTIFICATION)
#define ZCL_CLUSTER_ID_PP(id)           ((id) == ZCL_CLUSTER_ID_GENERAL_POWER_PROFILE)
#define ZCL_CLUSTER_ID_DL(id)           ((id) == ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK)


/*********************************************************************
 * TYPEDEFS
 */
// zb_zcl_process_message() return code
typedef enum e_zcl_proc_msg_status
{
    ZCL_PROC_MSG_SUCCESS,                     // Message was processed successfully
    ZCL_PROC_MSG_INVALID,                     // Format or parameter was wrong
    ZCL_PROC_MSG_EP_NOT_FOUND,                // Endpoint descriptor not found
    ZCL_PROC_MSG_NOT_OPERATIONAL,             // Can't respond to this command
    ZCL_PROC_MSG_INTERPAN_FOUNDATION_CMD,     // INTER-PAN and Foundation commands are not allowed
    ZCL_PROC_MSG_NOT_SECURE,                  // Security is required but the message is not secured
    ZCL_PROC_MSG_MANUFACTURER_SPECIFIC,       // Manufacturer specific command - not handled
    ZCL_PROC_MSG_MANUFACTURER_SPECIFIC_DR,    // Manufacturer specific command - not handled, but default response is sent
    ZCL_PROC_MSG_NOT_HANDLED,                 // No default response is sent and the message is not handled
    ZCL_PROC_MSG_NOT_HANDLED_DR,              // Default response is sent and the message is not handled
} e_zcl_proc_msg_status_t;


// ZCL header - frame control field
typedef union s_zb_zcl_frame_ctrl
{
    struct
    {
        uint8_t type : 2;                // Frame type PROFILE_CMD or SPECIFIC_CMD
        uint8_t manu_specific : 1;       // Manufacturer specific true or false
        uint8_t direction : 1;           // Direction CLIENT_SERVER_DIR or SERVER_CLIENT_DIR
        uint8_t disable_default_rsp : 1; // Disable default response true or false
        uint8_t reserved : 3;            // Reserved
    };
    uint8_t byte;
} s_zb_zcl_frame_ctrl_t;

// ZCL header
typedef struct s_zb_zcl_frame_header
{
    s_zb_zcl_frame_ctrl_t fc;     // Frame control field
    uint16_t manuf_code;                     // Manufacturer code if manu_specific is true
    uint8_t trans_seq_num;                   // Transaction sequence number
    uint8_t command_id;                      // Command ID
} s_zb_zcl_frame_header_t;


// ZCL command - read attributes
TYPEDEF_STRUCT_PACKED s_zb_zcl_read_attr_cmd
{
    uint8_t num_attr;   // Number of attributes to read
    uint16_t attr_id[]; // Supported attribute list - this structure should
                        // be allocated with the appropriate number of attributes
} s_zb_zcl_read_attr_cmd_t;

// ZCL response processing - read attributes response
TYPEDEF_STRUCT_PACKED s_zb_zcl_read_attr_rsp_info
{
    uint16_t attr_id;   // Attribute ID
    uint8_t status;     // Status should be ZCL_STATUS_SUCCESS or error
    uint8_t data_type;  // Data type
    uint8_t *data;     // This structure is allocated, so the data is HERE
                        // the size depends on the attribute data type
} s_zb_zcl_read_attr_rsp_info_t;

// ZCL response processing - read attributes response command
TYPEDEF_STRUCT_PACKED s_zb_zcl_read_attr_rsp_cmd
{
    uint8_t num_attr;                           // Number of attributes in the list
    s_zb_zcl_read_attr_rsp_info_t attr_list[]; // This structure is allocated, so the data is HERE
} s_zb_zcl_read_attr_rsp_cmd_t;

// ZCL command - read attributes info
TYPEDEF_STRUCT_PACKED s_zb_zcl_read_attr_info
{
    uint16_t attr_id;   // Attribute ID
    uint8_t status;     // Status should be ZCL_STATUS_SUCCESS or error
    uint8_t data_type;  // Data type
    uint8_t *data;      // This structure is allocated, so the data is HERE
                        // the size depends on the attribute data type
} s_zb_zcl_read_attr_info_t;

// ZCL command - write attributes info
TYPEDEF_STRUCT_PACKED s_zb_zcl_write_attr_info
{
    uint16_t attr_id;   // Attribute ID
    uint8_t data_type;  // Data type
    uint8_t *attr_data; // This structure is allocated, so the data is HERE
                        // the size depends on the attribute data type
} s_zb_zcl_write_attr_info_t;

// ZCL command - write attributes
TYPEDEF_STRUCT_PACKED s_zb_zcl_write_attr_cmd
{
    uint8_t num_attr;                                  // Number of attributes in the list
    s_zb_zcl_write_attr_info_t attr_list[]; // This structure is allocated, so the data is HERE
} s_zb_zcl_write_attr_cmd_t;

// ZCL command - write attributes response info
TYPEDEF_STRUCT_PACKED s_zb_zcl_write_attr_rsp_info
{
    uint8_t status;     // Status should be ZCL_STATUS_SUCCESS or error
    uint16_t attr_id;   // Attribute ID
} s_zb_zcl_write_attr_rsp_info_t;

// ZCL command - write attributes response
TYPEDEF_STRUCT_PACKED s_zb_zcl_write_attr_rsp_cmd
{
    uint8_t num_attr;                                      // Number of attributes in the list
    s_zb_zcl_write_attr_rsp_info_t attr_list[]; // This structure is allocated, so the data is HERE
} s_zb_zcl_write_attr_rsp_cmd_t;


// ZCL command - configure reporting record
TYPEDEF_STRUCT_PACKED s_zb_zcl_config_report_info
{
    uint8_t direction;          // to send or receive reports of the attribute
    uint16_t attr_id;           // attribute ID
    uint8_t data_type;          // attribute data type
    uint16_t min_report_int;    // minimum reporting interval
    uint16_t max_report_int;    // maximum reporting interval, 0xFFFF=off
    uint16_t timeout_period;    // timeout period
    uint8_t *reportable_change; // reportable change (only applicable to analog data type)
                                // the size depends on the attribute data type
} s_zb_zcl_config_report_info_t;

// ZCL command - configure reporting
TYPEDEF_STRUCT_PACKED s_zb_zcl_config_report_cmd
{
    uint8_t num_attr;                                     // Number of attributes in the list
    s_zb_zcl_config_report_info_t attr_list[]; // This structure is allocated, so the data is HERE
} s_zb_zcl_config_report_cmd_t;

// ZCL command - configure reporting response info
TYPEDEF_STRUCT_PACKED s_zb_zcl_config_report_rsp_info
{
    uint8_t status;     // Status should be ZCL_STATUS_SUCCESS or error
    uint8_t direction;  // whether attribute are reported or reports of attribute are received
    uint16_t attr_id;   // Attribute ID
} s_zb_zcl_config_report_rsp_info_t;

// ZCL command - configure reporting response
TYPEDEF_STRUCT_PACKED s_zb_zcl_config_report_rsp_cmd
{
    uint8_t num_attr;                                         // Number of attributes in the list
    s_zb_zcl_config_report_rsp_info_t attr_list[]; // This structure is allocated, so the data is HERE
} s_zb_zcl_config_report_rsp_cmd_t;

// ZCL command - read report configuration info
TYPEDEF_STRUCT_PACKED s_zb_zcl_read_report_cfg_info
{
    uint8_t direction;  // to send or receive reports of the attribute
    uint16_t attr_id;   // Attribute ID
} s_zb_zcl_read_report_cfg_info_t;

// ZCL command - read report configuration
TYPEDEF_STRUCT_PACKED s_zb_zcl_read_report_cfg_cmd
{
    uint8_t num_attr;                                         // Number of attributes in the list
    s_zb_zcl_read_report_cfg_info_t attr_list[];   // This structure is allocated, so the data is HERE
} s_zb_zcl_read_report_cfg_cmd_t;

// ZCL command - read report configuration response info
TYPEDEF_STRUCT_PACKED s_zb_zcl_read_report_cfg_rsp_info
{
    uint8_t status;             // Status should be ZCL_STATUS_SUCCESS or error
    uint8_t direction;          // whether attribute are reported or reports of attribute are received
    uint16_t attr_id;           // Attribute ID
    uint8_t data_type;          // attribute data type
    uint16_t min_report_int;    // minimum reporting interval
    uint16_t max_report_int;    // maximum reporting interval, 0xFFFF=off
    uint16_t timeout_period;    // timeout period
    uint8_t *reportable_change; // reportable change (only applicable to analog data type)
                                // the size depends on the attribute data type
} s_zb_zcl_read_report_cfg_rsp_info_t;

// ZCL command - read report configuration response
TYPEDEF_STRUCT_PACKED s_zb_zcl_read_report_cfg_rsp_cmd
{
    uint8_t num_attr;                                            // Number of attributes in the list
    s_zb_zcl_read_report_cfg_rsp_info_t attr_list[];  // This structure is allocated, so the data is HERE
} s_zb_zcl_read_report_cfg_rsp_cmd_t;

// ZCL command - report attribute info
TYPEDEF_STRUCT_PACKED s_zb_zcl_report_attr_info
{
    uint16_t attr_id;   // Attribute ID
    uint8_t data_type;  // attribute data type
    uint8_t *attr_data; // This structure is allocated, so the data is HERE
                        // the size depends on the attribute data type
} s_zb_zcl_report_attr_info_t;

// ZCL command - report attribute
TYPEDEF_STRUCT_PACKED s_zb_zcl_report_attr_cmd
{
    uint8_t num_attr;                                   // Number of attributes in the list
    s_zb_zcl_report_attr_info_t attr_list[]; // This structure is allocated, so the data is HERE
} s_zb_zcl_report_attr_cmd_t;

// ZCL command - default response
TYPEDEF_STRUCT_PACKED s_zb_zcl_default_rsp_cmd
{
    uint8_t command_id;     // Command ID
    uint8_t status_code;    // Status code
} s_zb_zcl_default_rsp_cmd_t;

// ZCL command - discover attributes
TYPEDEF_STRUCT_PACKED s_zb_zcl_discover_attrs_cmd
{
    uint16_t start_attr_id; // Specifies the minimum attribute ID to begin attribute discovery
    uint8_t max_attr_ids;   // Maximum number of attribute IDs that are to be returned
} s_zb_zcl_discover_attrs_cmd_t;

// ZCL command - discover attributes info
TYPEDEF_STRUCT_PACKED s_zb_zcl_discover_attrs_info
{
    uint16_t attr_id;   // Attribute ID
    uint8_t data_type;  // Attribute data type
} s_zb_zcl_discover_attrs_info_t;

// ZCL command - discover attributes response
TYPEDEF_STRUCT_PACKED s_zb_zcl_discover_attrs_cmd_rsp
{
    uint8_t disc_complete;                                 // Whether or not there're more attributes to be discovered
    uint8_t num_attr;                                      // Number of attributes in the list
    s_zb_zcl_discover_attrs_info_t attr_list[]; // This structure is allocated, so the data is HERE
} s_zb_zcl_discover_attrs_cmd_rsp_t;

// ZCL command - discover commands
TYPEDEF_STRUCT_PACKED s_zb_zcl_discover_cmds_cmd
{
    uint8_t start_cmd_id;          // Start command ID
    uint8_t max_cmd_id;            // Maximum command ID
} s_zb_zcl_discover_cmds_cmd_t;

// ZCL command - discover commands response
TYPEDEF_STRUCT_PACKED s_zb_zcl_discover_cmds_cmd_rsp
{
    uint8_t disc_complete;  // Whether or not there're more commands to be discovered
    uint8_t cmd_type;       // Command type
    uint8_t num_cmd;        // Number of commands in the list
    uint8_t *cmd_list;      // This structure is allocated, so the data is HERE
} s_zb_zcl_discover_cmds_cmd_rsp_t;

// ZCL command - discover attributes extended info
TYPEDEF_STRUCT_PACKED s_zb_zcl_discover_attrs_ext_info
{
    uint16_t attr_id;               // Attribute ID
    uint8_t attr_data_type;         // Attribute data type
    uint8_t attr_access_control;    // Attribute access control
} s_zb_zcl_discover_attrs_ext_info_t;

// ZCL command - discover attributes extended response
TYPEDEF_STRUCT_PACKED s_zb_zcl_discover_attrs_ext_rsp
{
    uint8_t disc_complete;  // Whether or not there're more attributes to be discovered
    uint8_t num_attr;       // Number of attributes in the list
    s_zb_zcl_discover_attrs_ext_info_t attr_list[];  // This structure is allocated, so the data is HERE
} s_zb_zcl_discover_attrs_ext_rsp_t;

TYPEDEF_STRUCT_PACKED s_utf8_string
{
    uint8_t str_len;
    uint8_t *pstr;
} s_utf8_string_t;

// Incoming ZCL message, this buffer will be allocated, cmd will point to the command record.
typedef struct s_zb_zcl_incoming_msg
{
    s_zb_af_incoming_msg_t *msg;
    s_zb_zcl_frame_header_t hdr;
    uint8_t *data;
    uint16_t data_len;
    void *attr_cmd;
} s_zb_zcl_incoming_msg_t;

// Function pointer type to handle incoming messages
typedef zb_status_t (*pfn_zcl_incoming_msg_handler_t)(s_zb_zcl_incoming_msg_t *msg);

// Function pointer type to handle Unhandled ZCL Foundation Commands
typedef uint8_t (*pfn_zcl_unhandled_cmd_handler_t)(s_zb_zcl_incoming_msg_t *msg);

// Attribute record
typedef struct s_zb_zcl_attribute
{
    uint16_t attr_id;
    uint8_t data_type;
    uint8_t access_control;
    uint8_t *attr_data;
} s_zb_zcl_attribute_t;

typedef struct s_zb_zcl_attribute_rec
{
    uint16_t cluster_id;
    s_zb_zcl_attribute_t attr;
} s_zb_zcl_attr_rec_t;

// Command record
typedef struct s_zb_zcl_command_rec
{
    uint16_t cluster_id;
    uint8_t command_id;
    uint8_t flag;
} s_zb_zcl_command_rec_t;

// Function pointer type to read/write attribute data.
typedef uint8_t (*pfn_zcl_read_write_callback_t)(uint16_t cluster_id, uint16_t attr_id, uint8_t operation, uint8_t *data, uint16_t *data_len);

// Callback function prototype to authorize a Read or Write operation on a given attribute.
typedef int (*pfn_zcl_authorize_callback_t)(s_zb_af_address_t *src_addr, s_zb_zcl_attr_rec_t *attr_rec, uint8_t operation);

typedef struct
{
    uint16_t cluster_id;
    uint8_t option;
} s_zb_zcl_option_rec_t;

// Parse received command
typedef struct
{
    uint8_t endpoint;
    uint16_t data_len;
    uint8_t *data;
} s_zb_zcl_parse_cmd_t;

typedef int (*pfn_zcl_manu_spec_profile_wide_cmd_callback_t)(s_zb_zcl_incoming_msg_t *msg);

// Register Callback function to handle messages externally
zb_status_t zb_zcl_register_external_cmd_handler(
    uint8_t endpoint, pfn_zcl_unhandled_cmd_handler_t pfn_unhandled_cmd_handler);

// Register Callback function to handle manufacturer specific profile wide commands
zb_status_t zb_zcl_register_manu_spec_profile_wide_cmd_handler(
    pfn_zcl_manu_spec_profile_wide_cmd_callback_t pfn_manu_spec_profile_wide_cmd_callback);

// Function for Plugin to register for incoming messages.
zb_status_t zb_zcl_register_plugin(
    uint16_t start_cluster_id, uint16_t end_cluster_id,
    pfn_zcl_incoming_msg_handler_t pfn_incoming_msg_handler);

// Register Application's command table.
zb_status_t zb_zcl_register_command_list(
    uint8_t endpoint, uint8_t cmds_list_size, const s_zb_zcl_command_rec_t cmds_list[]);

// Register Application's attribute table.
zb_status_t zb_zcl_register_attr_list(
    uint8_t endpoint, uint8_t num_attrs, const s_zb_zcl_attr_rec_t attr_list[]);

// Register Application's Cluster Option table
zb_status_t zb_zcl_register_cluster_option_list(
    uint8_t endpoint, uint8_t num_options, const s_zb_zcl_option_rec_t option_list[]);

// Get the option record that matches the cluster ID
uint8_t zb_zcl_get_cluster_option(uint8_t endpoint, uint16_t cluster_id);

// Register Application's callback function to read/write attribute data.
zb_status_t zb_zcl_register_read_write_callback(
    uint8_t endpoint,
    pfn_zcl_read_write_callback_t pfn_read_write_callback,
    pfn_zcl_authorize_callback_t pfn_authorize_callback);

// Process incoming ZCL message
e_zcl_proc_msg_status_t zb_zcl_process_message(s_zb_af_incoming_msg_t *msg);

// Function for sending a command
zb_status_t zb_zcl_send_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, uint8_t command_id, uint8_t specific, uint8_t direction,
    uint8_t disable_default_rsp, uint16_t manuf_code, uint8_t seq_num,  uint16_t cmd_len, uint8_t *cmd_data);

// Function for reading an attribute
zb_status_t zb_zcl_send_read(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_read_attr_cmd_t *read_cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint8_t seq_num);

// Function for reading a manufacturer-specific attribute (manuf_code != 0).
// With manuf_code == 0 this is identical to zb_zcl_send_read().
zb_status_t zb_zcl_send_read_manu(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_read_attr_cmd_t *read_cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint16_t manuf_code, uint8_t seq_num);

// Function for reading a local attribute
zb_status_t zb_zcl_read_attr_data(
    uint8_t endpoint, uint16_t cluster_id, uint16_t attr_id,
    uint8_t *attr_data, uint16_t *data_len);

// Function for sending a read response command
zb_status_t zb_zcl_send_read_rsp(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_read_attr_rsp_cmd_t *read_rsp,
    uint8_t direction, uint8_t disable_default_rsp, uint8_t seq_num);

// Function for writing an attribute
zb_status_t zb_zcl_send_write(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint16_t cluster_id,
    s_zb_zcl_write_attr_cmd_t *write_cmd, uint8_t cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint8_t seq_num);

// Function for writing a manufacturer-specific attribute (manuf_code != 0).
// With manuf_code == 0 this is identical to zb_zcl_send_write().
zb_status_t zb_zcl_send_write_manu(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint16_t cluster_id,
    s_zb_zcl_write_attr_cmd_t *write_cmd, uint8_t cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint16_t manuf_code, uint8_t seq_num);

// Function for configuring the reporting mechanism for one or more attributes
zb_status_t zb_zcl_send_config_report_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_config_report_cmd_t *config_report_cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint8_t seq_num);

// Configure reporting for a manufacturer-specific attribute (manuf_code != 0).
// With manuf_code == 0 this is identical to zb_zcl_send_config_report_cmd().
zb_status_t zb_zcl_send_config_report_cmd_manu(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_config_report_cmd_t *config_report_cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint16_t manuf_code, uint8_t seq_num);

// Function for reading the configuration details of the reporting mechanism
zb_status_t zb_zcl_send_read_report_cfg_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_read_report_cfg_cmd_t *read_report_cfg_cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint8_t seq_num);

// Function for sending a default response command
zb_status_t zb_zcl_send_default_rsp_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_default_rsp_cmd_t *default_rsp_cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint16_t manuf_code, uint8_t seq_num);

// Function to parse the "Profile" read commands
void *zb_zcl_parse_in_read_cmd(s_zb_zcl_parse_cmd_t *cmd);

// Function to check to see if data type is analog
uint8_t zb_zcl_is_analog_data_type(uint8_t data_type);

// Function to parse header of the ZCL format
uint8_t *zb_zcl_parse_header(s_zb_zcl_frame_header_t *header, uint8_t *data);

// Function to find the attribute record that matches the parameters
uint8_t zb_zcl_find_attr_rec(
    uint8_t endpoint, uint16_t cluster_id, uint16_t attr_id, s_zb_zcl_attr_rec_t *attr_rec);

// Function to return the length of the data type
uint8_t zb_zcl_get_data_type_length(uint8_t data_type);

// Function to return the length of the attribute data
uint16_t zb_zcl_get_attr_data_len(uint8_t data_type, uint8_t *data);

// Call to get original unprocessed AF message (not parsed by ZCL)
// NOTE: This function can only be called during a ZCL callback function
//       and the calling function must NOT change any data in the message.
s_zb_af_incoming_msg_t *zb_zcl_get_raw_af_incoming_msg(void);

// Call to get the transaction sequence number form the incoming message.
// NOTE: This function can only be called during a ZCL callback function
//       and the calling function must NOT change any data in the message.
uint8_t zb_zcl_get_parsed_trans_seq_num(void);

// Allocate the next outgoing ZCL Transaction Sequence Number for a
// locally-initiated ZCL command.  End-to-end (host <-> remote device),
// independent of the AF/APS transaction id used for AF_DATA_CONFIRM
// correlation.  A ZCL *response* must echo the request's sequence number
// instead (use zb_zcl_get_parsed_trans_seq_num()), not allocate a new one.
uint8_t zb_zcl_next_seq_num(void);

zb_status_t zb_zcl_send_read_attr_req(uint8_t src_ep, s_zb_af_address_t *dst_addr, uint16_t cluster_id, uint16_t *attr_ids, uint8_t attr_count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZCL_H_ */
