#ifndef DEVICE_ZB_DEVICE_ELECTRICAL_MEASUREMENT_H_
#define DEVICE_ZB_DEVICE_ELECTRICAL_MEASUREMENT_H_

#include "device/zb_device.h"

typedef struct s_zb_device_em_scaling
{
    uint16_t ac_volt_mult;
    uint16_t ac_volt_div;
    uint16_t ac_curr_mult;
    uint16_t ac_curr_div;
    uint16_t ac_power_mult;
    uint16_t ac_power_div;
    uint16_t ac_freq_mult; // 0 if not supported
    uint16_t ac_freq_div;  // 0 if not supported
    uint16_t dc_volt_mult;
    uint16_t dc_volt_div;
    uint16_t dc_curr_mult;
    uint16_t dc_curr_div;
    uint16_t dc_power_mult;
    uint16_t dc_power_div;

} s_zb_device_em_scaling_t;

typedef struct s_zb_device_em_ctx
{
    uint32_t measurement_type;
    s_zb_device_em_scaling_t scale;

    /* Raw (pre-scaling) AC phase-A counts as received from the device.
     * Kept so the scaled floats can be recomputed when the scaling
     * multiplier/divisor attributes arrive in a later read response. */
    uint16_t rms_voltage_raw;    /* 0xFFFF = invalid/unknown */
    uint16_t rms_current_raw;    /* 0xFFFF = invalid/unknown */
    int16_t  active_power_raw;   /* 0x8000 = invalid/unknown */
    uint16_t ac_frequency_raw;   /* 0xFFFF = invalid/unknown */
    int16_t  reactive_power_raw; /* 0x8000 = invalid/unknown */
    uint16_t apparent_power_raw; /* 0xFFFF = invalid/unknown */
    /* PowerFactor (int8, -100..+100) needs no scaling; stored in power_factor_a.
     * 0x80 (-128) = invalid/unknown. */

    /* AC Phase A (always present if AC bit set) */
    float voltage_a;            /* V */
    float current_a;            /* A */
    float active_power_a;       /* W */
    float reactive_power_a;     /* VAR */
    float apparent_power_a;     /* VA */
    int8_t power_factor_a;      /* -100...+100 */
    float ac_frequency;         /* Hz */

    /* AC phase B (only if PHASE_B bit set) */
    float voltage_b;
    float current_b;
    float active_power_b;
    float reactive_power_b;
    float apparent_power_b;
    int8_t power_factor_b;

    /* AC phase C (only if PHASE_C bit set) */
    float voltage_c;
    float current_c;
    float active_power_c;
    float reactive_power_c;
    float apparent_power_c;
    int8_t power_factor_c;

    /* 3-phase totals */
    float total_active_power;
    float total_reactive_power;
    float total_apparent_power;
    float neutral_current;

    /* DC (only if DC bit set) */
    float voltage_dc;
    float current_dc;
    float power_dc;
} s_zb_device_electrical_measurement_ctx_t;

extern s_zb_function_ops_t zb_device_electrical_measurement_ops;

#endif /* DEVICE_ZB_DEVICE_ELECTRICAL_MEASUREMENT_H_ */