#ifndef ZB_ZCL_LIGHTING_H_
#define ZB_ZCL_LIGHTING_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"

/******************************************************************************
 * CONSTANTS
 */

/*****************************************/
/***  Color Control Cluster Attributes ***/
/*****************************************/
// Color Information attributes set
#define ATTRID_COLOR_CONTROL_CURRENT_HUE                        0x0000
#define ATTRID_COLOR_CONTROL_CURRENT_SATURATION                 0x0001
#define ATTRID_COLOR_CONTROL_REMAINING_TIME                     0x0002
#define ATTRID_COLOR_CONTROL_CURRENT_X                          0x0003
#define ATTRID_COLOR_CONTROL_CURRENT_Y                          0x0004
#define ATTRID_COLOR_CONTROL_DRIFT_COMPENSATION                 0x0005
#define ATTRID_COLOR_CONTROL_COMPENSATION_TEXT                  0x0006
#define ATTRID_COLOR_CONTROL_COLOR_TEMPERATURE_MIREDS           0x0007
#define ATTRID_COLOR_CONTROL_COLOR_MODE                         0x0008
#define ATTRID_COLOR_CONTROL_OPTIONS                            0x000F

// Defined Primaries Inofrmation attribute Set
#define ATTRID_COLOR_CONTROL_NUMBER_OF_PRIMARIES                0x0010
#define ATTRID_COLOR_CONTROL_PRIMARY_1_X                        0x0011
#define ATTRID_COLOR_CONTROL_PRIMARY_1_Y                        0x0012
#define ATTRID_COLOR_CONTROL_PRIMARY_1_INTENSITY                0x0013
// 0x0014 is reserved
#define ATTRID_COLOR_CONTROL_PRIMARY_2_X                        0x0015
#define ATTRID_COLOR_CONTROL_PRIMARY_2_Y                        0x0016
#define ATTRID_COLOR_CONTROL_PRIMARY_2_INTENSITY                0x0017
// 0x0018 is reserved
#define ATTRID_COLOR_CONTROL_PRIMARY_3_X                        0x0019
#define ATTRID_COLOR_CONTROL_PRIMARY_3_Y                        0x001a
#define ATTRID_COLOR_CONTROL_PRIMARY_3_INTENSITY                0x001b

// Additional Defined Primaries Information attribute set
#define ATTRID_COLOR_CONTROL_PRIMARY_4_X                        0x0020
#define ATTRID_COLOR_CONTROL_PRIMARY_4_Y                        0x0021
#define ATTRID_COLOR_CONTROL_PRIMARY_4_INTENSITY                0x0022
// 0x0023 is reserved
#define ATTRID_COLOR_CONTROL_PRIMARY_5_X                        0x0024
#define ATTRID_COLOR_CONTROL_PRIMARY_5_Y                        0x0025
#define ATTRID_COLOR_CONTROL_PRIMARY_5_INTENSITY                0x0026
// 0x0027 is reserved
#define ATTRID_COLOR_CONTROL_PRIMARY_6_X                        0x0028
#define ATTRID_COLOR_CONTROL_PRIMARY_6_Y                        0x0029
#define ATTRID_COLOR_CONTROL_PRIMARY_6_INTENSITY                0x002a

// Defined Color Points Settings attribute set
#define ATTRID_COLOR_CONTROL_WHITE_POINT_X                      0x0030
#define ATTRID_COLOR_CONTROL_WHITE_POINT_Y                      0x0031
#define ATTRID_COLOR_CONTROL_COLOR_POINT_RX                     0x0032
#define ATTRID_COLOR_CONTROL_COLOR_POINT_RY                     0x0033
#define ATTRID_COLOR_CONTROL_COLOR_POINT_R_INTENSITY            0x0034
// 0x0035 is reserved
#define ATTRID_COLOR_CONTROL_COLOR_POINT_GX                     0x0036
#define ATTRID_COLOR_CONTROL_COLOR_POINT_GY                     0x0037
#define ATTRID_COLOR_CONTROL_COLOR_POINT_G_INTENSITY            0x0038
// 0x0039 is reserved
#define ATTRID_COLOR_CONTROL_COLOR_POINT_BX                     0x003a
#define ATTRID_COLOR_CONTROL_COLOR_POINT_BY                     0x003b
#define ATTRID_COLOR_CONTROL_COLOR_POINT_B_INTENSITY            0x003c
// 0x003d is reserved
#define ATTRID_COLOR_CONTROL_ENHANCED_CURRENT_HUE               0x4000
#define ATTRID_COLOR_CONTROL_ENHANCED_COLOR_MODE                0x4001
#define ATTRID_COLOR_CONTROL_COLOR_LOOP_ACTIVE                  0x4002
#define ATTRID_COLOR_CONTROL_COLOR_LOOP_DIRECTION               0x4003
#define ATTRID_COLOR_CONTROL_COLOR_LOOP_TIME                    0x4004
#define ATTRID_COLOR_CONTROL_COLOR_LOOP_START_ENHANCED_HUE      0x4005
#define ATTRID_COLOR_CONTROL_COLOR_LOOP_STORED_ENHANCED_HUE     0x4006
#define ATTRID_COLOR_CONTROL_COLOR_CAPABILITIES                 0x400a
#define ATTRID_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MIN_MIREDS            0x400b
#define ATTRID_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MAX_MIREDS            0x400c
#define ATTRID_COLOR_CONTROL_COUPLE_COLOR_TEMP_TO_LEVEL_MIN_MIREDS     0x400d
#define ATTRID_COLOR_CONTROL_START_UP_COLOR_TEMPERATURE_MIREDS         0x4010

/***  Color Information attributes range limits   ***/
#define LIGHTING_COLOR_HUE_MAX                                  0xfe
#define LIGHTING_COLOR_SAT_MAX                                  0xfe
#define LIGHTING_COLOR_REMAINING_TIME_MAX                       0xfffe
#define LIGHTING_COLOR_CURRENT_X_MAX                            0xfeff
#define LIGHTING_COLOR_CURRENT_Y_MAX                            0xfeff
#define LIGHTING_COLOR_TEMPERATURE_MAX                          0xfeff

/*** Drift Compensation Attribute values ***/
#define DRIFT_COMP_NONE                                         0x00
#define DRIFT_COMP_OTHER_UNKNOWN                                0x01
#define DRIFT_COMP_TEMPERATURE_MONITOR                          0x02
#define DRIFT_COMP_OPTICAL_LUMINANCE_MONITOR_FEEDBACK           0x03
#define DRIFT_COMP_OPTICAL_COLOR_MONITOR_FEEDBACK               0x04

/*** Color Mode Attribute values ***/
#define COLOR_MODE_CURRENT_HUE_SATURATION                       0x00
#define COLOR_MODE_CURRENT_X_Y                                  0x01
#define COLOR_MODE_COLOR_TEMPERATURE                            0x02

/*** Enhanced Color Mode Attribute values ***/
#define ENHANCED_COLOR_MODE_CURRENT_HUE_SATURATION              0x00
#define ENHANCED_COLOR_MODE_CURRENT_X_Y                         0x01
#define ENHANCED_COLOR_MODE_COLOR_TEMPERATURE                   0x02
#define ENHANCED_COLOR_MODE_ENHANCED_CURRENT_HUE_SATURATION     0x03

/*** Color Capabilities Attribute bit masks ***/
#define COLOR_CAPABILITIES_ATTR_BIT_NONE                        0x00
#define COLOR_CAPABILITIES_ATTR_BIT_HUE_SATURATION              0x01
#define COLOR_CAPABILITIES_ATTR_BIT_ENHANCED_HUE                0x02
#define COLOR_CAPABILITIES_ATTR_BIT_COLOR_LOOP                  0x04
#define COLOR_CAPABILITIES_ATTR_BIT_X_Y_ATTRIBUTES              0x08
#define COLOR_CAPABILITIES_ATTR_BIT_COLOR_TEMPERATURE           0x10

/*****************************************/
/***  Color Control Cluster Commands   ***/
/*****************************************/
#define COMMAND_COLOR_CONTROL_MOVE_TO_HUE                                0x00
#define COMMAND_COLOR_CONTROL_MOVE_HUE                                   0x01
#define COMMAND_COLOR_CONTROL_STEP_HUE                                   0x02
#define COMMAND_COLOR_CONTROL_MOVE_TO_SATURATION                         0x03
#define COMMAND_COLOR_CONTROL_MOVE_SATURATION                            0x04
#define COMMAND_COLOR_CONTROL_STEP_SATURATION                            0x05
#define COMMAND_COLOR_CONTROL_MOVE_TO_HUE_AND_SATURATION                 0x06
#define COMMAND_COLOR_CONTROL_MOVE_TO_COLOR                              0x07
#define COMMAND_COLOR_CONTROL_MOVE_COLOR                                 0x08
#define COMMAND_COLOR_CONTROL_STEP_COLOR                                 0x09
#define COMMAND_COLOR_CONTROL_MOVE_TO_COLOR_TEMPERATURE                  0x0a
#define COMMAND_COLOR_CONTROL_ENHANCED_MOVE_TO_HUE                       0x40
#define COMMAND_COLOR_CONTROL_ENHANCED_MOVE_HUE                          0x41
#define COMMAND_COLOR_CONTROL_ENHANCED_STEP_HUE                          0x42
#define COMMAND_COLOR_CONTROL_ENHANCED_MOVE_TO_HUE_AND_SATURATION        0x43
#define COMMAND_COLOR_CONTROL_COLOR_LOOP_SET                             0x44
#define COMMAND_COLOR_CONTROL_STOP_MOVE_STEP                             0x47
#define COMMAND_COLOR_CONTROL_MOVE_COLOR_TEMPERATURE                     0x4b
#define COMMAND_COLOR_CONTROL_STEP_COLOR_TEMPERATURE                     0x4c

/***  Move To Hue Cmd payload: direction field values  ***/
#define LIGHTING_MOVE_TO_HUE_DIRECTION_SHORTEST_DISTANCE                 0x00
#define LIGHTING_MOVE_TO_HUE_DIRECTION_LONGEST_DISTANCE                  0x01
#define LIGHTING_MOVE_TO_HUE_DIRECTION_UP                                0x02
#define LIGHTING_MOVE_TO_HUE_DIRECTION_DOWN                              0x03
/***  Move Hue Cmd payload: moveMode field values   ***/
#define LIGHTING_MOVE_HUE_STOP                                           0x00
#define LIGHTING_MOVE_HUE_UP                                             0x01
#define LIGHTING_MOVE_HUE_DOWN                                           0x03
/***  Step Hue Cmd payload: stepMode field values ***/
#define LIGHTING_STEP_HUE_UP                                             0x01
#define LIGHTING_STEP_HUE_DOWN                                           0x03
/***  Move Saturation Cmd payload: moveMode field values ***/
#define LIGHTING_MOVE_SATURATION_STOP                                    0x00
#define LIGHTING_MOVE_SATURATION_UP                                      0x01
#define LIGHTING_MOVE_SATURATION_DOWN                                    0x03
/***  Step Saturation Cmd payload: stepMode field values ***/
#define LIGHTING_STEP_SATURATION_UP                                      0x01
#define LIGHTING_STEP_SATURATION_DOWN                                    0x03
/***  Color Loop Set Cmd payload: action field values  ***/
#define LIGHTING_COLOR_LOOP_ACTION_DEACTIVATE                            0x00
#define LIGHTING_COLOR_LOOP_ACTION_ACTIVATE_FROM_START_HUE               0x01
#define LIGHTING_COLOR_LOOP_ACTION_ACTIVATE_FROM_ENH_CURR_HUE            0x02
/***  Color Loop Set Cmd payload: direction field values   ***/
#define LIGHTING_COLOR_LOOP_DIRECTION_DECREMENT                          0x00
#define LIGHTING_COLOR_LOOP_DIRECTION_INCREMENT                          0x01

/*****************************************************************************/
/***          Ballast Configuration Cluster Attributes                     ***/
/*****************************************************************************/
// Ballast Information attribute set
#define ATTRID_BALLAST_CONFIGURATION_PHYSICAL_MIN_LEVEL                  0x0000
#define ATTRID_BALLAST_CONFIGURATION_PHYSICAL_MAX_LEVEL                  0x0001
#define ATTRID_BALLAST_CONFIGURATION_BALLAST_STATUS                      0x0002
/*** Ballast Status Attribute values (by bit number) ***/
#define LIGHTING_BALLAST_STATUS_NON_OPERATIONAL                          1 // bit 0 is set
#define LIGHTING_BALLAST_STATUS_LAMP_IS_NOT_IN_SOCKET                    2 // bit 1 is set
// Ballast Settings attributes set
#define ATTRID_BALLAST_CONFIGURATION_MIN_LEVEL                           0x0010
#define ATTRID_BALLAST_CONFIGURATION_MAX_LEVEL                           0x0011
#define ATTRID_BALLAST_CONFIGURATION_POWER_ON_LEVEL                      0x0012
#define ATTRID_BALLAST_CONFIGURATION_POWER_ON_FADE_TIME                  0x0013
#define ATTRID_BALLAST_CONFIGURATION_INTRINSIC_BALLAST_FACTOR            0x0014
#define ATTRID_BALLAST_CONFIGURATION_BALLAST_FACTOR_ADJUSTMENT           0x0015
// Lamp Information attributes set
#define ATTRID_BALLAST_CONFIGURATION_LAMP_QUANTITY                       0x0020
// Lamp Settings attributes set
#define ATTRID_BALLAST_CONFIGURATION_LAMP_TYPE                           0x0030
#define ATTRID_BALLAST_CONFIGURATION_LAMP_MANUFACTURER                   0x0031
#define ATTRID_BALLAST_CONFIGURATION_LAMP_RATED_HOURS                    0x0032
#define ATTRID_BALLAST_CONFIGURATION_LAMP_BURN_HOURS                     0x0033
#define ATTRID_BALLAST_CONFIGURATION_LAMP_ALARM_MODE                     0x0034
#define ATTRID_BALLAST_CONFIGURATION_LAMP_BURN_HOURS_TRIP_POINT          0x0035
/*** Lamp Alarm Mode attribute values  ***/
#define LIGHTING_BALLAST_LAMP_ALARM_MODE_BIT_0_NO_ALARM                  0
#define LIGHTING_BALLAST_LAMP_ALARM_MODE_BIT_0_ALARM                     1

/*******************************************************************************
* TYPEDEFS
*/

typedef struct s_zb_zcl_color_control_move_to_hue
{
    uint8_t hue;
    uint8_t direction;
    uint16_t transition_time;
} s_zb_zcl_color_control_move_to_hue_t;

typedef struct s_zb_zcl_color_control_move_hue
{
    uint8_t move_mode;
    uint8_t rate;
} s_zb_zcl_color_control_move_hue_t;

typedef struct s_zb_zcl_color_control_step_hue
{
    uint8_t step_mode;
    uint8_t transition_time;
} s_zb_zcl_color_control_step_hue_t;

typedef struct s_zb_zcl_color_control_move_to_saturation
{
    uint8_t saturation;
    uint16_t transition_time;
} s_zb_zcl_color_control_move_to_saturation_t;

typedef struct s_zb_zcl_color_control_move_saturation
{
    uint8_t move_mode;
    uint8_t rate;
} s_zb_zcl_color_control_move_saturation_t;

typedef struct s_zb_zcl_color_control_step_saturation
{
    uint8_t step_mode;
    uint16_t transition_time;
} s_zb_zcl_color_control_step_saturation_t;

typedef struct s_zb_zcl_color_control_move_to_hue_and_saturation
{
    uint8_t hue;
    uint8_t saturation;
    uint16_t transition_time;
} s_zb_zcl_color_control_move_to_hue_and_saturation_t;

typedef struct s_zb_zcl_color_control_move_to_color
{
    uint16_t color_x;
    uint16_t color_y;
    uint16_t transition_time;
} s_zb_zcl_color_control_move_to_color_t;

typedef struct s_zb_zcl_color_control_move_color
{
    int16_t rate_x;
    int16_t rate_y;
} s_zb_zcl_color_control_move_color_t;

typedef struct s_zb_zcl_color_control_step_color
{
    int16_t step_x;
    int16_t step_y;
    uint16_t transition_time;
} s_zb_zcl_color_control_step_color_t;

typedef struct s_zb_zcl_color_control_move_to_color_temperature
{
    uint16_t color_temperature;
    uint16_t transition_time;
} s_zb_zcl_color_control_move_to_color_temperature_t;

typedef struct s_zb_zcl_color_control_enhanced_move_to_hue
{
    uint16_t enhanced_hue;
    uint8_t direction;
    uint16_t transition_time;
} s_zb_zcl_color_control_enhanced_move_to_hue_t;

typedef struct s_zb_zcl_color_control_enhanced_move_hue
{
    uint8_t move_mode;
    uint16_t rate;
} s_zb_zcl_color_control_enhanced_move_hue_t;

typedef struct s_zb_zcl_color_control_enhanced_step_hue
{
    uint8_t step_mode;
    uint16_t step_size;
    uint16_t transition_time;
} s_zb_zcl_color_control_enhanced_step_hue_t;

typedef struct
{
    uint16_t enhanced_hue;
    uint8_t saturation;
    uint16_t transition_time;
} s_zb_zcl_color_control_enhanced_move_to_hue_and_saturation_t;

typedef struct s_zb_zcl_color_control_color_loop_set
{
    struct {
        uint8_t action : 1;
        uint8_t direction : 1;
        uint8_t time : 1;
        uint8_t start_hue : 1;
        uint8_t reserved : 4;
    } bits;
    uint8_t byte;
} s_zb_zcl_color_control_color_loop_update_flags_t;

typedef struct s_zb_zcl_color_control_move_color_temperature
{
    s_zb_zcl_color_control_color_loop_update_flags_t update_flags;
    uint8_t action;     // LIGHTING_COLOR_LOOP_ACTION_DEACTIVATE,
                        // LIGHTING_COLOR_LOOP_ACTION_ACTIVATE_FROM_START_HUE,
                        // LIGHTING_COLOR_LOOP_ACTION_ACTIVATE_FROM_ENH_CURR_HUE
    uint8_t direction;  // LIGHTING_COLOR_LOOP_DIRECTION_DECREMENT, LIGHTING_COLOR_LOOP_DIRECTION_INCREMENT
    uint16_t time;      // time in seconds to perform the color loop
    uint16_t start_hue; // start hue for the color loop
} s_zb_zcl_color_control_color_loop_set_t;

typedef struct s_zb_zcl_color_control_step_color_temperature
{
    uint8_t move_mode;
    uint16_t rate;
    uint16_t minimum_mireds;
    uint16_t maximum_mireds;
} s_zb_zcl_color_control_move_color_temperature_t;

typedef struct s_zb_zcl_color_control_enhanced_move_to_hue_and_saturation
{
    uint8_t step_mode;
    uint16_t step_size;
    uint16_t transition_time;
    uint16_t minimum_mireds;
    uint16_t maximum_mireds;
} s_zb_zcl_color_control_step_color_temperature_t;

/******************************************************************************
 * FUNCTION MACROS
 */
#define zb_zcl_lighting_color_control_send_stop_move_step_cmd(src_ep, dst_addr, disable_default_rsp, seq_num) \
    zb_zcl_send_cmd(src_ep, dst_addr, \
        ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_STOP_MOVE_STEP, \
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL)

/******************************************************************************
 * FUNCTIONS
 */

// ZCL Color Control Cluster Client Commands
/**
 * @brief Send a Move To Hue command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param hue - target hue
 * @param direction - direction to move in
 * @param transition_time - time to perform the color change, equal of the value of the field in 1/10 seconds
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_move_to_hue_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t hue, uint8_t direction, uint16_t transition_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Move Hue command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param move_mode - LIGHTING_MOVE_HUE_STOP, LIGHTING_MOVE_HUE_UP, LIGHTING_MOVE_HUE_DOWN
 * @param rate - the movement in steps per second, where step is a change in the device's hue of one unit
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_move_hue_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t move_mode, uint8_t rate,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Step Hue command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param step_mode - LIGHTING_STEP_HUE_UP, LIGHTING_STEP_HUE_DOWN
 * @param step_size - number of hue units to step
 * @param transition_time - time to perform the color change, equal of the value of the field in 1/10 seconds
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_step_hue_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t step_mode, uint8_t step_size, uint8_t transition_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Move To Saturation command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param saturation - target saturation
 * @param transition_time - time to perform the color change, equal of the value of the field in 1/10 seconds
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_move_to_saturation_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t saturation, uint16_t transition_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Move Saturation command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param move_mode - LIGHTING_MOVE_SATURATION_STOP, LIGHTING_MOVE_SATURATION_UP, LIGHTING_MOVE_SATURATION_DOWN
 * @param rate - the movement in steps per second, where step is a change in the device's saturation of one unit
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_move_saturation_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t move_mode, uint8_t rate,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Step Saturation command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param step_mode - LIGHTING_STEP_SATURATION_UP, LIGHTING_STEP_SATURATION_DOWN
 * @param step_size - number of units to change the saturation level by
 * @param transition_time - time to perform the color change, equal of the value of the field in 1/10 seconds
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_step_saturation_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t step_mode, uint8_t step_size, uint8_t transition_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Move To Hue And Saturation command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param hue - target hue
 * @param saturation - target saturation
 * @param transition_time - time to perform the color change, equal of the value of the field in 1/10 seconds
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_move_to_hue_and_saturation_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t hue, uint8_t saturation, uint16_t transition_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Move To Color command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param color_x - target color X
 * @param color_y - target color Y
 * @param transition_time - time to perform the color change, equal of the value of the field in 1/10 seconds
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_move_to_color_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t color_x, uint16_t color_y, uint16_t transition_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Move Color command to the server
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] rate_x Rate of movement in steps per second. A step is a change in the device's CurrentX attribute of one unit
 * @param[in] rate_y Rate of movement in steps per second. A step is a change in the device's CurrentY attribute of one unit
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_move_color_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    int16_t rate_x, int16_t rate_y,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Step Color command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param step_x - change to be added to the device's CurrentX attribute
 * @param step_y - change to be added to the device's CurrentY attribute
 * @param transition_time - time to perform the color change, equal of the value of the field in 1/10 seconds
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_step_color_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    int16_t step_x, int16_t step_y, uint16_t transition_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Move To Color Temperature command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param color_temperature - color temperature to move to
 * @param transition_time - time to perform the color change, equal of the value of the field in 1/10 seconds
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_move_to_color_temperature_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t color_temperature, uint16_t transition_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Enhanced Move To Hue command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param enhanced_hue - enhanced hue to move to
 * @param direction - direction to move in
 * @param transition_time - time to perform the color change, equal of the value of the field in 1/10 seconds
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_enhanced_move_to_hue_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t enhanced_hue, uint8_t direction, uint16_t transition_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Enhanced Move Hue command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param move_mode - LIGHTING_MOVE_HUE_STOP, LIGHTING_MOVE_HUE_UP, LIGHTING_MOVE_HUE_DOWN
 * @param rate - the movement in steps per second, where step is a change in the device's hue of one unit
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_enhanced_move_hue_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t move_mode, uint16_t rate,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Enhanced Step Hue command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param step_mode - LIGHTING_STEP_HUE_UP, LIGHTING_STEP_HUE_DOWN
 * @param step_size - change to the current value of the device's hue
 * @param transition_time - the movement in steps per 1/10 second
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_enhanced_step_hue_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t step_mode, uint16_t step_size, uint16_t transition_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Enhanced Move To Hue And Saturation command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param enhanced_hue - enhanced hue to move to
 * @param saturation - saturation to move to
 * @param transition_time - time to move, equal of the value of the field in 1/10 seconds
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_enhanced_move_to_hue_and_saturation_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t enhanced_hue, uint8_t saturation, uint16_t transition_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

/**
 * @brief Send a Color Loop Set command to the server
 * 
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param update_flags - which color loop attributes to update before the color loop is started
 * @param action - action to take for the color loop
 * @param direction - direction for the color loop (decrement or increment)
 * @param time - number of seconds over which to perform a full color loop
 * @param start_hue - starting hue to use for the color loop
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t zb_zcl_lighting_color_control_send_color_loop_set_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_color_control_color_loop_update_flags_t update_flags,
    uint8_t action, uint8_t direction, uint16_t time, uint16_t start_hue,
    uint8_t disable_default_rsp, uint8_t seq_num);

/******************************************************************************
 * Colour-space conversions
 *
 * Three representations meet in this cluster and devices support different
 * subsets of them:
 *
 *   hue / saturation  ZCL bytes, 0..254 (255 is reserved). Hue spans the full
 *                     colour circle, saturation runs grey..pure.
 *   CIE xy            CurrentX / CurrentY, unsigned 16-bit in units of
 *                     1/65536, capped at 0xFEFF by the spec.
 *   RGB               plain 8-bit sRGB, which is what an application or a UI
 *                     colour picker actually deals in.
 *
 * Brightness is NOT part of any of them here: it belongs to the Level Control
 * cluster. Every conversion therefore works at full value, and an RGB triple
 * that goes in dark comes back out at full brightness with the same hue.
 * Round-tripping preserves the colour, not the intensity.
 *
 * All conversions use the sRGB primaries with a D65 white point, matching what
 * zigbee2mqtt and Hue-style controllers assume.
 ******************************************************************************/

/**
 * @brief Convert a ZCL hue / saturation pair to 8-bit sRGB
 *
 * Evaluated at full value, so the result is the brightest RGB of that hue.
 *
 * @param hue - ZCL hue, 0..254 mapped onto 0..360 degrees
 * @param saturation - ZCL saturation, 0..254
 * @param r, g, b - out: 0..255 each. NULL pointers are rejected (no-op).
 */
void zb_zcl_lighting_hue_sat_to_rgb(
    uint8_t hue, uint8_t saturation, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @brief Convert 8-bit sRGB to a ZCL hue / saturation pair
 *
 * The value (brightness) component is discarded - set it through the Level
 * Control cluster. Pure black has no hue, and comes back as hue 0 / sat 0.
 *
 * @param r, g, b - 0..255 each
 * @param hue - out: ZCL hue, 0..254
 * @param saturation - out: ZCL saturation, 0..254
 */
void zb_zcl_lighting_rgb_to_hue_sat(
    uint8_t r, uint8_t g, uint8_t b, uint8_t *hue, uint8_t *saturation);

/**
 * @brief Convert 8-bit sRGB to CIE 1931 xy chromaticity
 *
 * @param r, g, b - 0..255 each
 * @param color_x - out: CurrentX, in units of 1/65536
 * @param color_y - out: CurrentY, in units of 1/65536
 */
void zb_zcl_lighting_rgb_to_xy(
    uint8_t r, uint8_t g, uint8_t b, uint16_t *color_x, uint16_t *color_y);

/**
 * @brief Convert CIE 1931 xy chromaticity to 8-bit sRGB
 *
 * Normalised so the brightest channel is 255 - xy carries no intensity, and a
 * dim triple would be indistinguishable from a desaturated one. Colours
 * outside the sRGB gamut (which the xy plane can express and a monitor cannot)
 * are clipped to the nearest representable one.
 *
 * @param color_x, color_y - CurrentX / CurrentY, in units of 1/65536
 * @param r, g, b - out: 0..255 each
 */
void zb_zcl_lighting_xy_to_rgb(
    uint16_t color_x, uint16_t color_y, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @brief Convert a ZCL hue / saturation pair into CIE 1931 xy chromaticity
 *
 * For devices whose ColorCapabilities advertise the CurrentX / CurrentY
 * attributes but not the hue / saturation commands.
 *
 * @param hue - ZCL hue, 0..254 mapped onto 0..360 degrees
 * @param saturation - ZCL saturation, 0..254
 * @param color_x - out: CurrentX, in units of 1/65536
 * @param color_y - out: CurrentY, in units of 1/65536
 */
void zb_zcl_lighting_hue_sat_to_xy(
    uint8_t hue, uint8_t saturation, uint16_t *color_x, uint16_t *color_y);

/**
 * @brief Convert CIE 1931 xy chromaticity into a ZCL hue / saturation pair
 *
 * The inverse of zb_zcl_lighting_hue_sat_to_xy(), for reporting the colour of
 * a light that only exposes CurrentX / CurrentY in terms an application that
 * thinks in hue / saturation can use.
 *
 * @param color_x, color_y - CurrentX / CurrentY, in units of 1/65536
 * @param hue - out: ZCL hue, 0..254
 * @param saturation - out: ZCL saturation, 0..254
 */
void zb_zcl_lighting_xy_to_hue_sat(
    uint16_t color_x, uint16_t color_y, uint8_t *hue, uint8_t *saturation);

/**
 * @brief Set a light's colour using whichever command it supports
 *
 * Not every colour light implements Move To Hue And Saturation: a device that
 * advertises only the CurrentX / CurrentY or the enhanced-hue capability
 * answers it with a Default Response of UNSUP_CLUSTER_COMMAND (0x81). This
 * picks the command matching @p color_capabilities, preferring plain hue /
 * saturation, then enhanced hue / saturation, then Move To Color.
 *
 * @param src_ep - Source endpoint
 * @param dst_addr - Destination address
 * @param color_capabilities - the device's ColorCapabilities attribute
 *                             (0x400A); 0 when it has not been read yet, in
 *                             which case plain hue / saturation is assumed
 * @param hue - target hue, 0..254
 * @param saturation - target saturation, 0..254
 * @param transition_time - time to perform the colour change, in 1/10 seconds
 * @param disable_default_rsp - Whether to disable the default response
 * @param seq_num - Sequence number
 * @return zb_status_t - ZB_SUCCESS if a command was sent, ZB_FAIL if the
 *         device advertises no colour capability this can drive
 */
zb_status_t zb_zcl_lighting_color_control_send_hue_sat_by_caps(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint16_t color_capabilities,
    uint8_t hue, uint8_t saturation, uint16_t transition_time,
    uint8_t disable_default_rsp, uint8_t seq_num);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZCL_LIGHTING_H_ */