#include "znp/zb_znp.h"
#include "znp/zb_znp_sbl.h"

#define TAG "ZNP_SBL"

#define SBL_MAX_TX_BUFFER_SIZE          256
#define SBL_MAX_DATA_SIZE               252
#define SBL_FLASH_SECTOR_SIZE           (8 * 1024)
#define SBL_FLASH_NV_BASE_ADDRESS       0xA2000
#define SBL_FLASH_NV_NUMBER_OF_SECTOR   0

static uint8_t g_tx_buffer[SBL_MAX_TX_BUFFER_SIZE];

static void
sbl_report_progress(zb_znp_sbl_progress_cb_t progress_cb, void *progress_ctx,
                    const char *what, uint8_t percent, uint8_t *previous_percent)
{
    if (((percent / 10) <= (*previous_percent / 10)) && (percent < 100))
    {
        return;
    }
    *previous_percent = percent;
    ZB_LOGI(TAG, "%s...%u%%", what, (unsigned)percent);
    if (progress_cb != NULL)
    {
        progress_cb(percent, progress_ctx);
    }
}

uint8_t
zb_znp_sbl_cal_crc(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    while (len--)
    {
        crc += data[len];
    }
    return crc;
}

zb_status_t
zb_znp_sbl_send_get_status_cmd(uint32_t timeout, uint8_t *status)
{
    uint8_t buff[SBL_COMMAND_GET_STATUS_LEN] = {SBL_COMMAND_GET_STATUS_LEN, SBL_COMMAND_GET_STATUS, SBL_COMMAND_GET_STATUS};
    s_zb_znp_req_t req;
    req.type = ZNP_CMD_REQ_TYPE_SBL;
    req.start_tick = zb_os_now_ms();
    req.sbl.cmd = SBL_COMMAND_GET_STATUS;
    req.sbl.data = buff;
    req.sbl.data_len = SBL_COMMAND_GET_STATUS_LEN;
    req.sbl.resp_len = 1;
    req.sbl.resp_data = status;
    req.wants_response = true;
    req.timeout_ms = timeout;
    return zb_znp_send_cmd_req(&req);
}

zb_status_t
zb_znp_sbl_send_cmd_with_status_check(s_zb_znp_req_t *req, uint32_t timeout, uint8_t try_count)
{
    uint8_t status = 0;
    while (try_count > 0)
    {
        try_count--;
        int err = zb_znp_send_cmd_req(req);
        if (err != ZB_SUCCESS)
        {
            continue;
        }
        err = zb_znp_sbl_send_get_status_cmd(timeout, &status);
        if (err != ZB_SUCCESS || status != SBL_COMMAND_RET_SUCCESS)
        {
            continue;
        }
        return ZB_SUCCESS;
    }
    ZB_LOGE(TAG, "Send command failed, status %d", status);
    return ZB_ERR_SBL_COMMAND_FAILED;
}

zb_status_t
zb_znp_sbl_send_download_cmd(size_t total_len, uint32_t timeout, uint8_t try_count)
{
    uint32_t start_addr = 0;
    uint8_t buff[SBL_COMMAND_DOWNLOAD_LEN] = {SBL_COMMAND_DOWNLOAD_LEN, 0, SBL_COMMAND_DOWNLOAD,
        (start_addr >> 24) & 0xFF, (start_addr >> 16) & 0xFF, (start_addr >> 8) & 0xFF, start_addr & 0xFF,
        (total_len >> 24) & 0xFF, (total_len >> 16) & 0xFF, (total_len >> 8) & 0xFF, total_len & 0xFF};
    buff[1] = zb_znp_sbl_cal_crc(buff + 2, SBL_COMMAND_DOWNLOAD_LEN - 2);

    s_zb_znp_req_t req;
    req.type = ZNP_CMD_REQ_TYPE_SBL;
    req.start_tick = zb_os_now_ms();
    req.sbl.cmd = SBL_COMMAND_DOWNLOAD;
    req.sbl.data = buff;
    req.sbl.data_len = SBL_COMMAND_DOWNLOAD_LEN;
    req.sbl.resp_len = 0;
    req.sbl.resp_data = NULL;
    req.wants_response = true;
    req.timeout_ms = timeout;
    return zb_znp_sbl_send_cmd_with_status_check(&req, timeout, try_count);
}

zb_status_t
zb_znp_sbl_send_data_cmd(uint8_t *data, uint8_t len, uint32_t timeout, uint8_t try_count)
{
    data[0] = len;
    data[2] = SBL_COMMAND_SEND_DATA;
    data[1] = zb_znp_sbl_cal_crc(data + 2, len - 2);

    s_zb_znp_req_t req;
    req.type = ZNP_CMD_REQ_TYPE_SBL;
    req.start_tick = zb_os_now_ms();
    req.sbl.cmd = SBL_COMMAND_SEND_DATA;
    req.sbl.data = data;
    req.sbl.data_len = len;
    req.sbl.resp_len = 0;
    req.sbl.resp_data = NULL;
    req.wants_response = true;
    req.timeout_ms = timeout;
    return zb_znp_sbl_send_cmd_with_status_check(&req, timeout, try_count);
}

zb_status_t
zb_znp_sbl_send_sector_erase_cmd(uint32_t sector_addr, uint32_t timeout, uint8_t try_count)
{
    uint8_t buff[SBL_COMMAND_SECTOR_ERASE_LEN] = {SBL_COMMAND_SECTOR_ERASE_LEN, 0, SBL_COMMAND_SECTOR_ERASE,
        (sector_addr >> 24) & 0xFF, (sector_addr >> 16) & 0xFF, (sector_addr >> 8) & 0xFF, sector_addr & 0xFF};
    buff[1] = zb_znp_sbl_cal_crc(buff + 2, SBL_COMMAND_SECTOR_ERASE_LEN - 2);
    s_zb_znp_req_t req;
    req.type = ZNP_CMD_REQ_TYPE_SBL;
    req.start_tick = zb_os_now_ms();
    req.sbl.cmd = SBL_COMMAND_SECTOR_ERASE;
    req.sbl.data = buff;
    req.sbl.data_len = SBL_COMMAND_SECTOR_ERASE_LEN;
    req.sbl.resp_len = 0;
    req.sbl.resp_data = NULL;
    req.wants_response = true;
    req.timeout_ms = timeout;
    return zb_znp_sbl_send_cmd_with_status_check(&req, timeout, try_count);
}

zb_status_t
zb_znp_sbl_send_crc32_cmd(uint32_t start_addr, uint32_t size, uint32_t read_count, uint32_t timeout, uint8_t try_count, uint32_t *received_crc32)
{
    uint8_t buff[SBL_COMMAND_CRC32_LEN] = {SBL_COMMAND_CRC32_LEN, 0, SBL_COMMAND_CRC32,
        (start_addr >> 24) & 0xFF, (start_addr >> 16) & 0xFF, (start_addr >> 8) & 0xFF, start_addr & 0xFF,
        (size >> 24) & 0xFF, (size >> 16) & 0xFF, (size >> 8) & 0xFF, size & 0xFF,
        (read_count >> 24) & 0xFF, (read_count >> 16) & 0xFF, (read_count >> 8) & 0xFF, read_count & 0xFF};
    buff[1] = zb_znp_sbl_cal_crc(buff + 2, SBL_COMMAND_CRC32_LEN - 2);
    uint8_t resp_data[sizeof(uint32_t)] = {0};
    s_zb_znp_req_t req;
    req.type = ZNP_CMD_REQ_TYPE_SBL;
    req.start_tick = zb_os_now_ms();
    req.sbl.cmd = SBL_COMMAND_CRC32;
    req.sbl.data = buff;
    req.sbl.data_len = SBL_COMMAND_CRC32_LEN;
    req.sbl.resp_len = sizeof(uint32_t);
    req.sbl.resp_data = resp_data;
    req.wants_response = true;
    req.timeout_ms = timeout;
    int err = zb_znp_sbl_send_cmd_with_status_check(&req, timeout, try_count);
    if (err == ZB_OK)
    {
        *received_crc32 = resp_data[0] << 24 | resp_data[1] << 16 | resp_data[2] << 8 | resp_data[3];
    }
    return err;
}

zb_status_t
zb_znp_sbl_send_ping_cmd(uint32_t timeout)
{
    uint8_t data[SBL_COMMAND_PING_LEN] = {SBL_COMMAND_PING_LEN, SBL_COMMAND_PING, SBL_COMMAND_PING};
    s_zb_znp_req_t req;
    req.type = ZNP_CMD_REQ_TYPE_SBL;
    req.start_tick = zb_os_now_ms();
    req.sbl.cmd = SBL_COMMAND_PING;
    req.sbl.data = data;
    req.sbl.data_len = SBL_COMMAND_PING_LEN;
    req.sbl.resp_len = 0;
    req.sbl.resp_data = NULL;
    req.wants_response = true;
    req.timeout_ms = timeout;
    return zb_znp_send_cmd_req(&req);
}

zb_status_t
zb_znp_sbl_upgrade_firmware(const char *file_path, bool erase_nv_data,
                            zb_znp_sbl_progress_cb_t progress_cb, void *progress_ctx)
{
    int err = ZB_SUCCESS;
    size_t file_size = 0;
    size_t original_file_size = 0;
    size_t file_offset = 0;
    uint32_t calculated_crc32 = 0;
    uint32_t received_crc32 = 0;
    uint8_t num_sector_erase = 0;
    uint8_t total_sector_erase = 0;
    uint32_t erase_sector_address = 0;
    uint8_t current_percent = 0;
    uint8_t previous_percent = 0;

    if (ZB_IS_FILE_EXISTS(file_path) != ZB_SUCCESS)
    {
        ZB_LOGE(TAG, "File %s not found", file_path);
        return ZB_FAILURE;
    }

    file_size = ZB_GET_FILE_SIZE(file_path);
    original_file_size = file_size;
    ZB_LOGI(TAG, "File size %lu", file_size);
    num_sector_erase = (file_size + SBL_FLASH_SECTOR_SIZE - 1) / SBL_FLASH_SECTOR_SIZE;
    total_sector_erase = erase_nv_data ? num_sector_erase + SBL_FLASH_NV_NUMBER_OF_SECTOR : num_sector_erase;
    ZB_LOGI(TAG, "Number of sector erase %d", total_sector_erase);
    previous_percent = 0;
    current_percent = 0;
    for (uint8_t i = 0; i < total_sector_erase; i++)
    {
        if (i >= num_sector_erase)
        {
            erase_sector_address = SBL_FLASH_NV_BASE_ADDRESS + (i - num_sector_erase) * SBL_FLASH_SECTOR_SIZE;
        }
        else
        {
            erase_sector_address = i * SBL_FLASH_SECTOR_SIZE;
        }
        err = zb_znp_sbl_send_sector_erase_cmd(erase_sector_address, SBL_COMMAND_DELAY_MS, SBL_COMMAND_TRY_COUNT_MAX);
        if (err != ZB_SUCCESS)
        {
            ZB_LOGE(TAG, "Send sector %d erase command failed", i);
            goto exit;
        }
        current_percent = ((i + 1) * 50) / total_sector_erase;
        sbl_report_progress(progress_cb, progress_ctx, "Erasing sector",
                            current_percent, &previous_percent);
    }

    err = zb_znp_sbl_send_download_cmd(file_size, SBL_COMMAND_DELAY_MS, SBL_COMMAND_TRY_COUNT_MAX);
    if (err != ZB_SUCCESS)
    {
        ZB_LOGE(TAG, "Send download command failed");
        goto exit;
    }
    while (file_size)
    {
        uint8_t send_byte = file_size > SBL_MAX_DATA_SIZE ? SBL_MAX_DATA_SIZE : file_size;
        int read_byte = ZB_FILE_READ(file_path, g_tx_buffer + SBL_COMMAND_SEND_DATA_LEN_MIN, send_byte, file_offset);
        if (read_byte != send_byte)
        {
            ZB_LOGE(TAG, "Read file failed, offset %lu", file_offset);
            err = ZB_ERR_SBL_FILE_READ;
            goto exit;
        }
        calculated_crc32 = ZB_CRC32(calculated_crc32, g_tx_buffer + SBL_COMMAND_SEND_DATA_LEN_MIN, send_byte);

        err = zb_znp_sbl_send_data_cmd(g_tx_buffer, send_byte + SBL_COMMAND_SEND_DATA_LEN_MIN, SBL_COMMAND_DELAY_MS, SBL_COMMAND_TRY_COUNT_MAX);
        if (err != ZB_SUCCESS)
        {
            ZB_LOGE(TAG, "Send data failed, offset %lu", file_offset);
            goto exit;
        }

        file_size -= send_byte;
        file_offset += send_byte;
        current_percent = 50 + (uint8_t)((file_offset * 50) / original_file_size);
        sbl_report_progress(progress_cb, progress_ctx, "Updating firmware to co-processor",
                            current_percent, &previous_percent);
    }

    err = zb_znp_sbl_send_crc32_cmd(0, file_offset, 0, SBL_COMMAND_DELAY_MS * 5, SBL_COMMAND_TRY_COUNT_MAX, &received_crc32);
    if (err != ZB_SUCCESS)
    {
        ZB_LOGE(TAG, "Send crc32 command failed");
        goto exit;
    }

    if (received_crc32 != calculated_crc32)
    {
        ZB_LOGE(TAG, "CRC32 mismatch, calculated %08X, received %08X", calculated_crc32, received_crc32);
        err = ZB_ERR_SBL_CRC32;
        goto exit;
    }
    ZB_LOGI(TAG, "Firmware written to co-processor successfully");
    sbl_report_progress(progress_cb, progress_ctx, "Updating firmware to co-processor",
                        100, &previous_percent);
    err = ZB_SUCCESS;
exit:
    return err;
}
