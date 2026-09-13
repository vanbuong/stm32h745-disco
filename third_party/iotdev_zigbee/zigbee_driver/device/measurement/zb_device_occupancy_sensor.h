#ifndef DEVICE_ZB_DEVICE_OCCUPANCY_SENSOR_H_
#define DEVICE_ZB_DEVICE_OCCUPANCY_SENSOR_H_

#include "device/zb_device.h"

typedef struct s_zb_device_occupancy_config
{
    uint16_t occupied_to_unoccupied_delay;
    uint16_t unoccupied_to_occupied_delay;
    uint8_t unoccupied_to_occupied_threshold;
} s_zb_device_occupancy_config_t;

typedef struct s_zb_device_occupancy_sensor_ctx
{
    bool occupied;
    uint8_t sensor_type;
    uint8_t sensor_type_bitmap;
    s_zb_device_occupancy_config_t ultrasonic_config;
    s_zb_device_occupancy_config_t pir_config;
    s_zb_device_occupancy_config_t physical_contact_config;

    /* Host-side motion timeout, for sensors that announce a detection and then
     * stay silent instead of reporting that it ended (Aqara). Without it,
     * `occupied` latches true after the first detection.
     *   auto_clear_ms : 0 = off (the sensor is trusted to send occupancy=0).
     *                   Seeded per model from the Device Quirk Register, then
     *                   replaced by the device's own
     *                   PIROccupiedToUnoccupiedDelay if it reports one.
     *   clear_at_ms   : monotonic deadline, 0 = not armed. Re-armed by every
     *                   detection, so continuous motion never clears. */
    uint32_t auto_clear_ms;
    uint32_t clear_at_ms;
} s_zb_device_occupancy_sensor_ctx_t;

extern s_zb_function_ops_t zb_device_occupancy_sensor_ops;

#endif /* DEVICE_ZB_DEVICE_OCCUPANCY_SENSOR_H_ */