#ifndef ZB_SENSOR_UNITS_H_
#define ZB_SENSOR_UNITS_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common_types.h"

/*
 * Lookup helpers for e_zb_sensor_unit_t (defined in zb_common_types.h).
 *
 * Battery, electrical, energy, occupancy, and IAS events use dedicated union
 * members; units are implied by field name (%, V, kWh, …).
 */

e_zb_sensor_unit_t
zb_sensor_unit_for_event(e_zb_event_type_t type);

const char *
zb_sensor_unit_to_string(e_zb_sensor_unit_t unit);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_SENSOR_UNITS_H_ */
