#include "common/zb_sensor_units.h"

e_zb_sensor_unit_t
zb_sensor_unit_for_event(e_zb_event_type_t type)
{
    switch (type)
    {
        case ZB_EVENT_SENSOR_TEMPERATURE:  return ZB_UNIT_DEGC;
        case ZB_EVENT_SENSOR_HUMIDITY:     return ZB_UNIT_PERCENT_RH;
        case ZB_EVENT_SENSOR_PRESSURE:     return ZB_UNIT_HPA;
        case ZB_EVENT_SENSOR_ILLUMINANCE:  return ZB_UNIT_LUX;
        case ZB_EVENT_SENSOR_FLOW:         return ZB_UNIT_M3_PER_H;
        case ZB_EVENT_SENSOR_CO2:          return ZB_UNIT_PPM;
        case ZB_EVENT_SENSOR_TVOC:
        case ZB_EVENT_SENSOR_FORMALDEHYDE: return ZB_UNIT_PPB;
        case ZB_EVENT_SENSOR_PM25:
        case ZB_EVENT_SENSOR_PM10:
        case ZB_EVENT_SENSOR_PM1:          return ZB_UNIT_UG_PER_M3;
        case ZB_EVENT_SENSOR_IAQ:          return ZB_UNIT_NONE;
        case ZB_EVENT_SENSOR_ECO2:         return ZB_UNIT_PPM;
        default:                           return ZB_UNIT_NONE;
    }
}

const char *
zb_sensor_unit_to_string(e_zb_sensor_unit_t unit)
{
    switch (unit)
    {
        case ZB_UNIT_DEGC:         return "°C";
        case ZB_UNIT_PERCENT_RH:   return "%RH";
        case ZB_UNIT_HPA:          return "hPa";
        case ZB_UNIT_LUX:          return "lux";
        case ZB_UNIT_M3_PER_H:     return "m³/h";
        case ZB_UNIT_PPM:          return "ppm";
        case ZB_UNIT_UG_PER_M3:    return "µg/m³";
        case ZB_UNIT_PPB:          return "ppb";
        default:                   return "";
    }
}
