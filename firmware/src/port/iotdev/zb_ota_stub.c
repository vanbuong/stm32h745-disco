#include "ota/zb_ota_server.h"

void zb_ota_server_init(void)
{
}

void zb_ota_server_deinit(void)
{
}

void zb_ota_server_add_file_entry(char *file_path)
{
    (void)file_path;
}

void zb_ota_server_remove_file_entry(char *file_path)
{
    (void)file_path;
}

s_zb_ota_server_file_entry_t *zb_ota_server_get_file_entry(uint16_t manu_code, uint16_t image_type,
                                                           uint32_t file_version)
{
    (void)manu_code;
    (void)image_type;
    (void)file_version;
    return NULL;
}

void zb_ota_task(void)
{
}

int zb_ota_server_queue_add(s_zb_event_t *event)
{
    (void)event;
    return ZB_OK;
}

void zb_ota_server_abort(void)
{
}

bool zb_ota_server_get_progress(uint8_t *percent_out)
{
    if (percent_out != NULL) {
        *percent_out = 0u;
    }
    return false;
}

uint64_t zb_ota_server_get_client_ieee(void)
{
    return 0;
}
