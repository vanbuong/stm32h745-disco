#ifndef ZB_COMMON_UNKNOWN_H_
#define ZB_COMMON_UNKNOWN_H_

#ifdef ZB_USE_VFS
#include "zb_port_fs.h"
#include "zb_osal.h"
#endif

#define ZB_LOGS(TAG, fmt, ...)                  do { } while (0)
#define ZB_LOGS_BUFFER_HEX(TAG, data, len)      do { } while (0)
#define ZB_LOG_BUFFER_HEX(TAG, data, len)       do { } while (0)
#define ZB_LOGW(TAG, fmt, ...)                  do { } while (0)
#define ZB_LOGI(TAG, fmt, ...)                  do { } while (0)
#define ZB_LOGXI(TAG, fmt, ...)                 do { } while (0)
#define ZB_LOGD(TAG, fmt, ...)                  do { } while (0)
#define ZB_LOGE(TAG, fmt, ...)                  do { } while (0)

#define ZB_MEM_MALLOC                           malloc
#define ZB_MEM_CALLOC                           calloc
#define ZB_MEM_REALLOC                          realloc
#define ZB_MEM_FREE                             free

#define TYPEDEF_STRUCT_PACKED                   typedef struct

#define ZB_TASK_DELAY_MS(x)

#ifdef ZB_USE_VFS
#define SYS_TIME_NOW_MS()                       zb_os_now_ms()
#define SYS_TIME_NOW_US()                       (zb_os_now_ms() * 1000u)
#define ZB_DIR                                  "/user/home/zb"
#define ZB_OTA_DIR                              ZB_DIR "/ota"
#define ZB_DEVICE_DB_FILENAME                   ZB_DIR "/zb_device.db"
#define ZB_NETWORK_CFG_FILENAME                 ZB_DIR "/zb_network.cfg"
#define ZB_CRC32(crc, data, size)               zb_port_crc32((crc), (data), (size))
#define ZB_GET_FILE_SIZE(file_path)             zb_port_file_size(file_path)
#define ZB_FILE_READ(file_path, data, size, offset) \
    zb_port_file_read((file_path), (data), (size), (offset))
#define ZB_FILE_WRITE(file_path, data, size, offset) \
    zb_port_file_write((file_path), (data), (size), (offset))
#define ZB_IS_FILE_EXISTS(file_path)            iotdev_is_file_exists(file_path)
#define ZB_CREATE_DIR(dir_path)                 iotdev_create_dir(dir_path)
#else
#define SYS_TIME_NOW_MS()                       (0)
#define SYS_TIME_NOW_US()                       (0)
#define ZB_DIR                                  "/zb"
#define ZB_OTA_DIR                              ZB_DIR "/zb_ota"
#define ZB_DEVICE_DB_FILENAME                   ZB_DIR "/zb_device.db"
#define ZB_NETWORK_CFG_FILENAME                 ZB_DIR "/zb_network.cfg"
#define ZB_CRC32(crc, data, size)               (0u)
#define ZB_GET_FILE_SIZE(file_path)             (0)
#define ZB_FILE_READ(file_path, data, size, offset)   (0)
#define ZB_FILE_WRITE(file_path, data, size, offset)  (0)
#define ZB_IS_FILE_EXISTS(file_path)            (-1)
#define ZB_CREATE_DIR(dir_path)                 (0)
#endif

#endif /* ZB_COMMON_UNKNOWN_H_ */