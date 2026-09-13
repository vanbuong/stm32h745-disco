/*
 * zb_zcl_manu.h
 *
 * Manufacturer-specific ZCL profile-wide (foundation) command dispatch.
 *
 * Some vendors send profile-wide commands (Report Attributes, Read Attributes
 * Response, ...) whose payload does NOT follow the ZCL attribute-record layout.
 * The best known example is Lumi/Aqara (manufacturer code 0x115F), which
 * reports a proprietary TLV blob on Basic cluster attribute 0xFF01/0xF7, and
 * occasionally uses data types (STRUCT/ARRAY) that the generic parser has no
 * length rule for. Feeding such a frame to the generic parser makes it walk off
 * the end of the AF payload and crash the firmware.
 *
 * This module provides a small table of per-manufacturer handlers that is
 * consulted BEFORE the generic parser runs. A handler receives the raw,
 * unparsed ZCL payload and is free to decode it however the vendor requires.
 * If no handler matches, the common (generic) path runs exactly as before.
 *
 * This module is only the mechanism. The handlers themselves, and the vendor
 * identifiers they match on, live one per vendor under zigbee_driver/manu/ and
 * are registered by zb_manu_register_all() - see manu/zb_manu.h.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_ZCL_MANU_H_
#define ZB_ZCL_MANU_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "zcl/zb_zcl.h"

/** Wildcards for the match fields of a handler entry. */
#define ZB_ZCL_MANU_ANY_CLUSTER   0xFFFFu
#define ZB_ZCL_MANU_ANY_CMD       0xFFu

/* Manufacturer codes are not listed here - each vendor defines its own in
 * manu/<vendor>/zb_manu_<vendor>.h (ZB_MANUFACTURER_CODE_*), next to the
 * cluster and attribute ids that only mean anything alongside it. This module
 * is the dispatch mechanism and stays vendor agnostic. */

/**
 * @brief Manufacturer-specific profile-wide command handler.
 *
 * Called with the RAW frame: @p msg->data points at the first byte after the
 * ZCL header and @p msg->data_len is the number of payload bytes remaining.
 * @p msg->attr_cmd is always NULL - nothing has been parsed yet, which is the
 * whole point: the handler owns the decoding.
 *
 * The handler runs on the zb_core driver task (same context as the generic
 * ZCL path), so it must not issue a blocking ZNP SREQ.
 *
 * @param msg  Incoming ZCL message with an unparsed payload.
 * @return ZCL status to report back to the device: ZB_SUCCESS when the frame
 *         was consumed, or a ZCL_STATUS_* error code. Returning
 *         ZCL_STATUS_UNSUP_MANU_GENERAL_COMMAND makes the caller fall back to
 *         the common handler as if no entry had matched.
 */
typedef zb_status_t (*pfn_zcl_manu_cmd_handler_t)(s_zb_zcl_incoming_msg_t *msg);

/** One row of the manufacturer dispatch table. */
typedef struct s_zb_zcl_manu_handler
{
    uint16_t manuf_code;                /* Manufacturer code, exact match      */
    uint16_t cluster_id;                /* Cluster, or ZB_ZCL_MANU_ANY_CLUSTER */
    uint8_t command_id;                 /* ZCL cmd, or ZB_ZCL_MANU_ANY_CMD     */
    pfn_zcl_manu_cmd_handler_t handler; /* Decoder for this vendor frame       */
    const char *name;                   /* For logging, may be NULL            */
} s_zb_zcl_manu_handler_t;

/**
 * @brief Register a manufacturer-specific handler.
 *
 * Entries are matched in registration order; the first match wins, so register
 * the most specific entries first. Registering the same
 * (manuf_code, cluster_id, command_id) triple twice replaces the first entry.
 *
 * @param entry  Handler description. The struct is copied; @c name must point
 *               at storage that outlives the registration (a string literal).
 * @return ZB_SUCCESS, ZB_INVALID_PARAMETER, or ZB_MEM_ERROR when the table is
 *         full (see ZB_ZCL_MANU_MAX_HANDLERS).
 */
zb_status_t zb_zcl_manu_register_handler(const s_zb_zcl_manu_handler_t *entry);

/**
 * @brief Look up a handler for an incoming manufacturer-specific frame.
 *
 * @return Matching entry, or NULL when the common handler should be used.
 */
const s_zb_zcl_manu_handler_t *zb_zcl_manu_find_handler(
    uint16_t manuf_code, uint16_t cluster_id, uint8_t command_id);

/**
 * @brief Remove every registered handler (used by tests / re-init).
 */
void zb_zcl_manu_clear_handlers(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZCL_MANU_H_ */
