#ifndef ZB_OTA_SERVER_H_
#define ZB_OTA_SERVER_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"
#include "core/zb_core.h"
#include "ota/zb_ota_common.h"

#define ZB_OTA_FILE_NAME_MAX 64

typedef struct s_zb_ota_server_file_entry
{
    char file_path[ZB_OTA_FILE_NAME_MAX];
    s_zb_zcl_ota_file_info_t file_info;
    uint32_t file_size;
} s_zb_ota_server_file_entry_t;

void zb_ota_server_init(void);
void zb_ota_server_deinit(void);
void zb_ota_server_add_file_entry(char *file_path);
void zb_ota_server_remove_file_entry(char *file_path);
s_zb_ota_server_file_entry_t* zb_ota_server_get_file_entry(
    uint16_t manu_code, uint16_t image_type, uint32_t file_version);
void zb_ota_task(void);
int zb_ota_server_queue_add(s_zb_event_t *event);
void zb_ota_server_abort(void);
bool zb_ota_server_get_progress(uint8_t *percent_out);
uint64_t zb_ota_server_get_client_ieee(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COMPONENTS_IOTDEV_ZIGBEE_INCLUDE_OTA_IOTDEV_ZIGBEE_OTA_SERVER_H_ */