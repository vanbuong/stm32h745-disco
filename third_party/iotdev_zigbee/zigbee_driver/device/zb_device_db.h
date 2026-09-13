/*
 * zb_device_db.h
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_DEVICE_DB_H_
#define ZB_DEVICE_DB_H_

#include "common/zb_common.h"
#include "device/zb_device.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

zb_status_t zb_device_db_int(void);
zb_status_t zb_device_db_load_device(s_zb_device_t *device, char *path);
zb_status_t zb_device_db_update_device(s_zb_device_t *device, char *path);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_DEVICE_DB_H_ */
