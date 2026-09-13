/*
 * zb_manu_lumi.c
 *
 * Lumi / Xiaomi / Aqara raw-payload decoder.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#include "manu/lumi/zb_manu_lumi.h"

#include "common/zb_common.h"
#include "core/zb_core.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_ms.h"
#include "zcl/zb_zcl_general.h"
#include "zcl/zb_zcl_manu.h"

#define TAG "ZB_MANU_LUMI"

/* Lumi/Aqara proprietary attributes carrying the packed TLV blob. */
#define ZB_LUMI_ATTR_STRUCT             0xFF01
#define ZB_LUMI_ATTR_STRUCT_ALT         0xFF02
#define ZB_LUMI_ATTR_RAW                0x00F7

/* TLV tags (the subset we map onto standard attributes). */
#define ZB_LUMI_TAG_BATTERY_MV          0x01   /* uint16, millivolts */
#define ZB_LUMI_TAG_DEV_TEMP            0x03   /* int8, degrees C    */
#define ZB_LUMI_TAG_RSSI                0x05   /* uint16             */
/* Same quantity as attribute ATTRID_LUMI_DETECTION_INTERVAL, delivered inside
 * the heartbeat blob instead. See the note in zb_manu_lumi.h. */
#define ZB_LUMI_TAG_DETECTION_INTERVAL  0x69   /* uint8, seconds     */

/**
 * @brief Walk the Lumi/Aqara packed TLV blob.
 *
 * Layout: repeating { tag(1), zcl_data_type(1), value(length by type) }.
 * Confirmed against a real water-leak dump: "01 21 9F 0B" = tag 0x01, type 0x21
 * (uint16), value 2975 -> 2.975 V battery.
 *
 * Runs in two modes. With @p decode false it only validates, which is how the
 * caller decides where the TLV actually starts (Aqara sometimes prefixes an
 * entry count). With @p decode true it also injects the values it recognises.
 *
 * @return true if the walk consumed EXACTLY @p tlv_len bytes with every entry
 *         well-formed - i.e. this really is a TLV blob starting here.
 */
static bool
zb_manu_lumi_walk_tlv(s_zb_zcl_incoming_msg_t *msg, const uint8_t *tlv, uint16_t tlv_len, bool decode)
{
    const uint8_t *p = tlv;
    const uint8_t *end = tlv + tlv_len;

    if (tlv_len == 0)
    {
        return false;
    }

    while (p < end)
    {
        uint8_t tag;
        uint8_t type;
        uint16_t len;

        if ((end - p) < 2)
        {
            return false;   /* trailing garbage: not a clean TLV */
        }
        tag = *p++;
        type = *p++;

        if ((type == ZCL_DATATYPE_CHAR_STR) || (type == ZCL_DATATYPE_OCTET_STR))
        {
            if ((end - p) < 1)
                return false;
            len = (uint16_t)(*p + 1);
        }
        else
        {
            len = zb_zcl_get_data_type_length(type);
            if (len == 0)
            {
                if (decode)
                    ZB_LOGW(TAG, "Lumi TLV: tag 0x%02x has undecodable type 0x%02x", tag, type);
                return false;   /* length unknown - the rest cannot be walked */
            }
        }

        if (len > (uint16_t)(end - p))
        {
            if (decode)
                ZB_LOGW(TAG, "Lumi TLV: tag 0x%02x truncated (need %u, have %d)", tag, len, (int)(end - p));
            return false;
        }

        if (decode)
        {
            switch (tag)
            {
                case ZB_LUMI_TAG_BATTERY_MV:
                    if (len == 2)
                    {
                        uint16_t mv = BUILD_UINT16(p[0], p[1]);
                        /* ZCL BatteryVoltage is uint8 in 100 mV units. */
                        uint8_t battery_voltage = (uint8_t)(mv / 100u);
                        ZB_LOGI(TAG, "Lumi TLV: battery %u mV", mv);
                        zb_core_zcl_inject_report(msg, ZCL_CLUSTER_ID_GENERAL_POWER_CONFIG,
                            ATTRID_POWER_CONFIG_BATTERY_VOLTAGE, ZCL_DATATYPE_UINT8, &battery_voltage);
                    }
                    break;
                case ZB_LUMI_TAG_DEV_TEMP:
                    if (len == 1)
                    {
                        ZB_LOGI(TAG, "Lumi TLV: device temperature %d C", (int8_t)p[0]);
                    }
                    break;
                case ZB_LUMI_TAG_DETECTION_INTERVAL:
                    /* Same value as attribute 0x0102, just delivered inside the
                     * heartbeat instead. Injected as the standard
                     * PIROccupiedToUnoccupiedDelay so the occupancy function
                     * sizes its motion timeout from it. */
                    if (len == 1 && p[0] != 0)
                    {
                        uint16_t delay_s = p[0];
                        uint8_t delay_le[2] = { LO_UINT16(delay_s), HI_UINT16(delay_s) };
                        ZB_LOGI(TAG, "Lumi TLV: detection interval %u s", p[0]);
                        zb_core_zcl_inject_report(msg, ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING,
                            ATTRID_OCCUPANCY_SENSING_PIR_OCCUPIED_TO_UNOCCUPIED_DELAY,
                            ZCL_DATATYPE_UINT16, delay_le);
                    }
                    break;
                default:
                    ZB_LOGD(TAG, "Lumi TLV: unhandled tag 0x%02x type 0x%02x len %u", tag, type, len);
                    break;
            }
        }

        p += len;
    }

    return (p == end);
}

/**
 * @brief Decode a Lumi blob whose exact framing is not fixed across devices.
 *
 * Two layouts are seen in the wild for the same 0xFF01 attribute:
 *   a) the blob is the TLV list itself, and
 *   b) the blob starts with an entry-count byte, then the TLV list
 *      (this is what zigbee-herdsman's readMiStruct consumes).
 * Rather than guess, validate both and use whichever walks cleanly to the end.
 */
static void
zb_manu_lumi_decode_blob(s_zb_zcl_incoming_msg_t *msg, const uint8_t *blob, uint16_t blob_len)
{
    ZB_LOGI(TAG, "Lumi blob (%u bytes):", blob_len);
    ZB_LOG_BUFFER_HEX(TAG, blob, blob_len);

    if (zb_manu_lumi_walk_tlv(msg, blob, blob_len, false))
    {
        zb_manu_lumi_walk_tlv(msg, blob, blob_len, true);
        return;
    }

    if (blob_len >= 1 && zb_manu_lumi_walk_tlv(msg, blob + 1, (uint16_t)(blob_len - 1), false))
    {
        ZB_LOGI(TAG, "Lumi blob: leading byte 0x%02x is an entry count, skipping it", blob[0]);
        zb_manu_lumi_walk_tlv(msg, blob + 1, (uint16_t)(blob_len - 1), true);
        return;
    }

    /* Neither framing validates. Decode the more likely one anyway so partial
     * values (the battery tag is almost always first) are still recovered, and
     * the hex above shows what actually arrived. */
    ZB_LOGW(TAG, "Lumi blob: unrecognised framing, decoding best-effort");
    zb_manu_lumi_walk_tlv(msg, blob, blob_len, true);
}

/**
 * @brief Decode a scalar attribute on the Lumi 0xFCC0 ("opple") cluster.
 *
 * Newer Aqara devices put their readings on this manufacturer cluster rather
 * than on the standard ones. Where an attribute has a standard equivalent it is
 * injected as an ordinary report, so the normal function pipeline handles it
 * and nothing downstream needs to know about Aqara.
 *
 * IMPORTANT: which attribute id carries what is per-model and undocumented.
 * Only mappings confirmed against a real device should be added here; anything
 * else is logged with its value by the caller so it can be identified first.
 *
 * @return true if the attribute was recognised (decoded or deliberately
 *         ignored), false to let the caller log it as unknown.
 */
static bool
zb_manu_lumi_decode_opple_attr(s_zb_zcl_incoming_msg_t *msg, uint16_t attr_id,
    uint8_t data_type, const uint8_t *value, uint16_t len)
{
    switch (attr_id)
    {
        case ATTRID_LUMI_ILLUMINANCE_MOTION:
        {
            /* Motion sensor P1 (lumi.motion.ac02): bias-encoded lux, sent only
             * on detection (see ATTRID_LUMI_ILLUMINANCE_MOTION in
             * zb_manu_lumi.h). Split into the two standard reports it stands
             * for, so the ordinary occupancy / illuminance functions handle it. */
            if (len < 4)
            {
                ZB_LOGW(TAG, "Lumi: attr 0x0112 too short (%u bytes)", len);
                return true;
            }

            uint32_t raw = BUILD_UINT32(value[0], value[1], value[2], value[3]);
            uint32_t lux = (raw > ZB_LUMI_ILLUMINANCE_MAX_VALID || raw < ZB_LUMI_ILLUMINANCE_BIAS)
                               ? 0u
                               : (raw - ZB_LUMI_ILLUMINANCE_BIAS);
            ZB_LOGI(TAG, "Lumi: motion, illuminance raw=%lu -> %lu lux",
                (unsigned long)raw, (unsigned long)lux);

            /* The report's existence is the motion event - the value says
             * nothing about occupancy. Cleared later by a host-side timeout. */
            uint8_t occupied = 1;
            zb_core_zcl_inject_report(msg, ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING,
                ATTRID_OCCUPANCY_SENSING_OCCUPANCY, ZCL_DATATYPE_BITMAP8, &occupied);

            /* The standard MeasuredValue is log-encoded, not raw lux - the
             * illuminance function inverts exactly this, so encode here rather
             * than special-casing downstream. 0 lux has no logarithm; 0 is the
             * ZCL "too dark to measure" value. */
            uint16_t measured = 0;
            if (lux > 0)
            {
                float encoded = 10000.0f * log10f((float)lux) + 1.0f;
                measured = (encoded >= 65534.0f) ? 65534u : (uint16_t)encoded;
            }
            uint8_t measured_le[2] = { LO_UINT16(measured), HI_UINT16(measured) };
            zb_core_zcl_inject_report(msg, ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT,
                ATTRID_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE, ZCL_DATATYPE_UINT16, measured_le);
            return true;
        }

        case ATTRID_LUMI_DETECTION_INTERVAL:
        {
            /* How long the device itself considers a detection to last. Maps
             * exactly onto the standard PIROccupiedToUnoccupiedDelay, so inject
             * it there: the occupancy function uses that attribute to size its
             * motion timeout, and never has to know this is an Aqara. */
            if (len < 1 || value[0] == 0)
            {
                return true;
            }
            ZB_LOGI(TAG, "Lumi: detection interval = %u s", value[0]);

            uint16_t delay_s = value[0];
            uint8_t delay_le[2] = { LO_UINT16(delay_s), HI_UINT16(delay_s) };
            zb_core_zcl_inject_report(msg, ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING,
                ATTRID_OCCUPANCY_SENSING_PIR_OCCUPIED_TO_UNOCCUPIED_DELAY,
                ZCL_DATATYPE_UINT16, delay_le);
            return true;
        }

        case ATTRID_LUMI_MOTION_SENSITIVITY:
            ZB_LOGI(TAG, "Lumi: motion sensitivity = %u", (len >= 1) ? value[0] : 0u);
            return true;

        default:
            break;
    }

    (void)data_type;
    return false;
}

/**
 * @brief Manufacturer-specific handler for Lumi/Aqara (0x115F).
 *
 * Aqara devices report a proprietary TLV blob on Basic-cluster attributes
 * 0xFF01/0xFF02/0xF7 (battery voltage, device temperature, RSSI, ...) and use
 * ZCL data types the generic attribute-record walker has no length rule for.
 * Handing such a frame to the generic parser used to desynchronise the walk and
 * read past the end of the AF payload, crashing the gateway.
 *
 * Receives the RAW payload (nothing parsed), walks the report records itself,
 * and injects any value that maps onto a standard attribute back into the
 * normal pipeline - which is what gets it past the interview observer and into
 * the report cache while the device is still being commissioned.
 */
static zb_status_t
zb_manu_lumi_handler(s_zb_zcl_incoming_msg_t *msg)
{
    const uint8_t *p = msg->data;
    const uint8_t *end = msg->data + msg->data_len;

    ZB_LOGI(TAG, "ZCL Lumi/Aqara MSPW cmd 0x%02x from 0x%04x, ep=%d, cluster=0x%04x, %u raw bytes",
        msg->hdr.command_id, msg->msg->src_addr.short_addr,
        msg->msg->src_endpoint, msg->msg->cluster_id, msg->data_len);

    if (msg->hdr.command_id != ZCL_CMD_REPORT && msg->hdr.command_id != ZCL_CMD_READ_RSP)
    {
        return ZB_SUCCESS;  /* consumed; nothing to decode */
    }

    /* Walk the report records: attr_id(2) | data_type(1) | value. Bounds are
     * checked here because this payload is exactly the one the generic parser
     * cannot be trusted with. */
    while ((end - p) >= 3)
    {
        uint16_t attr_id = BUILD_UINT16(p[0], p[1]);
        uint8_t data_type;
        uint16_t len;
        const uint8_t *blob = NULL;   /* string payload, past the length byte */
        uint16_t blob_len = 0;

        p += 2;
        if (msg->hdr.command_id == ZCL_CMD_READ_RSP)
        {
            uint8_t status = *p++;
            if (status != ZCL_STATUS_SUCCESS)
            {
                continue;   /* failed record: no type, no value */
            }
            if ((end - p) < 1)
                break;
        }
        data_type = *p++;

        if ((data_type == ZCL_DATATYPE_CHAR_STR) || (data_type == ZCL_DATATYPE_OCTET_STR))
        {
            uint16_t avail;
            uint8_t declared;

            if ((end - p) < 1)
                break;
            declared = *p;
            avail = (uint16_t)(end - p);        /* includes the length byte itself */
            blob = p + 1;
            blob_len = declared;
            len = (uint16_t)(declared + 1);     /* standard: length byte + content */

            /* Aqara quirk: on some frames the length byte counts ITSELF, so the
             * declared value overruns the record by exactly one and the standard
             * reading reports a bogus truncation. Detect it precisely - the
             * standard reading does not fit but the self-inclusive one does -
             * and re-read rather than dropping the whole blob. Deliberately
             * scoped to this manufacturer handler: the generic ZCL parser must
             * stay strict for every other device. */
            if (len > avail && declared <= avail && declared > 0)
            {
                ZB_LOGI(TAG, "Lumi: attr 0x%04x length byte %u is self-inclusive (avail %u)",
                    attr_id, declared, avail);
                blob_len = (uint16_t)(declared - 1);
                len = declared;
            }
        }
        else
        {
            len = zb_zcl_get_data_type_length(data_type);
            if (len == 0)
            {
                ZB_LOGW(TAG, "Lumi: attr 0x%04x has undecodable type 0x%02x, stopping", attr_id, data_type);
                break;
            }
        }

        if (len > (uint16_t)(end - p))
        {
            ZB_LOGW(TAG, "Lumi: attr 0x%04x truncated (need %u, have %d), stopping",
                attr_id, len, (int)(end - p));
            break;
        }

        if ((attr_id == ZB_LUMI_ATTR_STRUCT || attr_id == ZB_LUMI_ATTR_STRUCT_ALT ||
             attr_id == ZB_LUMI_ATTR_RAW) &&
            ((data_type == ZCL_DATATYPE_CHAR_STR) || (data_type == ZCL_DATATYPE_OCTET_STR)))
        {
            zb_manu_lumi_decode_blob(msg, blob, blob_len);
        }
        else if (msg->msg->cluster_id != ZCL_CLUSTER_ID_MANU_LUMI ||
                 !zb_manu_lumi_decode_opple_attr(msg, attr_id, data_type, p, len))
        {
            /* Not something we map. Dump the value too: which 0xFCC0 attribute
             * a given Aqara model uses for its reading is model-specific and
             * not documented, so these lines are how a new one gets identified
             * (watch the log while triggering the sensor). */
            ZB_LOGI(TAG, "Lumi: attr 0x%04x type 0x%02x len %u (not decoded)", attr_id, data_type, len);
            ZB_LOG_BUFFER_HEX(TAG, p, len);
        }

        p += len;
    }

    return ZB_SUCCESS;
}

/* Rows for this vendor, most specific first. */
static const s_zb_zcl_manu_handler_t s_lumi_handlers[] = {
    {
        .manuf_code = ZB_MANUFACTURER_CODE_LUMI,
        .cluster_id = ZCL_CLUSTER_ID_GENERAL_BASIC,
        .command_id = ZB_ZCL_MANU_ANY_CMD,
        .handler    = zb_manu_lumi_handler,
        .name       = "lumi-basic",
    },
    {
        /* Newer Aqara devices (motion sensor P1, lumi.motion.ac02) moved the
         * proprietary heartbeat off Basic and onto 0xFCC0. The payload framing
         * is identical - the same 0x00F7 TLV blob carrying battery voltage -
         * so the same decoder handles it. Without this row the frame reaches
         * the generic parser, which cannot walk it. */
        .manuf_code = ZB_MANUFACTURER_CODE_LUMI,
        .cluster_id = ZCL_CLUSTER_ID_MANU_LUMI,
        .command_id = ZB_ZCL_MANU_ANY_CMD,
        .handler    = zb_manu_lumi_handler,
        .name       = "lumi-opple",
    },
};

zb_status_t
zb_manu_lumi_register(void)
{
    zb_status_t status = ZB_SUCCESS;

    for (size_t i = 0; i < sizeof(s_lumi_handlers) / sizeof(s_lumi_handlers[0]); i++)
    {
        zb_status_t row_status = zb_zcl_manu_register_handler(&s_lumi_handlers[i]);
        if (row_status != ZB_SUCCESS && status == ZB_SUCCESS)
        {
            status = row_status;
        }
    }
    return status;
}
