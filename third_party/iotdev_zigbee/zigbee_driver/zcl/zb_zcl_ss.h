/*
 * zb_zcl_ss.h
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_ZCL_SS_H_
#define ZB_ZCL_SS_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"
#include "zcl/zb_zcl.h"

/*********************************************************************
 * CONSTANTS
 */

/**
 * @defgroup SS_IAS_ZONE_ATTRID SS IAS Zone Attributes
 * @{
 * @brief This group defines the Zone Information attributes set
 * defined in the ZCL v7 specification
 */
///State of the zone
#define ATTRID_IAS_ZONE_ZONE_STATE                                       0x0000 // M, R, ENUM8
/// The Zone Type dictates the meaning of Alarm1 and 10828 Alarm2 bits of
/// the ZoneStatus attribute.
#define ATTRID_IAS_ZONE_ZONE_TYPE                                        0x0001 // M, R, ENUM16
/// The ZoneStatus attribute is a bit map with status of each alarm.
#define ATTRID_IAS_ZONE_ZONE_STATUS                                      0x0002 // M, R, BITMAP16
/** @} End SS_IAS_ZONE_ATTRID */

/**
 * @defgroup STATE_SS_IAS_ZONE SS IAS Zone State
 * @{
 * @brief This group defines the Zone State Attribute values
 * defined in the ZCL v7 specification
 */
/// Not enrolled
#define SS_IAS_ZONE_STATE_NOT_ENROLLED                                   0x00
/// Enrolled (the client will react to Zone State Change
/// Notification commands from the server)
#define SS_IAS_ZONE_STATE_ENROLLED                                       0x01
/** @} End STATE_SS_IAS_ZONE */

/**
 * @defgroup TYPE_SS_IAS_ZONE SS IAS Zone Type
 * @{
 * @brief This group defines the Zone Type Attribute values
 * defined in the ZCL v7 specification
 * NOTE: if more Zone Type Attribute values are added,
 */
/// Zone Type      |       Alarm1       |       Alarm2       |
/// Standard CIE   |    System Alarm    |                    |
#define SS_IAS_ZONE_TYPE_STANDARD_CIE                                    0x0000
/// Zone Type      |          Alarm1          |         Alarm2          |
/// Motion sensor  |   Intrusion indication   |   Presence indication   |
#define SS_IAS_ZONE_TYPE_MOTION_SENSOR                                   0x000D
/// Zone Type      |          Alarm1           |         Alarm2          |
/// Contact switch |   1st portal Open-Close   | 2nd portal Open-Close   |
#define SS_IAS_ZONE_TYPE_CONTACT_SWITCH                                  0x0015
/// Zone Type      | Alarm1  | Alarm2  | Description                        |
/// D/W handle     |   0b0   |   0b0   | (Door/window) closed               |
///                |   0b1   |   0b0   | (Door/window) tilted (partly open) |
///                |   0b1   |   0b1   | (Door/window) open                 |
#define SS_IAS_ZONE_TYPE_DOOR_WINDOW_HANDLE                              0x0016
/// Zone Type      |      Alarm1       |      Alarm2      |
/// Fire sensor    |  Fire indication  |                  |
#define SS_IAS_ZONE_TYPE_FIRE_SENSOR                                     0x0028
/// Zone Type     |           Alarm1            |      Alarm2      |
/// Water sensor  |  Water overflow indication  |                  |
#define SS_IAS_ZONE_TYPE_WATER_SENSOR                                    0x002A
/// Zone Type                   |    Alarm1     |       Alarm2       |
/// Carbon Monoxide (CO) sensor | CO indication | Cooking indication |
#define SS_IAS_ZONE_TYPE_CO_SENSOR                                       0x002B
/// Zone Type                 |     Alarm1      |      Alarm2      |
/// Personal emergency device | Fall/Concussion | Emergency button |
#define SS_IAS_ZONE_TYPE_PERSONAL_EMERGENCY_DEVICE                       0x002C
/// Zone Type                 |       Alarm1        |    Alarm2     |
/// Vibration/Movement sensor | Movement indication |   Vibration   |
#define SS_IAS_ZONE_TYPE_VIBRATION_MOVEMENT_SENSOR                       0x002D
/// Zone Type             |    Alarm1     |    Alarm2     |
/// Remote Control        |    Panic      |   Emergency   |
#define SS_IAS_ZONE_TYPE_REMOTE_CONTROL                                  0x010F
/// Zone Type             |    Alarm1     |    Alarm2     |
/// Key fob               |    Panic      |   Emergency   |
#define SS_IAS_ZONE_TYPE_KEY_FOB                                         0x0115
/// Zone Type             |   Alarm1   |    Alarm2     |
/// Keypad                |   Panic    |   Emergency   |
#define SS_IAS_ZONE_TYPE_KEYPAD                                          0x021D
/// Zone Type               |   Alarm1   |   Alarm2    |
/// Standard Warning Device |            |             |
#define SS_IAS_ZONE_TYPE_STANDARD_WARNING_DEVICE                         0x0225
/// Zone Type           |          Alarm1           |   Alarm2    |
/// Glass break sensor  |  Glass breakage detected  |             |
#define SS_IAS_ZONE_TYPE_GLASS_BREAK_SENSOR                              0x0226
/// Zone Type         |   Alarm1   |   Alarm2    |
/// Security repeater |            |             |
#define SS_IAS_ZONE_TYPE_SECURITY_REPEATER                               0x0229
/// Zone Type         |   Alarm1   |   Alarm2    |
/// Invalid Zone Type |            |             |
#define SS_IAS_ZONE_TYPE_INVALID_ZONE_TYPE                               0xFFFF
/** @} End TYPE_SS_IAS_ZONE */

/**
 * @defgroup STATUS_SS_IAS_ZONE SS IAS Zone Status
 * @{
 * @brief This group defines the Zone Status Attribute values
 * defined in the ZCL v7 specification
 */
 /// opened or alarmed
#define SS_IAS_ZONE_STATUS_ALARM1_ALARMED                                0x0001
/// opened or alarmed
#define SS_IAS_ZONE_STATUS_ALARM2_ALARMED                                0x0002
/// Tampered
#define SS_IAS_ZONE_STATUS_TAMPERED_YES                                  0x0004
/// Low battery
#define SS_IAS_ZONE_STATUS_BATTERY_LOW                                   0x0008
/// Notify
#define SS_IAS_ZONE_STATUS_SUPERVISION_REPORTS_YES                       0x0010
/// Notify restore
#define SS_IAS_ZONE_STATUS_RESTORE_REPORTS_YES                           0x0020
/// Trouble/Failure
#define SS_IAS_ZONE_STATUS_TROUBLE_YES                                   0x0040
/// AC/Mains fault
#define SS_IAS_ZONE_STATUS_AC_MAINS_FAULT                                0x0080
/// Sensor is in test mode
#define SS_IAS_ZONE_STATUS_TEST                                          0x0100
/// Sensor detects a defective battery
#define SS_IAS_ZONE_STATUS_BATTERY_DEFECT                                0x0200


/** @} End STATUS_SS_IAS_ZONE */

/**
 * @defgroup SS_IAS_ATTRID SS IAS Attribute
 * @{
 * @brief This group defines the Zone Settings attributes set
 * defined in the ZCL v7 specification
 */
/// The IAS_CIE_Address attribute specifies the address that commands
/// generated by the server SHALL be sent to.
#define ATTRID_SS_IAS_CIE_ADDRESS                                        0x0010 // M, R/W, IEEE ADDRESS
#define ATTRID_IAS_ZONE_IASCIE_ADDRESS                                   0x0010 // M, R/W, IEEE ADDRESS
/// A unique reference number allocated by the CIE at zone enrollment time.
#define ATTRID_IAS_ZONE_ZONE_ID                                          0x0011 // M, R, uint8_t
/// Provides the total number of sensitivity levels supported by the
/// IAS Zone server.
#define ATTRID_IAS_ZONE_NUMBER_OF_ZONE_SENSITIVITY_LEVELS_SUPPORTED      0x0012 // O, R, uint8_t
/// Allows an IAS Zone client to query and configure the IAS Zone server's
/// sensitivity level.
#define ATTRID_IAS_ZONE_CURRENT_ZONE_SENSITIVITY_LEVEL                   0x0013 // O, R, uint8_t
/** @} End SS_IAS_ATTRID */

/**
 * @defgroup SS_IAS_ZONE_STATUS_COMMAND_GENERATED SS IAS Zone Status Command Generated
 * @{
 * @brief This group defines the server commands generated (Server-to-Client in ZCL Header)
 * defined in the ZCL v7 specification
 */
 /// The Zone Status Change Notification command is generated when a change
 /// takes place in one or more bits of the ZoneStatus attribute.
#define COMMAND_IAS_ZONE_ZONE_STATUS_CHANGE_NOTIFICATION                 0x00
/// The Zone Enroll Request command is generated when a device embodying
/// the Zone server cluster wishes to be enrolled as an active alarm device.
#define COMMAND_IAS_ZONE_ZONE_ENROLL_REQUEST                             0x01
/** @} End SS_IAS_ZONE_STATUS_COMMAND_GENERATED */

/**
 * @defgroup SS_IAS_ZONE_STATUS_COMMAND_RECEIVED SS IAS Zone Status Command Received
 * @{
 * @brief This group defines the server commands received (Client-to-Server in ZCL Header)
 * defined in the ZCL v7 specification
 */
 /// On receipt, the device embodying the Zone server is notified that it is
 /// now enrolled as an active alarm device
#define COMMAND_IAS_ZONE_ZONE_ENROLL_RESPONSE                            0x00
/// Used to tell the IAS Zone server to commence normal operation mode.
#define COMMAND_IAS_ZONE_INITIATE_NORMAL_OPERATION_MODE                  0x01
/// This command enables IAS Zone servers to be remotely placed into a
/// test mode so that the user or installer MAY configure their field of view,
/// sensitivity, and other operational parameters.
#define COMMAND_IAS_ZONE_INITIATE_TEST_MODE                              0x02
/** @} End SS_IAS_ZONE_STATUS_COMMAND_RECEIVED */

/**
 * @defgroup ENROLL_RESPONSE_SS_IAS_ZONE SS IAS Zone Enroll Response
 * @{
 * @brief This group defines the permitted values for Enroll Response Code field
 * defined in the ZCL v7 specification
 */
#define SS_IAS_ZONE_STATUS_ENROLL_RESPONSE_CODE_SUCCESS                  0x00
#define SS_IAS_ZONE_STATUS_ENROLL_RESPONSE_CODE_NOT_SUPPORTED            0x01
#define SS_IAS_ZONE_STATUS_ENROLL_RESPONSE_CODE_NO_ENROLL_PERMIT         0x02
#define SS_IAS_ZONE_STATUS_ENROLL_RESPONSE_CODE_TOO_MANY_ZONES           0x03
/** @} End ENROLL_RESPONSE_SS_IAS_ZONE */

/**
 * @defgroup SS_IAS_ZONE_PAYLOAD SS IAS Zone Payload
 * @{
 * @brief This group defines the payload lengths
 * defined in the ZCL v7 specification
 */
#define PAYLOAD_LEN_ZONE_STATUS_CHANGE_NOTIFICATION   6
#define PAYLOAD_LEN_ZONE_ENROLL_REQUEST               4
#define PAYLOAD_LEN_ZONE_STATUS_ENROLL_RSP            2
#define PAYLOAD_LEN_ZONE_STATUS_INIT_TEST_MODE        2
/** @} End SS_IAS_ZONE_PAYLOAD */
/** @} End SS_IAS_ZONE */

/**
 * @defgroup SS_IAS_ACE_COMMAND_RECEIVED SS IAS ACE Commands Received
 * @{
 * @brief This group defines the server commands received (Client-to-Server in ZCL Header)
 * defined in the ZCL v7 specification
 */
/// On receipt of this command, the receiving device sets its arm mode
/// according to the value of the Arm Mode field.
#define COMMAND_IASACE_ARM                                           0x00
/// Provides IAS ACE clients with a method to send zone bypass requests
/// to the IAS ACE server.
#define COMMAND_IASACE_BYPASS                                        0x01
/// Command to indicate emergency.
#define COMMAND_IASACE_EMERGENCY                                     0x02
/// Command to indicate fire emergency.
#define COMMAND_IASACE_FIRE                                          0x03
/// Command to indicate panic emergency.
#define COMMAND_IASACE_PANIC                                         0x04
/// On receipt of this command, the device SHALL generate a Get Zone ID
/// Map Response command
#define COMMAND_IASACE_GET_ZONE_ID_MAP                               0x05
/// On receipt of this command, the device SHALL generate a Get Zone
/// Information Response command
#define COMMAND_IASACE_GET_ZONE_INFORMATION                          0x06
/// This command is used by ACE clients to request an update to the status.
#define COMMAND_IASACE_GET_PANEL_STATUS                              0x07
/// Provides IAS ACE clients with a way to retrieve the list of
/// zones to be bypassed.
#define COMMAND_IASACE_GET_BYPASSED_ZONE_LIST                        0x08
/// This command is used by ACE clients to request an update of the
/// status of the IAS Zone devices managed by the ACE server.
#define COMMAND_IASACE_GET_ZONE_STATUS                               0x09
/** @} End SS_IAS_ACE_COMMAND_RECEIVED */

/**
 * @defgroup SS_IAS_ACE_COMMAND_GENERATED SS IAS ACE Commands Generated
 * @{
 * @brief This group defines the server commands generated (Server-to-Client in ZCL Header)
 * defined in the ZCL v7 specification
 */
#define COMMAND_IASACE_ARM_RESPONSE                                     0x00
#define COMMAND_IASACE_GET_ZONE_ID_MAP_RESPONSE                         0x01
#define COMMAND_IASACE_GET_ZONE_INFORMATION_RESPONSE                    0x02
#define COMMAND_IASACE_ZONE_STATUS_CHANGED                              0x03
#define COMMAND_IASACE_PANEL_STATUS_CHANGED                             0x04
#define COMMAND_IASACE_GET_PANEL_STATUS_RESPONSE                        0x05
#define COMMAND_IASACE_SET_BYPASSED_ZONE_LIST                           0x06
#define COMMAND_IASACE_BYPASS_RESPONSE                                  0x07
#define COMMAND_IASACE_GET_ZONE_STATUS_RESPONSE                         0x08
/** @} End SS_IAS_ACE_COMMAND_GENERATED */

/**
 * @defgroup ARM_MODE_SS_IAS_ACE SS IAS ACE Arm Modes
 * @{
 * @brief This group defines the Arm Mode field permitted values
 * defined in the ZCL v7 specification
 */
#define SS_IAS_ACE_ARM_DISARM                                            0x00
#define SS_IAS_ACE_ARM_DAY_HOME_ZONES_ONLY                               0x01
#define SS_IAS_ACE_ARM_NIGHT_SLEEP_ZONES_ONLY                            0x02
#define SS_IAS_ACE_ARM_ALL_ZONES                                         0x03
/** @} End ARM_MODE_SS_IAS_ACE */

/**
 * @defgroup ARM_NOTIFICATION_SS_IAS_ACE SS IAS ACE Arm Notifications
 * @{
 * @brief This group defines the Arm Notification field permitted values
 * defined in the ZCL v7 specification
 */
#define SS_IAS_ACE_ARM_NOTIFICATION_ALL_ZONES_DISARMED                   0x00
#define SS_IAS_ACE_ARM_NOTIFICATION_DAY_HOME_ZONES_ONLY                  0x01
#define SS_IAS_ACE_ARM_NOTIFICATION_NIGHT_SLEEP_ZONES_ONLY               0x02
#define SS_IAS_ACE_ARM_NOTIFICATION_ALL_ZONES_ARMED                      0x03
#define SS_IAS_ACE_ARM_NOTIFICATION_INVALID_ARM_DISARM_CODE              0x04
#define SS_IAS_ACE_ARM_NOTIFICATION_NOT_READY_TO_ARM                     0x05
#define SS_IAS_ACE_ARM_NOTIFICATION_ALREADY_DISARMED                     0x06
/** @} End ARM_NOTIFICATION_SS_IAS_ACE */

/**
 * @defgroup PANEL_STATUS_SS_IAS_ACE SS IAS ACE Panel Status
 * @{
 * @brief This group defines the Panel Status field permitted values
 * defined in the ZCL v7 specification
 */
#define SS_IAS_ACE_PANEL_STATUS_ALL_ZONES_DISARMED                       0x00
#define SS_IAS_ACE_PANEL_STATUS_ARMED_STAY                               0x01
#define SS_IAS_ACE_PANEL_STATUS_ARMED_NIGHT                              0x02
#define SS_IAS_ACE_PANEL_STATUS_ARMED_AWAY                               0x03
#define SS_IAS_ACE_PANEL_STATUS_EXIT_DELAY                               0x04
#define SS_IAS_ACE_PANEL_STATUS_ENTRY_DELAY                              0x05
#define SS_IAS_ACE_PANEL_STATUS_NOT_READY_TO_ARM                         0x06
#define SS_IAS_ACE_PANEL_STATUS_IN_ALARM                                 0x07
#define SS_IAS_ACE_PANEL_STATUS_ARMING_STAY                              0x08
#define SS_IAS_ACE_PANEL_STATUS_ARMING_NIGHT                             0x09
#define SS_IAS_ACE_PANEL_STATUS_ARMING_AWAY                              0x0A
/** @} End PANEL_STATUS_SS_IAS_ACE */

/**
 * @defgroup AUDIBLE_NOTIFICATION_SS_IAS_ACE SS IAS ACE Audible Notification
 * @{
 * @brief This group defines the Audible Notification field permitted values
 * defined in the ZCL v7 specification
 */
#define SS_IAS_ACE_AUDIBLE_NOTIFICATION_MUTE                             0x00
#define SS_IAS_ACE_AUDIBLE_NOTIFICATION_DEFAULT_SOUND                    0x01
/** @} End AUDIBLE_NOTIFICATION_SS_IAS_ACE */

/**
 * @defgroup ALARM_STATUS_SS_IAS_ACE SS IAS ACE Alarm Status
 * @{
 * @brief This group defines the Alarm Status field permitted values
 * defined in the ZCL v7 specification
 */
#define SS_IAS_ACE_ALARM_STATUS_NO_ALARM                                 0x00
#define SS_IAS_ACE_ALARM_STATUS_BURGLAR                                  0x01
#define SS_IAS_ACE_ALARM_STATUS_FIRE                                     0x02
#define SS_IAS_ACE_ALARM_STATUS_EMERGENCY                                0x03
#define SS_IAS_ACE_ALARM_STATUS_POLICE_PANIC                             0x04
#define SS_IAS_ACE_ALARM_STATUS_FIRE_PANIC                               0x05
#define SS_IAS_ACE_ALARM_STATUS_EMERGENCY_PANIC                          0x06
/** @} End ALARM_STATUS_SS_IAS_ACE */

/**
 * @defgroup BYPASS_RESULT_SS_IAS_ACE SS IAS ACE Bypass Result
 * @{
 * @brief This group defines the Bypass Result field permitted values
 * defined in the ZCL v7 specification
 */
#define SS_IAS_ACE_BYPASS_RESULT_ZONE_BYPASSED                           0x00
#define SS_IAS_ACE_BYPASS_RESULT_ZONE_NOT_BYPASSED                       0x01
#define SS_IAS_ACE_BYPASS_RESULT_NOT_ALLOWED                             0x02
#define SS_IAS_ACE_BYPASS_RESULT_INVALID_ZONE_ID                         0x03
#define SS_IAS_ACE_BYPASS_RESULT_UNKNOWN_ZONE_ID                         0x04
#define SS_IAS_ACE_BYPASS_RESULT_INVALID_ARM_DISARM_CODE                 0x05
/** @} End BYPASS_RESULT_SS_IAS_ACE */

/**
 * @defgroup FIELD_SS_IAS_ACE SS IAS ACE Field Lengths
 * @{
 * @brief This group defines the field lengths
 */
#define ZONE_ID_MAP_ARRAY_SIZE  16
#define ARM_DISARM_CODE_LEN     8
#define ZONE_LABEL_LEN          24
/** @} End FIELD_SS_IAS_ACE */

/**
 * @defgroup SS_IAS_ACE_PAYLOAD_LEN SS IAS ACE Payload Length
 * @{
 * @brief This group defines the payload lengths
 */
#define PAYLOAD_LEN_GET_ZONE_STATUS                 5
#define PAYLOAD_LEN_PANEL_STATUS_CHANGED            4
#define PAYLOAD_LEN_GET_PANEL_STATUS_RESPONSE       4
/** @} End SS_IAS_ACE_PAYLOAD_LEN */
/** @} End SS_IAS_ACE */

/**
 * @defgroup SS_IAS_WD_ATTRID SS IAS WD Attribute ID
 * @{
 * @brief This group defines the Maximum Duration attribute
 * defined in the ZCL v7 specification
 */
/// The MaxDuration attribute specifies the maximum time in seconds that the
/// siren will sound continuously, regardless of start/stop commands.
#define ATTRID_IAS_WD_MAX_DURATION                                      0x0000
/** @} End SS_IAS_WD_ATTRID */

/**
 * @defgroup SS_IAS_WD_COMMAND_RECIEVED SS IAS WD Commands Received
 * @{
 * @brief This group defines the server commands received (Client-to-Server in ZCL Header)
 * defined in the ZCL v7 specification
 */
/// This command starts the WD operation. The WD alerts the surrounding area
/// by audible (siren) and visual (strobe) signals.
#define COMMAND_IAS_WD_START_WARNING                                    0x00
/// This command uses the WD capabilities to emit a quick audible/visible
/// pulse called a "squawk". The squawk command has no effect if the WD
/// is currently active (warning in progress).
#define COMMAND_IAS_WD_SQUAWK                                           0x01
/** @} End SS_IAS_WD_COMMAND_RECIEVED */

/**
 * @defgroup START_WARNING_MODE_SS_IAS_WD SS IAS WD Warning Mode
 * @{
 * @brief This group defines the warning mode field values
 * defined in the ZCL v7 specification
 */
#define SS_IAS_START_WARNING_WARNING_MODE_STOP                           0
#define SS_IAS_START_WARNING_WARNING_MODE_BURGLAR                        1
#define SS_IAS_START_WARNING_WARNING_MODE_FIRE                           2
#define SS_IAS_START_WARNING_WARNING_MODE_EMERGENCY                      3
#define SS_IAS_START_WARNING_WARNING_MODE_POLICE_PANIC                   4
#define SS_IAS_START_WARNING_WARNING_MODE_FIRE_PANIC                     5
#define SS_IAS_START_WARNING_WARNING_MODE_EMERGENCY_PANIC                6
/** @} End START_WARNING_MODE_SS_IAS_WD */

/**
 * @defgroup START_WARNING_STROBE_SS_IAS_WD SS IAS WD Warning Strobe
 * @{
 * @brief This group defines the start warning: strobe field values
 * defined in the ZCL v7 specification
 */
#define SS_IAS_START_WARNING_STROBE_NO_STROBE_WARNING                    0
#define SS_IAS_START_WARNING_STROBE_USE_STPOBE_IN_PARALLEL_TO_WARNING    1
/** @} End START_WARNING_STROBE_SS_IAS_WD */

/**
 * @defgroup SIREN_LEVEL_SS_IAS_WD SS IAS WD Siren Level
 * @{
 * @brief This group defines the siren level field values
 * defined in the ZCL v7 specification
 */
#define SS_IAS_SIREN_LEVEL_LOW_LEVEL_SOUND                               0
#define SS_IAS_SIREN_LEVEL_MEDIUM_LEVEL_SOUND                            1
#define SS_IAS_SIREN_LEVEL_HIGH_LEVEL_SOUND                              2
#define SS_IAS_SIREN_LEVEL_VERY_HIGH_LEVEL_SOUND                         3
/** @} End SIREN_LEVEL_SS_IAS_WD */

/**
 * @defgroup STROBE_LEVEL_SS_IAS_WD SS IAS WD Strobe Level
 * @{
 * @brief This group defines the strobe level field values
 * defined in the ZCL v7 specification
 */
#define SS_IAS_STROBE_LEVEL_LOW_LEVEL_STROBE                             0
#define SS_IAS_STROBE_LEVEL_MEDIUM_LEVEL_STROBE                          1
#define SS_IAS_STROBE_LEVEL_HIGH_LEVEL_STROBE                            2
#define SS_IAS_STROBE_LEVEL_VERY_HIGH_LEVEL_STROBE                       3
/** @} End STROBE_LEVEL_SS_IAS_WD */

/**
 * @defgroup SQUAWK_MODE_SS_IAS_WD SS IAS WD Squawk Mode
 * @{
 * @brief This group defines the squawk mode field values
 * defined in the ZCL v7 specification
 */
#define SS_IAS_SQUAWK_SQUAWK_MODE_SYSTEM_ALARMED_NOTIFICATION_SOUND      0
#define SS_IAS_SQUAWK_SQUAWK_MODE_SYSTEM_DISARMED_NOTIFICATION_SOUND     1
/** @} End SQUAWK_MODE_SS_IAS_WD */

/**
 * @defgroup SQUAWK_STROBE_SS_IAS_WD SS IAS WD Squawk Strobe
 * @{
 * @brief This group defines the squawk strobe field values
 * defined in the ZCL v7 specification
 */
#define SS_IAS_SQUAWK_STROBE_NO_STROBE_SQUAWK                            0
#define SS_IAS_SQUAWK_STROBE_USE_STROBE_BLINK_IN_PARALLEL_TO_SQUAWK      1
/** @} End SQUAWK_STROBE_SS_IAS_WD */

/**
 * @defgroup SQUAWK_LEVEL_SS_IAS_WD SS IAS WD Squawk Level
 * @{
 * @brief This group defines the squawk level field values
 * defined in the ZCL v7 specification
 */
#define SS_IAS_SQUAWK_SQUAWK_LEVEL_LOW_LEVEL_SOUND                       0
#define SS_IAS_SQUAWK_SQUAWK_LEVEL_MEDIUM_LEVEL_SOUND                    1
#define SS_IAS_SQUAWK_SQUAWK_LEVEL_HIGH_LEVEL_SOUND                      2
#define SS_IAS_SQUAWK_SQUAWK_LEVEL_VERY_HIGH_LEVEL_SOUND                 3
/** @} End SQUAWK_LEVEL_SS_IAS_WD */

/**
 * @defgroup MAX_ENTRIES_SS_IAS_WD SS IAS WD Maximum Entries
 * @{
 * @brief This group defines the maximum number of entries in the Zone table
 */
#define ZCL_SS_MAX_ZONES                                                 256
#define ZCL_SS_MAX_ZONE_ID                                               254
/** @} End MAX_ENTRIES_SS_IAS_WD */
/** @} End SS_IAS_WD */

/*********************************************************************
 * TYPEDEFS
 */
/**
 * @defgroup ZCL_IAS_ZONE_CALLBACKS ZCL IAS Zone Structs and Callbacks
 * @{
 * @brief This group defines the structs and callbacks used for IAS Zone devices
 */
/*** Structures used for callback functions  ***/
typedef struct s_zb_zcl_ss_zone_change_notification
{
    uint16_t zone_status;
    uint8_t extended_status;
    uint8_t zone_id;
    uint16_t delay;
} s_zb_zcl_ss_zone_change_notification_t;

typedef struct s_zb_zcl_ss_zone_enroll_request
{
    s_zb_af_address_t *src_addr;
    uint8_t zone_id;
    uint16_t zone_type;
    uint16_t manufacturer_code;
} s_zb_zcl_ss_zone_enroll_request_t;

typedef struct s_zb_zcl_ss_zone_enroll_response
{
    uint8_t response_code;
    uint8_t zone_id;
    uint16_t source_addr;
} s_zb_zcl_ss_zone_enroll_response_t;

// This callback is called to process a Change Notification command
typedef zb_status_t (*pfn_zcl_ss_change_notification_t)(s_zb_zcl_ss_zone_change_notification_t *noti, s_zb_af_address_t *src_addr);

// This callback is called to process a Enroll Request command
typedef zb_status_t (*pfn_zcl_ss_enroll_request_t)(s_zb_zcl_ss_zone_enroll_request_t *req, uint8_t endpoint);

/**
 * @defgroup ZCL_IAS_ACE_CALLBACKS ZCL IAS ACE Structs and Callbacks
 * @{
 * @brief This group defines the structs and callbacks used for IAS ACE devices
 */
typedef struct s_zb_zcl_ss_ace_arm
{
    uint8_t arm_mode;                   // The Arm Mode field
    s_utf8_string_t arm_disarm_code;    // The Arm/Disarm code SHALL be a code entered into the ACE client
    uint8_t zone_id;                    // The Zone ID is the index of the Zone in the CIE's zone table
} s_zb_zcl_ace_arm_t;

typedef struct s_zb_zcl_ss_ace_bypass
{
    uint8_t number_of_zones;
    uint8_t *bypass_buf;                // Zone IDs array of 256 entries one byte each
    s_utf8_string_t arm_disarm_code;    // The Arm/Disarm code SHALL be a code entered into the ACE client
} s_zb_zcl_ace_bypass_t;

typedef struct s_zb_zcl_ss_ace_get_zone_status
{
    uint8_t starting_zone_id;       // at which the client like to obtain information
    uint8_t max_num_zone_ids;       // Number of Zone statuses returned by Server to Client
    uint8_t zone_status_mask_flag;  // boolean field
    uint16_t zone_status_mask;      // Coupled with the Zone status mask flag field, functions as a mask
                                    // to enable IAS ACE clients to get information about 11204 the Zone IDs whose ZoneStatus attribute
} s_zb_zcl_ace_get_zone_status_t;

typedef struct s_zb_zcl_ss_ace_get_zone_info_rsp
{
    uint8_t zone_id;
    uint16_t zone_type;
    uint64_t ieee_addr;
    s_utf8_string_t zone_label;
} s_zb_zcl_ace_get_zone_info_rsp_t;

typedef struct s_zb_zcl_ss_ace_zone_status_changed
{
    uint8_t zone_id;
    uint16_t zone_status;
    uint8_t audible_notification;
    s_utf8_string_t zone_label;
} s_zb_zcl_ace_zone_status_changed_t;

typedef struct s_zb_zcl_ss_ace_panel_status_changed
{
    uint8_t panel_status;
    uint8_t seconds_remaining;
    uint8_t audible_notification;
    uint8_t alarm_status;
} s_zb_zcl_ace_panel_status_changed_t;

typedef struct s_zb_zcl_ss_ace_panel_status_rsp
{
    uint8_t panel_status;
    uint8_t seconds_remaining;
    uint8_t audible_notification;
    uint8_t alarm_status;
} s_zb_zcl_ace_panel_status_rsp_t;

typedef struct s_zb_zcl_ss_ace_set_bypassed_zone_list
{
    uint8_t number_of_zones;
    uint8_t *zone_id;
} s_zb_zcl_ace_set_bypassed_zone_list_t;

typedef struct s_zb_zcl_ss_ace_bypass_rsp
{
    uint8_t number_of_zones;
    uint8_t *bypass_result;
} s_zb_zcl_ace_bypass_rsp_t;

typedef struct s_zb_zcl_ss_ace_zone_status
{
    uint8_t zone_id;
    uint16_t zone_status;
} s_zb_zcl_ace_zone_status_t;

typedef struct s_zb_zcl_ss_ace_get_zone_status_rsp
{
    uint8_t zone_status_complete;
    uint8_t number_of_zones;
    s_zb_zcl_ace_zone_status_t *zone_info;
} s_zb_zcl_ace_get_zone_status_rsp_t;

// Typedef for IAS ACE Zone table
typedef struct s_zb_zcl_ss_ias_ace_zone_table
{
    uint8_t zone_id;
    uint16_t zone_type;
    uint64_t zone_address;
} s_zb_zcl_ias_ace_zone_table_t;

/// This callback is called to process an Arm command
typedef uint8_t (*pfn_zcl_ss_ace_arm_t)( s_zb_zcl_ace_arm_t *pCmd );

/// This callback is called to process a Bypass command
typedef zb_status_t (*pfn_zcl_ss_ace_bypass_t)( s_zb_zcl_ace_bypass_t *pCmd );

/// This callback is called to process an Emergency command
typedef zb_status_t (*pfn_zcl_ss_ace_emergency_t)( void );

/// This callback is called to process a Fire command
typedef zb_status_t (*pfn_zcl_ss_ace_fire_t)( void );

/// This callback is called to process a Panic command
typedef zb_status_t (*pfn_zcl_ss_ace_panic_t)( void );

/// This callback is called to process a Get Zone ID Map command
typedef zb_status_t (*pfn_zcl_ss_ace_get_zone_id_map_t)( void );

/// This callback is called to process a Get Zone Information command
typedef zb_status_t (*pfn_zcl_ss_ace_get_zone_information_t)( s_zb_zcl_incoming_msg_t *pInMsg );

/// This callback is called to process a Get Panel Status command
typedef zb_status_t (*pfn_zcl_ss_ace_get_panel_status_t)( s_zb_zcl_incoming_msg_t *pInMsg );

/// This callback is called to process a Get Bypassed Zone List command
typedef zb_status_t (*pfn_zcl_ss_ace_get_bypassed_zone_list_t)( s_zb_zcl_incoming_msg_t *pInMsg );

/// This callback is called to process a Get Zone Status command
typedef zb_status_t (*pfn_zcl_ss_ace_get_zone_status_t)( s_zb_zcl_incoming_msg_t *pInMsg );

/// This callback is called to process an Arm Response command
typedef zb_status_t (*pfn_zcl_ss_ace_arm_response_t)( uint8_t arm_notification );

/// This callback is called to process a Get Zone ID Map Response command
typedef zb_status_t (*pfn_zcl_ss_ace_get_zone_id_map_response_t)( uint16_t *zone_id_map );

/// This callback is called to process a Get Zone Information Response command
typedef zb_status_t (*pfn_zcl_ss_ace_get_zone_information_response_t)( s_zb_zcl_ace_get_zone_info_rsp_t *rsp );

/// This callback is called to process a Zone Status Changed command
typedef zb_status_t (*pfn_zcl_ss_ace_zone_status_changed_t)( s_zb_zcl_ace_zone_status_changed_t *cmd );

/// This callback is called to process a Panel Status Changed command
typedef zb_status_t (*pfn_zcl_ss_ace_panel_status_changed_t)( s_zb_zcl_ace_panel_status_changed_t *cmd );

/// This callback is called to process a Get Panel Status Response command
typedef zb_status_t (*pfn_zcl_ss_ace_get_panel_status_response_t)( s_zb_zcl_ace_panel_status_rsp_t *rsp );

/// This callback is called to process a Set Bypassed Zone List command
typedef zb_status_t (*pfn_zcl_ss_ace_set_bypassed_zone_list_t)( s_zb_zcl_ace_set_bypassed_zone_list_t *cmd );

/// This callback is called to process an Bypass Response command
typedef zb_status_t (*pfn_zcl_ss_ace_bypass_response_t)( s_zb_zcl_ace_bypass_rsp_t *rsp );

/// This callback is called to process an Get Zone Status Response command
typedef zb_status_t (*pfn_zcl_ss_ace_get_zone_status_response_t)( s_zb_zcl_ace_get_zone_status_rsp_t *rsp );

/**
 * @defgroup ZCL_IAS_WD_CALLBACKS ZCL IAS WD Structs and Callbacks
 * @{
 * @brief This group defines the structs and callbacks used for IAS WD devices
 */
typedef union s_zb_zcl_ss_wd_warning
{
    struct
    {
        uint8_t warn_mode : 4;
        uint8_t warn_strobe : 2;
        uint8_t warn_siren_level : 2;
    } warning_bits;
    uint8_t warning_byte;
} s_zb_zcl_wd_warning_t;

typedef struct s_zb_zcl_ss_wd_start_warning
{
    s_zb_zcl_wd_warning_t warning_message;
    uint16_t warning_duration;
    uint8_t strobe_duty_cycle;
    uint8_t strobe_level;
} s_zb_zcl_wd_start_warning_t;

typedef struct s_zb_zcl_ss_wd_squawk_bits
{
    uint8_t squawk_mode : 4;
    uint8_t strobe : 1;
    uint8_t reserved : 1;
    uint8_t squawk_level : 2;
} s_zb_zcl_wd_squawk_bits_t;

typedef union s_zb_zcl_ss_wd_squawk
{
    s_zb_zcl_wd_squawk_bits_t squawk_bits;
    uint8_t squawk_byte;
} s_zb_zcl_wd_squawk_t;

// Register Callback table entry - enter function pointers for callbacks that
// the application wishes to handle
typedef struct s_zb_zcl_ss_app_callbacks
{
    pfn_zcl_ss_change_notification_t                    pfn_zone_change_notification;
    pfn_zcl_ss_enroll_request_t                         pfn_zone_enroll_request;
    pfn_zcl_ss_ace_arm_t                                pfn_ace_arm;
    pfn_zcl_ss_ace_bypass_t                             pfn_ace_bypass;
    pfn_zcl_ss_ace_emergency_t                          pfn_ace_emergency;
    pfn_zcl_ss_ace_fire_t                               pfn_ace_fire;
    pfn_zcl_ss_ace_panic_t                              pfn_ace_panic;
    pfn_zcl_ss_ace_get_zone_id_map_t                    pfn_ace_get_zone_id_map;
    pfn_zcl_ss_ace_get_zone_information_t               pfn_ace_get_zone_information;
    pfn_zcl_ss_ace_get_panel_status_t                   pfn_ace_get_panel_status;
    pfn_zcl_ss_ace_get_bypassed_zone_list_t             pfn_ace_get_bypassed_zone_list;
    pfn_zcl_ss_ace_get_zone_status_t                    pfn_ace_get_zone_status;
    pfn_zcl_ss_ace_arm_response_t                       pfn_ace_arm_response;
    pfn_zcl_ss_ace_get_zone_id_map_response_t           pfn_ace_get_zone_id_map_response;
    pfn_zcl_ss_ace_get_zone_information_response_t      pfn_ace_get_zone_information_response;
    pfn_zcl_ss_ace_zone_status_changed_t                pfn_ace_zone_status_changed;
    pfn_zcl_ss_ace_panel_status_changed_t               pfn_ace_panel_status_changed;
    pfn_zcl_ss_ace_get_panel_status_response_t          pfn_ace_get_panel_status_response;
    pfn_zcl_ss_ace_set_bypassed_zone_list_t             pfn_ace_set_bypassed_zone_list;
    pfn_zcl_ss_ace_bypass_response_t                    pfn_ace_bypass_response;
    pfn_zcl_ss_ace_get_zone_status_response_t           pfn_ace_get_zone_status_response;
} s_zb_zcl_ss_app_callbacks_t;

/*********************************************************************
 * FUNCTION MACROS
 */
// ZCL IAS Zone Cluster Client Commands
#define zb_zcl_ss_ias_send_zone_status_init_normal_operation_mode(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        CLUSTER_ID_IAS_ZONE, COMMAND_IAS_ZONE_INITIATE_NORMAL_OPERATION_MODE, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

// ZCL ACE Cluster Client Commands
#define zb_zcl_ss_send_ias_ace_emergency_cmd(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        CLUSTER_ID_IAS_ACE, COMMAND_IASACE_EMERGENCY, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

#define zb_zcl_ss_send_ias_ace_fire_cmd(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        CLUSTER_ID_IAS_ACE, COMMAND_IASACE_FIRE, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

#define zb_zcl_ss_send_ias_ace_panic_cmd(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        CLUSTER_ID_IAS_ACE, COMMAND_IASACE_PANIC, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

#define zb_zcl_ss_send_ias_ace_get_zone_id_map_cmd(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        CLUSTER_ID_IAS_ACE, COMMAND_IASACE_GET_ZONE_ID_MAP, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

#define zb_zcl_ss_send_ias_ace_get_panel_status_cmd(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        CLUSTER_ID_IAS_ACE, COMMAND_IASACE_GET_PANEL_STATUS, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

#define zb_zcl_ss_send_ias_ace_get_bypassed_zone_list_cmd(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        CLUSTER_ID_IAS_ACE, COMMAND_IASACE_GET_BYPASSED_ZONE_LIST, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)


/*********************************************************************
 * FUNCTIONS
 */

/**
 * @brief Register command callbacks
 * 
 * @param endpoint - Endpoint
 * @param callbacks - Callbacks
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_register_command_callbacks(
    uint8_t endpoint, s_zb_zcl_ss_app_callbacks_t *callbacks);

// ZCL IAS Zone Cluster Client Commands
/**
 * @brief Send IAS Zone Enroll Command Response
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param response_code - Response Code
 * @param zone_id - Zone ID
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_ias_send_zone_status_enroll_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t response_code, uint8_t zone_id,
    uint8_t disable_default_rsp, uint8_t seq_num);


// ZCL IAS Zone - ACE helper functions
/**
 * @brief Update zone address
 * 
 * @param endpoint - Endpoint of zone
 * @param zone_id - Zone ID to look for zone
 * @param ieee_addr - IEEE Address
 * @return void
 */
void zb_zcl_ss_update_zone_address(uint8_t endpoint, uint8_t zone_id, uint64_t ieee_addr);

/**
 * @brief Remove zone
 * 
 * @param endpoint - Endpoint of zone to be removed
 * @param zone_id - Zone ID to look for zone
 * @return uint8_t - true if removed, false if not found
 */
uint8_t zb_zcl_ss_remove_zone(uint8_t endpoint, uint8_t zone_id);

/**
 * @brief Find zone
 * 
 * @param endpoint - Endpoint of zone to be added
 * @param zone_id - Zone ID to look for zone
 * @return s_zb_zcl_ias_ace_zone_table_t * - Zone table entry
 */
s_zb_zcl_ias_ace_zone_table_t *zb_zcl_ss_find_zone(uint8_t endpoint, uint8_t zone_id);

// ZCL ACE Cluster Client Commands
/**
 * @brief Send IAS ACE Arm Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param cmd - Arm Command
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_arm_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_arm_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send IAS ACE Bypass Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param cmd - Bypass Command
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_bypass_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_bypass_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send IAS ACE Get Zone Information Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param zone_id - Zone ID
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_get_zone_information_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t zone_id, uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send IAS ACE Get Zone Status Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param cmd - Get Zone Status Command
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_get_zone_status_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_get_zone_status_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num);

// ZCL ACE Cluster Server Commands
/**
 * @brief Send IAS ACE Arm Response Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param arm_notification - Arm Notification
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_arm_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t arm_notification, uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send IAS ACE Get Zone ID Map Response Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param zone_id_map - pointer to an array of 16 uint16_t
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_get_zone_id_map_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t *zone_id_map, uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send IAS ACE Get Zone Information Response Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param rsp - Response structure
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_get_zone_information_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_get_zone_info_rsp_t *rsp,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send IAS ACE Zone Status Changed Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param cmd - Zone Status Changed Command
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_zone_status_changed_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_zone_status_changed_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send IAS ACE Panel Status Changed Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param cmd - Panel Status Changed Command
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_panel_status_changed_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_panel_status_changed_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send IAS ACE Get Panel Status Response Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param rsp - Response structure
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_get_panel_status_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_panel_status_rsp_t *rsp,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send IAS ACE Set Bypassed Zone List Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param cmd - Set Bypassed Zone List Command
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_set_bypassed_zone_list_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_set_bypassed_zone_list_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send IAS ACE Bypass Response Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param rsp - Response structure
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_bypass_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_bypass_rsp_t *rsp,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send IAS ACE Get Zone Status Response Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param rsp - Response structure
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_get_zone_status_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_get_zone_status_rsp_t *rsp,
    uint8_t disable_default_rsp, uint8_t seq_num);

// ZCL Warning Device Cluster Client Commands
/**
 * @brief Send IAS WD Start Warning Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param cmd - Start Warning Command
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_wd_start_warning_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_wd_start_warning_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send IAS WD Squawk Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param cmd - Squawk Command
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_wd_squawk_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_wd_squawk_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZCL_SS_H_ */