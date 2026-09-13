/*
 * zb_zcl_general.h
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_ZCL_GENERAL_H_
#define ZB_ZCL_GENERAL_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "zcl/zb_zcl.h"

/*********************************************************************
 * CONSTANTS
 */
 #define ZCL_SCENE_NAME_LEN                                16

 //According to Cluster Library 07-5123 Revision 7
 #define BASIC_ZCL_VERSION                                 0x03
 
 /**
  * @defgroup ZCL_GENERAL_BASIC_CLUSTER ZCL Basic Cluster
  * @{
  */
/********************************/
/*** Basic Cluster Attributes ***/
/********************************/
// Basic Device Information
/// The ZCLVersion attribute represents a published set of foundation items,
/// such as global commands and functional descriptions.
#define ATTRID_BASIC_ZCL_VERSION                          0x0000
/// The ApplicationVersion attribute is 8 bits in length and specifies the
/// version number of the application software contained in the device.
#define ATTRID_BASIC_APPLICATION_VERSION                  0x0001
/// The StackVersion attribute is 8 bits in length and specifies the version
/// number of the implementation of the stack contained in the device.
#define ATTRID_BASIC_STACK_VERSION                        0x0002
/// The HWVersion attribute is 8 bits in length and specifies the version
/// number of the hardware of the device.
#define ATTRID_BASIC_HW_VERSION                           0x0003
/// The ManufacturerName attribute is a maximum of 32 bytes in length and
/// specifies the name of the manufacturer as a character string.
#define ATTRID_BASIC_MANUFACTURER_NAME                    0x0004
/// The ModelIdentifier attribute is a maximum of 32 bytes in length and
/// specifies the model number (or other identifier) assigned by the
/// manufacturer as a character string.
#define ATTRID_BASIC_MODEL_IDENTIFIER                     0x0005
///The DateCode attribute is a character string with a maximum length of
/// 16 bytes. The first 8 characters specify the date of manufacturer of
/// the device in international date notation according to ISO 8601.
#define ATTRID_BASIC_DATE_CODE                            0x0006
/// The PowerSource attribute is 8 bits in length and specifies the source(s)
/// of power available to the device.
#define ATTRID_BASIC_POWER_SOURCE                         0x0007
/// The GenericDeviceClass attribute defines the field of application of the
/// GenericDeviceType attribute.
#define ATTRID_BASIC_GENERIC_DEVICE_CLASS                       0x0008
/// The GenericDeviceType attribute allows an application to show an icon on
/// a rich user interface
#define ATTRID_BASIC_GENERIC_DEVICE_TYPE                        0x0009
/// The ProductCode attribute allows an application to specify a code for
/// the product.
#define ATTRID_BASIC_PRODUCT_CODE                         0x000A
/// The ProductURL attribute specifies a link to a web page containing
/// specific product information.
#define ATTRID_BASIC_PRODUCT_URL                          0x000B
/// Vendor specific human readable (displayable) string representing the
/// versions of one of more program images supported on the device.
#define ATTRID_BASIC_MANUFACTURER_VERSION_DETAILS         0x000C
/// Vendor specific human readable (displayable) serial number.
#define ATTRID_BASIC_SERIAL_NUMBER                        0x000D
/// Vendor specific human readable (displayable) product label.
#define ATTRID_BASIC_PRODUCT_LABEL                        0x000E
/// The LocationDescription attribute is a maximum of 16 bytes in length and
/// describes the physical location of the device  as a character string.
#define ATTRID_BASIC_LOCATION_DESCRIPTION                 0x0010
/// The PhysicalEnvironment attribute is 8 bits in length and specifies the
/// type of physical environment in which the device will operate.
#define ATTRID_BASIC_PHYSICAL_ENVIRONMENT                 0x0011
/// The DeviceEnabled attribute is a Boolean and specifies whether the device
/// is enabled or disabled.
#define ATTRID_BASIC_DEVICE_ENABLED                       0x0012
/// The AlarmMask attribute is 8 bits in length and specifies which of a
/// number of general alarms MAY be generated.
#define ATTRID_BASIC_ALARM_MASK                           0x0013
/// The DisableLocalConfig attribute allows a number of local device
/// configuration functions to be disabled.
#define ATTRID_BASIC_DISABLE_LOCAL_CONFIG                 0x0014
/// The SWBuildID attribute represents a detailed, manufacturer-specific
/// reference to the version of the software.
#define ATTRID_BASIC_SW_BUILD_ID                          0x4000


/*** Power Source Attribute values ***/
// Bits b0-b6 represent the primary power source of the device
#define POWER_SOURCE_UNKNOWN                              0x00
#define POWER_SOURCE_MAINS_1_PHASE                        0x01
#define POWER_SOURCE_MAINS_3_PHASE                        0x02
#define POWER_SOURCE_BATTERY                              0x03
#define POWER_SOURCE_DC                                   0x04
#define POWER_SOURCE_EMERG_MAINS_CONST_PWR                0x05
#define POWER_SOURCE_EMERG_MAINS_XFER_SW                  0x06
  // Bit b7 indicates whether the device has a secondary power source in the
  // form of a battery backup

/*** Power Source Attribute bits  ***/
#define POWER_SOURCE_PRIMARY                              0x7F
#define POWER_SOURCE_SECONDARY                            0x80

/*** Application Profile Type Attribute values ***/
#define APPLICATION_PROFILE_TYPE_ZIGBEE_BUILDING_AUTOMATION   0x00
#define APPLICATION_PROFILE_TYPE_ZIGBEE_REMOTE_CONTROL        0x01
#define APPLICATION_PROFILE_TYPE_ZIGBEE_SMART_ENERGY          0x02
#define APPLICATION_PROFILE_TYPE_ZIGBEE_HEALTH_CARE           0x03
#define APPLICATION_PROFILE_TYPE_ZIGBEE_HOME_AUTOMATION       0x04
#define APPLICATION_PROFILE_TYPE_ZIGBEE_INPUT_DEVICE          0x05
#define APPLICATION_PROFILE_TYPE_ZIGBEE_LIGHT_LINK            0X06
#define APPLICATION_PROFILE_TYPE_ZIGBEE_RETAIL_SERVICES       0x07
#define APPLICATION_PROFILE_TYPE_ZIGBEE_TELECOM_SERVICES      0X08

/*** Physical Environment Attribute values ***/
#define PHY_UNSPECIFIED_ENV            0x00
#define PHY_MIRROR_CAPACITY_ENV        0x01
#define PHY_BAR                        0x02
#define PHY_COURTYARD                  0x03
#define PHY_BATHROOM                   0x04
#define PHY_BEDROOM                    0x05
#define PHY_BILIARD_ROOM               0x06
#define PHY_UTILITY_TOOM               0x07
#define PHY_CELLAR                     0x08
#define PHY_STORAGE_CLOSET             0x09
#define PHY_THEATER                    0x0A
#define PHY_OFFICE                     0x0B
#define PHY_DECK                       0x0C
#define PHY_DEN                        0x0D
#define PHY_DINING_ROOM                0x0E
#define PHY_ELECTRICAL_ROOM            0x0F
#define PHY_ELEVTOR                    0x10
#define PHY_ENTRY                      0x11
#define PHY_FAMILLY_ROOM               0x12
#define PHY_MAIN_FLOOR                 0x13
#define PHY_UPSTAIRS                   0x14
#define PHY_DOWNSTAIRS                 0x15
#define PHY_BASEMENT_LOWER             0x16
#define PHY_GALLERY                    0x17
#define PHY_GAME_ROOM                  0x18
#define PHY_GARAGE                     0x19
#define PHY_GYM                        0x1A
#define PHY_HALLWAY                    0x1B
#define PHY_HOUSE                      0x1C
#define PHY_KITCHEN                    0x1D
#define PHY_LAUNDRY_ROOM               0x1E
#define PHY_LIBRARY                    0x1F
#define PHY_MASTER_BEDROOM             0x20
#define PHY_MUD_ROOM                   0x21
#define PHY_NURSERY                    0x22
#define PHY_PANTRY                     0x23
#define PHY_OFFICE2                    0x24
#define PHY_OUTSIDE                    0x25
#define PHY_POOL                       0x26
#define PHY_PORCH                      0x27
#define PHY_SEWING_ROOM                0x28
#define PHY_SITTING_ROOM               0x29
#define PHY_STAIRWAY                   0x2A
#define PHY_YARD                       0x2B
#define PHY_ATTIC                      0x2C
#define PHY_HOT_TUB                    0x2D
#define PHY_LIVING_ROOM                0x2E
#define PHY_SAUNA                      0x2F
#define PHY_SHOP_WORKSHOP              0x30
#define PHY_GUEST_BEDROOM              0x31
#define PHY_GUEST_BATH                 0x32
#define PHY_POWDER_ROOM                0x33
#define PHY_BACK_YARD                  0x34
#define PHY_FRONT_YARD                 0x35
#define PHY_PATIO                      0x36
#define PHY_DRIVEWAY                   0x37
#define PHY_SUN_ROOM                   0x38
#define PHY_LIVING_ROOM2               0x39
#define PHY_SPA                        0x3A
#define PHY_WIRLPOOL                   0x3B
#define PHY_SHED                       0x3C
#define PHY_EQUIPMENT_STORAGE          0x3D
#define PHY_HOBBY_CRAFT_ROOM           0x3E
#define PHY_FOUNTAIN                   0x3F
#define PHY_POND                       0x40
#define PHY_RECEPTION_ROOM             0x41
#define PHY_BREAKFAST_ROOM             0x42
#define PHY_NOOK                       0x43
#define PHY_GARDEN                     0x44
#define PHY_BALCONY                    0x45
#define PHY_PANIC_ROOM                 0x46
#define PHY_TERACE                     0x47
#define PHY_ROOF                       0x48
#define PHY_TOILET                     0x49
#define PHY_TOILET_MAIN                0x4A
#define PHY_OUTSIDE_TOILET             0x4B
#define PHY_SHOWER_ROOM                0x4C
#define PHY_STUDY                      0x4D
#define PHY_FRONT_GARDEN               0x4E
#define PHY_BACK_GARDEN                0x4F
#define PHY_KETTLE                     0x50
#define PHY_TELEVISION                 0x51
#define PHY_STOVE                      0x52
#define PHY_MICROWAVE                  0x53
#define PHY_TOASTER                    0x54
#define PHY_VACUUM                     0x55
#define PHY_APPLIANCE                  0x56
#define PHY_FRONT_DOOR                 0x57
#define PHY_BACK_DOOR                  0x58
#define PHY_FRIDGE_DOOR                0x59
#define PHY_MEDICATION_CABINET_DOOR    0x60
#define PHY_WARDROBE_DOOR              0x61
#define PHY_FRONT_CUPBOARD_DOOR        0x62
#define PHY_OTHER_DOOR                 0x63
#define PHY_WAITING_ROOM               0x64
#define PHY_TRAGE_ROOM                 0x65
#define PHY_DOCTORS_OFFICE             0x66
#define PHY_PATIENTS_PROVATE_ROOM      0x67
#define PHY_CONSULTATION_ROOM          0x68
#define PHY_NURSE_STATION              0x69
#define PHY_WARD                       0x6A
#define PHY_CORRIDOR                   0x6B
#define PHY_OPERATING_THEATHRE         0x6C
#define PHY_DENTAL_SURGERY_ROOM        0x6D
#define PHY_MEDICAL_IMAGING_ROOM       0x6E
#define PHY_DECONTAMINATION_ROOM       0x6F
#define PHY_ATRIUM                     0x70
#define PHY_MIRROR                     0x71
#define PHY_UNKNOWN_ENV                0xFF

/*** Device Enable Attribute values ***/
#define DEVICE_DISABLED                                   0x00
#define DEVICE_ENABLED                                    0x01

/*** Alarm Mask Attribute bits ***/
#define ALARM_MASK_GENERAL_HW_FAULT                           0x01
#define ALARM_MASK_GENERAL_SW_FAULT                           0x02

/******************************/
/*** Basic Cluster Commands ***/
/******************************/
/// On receipt of this command, the device resets all the attributes of all
/// its clusters to their factory defaults.
#define COMMAND_BASIC_RESET_TO_FACTORY_DEFAULTS                  0x00

/** @} End ZCL_GENERAL_BASIC_CLUSTER */

/**
* @defgroup ZCL_GENERAL_POWER_CONFIGURATION_CLUSTER ZCL Power Configuration Cluster
* @{
*/
/**********************************************/
/*** Power Configuration Cluster Attributes ***/
/**********************************************/
// Mains Information
/// The MainsVoltage attribute is 16 bits in length and specifies the actual
/// (measured) RMS voltage (or DC voltage in the 3501 case of a DC supply)
/// currently applied to the device, measured in units of 100mV.
#define ATTRID_POWER_CONFIG_MAINS_VOLTAGE                    0x0000
/// The MainsFrequency attribute is 8 bits in length and represents the
/// frequency, in Hertz
#define ATTRID_POWER_CONFIG_MAINS_FREQUENCY                  0x0001

// Mains Settings
/// The MainsAlarmMask attribute is 8 bits in length and specifies which
/// mains alarms MAY be generated
#define ATTRID_POWER_CONFIG_MAINS_ALARM_MASK                 0x0010
/// The MainsVoltageMinThreshold attribute is 16 bits in length and specifies
/// the lower alarm threshold, measured in units of 100mV, for the MainsVoltage
/// attribute.
#define ATTRID_POWER_CONFIG_MAINS_VOLTAGE_MIN_THRESHOLD      0x0011
/// The MainsVoltageMaxThreshold attribute is 16 bits in length and specifies
/// the upper alarm threshold, measured in units of 100mV, for the MainsVoltage
/// attribute.
#define ATTRID_POWER_CONFIG_MAINS_VOLTAGE_MAX_THRESHOLD      0x0012
/// The MainsVoltageDwellTripPoint attribute is 16 bits in length and specifies
/// the length of time, in seconds that the 3545 value of MainsVoltage MAY
/// exist beyond either of its thresholds before an alarm is generated.
#define ATTRID_POWER_CONFIG_MAINS_VOLTAGE_DWELL_TRIP_POINT   0x0013

// Battery Information
/// The BatteryVoltage attribute is 8 bits in length and specifies the current
/// actual (measured) battery voltage, in units of 100mV.
#define ATTRID_POWER_CONFIG_BATTERY_VOLTAGE                  0x0020
/// Specifies the remaining battery life as a half integer percentage of the
/// full battery capacity
#define ATTRID_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING     0x0021

// Battery Information Default Attribute Value
#define ATTR_DEFAULT_POWER_CFG_BATTERY_PERCENTAGE_REMAINING    0

// Battery Settings
/// The BatteryManufacturer attribute is a maximum of 16 bytes in length and
/// specifies the name of the battery manufacturer as a character string.
#define ATTRID_POWER_CONFIG_BATTERY_MANUFACTURER             0x0030
/// The BatterySize attribute is an enumeration which specifies the type of
/// battery being used by the device.
#define ATTRID_POWER_CONFIG_BATTERY_SIZE                     0x0031
/// The BatteryAHrRating attribute is 16 bits in length and specifies the
/// Ampere-hour rating of the battery, measured in units of 10mAHr.
#define ATTRID_POWER_CONFIG_BATTERY_A_HR_RATING              0x0032
/// The BatteryQuantity attribute is 8 bits in length and specifies the number
/// of battery cells used to power the device.
#define ATTRID_POWER_CONFIG_BATTERY_QUANTITY                 0x0033
/// The BatteryRatedVoltage attribute is 8 bits in length and specifies the
/// rated voltage of the battery being used in the device, measured in
/// units of 100mV.
#define ATTRID_POWER_CONFIG_BATTERY_RATED_VOLTAGE            0x0034
/// The BatteryAlarmMask attribute specifies which battery alarms must
/// be generated.
#define ATTRID_POWER_CONFIG_BATTERY_ALARM_MASK               0x0035
/// Specifies the low battery voltage alarm threshold, measured in units of
/// 100mV at which the device can no longer operate or transmit via its radio
#define ATTRID_POWER_CONFIG_BATTERY_VOLTAGE_MIN_THRESHOLD    0x0036
/// Specify the low voltage alarm thresholds, measured in units of 100mV,
///for the BatteryVoltage attribute. BatteryVoltageThreshold1 is higher than
/// the level specified to trigger BatteryVoltageMinThreshold.
#define ATTRID_POWER_CONFIG_BATTERY_VOLTAGE_THRESHOLD_1      0x0037
/// Specify the low voltage alarm thresholds, measured in units of 100mV,
/// for the BatteryVoltage attribute. BatteryVoltageThreshold2 is higher than
/// the level specified to trigger BatteryVoltageThreshold.
#define ATTRID_POWER_CONFIG_BATTERY_VOLTAGE_THRESHOLD_2      0x0038
/// Specify the low voltage alarm thresholds, measured in units of 100mV,
/// for the BatteryVoltage attribute. BatteryVoltageThreshold3 is higher than
/// the level specified to trigger BatteryVoltageThreshold2.
#define ATTRID_POWER_CONFIG_BATTERY_VOLTAGE_THRESHOLD_3      0x0039
/// Specifies the low battery percentage alarm threshold, measured in
/// percentage.
#define ATTRID_POWER_CONFIG_BATTERY_PERCENTAGE_MIN_THRESHOLD 0x003A
/// Specifies the low battery percentage alarm threshold, measured in
/// percentage. BatteryPercentageThreshold1 is higher than the level specified
/// to trigger BatteryPercentageMinThreshold.
#define ATTRID_POWER_CONFIG_BATTERY_PERCENTAGE_THRESHOLD_1   0x003B
/// Specifies the low battery percentage alarm threshold, measured in
/// percentage. BatteryPercentageThreshold2 is higher than the level specified
/// to trigger BatteryPercentageThreshold.
#define ATTRID_POWER_CONFIG_BATTERY_PERCENTAGE_THRESHOLD_2   0x003C
/// Specifies the low battery percentage alarm threshold, measured in
/// percentage. BatteryPercentageThreshold3 is higher than the level specified
/// to trigger BatteryPercentageThreshold2.
#define ATTRID_POWER_CONFIG_BATTERY_PERCENTAGE_THRESHOLD_3   0x003D
/// Specifies the current state of the device's battery alarms.
#define ATTRID_POWER_CONFIG_BATTERY_ALARM_STATE              0x003E

// Battery 2 Information
/// The BatteryVoltage attribute is 8 bits in length and specifies the current
/// actual (measured) battery voltage, in units of 100mV.
#define ATTRID_POWER_CONFIG_BATTERY_2_VOLTAGE                  0x0040
/// Specifies the remaining battery life as a half integer percentage of the
/// full battery capacity
#define ATTRID_POWER_CONFIG_BATTERY_2_PERCENTAGE_REMAINING     0x0041

// Battery Information Default Attribute Value
#define ATTR_DEFAULT_POWER_CFG_BATTERY_2_PERCENTAGE_REMAINING    0

// Battery 2 Settings
/// The BatteryManufacturer attribute is a maximum of 16 bytes in length and
/// specifies the name of the battery manufacturer as a character string.
#define ATTRID_POWER_CONFIG_BATTERY_2_MANUFACTURER             0x0050
/// The BatterySize attribute is an enumeration which specifies the type of
/// battery being used by the device.
#define ATTRID_POWER_CONFIG_BATTERY_2_SIZE                     0x0051
/// The BatteryAHrRating attribute is 16 bits in length and specifies the
/// Ampere-hour rating of the battery, measured in units of 10mAHr.
#define ATTRID_POWER_CONFIG_BATTERY_2_A_HR_RATING              0x0052
/// The BatteryQuantity attribute is 8 bits in length and specifies the number
/// of battery cells used to power the device.
#define ATTRID_POWER_CONFIG_BATTERY_2_QUANTITY                 0x0053
/// The BatteryRatedVoltage attribute is 8 bits in length and specifies the
/// rated voltage of the battery being used in the device, measured in
/// units of 100mV.
#define ATTRID_POWER_CONFIG_BATTERY_2_RATED_VOLTAGE            0x0054
/// The BatteryAlarmMask attribute specifies which battery alarms must
/// be generated.
#define ATTRID_POWER_CONFIG_BATTERY_2_ALARM_MASK               0x0055
/// Specifies the low battery voltage alarm threshold, measured in units of
/// 100mV at which the device can no longer operate or transmit via its radio
#define ATTRID_POWER_CONFIG_BATTERY_2_VOLTAGE_MIN_THRESHOLD    0x0056
/// Specify the low voltage alarm thresholds, measured in units of 100mV,
///for the BatteryVoltage attribute. BatteryVoltageThreshold1 is higher than
/// the level specified to trigger BatteryVoltageMinThreshold.
#define ATTRID_POWER_CONFIG_BATTERY_2_VOLTAGE_THRESHOLD_1      0x0057
/// Specify the low voltage alarm thresholds, measured in units of 100mV,
/// for the BatteryVoltage attribute. BatteryVoltageThreshold2 is higher than
/// the level specified to trigger BatteryVoltageThreshold.
#define ATTRID_POWER_CONFIG_BATTERY_2_VOLTAGE_THRESHOLD_2      0x0058
/// Specify the low voltage alarm thresholds, measured in units of 100mV,
/// for the BatteryVoltage attribute. BatteryVoltageThreshold3 is higher than
/// the level specified to trigger BatteryVoltageThreshold2.
#define ATTRID_POWER_CONFIG_BATTERY_2_VOLTAGE_THRESHOLD_3      0x0059
/// Specifies the low battery percentage alarm threshold, measured in
/// percentage.
#define ATTRID_POWER_CONFIG_BATTERY_2_PERCENTAGE_MIN_THRESHOLD 0x005A
/// Specifies the low battery percentage alarm threshold, measured in
/// percentage. BatteryPercentageThreshold1 is higher than the level specified
/// to trigger BatteryPercentageMinThreshold.
#define ATTRID_POWER_CONFIG_BATTERY_2_PERCENTAGE_THRESHOLD_1   0x005B
/// Specifies the low battery percentage alarm threshold, measured in
/// percentage. BatteryPercentageThreshold2 is higher than the level specified
/// to trigger BatteryPercentageThreshold.
#define ATTRID_POWER_CONFIG_BATTERY_2_PERCENTAGE_THRESHOLD_2   0x005C
/// Specifies the low battery percentage alarm threshold, measured in
/// percentage. BatteryPercentageThreshold3 is higher than the level specified
/// to trigger BatteryPercentageThreshold2.
#define ATTRID_POWER_CONFIG_BATTERY_2_PERCENTAGE_THRESHOLD_3   0x005D
/// Specifies the current state of the device's battery alarms.
#define ATTRID_POWER_CONFIG_BATTERY_2_ALARM_STATE              0x005E

// Battery 3 Information
/// The BatteryVoltage attribute is 8 bits in length and specifies the current
/// actual (measured) battery voltage, in units of 100mV.
#define ATTRID_POWER_CONFIG_BATTERY_3_VOLTAGE                  0x0060
/// Specifies the remaining battery life as a half integer percentage of the
/// full battery capacity
#define ATTRID_POWER_CONFIG_BATTERY_3_PERCENTAGE_REMAINING     0x0061

// Battery Information Default Attribute Value
#define ATTR_DEFAULT_POWER_CFG_BATTERY_3_PERCENTAGE_REMAINING    0

// Battery 3 Settings
/// The BatteryManufacturer attribute is a maximum of 16 bytes in length and
/// specifies the name of the battery manufacturer as a character string.
#define ATTRID_POWER_CONFIG_BATTERY_3_MANUFACTURER             0x0070
/// The BatterySize attribute is an enumeration which specifies the type of
/// battery being used by the device.
#define ATTRID_POWER_CONFIG_BATTERY_3_SIZE                     0x0071
/// The BatteryAHrRating attribute is 16 bits in length and specifies the
/// Ampere-hour rating of the battery, measured in units of 10mAHr.
#define ATTRID_POWER_CONFIG_BATTERY_3_A_HR_RATING              0x0072
/// The BatteryQuantity attribute is 8 bits in length and specifies the number
/// of battery cells used to power the device.
#define ATTRID_POWER_CONFIG_BATTERY_3_QUANTITY                 0x0073
/// The BatteryRatedVoltage attribute is 8 bits in length and specifies the
/// rated voltage of the battery being used in the device, measured in
/// units of 100mV.
#define ATTRID_POWER_CONFIG_BATTERY_3_RATED_VOLTAGE            0x0074
/// The BatteryAlarmMask attribute specifies which battery alarms must
/// be generated.
#define ATTRID_POWER_CONFIG_BATTERY_3_ALARM_MASK               0x0075
/// Specifies the low battery voltage alarm threshold, measured in units of
/// 100mV at which the device can no longer operate or transmit via its radio
#define ATTRID_POWER_CONFIG_BATTERY_3_VOLTAGE_MIN_THRESHOLD    0x0076
/// Specify the low voltage alarm thresholds, measured in units of 100mV,
///for the BatteryVoltage attribute. BatteryVoltageThreshold1 is higher than
/// the level specified to trigger BatteryVoltageMinThreshold.
#define ATTRID_POWER_CONFIG_BATTERY_3_VOLTAGE_THRESHOLD_1      0x0077
/// Specify the low voltage alarm thresholds, measured in units of 100mV,
/// for the BatteryVoltage attribute. BatteryVoltageThreshold2 is higher than
/// the level specified to trigger BatteryVoltageThreshold.
#define ATTRID_POWER_CONFIG_BATTERY_3_VOLTAGE_THRESHOLD_2      0x0078
/// Specify the low voltage alarm thresholds, measured in units of 100mV,
/// for the BatteryVoltage attribute. BatteryVoltageThreshold3 is higher than
/// the level specified to trigger BatteryVoltageThreshold2.
#define ATTRID_POWER_CONFIG_BATTERY_3_VOLTAGE_THRESHOLD_3      0x0079
/// Specifies the low battery percentage alarm threshold, measured in
/// percentage.
#define ATTRID_POWER_CONFIG_BATTERY_3_PERCENTAGE_MIN_THRESHOLD 0x007A
/// Specifies the low battery percentage alarm threshold, measured in
/// percentage. BatteryPercentageThreshold1 is higher than the level specified
/// to trigger BatteryPercentageMinThreshold.
#define ATTRID_POWER_CONFIG_BATTERY_3_PERCENTAGE_THRESHOLD_1   0x007B
/// Specifies the low battery percentage alarm threshold, measured in
/// percentage. BatteryPercentageThreshold2 is higher than the level specified
/// to trigger BatteryPercentageThreshold.
#define ATTRID_POWER_CONFIG_BATTERY_3_PERCENTAGE_THRESHOLD_2   0x007C
/// Specifies the low battery percentage alarm threshold, measured in
/// percentage. BatteryPercentageThreshold3 is higher than the level specified
/// to trigger BatteryPercentageThreshold2.
#define ATTRID_POWER_CONFIG_BATTERY_3_PERCENTAGE_THRESHOLD_3   0x007D
/// Specifies the current state of the device's battery alarms.
#define ATTRID_POWER_CONFIG_BATTERY_3_ALARM_STATE              0x007E

/*** Mains Alarm Mask Attribute bit ***/
#define MAINS_ALARM_MASK_VOLT_2_LOW                       0x01
#define MAINS_ALARM_MASK_VOLT_2_HI                        0x02
#define MAINS_ALARM_MASK_POWER_SUPP_LOST                  0x04

/*** Battery Size Attribute values ***/
#define BAT_SIZE_NO_BATTERY                               0x00
#define BAT_SIZE_BUILT_IN                                 0x01
#define BAT_SIZE_OTHER                                    0x02
#define BAT_SIZE_AA                                       0x03
#define BAT_SIZE_AAA                                      0x04
#define BAT_SIZE_C                                        0x05
#define BAT_SIZE_D                                        0x06
#define BAT_SIZE_CR2                                      0x07
#define BAT_SIZE_CR123A                                   0x08
#define BAT_SIZE_UNKNOWN                                  0xFF

/*** Battery Alarm Mask Attribute bit ***/
#define BAT_ALARM_MASK_VOLT_2_LOW                         0x01  // i.e., BatteryVoltageMinThreshold value has been reached
#define BAT_ALARM_MASK_BATTERY_ALARM_1                    0x02
#define BAT_ALARM_MASK_BATTERY_ALARM_2                    0x04
#define BAT_ALARM_MASK_BATTERY_ALARM_3                    0x08

/*** Alarm Code Field Enumerations for Battery Alarm values ***/
#define ALARM_CODE_BAT_VOLT_MIN_THRES_BAT_SRC_1           0x10  // BatteryPercentageMinThreshold reached for Battery Source 1
#define ALARM_CODE_BAT_VOLT_THRES_1_BAT_SRC_1             0x11  // BatteryPercentageThreshold1 reached for Battery Source 1
#define ALARM_CODE_BAT_VOLT_THRES_2_BAT_SRC_1             0x12  // BatteryPercentageThreshold2 reached for Battery Source 1
#define ALARM_CODE_BAT_VOLT_THRES_3_BAT_SRC_1             0x13  // BatteryPercentageThreshold3 reached for Battery Source 1
#define ALARM_CODE_BAT_VOLT_MIN_THRES_BAT_SRC_2           0x20  // BatteryPercentageMinThreshold reached for Battery Source 2
#define ALARM_CODE_BAT_VOLT_THRES_1_BAT_SRC_2             0x21  // BatteryPercentageThreshold1 reached for Battery Source 2
#define ALARM_CODE_BAT_VOLT_THRES_2_BAT_SRC_2             0x22  // BatteryPercentageThreshold2 reached for Battery Source 2
#define ALARM_CODE_BAT_VOLT_THRES_3_BAT_SRC_2             0x23  // BatteryPercentageThreshold3 reached for Battery Source 2
#define ALARM_CODE_BAT_VOLT_MIN_THRES_BAT_SRC_3           0x30  // BatteryPercentageMinThreshold reached for Battery Source 3
#define ALARM_CODE_BAT_VOLT_THRES_1_BAT_SRC_3             0x31  // BatteryPercentageThreshold1 reached for Battery Source 3
#define ALARM_CODE_BAT_VOLT_THRES_2_BAT_SRC_3             0x32  // BatteryPercentageThreshold2 reached for Battery Source 3
#define ALARM_CODE_BAT_VOLT_THRES_3_BAT_SRC_3             0x33  // BatteryPercentageThreshold3 reached for Battery Source 3
#define ALARM_CODE_BAT_MAINS_POWER_SUPP_LOST              0x3A  // Mains power supply lost/unavailable
#define ALARM_CODE_BAT_ALARM_NOT_GEN                      0xFF  // Alarm shall not be generated

/*** Battery Alarm State Attribute bit ***/
#define BAT_ALARM_STATE_BAT_VOLT_MIN_THRES_BAT_SRC_1      0x00000001
#define BAT_ALARM_STATE_BAT_VOLT_THRES_1_BAT_SRC_1        0x00000002
#define BAT_ALARM_STATE_BAT_VOLT_THRES_2_BAT_SRC_1        0x00000004
#define BAT_ALARM_STATE_BAT_VOLT_THRES_3_BAT_SRC_1        0x00000008
#define BAT_ALARM_STATE_BAT_VOLT_MIN_THRES_BAT_SRC_2      0x00000400
#define BAT_ALARM_STATE_BAT_VOLT_THRES_1_BAT_SRC_2        0x00000800
#define BAT_ALARM_STATE_BAT_VOLT_THRES_2_BAT_SRC_2        0x00001000
#define BAT_ALARM_STATE_BAT_VOLT_THRES_3_BAT_SRC_2        0x00002000
#define BAT_ALARM_STATE_BAT_VOLT_MIN_THRES_BAT_SRC_3      0x00100000
#define BAT_ALARM_STATE_BAT_VOLT_THRES_1_BAT_SRC_3        0x00200000
#define BAT_ALARM_STATE_BAT_VOLT_THRES_2_BAT_SRC_3        0x00400000
#define BAT_ALARM_STATE_BAT_VOLT_THRES_3_BAT_SRC_3        0x00800000
#define BAT_ALARM_STATE_MAINS_POWER_SUPP_LOST             0x40000000

/********************************************/
/*** Power Configuration Cluster Commands ***/
/********************************************/
  // No cluster specific commands

/** @} End ZCL_GENERAL_POWER_CONFIGURATION_CLUSTER */

/**
* @defgroup ZCL_GENERAL_DEVICE_TEMP_CONFIGURATION ZCL Device Temperature Configuration Cluster
* @{
*/

/***********************************************************/
/*** Device Temperature Configuration Cluster Attributes ***/
/***********************************************************/
// Device Temperature Information
/// The CurrentTemperature attribute is 16 bits in length and specifies the
/// current internal temperature, in degrees Celsius, of the device.
#define ATTRID_DEVICE_TEMPERATURE_CONFIGURATION_CURRENT_TEMPERATURE                    0x0000
/// The MinTempExperienced attribute is 16 bits in length and specifies the
/// minimum internal temperature, in degrees Celsius, the device has experienced
/// while powered.
#define ATTRID_DEVICE_TEMPERATURE_CONFIGURATION_MIN_TEMP_EXPERIENCED                   0x0001
/// The MaxTempExperienced attribute is 16 bits in length and specifies the
/// maximum internal temperature, in degrees  Celsius, the device has experienced
/// while powered.
#define ATTRID_DEVICE_TEMPERATURE_CONFIGURATION_MAX_TEMP_EXPERIENCED                   0x0002
/// The MaxTempExperienced attribute is 16 bits in length and specifies the
/// maximum internal temperature, in degrees Celsius, the device has experienced
/// while powered.
#define ATTRID_DEVICE_TEMPERATURE_CONFIGURATION_OVER_TEMP_TOTAL_DWELL                  0x0003

// Device Temperature Settings
/// The DeviceTempAlarmMask attribute is 8 bits in length and specifies which
/// alarms MAY be generated.
#define ATTRID_DEVICE_TEMPERATURE_CONFIGURATION_DEVICE_TEMP_ALARM_MASK                 0x0010
/// The LowTempThreshold attribute is 16 bits in length and specifies the lower
/// alarm threshold.
#define ATTRID_DEVICE_TEMPERATURE_CONFIGURATION_LOW_TEMP_THRESHOLD                     0x0011
/// The HighTempThreshold attribute is 16 bits in length and specifies the
/// upper alarm threshold.
#define ATTRID_DEVICE_TEMPERATURE_CONFIGURATION_HIGH_TEMP_THRESHOLD                    0x0012
/// The LowTempDwellTripPoint attribute is 24 bits in length and specifies the
/// length of time, in seconds, that the value  of CurrentTemperature
#define ATTRID_DEVICE_TEMPERATURE_CONFIGURATION_LOW_TEMP_DWELL_TRIP_POINT              0x0013
/// The HighTempDwellTripPoint attribute is 24 bits in length and specifies the
/// length of time, in seconds, that the value of CurrentTemperature
#define ATTRID_DEVICE_TEMPERATURE_CONFIGURATION_HIGH_TEMP_DWELL_TRIP_POINT             0x0014

/*** Device Temp Alarm_Mask Attribute bits ***/
#define DEV_TEMP_ALARM_MASK_2_LOW                         0x01
#define DEV_TEMP_ALARM_MASK_2_HI                          0x02

/*********************************************************/
/*** Device Temperature Configuration Cluster Commands ***/
/*********************************************************/
  // No cluster specific commands

/** @} End ZCL_GENERAL_DEVICE_TEMP_CONFIGURATION */

/**
* @defgroup ZCL_GENERAL_IDENTIFY_CLUSTER ZCL Identify Cluster
* @{
*/
/***********************************/
/*** Identify Cluster Attributes ***/
/***********************************/
/// The IdentifyTime attribute specifies the remaining length of time, in seconds,
/// that the device will continue to identify itself.
#define ATTRID_IDENTIFY_IDENTIFY_TIME                             0x0000

/*********************************/
/*** Identify Cluster Commands ***/
/*********************************/
/// The identify command starts or stops the receiving device identifying itself.
#define COMMAND_IDENTIFY_IDENTIFY                                 0x00
/// The identify query command allows the sending device to request the target
/// or targets to respond if they are currently identifying themselves.
#define COMMAND_IDENTIFY_IDENTIFY_QUERY                           0x01
/// The Trigger Effect command allows the support of feedback to the user,
/// such as a certain light effect.
#define COMMAND_IDENTIFY_TRIGGER_EFFECT                  0x40
/// The identify query response command is generated in response to receiving
/// an Identify Query command.
#define COMMAND_IDENTIFY_IDENTIFY_QUERY_RESPONSE                       0x00

/*** Values of 'effect identifier' field of 'trigger effect' command ***/
#define EFFECT_ID_BLINK              0x00 // Light is turned on/off once
#define EFFECT_ID_BREATHE            0x01 // Light turned on/off over 1s, repeated 15 times
#define EFFECT_ID_OKAY               0x02 // Colored light turns green for 1s; colored light flashes twice
#define EFFECT_ID_CHANNEL_CHANGE     0x0b // Colored light turns orange for 8s; non-colored light switches to max brightness for 0.5s and then min brightness for 7.5s
#define EFFECT_ID_FINISH_EFFECT      0xfe // Finish effect
#define EFFECT_ID_STOP_EFFECT        0xff // Stop effect

/*** Values of 'effect variant' field of 'trigger effect' command ***/
#define EFFECT_VARIANT_DEFAULT                           0x00 // Default

/** @} End ZCL_GENERAL_IDENTIFY_CLUSTER */

/**
* @defgroup ZCL_GENERAL_GROUPS_CLUSTER ZCL Groups Cluster
* @{
*/

/********************************/
/*** Group Cluster Attributes ***/
/********************************/
/// The most significant bit of the NameSupport attribute indicates whether
/// or not group names are supported.
#define ATTRID_GROUPS_NAME_SUPPORT                         0x0000

/******************************/
/*** Group Cluster Commands ***/
/******************************/
/// The Add Group command allows the sending device to add group membership in
/// a particular group for one or more endpoints on the receiving device.
#define COMMAND_GROUPS_ADD_GROUP                                 0x00
/// The view group command allows the sending device to request that the
/// receiving entity or entities respond with a view group response command
/// containing the application name string for a particular group.
#define COMMAND_GROUPS_VIEW_GROUP                                0x01
/// The get group membership command allows the sending device to inquire about
/// the group membership of the receiving device and endpoint in a number of ways.
#define COMMAND_GROUPS_GET_GROUP_MEMBERSHIP                      0x02
/// The remove group command allows the sender to request that the receiving
/// entity or entities remove their membership, if any, in a particular group.
#define COMMAND_GROUPS_REMOVE_GROUP                              0x03
/// The remove all groups command allows the sending device to direct the
/// receiving entity or entities to remove all group associations.
#define COMMAND_GROUPS_REMOVE_ALL_GROUPS                          0x04
/// The add group if identifying command allows the sending device to add group
/// membership in a particular group for one or more endpoints on the receiving device
#define COMMAND_GROUPS_ADD_GROUP_IF_IDENTIFYING                  0x05

/// The add group response is sent by the groups cluster server in response to
/// an add group command.
#define COMMAND_GROUPS_ADD_GROUP_RESPONSE                             0x00
/// The view group response command is sent by the groups cluster server in
/// response to a view group command.
#define COMMAND_GROUPS_VIEW_GROUP_RESPONSE                            0x01
/// The get group membership response command is sent by the groups cluster
/// server in response to a get group membership command.
#define COMMAND_GROUPS_GET_GROUP_MEMBERSHIP_RESPONSE                  0x02
/// The remove group response command is generated by an application entity in
/// response to the receipt of a remove group command.
#define COMMAND_GROUPS_REMOVE_GROUP_RESPONSE                          0x03

/** @} End ZCL_GENERAL_GROUPS_CLUSTER */

/**
* @defgroup ZCL_GENERAL_SCENES_CLUSTER ZCL Scenes Cluster
* @{
*/

/*********************************/
/*** Scenes Cluster Attributes ***/
/*********************************/
// Scene Management Information
// The SceneCount attribute specifies the number of scenes currently in the
/// device's scene table.
#define ATTRID_SCENES_SCENE_COUNT                         0x0000
/// The CurrentScene attribute holds the Scene ID of the scene last invoked.
#define ATTRID_SCENES_CURRENT_SCENE                       0x0001
/// The CurrentGroup attribute holds the Group ID of the scene last invoked,
/// or 0x0000 if the scene last invoked is not associated with a group
#define ATTRID_SCENES_CURRENT_GROUP                       0x0002
/// The SceneValid attribute indicates whether the state of the device
/// corresponds to that associated with the CurrentScene and CurrentGroup attributes.
#define ATTRID_SCENES_SCENE_VALID                         0x0003
/// The most significant bit of the NameSupport attribute indicates whether or
/// not scene names are supported.
#define ATTRID_SCENES_NAME_SUPPORT                        0x0004
/// The LastConfiguredBy attribute is 64 bits in length and specifies the IEEE
/// address of the device that last configured the scene table.
#define ATTRID_SCENES_LAST_CONFIGURED_BY                  0x0005

/*******************************/
/*** Scenes Cluster Commands ***/
/*******************************/
/// On receipt of this command, the device SHALL (if possible) create an entry
/// in the Scene Table with fields copied from the command payload
#define COMMAND_SCENES_ADD_SCENE                                 0x00
/// On receipt of this command, the device SHALL generate an appropriate View
/// Scene Response command.
#define COMMAND_SCENES_VIEW_SCENE                                0x01
/// On receipt of this command, the device SHALL (if possible) remove from its
/// Scene Table the entry with this Scene ID and group ID.
#define COMMAND_SCENES_REMOVE_SCENE                              0x02
/// On receipt of this command, the device SHALL, if possible, remove from its
/// Scene Table all entries with this Group ID.
#define COMMAND_SCENES_REMOVE_ALL_SCENES                          0x03
/// On receipt of this command, the device SHALL (if possible) add an entry to
/// the Scene Table with the Scene ID and  Group ID given in the command.
#define COMMAND_SCENES_STORE_SCENE                               0x04
/// On receipt of this command, the device SHALL (if possible) locate the entry
///in its Scene Table with the Group ID and Scene ID given in the command.
#define COMMAND_SCENES_RECALL_SCENE                              0x05
/// The Get Scene Membership command can be used to find an unused scene number
/// within the group when no commissioning tool is in the network.
#define COMMAND_SCENES_GET_SCENE_MEMBERSHIP                      0x06
/// The Enhanced Add Scene command allows a scene to be added using a finer
/// scene transition time than the Add Scene command.
#define COMMAND_SCENES_ENHANCED_ADD_SCENE                        0x40
/// The Enhanced View Scene command allows a scene to be retrieved using a
/// finer scene transition time than the View Scene command.
#define COMMAND_SCENES_ENHANCED_VIEW_SCENE                       0x41
/// The Copy Scene command allows a device to efficiently copy scenes from one
/// group/scene identifier pair to another group/scene identifier pair.
#define COMMAND_SCENES_COPY_SCENE                                0x42

/// This command is generated in response to a received Add Scene command.
#define COMMAND_SCENES_ADD_SCENE_RESPONSE                             0x00
/// This command is generated in response to a received View Scene command.
#define COMMAND_SCENES_VIEW_SCENE_RESPONSE                            0x01
/// This command is generated in response to a received Remove Scene command.
#define COMMAND_SCENES_REMOVE_SCENE_RESPONSE                          0x02
/// This command is generated in response to a received Remove All Scenes command.
#define COMMAND_SCENES_REMOVE_ALL_SCENES_RESPONSE                      0x03
/// This command is generated in response to a received Store Scene command.
#define COMMAND_SCENES_STORE_SCENE_RESPONSE                           0x04
/// This command is generated in response to a received Get Scene Membership command.
#define COMMAND_SCENES_GET_SCENE_MEMBERSHIP_RESPONSE                  0x06
/// The Enhanced Add Scene Response command allows a device to respond to an
/// Enhanced Add Scene command.
#define COMMAND_SCENES_ENHANCED_ADD_SCENE_RESPONSE                    0x40
/// The Enhanced View Scene Response command allows a device to respond to an
/// Enhanced View Scene command using a finer scene transition time that was
/// available in the ZCL.
#define COMMAND_SCENES_ENHANCED_VIEW_SCENE_RESPONSE                   0x41
/// The Copy Scene Response command allows a device to respond to a
/// Copy Scene command.
#define COMMAND_SCENES_COPY_SCENE_RESPONSE                            0x42

// command parameter
#define SCENE_COPY_MODE_ALL_BIT                           0x01

/** @} End ZCL_GENERAL_SCENES_CLUSTER */

/**
* @defgroup ZCL_GENERAL_ON_OFF_CLUSTER ZCL On Off Cluster
* @{
*/

/*********************************/
/*** On/Off Cluster Attributes ***/
/*********************************/
/// Represents the On/Off device state
#define ATTRID_ON_OFF_ON_OFF                                     0x0000
/// In order to support the use case where the user gets back the last setting
/// of the devices
#define ATTRID_ON_OFF_GLOBAL_SCENE_CONTROL                0x4000
/// The OnTime attribute specifies the length of time (in 1/10ths second) that
/// the "on" state SHALL be maintained before automatically transitioning to the
/// "off" state when using the On with timed off command.
#define ATTRID_ON_OFF_ON_TIME                             0x4001
/// The OffWaitTime attribute specifies the length of time (in 1/10ths second)
/// that the "off" state SHALL be guarded to prevent an on command turning the
/// device back to its "on" state (e.g., when leaving a room, the lights are
/// turned off but an occupancy sensor detects the leaving person and attempts
/// to turn the lights back on).
#define ATTRID_ON_OFF_OFF_WAIT_TIME                       0x4002
// Start Up On/Off Attribute
#define ATTRID_ON_OFF_START_UP_ON_OFF                     0x4003

/*******************************/
/*** On/Off Cluster Commands ***/
/*******************************/
/// On receipt of this command, a device SHALL enter its 'Off' state.
#define COMMAND_ON_OFF_OFF                                       0x00
/// On receipt of this command, a device SHALL enter its 'On' state.
#define COMMAND_ON_OFF_ON                                        0x01
/// On receipt of this command, if a device is in its 'Off' state it SHALL
///enter its 'On' state. Otherwise, if it is in its 'On' state it SHALL
/// enter its 'Off' state.
#define COMMAND_ON_OFF_TOGGLE                                    0x02
/// The Effect Identifier field is 8-bits in length and specifies the fading
/// effect to use when switching the device off.
#define COMMAND_ON_OFF_OFF_WITH_EFFECT                           0x40
/// The On With Recall Global Scene command allows the recall of the settings
/// when the device was turned off.
#define COMMAND_ON_OFF_ON_WITH_RECALL_GLOBAL_SCENE               0x41
/// The On With Timed Off command allows devices to be turned on for a specific
/// duration with a guarded off duration so that SHOULD the device be
/// subsequently switched off, further On With Timed Off commands.
#define COMMAND_ON_OFF_ON_WITH_TIMED_OFF                         0x42

/*** Values of 'effect identifier' field of 'off with effect' command  ***/
#define EFFECT_ID_DELAY_ALL_OFF                           0x00
#define EFFECT_ID_DYING_LIGHT                             0x01

/*** Values of 'effect variant' field of 'off with effect' command ***/
// Effect identifier value = 0x00
#define EFFECT_VARIANT_FADE_TO_OFF                        0x00 // Fade to off in 0.8 seconds (default)
#define EFFECT_VARIANT_NO_FADE                            0x01 // No fade
#define EFFECT_VARIANT_DIM_DOWN                           0x01 // 50% dim down and fade to off in 12s

// Effect identifier value = 0x01
#define EFFECT_VARIANT_DIM_UP                             0x00 // 20% dim up in 0.5s then fade to off in 1s (default)

/** @} End ZCL_GENERAL_ON_OFF_CLUSTER */

/**
* @defgroup ZCL_GENERAL_COMMISSIONING_CLUSTER ZCL Commissioning Cluster
* @{
*/

/****************************************/
/*** Commissioning Cluster Attributes ***/
/****************************************/
#define ATTRID_COMMISSIONING_SHORT_ADDRESS 			        0x0000
#define ATTRID_COMMISSIONING_EXTENDED_PAN_ID 		    	0x0001
#define ATTRID_COMMISSIONING_PAN_ID 			            0x0002
#define ATTRID_COMMISSIONING_CHANNEL_MASK 		        	0x0003
#define ATTRID_COMMISSIONING_PROTOCOL_VERSION 	   	    	0x0004
#define ATTRID_COMMISSIONING_STACK_PROFILE 			        0x0005
#define ATTRID_COMMISSIONING_STARTUP_CONTROL 		    	0x0006
#define ATTRID_COMMISSIONING_TRUST_CENTER_ADDRESS 	    	0x0010
#define ATTRID_COMMISSIONING_TRUST_CENTER_MASTER_KEY    	0x0011
#define ATTRID_COMMISSIONING_NETWORK_KEY 			        0x0012
#define ATTRID_COMMISSIONING_USE_INSECURE_JOIN 		    	0x0013
#define ATTRID_COMMISSIONING_PRECONFIGURED_LINK_KEY     	0x0014
#define ATTRID_COMMISSIONING_NETWORK_KEY_SEQ_NUM 	    	0x0015
#define ATTRID_COMMISSIONING_NETWORK_KEY_TYPE 		    	0x0016
#define ATTRID_COMMISSIONING_NETWORK_MANAGER_ADDRESS    	0x0017
#define ATTRID_COMMISSIONING_SCAN_ATTEMPTS 			        0x0020
#define ATTRID_COMMISSIONING_TIME_BETWEEN_SCANS 	    	0x0021
#define ATTRID_COMMISSIONING_REJOIN_INTERVAL 		    	0x0022
#define ATTRID_COMMISSIONING_MAX_REJOIN_INTERVAL 	    	0x0023
#define ATTRID_COMMISSIONING_INDIRECT_POLL_RATE 	    	0x0030
#define ATTRID_COMMISSIONING_PARENT_RETRY_THRESHOLD     	0x0031
#define ATTRID_COMMISSIONING_CONCENTRATOR_FLAG 			    0x0040
#define ATTRID_COMMISSIONING_CONCENTRATOR_RADIUS 	    	0x0041
#define ATTRID_COMMISSIONING_CONCENTRATOR_DISCOVERY_TIME    0x0042

/**************************************/
/*** Commissioning Cluster Commands ***/
/**************************************/
#define COMMAND_COMMISSIONING_RESTART_DEVICE 			                    0x00
#define COMMAND_COMMISSIONING_SAVE_STARTUP_PARAMETERS 			            0x01
#define COMMAND_COMMISSIONING_RESTORE_STARTUP_PARAMETERS 		        	0x02
#define COMMAND_COMMISSIONING_RESET_STARTUP_PARAMETERS 		            	0x03
#define COMMAND_COMMISSIONING_RESTART_DEVICE_RESPONSE 		    	        0x00
#define COMMAND_COMMISSIONING_SAVE_STARTUP_PARAMETERS_RESPONSE 		    	0x01
#define COMMAND_COMMISSIONING_RESTORE_STARTUP_PARAMETERS_RESPONSE 			0x02
#define COMMAND_COMMISSIONING_RESET_STARTUP_PARAMETERS_RESPONSE 			0x03

/** @} End ZCL_GENERAL_COMMISSIONING_CLUSTER */

/**
* @defgroup ZCL_GENERAL_POLL_CONTROL_CLUSTER ZCL Poll Control Cluster
* @{
*/

/****************************************/
/*** Poll Control Cluster Attributes ***/
/****************************************/
#define ATTRID_POLL_CONTROL_CHECK_IN_INTERVAL 			0x0000
#define ATTRID_POLL_CONTROL_LONG_POLL_INTERVAL 			0x0001
#define ATTRID_POLL_CONTROL_SHORT_POLL_INTERVAL 		0x0002
#define ATTRID_POLL_CONTROL_FAST_POLL_TIMEOUT 			0x0003
#define ATTRID_POLL_CONTROL_CHECK_IN_INTERVAL_MIN 		0x0004
#define ATTRID_POLL_CONTROL_LONG_POLL_INTERVAL_MIN 		0x0005
#define ATTRID_POLL_CONTROL_FAST_POLL_TIMEOUT_MAX 		0x0006

/**************************************/
/*** Poll Control Cluster Commands ***/
/**************************************/
#define COMMAND_POLL_CONTROL_CHECK_IN 			                0x00
#define COMMAND_POLL_CONTROL_CHECK_IN_RESPONSE 		        	0x00
#define COMMAND_POLL_CONTROL_FAST_POLL_STOP 		        	0x01
#define COMMAND_POLL_CONTROL_SET_LONG_POLL_INTERVAL 			0x02
#define COMMAND_POLL_CONTROL_SET_SHORT_POLL_INTERVAL 			0x03

/** @} End ZCL_GENERAL_POLL_CONTROL_CLUSTER */

/**
* @defgroup ZCL_GENERAL_ON_OFF_SWITCH_CONFIG_CLUSTER ZCL On Off Switch Config Cluster
* @{
*/

/****************************************/
/*** On/Off Switch Cluster Attributes ***/
/****************************************/
// Switch Information
/// The SwitchType attribute specifies the basic functionality of the On/Off
/// switching device.
#define ATTRID_ON_OFF_SWITCH_CONFIGURATION_SWITCH_TYPE                          0x0000

// Switch Settings
/// The SwitchActions attribute is 8 bits in length and specifies the commands
///of the On/Off cluster
#define ATTRID_ON_OFF_SWITCH_CONFIGURATION_SWITCH_ACTIONS                       0x0010

/*** On Off Switch Type attribute values ***/
#define ON_OFF_SWITCH_CONFIGURATION_SWITCH_TYPE_TOGGLE                          0x00
#define ON_OFF_SWITCH_CONFIGURATION_SWITCH_TYPE_MOMENTARY                       0x01
#define ON_OFF_SWITCH_CONFIGURATION_SWITCH_TYPE_MULTIFUNCTION                   0x02

/*** On Off Switch Actions attribute values ***/
#define ON_OFF_SWITCH_CONFIGURATION_SWITCH_ACTIONS_ON                           0x00
#define ON_OFF_SWITCH_CONFIGURATION_SWITCH_ACTIONS_OFF                          0x01
#define ON_OFF_SWITCH_CONFIGURATION_SWITCH_ACTIONS_TOGGLE                       0x02

/**************************************/
/*** On/Off Switch Cluster Commands ***/
/**************************************/
  // No cluster specific commands

/** @} End ZCL_GENERAL_ON_OFF_SWITCH_CONFIG_CLUSTER */

/**
* @defgroup ZCL_GENERAL_LEVEL_CONTROL_CLUSTER ZCL Level Control Cluster
* @{
*/

/****************************************/
/*** Level Control Cluster Attributes ***/
/****************************************/
/// The CurrentLevel attribute represents the current level of this device.
/// The meaning of 'level' is device dependent.
#define ATTRID_LEVEL_CURRENT_LEVEL                        0x0000
/// The RemainingTime attribute represents the time remaining until the
/// current command is complete - it is specified in 1/10ths of a second.
#define ATTRID_LEVEL_REMAINING_TIME                       0x0001
/// The MinLevel attribute indicates the minimum value of CurrentLevel that is
/// capable of being assigned.
#define ATTRID_LEVEL_MIN_LEVEL                            0x0002
/// The MaxLevel attribute indicates the maximum value of CurrentLevel that is
/// capable of being assigned.
#define ATTRID_LEVEL_MAX_LEVEL                            0x0003
/// The CurrentFrequency attribute represents the frequency that the devices
/// is at CurrentLevel. A CurrentFrequency of 0 is unknown.
#define ATTRID_LEVEL_CURRENT_FREQUENCY                    0x0004
/// The MinFrequency attribute indicates the minimum value of CurrentFrequency
/// that is capable of being assigned. MinFrequency shall be less than or equal
/// to MaxFrequency. A value of 0 indicates undefined.
#define ATTRID_LEVEL_MIN_FREQUENCY                        0x0005
/// The MaxFrequency attribute indicates the maximum value of CurrentFrequency
/// that is capable of being assigned. MaxFrequency shall be greater than or
/// equal to MinFrequency. A value of 0 indicates undefined.
#define ATTRID_LEVEL_MAX_FREQUENCY                        0x0006
/// The OnOffTransitionTime attribute represents the time taken to move to or
/// from the target level when On of Off commands are received by an On/Off
/// cluster on the same endpoint. It is specified in 1/10ths of a second.
#define ATTRID_LEVEL_ON_OFF_TRANSITION_TIME               0x0010
/// The OnLevel attribute determines the value that the CurrentLevel attribute
/// is set to when the OnOff attribute of an On/Off cluster on the same endpoint
/// is set to On, as a result of processing an On/Off cluster command.
#define ATTRID_LEVEL_ON_LEVEL                             0x0011
/// The OnTransitionTime attribute represents the time taken to move the current
/// level from the minimum level to the maximum level when an On command is
/// received by an On/Off cluster on the same endpoint.
#define ATTRID_LEVEL_ON_TRANSITION_TIME                   0x0012
/// The OffTransitionTime attribute represents the time taken to move the current
/// level from the maximum level to the minimum level when an Off command is
/// received by an On/Off cluster on the same endpoint.
#define ATTRID_LEVEL_OFF_TRANSITION_TIME                  0x0013
/// The DefaultMoveRate attribute determines the movement rate, in units per
/// second, when a Move command is received with a Rate parameter of 0xFF.
#define ATTRID_LEVEL_DEFAULT_MOVE_RATE                    0x0014
/// The Options attribute is meant to be changed only during commissioning.
#define ATTRID_LEVEL_OPTIONS                              0x000F
/// The StartUpCurrentLevel attribute SHALL define the desired startup level
///vfor a device when it is supplied with power and this level SHALL be reflected in the CurrentLevel attribute.
#define ATTRID_LEVEL_START_UP_CURRENT_LEVEL               0x4000

  // Level Control Default Attribute Values
#define ATTR_DEFAULT_LEVEL_CURRENT_LEVEL                  0xFF
#define ATTR_DEFAULT_LEVEL_REMAINING_TIME                 0x0000
#define ATTR_DEFAULT_LEVEL_MIN_LEVEL                      0x00
#define ATTR_DEFAULT_LEVEL_MAX_LEVEL                      0xFF
#define ATTR_DEFAULT_LEVEL_CURRENT_FREQUENCY              0x0000
#define ATTR_DEFAULT_LEVEL_MIN_FREQUENCY                  0x0000
#define ATTR_DEFAULT_LEVEL_MAX_FREQUENCY                  0x0000
#define ATTR_DEFAULT_LEVEL_ON_OFF_TRANSITION_TIME         0x0000
#define ATTR_DEFAULT_LEVEL_ON_LEVEL                       0xFF
#define ATTR_DEFAULT_LEVEL_ON_TRANSITION_TIME             0xFFFF
#define ATTR_DEFAULT_LEVEL_OFF_TRANSITION_TIME            0xFFFF
#define ATTR_DEFAULT_LEVEL_DEFAULT_MOVE_RATE              0
#define ATTR_DEFAULT_LEVEL_OPTIONS                        0x00
#define ATTR_DEFAULT_LEVEL_START_UP_CURRENT_LEVEL         0x00

#define ATTR_LEVEL_MAX_LEVEL                              0xFE   // MAX level
#define ATTR_LEVEL_MIN_LEVEL                              0x01   // MIN level
#define ATTR_LEVEL_MID_LEVEL                              0x7E   // MID level
#define ATTR_LEVEL_ON_LEVEL_NO_EFFECT                     0xFF

/**************************************/
/*** Level Control Cluster Commands ***/
/**************************************/
/// On receipt of this command, a device SHALL move from its current level to
/// the value given in the Level field.
#define COMMAND_LEVEL_MOVE_TO_LEVEL                       0x00
/// On receipt of this command, a device SHALL move from its current level in
/// an up or down direction in a continuous fashion.
#define COMMAND_LEVEL_MOVE                                0x01
/// On receipt of this command, a device SHALL move from its current level in
/// an up or down direction.
#define COMMAND_LEVEL_STEP                                0x02
/// Upon receipt of this command, any Move to Level, Move or Step command
/// (and their 'with On/Off' variants) currently in process SHALL be terminated.
#define COMMAND_LEVEL_STOP                                0x03
/// The Move to Level (with On/Off), command have identical payload to the Move
/// to Level command. Before commencing any command that has the effect of
/// setting the CurrentLevel above the minimum level allowed by the device, the
/// OnOff attribute of the On/Off cluster on the same endpoint, if implemented,
/// SHALL be set to On. If any command that has the effect of setting the
/// CurrentLevel to the minimum level allowed by the device, the  OnOff attribute
/// of the On/Off cluster on the same endpoint, if implemented, SHALL be set to Off.
#define COMMAND_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF           0x04
/// The Move (with On/Off), command have identical payload to the Move command.
/// Before commencing any command that has the effect of setting the CurrentLevel
/// above the minimum level allowed by the device, the OnOff attribute of the
/// On/Off cluster on the same endpoint, if implemented, SHALL be set to On.
/// If any command that has the effect of setting the CurrentLevel to the minimum
/// level allowed by the device, the  OnOff attribute  of the On/Off cluster on
/// the same endpoint, if implemented, SHALL be set to Off.
#define COMMAND_LEVEL_MOVE_WITH_ON_OFF                    0x05
/// The Step (with On/Off), command have identical payload to the Step command.
/// Before commencing any command that has the effect of setting the CurrentLevel
/// above the minimum level allowed by the device, the OnOff attribute of the
/// On/Off cluster on the same endpoint, if implemented, SHALL be set to On.
/// If any command that has the effect of setting the CurrentLevel to the minimum
/// level allowed by the device, the  OnOff attribute  of the On/Off cluster on
/// the same endpoint, if implemented, SHALL be set to Off.
#define COMMAND_LEVEL_STEP_WITH_ON_OFF                    0x06
/// The Stop (with On/Off), command have identical payload to the Stop command.
/// Before commencing any command that has the effect of setting the CurrentLevel
/// above the minimum level allowed by the device, the OnOff attribute of the
/// On/Off cluster on the same endpoint, if implemented, SHALL be set to On.
/// If any command that has the effect of setting the CurrentLevel to the minimum
/// level allowed by the device, the  OnOff attribute  of the On/Off cluster on
/// the same endpoint, if implemented, SHALL be set to Off.
#define COMMAND_LEVEL_STOP_WITH_ON_OFF                    0x07
/// Upon receipt of this command, the device shall change its current frequency
/// to the requested frequency, or to the 4785 closest frequency that it
/// can generate.
#define COMMAND_LEVEL_MOVE_TO_CLOSEST_FREQUENCY           0x08


/*** Level Control Move (Mode) Command values ***/
#define LEVEL_MOVE_UP                                     0x00
#define LEVEL_MOVE_DOWN                                   0x01

/*** Level Control Step (Mode) Command values ***/
#define LEVEL_STEP_UP                                     0x00
#define LEVEL_STEP_DOWN                                   0x01

/** @} End ZCL_GENERAL_LEVEL_CONTROL_CLUSTER */

/**
* @defgroup ZCL_GENERAL_ALARMS_CLUSTER ZCL Alarms Cluster
* @{
*/

/*********************************/
/*** Alarms Cluster Attributes ***/
/*********************************/
// Alarm Information
/// The AlarmCount attribute is 16 bits in length and specifies the number of
/// entries currently in the alarm table.
#define ATTRID_ALARMS_ALARM_COUNT                                0x0000

/*******************************/
/*** Alarms Cluster Commands ***/
/*******************************/
// Client generated commands
/// This command resets a specific alarm.
#define COMMAND_ALARMS_RESET_ALARM                        0x00
/// This command resets all alarms.
#define COMMAND_ALARMS_RESET_ALL_ALARMS                   0x01
/// This command causes the alarm with the earliest generated alarm entry in
/// the alarm table to be reported in a get alarm response command
#define COMMAND_ALARMS_GET_ALARM                          0x02
/// This command causes the alarm table to be cleared, and does not have
/// a payload.
#define COMMAND_ALARMS_RESET_ALARM_LOG                    0x03

// Server generated commands
/// The alarm command signals an alarm situation on the sending device.
#define COMMAND_ALARMS_ALARM                              0x00
/// The get alarm response command returns the results of a request to
/// retrieve information from the alarm log, along  with a time stamp indicating
/// when the alarm situation was detected.
#define COMMAND_ALARMS_GET_ALARM_RESPONSE                 0x01

/** @} End ZCL_GENERAL_ALARMS_CLUSTER */

/**
* @defgroup ZCL_GENERAL_TIME_CLUSTER ZCL Time Cluster
* @{
*/

/*******************************/
/*** Time Cluster Attributes ***/
/*******************************/
/// The Time attribute is 32 bits in length and holds the time value of a real
/// time clock.
#define ATTRID_TIME_TIME                                  0x0000
/// The TimeStatus attribute holds a number of bit fields for status.
#define ATTRID_TIME_TIME_STATUS                           0x0001
/// The TimeZone attribute indicates the local time zone, as a signed offset
/// in seconds from the Time attribute value.
#define ATTRID_TIME_TIME_ZONE                             0x0002
/// The DstStart attribute indicates the DST start time in seconds. The value
/// 0xffffffff indicates an invalid DST start time.
#define ATTRID_TIME_DST_START                             0x0003
/// The DstEnd attribute indicates the DST end time in seconds. The value
/// 0xffffffff indicates an invalid DST end time.
#define ATTRID_TIME_DST_END                               0x0004
/// The DstShift attribute represents a signed offset in seconds from the
/// standard time, to be applied between the times DstStart and DstEnd to
/// calculate the Local Time.
#define ATTRID_TIME_DST_SHIFT                             0x0005
/// The local Standard Time is given by the equation:
/// Standard Time = Time + TimeZone
#define ATTRID_TIME_STANDARD_TIME                         0x0006
/// The Local Time is given by the equation:
/// Local Time = Standard Time + DstShift (if DstStart <= Time <= DstEnd)
/// Local Time = Standard Time (if Time < DstStart or Time > DstEnd)
#define ATTRID_TIME_LOCAL_TIME                            0x0007
/// The LastSetTime attribute indicates the most recent time that the Time
/// attribute was set, either internally or over the network (thus it holds a
/// copy of the last value that Time was set to).
#define ATTRID_TIME_LAST_SET_TIME                         0x0008
///The ValidUntilTime attribute indicates a time, later than LastSetTime,
/// up to which the Time attribute MAY be trusted.
#define ATTRID_TIME_VALID_UNTIL_TIME                      0x0009

#define TIME_SECONDS_IN_ONE_DAY                       (60*60*24L) // one day in seconds
#define TIME_INVALID_TIME_ZONE                               -1L

  /*** DST Shift Range Values ***/
#define TIME_DST_SHIFT_MIN                                0xFFFEAE80
#define TIME_DST_SHIFT_MAX                                0x00015180

  /*** TimeStatus Attribute bits ***/
#define TIME_STATUS_MASTER                                0x01
#define TIME_STATUS_SYNCH                                 0x02
#define TIME_STATUS_MASTER_ZONE_DST                       0x04

/*****************************/
/*** Time Cluster Commands ***/
/*****************************/
  // No cluster specific commands

/** @} End ZCL_GENERAL_TIME_CLUSTER */

/**
* @defgroup ZCL_GENERAL_RSSI_LOCATION_CLUSTER ZCL RSSI Location Cluster
* @{
*/

/***********************************/
/*** RSSI Location Cluster Attributes ***/
/***********************************/
// Location Information
/// The LocationType attribute is 8 bits long and is divided into bit fields.
#define ATTRID_LOCATION_TYPE                              0x0000
/// The LocationMethod attribute hpdls the method value.
#define ATTRID_LOCATION_METHOD                            0x0001
/// The LocationAge attribute indicates the amount of time, measured in seconds,
/// that has transpired since the location information was last calculated.
#define ATTRID_LOCATION_AGE                               0x0002
/// The QualityMeasure attribute is a measure of confidence in the corresponding
/// location information.
#define ATTRID_LOCATION_QUALITY_MEASURE                   0x0003
/// The NumberOfDevices attribute is the number of devices whose location data
/// were used to calculate the last location value.
#define ATTRID_LOCATION_NUM_DEVICES                       0x0004

// Location Settings
/// Represent orthogonal linear coordinates x in meters as coordenate1 / 10.
#define ATTRID_LOCATION_COORDINATE1                       0x0010
/// Represent orthogonal linear coordinates y in meters as coordenate2 / 10.
#define ATTRID_LOCATION_COORDINATE2                       0x0011
/// Represent orthogonal linear coordinates z in meters as coordenate3 / 10.
#define ATTRID_LOCATION_COORDINATE3                       0x0012
/// The Power attribute specifies the value of the average power P0, measured in
/// dBm, received at a reference distance of one meter from the transmitter.
#define ATTRID_LOCATION_POWER                             0x0013
/// The PathLossExponent attribute specifies the value of the Path Loss Exponent
/// n, an exponent that describes the rate at which the signal power decays with
/// increasing distance from the transmitter.
#define ATTRID_LOCATION_PATH_LOSS_EXPONENT                0x0014
/// The ReportingPeriod attribute specifies the time in seconds between
/// successive reports of the device's location by means of the Location Data
/// Notification command.
#define ATTRID_LOCATION_REPORT_PERIOD                     0x0015
/// The CalculationPeriod attribute specifies the time in milliseconds between
/// successive calculations of the device's location.
#define ATTRID_LOCATION_CALC_PERIOD                       0x0016
/// The NumberRSSIMeasurements attribute specifies the number of RSSI
///measurements to be used to generate one location estimate.
#define ATTRID_LOCATION_NUM_RSSI_MEASUREMENTS             0x0017

/*** Location Type attribute bits ***/
#define LOCATION_TYPE_ABSOLUTE                            0x01
#define LOCATION_TYPE_2_D                                 0x02
#define LOCATION_TYPE_COORDINATE_SYSTEM                   0x0C

/*** Location Method attribute values ***/
#define LOCATION_METHOD_LATERATION                        0x00
#define LOCATION_METHOD_SIGNPOSTING                       0x01
#define LOCATION_METHOD_RF_FINGER_PRINT                   0x02
#define LOCATION_METHOD_OUT_OF_BAND                       0x03
#define LOCATION_METHOD_CENTRALIZED                       0x04

/*********************************/
/*** Location Cluster Commands ***/
/*********************************/
/// This command is used to set a device's absolute (known, not calculated)
/// location and the channel parameters corresponding to that location.
#define COMMAND_LOCATION_SET_ABSOLUTE                      0x00
/// This command is used to set a device's location parameters, which will be
/// used for calculating and reporting measured 5098 location. This command is
/// invalid unless the Absolute bit of the LocationType attribute has a value of 0.
#define COMMAND_LOCATION_SET_DEV_CFG                       0x01
/// This command is used to request the location parameters of a device.
#define COMMAND_LOCATION_GET_DEV_CFG                       0x02
/// This command is used to request a device's location information and
/// channel parameters.
#define COMMAND_LOCATION_GET_DATA                          0x03
/// This command is sent by a device in response to an RSSI Request command.
#define COMMAND_LOCATION_RSSI_RESPONSE                     0x04
/// This command is used to alert a node to start sending multiple packets so
/// that all its one-hop neighbors can calculate the mean RSSI value of the
/// radio link.
#define COMMAND_LOCATION_SEND_PINGS                        0x05
/// This command is sent by an anchor node when it joins the network, if it
/// is already commissioned with the coordinates, to announce itself so that
/// the central device knows the exact position of that device.
#define COMMAND_LOCATION_ANCHOR_NODE_ANNOUNCE              0x06

/// This command is sent by a device in response to a Get Device
/// Configuration command.
#define COMMAND_LOCATION_DEV_CFG_RSP                       0x00
/// This command is sent by a device in response to a request for location
/// information and channel parameters.
#define COMMAND_LOCATION_DATA_RSP                          0x01
/// This command is sent periodically by a device to announce its location
/// information and channel parameters.
#define COMMAND_LOCATION_DATA_NOTIF                        0x02
/// This command is identical in format and use to the Location Data
/// Notification command, except that the Power, Path Loss Exponent and
/// Location Method fields are not included.
#define COMMAND_LOCATION_COMPACT_DATA_NOTIF                0x03
/// This command is sent periodically by a device to enable listening devices
/// to measure the received signal strength in the absence of other transmissions
/// from that device. The period is given by the ReportingPeriod attribute.
#define COMMAND_LOCATION_RSSI_PING                         0x04
/// A device uses this command to ask one, more, or all its one-hop neighbors
/// for the (mean) RSSI value they hear from itself.
#define COMMAND_LOCATION_RSSI_REQUEST                      0x05
/// This command is sent by a device to report its measurements of the link
/// between itself and one or more neighbors.
#define COMMAND_LOCATION_RSSI_REPORT_RSSI_MEASUREMENTS     0x06

#define COMMAND_LOCATION_REQUEST_OWN                        0x07
/** @} End ZCL_GENERAL_RSSI_LOCATION_CLUSTER */

/**
* @defgroup ZCL_GENERAL_BASIC_IO_CLUSTER ZCL Basic IO Cluster
* @{
*/

/**********************************************************/
/*** Input, Output and Value (Basic) Cluster Attributes ***/
/**********************************************************/
/// This attribute, of type Character string, MAY be used to hold a human
/// readable description of the ACTIVE state of a binary PresentValue.
#define ATTRID_IOV_BASIC_ACTIVE_TEXT                        0x0004
/// This attribute, of type Array of Character strings, holds descriptions of
/// all possible states of a multistate PresentValue.
#define ATTRID_IOV_BASIC_STATE_TEXT                         0x000E
/// The Description attribute, of type Character string, MAY be used to hold a
/// description of the usage of the input, output or value, as appropriate to
/// the cluster.
#define ATTRID_IOV_BASIC_DESCRIPTION                        0x001C
/// This attribute, of type Character string, MAY be used to hold a human
/// readable description of the INACTIVE state of a binary PresentValue.
#define ATTRID_IOV_BASIC_INACTIVE_TEXT                      0x002E
/// The MaxPresentValue attribute, of type Single precision, indicates the
/// highest value that can be reliably obtained for the PresentValue attribute
/// of an Analog Input cluster, or which can reliably be used for the PresentValue
/// attribute of an Analog Output or Analog Value cluster.
#define ATTRID_IOV_BASIC_MAX_PRESENT_VALUE                  0x0041
/// This property, of type 32-bit unsigned integer, represents the minimum
/// number of seconds that a binary PresentValue SHALL remain in the INACTIVE
/// state after a write to PresentValue causes it to assume the INACTIVE state.
#define ATTRID_IOV_BASIC_MIN_OFF_TIME                       0x0042
/// This property, of type 32-bit unsigned integer, represents the minimum
/// number of seconds that a binary PresentValue SHALL remain in the ACTIVE state
/// after a write to PresentValue causes it to assume the ACTIVE state.
#define ATTRID_IOV_BASIC_MIM_ON_TIME                        0x0043
/// The MinPresentValue attribute, of type Single precision, indicates the
/// lowest value that can be reliably obtained for the PresentValue attribute of
/// an Analog Input cluster, or which can reliably be used for the PresentValue
/// attribute of an Analog Output or Analog Value cluster.
#define ATTRID_IOV_BASIC_MIN_PRESENT_VALUE                  0x0045
/// This attribute, of type Unsigned 16-bit integer, defines the number of
/// states that a multistate PresentValue MAY have.
#define ATTRID_IOV_BASIC_NUM_OF_STATES                      0x004A
/// The OutOfService attribute, of type Boolean, indicates whether (TRUE) or
/// not (FALSE) the physical input, output or value that the cluster represents
/// is not in service.
#define ATTRID_IOV_BASIC_OUT_OF_SERVICE                     0x0051
/// This attribute, of type enumeration, indicates the relationship between the
/// physical state of the input (or output as appropriate for the cluster) and
/// the logical state represented by a binary PresentValue attribute, when
/// OutOfService is FALSE. If the Polarity attribute is NORMAL (0), then the
/// ACTIVE (1) state of the PresentValue attribute is also the ACTIVE or ON
/// state of the physical input (or output). If the Polarity attribute is
/// REVERSE (1), then the ACTIVE (1) state of the PresentValue attribute is the
/// INACTIVE or OFF state of the physical input (or output).
#define ATTRID_IOV_BASIC_POLARITY                           0x0054
/// The PresentValue attribute indicates the current value of the input,
/// output or value, as appropriate for the cluster.
#define ATTRID_IOV_BASIC_PRESENT_VALUE                      0x0055
/// The PriorityArray attribute is an array of 16 structures. The first element
/// of each structure is a Boolean, and the second element is of the same
/// type as the PresentValue attribute of the corresponding cluster.
#define ATTRID_IOV_BASIC_PRIORITY_ARRAY                     0x0057
/// The Reliability attribute, of type 8-bit enumeration, provides an indication
/// of whether the PresentValue or the operation of the physical input, output
/// or value in question (as appropriate for the cluster) is "reliable" as far
/// as can be 5590 determined and, if not, why not.
#define ATTRID_IOV_BASIC_RELIABILITY                        0x0067
/// The RelinquishDefault attribute is the default value to be used for the
/// PresentValue attribute when all elements of the PriorityArray attribute are
/// marked as invalid.
#define ATTRID_IOV_BASIC_RELINQUISH_DEFAULT                 0x0068
/// This attribute, of type Single precision, indicates the smallest recognizable
/// change to PresentValue.
#define ATTRID_IOV_BASIC_RESOLUTION                         0x006A
///This attribute, of type bitmap, represents four Boolean flags that indicate
/// the general "health" of the analog sensor.
#define ATTRID_IOV_BASIC_STATUS_FLAG                        0x006F
/// The EngineeringUnits attribute indicates the physical units associated with
/// the value of the PresentValue attribute of an Analog cluster.
#define ATTRID_IOV_BASIC_ENGINEERING_UNITS                  0x0075
/// The ApplicationType attribute is an unsigned 32 bit integer that indicates
/// the specific application usage for this cluster.
#define ATTRID_IOV_BASIC_APP_TYPE                           0x0100

/**********************************************************/
/*** Appliance Control Cluster Attributes ***/
/**********************************************************/
#ifdef ZCL_APPLIANCE_CONTROL
/// StartTime attribute determines the time (either relative or absolute) of
/// the start of the machine activity.
#define ATTRID_APPLIANCE_CONTROL_START_TIME                 0x0000
/// FinishTime attribute determines the time (either relative or absolute) of
/// the expected end of the machine activity.
#define ATTRID_APPLIANCE_CONTROL_FINISH_TIME                0x0001
/// RemainingTime attribute determines the time, in relative format, of the
/// remaining time of the machine cycle.
#define ATTRID_APPLIANCE_CONTROL_REMAINING_TIME             0x0002
#endif

/*** StatusFlags attribute bits ***/
#define STATUS_FLAGS_IN_ALARM                               0x01
#define STATUS_FLAGS_FAULT                                  0x02
#define STATUS_FLAGS_OVERRIDDEN                             0x04
#define STATUS_FLAGS_OUT_OF_SERVICE                         0x08

/*** Reliability attribute types ***/
#define RELIABILITY_NO_FAULT_DETECTED                       0x00
#define RELIABILITY_NO_SENSOR                               0x01
#define RELIABILITY_OVER_RANGE                              0x02
#define RELIABILITY_UNDER_RANGE                             0x03
#define RELIABILITY_OPEN_LOOP                               0x04
#define RELIABILITY_SHORTED_LOOP                            0x05
#define RELIABILITY_NO_OUTPUT                               0x06
#define RELIABILITY_UNRELIABLE_OTHER                        0x07
#define RELIABILITY_PROCESS_ERROR                           0x08
#define RELIABILITY_MULTI_STATE_FAULT                       0x09
#define RELIABILITY_CONFIG_ERROR                            0x0A

/*** EngineeringUnits attribute values ***/
// Values 0x0000 to 0x00fe are reserved for the list of engineering units with
// corresponding values specified in Clause 21 of the BACnet standard.

#define ENGINEERING_UNITS_OTHER                             0x00FF

// Values 0x0100 to 0xffff are available for proprietary use.

/*** Polarity attribute values ***/
#define POLARITY_NORMAL                                     0x00
#define POLARITY_REVERSE                                    0x01

/*** ApplicationType attribute bits ***/
// ApplicationType is subdivided into Group, Type and an Index number.

// Application Group = Bits 24 - 31. An indication of the cluster this
// attribute is part of.
#define APP_GROUP                                           0xFF000000

// Application Type = Bits 16 - 23. For Analog clusters, the physical
// quantity that the Present Value attribute of the cluster represents.
// For Binary and Multistate clusters, the application usage domain.
#define APP_TYPE                                            0x00FF0000

// Application Index = Bits 0 - 15. The specific application usage of
// the cluster
#define APP_INDEX                                           0x0000FFFF

/*** Application Groups ***/
#define APP_GROUP_AI                                        0x00 // Analog Input
#define APP_GROUP_AO                                        0x01 // Analog Output
#define APP_GROUP_AV                                        0x02 // Analog Value
#define APP_GROUP_BI                                        0x03 // Binary Input
#define APP_GROUP_BO                                        0x04 // Binary Output
#define APP_GROUP_BV                                        0x05 // Binary Value
#define APP_GROUP_MI                                        0x0D // Multistate Input
#define APP_GROUP_MO                                        0x0E // Multistate Output
#define APP_GROUP_MV                                        0x13 // Multistate Value

/*** Application Types ***/

// Analog Input (AI) Types:
//   Group = 0x00.
//   Types = 0x00 - 0x0E.
//   Types 0x0F to 0xFE are reserved, Type = 0xFF indicates other.
#define APP_TYPE_AI_TEMP_C                                  0x00 // Temperature in degrees C
#define APP_TYPE_AI_HUMIDITY_PERCENT                        0x01 // Relative humidity in %
#define APP_TYPE_AI_PRESSURE_PASCAL                         0x02 // Pressure in Pascal
#define APP_TYPE_AI_FLOW_LIT_SEC                            0x03 // Flow in liters/second
#define APP_TYPE_AI_PERCENT                                 0x04 // Percentage %
#define APP_TYPE_AI_PARTS_PER_MIL                           0x05 // Parts per Million PPM
#define APP_TYPE_AI_ROTATION_SPEED                          0x06 // Rotational Speed in RPM
#define APP_TYPE_AI_CURRENT_AMPS                            0x07 // Current in Amps
#define APP_TYPE_AI_FREQUENCY_HZ                            0x08 // Frequency in Hz
#define APP_TYPE_AI_PWR_WATTS                               0x09 // Power in Watts
#define APP_TYPE_AI_PWR_KW                                  0x0A // Power in kW
#define APP_TYPE_AI_ENERGY_KWH                              0x0B // Energy in kWH
#define APP_TYPE_AI_COUNT                                   0x0C // Count - Unitless
#define APP_TYPE_AI_ENTHALPY_KJ_KG                          0x0D // Enthalpy in KJoules/Kg
#define APP_TYPE_AI_TIME_SEC                                0x0E // Time in Seconds

// Analog Output (AO) Types:
//   Group = 0x01.
//   Types = 0x00 - 0x0E.
//   Types 0x0F to 0xFE are reserved, Type = 0xFF indicates other.
#define APP_TYPE_AO_TEMP_C                                  0x00 // Temperature in degrees C
#define APP_TYPE_AO_HUMIDITY_PERCENT                        0x01 // Relative Humidity in %
#define APP_TYPE_AO_PRESSURE_PASCAL                         0x02 // Pressure in Pascal
#define APP_TYPE_AO_FLOW_LIT_SEC                            0x03 // Flow in liters/second
#define APP_TYPE_AO_PERCENT                                 0x04 // Percentage %
#define APP_TYPE_AO_PARTS_PER_MIL                           0x05 // Parts per Million PPM
#define APP_TYPE_AO_ROTATION_SPEED                          0x06 // Rotational Speed in RPM
#define APP_TYPE_AO_CURRENT_AMPS                            0x07 // Current in Amps
#define APP_TYPE_AO_FREQUENCY_HZ                            0x08 // Frequency in Hz
#define APP_TYPE_AO_PWR_WATTS                               0x09 // Power in Watts
#define APP_TYPE_AO_PWR_KW                                  0x0A // Power in kW
#define APP_TYPE_AO_ENERGY_KWH                              0x0B // Energy in kWH
#define APP_TYPE_AO_COUNT                                   0x0C // Count - Unitless
#define APP_TYPE_AO_ENTHALPY_KJ_KG                          0x0D // Enthalpy in KJoules/Kg
#define APP_TYPE_AO_TIME_SEC                                0x0E // Time in Seconds

// Analog Value (AV) Types:
//   Group = 0x02.
//   Types = 0x00 - 0x03.
//   Types 0x04 to 0xFE are reserved, Type = 0xFF indicates other.
#define APP_TYPE_AV_TEMP_C                                  0x00 // Temperature in Degrees C
#define APP_TYPE_AV_AREA_SQ_METER                           0x01 // Area in Square Metres
#define APP_TYPE_AV_MULTIPLIER_NUM                          0x02 // Multiplier - Number
#define APP_TYPE_AV_FLOW_LIT_SEC                            0x03 // Flow in Litres/Second

// Binary Input (BI) Types:
//   Group = 0x03.
//   Types = 0x00 - 0x01.
//   Types 0x02 to 0xFE are reserved, Type = 0xFF indicates other.
//   Present Value = 0 represents False, Off, Normal
//   Present Value = 1 represents True, On, Alarm
#define APP_TYPE_BI_DOMAIN_HVAC                             0x00 // Application Domain HVAC
#define APP_TYPE_BI_DOMAIN_SEC                              0x01 // Application Domain Security

// Binary Output (BO) Types:
//   Group = 0x04.
//   Types = 0x00 - 0x01.
//   Types 0x02 to 0xFE are reserved, Type = 0xFF indicates other.
//   Present Value = 0 represents False, Off, Normal
//   Present Value = 1 represents True, On, Alarm
#define APP_TYPE_BO_DOMAIN_HVAC                             0x00 // Application Domain HVAC
#define APP_TYPE_BO_DOMAIN_SEC                              0x02 // Application Domain Security

// Binary Value (BV) Types:
//   Group = 0x05.
//   Type = 0x00.
//   Types 0x01 to 0xFE are reserved, Type = 0xFF indicates other.
//   Present Value = 0 represents False, Off, Normal
//   Present Value = 1 represents True, On, Alarm
#define APP_TYPE_BV                                         0x00 // Type = 0x00

// Multistate Input (MI) Types:
//   Group = 0x0D.
//   Type = 0x00.
//   Types 0x01 to 0xFE are reserved, Type = 0xFF indicates other.
#define APP_TYPE_MI_DOMAIN_HVAC                             0x00 // Application Domain HVAC

// Multistate Output (MO) Types:
//   Group = 0x0E.
//   Type = 0x00.
//   Types 0x01 to 0xFE are reserved, Type = 0xFF indicates other.
#define APP_TYPE_MO_DOMAIN_HVAC                             0x00 // Application Domain HVAC

// Multistate Value (MV) Types:
//   Group = 0x13.
//   Type = 0x00.
//   Types 0x01 to 0xFE are reserved, Type = 0xFF indicates other.
#define APP_TYPE_MV_DOMAIN_HVAC                             0x00 // Application Domain HVAC

/*** Application Indexes ***/

// Analog Input (AI) Indexes
//   Group = 0x00.

// AI Temperature in degrees C Indexes:
//   Type = 0x00.
//   Indexes = 0x0000 - 0x003C.
//   Indexed 0x003D - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_TEMP_2_PIPE_ENTER_WATER                0x00 // 2 Pipe Entering Water Temperature AI

// AI Relative humidity in % Indexes:
//   Type = 0x01.
//   Indexes = 0x0000 - 0x0008.
//   Indexed 0x0009 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_HUMIDITY_DISCHARGE                     0x00 // Discharge Humidity AI

// AI Pressure in Pascal Indexes:
//   Type = 0x02.
//   Indexes = 0x0000 - 0x001E.
//   Indexed 0x001F - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_PRESSURE_BOIL_PUMP_DIFF                0x00 // Boiler Pump Differential Pressure AI

// AI Flow in liters/second Indexes:
//   Type = 0x03.
//   Indexes = 0x0000 - 0x0015.
//   Indexed 0x0016 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_FLOW_CHILLED_WATER                     0x00 // Chilled Water Flow AI

// AI Percentage % Indexes:
//   Type = 0x04.
//   Index = 0x0000.
//   Indexed 0x0001 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_PERCENT_CHILLER_FULL_LOAD_AMP          0x00 // Chiller % Full Load Amperage AI

// AI Parts per Million PPM Indexes:
//   Type = 0x05.
//   Indexes = 0x0000 - 0x0001.
//   Indexed 0x0002 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_PARTS_RETURN_CO2                       0x00 // Return Carbon Dioxide AI

// AI Rotational Speed in RPM Indexes:
//   Type = 0x06.
//   Indexes = 0x0000 - 0x0007.
//   Indexed 0x0008 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_ROTATION_EXHAUST_FAN_REMOTE            0x00 // Exhaust Fan Remote Speed AI

// AI Current in Amps Indexes:
//   Type = 0x07.
//   Index = 0x0000.
//   Indexed 0x0001 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_CURRENT_AMPS_CHILLER                   0x00 // Chiller Amps AI

// AI Frequency in Hz Indexes:
//   Type = 0x08.
//   Index = 0x0000.
//   Indexed 0x0001 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_FREQUENCY_SPEED_DRIVE_OUTPUT           0x00 // Variable Speed Drive Output Frequency AI

// AI Power in Watts Indexes:
//   Type = 0x09.
//   Index = 0x0000.
//   Indexed 0x0001 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_PWR_WATTS_CONSUMPTION                  0x00 // Power Consumption AI

// AI Power in kW Indexes:
//   Type = 0x0A.
//   Indexes = 0x0000 - 0x0001.
//   Indexed 0x0002 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_PWR_KW_ABSOLUTE                        0x00 // Absolute Power AI

// AI Energy in kWH Indexes:
//   Type = 0x0B.
//   Index = 0x0000.
//   Indexed 0x0001 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_ENERGY_KWH_SPEED_DRIVE                 0x00 // Variable Speed Drive Kilowatt Hours AI

// AI Count - Unitless Indexes:
//   Type = 0x0C.
//   Index = 0x0000.
//   Indexed 0x0001 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_COUNT                                  0x00 // Count

// AI Enthalpy in KJoules/Kg Indexes:
//   Type = 0x0D.
//   Indexes = 0x0000 - 0x0002.
//   Indexed 0x0003 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_ENTHALPY_OUTDOOR_AIR                   0x00 // Outdoor Air Enthalpy AI

// AI Time in Seconds Indexes:
//   Type = 0x0E.
//   Index = 0x0000.
//   Indexed 0x0001 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AI_TIME_RELATIVE                          0x00 // Relative time AI


// Analog Output (AO) types
//   Group = 0x01.

// AO Temperature in degrees C Indexes:
//   Type = 0x00.
//   Indexes = 0x0000 - 0x0009.
//   Indexed 0x000A - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AO_TEMP_BOILER                            0x00 // Boiler AO

// AO Relative Humidity in % Indexes:
//   Type = 0x01.
//   Indexes = 0x0000 - 0x0001.
//   Indexed 0x0002 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AO_HUMIDITY_HUMIDIFICATION                0x00 // Humidification AO

// AO Pressure in Pascal Indexes:
//   Type = 0x02.
//   Indexed 0x0000 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.

// AO Flow in liters/second Indexes:
//   Type = 0x03.
//   Indexed 0x0000 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.

// AO Percentage % Indexes:
//   Type = 0x04.
//   Indexes = 0x0000 - 0x002D.
//   Indexed 0x002E - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AO_PERCENT_FACE_BYPASS_DAMPER             0x00 // Face & Bypass Damper AO

// AO Parts per Million PPM Indexes:
//   Type = 0x05.
//   Index = 0x0000.
//   Indexed 0x0001 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AO_PARTS_SPACE_CO2_LIMIT                  0x00 // Space Carbon Dioxide limit AO

// AO Rotational Speed in RPM Indexes:
//   Type = 0x06.
//   Indexes = 0x0000 - 0x0004.
//   Indexed 0x0005 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AO_ROTATION_EXHAUST_FAN_SPEED             0x00 // Exhaust Fan Speed AO

// AO Current in Amps Indexes:
//   Type = 0x07.
//   Indexed 0x0000 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.

// AO Frequency in Hz Indexes:
//   Type = 0x08.
//   Indexed 0x0000 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.

// AO Power in Watts Indexes:
//   Type = 0x09.
//   Indexed 0x0000 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.

// AO Power in kW Indexes:
//   Type = 0x0A.
//   Indexed 0x0000 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.

// AO Energy in kWH Indexes:
//   Type = 0x0B.
//   Indexed 0x0000 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.

// AO Count - Unitless Indexes:
//   Type = 0x0C.
//   Indexed 0x0000 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.

// AO Enthalpy in KJoules/Kg Indexes:
//   Type = 0x0D.
//   Indexed 0x0000 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.

// AO Time in Seconds Indexes:
//   Type = 0x0E.
//   Index = 0x0000.
//   Indexed 0x0001 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AO_TIME_RELATIVE                          0x00 // Relative time AO


// Analog Value (AV) types
//   Group = 0x02.

// AV Temperature in Degrees C Indexes:
//   Type = 0x00.
//   Indexes = 0x0000 - 0x000F.
//   Indexed 0x0010 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AV_TEMP_SETPOINT_OFFSET                   0x00 // Setpoint Offset AV

// AV Area in Square Metres Indexes:
//   Type = 0x01.
//   Index = 0x0000.
//   Indexed 0x0001 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AV_AREA_DUCT                              0x00 // Duct Area AV

// AV Multiplier - Number Indexes:
//   Type = 0x02.
//   Index = 0x0000.
//   Indexed 0x0001 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AV_MULTIPLIER_GAIN                        0x00 // Gain multiplier AV

// AV Flow in Litres/Second Indexes:
//   Type = 0x03.
//   Indexes = 0x0000 - 0x0005.
//   Indexed 0x0006 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_AV_FLOW_MIN_AIR                           0x00 // Minimum Air Flow AV


// Binary Input (BI) types
//   Group = 0x03.

// BI Application Domain HVAC Indexes:
//   Type = 0x00.
//   Indexes = 0x0000 - 0x0094.
//   Indexed 0x0095 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_BI_DOMAIN_HVAC_2_PIPE_PUMP_STAT           0x00 // 2 Pipe Pump Status BI

// BI Application Domain Security Indexes:
//   Type = 0x01.
//   Indexes = 0x0000 - 0x0008.
//   Indexed 0x0009 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_BI_DOMAIN_SEC_GLASS_BREAK_DETECT          0x00 // Glass Breakage Detection


// Binary Output (BO) types
//   Group = 0x04.

// BO Application Domain HVAC Indexes:
//   Type = 0x00.
//   Indexes = 0x0000 - 0x0076.
//   Indexed 0x0078 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_BO_DOMAIN_HVAC_2_PIPE_CIR_PUMP            0x00 // 2 Pipe Circulation Pump BO

// BO Application Domain Security Indexes:
//   Type = 0x02.
//   Indexes = 0x0000 - 0x0003.
//   Indexed 0x0004 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_BO_DOMAIN_SEC_ARM_DISARM_CMD              0x00 // Arm Disarm Command BO


// Binary Value (BV) types
//   Group = 0x05.

// BV Type Indexes:
//   Type = 0x00.
//   Indexed 0x0000 - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.

// Multistate Input (MI) types
//   Group = 0x0D.

// MI Application Domain HVAC Indexes:
//   Type = 0x00.
//   Indexes = 0x0000 - 0x000B.
//   Indexed 0x000C - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_MI_DOMAIN_HVAC_OFF_ON_AUTO                0x00 // Off, On, Auto


// Multistate Output (MO)types
//   Group = 0x0E.

// MO Application Domain HVAC Indexes:
//   Type = 0x00.
//   Indexes = 0x0000 - 0x000B.
//   Indexed 0x000C - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_MO_DOMAIN_HVAC_OFF_ON_AUTO                0x00 // Off, On, Auto


// Multistate Value (MV) types
//   Group = 0x13.

// MV Application Domain HVAC Indexes:
//   Type = 0x00.
//   Indexes = 0x0000 - 0x000B.
//   Indexed 0x000C - 0x01FF are reserved, 0x0200 - 0xFFFE are Vendor
//   defined, Index = 0xFFFF indicates other.
#define APP_INDEX_MV_DOMAIN_HVAC_OFF_ON_AUTO                0x00 // Off, On, Auto

/** @} End ZCL_GENERAL_BASIC_IO_CLUSTER */

// The maximum number of characters to allow in a scene's name
// remember that the first byte is the length
#define ZCL_GENERAL_SCENE_NAME_LEN                           16


#ifdef ZCL_ON_OFF
//for On/Off cluster (onOff attibute)
#define ZCL_ON_OFF_EXTENSION_SIZE  (2 + 1 + 1)
#else
#define ZCL_ON_OFF_EXTENSION_SIZE   0
#endif

#ifdef ZCL_LEVEL_CTRL
//for Level Control cluster (currentLevel attribute)
#define ZCL_LEVEL_CTRL_EXTENSION_SIZE (2 + 1 + 1)
#else
#define ZCL_LEVEL_CTRL_EXTENSION_SIZE  0
#endif

#ifdef ZCL_LIGHTING
//for Color Control cluster (currentX/currentY/EnhancedCurrentHue/CurrentSaturation/colorLoopActive/colorLoopDirection/colorLoopTime attributes)
#define ZCL_LIGHTING_EXTENSION_SIZE  (2 + 1 + 11)
#else
#define ZCL_LIGHTING_EXTENSION_SIZE  0
#endif


#ifdef ZCL_DOORLOCK
//for Door Lock cluster (Lock State attribute)
#define ZCL_DOORLOCK_EXTENSION_SIZE (2 + 1 + 1)
#else
#define ZCL_DOORLOCK_EXTENSION_SIZE 0
#endif

#ifdef ZCL_WINDOWCOVERING
//for Window Covering cluster (LiftPercentage/TiltPercentage attributes)
#define ZCL_WINDOWCOVERING_EXTENSION_SIZE (2 + 1 + 2)
#else
#define ZCL_WINDOWCOVERING_EXTENSION_SIZE 0
#endif

#if !defined ( ZCL_GENERAL_SCENE_EXT_LEN )
// Scene extension length is defined by:
// Cluster ID + Attribute ID + Attribute data
// The maximum length of the scene extension field:
#define ZCL_GENERAL_SCENE_EXT_LEN   (ZCL_ON_OFF_EXTENSION_SIZE + ZCL_LEVEL_CTRL_EXTENSION_SIZE + ZCL_LIGHTING_EXTENSION_SIZE + ZCL_DOORLOCK_EXTENSION_SIZE + ZCL_WINDOWCOVERING_EXTENSION_SIZE)
#endif  //!defined ( ZCL_GENERAL_SCENE_EXT_LEN )

// The maximum number of entries in the Scene table
#if !defined ( ZCL_GENERAL_MAX_SCENES )
#define ZCL_GENERAL_MAX_SCENES                               5
#endif

#if (ZCL_GENERAL_MAX_SCENES > 16)
#warning: "According to latest ZCL version 7, secction 3.7.2.3.2 Maximum Number of Scenes: The number of scenes capable of being stored in the table is defined by the profile in which this cluster is used. The default maximum, in the absence of specification by the profile, is 16."
#endif

/*********************************************************************
 * TYPEDEFS
 */
// The format of a Scene Table Entry
typedef struct s_zb_zcl_general_scene
{
    uint16_t group_id;
    uint8_t scene_id;
    uint16_t trans_time;
    uint16_t trans_time_100ms;
    uint8_t name[ZCL_GENERAL_SCENE_NAME_LEN];
    uint8_t ext_len;
    uint8_t ext_field[ZCL_GENERAL_SCENE_EXT_LEN];
} s_zb_zcl_general_scene_t;

// The format of an Update Commission State Command Payload
typedef struct s_zb_zcl_identify_update_comm_state
{
    uint8_t action;
    uint8_t commission_state_mask;
} s_zb_zcl_identify_update_comm_state_t;

// The format of an Alarm Table entry
typedef struct s_zb_zcl_general_alarm
{
    uint8_t code;
    uint16_t cluster_id;
    uint32_t timestamp;
} s_zb_zcl_general_alarm_t;

// Structures used for callback functions
typedef struct s_zb_zcl_identify
{
    s_zb_af_address_t *src_addr; // requestor's address
    uint16_t identify_time;                 // number of seconds the device will continue to identify itself
} s_zb_zcl_identify_t;

typedef struct s_zb_zcl_identify_query
{
    s_zb_af_address_t *src_addr; // requestor's address
} s_zb_zcl_identify_query_t;

typedef struct s_zb_zcl_identify_trigger_effect
{
    s_zb_af_address_t *src_addr; // requestor's address
    uint8_t effect_id;                      // identify effect to use
    uint8_t effect_variant;                 // which variant of effect to be triggered
} s_zb_zcl_identify_trigger_effect_t;

typedef struct s_zb_zcl_identify_query_rsp
{
    s_zb_af_address_t *src_addr; // requestor's address
    uint16_t timeout;                       // number of seconds the device will continue to identify itself
} s_zb_zcl_identify_query_rsp_t;

typedef struct s_zb_zcl_identify_off_with_effect
{
    s_zb_af_address_t *src_addr; // requestor's address
    uint8_t effect_id;                      // identify effect to use
    uint8_t effect_variant;                 // which variant of effect to be triggered
} s_zb_zcl_identify_off_with_effect_t;

typedef union u_zb_zcl_on_off_ctrl
{
    struct
    {
        uint8_t accept_only_when_on:1;
        uint8_t reserved:7;
    } bits;
    uint8_t byte;
} s_zb_zcl_on_off_ctrl_t;

typedef struct s_zb_zcl_on_with_time_off
{
    s_zb_zcl_on_off_ctrl_t on_off_ctrl;  // how the lamp is to be operated
    uint16_t on_time;                               // the length of time (in 1/10ths second) that the lamp is to remain on, before automatically turning off
    uint16_t off_wait_time;                         // the length of time (in 1/10ths second) that the lamp shall remain off, and guarded to prevent an on command turning the light back on
} s_zb_zcl_on_with_time_off_t;

typedef struct s_zb_zcl_level_ctrl_move
{
    uint8_t level;              // new level to move to
    uint16_t transition_time;   // time to take to move to the new level (in seconds)
    uint8_t with_on_off;        // with on/off command
} s_zb_zcl_level_ctrl_move_to_level_t;

typedef struct
{
    uint8_t move_mode;          // Move mode which is either LEVEL_MOVE_STOP, LEVEL_MOVE_UP,
                                // LEVEL_MOVE_ON_AND_UP, LEVEL_MOVE_DOWN, LEVEL_MOVE_DOWN_AND_OFF
    uint8_t rate;               // rate of movement in steps per second
    uint8_t with_on_off;        // with on/off command
} s_zb_zcl_level_ctrl_move_t;

typedef struct s_zb_zcl_level_ctrl_step
{
    uint8_t step_mode;          // Step mode which is either LEVEL_STEP_UP, LEVEL_STEP_ON_AND_UP,
                                // LEVEL_STEP_DOWN, or LEVEL_STEP_DOWN_AND_OFF
    uint8_t amount;             // number of levels to step
    uint16_t trans_time;        // time, in 1/10ths of a second, to take to perform the step
    uint8_t with_on_off;        // with on/off command
} s_zb_zcl_level_ctrl_step_t;

typedef struct s_zb_zcl_level_ctrl_stop
{
    uint8_t with_on_off;        // with on/off command
} s_zb_zcl_level_ctrl_stop_t;

typedef struct s_zb_zcl_level_ctrl_move_freq
{
    uint16_t freq;              // New frequency to move to
} s_zb_zcl_level_ctrl_move_freq_t;

typedef struct s_zb_zcl_groups_rsp
{
    s_zb_af_address_t *src_addr; // requestor's address
    uint8_t cmd_id;                         // which group message - COMMAND_GROUPS_ADD_GROUP_RESPONSE, COMMAND_GROUPS_VIEW_GROUP_RESPONSE,
                                            // COMMAND_GROUPS_REMOVE_GROUP_RESPONSE or COMMAND_GROUPS_GET_GROUP_MEMBERSHIP_RESPONSE
    uint8_t status;                         // GROUP_STATUS_SUCCESS, GROUP_STATUS_TABLE_FULL, GROUP_STATUS_ALREADY_IN_TABLE,
                                            // GROUP_STATUS_NOT_IN_TABLE. Not valid for COMMAND_GROUPS_GET_GROUP_MEMBERSHIP_RESPONSE
    uint8_t group_count;                    // number of groups contained in group list
    uint16_t *group_list;                   // what group IDs the action was performed on
    uint8_t capacity;                       // remaining capacity of group table
    uint8_t *group_name;                    // only valid for COMMAND_GROUPS_VIEW_GROUP_RESPONSE
} s_zb_zcl_groups_rsp_t;

typedef struct s_zb_zcl_scene_req
{
    s_zb_af_address_t *src_addr;     // requestor's address
    s_zb_zcl_general_scene_t *scene;  // pointer to the scene structure
} s_zb_zcl_scene_req_t;

typedef struct s_zb_zcl_scene_rsp
{
    s_zb_af_address_t *src_addr;     // requestor's address
    uint8_t cmd_id;                             // which response - COMMAND_SCENES_ADD_SCENE_REPONSE, COMMAND_SCENES_VIEW_SCENE_RESPONSE,
                                                // COMMAND_SCENES_REMOVE_SCENE_RESPONSE, COMMAND_SCENES_REMOVE_ALL_SCENES_RESPONSE,
                                                // COMMAND_SCENES_STORE_SCENE_RESPONSE, COMMAND_SCENES_GET_MEMBERSHIP_RESPONSE
    uint8_t status;                             // response status
    uint8_t scene_count;                        // number of scenes in the scene list (only valid for COMMAND_SCENES_GET_MEMBERSHIP_RESPONSE)
    uint8_t *scene_list;                        // list of scene IDs (only valid for COMMAND_SCENES_GET_MEMBERSHIP_RESPONSE)
    uint8_t capacity;                           // remaining capacity of the scene table (only valid for COMMAND_SCENES_GET_MEMBERSHIP_RESPONSE)
    s_zb_zcl_general_scene_t *scene;  // pointer to the scene structure
} s_zb_zcl_scene_rsp_t;

typedef struct s_zb_zcl_alarms
{
    s_zb_af_address_t *src_addr; // requestor's address
    uint8_t cmd_id;                         // COMMAND_ALARMS_ALARM or COMMAND_ALARMS_GET_ALARM_RESPONSE
    uint8_t status;                         // response status (only applicable to COMMAND_ALARMS_GET_ALARM_RESPONSE)
    uint8_t alarm_code;                     // response status (only applicable to COMMAND_ALARMS_GET_ALARM_RESPONSE)
    uint16_t cluster_id;                     // the id of the cluster whose attribute generated the alarm
    uint32_t timestamp;                     // the time at which the alarm occurred (only applicable to COMMAND_ALARMS_GET_ALARM_RESPONSE)
} s_zb_zcl_alarms_t;

// This callback is called to process an incomming Identify Query Response command.
typedef void (*pfn_zcl_general_identify_query_rsp_t)(s_zb_zcl_identify_query_rsp_t *p_rsp);

// This callback is called to process an received Group Response message.
// This means that this app sent the request message.
typedef void (*pfn_zcl_general_groups_rsp_t)(s_zb_zcl_groups_rsp_t *p_rsp);

// This callback is called to process an incoming Scene responses. This means
// that this app sent the request for this response.
typedef void (*pfn_zcl_general_scene_rsp_t)(s_zb_zcl_scene_rsp_t *p_rsp);

// This callback is called to process an incoming Alarm request or response command.
typedef void (*pfn_zcl_general_alarms_t)(uint8_t direction,s_zb_zcl_alarms_t *p_alarm);

// Register Callbacks table entry - enter function pointers for callbacks that
// the application would like to receive
typedef struct
{
    pfn_zcl_general_identify_query_rsp_t pfn_identify_query_rsp;
    pfn_zcl_general_groups_rsp_t pfn_groups_rsp;
    pfn_zcl_general_scene_rsp_t pfn_scene_rsp;
    pfn_zcl_general_alarms_t pfn_alarms;
} s_zb_zcl_general_app_callbacks_t;

/*********************************************************************
 * FUNCTIONS MACROS
 */
// ZCL_BASIC_COMMANDS
/**
 * @brief Send a Reset to Factory Defaults command - COMMAND_BASIC_RESET_FACTORY_DEFAULTS
 * Use like:
 *     zb_status_t zb_zcl_general_send_basic_reset_factory_defaults(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_basic_reset_factory_defaults(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        ZCL_CLUSTER_ID_GENERAL_BASIC, COMMAND_BASIC_RESET_TO_FACTORY_DEFAULTS, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

// ZCL_IDENTIFY
/**
 * @brief Send a Identify Query command 
 * Use like:
 *     zb_status_t zb_zcl_general_send_identify_query(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_identify_query(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        ZCL_CLUSTER_ID_GENERAL_IDENTIFY, COMMAND_IDENTIFY_IDENTIFY_QUERY, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

// ZCL_GROUPS
/**
 * @brief Send a Add Group command
 * Use like:
 *     zb_status_t zb_zcl_general_send_add_group(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint16_t group_id, uint8_t *group_name, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_add_group(src_ep, dst_addr, group_id, group_name, disable_default_rsp, seq_num) \
    zb_zcl_general_send_add_group_request(src_ep, dst_addr, \
        COMMAND_GROUPS_ADD_GROUP, group_id, group_name, disable_default_rsp, seq_num)

/**
 * @brief Send a Group View command
 * Use like:
 *     zb_status_t zb_zcl_general_send_group_view(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint16_t group_id, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_group_view(src_ep, dst_addr, group_id, disable_default_rsp, seq_num) \
    zb_zcl_general_send_group_request(src_ep, dst_addr, \
        COMMAND_GROUPS_VIEW_GROUP, group_id, disable_default_rsp, seq_num)

/**
 * @brief Send a Group Get Membership command
 * Use like:
 *     zb_status_t zb_zcl_general_send_group_get_membership(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t group_cnt, uint16_t *group_list, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_group_get_membership(src_ep, dst_addr, group_cnt, group_list, disable_default_rsp, seq_num) \
    zb_zcl_general_send_group_get_membership_request(src_ep, dst_addr, \
        COMMAND_GROUPS_GET_GROUP_MEMBERSHIP, false, ZCL_FRAME_CLIENT_SERVER_DIR, 0, group_cnt, group_list, disable_default_rsp, seq_num)

/**
 * @brief Send a Remove Group command
 * Use like:
 *     zb_status_t zb_zcl_general_send_group_remove(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint16_t group_id, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_group_remove(src_ep, dst_addr, group_id, disable_default_rsp, seq_num) \
    zb_zcl_general_send_group_request(src_ep, dst_addr, \
        COMMAND_GROUPS_REMOVE_GROUP, group_id, disable_default_rsp, seq_num)

/**
 * @brief Send a Remove All Groups command
 * Use like:
 *     zb_status_t zb_zcl_general_send_group_remove_all(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_group_remove_all(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        ZCL_CLUSTER_ID_GENERAL_GROUPS, COMMAND_GROUPS_REMOVE_ALL_GROUPS, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

/**
 * @brief Send a Add Group If Identifying command
 * Use like:
 *     zb_status_t zb_zcl_general_send_group_add_if_identifying(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint16_t group_id, uint8_t *group_name, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_group_add_if_identifying(src_ep, dst_addr, group_id, group_name, disable_default_rsp, seq_num) \
    zb_zcl_general_send_add_group_request(src_ep, dst_addr, \
        COMMAND_GROUPS_ADD_GROUP_IF_IDENTIFYING, group_id, group_name, disable_default_rsp, seq_num)

// ZCL_SCENES
/**
 * @brief Send a Add Scene command
 * Use like:
 *     zb_status_t zb_zcl_general_send_add_scene(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, s_zb_zcl_general_scene_t *scene, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_add_scene(src_ep, dst_addr, scene, disable_default_rsp, seq_num) \
    zb_zcl_general_send_add_scene_request(src_ep, dst_addr, \
        COMMAND_SCENES_ADD_SCENE, scene, disable_default_rsp, seq_num)

/**
 * @brief Send a View Scene command
 * Use like:
 *     zb_status_t zb_zcl_general_send_scene_view(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint16_t group_id, uint8_t scene_id, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_scene_view(src_ep, dst_addr, group_id, scene_id, disable_default_rsp, seq_num) \
    zb_zcl_general_send_scene_request(src_ep, dst_addr, \
        COMMAND_SCENES_VIEW_SCENE, group_id, scene_id, disable_default_rsp, seq_num)

/**
 * @brief Send a Remove Scene command
 * Use like:
 *     zb_status_t zb_zcl_general_send_scene_remove(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint16_t group_id, uint8_t scene_id, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_scene_remove(src_ep, dst_addr, group_id, scene_id, disable_default_rsp, seq_num) \
    zb_zcl_general_send_scene_request(src_ep, dst_addr, \
        COMMAND_SCENES_REMOVE_SCENE, group_id, scene_id, disable_default_rsp, seq_num)

/**
 * @brief Send a Store Scene command
 * Use like:
 *     zb_status_t zb_zcl_general_send_scene_store(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint16_t group_id, uint8_t scene_id, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_scene_store(src_ep, dst_addr, group_id, scene_id, disable_default_rsp, seq_num) \
    zb_zcl_general_send_scene_request(src_ep, dst_addr, \
        COMMAND_SCENES_STORE_SCENE, group_id, scene_id, disable_default_rsp, seq_num)

/**
 * @brief Send a Recall Scene command
 * Use like:
 *     zb_status_t zb_zcl_general_send_scene_recall(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint16_t group_id, uint8_t scene_id, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_scene_recall(src_ep, dst_addr, group_id, scene_id, disable_default_rsp, seq_num) \
    zb_zcl_general_send_scene_request(src_ep, dst_addr, \
        COMMAND_SCENES_RECALL_SCENE, group_id, scene_id, disable_default_rsp, seq_num)

/**
 * @brief Send a Scene Remove All command
 * Use like:
 *     zb_status_t zb_zcl_general_send_scene_remove_all(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint16_t group_id, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_scene_remove_all(src_ep, dst_addr, group_id, disable_default_rsp, seq_num) \
    zb_zcl_general_send_scene_request(src_ep, dst_addr, \
        COMMAND_SCENES_REMOVE_ALL_SCENES, group_id, 0, disable_default_rsp, seq_num)

/**
 * @brief Send a Scene Get Membership command
 * Use like:
 *     zb_status_t zb_zcl_general_send_scene_get_membership(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint16_t group_id, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_scene_get_membership(src_ep, dst_addr, group_id, disable_default_rsp, seq_num) \
    zb_zcl_general_send_scene_request(src_ep, dst_addr, \
        COMMAND_SCENES_GET_SCENE_MEMBERSHIP, group_id, 0, disable_default_rsp, seq_num)

/**
 * @brief Send a Enhanced Add Scene command
 * Use like:
 *     zb_status_t zb_zcl_general_send_enhanced_add_scene(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, s_zb_zcl_general_scene_t *scene, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_enhanced_add_scene(src_ep, dst_addr, scene, disable_default_rsp, seq_num) \
    zb_zcl_general_send_add_scene_request(src_ep, dst_addr, \
        COMMAND_SCENES_ENHANCED_ADD_SCENE, scene, disable_default_rsp, seq_num)

/**
 * @brief Send a Enhanced View Scene command
 * Use like:
 *     zb_status_t zb_zcl_general_send_scene_enhanced_view(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint16_t group_id, uint8_t scene_id, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_scene_enhanced_view(src_ep, dst_addr, group_id, scene_id, disable_default_rsp, seq_num) \
    zb_zcl_general_send_scene_request(src_ep, dst_addr, \
        COMMAND_SCENES_ENHANCED_VIEW_SCENE, group_id, scene_id, disable_default_rsp, seq_num)

// ZCL_ON_OFF
/**
 * @brief Send an On Off Command = COMMAND_ON_OFF_OFF
 * Use like:
 *     zb_status_t zb_zcl_general_send_on_off_cmd_off(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_on_off_cmd_off(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        ZCL_CLUSTER_ID_GENERAL_ON_OFF, COMMAND_ON_OFF_OFF, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

/**
 * @brief Send an On Off Command = COMMAND_ON_OFF_ON
 * Use like:
 *     zb_status_t zb_zcl_general_send_on_off_cmd_on(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_on_off_cmd_on(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        ZCL_CLUSTER_ID_GENERAL_ON_OFF, COMMAND_ON_OFF_ON, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

/**
 * @brief Send an On Off Command = COMMAND_ON_OFF_TOGGLE
 * Use like:
 *     zb_status_t zb_zcl_general_send_on_off_cmd_toggle(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_on_off_cmd_toggle(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        ZCL_CLUSTER_ID_GENERAL_ON_OFF, COMMAND_ON_OFF_TOGGLE, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

/**
 * @brief Send an On With Recall Global Scene command - COMMAND_ON_OFF_ON_WITH_RECALL_GLOBAL_SCENE
 * Use like:
 *     zb_status_t zb_zcl_general_send_on_off_cmd_on_with_recall_global_scene(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_on_off_cmd_on_with_recall_global_scene(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        ZCL_CLUSTER_ID_GENERAL_ON_OFF, COMMAND_ON_OFF_ON_WITH_RECALL_GLOBAL_SCENE, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

// ZCL_LEVEL_CONTROL
/**
 * @brief Send a Level Control Move to Level Command - COMMAND_LEVEL_MOVE_TO_LEVEL
 * Use like:
 *     zb_status_t zb_zcl_general_send_level_control_move_to_level(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t level, uint16_t trans_time, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_level_control_move_to_level(src_ep, dst_addr, level, trans_time, disable_default_rsp, seq_num) \
    zb_zcl_general_send_level_control_move_to_level_request(src_ep, dst_addr, \
        COMMAND_LEVEL_MOVE_TO_LEVEL, level, trans_time, disable_default_rsp, seq_num)


/**
 * @brief Send a Level Control Move Command - COMMAND_LEVEL_MOVE
 * Use like:
 *     zb_status_t zb_zcl_general_send_level_control_move(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t move_mode, uint8_t rate, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_level_control_move(src_ep, dst_addr, move_mode, rate, disable_default_rsp, seq_num) \
    zb_zcl_general_send_level_control_move_request(src_ep, dst_addr, \
        COMMAND_LEVEL_MOVE, move_mode, rate, disable_default_rsp, seq_num)

/**
 * @brief Send a Level Control Step Command - COMMAND_LEVEL_STEP
 * Use like:
 *     zb_status_t zb_zcl_general_send_level_control_step(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t step_mode, uint8_t step_size, uint16_t trans_time, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_level_control_step(src_ep, dst_addr, step_mode, step_size, trans_time, disable_default_rsp, seq_num) \
    zb_zcl_general_send_level_control_step_request(src_ep, dst_addr, \
        COMMAND_LEVEL_STEP, step_mode, step_size, trans_time, disable_default_rsp, seq_num)

/**
 * @brief Send a Level Control Stop Command - COMMAND_LEVEL_STOP
 * Use like:
 *     zb_status_t zb_zcl_general_send_level_control_stop(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_level_control_stop(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, COMMAND_LEVEL_STOP, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

/**
 * @brief Send a Level Control Move to Level With On/Off Command - COMMAND_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF
 * Use like:
 *     zb_status_t zb_zcl_general_send_level_control_move_to_level_with_on_off(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t level, uint16_t trans_time, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_level_control_move_to_level_with_on_off(src_ep, dst_addr, level, trans_time, disable_default_rsp, seq_num) \
    zb_zcl_general_send_level_control_move_to_level_request(src_ep, dst_addr, \
        COMMAND_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF, level, trans_time, disable_default_rsp, seq_num)

/**
 * @brief Send a Level Control Move With On/Off Command - COMMAND_LEVEL_MOVE_WITH_ON_OFF
 * Use like:
 *     zb_status_t zb_zcl_general_send_level_control_move_with_on_off(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t move_mode, uint8_t rate, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_level_control_move_with_on_off(src_ep, dst_addr, move_mode, rate, disable_default_rsp, seq_num) \
    zb_zcl_general_send_level_control_move_request(src_ep, dst_addr, \
        COMMAND_LEVEL_MOVE_WITH_ON_OFF, move_mode, rate, disable_default_rsp, seq_num)

/**
 * @brief Send a Level Control Step With On/Off Command - COMMAND_LEVEL_STEP_WITH_ON_OFF
 * Use like:
 *     zb_status_t zb_zcl_general_send_level_control_step_with_on_off(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t step_mode, uint8_t step_size, uint16_t trans_time, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_level_control_step_with_on_off(src_ep, dst_addr, step_mode, step_size, trans_time, disable_default_rsp, seq_num) \
    zb_zcl_general_send_level_control_step_request(src_ep, dst_addr, \
        COMMAND_LEVEL_STEP_WITH_ON_OFF, step_mode, step_size, trans_time, disable_default_rsp, seq_num)

/**
 * @brief Send a Level Control Stop With On/Off Command - COMMAND_LEVEL_STOP_WITH_ON_OFF
 * Use like:
 *     zb_status_t zb_zcl_general_send_level_control_stop_with_on_off(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_level_control_stop_with_on_off(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, COMMAND_LEVEL_STOP_WITH_ON_OFF, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

// ZCL_ALARMS
/**
 * @brief Send an Alarm Reset All command - COMMAND_ALARMS_RESET_ALL_ALARMS
 * Use like:
 *     zb_status_t zb_zcl_general_send_alarms_reset_all(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_alarms_reset_all(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        ZCL_CLUSTER_ID_GENERAL_ALARMS, COMMAND_ALARMS_RESET_ALL_ALARMS, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

/**
 * @brief Send an Alarm Get command - COMMAND_ALARMS_GET_ALARM
 * Use like:
 *     zb_status_t zb_zcl_general_send_alarms_get(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_alarms_get(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        ZCL_CLUSTER_ID_GENERAL_ALARMS, COMMAND_ALARMS_GET_ALARM, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

/**
 * @brief Send an Alarm Reset Alarm Log command - COMMAND_ALARMS_RESET_ALARM_LOG
 * Use like:
 *     zb_status_t zb_zcl_general_send_alarms_reset_log(
 *                   uint16_t src_ep, s_zb_af_address_t *dst_addr, uint8_t disable_default_rsp, uint8_t seq_num);
 */
#define zb_zcl_general_send_alarms_reset_log(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        ZCL_CLUSTER_ID_GENERAL_ALARMS, COMMAND_ALARMS_RESET_ALARM_LOG, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

/****************************************************************************
 * FUNCTIONS
 */
// Register for callback from this cluster library
zb_status_t zb_zcl_general_register_command_callbacks(
    uint8_t endpoint, s_zb_zcl_general_app_callbacks_t *callbacks);

// ZCL_IDENTIFY
/**
 * @brief Send an Identify message
 */
zb_status_t zb_zcl_general_send_identify(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t identify_time, uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send an Identify Query message
 */
zb_status_t zb_zcl_general_send_identify_trigger_effect(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t effect_id, uint8_t effect_variant,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send an Identify Query Response message
 */
zb_status_t zb_zcl_general_send_identify_query_response(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t timeout, uint8_t disable_default_rsp, uint8_t seq_num);

// ZCL_ON_OFF
/**
 * @brief Call to send out an Off with Effect Command
 *      effect_id - fading effect to use when switching the device off.
 *      effect_variant - which variant of effect to be triggered
 */
zb_status_t zb_zcl_general_send_on_off_cmd_off_with_effect(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t effect_id, uint8_t effect_variant,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Call to send out an On with Timed Off Command
 *      on_of_ctrl - how the lamp is to be operated
 *      on_time - the length of time (in 1/10ths second) that the lamp is to remain on, before automatically turning off
 *      off_wait_time - the length of time (in 1/10ths second) that the lamp shall remain off, and guarded to prevent an on command turning the light back on
 */
zb_status_t zb_zcl_general_send_on_off_cmd_on_with_timed_off(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_on_off_ctrl_t on_off_ctrl, uint16_t on_time,
    uint16_t off_wait_time, uint8_t disable_default_rsp, uint8_t seq_num);

// ZCL_LEVEL_CTRL
/**
 * @brief Call to send out a Level Control Move to Level Request
 *      cmd - Move or Move with On/Off
 *      level - new level to move to
 *      trans_time - time to take to move to the new level (in seconds)
 */
zb_status_t zb_zcl_general_send_level_control_move_to_level_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint8_t level, uint16_t trans_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Call to send out a Level Control Move Request
 *      cmd - Step or Step with On/Off
 *      move_mode - LEVEL_STEP_UP or LEVEL_STEP_DOWN
 *      rate - number of steps to take per second
 */
zb_status_t zb_zcl_general_send_level_control_move_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint8_t move_mode, uint8_t rate,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Call to send out a level control step request
 *      cmd - Step or Step with On/Off
 *      step_mode - LEVEL_STEP_UP or LEVEL_STEP_DOWN
 *      step_size - number of steps to take
 *      trans_time - time to take to perform a single step
 */
zb_status_t zb_zcl_general_send_level_control_step_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint8_t step_mode, uint8_t step_size, uint16_t trans_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Call to send out a level control stop request
 *      cmd - Stop or Stop with On/Off
 */
zb_status_t zb_zcl_general_send_level_control_stop_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint8_t disable_default_rsp, uint8_t seq_num);

// ZCL_GROUPS
/**
 * @brief Send a Group command (request) - not Group Add or Remove All
 */
zb_status_t zb_zcl_general_send_group_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint16_t group_id, uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Group Add Request message
 *      group_name - pointer to the group nam. This is a Zigbee string data type, so the first byte is the length of the string.
 */
zb_status_t zb_zcl_general_send_add_group_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint16_t group_id, uint8_t *group_name,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Call to send Group Membership Command
 */
zb_status_t zb_zcl_general_send_group_get_membership_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint8_t rsp_cmd, uint8_t direction, uint8_t capacity,
    uint8_t group_cnt, uint16_t *group_list, uint8_t disable_default_rsp, uint8_t seq_num);

// ZCL_SCENES
/**
 * @brief Send an (Enhanced) Add Scene Request message
 */
zb_status_t zb_zcl_general_send_add_scene_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, s_zb_zcl_general_scene_t *scene,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Scene command (request) - not Scene Add
 */
zb_status_t zb_zcl_general_send_scene_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint16_t group_id, uint8_t scene_id,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send Scene Copy Request message
 */
zb_status_t zb_zcl_general_send_scene_copy(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t mode, uint16_t group_id_from, uint8_t scene_id_from,
    uint16_t group_id_to, uint8_t scene_id_to,
    uint8_t disable_default_rsp, uint8_t seq_num);

// ZCL_ALARMS
/**
 * @brief Send out an Alarm Command
 */
zb_status_t zb_zcl_general_send_alarms(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t alarm_code, uint16_t cluster_id,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send out an Alarm Reset command
 * 
 */
zb_status_t zb_zcl_general_send_alarms_reset(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t alarm_code, uint16_t cluster_id,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send out an Alarm Get Response command
 */
zb_status_t zb_zcl_general_send_alarms_get_response(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t status, uint8_t alarm_code, uint16_t cluster_id,
    uint32_t timestamp, uint8_t disable_default_rsp, uint8_t seq_num);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZCL_GENERAL_H_ */