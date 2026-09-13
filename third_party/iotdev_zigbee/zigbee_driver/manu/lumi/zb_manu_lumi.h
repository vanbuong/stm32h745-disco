/*
 * zb_manu_lumi.h
 *
 * Lumi / Xiaomi / Aqara (manufacturer code 0x115F) vendor definitions and
 * frame decoder registration.
 *
 * Everything Lumi-specific lives under manu/lumi/: the identifiers below, and
 * the raw-payload decoder in zb_manu_lumi.c. The generic ZCL layer knows
 * nothing about this vendor - the decoder normalises what it can into standard
 * attribute reports (see zb_core_zcl_inject_report()), so the device functions
 * downstream never special-case Aqara.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_MANU_LUMI_H_
#define ZB_MANU_LUMI_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"

/** Xiaomi / Aqara / Lumi manufacturer code. */
#define ZB_MANUFACTURER_CODE_LUMI                           0x115F

/* Lumi/Aqara "opple" cluster. Newer Aqara devices (the P1 motion sensor and
 * friends) carry almost everything here - the packed TLV heartbeat (battery),
 * device configuration, and on some models the readings themselves - instead of
 * on the standard clusters. Reads/writes/reports on it require the Lumi
 * manufacturer code. */
#define ZCL_CLUSTER_ID_MANU_LUMI                            0xFCC0

/* Lumi 0xFCC0 attributes we act on.
 *
 * 0x0112 (uint32) is what the motion sensor P1 (lumi.motion.ac02) sends instead
 * of a standard Occupancy report. It is a single number, NOT a packed record:
 * the illuminance in lux, biased by +65536. The sensor emits it only when it
 * detects motion, so the arrival of the report IS the motion event; there is no
 * "motion cleared" report and the host must time occupancy out itself.
 *
 * Sample "23 00 01 00" = 0x00010023 = 65571 -> 65571 - 65536 = 35 lux. The
 * always-1 third byte is just bit 16 of the bias, not a status flag.
 *
 * Matches zigbee-herdsman-converters' lumi_occupancy_illuminance:
 * https://github.com/Koenkk/zigbee-herdsman-converters/blob/master/src/lib/lumi.ts
 */
#define ATTRID_LUMI_ILLUMINANCE_MOTION                      0x0112
#define ZB_LUMI_ILLUMINANCE_BIAS                            65536u
/* The P1 emits absurd values in the dark; upstream treats anything above this
 * as 0 lux. See https://github.com/Koenkk/zigbee2mqtt/issues/12596 */
#define ZB_LUMI_ILLUMINANCE_MAX_VALID                       130536u

/* Seconds of quiet after which motion should be considered over: the device's
 * own configured value, writable (uint8, seconds). 30 s is the P1 default.
 *
 * Careful - the same quantity reaches us two different ways, and only one of
 * them is an attribute id:
 *   * 0x0102 (258) is the real 0xFCC0 attribute. This is what you read and
 *     write.
 *   * TLV tag 0x69 (105) is the same value carried inside the 0x00F7 heartbeat
 *     blob (ZB_LUMI_TAG_DETECTION_INTERVAL, private to zb_manu_lumi.c).
 * Anything above 255 can only be an attribute id; a value <= 255 seen inside
 * the F7 blob is a TLV tag. */
#define ATTRID_LUMI_DETECTION_INTERVAL                      0x0102
#define ZB_LUMI_DETECTION_INTERVAL_DEFAULT_S                30u

/* PIR sensitivity, writable (uint8). Discrete steps only. */
#define ATTRID_LUMI_MOTION_SENSITIVITY                      0x010C
#define ZB_LUMI_MOTION_SENSITIVITY_LOW                      1u
#define ZB_LUMI_MOTION_SENSITIVITY_MEDIUM                   2u
#define ZB_LUMI_MOTION_SENSITIVITY_HIGH                     3u

/**
 * @brief Register the Lumi/Aqara manufacturer-specific frame handlers.
 *
 * Called from zb_manu_register_all(); no need to call it directly.
 *
 * @return ZB_SUCCESS when every row was accepted, otherwise the first failure.
 */
zb_status_t zb_manu_lumi_register(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_MANU_LUMI_H_ */
