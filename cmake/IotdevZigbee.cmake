if(NOT DEFINED H745_ROOT)
    get_filename_component(H745_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()
include_guard(GLOBAL)

set(IOTDEV_ZB_ROOT ${H745_ROOT}/third_party/iotdev_zigbee)
set(IOTDEV_ZB_DRV ${IOTDEV_ZB_ROOT}/zigbee_driver)
set(IOTDEV_ZB_PORT ${H745_ROOT}/firmware/src/port/iotdev)
set(IOTDEV_ZB_NANOPB ${IOTDEV_ZB_PORT}/iotdev_utils/iotdev_protobuf/nanopb)

set(IOTDEV_ZB_VENDOR_SRC
    ${IOTDEV_ZB_DRV}/proto/zb_device.pb.c
    ${IOTDEV_ZB_DRV}/af/zb_af.c
    ${IOTDEV_ZB_DRV}/common/zb_common.c
    ${IOTDEV_ZB_DRV}/common/zb_sensor_units.c
    ${IOTDEV_ZB_DRV}/core/zb_core.c
    ${IOTDEV_ZB_DRV}/device/generic/zb_device_basic_info.c
    ${IOTDEV_ZB_DRV}/device/generic/zb_device_battery_sensor.c
    ${IOTDEV_ZB_DRV}/device/generic/zb_device_dimmer_switch.c
    ${IOTDEV_ZB_DRV}/device/generic/zb_device_on_off_switch.c
    ${IOTDEV_ZB_DRV}/device/generic/zb_device_button.c
    ${IOTDEV_ZB_DRV}/device/generic/zb_device_mains_power_outlet.c
    ${IOTDEV_ZB_DRV}/device/generic/zb_device_relay.c
    ${IOTDEV_ZB_DRV}/device/generic/zb_device_on_off_smart_plug.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_temperature_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_humidity_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_pressure_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_flow_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_illuminance_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_pm25_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_co2_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_pm10_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_pm1_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_tvoc_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_develco_voc_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_formaldehyde_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_iaq_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_eco2_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_occupancy_sensor.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_electrical_measurement.c
    ${IOTDEV_ZB_DRV}/device/measurement/zb_device_energy_meter.c
    ${IOTDEV_ZB_DRV}/device/intruder/zb_device_ias_zone.c
    ${IOTDEV_ZB_DRV}/device/intruder/zb_device_ias_warning.c
    ${IOTDEV_ZB_DRV}/device/lighting/zb_device_color_temp_light.c
    ${IOTDEV_ZB_DRV}/device/lighting/zb_device_color_light.c
    ${IOTDEV_ZB_DRV}/device/lighting/zb_device_dimmable_light.c
    ${IOTDEV_ZB_DRV}/device/lighting/zb_device_on_off_light.c
    ${IOTDEV_ZB_DRV}/device/lighting/zb_device_extended_color_light.c
    ${IOTDEV_ZB_DRV}/manu/zb_manu.c
    ${IOTDEV_ZB_DRV}/manu/lumi/zb_manu_lumi.c
    ${IOTDEV_ZB_DRV}/manu/tuya/zb_manu_tuya.c
    ${IOTDEV_ZB_DRV}/device/zb_device_db.c
    ${IOTDEV_ZB_DRV}/device/zb_device_state_cache.c
    ${IOTDEV_ZB_DRV}/device/zb_device_schema.c
    ${IOTDEV_ZB_DRV}/device/zb_device_manager.c
    ${IOTDEV_ZB_DRV}/network/zb_network_service.c
    ${IOTDEV_ZB_DRV}/zcl/zb_zcl.c
    ${IOTDEV_ZB_DRV}/zcl/zb_zcl_manu.c
    ${IOTDEV_ZB_DRV}/zcl/zb_zcl_general.c
    ${IOTDEV_ZB_DRV}/zcl/zb_zcl_closures.c
    ${IOTDEV_ZB_DRV}/zcl/zb_zcl_poll_control.c
    ${IOTDEV_ZB_DRV}/zcl/zb_zcl_lighting.c
    ${IOTDEV_ZB_DRV}/zcl/zb_zcl_electrical_measurement.c
    ${IOTDEV_ZB_DRV}/zcl/zb_zcl_smart_energy.c
    ${IOTDEV_ZB_DRV}/zcl/zb_zcl_ss.c
    ${IOTDEV_ZB_DRV}/zcl/zb_zcl_ota.c
    ${IOTDEV_ZB_DRV}/zdo/zb_zdo.c
    ${IOTDEV_ZB_DRV}/znp/zb_znp_mt_af.c
    ${IOTDEV_ZB_DRV}/znp/zb_znp_mt_app.c
    ${IOTDEV_ZB_DRV}/znp/zb_znp_mt_app_cfg.c
    ${IOTDEV_ZB_DRV}/znp/zb_znp_mt_util.c
    ${IOTDEV_ZB_DRV}/znp/zb_znp_mt_sys.c
    ${IOTDEV_ZB_DRV}/znp/zb_znp_mt_zdo.c
    ${IOTDEV_ZB_DRV}/znp/zb_znp_sbl.c
    ${IOTDEV_ZB_DRV}/znp/zb_znp.c
)

set(IOTDEV_ZB_PORT_SRC
    ${IOTDEV_ZB_PORT}/zb_osal_superloop.c
    ${IOTDEV_ZB_PORT}/zb_plat_serial_h745.c
    ${IOTDEV_ZB_PORT}/zb_port_fs.c
    ${IOTDEV_ZB_PORT}/zb_port_time.c
    ${IOTDEV_ZB_PORT}/iotdev_shims.c
    ${IOTDEV_ZB_PORT}/zb_ota_stub.c
    ${IOTDEV_ZB_NANOPB}/pb_common.c
    ${IOTDEV_ZB_NANOPB}/pb_encode.c
    ${IOTDEV_ZB_NANOPB}/pb_decode.c
)

function(stm32_add_iotdev_zigbee TGT)
    target_sources(${TGT} PRIVATE ${IOTDEV_ZB_VENDOR_SRC} ${IOTDEV_ZB_PORT_SRC})
    target_include_directories(${TGT} PRIVATE
        ${IOTDEV_ZB_ROOT}
        ${IOTDEV_ZB_DRV}
        ${IOTDEV_ZB_DRV}/osal
        ${IOTDEV_ZB_DRV}/proto
        ${IOTDEV_ZB_PORT}
        ${IOTDEV_ZB_NANOPB}
        ${H745_ROOT}/firmware/include
    )
    target_compile_definitions(${TGT} PRIVATE
        ZB_IOTDEV_DRIVER
        ZB_USE_VFS
        ZB_MAX_DEVICE=32
    )
    set_source_files_properties(${IOTDEV_ZB_VENDOR_SRC} ${IOTDEV_ZB_NANOPB}/pb_common.c
        ${IOTDEV_ZB_NANOPB}/pb_encode.c ${IOTDEV_ZB_NANOPB}/pb_decode.c
        PROPERTIES COMPILE_FLAGS "-w")
endfunction()
