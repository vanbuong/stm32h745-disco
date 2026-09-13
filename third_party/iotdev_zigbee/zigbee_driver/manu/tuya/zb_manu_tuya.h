/*
 * zb_manu_tuya.h
 *
 * Tuya vendor definitions and EF00 datapoint decoder registration.
 *
 * Tuya's TS0601 devices expose no standard measurement clusters at all. Every
 * reading, setting and command travels as a "datapoint" (DP) record on the
 * manufacturer cluster 0xEF00, which is a thin wrapper around the serial
 * protocol Tuya's own MCUs speak. A TS0601 temperature/humidity sensor
 * therefore advertises 0xEF00 (and often 0xED00 / 0xE000, which carry nothing
 * we need) but neither 0x0402 nor 0x0405.
 *
 * The decoder in zb_manu_tuya.c normalises the DP records into standard
 * attribute reports (see zb_core_zcl_inject_report()), so the temperature,
 * humidity and battery functions downstream never special-case Tuya. The DP
 * numbering is per product, not per cluster, so the map itself lives in the
 * device schema's quirk tables (zb_device_schema_tuya_dp_find()), keyed by
 * ManufacturerName/ModelIdentifier.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_MANU_TUYA_H_
#define ZB_MANU_TUYA_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"

/* Tuya's MCU-protocol cluster. Unlike the Lumi 0xFCC0 cluster this one is NOT
 * manufacturer-specific on the wire: frames arrive as ordinary cluster-specific
 * commands with no manufacturer code, which is why the decoder registers as a
 * ZCL plugin rather than through the zb_zcl_manu dispatch table. */
#define ZCL_CLUSTER_ID_MANU_TUYA                            0xEF00

/* Cluster-specific commands, device -> hub, that carry DP records. Tuya uses
 * several interchangeably depending on whether the report was solicited. */
#define ZCL_CMD_TUYA_DATA_RESPONSE                          0x01
#define ZCL_CMD_TUYA_DATA_REPORT                            0x02
#define ZCL_CMD_TUYA_DATA_REPORT_ALT                        0x06

/* Device asks the gateway for the local time. Harmless to ignore; the readings
 * keep coming, only the unit's own clock display is left unset. */
#define ZCL_CMD_TUYA_MCU_SYNC_TIME                          0x24

/* DP record value encodings (the "type" byte of a record). */
#define ZB_TUYA_DP_TYPE_RAW                                 0x00
#define ZB_TUYA_DP_TYPE_BOOL                                0x01
#define ZB_TUYA_DP_TYPE_VALUE                               0x02  /* 4-byte big-endian signed */
#define ZB_TUYA_DP_TYPE_STRING                              0x03
#define ZB_TUYA_DP_TYPE_ENUM                                0x04
#define ZB_TUYA_DP_TYPE_BITMAP                              0x05

/**
 * @brief Register the Tuya 0xEF00 datapoint decoder.
 *
 * Called from zb_manu_register_all(). Installs a ZCL plugin for cluster
 * 0xEF00; the decoder then runs for every cluster-specific frame on it.
 */
zb_status_t zb_manu_tuya_register(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_MANU_TUYA_H_ */
