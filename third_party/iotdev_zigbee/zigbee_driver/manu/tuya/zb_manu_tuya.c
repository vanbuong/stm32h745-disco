/*
 * zb_manu_tuya.c
 *
 * Tuya 0xEF00 datapoint decoder: turns DP records into standard attribute
 * reports so the ordinary device functions can consume them.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#include "manu/tuya/zb_manu_tuya.h"

#include "common/zb_common.h"
#include "af/zb_af.h"
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "device/zb_device_schema.h"
#include "zcl/zb_zcl.h"

#define TAG "ZB_MANU_TUYA"

/* seq(2) then one or more records of dp(1) type(1) len(2, big-endian) data(len). */
#define ZB_TUYA_FRAME_SEQ_LEN       2
#define ZB_TUYA_DP_HEADER_LEN       4

static zb_status_t zcl_tuya_handle_incoming(s_zb_zcl_incoming_msg_t *msg);

/**
 * @brief Decode a DP record's payload into a scalar.
 *
 * Tuya encodes multi-byte values BIG-endian, the opposite of ZCL. Only the
 * numeric encodings are handled; raw and string records carry no scalar and are
 * left to the caller to skip.
 *
 * @return true when @p out was written.
 */
static bool
tuya_dp_value(uint8_t dp_type, const uint8_t *data, uint16_t len, int32_t *out)
{
    switch (dp_type)
    {
        case ZB_TUYA_DP_TYPE_VALUE:
            if (len != 4)
            {
                return false;
            }
            *out = (int32_t)(((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
                             ((uint32_t)data[2] << 8)  | (uint32_t)data[3]);
            return true;

        case ZB_TUYA_DP_TYPE_BOOL:
        case ZB_TUYA_DP_TYPE_ENUM:
            if (len != 1)
            {
                return false;
            }
            *out = (int32_t)data[0];
            return true;

        case ZB_TUYA_DP_TYPE_BITMAP:
            if (len == 1)
            {
                *out = (int32_t)data[0];
            }
            else if (len == 2)
            {
                *out = (int32_t)(((uint32_t)data[0] << 8) | data[1]);
            }
            else if (len == 4)
            {
                *out = (int32_t)(((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
                                 ((uint32_t)data[2] << 8)  | (uint32_t)data[3]);
            }
            else
            {
                return false;
            }
            return true;

        default:
            return false;   /* raw / string: nothing scalar to map */
    }
}

/**
 * @brief Re-emit one mapped datapoint as a standard attribute report.
 *
 * The value is rescaled from the DP's own units into the attribute's units (a
 * Tuya temperature DP counts tenths of a degree, the ZCL attribute hundredths)
 * and written little-endian, which is what the ZCL attribute parsers expect.
 */
static void
tuya_inject_dp(const s_zb_zcl_incoming_msg_t *msg, const s_zb_tuya_dp_map_t *map, int32_t value)
{
    int32_t scaled = value;

    if (map->divisor != 0)
    {
        scaled = (value * map->multiplier) / map->divisor;
    }

    uint8_t buf[4] = {0};
    switch (map->data_type)
    {
        case ZCL_DATATYPE_UINT8:
        case ZCL_DATATYPE_INT8:
            buf[0] = (uint8_t)scaled;
            break;

        case ZCL_DATATYPE_UINT16:
        case ZCL_DATATYPE_INT16:
            buf[0] = LO_UINT16((uint16_t)scaled);
            buf[1] = HI_UINT16((uint16_t)scaled);
            break;

        case ZCL_DATATYPE_UINT32:
        case ZCL_DATATYPE_INT32:
            buf[0] = (uint8_t)(scaled & 0xFF);
            buf[1] = (uint8_t)((scaled >> 8) & 0xFF);
            buf[2] = (uint8_t)((scaled >> 16) & 0xFF);
            buf[3] = (uint8_t)((scaled >> 24) & 0xFF);
            break;

        default:
            ZB_LOGW(TAG, "DP %u: unsupported target data type 0x%02x", map->dp_id, map->data_type);
            return;
    }

    ZB_LOGI(TAG, "DP %u = %ld -> cluster 0x%04x attr 0x%04x = %ld",
            map->dp_id, (long)value, map->cluster_id, map->attr_id, (long)scaled);

    zb_core_zcl_inject_report(msg, map->cluster_id, map->attr_id, map->data_type, buf);
}

/**
 * @brief ZCL plugin entry point for cluster 0xEF00.
 *
 * Frames arrive as ordinary cluster-specific commands, so @p msg->data points
 * at the raw payload after the ZCL header. Runs on the zb_core driver task, the
 * same context as the standard ZCL path.
 */
static zb_status_t
zcl_tuya_handle_incoming(s_zb_zcl_incoming_msg_t *msg)
{
    if (msg == NULL || msg->msg == NULL)
    {
        return ZB_FAILURE;
    }

    switch (msg->hdr.command_id)
    {
        case ZCL_CMD_TUYA_DATA_RESPONSE:
        case ZCL_CMD_TUYA_DATA_REPORT:
        case ZCL_CMD_TUYA_DATA_REPORT_ALT:
            break;

        case ZCL_CMD_TUYA_MCU_SYNC_TIME:
            /* The device is asking us to set its clock. Ignoring it costs only
             * the unit's own time display; the readings are unaffected. */
            ZB_LOGD(TAG, "0x%04x - MCU time sync request ignored", msg->msg->src_addr.short_addr);
            return ZB_SUCCESS;

        default:
            ZB_LOGD(TAG, "0x%04x - unhandled Tuya command 0x%02x",
                    msg->msg->src_addr.short_addr, msg->hdr.command_id);
            return ZB_SUCCESS;
    }

    /* The DP map is per product, so the device has to be known (and its Basic
     * cluster read) before a record can be interpreted. */
    s_zb_device_t *device = zb_device_manager_find_by_short_addr(msg->msg->src_addr.short_addr);
    if (device == NULL)
    {
        ZB_LOGW(TAG, "Tuya report from unknown device 0x%04x", msg->msg->src_addr.short_addr);
        return ZB_SUCCESS;
    }

    const uint8_t *p = msg->data;
    uint16_t remaining = msg->data_len;

    if (p == NULL || remaining < ZB_TUYA_FRAME_SEQ_LEN)
    {
        ZB_LOGW(TAG, "0x%llx - Tuya frame too short (%u bytes)", device->ieee_addr, (unsigned)remaining);
        return ZB_SUCCESS;
    }
    p += ZB_TUYA_FRAME_SEQ_LEN;         /* frame sequence number, not needed */
    remaining -= ZB_TUYA_FRAME_SEQ_LEN;

    /* One frame can carry several records back to back. */
    while (remaining >= ZB_TUYA_DP_HEADER_LEN)
    {
        uint8_t  dp_id   = p[0];
        uint8_t  dp_type = p[1];
        uint16_t dp_len  = (uint16_t)(((uint16_t)p[2] << 8) | p[3]);

        p += ZB_TUYA_DP_HEADER_LEN;
        remaining -= ZB_TUYA_DP_HEADER_LEN;

        if (dp_len > remaining)
        {
            ZB_LOGW(TAG, "0x%llx - DP %u claims %u bytes, only %u left",
                    device->ieee_addr, dp_id, (unsigned)dp_len, (unsigned)remaining);
            break;
        }

        const s_zb_tuya_dp_map_t *map =
            zb_device_schema_tuya_dp_find(device->manufacturer, device->model, dp_id);
        if (map != NULL)
        {
            int32_t value = 0;
            if (tuya_dp_value(dp_type, p, dp_len, &value))
            {
                tuya_inject_dp(msg, map, value);
            }
            else
            {
                ZB_LOGW(TAG, "0x%llx - DP %u: cannot decode type 0x%02x len %u",
                        device->ieee_addr, dp_id, dp_type, (unsigned)dp_len);
            }
        }
        else
        {
            /* Unmapped datapoints are the norm - these devices report settings,
             * alarm thresholds and report intervals alongside the readings. */
            ZB_LOGD(TAG, "0x%llx - DP %u (type 0x%02x, %u bytes) not mapped",
                    device->ieee_addr, dp_id, dp_type, (unsigned)dp_len);
        }

        p += dp_len;
        remaining -= dp_len;
    }

    return ZB_SUCCESS;
}

zb_status_t
zb_manu_tuya_register(void)
{
    return zb_zcl_register_plugin(ZCL_CLUSTER_ID_MANU_TUYA,
                                  ZCL_CLUSTER_ID_MANU_TUYA,
                                  zcl_tuya_handle_incoming);
}
