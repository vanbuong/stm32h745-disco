#ifndef ZB_ZCL_CLOSURES_H_
#define ZB_ZCL_CLOSURES_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "zcl/zb_zcl.h"

/*********************************************************************
 * CONSTANTS
 */

/**
* @defgroup CLOSURE_SHADE_MACROS ZCL Closure Shade Config Cluster Macros
* @brief This group provides ZCL Closure Shade Config Cluster macros
* defined in the ZCL v7 specification
* @{
*/

/**********************************************/
/*** Shade Configuration Cluster Attributes ***/
/**********************************************/
  // Shade information attributes set
/// The PhysicalClosedLimit attribute indicates the most closed (numerically lowest)
/// position that the shade can physically move to.
#define ATTRID_SHADE_CONFIGURATION_PHYSICAL_CLOSED_LIMIT               0x0000 // O, R, uint16_t
/// The MotorStepSize attribute indicates the angle the shade motor moves for
/// one step, measured in 1/10ths of a degree.
#define ATTRID_SHADE_CONFIGURATION_MOTOR_STEP_SIZE                     0x0001 // O, R, uint8_t
/// The Status attribute indicates the status of a number of shade functions.
#define ATTRID_SHADE_CONFIGURATION_STATUS                              0x0002 // M, R/W, BITMAP8

/*** Status attribute bit values ***/
/// Shade operational if set
#define CLOSURES_STATUS_SHADE_IS_OPERATIONAL                           0x01
/// Shade adjusting if set
#define CLOSURES_STATUS_SHADE_IS_ADJUSTING                             0x02
/// Shade direction opening if set
#define CLOSURES_STATUS_SHADE_DIRECTION                                0x04
/// Direction corresponding to forward direction of motor, opening if set
#define CLOSURES_STATUS_SHADE_MOTOR_FORWARD_DIRECTION                  0x08

// Shade settings attributes set
/// The ClosedLimit attribute indicates the most closed position that the
/// shade can move to.
#define ATTRID_SHADE_CONFIGURATION_CLOSED_LIMIT                        0x0010
/// The Mode attribute indicates the current operating mode of the shade.
#define ATTRID_SHADE_CONFIGURATION_MODE                                0x0011

/*** Mode attribute values ***/
#define CLOSURES_MODE_NORMAL_MODE                                      0x00
#define CLOSURES_MODE_CONFIGURE_MODE                                   0x01

// cluster has no specific commands

/**********************************************/
/*** Logical Cluster ID - for mapping only  ***/
/***  These are not to be used over-the-air ***/
/**********************************************/
/// The ClosedLimit attribute indicates the most closed position that the
/// shade can move to.
#define ZCL_CLOSURES_LOGICAL_CLUSTER_ID_SHADE_CONFIG                   0x0010
/** @} End CLOSURE_SHADE_MACROS */

/**
* @defgroup CLOSURE_DOORLOCK_MACROS ZCL Closure Doorlock Cluster Macros
* @brief This group provides ZCL Closure Doorlock Cluster macros
* defined in the ZCL v7 specification
* @{
*/

/************************************/
/*** Door Lock Cluster Attributes ***/
/************************************/

/// Enum for lock state
#define ATTRID_DOOR_LOCK_LOCK_STATE                                          0x0000
/// Enum for lock type
#define ATTRID_DOOR_LOCK_LOCK_TYPE                                           0x0001
/// Actuator enabled of set
#define ATTRID_DOOR_LOCK_ACTUATOR_ENABLED                                    0x0002
/// Enum for door state
#define ATTRID_DOOR_LOCK_DOOR_STATE                                          0x0003
/// This attribute holds the number of door open events that have occurred
/// since it was last zeroed.
#define ATTRID_DOOR_LOCK_DOOR_OPEN_EVENTS                                    0x0004
/// This attribute holds the number of door closed events that have occurred
/// since it was last zeroed.
#define ATTRID_DOOR_LOCK_DOOR_CLOSED_EVENTS                                  0x0005
/// This attribute holds the number of minutes the door has been open since
/// the last time it transitioned from closed to open.
#define ATTRID_DOOR_LOCK_OPEN_PERIOD                                         0x0006

// User, PIN, Schedule, & Log Information Attributes
/// The number of available log records.
#define ATTRID_DOOR_LOCK_NUMBER_OF_LOG_RECORDS_SUPPORTED                     0x0010  // O, R, uint16_t
/// Number of total users supported by the lock.
#define ATTRID_DOOR_LOCK_NUMBER_OF_TOTAL_USERS_SUPPORTED                     0x0011  // O, R, uint16_t
/// The number of PIN users supported.
#define ATTRID_DOOR_LOCK_NUMBER_OF_PIN_USERS_SUPPORTED                       0x0012  // O, R, uint16_t
/// The number of RFID users supported.
#define ATTRID_DOOR_LOCK_NUMBER_OF_RFID_USERS_SUPPORTED                      0x0013  // O, R, uint16_t
/// number of configurable week day schedule supported per user.
#define ATTRID_DOOR_LOCK_NUMBER_OF_WEEK_DAY_SCHEDULES_SUPPORTED_PER_USER     0x0014  // O, R, uint8_t
/// The number of configurable year day schedule supported per user.
#define ATTRID_DOOR_LOCK_NUMBER_OF_YEAR_DAY_SCHEDULES_SUPPORTED_PER_USER     0x0015  // O, R, uint8_t
/// The number of holiday schedules supported for the entire door lock device.
#define ATTRID_DOOR_LOCK_NUMBER_OF_HOLIDAY_SCHEDULES_SUPPORTED               0x0016  // O, R, uint8_t
/// An 8 bit value indicates the maximum length in bytes of a PIN Code on this device.
#define ATTRID_DOOR_LOCK_MAX_PIN_CODE_LENGTH                                 0x0017  // O, R, uint8_t
/// An 8 bit value indicates the minimum length in bytes of a PIN Code on this device.
#define ATTRID_DOOR_LOCK_MIN_PIN_CODE_LENGTH                                 0x0018  // O, R, uint8_t
/// An 8 bit value indicates the maximum length in bytes of a RFID Code on this device.
#define ATTRID_DOOR_LOCK_MAX_RFID_CODE_LENGTH                                0x0019  // O, R, uint8_t
/// An 8 bit value indicates the minimum length in bytes of a RFID Code on this device.
#define ATTRID_DOOR_LOCK_MIN_RFID_CODE_LENGTH                                0x001A  // O, R, uint8_t

// Operational Settings Attributes
/// Enable/disable event logging.
#define ATTRID_DOOR_LOCK_ENABLE_LOGGING                                      0x0020  // O, R/W, BOOLEAN
/// Modifies the language for the on-screen or audible user interface using three bytes from ISO-639-1.
#define ATTRID_DOOR_LOCK_LANGUAGE                                            0x0021  // O, R/W, CHAR STRING
/// Configuration for LED events
#define ATTRID_DOOR_LOCK_LED_SETTINGS                                        0x0022  // O, R/W, uint8_t
/// The number of seconds to wait after unlocking a lock before it automatically
/// locks again.
#define ATTRID_DOOR_LOCK_AUTO_RELOCK_TIME                                    0x0023  // O, R/W, uint32_t
/// The sound volume on a door lock has three possible settings: silent, low
/// and high volumes.
#define ATTRID_DOOR_LOCK_SOUND_VOLUME                                        0x0024  // O, R/W, uint8_t
/// shows the current operating mode and which interfaces are enabled during
/// each of the operating mode.
#define ATTRID_DOOR_LOCK_OPERATING_MODE                                      0x0025  // O, R/W, ENUM8
/// This bitmap contains all operating bits of the Operating Mode Attribute
/// supported by the lock.
#define ATTRID_DOOR_LOCK_SUPPORTED_OPERATING_MODES                           0x0026  // O, R, BITMAP16
/// This attribute represents the default configurations as they are physically
/// set on the device.
#define ATTRID_DOOR_LOCK_DEFAULT_CONFIGURATION_REGISTER                      0x0027  // O, R, BITMAP16
/// Enable/disable local programming on the door lock.
#define ATTRID_DOOR_LOCK_ENABLE_LOCAL_PROGRAMMING                            0x0028  // O, R/W, BOOLEAN
/// Enable/disable the ability to lock the door lock with a single touch on the door lock.
#define ATTRID_DOOR_LOCK_ENABLE_ONE_TOUCH_LOCKING                            0x0029  // O, R/W, BOOLEAN
/// Enable/disable an inside LED that allows the user to see at a glance if the door is locked.
#define ATTRID_DOOR_LOCK_ENABLE_INSIDE_STATUS_LED                            0x002A  // O, R/W, BOOLEAN
/// Enable/disable a button inside the door that is used to put the lock into
/// privacy mode. When the lock is in privacy mode it cannot be manipulated from the outside.
#define ATTRID_DOOR_LOCK_ENABLE_PRIVACY_MODE_BUTTON                          0x002B  // O, R/W, BOOLEAN

// Security Settings Attributes
/// The number of incorrect codes or RFID presentment attempts a user is allowed
/// to enter before the door will enter a lockout state.
#define ATTRID_DOOR_LOCK_WRONG_CODE_ENTRY_LIMIT                              0x0030  // O, R/W, uint8_t
/// The number of seconds that the lock shuts down following wrong code entry.
#define ATTRID_DOOR_LOCK_USER_CODE_TEMPORARY_DISABLE_TIME                    0x0031  // O, R/W, uint8_t
/// Boolean set to True if it is ok for the door lock server to send PINs over the air.
#define ATTRID_DOOR_LOCK_SEND_PIN_OVER_THE_AIR                               0x0032  // O, R/W, BOOLEAN
/// Boolean set to True if the door lock server requires that an optional PINs
/// be included in the payload of RF lock operation events like Lock, Unlock
/// and Toggle in order to function.
#define ATTRID_DOORLOCK_REQUIRE_PIN_FOR_RF_OPERATION                         0x0033  // O, R/W, BOOLEAN
#define ATTRID_DOOR_LOCK_REQUIRE_PI_NFOR_RF_OPERATION                        0x0033  // O, R/W, BOOLEAN
/// Door locks MAY sometimes wish to implement a higher level of security
/// within the application protocol in addition to the default network security.
#define ATTRID_DOOR_LOCK_SECURITY_LEVEL                                      0x0034  // O, R, uint8_t

// Alarm and Event Masks Attributes
/// The alarm mask is used to turn on/off alarms for particular functions.
#define ATTRID_DOOR_LOCK_ALARM_MASK                                          0x0040  // O, R/W, BITMAP16
/// Event mask used to turn on and off the transmission of keypad operation events.
#define ATTRID_DOOR_LOCK_KEYPAD_OPERATION_EVENT_MASK                         0x0041  // O, R/W, BITMAP16
/// Event mask used to turn on and off the transmission of RF operation events.
#define ATTRID_DOOR_LOCK_RF_OPERATION_EVENT_MASK                             0x0042  // O, R/W, BITMAP16
/// Event mask used to turn on and off manual operation events.
#define ATTRID_DOOR_LOCK_MANUAL_OPERATION_EVENT_MASK                         0x0043  // O, R/W, BITMAP16
/// Event mask used to turn on and off RFID operation events.
#define ATTRID_DOOR_LOCK_RFID_OPERATION_EVENT_MASK                           0x0044  // O, R/W, BITMAP16
/// Event mask used to turn on and off keypad programming events.
#define ATTRID_DOOR_LOCK_KEYPAD_PROGRAMMING_EVENT_MASK                       0x0045  // O, R/W, BITMAP16
/// Event mask used to turn on and off RF programming events.
#define ATTRID_DOOR_LOCK_RF_PROGRAMMING_EVENT_MASK                           0x0046  // O, R/W, BITMAP16
/// Event mask used to turn on and off RFID programming events.
#define ATTRID_DOOR_LOCK_RFID_PROGRAMMING_EVENT_MASK                         0x0047  // O, R/W, BITMAP16

// User, PIN, Schedule, & Log Information Attribute Defaults
/// User, PIN, Schedule, Log Information Attribute Set default values
#define ATTR_DEFAULT_DOORLOCK_NUM_OF_LOCK_RECORDS_SUPPORTED                  0
#define ATTR_DEFAULT_DOORLOCK_NUM_OF_TOTAL_USERS_SUPPORTED                   0
#define ATTR_DEFAULT_DOORLOCK_NUM_OF_PIN_USERS_SUPPORTED                     0
#define ATTR_DEFAULT_DOORLOCK_NUM_OF_RFID_USERS_SUPPORTED                    0
#define ATTR_DEFAULT_DOORLOCK_NUM_OF_WEEK_DAY_SCHEDULES_SUPPORTED_PER_USER   0
#define ATTR_DEFAULT_DOORLOCK_NUM_OF_YEAR_DAY_SCHEDULES_SUPPORTED_PER_USER   0
#define ATTR_DEFAULT_DOORLOCK_NUM_OF_HOLIDAY_SCHEDULEDS_SUPPORTED            0
#define ATTR_DEFAULT_DOORLOCK_MAX_PIN_LENGTH                                 0x08
#define ATTR_DEFAULT_DOORLOCK_MIN_PIN_LENGTH                                 0x04
#define ATTR_DEFAULT_DOORLOCK_MAX_RFID_LENGTH                                0x14
#define ATTR_DEFAULT_DOORLOCK_MIN_RFID_LENGTH                                0x08

// Operational Settings Attribute Defaults
/// Operational Settings Attribute Set default values
#define ATTR_DEFAULT_DOORLOCK_ENABLE_LOGGING                                 0
#define ATTR_DEFAULT_DOORLOCK_LANGUAGE                                       {0,0,0}
#define ATTR_DEFAULT_DOORLOCK_LED_SETTINGS                                   0
#define ATTR_DEFAULT_DOORLOCK_AUTO_RELOCK_TIME                               0
#define ATTR_DEFAULT_DOORLOCK_SOUND_VOLUME                                   0
#define ATTR_DEFAULT_DOORLOCK_OPERATING_MODE                                 0
#define ATTR_DEFAULT_DOORLOCK_SUPPORTED_OPERATING_MODES                      0x0001
#define ATTR_DEFAULT_DOORLOCK_DEFAULT_CONFIGURATION_REGISTER                 0
#define ATTR_DEFAULT_DOORLOCK_ENABLE_LOCAL_PROGRAMMING                       0
#define ATTR_DEFAULT_DOORLOCK_ENABLE_ONE_TOUCH_LOCKING                       0
#define ATTR_DEFAULT_DOORLOCK_ENABLE_INSIDE_STATUS_LED                       0
#define ATTR_DEFAULT_DOORLOCK_ENABLE_PRIVACY_MODE_BUTTON                     0

// Security Settings Attribute Defaults
/// Security Settings Attribute Set default values
#define ATTR_DEFAULT_DOORLOCK_WRONG_CODE_ENTRY_LIMIT                         0
#define ATTR_DEFAULT_DOORLOCK_USER_CODE_TEMPORARY_DISABLE_TIME               0
#define ATTR_DEFAULT_DOORLOCK_SEND_PIN_OTA                                   0
#define ATTR_DEFAULT_DOORLOCK_REQUIRE_PIN_FOR_RF_OPERATION                   0
#define ATTR_DEFAULT_DOORLOCK_ZIGBEE_SECURITY_LEVEL                          0

// Alarm and Event Masks Attribute Defaults
/// The alarm mask is used to turn on/off alarms for particular functions.
#define ATTR_DEFAULT_DOORLOCK_ALARM_MASK                                     0x0000
/// Event mask used to turn on and off the transmission of keypad operation events.
#define ATTR_DEFAULT_DOORLOCK_KEYPAD_OPERATION_EVENT_MASK                    0x0000
/// Event mask used to turn on and off the transmission of RF operation events.
#define ATTR_DEFAULT_DOORLOCK_RF_OPERATION_EVENT_MASK                        0x0000
/// Event mask used to turn on and off manual operation events.
#define ATTR_DEFAULT_DOORLOCK_MANUAL_OPERATION_EVENT_MASK                    0x0000
/// Event mask used to turn on and off RFID operation events.
#define ATTR_DEFAULT_DOORLOCK_RFID_OPERATION_EVENT_MASK                      0x0000
/// Event mask used to turn on and off keypad programming events.
#define ATTR_DEFAULT_DOORLOCK_KEYPAD_PROGRAMMING_EVENT_MASK                  0x0000
/// Event mask used to turn on and off RF programming events.
#define ATTR_DEFAULT_DOORLOCK_RF_PROGRAMMING_EVENT_MASK                      0x0000
/// Event mask used to turn on and off RFID programming events.
#define ATTR_DEFAULT_DOORLOCK_RFID_PROGRAMMING_EVENT_MASK                    0x0000

 /******************************************************************************************
  * Operating Mode enumerations
  * Interface: (E = Enable; D = Disable)
  * Devices:  (K = Keypad; RF; RFID)
  */
/// Normal Mode: The lock operates normally. All interfaces are enabled.
#define DOORLOCK_OP_MODE_NORMAL                   0x00  // K = E;   RF = E;   RFID = E
/// Vacation Mode: Only RF interaction is enabled. The keypad cannot be operated.
#define DOORLOCK_OP_MODE_VACATION                 0x01  // K = D;   RF = E;   RFID = E
/// Privacy Mode: All external interaction with the door lock is disabled.
#define DOORLOCK_OP_MODE_PRIVACY                  0x02  // K = D;   RF = D;   RFID = D
/// No RF Lock or Unlock: This mode only disables RF interaction with the lock.
#define DOORLOCK_OP_MODE_NO_RF_LOCK_UNLOCK        0x03  // K = E;   RF = D;   RFID = E
/// Passage Mode: The lock is open or can be open or closed at will without
/// the use of a Keypad or other means of user validation.
#define DOORLOCK_OP_MODE_PASSAGE                  0x04  // K = N/A; RF = N/A; RFID = N/A

/*** Lock State Attribute types ***/
/// Lock state values.
#define CLOSURES_LOCK_STATE_NOT_FULLY_LOCKED               0x00
#define CLOSURES_LOCK_STATE_LOCKED                         0x01
#define CLOSURES_LOCK_STATE_UNLOCKED                       0x02

/*** Lock Type Attribute types ***/
/// Lock type values
#define CLOSURES_LOCK_TYPE_DEADBOLT                        0x00
#define CLOSURES_LOCK_TYPE_MAGNETIC                        0x01
#define CLOSURES_LOCK_TYPE_OTHER                           0x02
#define CLOSURES_LOCK_TYPE_MORTISE                         0x03
#define CLOSURES_LOCK_TYPE_RIM                             0x04
#define CLOSURES_LOCK_TYPE_LATCH_BOLT                      0x05
#define CLOSURES_LOCK_TYPE_CYLINDRICAL_LOCK                0x06
#define CLOSURES_LOCK_TYPE_TUBULAR_LOCK                    0x07
#define CLOSURES_LOCK_TYPE_INTERCONNECTED_LOCK             0x08
#define CLOSURES_LOCK_TYPE_DEAD_LATCH                      0x09
#define CLOSURES_LOCK_TYPE_DOOR_FURNITURE                  0x0A

/*** Door State Attribute types ***/
/// Door State values
#define CLOSURES_DOOR_STATE_OPEN                           0x00
#define CLOSURES_DOOR_STATE_CLOSED                         0x01
#define CLOSURES_DOOR_STATE_ERROR_JAMMED                   0x02
#define CLOSURES_DOOR_STATE_ERROR_FORCED_OPEN              0x03
#define CLOSURES_DOOR_STATE_ERROR_UNSPECIFIED              0x04

/**********************************/
/*** Door Lock Cluster Commands ***/
/**********************************/
  // Server Commands Received
/// This command causes the lock device to lock the door.
#define COMMAND_DOOR_LOCK_LOCK_DOOR                         0x00 // M  zclDoorLock_t
/// This command causes the lock device to unlock the door.
#define COMMAND_DOOR_LOCK_UNLOCK_DOOR                       0x01 // M  zclDoorLock_t
/// Request the status of the lock.
#define COMMAND_DOOR_LOCK_TOGGLE                            0x02 // O  zclDoorLock_t
/// This command causes the lock device to unlock the door with a timeout parameter.
#define COMMAND_DOOR_LOCK_UNLOCK_WITH_TIMEOUT               0x03 // O  zclDoorLockUnlockTimeout_t
/// Request a log record.
#define COMMAND_DOOR_LOCK_GET_LOG_RECORD                    0x04 // O  zclDoorLockGetLogRecord_t
/// Set a PIN into the lock.
#define COMMAND_DOOR_LOCK_SET_PIN_CODE                      0x05 // O  zclDoorLockSetPINCode_t
/// Retrieve a PIN Code.
#define COMMAND_DOOR_LOCK_GET_PIN_CODE                      0x06 // O  zclDoorLockUserID_t
/// Delete a PIN.
#define COMMAND_DOOR_LOCK_CLEAR_PIN_CODE                    0x07 // O  zclDoorLockUserID_t
/// Clear out all PINs on the lock.
#define COMMAND_DOOR_LOCK_CLEAR_ALL_PIN_CODES               0x08 // O  no payload
/// Set the status of a user ID.
#define COMMAND_DOOR_LOCK_SET_USER_STATUS                   0x09 // O  zclDoorLockSetUserStatus_t
/// Get the status of a user.
#define COMMAND_DOOR_LOCK_GET_USER_STATUS                   0x0A // O  zclDoorLockUserID_t
/// Set a weekly repeating schedule for a specified user.
#define COMMAND_DOOR_LOCK_SET_WEEKDAY_SCHEDULE              0x0B // O  zclDoorLockSetWeekDaySchedule_t
/// Retrieve the specific weekly schedule for the specific user.
#define COMMAND_DOOR_LOCK_GET_WEEKDAY_SCHEDULE              0x0C // O  zclDoorLockSchedule_t
/// Clear the specific weekly schedule for the specific user.
#define COMMAND_DOOR_LOCK_CLEAR_WEEKDAY_SCHEDULE            0x0D // O  zclDoorLockSchedule_t
/// Set a time-specific schedule ID for a specified user.
#define COMMAND_DOOR_LOCK_SET_YEAR_DAY_SCHEDULE             0x0E // O  zclDoorLockSetYearDaySchedule_t
/// Retrieve the specific year day schedule for the specific user.
#define COMMAND_DOOR_LOCK_GET_YEAR_DAY_SCHEDULE             0x0F // O  zclDoorLockSchedule_t
/// Clears the specific year day schedule for the specific user.
#define COMMAND_DOOR_LOCK_CLEAR_YEAR_DAY_SCHEDULE           0x10 // O  zclDoorLockSchedule_t
/// Set the holiday Schedule by specifying local start time and local end time
/// with respect to any Lock Operating Mode.
#define COMMAND_DOOR_LOCK_SET_HOLIDAY_SCHEDULE              0x11 // O  zclDoorLockSetHolidaySchedule_t
/// Get the holiday Schedule by specifying Holiday ID.
#define COMMAND_DOOR_LOCK_GET_HOLIDAY_SCHEDULE              0x12 // O  zclDoorLockHolidayScheduleID_t
/// Clear the holiday Schedule by specifying Holiday ID.
#define COMMAND_DOOR_LOCK_CLEAR_HOLIDAY_SCHEDULE            0x13 // O  zclDoorLockHolidayScheduleID_t
/// Set the type byte for a specified user.
#define COMMAND_DOOR_LOCK_SET_USER_TYPE                     0x14 // O  zclDoorLockSetUserType_t
/// Retrieve the type byte for a specific user.
#define COMMAND_DOOR_LOCK_GET_USER_TYPE                     0x15 // O  zclDoorLockUserID_t
/// Set an ID for RFID access into the lock.
#define COMMAND_DOOR_LOCK_SET_RFID_CODE                     0x16 // O  zclDoorLockSetRFIDCode_t
/// Retrieve an ID.
#define COMMAND_DOOR_LOCK_GET_RFID_CODE                     0x17 // O  zclDoorLockUserID_t
/// Delete an ID.
#define COMMAND_DOOR_LOCK_CLEAR_RFID_CODE                   0x18 // O  zclDoorLockUserID_t
/// Clear out all RFIDs on the lock.
#define COMMAND_DOOR_LOCK_CLEAR_ALL_RFID_CODES              0x19 // O  no payload

// Server Commands Generated
/// This command is sent in response to a Lock command with one status byte payload.
#define COMMAND_DOOR_LOCK_LOCK_DOOR_RESPONSE                     0x00 // M  status field
/// This command is sent in response to a Toggle command with one status byte payload.
#define COMMAND_DOOR_LOCK_UNLOCK_DOOR_RESPONSE                   0x01 // M  status field
/// This command is sent in response to a Toggle command with one status byte payload.
#define COMMAND_DOOR_LOCK_TOGGLE_RESPONSE                   0x02 // O  status field
/// This command is sent in response to an Unlock with Timeout command with
/// one status byte payload.
#define COMMAND_DOOR_LOCK_UNLOCK_WITH_TIMEOUT_RESPONSE           0x03 // O  status field
/// Returns the specified log record.
#define COMMAND_DOOR_LOCK_GET_LOG_RECORD_RESPONSE                0x04 // O  zclDoorLockGetLogRecordRsp_t
/// Returns status of the PIN set command.
#define COMMAND_DOOR_LOCK_SET_PIN_CODE_RESPONSE                  0x05 // O  status field
/// Returns the PIN for the specified user ID.
#define COMMAND_DOOR_LOCK_GET_PIN_CODE_RESPONSE                  0x06 // O  zclDoorLockGetPINCodeRsp_t
/// Returns pass/fail of the command.
#define COMMAND_DOOR_LOCK_CLEAR_PIN_CODE_RESPONSE                0x07 // O  status field
/// Returns pass/fail of the command.
#define COMMAND_DOOR_LOCK_CLEAR_ALL_PIN_CODES_RESPONSE           0x08 // O  status field
/// Returns the pass or fail value for the setting of the user status.
#define COMMAND_DOOR_LOCK_SET_USER_STATUS_RESPONSE               0x09 // O  status field
/// Returns the user status for the specified user ID.
#define COMMAND_DOOR_LOCK_GET_USER_STATUS_RESPONSE               0x0A // O  zclDoorLockGetUserStateRsp_t
/// Returns pass/fail of the command.
#define COMMAND_DOOR_LOCK_SET_WEEKDAY_SCHEDULE_RESPONSE         0x0B // O  status field
/// Returns the weekly repeating schedule data for the specified schedule ID.
#define COMMAND_DOOR_LOCK_GET_WEEKDAY_SCHEDULE_RESPONSE         0x0C // O  zclDoorLockGetWeekDayScheduleRsp_t
/// Returns pass/fail of the command.
#define COMMAND_DOOR_LOCK_CLEAR_WEEKDAY_SCHEDULE_RESPONSE       0x0D // O  status field
/// Returns pass/fail of the command.
#define COMMAND_DOOR_LOCK_SET_YEAR_DAY_SCHEDULE_RESPONSE         0x0E // O  status field
/// Returns the weekly repeating schedule data for the specified schedule ID.
#define COMMAND_DOOR_LOCK_GET_YEAR_DAY_SCHEDULE_RESPONSE         0x0F // O  zclDoorLockGetYearDayScheduleRsp_t
/// Returns pass/fail of the command.
#define COMMAND_DOOR_LOCK_CLEAR_YEAR_DAY_SCHEDULE_RESPONSE       0x10 // O  status field
/// Returns pass/fail of the command.
#define COMMAND_DOOR_LOCK_SET_HOLIDAY_SCHEDULE_RESPONSE          0x11 // O  status field
/// Returns the Holiday Schedule Entry for the specified Holiday ID.
#define COMMAND_DOOR_LOCK_GET_HOLIDAY_SCHEDULE_RESPONSE          0x12 // O  zclDoorLockGetHolidayScheduleRsp_t
/// Returns pass/fail of the command.
#define COMMAND_DOOR_LOCK_CLEAR_HOLIDAY_SCHEDULE_RESPONSE        0x13 // O  status field
/// Returns the pass or fail value for the setting of the user type.
#define COMMAND_DOOR_LOCK_SET_USER_TYPE_RESPONSE                 0x14 // O  status field
/// Returns the user type for the specified user ID.
#define COMMAND_DOOR_LOCK_GET_USER_TYPE_RESPONSE                 0x15 // O  zclDoorLockGetUserTypeRsp_t
/// Returns status of the Set RFID Code command.
#define COMMAND_DOOR_LOCK_SET_RFID_CODE_RESPONSE                 0x16 // O  status field
/// Returns the RFID code for the specified user ID.
#define COMMAND_DOOR_LOCK_GET_RFID_CODE_RESPONSE                 0x17 // O  zclDoorLockGetRFIDCodeRsp_t
/// Returns pass/fail of the command.
#define COMMAND_DOOR_LOCK_CLEAR_RFID_CODE_RESPONSE               0x18 // O  status field
/// Returns pass/fail of the command.
#define COMMAND_DOOR_LOCK_CLEAR_ALL_RFID_CODES_RESPONSE          0x19 // O  status field
/// The door lock server sends out operation event notification when the event
/// is triggered by the various event sources.
#define COMMAND_DOOR_LOCK_OPERATING_EVENT_NOTIFICATION      0x20 // O  zclDoorLockOperationalEventNotification_t
/// The door lock optionally sends out notifications (if they are enabled)
/// whenever there is a significant operational event on the lock.
#define COMMAND_DOOR_LOCK_PROGRAMMING_EVENT_NOTIFICATION    0x21 // O  zclDoorLockProgrammingEventNotification_t


/*** User Status Value enums ***/
/// The following User Status and User Type values are used in the payload
/// of multiple commands.
#define USER_STATUS_AVAILABLE                                   0x00
#define USER_STATUS_OCCUPIED_ENABLED                            0x01
#define USER_STATUS_RESERVED                                    0x02
#define USER_STATUS_OCCUPIED_DISABLED                           0x03

/*** User Type Value enums ***/
/// Used to indicate what the type is for a specific user ID.
#define USER_TYPE_UNRESTRICTED_USER                             0x00 // default
#define USER_TYPE_YEAR_DAY_SCHEDULE_USER                        0x01
#define USER_TYPE_WEEK_DAY_SCHEDULE_USER                        0x02
#define USER_TYPE_MASTER_USER                                   0x03

/*** Operation (Programming) Event Source Value enums ***/
/// A source value where available sources are
#define OPERATION_EVENT_SOURCE_KEYPAD                           0x00
#define OPERATION_EVENT_SOURCE_RF                               0x01
#define OPERATION_EVENT_SOURCE_MANUAL                           0x02   // "Reserved" for Programming Event
#define OPERATION_EVENT_SOURCE_RFID                             0x03
#define OPERATION_EVENT_SOURCE_INDETERMINATE                    0xFF

/*** Operation Event Code Value enums ***/
/// Operation Event Code Value enum
#define OPERATION_EVENT_CODE_UNKNOWN_OR_MFG_SPECIFIC            0x00 // Applicable: Keypad, RF, Manual, RFID
#define OPERATION_EVENT_CODE_LOCK                               0x01 // Applicable: Keypad, RF, Manual, RFID
#define OPERATION_EVENT_CODE_UNLOCK                             0x02 // Applicable: Keypad, RF, Manual, RFID
#define OPERATION_EVENT_CODE_LOCK_FAILURE_INVALID_PIN_OR_ID     0x03 // Applicable: Keypad, RF, RFID
#define OPERATION_EVENT_CODE_LOCK_FAILURE_INVALID_SCHEDULE      0x04 // Applicable: Keypad, RF, RFID
#define OPERATION_EVENT_CODE_UNLOCK_FAILURE_INVALID_PIN_OR_ID   0x05 // Applicable: Keypad, RF, RFID
#define OPERATION_EVENT_CODE_UNLOCK_FAILURE_INVALID_SCHEDULE    0x06 // Applicable: Keypad, RF, RFID
#define OPERATION_EVENT_CODE_ONE_TOUCH_LOCK                     0x07 // Applicable: Manual
#define OPERATION_EVENT_CODE_KEY_LOCK                           0x08 // Applicable: Manual
#define OPERATION_EVENT_CODE_KEY_UNLOCK                         0x09 // Applicable: Manual
#define OPERATION_EVENT_CODE_AUTO_LOCK                          0x0A // Applicable: Manual
#define OPERATION_EVENT_CODE_SCHEDULE_LOCK                      0x0B // Applicable: Manual
#define OPERATION_EVENT_CODE_SCHEDULE_UNLOCK                    0x0C // Applicable: Manual
#define OPERATION_EVENT_CODE_MANUAL_LOCK                        0x0D // Applicable: Manual
#define OPERATION_EVENT_CODE_MANUAL_UNLOCK                      0x0E // Applicable: Manual

/*** Programming Event Code enums ***/
/// Programming Event Code enums
#define PROGRAMMING_EVENT_CODE_UNKNOWN_OR_MFG_SPECIFIC          0x00 // Applicable: Keypad, RF, RFID
#define PROGRAMMING_EVENT_CODE_MASTER_CODE_CHANGED              0x01 // Applicable: Keypad
#define PROGRAMMING_EVENT_CODE_PIN_CODE_ADDED                   0x02 // Applicable: Keypad, RF
#define PROGRAMMING_EVENT_CODE_PIN_CODE_DELETED                 0x03 // Applicable: Keypad, RF
#define PROGRAMMING_EVENT_CODE_PIN_CODE_CHANGED                 0x04 // Applicable: Keypad, RF
#define PROGRAMMING_EVENT_CODE_RFID_CODE_ADDED                  0x05 // Applicable: RFID
#define PROGRAMMING_EVENT_CODE_RFID_CODE_DELETED                0x06 // Applicable: RFID

/// Door Lock cluster commands payload lengths
#define DOORLOCK_RES_PAYLOAD_LEN                                0x01
#define PAYLOAD_LEN_UNLOCK_TIMEOUT   2
#define PAYLOAD_LEN_GET_LOG_RECORD    2
#define PAYLOAD_LEN_SET_PIN_CODE    4 // not including pPIN
#define PAYLOAD_LEN_USER_ID   2
#define PAYLOAD_LEN_SET_USER_STATUS   3
#define PAYLOAD_LEN_SET_WEEK_DAY_SCHEDULE   8
#define PAYLOAD_LEN_SCHEDULE    3
#define PAYLOAD_LEN_SET_YEAR_DAY_SCHEDULE   11
#define PAYLOAD_LEN_SET_HOLIDAY_SCHEDULE    10
#define PAYLOAD_LEN_HOLIDAY_SCHEDULE    1
#define PAYLOAD_LEN_SET_USER_TYPE   3
#define PAYLOAD_LEN_SET_RFID_CODE   4 // not including pRfidCode
#define PAYLOAD_LEN_GET_LOG_RECORD_RSP    11  // not including pPIN
#define PAYLOAD_LEN_GET_PIN_CODE_RSP    4 // not including pCode
#define PAYLOAD_LEN_GET_USER_STATUS_RSP   3
#define PAYLOAD_LEN_GET_USER_TYPE_RSP   3
#define PAYLOAD_LEN_GET_WEEK_DAY_SCHEDULE_RSP   9
#define PAYLOAD_LEN_GET_YEAR_DAY_SCHEDULE_RSP   12
#define PAYLOAD_LEN_GET_HOLIDAY_SCHEDULE_RSP    11
#define PAYLOAD_LEN_GET_RFID_CODE_RSP   4 // not including pRfidCode
#define PAYLOAD_LEN_OPERATION_EVENT_NOTIFICATION    9 // not including pData
#define PAYLOAD_LEN_PROGRAMMING_EVENT_NOTIFICATION    11 // not including pData
/** @} End CLOSURE_DOORLOCK_MACROS */

/**
* @defgroup CLOSURE_WINDOW_MACROS ZCL Closure Window Covering Cluster Macros
* @brief This group provides ZCL Closure Windo Covering Cluster macros
* defined in the ZCL v7 specification
* @{
*/
/**********************************************/
/*** Window Covering Cluster Attribute Sets ***/
/**********************************************/
#define ATTRSET_WINDOW_COVERING_INFO                        0x0000
#define ATTRSET_WINDOW_COVERING_SETTINGS                    0x0010

/******************************************/
/*** Window Covering Cluster Attributes ***/
/******************************************/
//Window Covering Information
/// The WindowCoveringType attribute identifies the type of window covering
/// being controlled by this endpoint
#define ATTRID_WINDOW_COVERING_WINDOW_COVERING_TYPE                                                0x0000
/// The PhysicalClosedLimitLift attribute identifies the maximum possible encoder
/// position possible (in centimeters) to position the height of the window
/// covering - this is ignored if the device is running in Open Loop Control.
#define ATTRID_WINDOW_COVERING_PHYSICAL_CLOSED_LIMIT_LIFT                                          0x0001
/// The PhysicalClosedLimitTilt attribute identifies the maximum possible
/// encoder position possible (tenth of a degrees) to position the angle of the
/// window covering - this is ignored if the device is running in Open Loop Control.
#define ATTRID_WINDOW_COVERING_PHYSICAL_CLOSED_LIMIT_TILT                                          0x0002
/// The CurrentPositionLift attribute identifies the actual position (in centimeters)
/// of the window covering from the top of the shade if Closed Loop Control is
/// enabled. This attribute is ignored if the device is running in Open Loop Control.
#define ATTRID_WINDOW_COVERING_CURRENT_POSITION_LIFT                                               0x0003
/// The NumberOfActuationsTilt attribute identifies the total number of tilt
/// actuations applied to the Window Covering since the device was installed.
#define ATTRID_WINDOW_COVERING_CURRENT_POSITION_TILT                                               0x0004
/// The NumberOfActuationsLift attribute identifies the total number of lift
/// actuations applied to the Window Covering since the device was installed.
#define ATTRID_WINDOW_COVERING_NUMBER_OF_ACTUATIONS_LIFT                                           0x0005
/// The NumberOfActuationsTilt attribute identifies the total number of tilt
/// actuations applied to the Window Covering since the device was installed.
#define ATTRID_WINDOW_COVERING_NUMBER_OF_ACTUATIONS_TILT                                           0x0006
/// The ConfigStatus attribute makes configuration and status information available.
#define ATTRID_WINDOW_COVERING_CONFIG_OR_STATUS                                                    0x0007
/// The CurrentPositionLiftPercentage attribute identifies the actual position as
/// a percentage between the InstalledOpenLimitLift attribute and the
/// InstalledClosedLimitLift attribute of the window covering from the up/open
/// position if Closed Loop Control is enabled.
#define ATTRID_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE                                    0x0008
/// The CurrentPositionTiltPercentage attribute identifies the actual position as
/// a percentage between the  InstalledOpenLimitTilt attribute and the
/// InstalledClosedLimitTilt attribute of the window covering from the up/open
/// position if Closed Loop Control is enabled.
#define ATTRID_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE                                    0x0009

//Window Covering Setting
/// The InstalledOpenLimitLift attribute identifies the Open Limit for Lifting
/// the Window Covering whether position (in centimeters) is encoded or timed.
#define ATTRID_WINDOW_COVERING_INSTALLED_OPEN_LIMIT_LIFT                                               0x0010
/// The InstalledClosedLimitLift attribute identifies the Closed Limit for
/// Lifting the Window Covering whether position (in centimeters) is encoded or timed.
#define ATTRID_WINDOW_COVERING_INSTALLED_CLOSED_LIMIT_LIFT                                             0x0011
/// The InstalledOpenLimitTilt attribute identifies the Open Limit for Tilting
/// the Window Covering whether position (in  tenth of a degree) is encoded or timed.
#define ATTRID_WINDOW_COVERING_INSTALLED_OPEN_LIMIT_TILT                                               0x0012
/// The InstalledClosedLimitTilt attribute identifies the Closed Limit for Tilting
/// the Window Covering whether position (in tenth of a degree) is encoded or timed.
#define ATTRID_WINDOW_COVERING_INSTALLED_CLOSED_LIMIT_TILT                                             0x0013
/// The VelocityLift attribute identifies the velocity (in centimeters per second)
/// associated with Lifting the Window Covering.
#define ATTRID_WINDOW_COVERING_VELOCITY_LIFT                                                           0x0014
/// The AccelerationTimeLift attribute identifies any ramp up times to reaching
/// the velocity setting (in tenth of a second) for positioning the Window Covering.
#define ATTRID_WINDOW_COVERING_ACCELERATION_TIME_LIFT                                                  0x0015
/// The DecelerationTimeLift attribute identifies any ramp down times associated
/// with stopping the positioning (in tenth of a second) of the Window Covering.
#define ATTRID_WINDOW_COVERING_DECELERATION_TIME_LIFT                                                  0x0016
/// The Mode attribute allows configuration of the Window Covering.
#define ATTRID_WINDOW_COVERING_MODE                                                                    0x0017
/// Identifies the number of Intermediate Setpoints supported by the Window Covering
/// for Lift and then identifies the position settings for those Intermediate
/// Setpoints if Closed Loop Control is supported.
#define ATTRID_WINDOW_COVERING_INTERMEDIATE_SETPOINTS_LIFT                                             0x0018
/// Identifies the number of Intermediate Setpoints supported by the Window
/// Covering for Tilt and then identifies the position settings for those
/// Intermediate Setpoints if Closed Loop Control is supported.
#define ATTRID_WINDOW_COVERING_INTERMEDIATE_SETPOINTS_TILT                                             0x0019

/*** Window Covering Type Attribute types ***/
/// Window Covering Type enum
#define CLOSURES_WINDOW_COVERING_TYPE_ROLLERSHADE                       0x00
#define CLOSURES_WINDOW_COVERING_TYPE_ROLLERSHADE_2_MOTOR               0x01
#define CLOSURES_WINDOW_COVERING_TYPE_ROLLERSHADE_EXTERIOR              0x02
#define CLOSURES_WINDOW_COVERING_TYPE_ROLLERSHADE_EXTERIOR_2_MOTOR      0x03
#define CLOSURES_WINDOW_COVERING_TYPE_DRAPERY                           0x04
#define CLOSURES_WINDOW_COVERING_TYPE_AWNING                            0x05
#define CLOSURES_WINDOW_COVERING_TYPE_SHUTTER                           0x06
#define CLOSURES_WINDOW_COVERING_TYPE_TILT_BLIND_TILT_ONLY              0x07
#define CLOSURES_WINDOW_COVERING_TYPE_TILT_BLIND_LIFT_AND_TILT          0x08
#define CLOSURES_WINDOW_COVERING_TYPE_PROJECTOR_SCREEN                  0x09


/****************************************/
/*** Window Covering Cluster Commands ***/
/****************************************/
/// Upon receipt of this command, the Window Covering will adjust the window so
/// the physical lift is at the  InstalledOpenLimit - Lift and the tilt is at the
/// InstalledOpenLimit - Tilt. This will happen as fast as possible.
#define COMMAND_WINDOW_COVERING_UP_OR_OPEN                            ( 0x00 )
/// Upon receipt of this command, the Window Covering will adjust the window so
/// the physical lift is at the InstalledClosedLimit - Lift and the tilt is at the
/// InstalledClosedLimit - Tilt. This will happen as fast as possible.
#define COMMAND_WINDOW_COVERING_DOWN_OR_CLOSE                         ( 0x01 )
/// Upon receipt of this command, the Window Covering will stop any adjusting to
/// the physical tilt and lift that is currently  occurring.
#define COMMAND_WINDOW_COVERING_STOP                               ( 0x02 )
/// Upon receipt of this command, the Window Covering will adjust the window so
/// the physical lift is at the lift value specified in the payload of this command
/// as long as that value is not larger than InstalledOpenLimit - Lift and not
/// smaller than InstalledClosedLimit - Lift.
#define COMMAND_WINDOW_COVERING_GO_TO_LIFT_VALUE                   ( 0x04 )
/// Upon receipt of this command, the Window Covering will adjust the window so
/// the physical lift is at the lift percentage specified in the payload of this command.
#define COMMAND_WINDOW_COVERING_GO_TO_LIFT_PERCENTAGE              ( 0x05 )
/// Upon receipt of this command, the Window Covering will adjust the window so
/// the physical tilt is at the tilt value specified in the payload of this command
/// as long as that value is not larger than InstalledOpenLimit - Tilt and not
/// smaller than InstalledClosedLimit - Tilt.
#define COMMAND_WINDOW_COVERING_GO_TO_TILT_VALUE                   ( 0x07 )
/// Upon receipt of this command, the Window Covering will adjust the window so
/// the physical tilt is at the tilt percentage specified in the payload of this command.
#define COMMAND_WINDOW_COVERING_GO_TO_TILT_PERCENTAGE              ( 0x08 )

#define ZCL_WC_GOTOVALUEREQ_PAYLOADLEN                      ( 2 )
#define ZCL_WC_GOTOPERCENTAGEREQ_PAYLOADLEN                 ( 1 )

  /** @} End CLOSURE_WINDOW_MACROS */

/*********************************************************************
 * TYPEDEFS
 */

/**
 * @brief Window Covering Cluster - Bits in Config/status attribute
 * 
 */
typedef struct s_zb_zcl_closures_wc_info_config_status
{
    uint8_t operational : 1;                // Window Covering is operational or not
    uint8_t online : 1;                     // Window Covering is enabled for transmitting over the Zigbee network or not
    uint8_t commands_reserved : 1;          // Identifies the direction of rotation for the Window Covering
    uint8_t lift_control : 1;               // Identifies lift control supports open loop or closed loop
    uint8_t tilt_control : 1;               // Identifies tilt control supports open loop or closed loop
    uint8_t lift_encoder_controlled : 1;    // Identifies lift control uses Timer or Encoder
    uint8_t tilt_encoder_controlled : 1;    // Identifies tilt control uses Timer or Encoder
    uint8_t reserved : 1;                   // Reserved for future use
} s_zb_zcl_closures_wc_info_config_status_t;

/**
 * @brief Window Covering Cluster - Bits in Mode attribute
 * 
 */
typedef struct s_zb_zcl_closures_wc_set_mode
{
    uint8_t motor_reverse_direction : 1;                // Define the direction of the motor rotation
    uint8_t run_in_calibration_mode : 1;                // Define Window Covering is in calibration mode or in normal mode
    uint8_t run_in_maintenance_mode : 1;                // Define Window Covering is in maintenance mode or in normal mode
    uint8_t led_feedback : 1;                           // Enables or Disables feedback LED
    uint8_t reserved : 4;                               // Reserved for future use
} s_zb_zcl_closures_wc_set_mode_t;

/**
 * @brief ZCL Door Lock Cluster - Server Commands Received structs
 * 
 */
typedef struct s_zb_door_lock_cmd
{
    uint8_t *pin_rfid_code;     // The PIN/RFID codes defined in this specification are all in ZCL OCTET STRING format
} s_zb_zcl_door_lock_t;

typedef struct s_zb_door_lock_unlock_timeout
{
    uint16_t timeout;           // The timeout value in seconds
    uint8_t *pin_rfid_code;     // The PIN/RFID codes defined in this specification are all in ZCL OCTET STRING format
} s_zb_zcl_door_lock_unlock_timeout_t;

typedef struct s_zb_door_lock_get_log_record
{
    uint16_t log_index;        // The index of the log record to retrieve
} s_zb_zcl_door_lock_get_log_record_t;

typedef struct s_zb_door_lock_set_pin_code
{
    uint16_t user_id;           // User ID is between 0 - [# of PIN Users Supported attribute].
    uint8_t user_status;        // Only the values 1 (Occupied/Enabled) and 3 (Occupied/Disabled) are allowed for User Status.
    uint8_t user_type;          // e.g. USER_TYPE_UNRESTRICTED_USER.
    uint8_t *pin;               // variable length string.
} s_zb_zcl_door_lock_set_pin_code_t;

typedef struct s_zb_door_lock_user_id
{
    uint16_t user_id;           // User ID is between 0 - [# of PIN Users Supported attribute].
} s_zb_zcl_door_lock_user_id_t;

typedef struct s_zb_door_lock_set_user_status
{
    uint16_t user_id;
    uint8_t user_status;
} s_zb_zcl_door_lock_set_user_status_t;

typedef struct s_zb_door_lock_set_week_day_schedule
{
    uint8_t schedule_id;    // number is between 0 - [# of Week Day Schedules Per User attribute].
    uint16_t user_id;       // is between 0 - [# of Total Users Supported attribute].
    uint8_t days_mask;      // bitmask of the effective days in the order XSFTWTMS.
    uint8_t start_hour;     // in decimal format represented by 0x00 - 0x17 (00 to 23 hours).
    uint8_t start_minute;   // in decimal format represented by 0x00 - 0x3B (00 to 59 mins).
    uint8_t end_hour;       // in decimal format represented by 0x00 - 0x17 (00 to 23 hours).
                            // End Hour SHALL be equal or greater 10212 than Start Hour.
    uint8_t end_minute;     // in decimal format represented by 0x00 - 0x3B (00 to 59 mins).
} s_zb_zcl_door_lock_set_week_day_schedule_t;

typedef struct s_zb_door_lock_schedule
{
    uint8_t schedule_id;
    uint16_t user_id;
} s_zb_zcl_door_lock_schedule_t;

typedef struct s_zb_door_lock_set_year_day_schedule
{
    uint8_t schedule_id;                // number is between 0 - [# of Week Day Schedules Per User attribute].
    uint16_t user_id;                   // is between 0 - [# of Total Users Supported attribute].
    uint32_t zigbee_local_start_time;   // Start time and end time are given in LocalTime.
    uint32_t zigbee_local_end_time;     // When the Server Device receives the command,
                                        // the Server Device MAY change the user type to the specific schedule user type.
} s_zb_zcl_door_lock_set_year_day_schedule_t;

typedef struct s_zb_door_lock_set_holiday_schedule
{
    uint8_t schedule_id;                    // Holiday Schedule ID number is between 0 - [# of Holiday Schedules Supported attribute].
    uint32_t zigbee_local_start_time;       // Start time and end time are given in LocalTime.
    uint32_t zigbee_local_end_time;         // End of Holiday time.
    uint8_t operating_mode_during_holiday;  // Operating Mode during Holiday.
} s_zb_zcl_door_lock_set_holiday_schedule_t;

typedef struct s_zb_door_lock_holiday_schedule_id
{
    uint8_t schedule_id;
} s_zb_zcl_door_lock_holiday_schedule_id_t;

typedef struct s_zb_door_lock_set_user_type
{
    uint16_t user_id;       // User ID is between 0 - [# of PIN Users Supported attribute].
    uint8_t user_type;      // e.g. USER_TYPE_UNRESTRICTED_USER.
} s_zb_zcl_door_lock_set_user_type_t;

typedef struct s_zb_door_lock_set_rfid_code
{
    uint16_t user_id;       // User ID is between 0 - [# of PIN Users Supported attribute].
    uint8_t user_status;    // Only the values 1 (Occupied/Enabled) and 3 (Occupied/Disabled) are allowed for User Status.
    uint8_t user_type;      // e.g. USER_TYPE_UNRESTRICTED_USER.
    uint8_t *rfid_code;     // The RFID codes defined in this specification are all in ZCL OCTET STRING format
} s_zb_zcl_door_lock_set_rfid_code_t;

/**
 * @brief ZCL Door Lock Cluster - Client commands received structs
 * 
 */
typedef struct s_zb_door_lock_get_log_record_rsp
{
    uint16_t log_index;         // The index of the log record to retrieve
    uint32_t timestamp;         // The timestamp of the log record
    uint8_t event_type;         // The type of the event
    uint8_t source;             // The source of the log entry
    uint8_t event_id_alarm_code;
    uint16_t user_id;
    uint8_t *pin;
} s_zb_zcl_door_lock_get_log_record_rsp_t;

typedef struct s_zb_door_lock_get_pin_code_rsp
{
    uint16_t user_id;
    uint8_t user_status;
    uint8_t user_type;
    uint8_t *code;
} s_zb_zcl_door_lock_get_pin_code_rsp_t;

typedef struct s_zb_door_lock_get_user_status_rsp
{
    uint16_t user_id;
    uint8_t user_status;
} s_zb_zcl_door_lock_get_user_status_rsp_t;

typedef struct s_zb_door_lock_get_user_type_rsp
{
    uint16_t user_id;
    uint8_t user_type;
} s_zb_zcl_door_lock_get_user_type_rsp_t;

typedef struct s_zb_door_lock_get_week_day_schedule_rsp
{
    uint8_t schedule_id;
    uint16_t user_id;
    uint8_t status;
    uint8_t days_mask;
    uint8_t start_hour;
    uint8_t start_minute;
    uint8_t end_hour;
    uint8_t end_minute;
} s_zb_zcl_door_lock_get_week_day_schedule_rsp_t;

typedef struct s_zb_door_lock_get_year_day_schedule_rsp
{
    uint8_t schedule_id;
    uint16_t user_id;
    uint8_t status;
    uint32_t zigbee_local_start_time;
    uint32_t zigbee_local_end_time;
} s_zb_zcl_door_lock_get_year_day_schedule_rsp_t;

typedef struct s_zb_door_lock_get_holiday_schedule_rsp
{
    uint8_t schedule_id;
    uint8_t status;
    uint32_t zigbee_local_start_time;
    uint32_t zigbee_local_end_time;
    uint8_t operating_mode_during_holiday;
} s_zb_zcl_door_lock_get_holiday_schedule_rsp_t;

typedef struct s_zb_door_lock_get_rfid_code_rsp
{
    uint16_t user_id;
    uint8_t user_status;
    uint8_t user_type;
    uint8_t *rfid_code;
} s_zb_zcl_door_lock_get_rfid_code_rsp_t;

typedef struct s_zb_door_lock_operating_event_notification
{
    uint8_t event_source;           // This field indicates where the event was triggered from.
    uint8_t event_code;             // significant operation event on the lock.
    uint16_t user_id;               // The User ID who performed the event.
    uint8_t pin;                    // The PIN that is associated with the User ID who performed the event.
    uint32_t zigbee_local_time;     // The LocalTime that indicates when the event is triggered.
    uint8_t *data;                  // The operation event notification command contains a variable string
} s_zb_zcl_door_lock_operating_event_notification_t;

typedef struct s_zb_door_lock_programming_event_notification
{
    uint8_t event_source;           // This field indicates where the event was triggered from.
    uint8_t event_code;             // significant programming  event on the lock.
    uint16_t user_id;               // The User ID who performed the event.
    uint8_t pin;                    // The PIN that is associated with the User ID who performed the event.
    uint8_t user_type;              // The User Type that is associated with the User ID who performed the event.
    uint8_t user_status;            // The User Status that is associated with the User ID who performed the event.
    uint32_t zigbee_local_time;     // The LocalTime that indicates when the event is triggered.
    uint8_t *data;                  // The programming event notification command contains a variable string
} s_zb_zcl_door_lock_programming_event_notification_t;

/// This callback is called to process an incoming Door Lock Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Lock Door Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_lock_door_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Unlock Door Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_unlock_door_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Toggle Door Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_toggle_door_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Unlock With Timeout Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_unlock_with_timeout_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Get Log Record Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_get_log_record_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, s_zb_zcl_door_lock_get_log_record_rsp_t *pCmd );

/// This callback is called to process an incoming Set PIN Code Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_set_pin_code_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Get PIN Code Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_get_pin_code_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, s_zb_zcl_door_lock_get_pin_code_rsp_t *pCmd );

/// This callback is called to process an incoming Clear PIN Code Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_clear_pin_code_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Clear All PIN Codes Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_clear_all_pin_codes_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Set User Status Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_set_user_status_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Get User Status Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_get_user_status_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, s_zb_zcl_door_lock_get_user_status_rsp_t *pCmd );

/// This callback is called to process an incoming Set Week Day Schedule Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_set_week_day_schedule_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Get Week Day Schedule Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_get_week_day_schedule_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, s_zb_zcl_door_lock_get_week_day_schedule_rsp_t *pCmd );

/// This callback is called to process an incoming Clear Week Day Schedule Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_clear_week_day_schedule_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Set Year Day Schedule Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_set_year_day_schedule_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Get Year Day Schedule Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_get_year_day_schedule_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, s_zb_zcl_door_lock_get_year_day_schedule_rsp_t *pCmd );

/// This callback is called to process an incoming Clear Year Day Schedule Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_clear_year_day_schedule_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Set Holiday Schedule Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_set_holiday_schedule_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Get Holiday Schedule Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_get_holiday_schedule_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, s_zb_zcl_door_lock_get_holiday_schedule_rsp_t *pCmd );

/// This callback is called to process an incoming Clear Holiday Schedule Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_clear_holiday_schedule_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Set User Type Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_set_user_type_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Get User Type Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_get_user_type_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, s_zb_zcl_door_lock_get_user_type_rsp_t *pCmd );

/// This callback is called to process an incoming Set RFID Code Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_set_rfid_code_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Get RFID Code Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_get_rfid_code_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, s_zb_zcl_door_lock_get_rfid_code_rsp_t *pCmd );

/// This callback is called to process an incoming Clear RFID Code Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_clear_rfid_code_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Clear All RFID Codes Response command
typedef zb_status_t (*pfn_zcl_closures_door_lock_clear_all_rfid_codes_rsp_t) ( s_zb_zcl_incoming_msg_t *pInMsg, uint8_t status );

/// This callback is called to process an incoming Operation Event Notification command
typedef zb_status_t (*pfn_zcl_closures_door_lock_operation_event_notification_t) ( s_zb_zcl_incoming_msg_t *pInMsg, s_zb_zcl_door_lock_operating_event_notification_t *pCmd );

/// This callback is called to process an incoming Programming Event Notification command
typedef zb_status_t (*pfn_zcl_closures_door_lock_programming_event_notification_t) ( s_zb_zcl_incoming_msg_t *pInMsg, s_zb_zcl_door_lock_programming_event_notification_t *pCmd );


/**
 * @brief Register Callbacks DoorLock Cluster table entry - enter function pointers for callbacks that
 * the application would like to receive
 */
 typedef struct
 {
   pfn_zcl_closures_door_lock_rsp_t                             pfn_door_lock_lock_door_rsp;                     //!< (COMMAND_DOOR_LOCK_LOCK_DOOR_RESPONSE)
   pfn_zcl_closures_door_lock_unlock_with_timeout_rsp_t         pfn_door_lock_unlock_with_timeout_rsp;           //!< (COMMAND_DOOR_LOCK_UNLOCK_WITH_TIMEOUT_RESPONSE)
   pfn_zcl_closures_door_lock_get_log_record_rsp_t              pfn_door_lock_get_log_record_rsp;                //!< (COMMAND_DOOR_LOCK_GET_LOG_RECORD_RESPONSE)
   pfn_zcl_closures_door_lock_set_pin_code_rsp_t                pfn_door_lock_set_pin_code_rsp;                  //!< (COMMAND_DOOR_LOCK_SET_PIN_CODE_RESPONSE)
   pfn_zcl_closures_door_lock_get_pin_code_rsp_t                pfn_door_lock_get_pin_code_rsp;                  //!< (COMMAND_DOOR_LOCK_GET_PIN_CODE_RESPONSE)
   pfn_zcl_closures_door_lock_clear_pin_code_rsp_t              pfn_door_lock_clear_pin_code_rsp;                //!< (COMMAND_DOOR_LOCK_CLEAR_PIN_CODE_RESPONSE)
   pfn_zcl_closures_door_lock_clear_all_pin_codes_rsp_t         pfn_door_lock_clear_all_pin_codes_rsp;           //!< (COMMAND_DOOR_LOCK_CLEAR_ALL_PIN_CODES_RESPONSE)
   pfn_zcl_closures_door_lock_set_user_status_rsp_t             pfn_door_lock_set_user_status_rsp;               //!< (COMMAND_DOOR_LOCK_SET_USER_STATUS_RESPONSE)
   pfn_zcl_closures_door_lock_get_user_status_rsp_t             pfn_door_lock_get_user_status_rsp;               //!< (COMMAND_DOOR_LOCK_GET_USER_STATUS_RESPONSE)
   pfn_zcl_closures_door_lock_set_week_day_schedule_rsp_t       pfn_door_lock_set_week_day_schedule_rsp;         //!< (COMMAND_DOOR_LOCK_SET_WEEKDAY_SCHEDULE_RESPONSE)
   pfn_zcl_closures_door_lock_get_week_day_schedule_rsp_t       pfn_door_lock_get_week_day_schedule_rsp;         //!< (COMMAND_DOOR_LOCK_GET_WEEKDAY_SCHEDULE_RESPONSE)
   pfn_zcl_closures_door_lock_clear_week_day_schedule_rsp_t     pfn_door_lock_clear_week_day_schedule_rsp;       //!< (COMMAND_DOOR_LOCK_CLEAR_WEEKDAY_SCHEDULE_RESPONSE)
   pfn_zcl_closures_door_lock_set_year_day_schedule_rsp_t       pfn_door_lock_set_year_day_schedule_rsp;         //!< (COMMAND_DOOR_LOCK_SET_YEAR_DAY_SCHEDULE_RESPONSE)
   pfn_zcl_closures_door_lock_get_year_day_schedule_rsp_t       pfn_door_lock_get_year_day_schedule_rsp;         //!< (COMMAND_DOOR_LOCK_GET_YEAR_DAY_SCHEDULE_RESPONSE)
   pfn_zcl_closures_door_lock_clear_year_day_schedule_rsp_t     pfn_door_lock_clear_year_day_schedule_rsp;       //!< (COMMAND_DOOR_LOCK_CLEAR_YEAR_DAY_SCHEDULE_RESPONSE)
   pfn_zcl_closures_door_lock_set_holiday_schedule_rsp_t        pfn_door_lock_set_holiday_schedule_rsp;          //!< (COMMAND_DOOR_LOCK_SET_HOLIDAY_SCHEDULE_RESPONSE)
   pfn_zcl_closures_door_lock_get_holiday_schedule_rsp_t        pfn_door_lock_get_holiday_schedule_rsp;          //!< (COMMAND_DOOR_LOCK_GET_HOLIDAY_SCHEDULE_RESPONSE)
   pfn_zcl_closures_door_lock_clear_holiday_schedule_rsp_t      pfn_door_lock_clear_holiday_schedule_rsp;        //!< (COMMAND_DOOR_LOCK_CLEAR_HOLIDAY_SCHEDULE_RESPONSE)
   pfn_zcl_closures_door_lock_set_user_type_rsp_t               pfn_door_lock_set_user_type_rsp;                 //!< (COMMAND_DOOR_LOCK_SET_USER_TYPE_RESPONSE)
   pfn_zcl_closures_door_lock_get_user_type_rsp_t               pfn_door_lock_get_user_type_rsp;                 //!< (COMMAND_DOOR_LOCK_GET_USER_TYPE_RESPONSE)
   pfn_zcl_closures_door_lock_set_rfid_code_rsp_t               pfn_door_lock_set_rfid_code_rsp;                 //!< (COMMAND_DOOR_LOCK_SET_RFID_CODE_RESPONSE)
   pfn_zcl_closures_door_lock_get_rfid_code_rsp_t               pfn_door_lock_get_rfid_code_rsp;                 //!< (COMMAND_DOOR_LOCK_GET_RFID_CODE_RESPONSE)
   pfn_zcl_closures_door_lock_clear_rfid_code_rsp_t             pfn_door_lock_clear_rfid_code_rsp;               //!< (COMMAND_DOOR_LOCK_CLEAR_RFID_CODE_RESPONSE)
   pfn_zcl_closures_door_lock_clear_all_rfid_codes_rsp_t        pfn_door_lock_clear_all_rfid_codes_rsp;          //!< (COMMAND_DOOR_LOCK_CLEAR_ALL_RFID_CODES_RESPONSE)
   pfn_zcl_closures_door_lock_operation_event_notification_t    pfn_door_lock_operating_event_notification;      //!< (COMMAND_DOOR_LOCK_OPERATING_EVENT_NOTIFICATION)
   pfn_zcl_closures_door_lock_programming_event_notification_t  pfn_door_lock_programming_event_notification;    //!< (COMMAND_DOOR_LOCK_PROGRAMMING_EVENT_NOTIFICATION)
 } s_zb_zcl_closures_door_lock_app_callbacks_t;

/*********************************************************************
 * FUNCTIONS
 */
/**
 * @defgroup ZCL_CLOSURE_FUNCTIONS ZCL Closure Functions
 * @{
 * @brief This group defines the functions for Closure devices
 */

/**
 * @brief ZCL Door Lock Cluster CLient Commands
 * 
 */

/**
 * @brief Register Callbacks for Door Lock Cluster commands
 * 
 * @param[in] endpoint Endpoint
 * @param[in] callbacks Callbacks
 * @return zb_status_t Status of the registration
 */
zb_status_t zb_zcl_closures_register_door_lock_cmd_callbacks(
    uint8_t endpoint, s_zb_zcl_closures_door_lock_app_callbacks_t *callbacks);

/**
 * @brief Send a Door Lock request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cmd COMMAND_DOOR_LOCK_LOCK_DOOR, COMMAND_DOOR_LOCK_UNLOCK_DOOR, COMMAND_DOOR_LOCK_TOGGLE
 * @param[in] payload aPinRfidCode - PIN/RFID code in ZCL Octet String Format
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint8_t cmd,
    s_zb_zcl_door_lock_t *payload, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Door Lock Unlock With Timeout request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] payload
 *        aPinRfidCode - PIN/RFID code in ZCL Octet String Format
 *        timeout - Timeout in seconds
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_unlock_with_timeout_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_door_lock_unlock_timeout_t *payload, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Door Lock Get Log Record request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] log_index Log number between 1 - [max log attribute]
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_get_log_record_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t log_index, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Door Lock Set Pin Code request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] payload
 *        user_id - User ID is between 0 - [# of PIN Users Supported attribute].
 *        user_status - Used to indicate what the status is for a specific User ID
 *        user_type - Used to indicate what the type is for a specific User ID
 *        pin - A ZigBee string indicating the PIN code used to create the event on the door lock
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_set_pin_code_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_door_lock_set_pin_code_t *payload, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Door Lock User ID request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cmd COMMAND_DOOR_LOCK_GET_PIN_CODE, COMMAND_DOOR_LOCK_CLEAR_PIN_CODE,
 *              COMMAND_DOOR_LOCK_GET_USER_STATUS, COMMAND_DOOR_LOCK_GET_USER_TYPE,
 *              COMMAND_DOOR_LOCK_GET_RFID_CODE, COMMAND_DOOR_LOCK_CLEAR_RFID_CODE
 * @param[in] user_id User ID is between 0 - [# of PIN Users Supported attribute].
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_user_id_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint8_t cmd,
    uint16_t user_id, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Door Lock Clear All Codes request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cmd COMMAND_DOOR_LOCK_CLEAR_ALL_PIN_CODES, COMMAND_DOOR_LOCK_CLEAR_ALL_RFID_CODES
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_clear_all_codes_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Door Lock Set User Status request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] user_id User ID is between 0 - [# of PIN Users Supported attribute].
 * @param[in] user_status Used to indicate what the status is for a specific User ID
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_set_user_status_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t user_id, uint8_t user_status, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Door Lock Set Week Day Schedule request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] schedule_id Schedule ID is between 0 - [# of Week Day Schedules Per User attribute].
 * @param[in] user_id User ID is between 0 - [# of Total Users Supported attribute].
 * @param[in] days_mask Days mask is a bitmask of the effective days in the order XSFTWTMS.
 * @param[in] start_hour Start hour is in decimal format represented by 0x00 - 0x17 (00 to 23 hours).
 * @param[in] start_minute Start minute is in decimal format represented by 0x00 - 0x3B (00 to 59 mins).
 * @param[in] end_hour End hour is in decimal format represented by 0x00 - 0x17 (00 to 23 hours).
 * @param[in] end_minute End minute is in decimal format represented by 0x00 - 0x3B (00 to 59 mins).
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_set_week_day_schedule_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t schedule_id, uint16_t user_id, uint8_t days_mask,
    uint8_t start_hour, uint8_t start_minute, uint8_t end_hour,
    uint8_t end_minute, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Door Lock Schedule request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cmd COMMAND_DOOR_LOCK_GET_WEEKDAY_SCHEDULE, COMMAND_DOOR_LOCK_CLEAR_WEEKDAY_SCHEDULE,
 *              COMMAND_DOOR_LOCK_GET_YEAR_DAY_SCHEDULE, COMMAND_DOOR_LOCK_CLEAR_YEAR_DAY_SCHEDULE
 * @param[in] schedule_id Schedule ID is between 0 - [# of Week Day Schedules Per User attribute].
 * @param[in] user_id User ID is between 0 - [# of Total Users Supported attribute].
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_schedule_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint8_t cmd,
    uint8_t schedule_id, uint16_t user_id, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Door Lock Set Year Day Schedule request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] schedule_id Schedule ID is between 0 - [# of Week Day Schedules Per User attribute].
 * @param[in] user_id User ID is between 0 - [# of Total Users Supported attribute].
 * @param[in] zigbee_local_start_time Start time of the Year Day Schedule representing by ZigBeeLocalTime
 * @param[in] zigbee_local_end_time End time of the Year Day Schedule representing by ZigBeeLocalTime
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_set_year_day_schedule_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t schedule_id, uint16_t user_id, uint32_t zigbee_local_start_time,
    uint32_t zigbee_local_end_time, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Door Lock Set Holiday Schedule request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] schedule_id A unique ID for given Holiday Schedule (0 to 254)
 * @param[in] zigbee_local_start_time Start time of the Year Day Schedule representing by ZigBeeLocalTime
 * @param[in] zigbee_local_end_time End time of the Year Day Schedule representing by ZigBeeLocalTime
 * @param[in] operating_mode_during_holiday A valid enumeration value as listed in operating mode attribute
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_set_holiday_schedule_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t schedule_id, uint32_t zigbee_local_start_time, uint32_t zigbee_local_end_time,
    uint8_t operating_mode_during_holiday, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Door Lock Holiday Schedule request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cmd COMMAND_DOOR_LOCK_GET_HOLIDAY_SCHEDULE, COMMAND_DOOR_LOCK_CLEAR_HOLIDAY_SCHEDULE
 * @param[in] schedule_id A unique ID for given Holiday Schedule (0 to 254)
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_holiday_schedule_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint8_t cmd,
    uint8_t schedule_id, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Door Lock Set User Type request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] user_id User ID is between 0 - [# of PIN Users Supported attribute].
 * @param[in] user_type User Type is a valid enumeration value as listed in user type attribute
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_set_user_type_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint16_t user_id,
    uint8_t user_type, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Door Lock Set RFID Code request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] payload Payload
 * @param[in] payload->user_id User ID is between 0 - [# of PIN Users Supported attribute].
 * @param[in] payload->user_status Used to indicate what the status is for a specific User ID
 * @param[in] payload->user_type Used to indicate what the type is for a specific User ID
 * @param[in] payload->rfid_code A ZigBee string indicating the RFID code used to create the event
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_send_door_lock_set_rfid_code_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_door_lock_set_rfid_code_t *payload,
    uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief ZCL Window Covering Cluster Client Commands
 * 
 */

/**
 * @brief Send a Window Covering Simple request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cmd Command to send
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_window_covering_simple_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Window Covering Send Go To Value request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cmd Command for COMMAND_WINDOW_COVERING_GO_TO_LIFT_VALUE
 * @param[in] value Value to send
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_window_covering_send_goto_value_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint8_t cmd,
    uint16_t value, uint8_t disable_default_rsp, uint8_t seq_num );

/**
 * @brief Send a Window Covering Send Go To Percentage request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cmd Command for COMMAND_WINDOW_COVERING_GO_TO_LIFT_PERCENTAGE
 * @param[in] percentage_value Percentage value to send
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t zb_zcl_closures_window_covering_send_goto_percentage_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint8_t cmd,
    uint8_t percentage_value, uint8_t disable_default_rsp, uint8_t seq_num );

/*********************************************************************
 * FUNCTION MACROS
 */

#define zb_zcl_closures_send_door_lock_lock_door(src_ep, dst_addr, payload, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_LOCK_DOOR, payload, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_unlock_door(src_ep, dst_addr, payload, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_UNLOCK_DOOR, payload, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_toggle(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_TOGGLE, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_get_pin_code(src_ep, dst_addr, user_id, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_user_id_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_GET_PIN_CODE, user_id, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_clear_pin_code(src_ep, dst_addr, user_id, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_user_id_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_CLEAR_PIN_CODE, user_id, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_clear_all_pin_codes(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_clear_all_codes_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_CLEAR_ALL_PIN_CODES, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_get_user_status(src_ep, dst_addr, user_id, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_user_id_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_GET_USER_STATUS, user_id, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_get_weekday_schedule(src_ep, dst_addr, schedule_id, user_id, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_schedule_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_GET_WEEKDAY_SCHEDULE, schedule_id, user_id, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_clear_weekday_schedule(src_ep, dst_addr, schedule_id, user_id, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_schedule_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_CLEAR_WEEKDAY_SCHEDULE, schedule_id, user_id, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_get_year_day_schedule(src_ep, dst_addr, schedule_id, user_id, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_schedule_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_GET_YEAR_DAY_SCHEDULE, schedule_id, user_id, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_clear_year_day_schedule(src_ep, dst_addr, schedule_id, user_id, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_schedule_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_CLEAR_YEAR_DAY_SCHEDULE, schedule_id, user_id, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_get_holiday_schedule(src_ep, dst_addr, schedule_id, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_holiday_schedule_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_GET_HOLIDAY_SCHEDULE, schedule_id, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_clear_holiday_schedule(src_ep, dst_addr, schedule_id, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_holiday_schedule_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_CLEAR_HOLIDAY_SCHEDULE, schedule_id, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_get_user_type(src_ep, dst_addr, user_id, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_user_id_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_GET_USER_TYPE, user_id, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_get_rfid_code(src_ep, dst_addr, user_id, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_user_id_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_GET_RFID_CODE, user_id, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_clear_rfid_code(src_ep, dst_addr, user_id, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_user_id_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_CLEAR_RFID_CODE, user_id, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_door_lock_clear_all_rfid_codes(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_closures_send_door_lock_clear_all_codes_request(src_ep, dst_addr, \
        COMMAND_DOOR_LOCK_CLEAR_ALL_RFID_CODES, disable_default_rsp, seq_num)

/**
 * @brief ZCL Window Covering Cluster Client Commands
 * 
 */
#define zb_zcl_closures_send_up_open(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_closures_window_covering_simple_request(src_ep, dst_addr, \
        COMMAND_WINDOW_COVERING_UP_OR_OPEN, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_down_close(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_closures_window_covering_simple_request(src_ep, dst_addr, \
        COMMAND_WINDOW_COVERING_DOWN_OR_CLOSE, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_stop(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_closures_window_covering_simple_request(src_ep, dst_addr, \
        COMMAND_WINDOW_COVERING_STOP, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_goto_lift_value(src_ep, dst_addr, lift_value, disable_default_rsp, seq_num) \
    zb_zcl_closures_window_covering_send_goto_value_request(src_ep, dst_addr, \
        COMMAND_WINDOW_COVERING_GO_TO_LIFT_VALUE, lift_value, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_goto_lift_percentage(src_ep, dst_addr, percentage_value, disable_default_rsp, seq_num) \
    zb_zcl_closures_window_covering_send_goto_percentage_request(src_ep, dst_addr, \
        COMMAND_WINDOW_COVERING_GO_TO_LIFT_PERCENTAGE, percentage_value, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_goto_tilt_value(src_ep, dst_addr, tilt_value, disable_default_rsp, seq_num) \
    zb_zcl_closures_window_covering_send_goto_value_request(src_ep, dst_addr, \
        COMMAND_WINDOW_COVERING_GO_TO_TILT_VALUE, tilt_value, disable_default_rsp, seq_num)

#define zb_zcl_closures_send_goto_tilt_percentage(src_ep, dst_addr, tilt_percentage, disable_default_rsp, seq_num) \
    zb_zcl_closures_window_covering_send_goto_percentage_request(src_ep, dst_addr, \
        COMMAND_WINDOW_COVERING_GO_TO_TILT_PERCENTAGE, tilt_percentage, disable_default_rsp, seq_num)
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZCL_CLOSURES_H_ */