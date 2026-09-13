#include "ota/zb_ota_server.h"
#include "common/zb_common.h"
#include "af/zb_af.h"
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "zcl/zb_zcl_ota.h"
#include "zb_osal.h"
#include <dirent.h>

#define TAG "ZB_OTA"

#define ZB_OTA_SERVER_FILE_ENTRY_MAX            10
#define ZB_OTA_SERVER_WAIT_FOR_DATA_CUR_TIME    0 // Not support UTC time
#define ZB_OTA_SERVER_WAIT_FOR_DATA_REQ_TIME    1 // 1 seconds
#define ZB_OTA_SERVER_MIN_BLOCK_PERIOD_MS       50 // 50ms
#define ZB_OTA_SERVER_UPGRADE_DELAY_SECONDS     1 // 1 seconds
#define ZB_OTA_SERVER_EVENT_QUEUE_SIZE          8

typedef struct
{
    uint8_t state;
    uint8_t trans_id;
    uint64_t client_ieee;
    uint16_t client_nwk_addr;
    uint8_t client_ep;
    uint8_t last_percent;
    s_zb_ota_server_file_entry_t file_entry;
} s_zb_ota_server_context_t;

static s_zb_ota_server_file_entry_t g_ota_files[ZB_OTA_SERVER_FILE_ENTRY_MAX];
static uint8_t g_ota_file_count = 0;
static zb_os_queue_t g_ota_server_event_queue;

static s_zb_ota_server_context_t g_ota_server_context = {
    .state = ZB_OTA_UPGRADE_STATUS_NORMAL,
    .trans_id = 0,
    .client_ieee = 0,
    .client_nwk_addr = 0,
    .client_ep = 0,
    .file_entry = {
        .file_path = "",
        .file_info = {},
        .file_size = 0,
    },
};

static uint64_t
zb_ota_server_client_ieee_from_msg(s_zb_af_incoming_msg_t *msg)
{
    uint64_t ieee = 0;
    if (zb_device_manager_get_ieee_by_short_addr(msg->src_addr.short_addr, &ieee))
    {
        return ieee;
    }
    return 0;
}

static void
zb_ota_server_publish_event(e_zb_event_type_t type, uint8_t percent)
{
    s_zb_event_t evt = {0};
    evt.type = type;
    evt.ieee_addr = g_ota_server_context.client_ieee;
    if (type == ZB_EVENT_DEVICE_OTA_PROGRESS)
    {
        evt.ota_progress.percent = percent;
        evt.ota_progress.phase = ZB_OTA_PROGRESS_PHASE_DOWNLOAD;
        g_ota_server_context.last_percent = percent;
    }
    zb_core_publish_event(&evt);
}

static void
zb_ota_server_clear_session(bool publish_aborted)
{
    if (publish_aborted && g_ota_server_context.state == ZB_OTA_UPGRADE_STATUS_IN_PROGRESS)
    {
        zb_ota_server_publish_event(ZB_EVENT_DEVICE_OTA_ABORTED, 0);
    }
    g_ota_server_context.state = ZB_OTA_UPGRADE_STATUS_NORMAL;
    g_ota_server_context.client_ieee = 0;
    g_ota_server_context.client_nwk_addr = 0;
    g_ota_server_context.client_ep = 0;
    g_ota_server_context.last_percent = 0;
    memset(&g_ota_server_context.file_entry, 0, sizeof(g_ota_server_context.file_entry));
}

static uint8_t
zb_ota_server_progress_percent(uint32_t offset, uint32_t bytes_sent)
{
    if (g_ota_server_context.file_entry.file_size == 0)
    {
        return 0;
    }
    uint32_t done = offset + bytes_sent;
    if (done >= g_ota_server_context.file_entry.file_size)
    {
        return 100;
    }
    return (uint8_t)((done * 100U) / g_ota_server_context.file_entry.file_size);
}

static int
zb_ota_server_parse_image_block_request(uint8_t *data, uint32_t length, s_zb_zcl_ota_image_block_req_params_t *params)
{
    if (length < ZCL_OTA_PAYLOAD_MIN_LEN_IMAGE_BLOCK_REQ || length > ZCL_OTA_PAYLOAD_MAX_LEN_IMAGE_BLOCK_REQ)
    {
        return ZB_FAIL;
    }
    uint8_t *pdata = data;
    params->field_control = *pdata++;
    params->file_info.manufacturer_id = *(uint16_t*)pdata;
    pdata += 2;
    params->file_info.type = *(uint16_t*)pdata;
    pdata += 2;
    params->file_info.version = *(uint32_t*)pdata;
    pdata += 4;
    params->file_offset = *(uint32_t*)pdata;
    pdata += 4;
    params->max_data_size = *pdata++;
    if (params->field_control & ZCL_OTA_BLOCK_FC_NODES_IEEE_PRESENT)
    {
        params->node_addr = *(uint64_t*)pdata;
        pdata += 8;
    }
    if (params->field_control & ZCL_OTA_BLOCK_FC_REQ_DELAY_PRESENT)
    {
        params->block_req_delay = *(uint16_t*)pdata;
        pdata += 2;
    }
    return ZB_OK;
}

static int
zb_ota_server_build_entry_from_file(char *file_path, s_zb_ota_server_file_entry_t *entry)
{
    s_zb_zcl_ota_header_t ota_hdr;
    int file_size = ZB_GET_FILE_SIZE(file_path);
    if (ZB_FILE_READ(file_path, (uint8_t *)&ota_hdr, sizeof(ota_hdr), 0) == sizeof(ota_hdr))
    {
        if (ota_hdr.magic_number == ZB_OTA_HDR_MAGIC_NUMBER && file_size == ota_hdr.image_size)
        {
            entry->file_info = ota_hdr.file_info;
            entry->file_size = ota_hdr.image_size;
            strcpy(entry->file_path, file_path);
            return ZB_OK;
        }
    }
    return ZB_FAIL;
}

static void
zb_ota_server_build_ota_index(void)
{
    g_ota_file_count = 0;
    DIR *d = opendir(ZB_OTA_DIR);
    if (d == NULL)
    {
        ZB_LOGE(TAG, "Failed to open OTA directory: %s", ZB_OTA_DIR);
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL)
    {
        if (entry->d_type != DT_REG && entry->d_type != DT_UNKNOWN) continue;
        char full_path[ZB_OTA_FILE_NAME_MAX];
        strlcpy(full_path, ZB_OTA_DIR, sizeof(full_path));
        strlcat(full_path, "/", sizeof(full_path));
        strlcat(full_path, entry->d_name, sizeof(full_path));
        
        if (g_ota_file_count >= ZB_OTA_SERVER_FILE_ENTRY_MAX)
        {
            ZB_LOGW(TAG, "OTA index full, skipping %s", full_path);
            continue;
        }

        s_zb_ota_server_file_entry_t tmp;
        if (zb_ota_server_build_entry_from_file(full_path, &tmp) == ZB_OK)
        {
            g_ota_files[g_ota_file_count++] = tmp;
            ZB_LOGI(TAG, "Indexed: %s mfg=0x%X type=0x%X ver=0x%X size=%u",
                full_path, tmp.file_info.manufacturer_id, tmp.file_info.type, tmp.file_info.version, tmp.file_size);
        }
        else
        {
            ZB_LOGW(TAG, "Skip file: %s", full_path);
        }
    }
    closedir(d);

    ZB_LOGI(TAG, "Built %d OTA files", g_ota_file_count);
}

static const s_zb_ota_server_file_entry_t *
zb_ota_server_find_matching_ota(uint16_t manufacturer_code, uint16_t image_type, uint32_t file_version)
{
    for (uint8_t i = 0; i < g_ota_file_count; i++)
    {
        if (g_ota_files[i].file_info.manufacturer_id == manufacturer_code && g_ota_files[i].file_info.type == image_type && g_ota_files[i].file_info.version != file_version)
        {
            return &g_ota_files[i];
        }
    }
    return NULL;
}

static void
zb_ota_server_handle_query_next_image_request(s_zb_af_incoming_msg_t *msg)
{
    s_zb_zcl_ota_query_image_rsp_params_t rsp;
    s_zb_af_address_t dst_addr = {
        .address_mode = AF_ADDRESS_16BIT,
        .endpoint = msg->src_endpoint,
        .short_addr = msg->src_addr.short_addr,
    };
    s_zb_zcl_frame_header_t header;
    uint8_t *zcl_data = zb_zcl_parse_header(&header, msg->command.data);
    s_zb_zcl_ota_query_next_image_req_params_t *params = (s_zb_zcl_ota_query_next_image_req_params_t *)(zcl_data);
    ZB_LOGI(TAG, "Query Next Image Request: mfg=0x%X type=0x%X ver=0x%X",
        params->file_info.manufacturer_id, params->file_info.type, params->file_info.version);
    
    if (g_ota_server_context.state != ZB_OTA_UPGRADE_STATUS_NORMAL)
    {
        ZB_LOGW(TAG, "OTA server not in normal state, current state: %d", g_ota_server_context.state);
        rsp.status = ZCL_OTA_STATUS_NO_IMAGE_AVAILABLE;
        zb_zcl_ota_send_query_next_image_rsp(msg->dst_endpoint, &dst_addr, &rsp, header.trans_seq_num);
        return;
    }

    const s_zb_ota_server_file_entry_t *entry = zb_ota_server_find_matching_ota(params->file_info.manufacturer_id, params->file_info.type, params->file_info.version);
    if (entry == NULL)
    {
        ZB_LOGW(TAG, "No matching OTA file found");
        rsp.status = ZCL_OTA_STATUS_NO_IMAGE_AVAILABLE;
        zb_zcl_ota_send_query_next_image_rsp(msg->dst_endpoint, &dst_addr, &rsp, header.trans_seq_num);
        return;
    }
    strlcpy(g_ota_server_context.file_entry.file_path, entry->file_path, sizeof(g_ota_server_context.file_entry.file_path));
    g_ota_server_context.file_entry.file_info = entry->file_info;
    g_ota_server_context.file_entry.file_size = entry->file_size;
    g_ota_server_context.client_ieee = zb_ota_server_client_ieee_from_msg(msg);
    g_ota_server_context.client_nwk_addr = msg->src_addr.short_addr;
    g_ota_server_context.client_ep = msg->src_endpoint;
    rsp.status = ZCL_OTA_STATUS_SUCCESS;
    rsp.file_info = entry->file_info;
    rsp.image_size = entry->file_size;
    zb_zcl_ota_send_query_next_image_rsp(msg->dst_endpoint, &dst_addr, &rsp, header.trans_seq_num);
    zb_ota_server_publish_event(ZB_EVENT_DEVICE_OTA_STARTED, 0);
    return;
}

static void
zb_ota_server_handle_image_block_request(s_zb_af_incoming_msg_t *msg)
{
    s_zb_zcl_frame_header_t header;
    uint8_t *zcl_data = zb_zcl_parse_header(&header, msg->command.data);
    uint8_t zcl_data_length = header.fc.manu_specific ? msg->command.data_length - 5 : msg->command.data_length - 3;
    s_zb_zcl_ota_image_block_req_params_t params;
    if (zb_ota_server_parse_image_block_request(zcl_data, zcl_data_length, &params) != ZB_OK)
    {
        ZB_LOGW(TAG, "Failed to parse image block request");
        return;
    }
    ZB_LOGI(TAG, "Image Block Request: mfg=0x%X type=0x%X ver=0x%X offset=%u max_data_size=%u, seq_num=%u",
        params.file_info.manufacturer_id, params.file_info.type, params.file_info.version, params.file_offset, params.max_data_size, header.trans_seq_num);
    s_zb_zcl_ota_image_block_rsp_params_t rsp;
    s_zb_af_address_t dst_addr = {
        .address_mode = AF_ADDRESS_16BIT,
        .endpoint = msg->src_endpoint,
        .short_addr = msg->src_addr.short_addr,
    };

    if (g_ota_server_context.file_entry.file_info.version != params.file_info.version)
    {
        ZB_LOGW(TAG, "No matching OTA file found");
        rsp.status = ZCL_OTA_STATUS_ABORT;
        zb_zcl_ota_send_image_block_rsp(msg->dst_endpoint, &dst_addr, &rsp, header.trans_seq_num);
        if (g_ota_server_context.state == ZB_OTA_UPGRADE_STATUS_IN_PROGRESS)
        {
            zb_ota_server_publish_event(ZB_EVENT_DEVICE_OTA_FAILED, 0);
            zb_ota_server_clear_session(false);
        }
        return;
    }
    if (g_ota_server_context.client_ieee == 0)
    {
        g_ota_server_context.client_ieee = zb_ota_server_client_ieee_from_msg(msg);
        g_ota_server_context.client_nwk_addr = msg->src_addr.short_addr;
        g_ota_server_context.client_ep = msg->src_endpoint;
    }
    g_ota_server_context.state = ZB_OTA_UPGRADE_STATUS_IN_PROGRESS;

    uint8_t data_size = params.max_data_size > ZCL_OTA_MAX_MTU_BYTES ? ZCL_OTA_MAX_MTU_BYTES : params.max_data_size;
    // Check if client supports rate limiting feature, and if client rate needs to be updated
    if ((params.field_control & ZCL_OTA_BLOCK_FC_REQ_DELAY_PRESENT) && (params.block_req_delay != ZB_OTA_SERVER_MIN_BLOCK_PERIOD_MS))
    {
        ZB_LOGI(TAG, "Client supports rate limiting feature, updating rate %u to %u ms", params.block_req_delay, ZB_OTA_SERVER_MIN_BLOCK_PERIOD_MS);
        rsp.status = ZCL_OTA_STATUS_WAIT_FOR_DATA;
        rsp.rsp.wait.current_time = 0;
        rsp.rsp.wait.request_time = 0;
        rsp.rsp.wait.block_req_delay = ZB_OTA_SERVER_MIN_BLOCK_PERIOD_MS;
        zb_zcl_ota_send_image_block_rsp(msg->dst_endpoint, &dst_addr, &rsp, header.trans_seq_num);
        return;
    }
    uint8_t *data = (uint8_t *)ZB_MEM_MALLOC(data_size);
    if (data == NULL)
    {
        ZB_LOGW(TAG, "Failed to allocate memory");
        rsp.status = ZCL_OTA_STATUS_WAIT_FOR_DATA;
        rsp.rsp.wait.current_time = ZB_OTA_SERVER_WAIT_FOR_DATA_CUR_TIME;
        rsp.rsp.wait.request_time = ZB_OTA_SERVER_WAIT_FOR_DATA_REQ_TIME;
        rsp.rsp.wait.block_req_delay = ZB_OTA_SERVER_MIN_BLOCK_PERIOD_MS;
        zb_zcl_ota_send_image_block_rsp(msg->dst_endpoint, &dst_addr, &rsp, header.trans_seq_num);
        return;
    }
    int ret = ZB_FILE_READ(g_ota_server_context.file_entry.file_path, data, data_size, params.file_offset);
    if (ret <= 0)
    {
        ZB_LOGW(TAG, "Failed to read file");
        rsp.status = ZCL_OTA_STATUS_WAIT_FOR_DATA;
        rsp.rsp.wait.current_time = ZB_OTA_SERVER_WAIT_FOR_DATA_CUR_TIME;
        rsp.rsp.wait.request_time = ZB_OTA_SERVER_WAIT_FOR_DATA_REQ_TIME;
        rsp.rsp.wait.block_req_delay = ZB_OTA_SERVER_MIN_BLOCK_PERIOD_MS;
        zb_zcl_ota_send_image_block_rsp(msg->dst_endpoint, &dst_addr, &rsp, header.trans_seq_num);
        ZB_MEM_FREE(data);
        return;
    }
    rsp.status = ZCL_OTA_STATUS_SUCCESS;
    rsp.rsp.success.file_info = params.file_info;
    rsp.rsp.success.file_offset = params.file_offset;
    rsp.rsp.success.data_size = ret;
    rsp.rsp.success.data = data;
    zb_zcl_ota_send_image_block_rsp(msg->dst_endpoint, &dst_addr, &rsp, header.trans_seq_num);
    zb_ota_server_publish_event(ZB_EVENT_DEVICE_OTA_PROGRESS,
        zb_ota_server_progress_percent(params.file_offset, (uint32_t)ret));
    ZB_MEM_FREE(data);
    return;
}

static void
zb_ota_server_handle_upgrade_end_request(s_zb_af_incoming_msg_t *msg)
{
    s_zb_zcl_frame_header_t header;
    uint8_t *zcl_data = zb_zcl_parse_header(&header, msg->command.data);
    s_zb_zcl_ota_upgrade_end_req_params_t *params = (s_zb_zcl_ota_upgrade_end_req_params_t *)(zcl_data);
    s_zb_zcl_ota_upgrade_end_rsp_params_t rsp;

    if (g_ota_server_context.client_ieee == 0)
    {
        g_ota_server_context.client_ieee = zb_ota_server_client_ieee_from_msg(msg);
    }

    if (params->status == ZCL_OTA_STATUS_SUCCESS)
    {
        s_zb_af_address_t dst_addr = {
            .address_mode = AF_ADDRESS_16BIT,
            .endpoint = msg->src_endpoint,
            .short_addr = msg->src_addr.short_addr,
        };
        time_t now = 0;
        time(&now);
        rsp.file_info = params->file_info;
        rsp.current_time = now;
        rsp.upgrade_time = now + ZB_OTA_SERVER_UPGRADE_DELAY_SECONDS;
        zb_zcl_ota_send_upgrade_end_rsp(msg->dst_endpoint, &dst_addr, &rsp, header.trans_seq_num);
        zb_ota_server_publish_event(ZB_EVENT_DEVICE_OTA_COMPLETED, 100);
        zb_ota_server_clear_session(false);
        return;
    }

    ZB_LOGW(TAG, "Upgrade end failed: status=0x%02x", params->status);
    zb_ota_server_publish_event(ZB_EVENT_DEVICE_OTA_FAILED, 0);
    zb_ota_server_clear_session(false);
}

void
zb_ota_server_init()
{
    if (iotdev_is_file_exists(ZB_OTA_DIR) != ZB_OK)
    {
        iotdev_create_dir(ZB_OTA_DIR);
    }

    zb_ota_server_build_ota_index();
    memset(&g_ota_server_context, 0, sizeof(g_ota_server_context));
    g_ota_server_event_queue = zb_os_queue_create(ZB_OTA_SERVER_EVENT_QUEUE_SIZE, sizeof(s_zb_event_t));
}

void
zb_ota_server_deinit()
{

}

void
zb_ota_server_add_file_entry(char *file_path)
{
    if (g_ota_file_count >= ZB_OTA_SERVER_FILE_ENTRY_MAX)
    {
        ZB_LOGW(TAG, "OTA index full, skipping %s", file_path);
        return;
    }

    s_zb_ota_server_file_entry_t tmp;
    if (zb_ota_server_build_entry_from_file(file_path, &tmp) == ZB_OK)
    {
        g_ota_files[g_ota_file_count++] = tmp;
        ZB_LOGI(TAG, "Added: %s mfg=0x%4X type=0x%4X ver=0x%8X size=%u",
            file_path, tmp.file_info.manufacturer_id, tmp.file_info.type, tmp.file_info.version, tmp.file_size);
    }
    else
    {
        ZB_LOGW(TAG, "Skip file: %s", file_path);
    }
}

void
zb_ota_task(void)
{
    s_zb_event_t event;
    zb_os_queue_recv(g_ota_server_event_queue, &event, ZB_OSAL_WAIT_FOREVER);
    s_zb_af_incoming_msg_t *msg = (s_zb_af_incoming_msg_t *)event.af_msg;
    s_zb_zcl_frame_header_t header;
    zb_zcl_parse_header(&header, msg->command.data);
    switch (header.command_id)
    {
        case ZCL_OTA_COMMAND_OTA_UPGRADE_QUERY_NEXT_IMAGE_REQUEST:
            zb_ota_server_handle_query_next_image_request(msg);
            break;
        case ZCL_OTA_COMMAND_OTA_UPGRADE_IMAGE_BLOCK_REQUEST:
            zb_ota_server_handle_image_block_request(msg);
            break;
        case ZCL_OTA_COMMAND_OTA_UPGRADE_UPGRADE_END_REQUEST:
            zb_ota_server_handle_upgrade_end_request(msg);
            break;
        default:
            ZB_LOGW(TAG, "Unknown OTA command: %d", header.command_id);
            break;
    }
    zb_af_incoming_msg_free(msg);
}

int
zb_ota_server_queue_add(s_zb_event_t *event)
{
    if (zb_os_queue_send(g_ota_server_event_queue, event, 1))
    {
        return ZB_OK;
    }
    return ZB_FAIL;
}

void
zb_ota_server_abort(void)
{
    zb_ota_server_clear_session(true);
}

bool
zb_ota_server_get_progress(uint8_t *percent_out)
{
    if (g_ota_server_context.state != ZB_OTA_UPGRADE_STATUS_IN_PROGRESS)
    {
        return false;
    }
    if (percent_out != NULL)
    {
        *percent_out = g_ota_server_context.last_percent;
    }
    return true;
}

uint64_t
zb_ota_server_get_client_ieee(void)
{
    return g_ota_server_context.client_ieee;
}
