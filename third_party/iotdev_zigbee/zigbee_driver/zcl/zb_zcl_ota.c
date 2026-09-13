/*
 * zb_zcl_ota.c
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_ota.h"

static uint8_t g_zcl_ota_seq_num = 0;

int zb_zcl_ota_send_image_notify(
    uint8_t src_endpoint, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ota_image_notify_params_t *params)
{
    uint8_t buf[ZCL_OTA_PAYLOAD_MAX_LEN_IMAGE_NOTIFY];
    uint8_t *pbuf = buf;
    bool disable_default_rsp = true;

    *pbuf++ = params->payload_type;
    *pbuf++ = params->query_jitter;
    if (params->payload_type >= ZCL_OTA_IMAGE_NOTIFY_PAYLOAD_JITTER_MFG)
    {
        *pbuf++ = LO_UINT16(params->file_info.manufacturer_id);
        *pbuf++ = HI_UINT16(params->file_info.manufacturer_id);
    }
    if (params->payload_type >= ZCL_OTA_IMAGE_NOTIFY_PAYLOAD_JITTER_MFG_TYPE)
    {
        *pbuf++ = LO_UINT16(params->file_info.type);
        *pbuf++ = HI_UINT16(params->file_info.type);
    }
    if (params->payload_type >= ZCL_OTA_IMAGE_NOTIFY_PAYLOAD_JITTER_MFG_TYPE_VERSION)
    {
        *pbuf++ = BREAK_UINT32(params->file_info.version, 0);
        *pbuf++ = BREAK_UINT32(params->file_info.version, 1);
        *pbuf++ = BREAK_UINT32(params->file_info.version, 2);
        *pbuf++ = BREAK_UINT32(params->file_info.version, 3);
    }

    if (dst_addr->address_mode == AF_ADDRESS_16BIT)
    {
        // For unicast disable_default_rsp should not be set
        disable_default_rsp = false;
    }

    return zb_zcl_send_cmd(
                src_endpoint, dst_addr, ZCL_CLUSTER_ID_OTA,
                ZCL_OTA_COMMAND_OTA_UPGRADE_IMAGE_NOTIFY, true,
                ZCL_FRAME_SERVER_CLIENT_DIR, disable_default_rsp, 0,
                g_zcl_ota_seq_num++, (uint16_t)(pbuf - buf), buf);
}

int zb_zcl_ota_send_query_specific_file_rsp(
    uint8_t src_endpoint, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ota_query_image_rsp_params_t *params, uint8_t seq_num)
{
    uint8_t buf[ZCL_OTA_PAYLOAD_MAX_LEN_QUERY_SPECIFIC_FILE_RSP];
    uint8_t *pbuf = buf;

    *pbuf++ = params->status;
    if (params->status == ZCL_OTA_STATUS_SUCCESS)
    {
        *pbuf++ = LO_UINT16(params->file_info.manufacturer_id);
        *pbuf++ = HI_UINT16(params->file_info.manufacturer_id);
        *pbuf++ = LO_UINT16(params->file_info.type);
        *pbuf++ = HI_UINT16(params->file_info.type);
        *pbuf++ = BREAK_UINT32(params->file_info.version, 0);
        *pbuf++ = BREAK_UINT32(params->file_info.version, 1);
        *pbuf++ = BREAK_UINT32(params->file_info.version, 2);
        *pbuf++ = BREAK_UINT32(params->file_info.version, 3);
        *pbuf++ = BREAK_UINT32(params->image_size, 0);
        *pbuf++ = BREAK_UINT32(params->image_size, 1);
        *pbuf++ = BREAK_UINT32(params->image_size, 2);
        *pbuf++ = BREAK_UINT32(params->image_size, 3);
    }

    return zb_zcl_send_cmd(
                src_endpoint, dst_addr, ZCL_CLUSTER_ID_OTA,
                ZCL_OTA_COMMAND_OTA_UPGRADE_QUERY_DEVICE_SPECIFIC_FILE_RESPONSE, true,
                ZCL_FRAME_SERVER_CLIENT_DIR, true, 0,
                seq_num, (uint16_t)(pbuf - buf), buf);
}

int zb_zcl_ota_send_query_next_image_rsp(
    uint8_t src_endpoint, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ota_query_image_rsp_params_t *params, uint8_t seq_num)
{
    uint8_t buf[ZCL_OTA_PAYLOAD_MAX_LEN_QUERY_NEXT_IMAGE_RSP];
    uint8_t *pbuf = buf;

    *pbuf++ = params->status;
    if (params->status == ZCL_OTA_STATUS_SUCCESS)
    {
        *pbuf++ = LO_UINT16(params->file_info.manufacturer_id);
        *pbuf++ = HI_UINT16(params->file_info.manufacturer_id);
        *pbuf++ = LO_UINT16(params->file_info.type);
        *pbuf++ = HI_UINT16(params->file_info.type);
        *pbuf++ = BREAK_UINT32(params->file_info.version, 0);
        *pbuf++ = BREAK_UINT32(params->file_info.version, 1);
        *pbuf++ = BREAK_UINT32(params->file_info.version, 2);
        *pbuf++ = BREAK_UINT32(params->file_info.version, 3);
        *pbuf++ = BREAK_UINT32(params->image_size, 0);
        *pbuf++ = BREAK_UINT32(params->image_size, 1);
        *pbuf++ = BREAK_UINT32(params->image_size, 2);
        *pbuf++ = BREAK_UINT32(params->image_size, 3);
    }

    return zb_zcl_send_cmd(
                src_endpoint, dst_addr, ZCL_CLUSTER_ID_OTA,
                ZCL_OTA_COMMAND_OTA_UPGRADE_QUERY_NEXT_IMAGE_RESPONSE, true,
                ZCL_FRAME_SERVER_CLIENT_DIR, true, 0,
                seq_num, (uint16_t)(pbuf - buf), buf);
}

int zb_zcl_ota_send_image_block_rsp(
    uint8_t src_endpoint, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ota_image_block_rsp_params_t *params, uint8_t seq_num)
{
    uint8_t *buf;
    uint8_t *pbuf;
    uint8_t len;
    uint8_t status;

    if (params->status == ZCL_OTA_STATUS_SUCCESS)
    {
        len = ZCL_OTA_PAYLOAD_MIN_LEN_IMAGE_BLOCK_RSP + params->rsp.success.data_size;
    }
    else if (params->status == ZCL_OTA_STATUS_WAIT_FOR_DATA)
    {
        len = ZCL_OTA_PAYLOAD_MIN_LEN_IMAGE_BLOCK_WAIT;
    }
    else
    {
        len = 1;
    }

    buf = (uint8_t *)ZB_MEM_MALLOC(len);
    if (buf == NULL)
    {
        return ZB_MEM_ERROR;
    }

    pbuf = buf;
    *pbuf++ = params->status;
    if (params->status == ZCL_OTA_STATUS_SUCCESS)
    {
        *pbuf++ = LO_UINT16(params->rsp.success.file_info.manufacturer_id);
        *pbuf++ = HI_UINT16(params->rsp.success.file_info.manufacturer_id);
        *pbuf++ = LO_UINT16(params->rsp.success.file_info.type);
        *pbuf++ = HI_UINT16(params->rsp.success.file_info.type);
        *pbuf++ = BREAK_UINT32(params->rsp.success.file_info.version, 0);
        *pbuf++ = BREAK_UINT32(params->rsp.success.file_info.version, 1);
        *pbuf++ = BREAK_UINT32(params->rsp.success.file_info.version, 2);
        *pbuf++ = BREAK_UINT32(params->rsp.success.file_info.version, 3);
        *pbuf++ = BREAK_UINT32(params->rsp.success.file_offset, 0);
        *pbuf++ = BREAK_UINT32(params->rsp.success.file_offset, 1);
        *pbuf++ = BREAK_UINT32(params->rsp.success.file_offset, 2);
        *pbuf++ = BREAK_UINT32(params->rsp.success.file_offset, 3);
        *pbuf++ = params->rsp.success.data_size;
        memcpy(pbuf, params->rsp.success.data, params->rsp.success.data_size);
    }
    else if (params->status == ZCL_OTA_STATUS_WAIT_FOR_DATA)
    {
        *pbuf++ = BREAK_UINT32(params->rsp.wait.current_time, 0);
        *pbuf++ = BREAK_UINT32(params->rsp.wait.current_time, 1);
        *pbuf++ = BREAK_UINT32(params->rsp.wait.current_time, 2);
        *pbuf++ = BREAK_UINT32(params->rsp.wait.current_time, 3);
        *pbuf++ = BREAK_UINT32(params->rsp.wait.request_time, 0);
        *pbuf++ = BREAK_UINT32(params->rsp.wait.request_time, 1);
        *pbuf++ = BREAK_UINT32(params->rsp.wait.request_time, 2);
        *pbuf++ = BREAK_UINT32(params->rsp.wait.request_time, 3);
        *pbuf++ = LO_UINT16(params->rsp.wait.block_req_delay);
        *pbuf++ = HI_UINT16(params->rsp.wait.block_req_delay);
    }

    status = zb_zcl_send_cmd(
                src_endpoint, dst_addr, ZCL_CLUSTER_ID_OTA,
                ZCL_OTA_COMMAND_OTA_UPGRADE_IMAGE_BLOCK_RESPONSE, true,
                ZCL_FRAME_SERVER_CLIENT_DIR, true, 0,
                seq_num, len, buf);
    ZB_MEM_FREE(buf);
    return status;
}

int zb_zcl_ota_send_upgrade_end_rsp(
    uint8_t src_endpoint, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ota_upgrade_end_rsp_params_t *params, uint8_t seq_num)
{
    uint8_t buf[ZCL_OTA_PAYLOAD_MIN_LEN_UPGRADE_END_RSP];
    uint8_t *pbuf = buf;

    *pbuf++ = LO_UINT16(params->file_info.manufacturer_id);
    *pbuf++ = HI_UINT16(params->file_info.manufacturer_id);
    *pbuf++ = LO_UINT16(params->file_info.type);
    *pbuf++ = HI_UINT16(params->file_info.type);
    *pbuf++ = BREAK_UINT32(params->file_info.version, 0);
    *pbuf++ = BREAK_UINT32(params->file_info.version, 1);
    *pbuf++ = BREAK_UINT32(params->file_info.version, 2);
    *pbuf++ = BREAK_UINT32(params->file_info.version, 3);
    *pbuf++ = BREAK_UINT32(params->current_time, 0);
    *pbuf++ = BREAK_UINT32(params->current_time, 1);
    *pbuf++ = BREAK_UINT32(params->current_time, 2);
    *pbuf++ = BREAK_UINT32(params->current_time, 3);
    *pbuf++ = BREAK_UINT32(params->upgrade_time, 0);
    *pbuf++ = BREAK_UINT32(params->upgrade_time, 1);
    *pbuf++ = BREAK_UINT32(params->upgrade_time, 2);
    *pbuf++ = BREAK_UINT32(params->upgrade_time, 3);

    return zb_zcl_send_cmd(
                src_endpoint, dst_addr, ZCL_CLUSTER_ID_OTA,
                ZCL_OTA_COMMAND_OTA_UPGRADE_UPGRADE_END_RESPONSE, true,
                ZCL_FRAME_SERVER_CLIENT_DIR, true, 0,
                seq_num, ZCL_OTA_PAYLOAD_MIN_LEN_UPGRADE_END_RSP, buf);
}

