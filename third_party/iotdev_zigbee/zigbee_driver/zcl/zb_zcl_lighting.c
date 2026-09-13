#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"
#include "zcl/zb_zcl_lighting.h"

#include <math.h>

#define TAG "ZCL_LIGHTING"

/*********************************************************************
 * FUNCTIONS
 */
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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[4];

    buf[0] = hue;
    buf[1] = direction;
    buf[2] = LO_UINT16(transition_time);
    buf[3] = HI_UINT16(transition_time);

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_MOVE_TO_HUE,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 4, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[2];

    buf[0] = move_mode;
    buf[1] = LO_UINT16(rate);

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_MOVE_HUE,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 2, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[3];

    buf[0] = step_mode;
    buf[1] = step_size;
    buf[2] = transition_time;

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_STEP_HUE,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 3, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[3];

    buf[0] = saturation;
    buf[1] = LO_UINT16(transition_time);
    buf[2] = HI_UINT16(transition_time);

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_MOVE_TO_SATURATION,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 3, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[2];

    buf[0] = move_mode;
    buf[1] = rate;

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_MOVE_SATURATION,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 2, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[3];

    buf[0] = step_mode;
    buf[1] = step_size;
    buf[2] = transition_time;

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_STEP_SATURATION,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 3, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[4];

    buf[0] = hue;
    buf[1] = saturation;
    buf[2] = LO_UINT16(transition_time);
    buf[3] = HI_UINT16(transition_time);

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_MOVE_TO_HUE_AND_SATURATION,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 4, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[6];

    buf[0] = LO_UINT16(color_x);
    buf[1] = HI_UINT16(color_x);
    buf[2] = LO_UINT16(color_y);
    buf[3] = HI_UINT16(color_y);
    buf[4] = LO_UINT16(transition_time);
    buf[5] = HI_UINT16(transition_time);

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_MOVE_TO_COLOR,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 6, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[4];

    buf[0] = LO_UINT16(rate_x);
    buf[1] = HI_UINT16(rate_x);
    buf[2] = LO_UINT16(rate_y);
    buf[3] = HI_UINT16(rate_y);

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_MOVE_COLOR,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 4, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[6];
    buf[0] = LO_UINT16(step_x);
    buf[1] = HI_UINT16(step_x);
    buf[2] = LO_UINT16(step_y);
    buf[3] = HI_UINT16(step_y);
    buf[4] = LO_UINT16(transition_time);
    buf[5] = HI_UINT16(transition_time);

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_STEP_COLOR,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 6, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[4];
    buf[0] = LO_UINT16(color_temperature);
    buf[1] = HI_UINT16(color_temperature);
    buf[2] = LO_UINT16(transition_time);
    buf[3] = HI_UINT16(transition_time);

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_MOVE_TO_COLOR_TEMPERATURE,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 4, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[5];

    buf[0] = LO_UINT16(enhanced_hue);
    buf[1] = HI_UINT16(enhanced_hue);
    buf[2] = direction;
    buf[3] = LO_UINT16(transition_time);
    buf[4] = HI_UINT16(transition_time);

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_ENHANCED_MOVE_TO_HUE,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 5, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[3];
    buf[0] = move_mode;
    buf[1] = LO_UINT16(rate);
    buf[2] = HI_UINT16(rate);

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_ENHANCED_MOVE_HUE,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 3, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[5];

    buf[0] = step_mode;
    buf[1] = LO_UINT16(step_size);
    buf[2] = HI_UINT16(step_size);
    buf[3] = LO_UINT16(transition_time);
    buf[4] = HI_UINT16(transition_time);

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_ENHANCED_STEP_HUE,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 5, buf);
}

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
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[5];

    buf[0] = LO_UINT16(enhanced_hue);
    buf[1] = HI_UINT16(enhanced_hue);
    buf[2] = saturation;
    buf[3] = LO_UINT16(transition_time);
    buf[4] = HI_UINT16(transition_time);

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_ENHANCED_MOVE_TO_HUE_AND_SATURATION,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 5, buf);
}

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
    uint8_t action, uint8_t direction, uint16_t time,
    uint16_t start_hue, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[7];

    buf[0] = update_flags.byte;
    buf[1] = action;
    buf[2] = direction;
    buf[3] = LO_UINT16(time);
    buf[4] = HI_UINT16(time);
    buf[5] = LO_UINT16(start_hue);
    buf[6] = HI_UINT16(start_hue);

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, COMMAND_COLOR_CONTROL_COLOR_LOOP_SET,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 7, buf);
}


/* ---------------------------------------------------------------------------
 * Colour-space conversions. See the header for what each representation means
 * and why brightness is deliberately not carried through any of them.
 * -------------------------------------------------------------------------*/

/* ZCL hue and saturation are bytes over 0..254 - 255 is reserved. */
#define ZCL_LIGHTING_HUE_SAT_MAX        254.0f

/** Expand one sRGB channel to linear light (inverse sRGB transfer curve). */
static float
zcl_lighting_srgb_to_linear(float c)
{
    return (c <= 0.04045f) ? (c / 12.92f) : powf((c + 0.055f) / 1.055f, 2.4f);
}

/** Compress one linear-light channel back to sRGB. */
static float
zcl_lighting_linear_to_srgb(float c)
{
    if (c <= 0.0f)
    {
        return 0.0f;
    }
    if (c >= 1.0f)
    {
        return 1.0f;
    }
    return (c <= 0.0031308f) ? (c * 12.92f) : (1.055f * powf(c, 1.0f / 2.4f) - 0.055f);
}

/** Round a 0..1 float to a 0..255 byte. */
static uint8_t
zcl_lighting_unit_to_byte(float v)
{
    if (v <= 0.0f)
    {
        return 0;
    }
    if (v >= 1.0f)
    {
        return 255;
    }
    return (uint8_t)(v * 255.0f + 0.5f);
}

void zb_zcl_lighting_hue_sat_to_rgb(
    uint8_t hue, uint8_t saturation, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if ((r == NULL) || (g == NULL) || (b == NULL))
    {
        return;
    }

    /* HSV -> RGB at value = 1: ZCL hue spans 0..254 over the whole circle. */
    float h = ((float)hue * 360.0f) / ZCL_LIGHTING_HUE_SAT_MAX;
    float chroma = (float)saturation / ZCL_LIGHTING_HUE_SAT_MAX;
    float hp = h / 60.0f;
    float second = chroma * (1.0f - fabsf(fmodf(hp, 2.0f) - 1.0f));
    float m = 1.0f - chroma;
    float rf, gf, bf;

    if (hp < 1.0f)      { rf = chroma; gf = second; bf = 0.0f;   }
    else if (hp < 2.0f) { rf = second; gf = chroma; bf = 0.0f;   }
    else if (hp < 3.0f) { rf = 0.0f;   gf = chroma; bf = second; }
    else if (hp < 4.0f) { rf = 0.0f;   gf = second; bf = chroma; }
    else if (hp < 5.0f) { rf = second; gf = 0.0f;   bf = chroma; }
    else                { rf = chroma; gf = 0.0f;   bf = second; }

    *r = zcl_lighting_unit_to_byte(rf + m);
    *g = zcl_lighting_unit_to_byte(gf + m);
    *b = zcl_lighting_unit_to_byte(bf + m);
}

void zb_zcl_lighting_rgb_to_hue_sat(
    uint8_t r, uint8_t g, uint8_t b, uint8_t *hue, uint8_t *saturation)
{
    if ((hue == NULL) || (saturation == NULL))
    {
        return;
    }

    float rf = (float)r / 255.0f;
    float gf = (float)g / 255.0f;
    float bf = (float)b / 255.0f;

    float max = (rf > gf) ? ((rf > bf) ? rf : bf) : ((gf > bf) ? gf : bf);
    float min = (rf < gf) ? ((rf < bf) ? rf : bf) : ((gf < bf) ? gf : bf);
    float chroma = max - min;

    /* Grey (including black): no hue to speak of, and saturation is zero. */
    if ((chroma <= 0.0f) || (max <= 0.0f))
    {
        *hue = 0;
        *saturation = 0;
        return;
    }

    float h;
    if (max == rf)      { h = fmodf((gf - bf) / chroma, 6.0f); }
    else if (max == gf) { h = ((bf - rf) / chroma) + 2.0f; }
    else                { h = ((rf - gf) / chroma) + 4.0f; }
    h *= 60.0f;
    if (h < 0.0f)
    {
        h += 360.0f;
    }

    /* 360 degrees maps back onto 0, not onto 255. */
    uint32_t hue_scaled = (uint32_t)((h * ZCL_LIGHTING_HUE_SAT_MAX / 360.0f) + 0.5f);
    if (hue_scaled > (uint32_t)ZCL_LIGHTING_HUE_SAT_MAX)
    {
        hue_scaled = 0;
    }

    *hue = (uint8_t)hue_scaled;
    *saturation = (uint8_t)(((chroma / max) * ZCL_LIGHTING_HUE_SAT_MAX) + 0.5f);
}

void zb_zcl_lighting_rgb_to_xy(
    uint8_t r, uint8_t g, uint8_t b, uint16_t *color_x, uint16_t *color_y)
{
    if ((color_x == NULL) || (color_y == NULL))
    {
        return;
    }

    /* sRGB -> CIE XYZ (D65), then normalise onto the xy chromaticity plane. */
    float rf = zcl_lighting_srgb_to_linear((float)r / 255.0f);
    float gf = zcl_lighting_srgb_to_linear((float)g / 255.0f);
    float bf = zcl_lighting_srgb_to_linear((float)b / 255.0f);

    float big_x = 0.4124f * rf + 0.3576f * gf + 0.1805f * bf;
    float big_y = 0.2126f * rf + 0.7152f * gf + 0.0722f * bf;
    float big_z = 0.0193f * rf + 0.1192f * gf + 0.9505f * bf;
    float sum = big_x + big_y + big_z;

    float cx = (sum > 0.0f) ? (big_x / sum) : 0.0f;
    float cy = (sum > 0.0f) ? (big_y / sum) : 0.0f;

    /* CurrentX / CurrentY are unsigned 16-bit in units of 1/65536, capped at
     * 0xFEFF by the cluster spec. */
    uint32_t xi = (uint32_t)(cx * 65536.0f + 0.5f);
    uint32_t yi = (uint32_t)(cy * 65536.0f + 0.5f);
    if (xi > LIGHTING_COLOR_CURRENT_X_MAX) xi = LIGHTING_COLOR_CURRENT_X_MAX;
    if (yi > LIGHTING_COLOR_CURRENT_Y_MAX) yi = LIGHTING_COLOR_CURRENT_Y_MAX;

    *color_x = (uint16_t)xi;
    *color_y = (uint16_t)yi;
}

void zb_zcl_lighting_xy_to_rgb(
    uint16_t color_x, uint16_t color_y, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if ((r == NULL) || (g == NULL) || (b == NULL))
    {
        return;
    }

    float cx = (float)color_x / 65536.0f;
    float cy = (float)color_y / 65536.0f;

    /* y = 0 is not a colour: the xy plane is undefined there. */
    if (cy <= 0.0f)
    {
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }

    /* Pick Y = 1 (full brightness) and recover the tristimulus values. */
    float big_y = 1.0f;
    float big_x = (cx * big_y) / cy;
    float big_z = ((1.0f - cx - cy) * big_y) / cy;

    /* CIE XYZ -> linear sRGB (D65), the inverse of the matrix above. */
    float rf =  3.2406f * big_x - 1.5372f * big_y - 0.4986f * big_z;
    float gf = -0.9689f * big_x + 1.8758f * big_y + 0.0415f * big_z;
    float bf =  0.0557f * big_x - 0.2040f * big_y + 1.0570f * big_z;

    /* Colours outside the sRGB gamut come out negative - clip them to the
     * nearest colour the primaries can actually make. */
    if (rf < 0.0f) rf = 0.0f;
    if (gf < 0.0f) gf = 0.0f;
    if (bf < 0.0f) bf = 0.0f;

    /* xy carries no intensity, so normalise to the brightest channel; without
     * this a saturated colour would come back arbitrarily dim. */
    float max = (rf > gf) ? ((rf > bf) ? rf : bf) : ((gf > bf) ? gf : bf);
    if (max > 0.0f)
    {
        rf /= max;
        gf /= max;
        bf /= max;
    }

    *r = zcl_lighting_unit_to_byte(zcl_lighting_linear_to_srgb(rf));
    *g = zcl_lighting_unit_to_byte(zcl_lighting_linear_to_srgb(gf));
    *b = zcl_lighting_unit_to_byte(zcl_lighting_linear_to_srgb(bf));
}

void zb_zcl_lighting_hue_sat_to_xy(
    uint8_t hue, uint8_t saturation, uint16_t *color_x, uint16_t *color_y)
{
    uint8_t r = 0, g = 0, b = 0;

    if ((color_x == NULL) || (color_y == NULL))
    {
        return;
    }
    zb_zcl_lighting_hue_sat_to_rgb(hue, saturation, &r, &g, &b);
    zb_zcl_lighting_rgb_to_xy(r, g, b, color_x, color_y);
}

void zb_zcl_lighting_xy_to_hue_sat(
    uint16_t color_x, uint16_t color_y, uint8_t *hue, uint8_t *saturation)
{
    uint8_t r = 0, g = 0, b = 0;

    if ((hue == NULL) || (saturation == NULL))
    {
        return;
    }
    zb_zcl_lighting_xy_to_rgb(color_x, color_y, &r, &g, &b);
    zb_zcl_lighting_rgb_to_hue_sat(r, g, b, hue, saturation);
}

zb_status_t zb_zcl_lighting_color_control_send_hue_sat_by_caps(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint16_t color_capabilities,
    uint8_t hue, uint8_t saturation, uint16_t transition_time,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    /* Capabilities not read back yet: plain hue / saturation is the mandatory
     * form for a hue/sat colour light, so it stays the optimistic default. */
    if (color_capabilities == 0 ||
        (color_capabilities & COLOR_CAPABILITIES_ATTR_BIT_HUE_SATURATION))
    {
        return zb_zcl_lighting_color_control_send_move_to_hue_and_saturation_cmd(
                    src_ep, dst_addr, hue, saturation, transition_time,
                    disable_default_rsp, seq_num);
    }

    if (color_capabilities & COLOR_CAPABILITIES_ATTR_BIT_ENHANCED_HUE)
    {
        /* Enhanced hue is the 16-bit form of the same circle, so the 8-bit hue
         * becomes its high byte. */
        ZB_LOGI(TAG, "Color caps 0x%04x: using enhanced hue/saturation", color_capabilities);
        return zb_zcl_lighting_color_control_send_enhanced_move_to_hue_and_saturation_cmd(
                    src_ep, dst_addr, (uint16_t)hue << 8, saturation,
                    transition_time, disable_default_rsp, seq_num);
    }

    if (color_capabilities & COLOR_CAPABILITIES_ATTR_BIT_X_Y_ATTRIBUTES)
    {
        uint16_t color_x = 0, color_y = 0;
        zb_zcl_lighting_hue_sat_to_xy(hue, saturation, &color_x, &color_y);
        ZB_LOGI(TAG, "Color caps 0x%04x: using move to color xy (0x%04x, 0x%04x)",
                color_capabilities, color_x, color_y);
        return zb_zcl_lighting_color_control_send_move_to_color_cmd(
                    src_ep, dst_addr, color_x, color_y, transition_time,
                    disable_default_rsp, seq_num);
    }

    ZB_LOGW(TAG, "Color caps 0x%04x: device supports no settable colour mode", color_capabilities);
    return ZB_FAIL;
}
