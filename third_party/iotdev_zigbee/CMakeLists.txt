set(ZB_DEVICE_LIST_PB_H ${CMAKE_CURRENT_LIST_DIR}/zigbee_driver/proto/zb_device.pb.h)
set(ZB_DEVICE_LIST_PB_C ${CMAKE_CURRENT_LIST_DIR}/zigbee_driver/proto/zb_device.pb.c)

set(COMPONENT_SRCS
	"source/iotdev_zigbee.c"
    ${ZB_DEVICE_LIST_PB_C}
    "zigbee_driver/af/zb_af.c"
    "zigbee_driver/common/zb_common.c"
    "zigbee_driver/common/zb_sensor_units.c"
    "zigbee_driver/osal/zb_osal_freertos.c"
    "zigbee_driver/core/zb_core.c"
    "zigbee_driver/device/generic/zb_device_basic_info.c"
    "zigbee_driver/device/generic/zb_device_battery_sensor.c"
    "zigbee_driver/device/generic/zb_device_dimmer_switch.c"
    "zigbee_driver/device/generic/zb_device_on_off_switch.c"
    "zigbee_driver/device/generic/zb_device_button.c"
    "zigbee_driver/device/generic/zb_device_mains_power_outlet.c"
    "zigbee_driver/device/generic/zb_device_relay.c"
    # zigbee_driver/"device/generic/zb_device_range_extender.c"
    "zigbee_driver/device/generic/zb_device_on_off_smart_plug.c"
    "zigbee_driver/device/measurement/zb_device_temperature_sensor.c"
    "zigbee_driver/device/measurement/zb_device_humidity_sensor.c"
    "zigbee_driver/device/measurement/zb_device_pressure_sensor.c"
    "zigbee_driver/device/measurement/zb_device_flow_sensor.c"
    "zigbee_driver/device/measurement/zb_device_illuminance_sensor.c"
    "zigbee_driver/device/measurement/zb_device_pm25_sensor.c"
    "zigbee_driver/device/measurement/zb_device_co2_sensor.c"
    "zigbee_driver/device/measurement/zb_device_pm10_sensor.c"
    "zigbee_driver/device/measurement/zb_device_pm1_sensor.c"
    "zigbee_driver/device/measurement/zb_device_tvoc_sensor.c"
    "zigbee_driver/device/measurement/zb_device_develco_voc_sensor.c"
    "zigbee_driver/device/measurement/zb_device_formaldehyde_sensor.c"
    "zigbee_driver/device/measurement/zb_device_iaq_sensor.c"
    "zigbee_driver/device/measurement/zb_device_eco2_sensor.c"
    "zigbee_driver/device/measurement/zb_device_occupancy_sensor.c"
    "zigbee_driver/device/measurement/zb_device_electrical_measurement.c"
    "zigbee_driver/device/measurement/zb_device_energy_meter.c"
    "zigbee_driver/device/intruder/zb_device_ias_zone.c"
    "zigbee_driver/device/intruder/zb_device_ias_warning.c"
    "zigbee_driver/device/lighting/zb_device_color_temp_light.c"
    "zigbee_driver/device/lighting/zb_device_color_light.c"
    "zigbee_driver/device/lighting/zb_device_dimmable_light.c"
    "zigbee_driver/device/lighting/zb_device_on_off_light.c"
    "zigbee_driver/device/lighting/zb_device_extended_color_light.c"
    "zigbee_driver/manu/zb_manu.c"
    "zigbee_driver/manu/lumi/zb_manu_lumi.c"
    "zigbee_driver/manu/tuya/zb_manu_tuya.c"
    "zigbee_driver/device/zb_device_db.c"
    "zigbee_driver/device/zb_device_state_cache.c"
    "zigbee_driver/device/zb_device_schema.c"
    "zigbee_driver/device/zb_device_manager.c"
    "zigbee_driver/network/zb_network_service.c"
    "zigbee_driver/ota/zb_ota_server.c"
    "zigbee_driver/platform/iotdev/zb_iotdev_serial.c"
    "zigbee_driver/zcl/zb_zcl.c"
    "zigbee_driver/zcl/zb_zcl_manu.c"
    "zigbee_driver/zcl/zb_zcl_general.c"
    "zigbee_driver/zcl/zb_zcl_closures.c"
    "zigbee_driver/zcl/zb_zcl_poll_control.c"
    "zigbee_driver/zcl/zb_zcl_lighting.c"
    "zigbee_driver/zcl/zb_zcl_electrical_measurement.c"
    "zigbee_driver/zcl/zb_zcl_smart_energy.c"
    "zigbee_driver/zcl/zb_zcl_ss.c"
    "zigbee_driver/zcl/zb_zcl_ota.c"
    "zigbee_driver/zdo/zb_zdo.c"
    "zigbee_driver/znp/zb_znp_mt_af.c"
    "zigbee_driver/znp/zb_znp_mt_app.c"
    "zigbee_driver/znp/zb_znp_mt_app_cfg.c"
    "zigbee_driver/znp/zb_znp_mt_util.c"
    "zigbee_driver/znp/zb_znp_mt_sys.c"
    "zigbee_driver/znp/zb_znp_mt_zdo.c"
    "zigbee_driver/znp/zb_znp_sbl.c"
    "zigbee_driver/znp/zb_znp.c"
)

set(COMPONENT_ADD_INCLUDEDIRS
    "."
    ".."
    "include"
    "zigbee_driver"
    "zigbee_driver/osal"
)

set(COMPONENT_PRIV_REQUIRES
    "iotdev_common"
    "iotdev_config"
    "iotdev_filesystem"
    "iotdev_gpio"
    "iotdev_taskmgr"
    "iotdev_timers"
    "iotdev_uart"
    "iotdev_utils"
    "iotdev_protobuf"
    "iotdev_modbus"
    "esp_driver_gpio"
    "esp_timer"
)

register_component()
target_compile_definitions(${COMPONENT_LIB} PUBLIC ZB_PLATFORM_IOTDEV)

set(NANOPB_PYTHON3  "python")
set(NANOPB_GENERATOR "${CMAKE_CURRENT_LIST_DIR}/../iotdev_utils/iotdev_protobuf/nanopb/generator/nanopb_generator.py")

set(IOTDEV_PROTO_SRCS "zb_device.proto")

add_custom_command(OUTPUT zb_device.pb.c zb_device.pb.h
     PRE_BUILD
     COMMAND ${NANOPB_PYTHON3} ${NANOPB_GENERATOR} --output-dir=. ${IOTDEV_PROTO_SRCS}
     DEPENDS ${CMAKE_CURRENT_LIST_DIR}/zigbee_driver/proto/zb_device.proto
     BYPRODUCTS ${ZB_DEVICE_LIST_PB_C} ${ZB_DEVICE_LIST_PB_H}
     VERBATIM
     WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/zigbee_driver/proto
     )

add_custom_target(zb_device DEPENDS zb_device.pb.c zb_device.pb.h)
add_dependencies(${COMPONENT_LIB} zb_device)
