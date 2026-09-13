#ifndef DEVICE_ZB_DEVICE_ENERGY_METER_H_
#define DEVICE_ZB_DEVICE_ENERGY_METER_H_

#include "device/zb_device.h"

/*
 * Energy meter function for the Smart Energy Metering cluster
 * (ZCL_CLUSTER_ID_SE_METERING, 0x0702).
 *
 * Tracks cumulative energy (CurrentSummationDelivered, uint48) and live
 * power (InstantaneousDemand, int24). Both raw values are scaled by the
 * Multiplier / Divisor formatting attributes before being reported up.
 */
typedef struct s_zb_device_energy_meter_ctx
{
    uint8_t  device_type;       /* ATTRID_SE_METERING_DEVICE_TYPE (ZCL_SE_METERING_*) */
    uint8_t  unit_of_measure;   /* ATTRID_SE_METERING_UOM                             */
    uint32_t multiplier;        /* ATTRID_SE_METERING_MULT (uint24), 0 => treated as 1 */
    uint32_t divisor;           /* ATTRID_SE_METERING_DIV  (uint24), 0 => treated as 1 */

    uint64_t summation_delivered_raw;  /* ATTRID_SE_METERING_CURR_SUMM_DLVD (uint48)  */
    int32_t  instantaneous_demand_raw; /* ATTRID_SE_METERING_INST_DMD (int24)         */

    float    energy;            /* scaled CurrentSummationDelivered (e.g. kWh) */
    float    power;             /* scaled InstantaneousDemand (e.g. kW)        */
} s_zb_device_energy_meter_ctx_t;

extern s_zb_function_ops_t zb_device_energy_meter_ops;

#endif /* DEVICE_ZB_DEVICE_ENERGY_METER_H_ */
