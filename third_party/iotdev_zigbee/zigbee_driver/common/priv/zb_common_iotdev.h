#ifndef ZB_COMMON_IOTDEV_H_
#define ZB_COMMON_IOTDEV_H_

#if defined(ZB_PLATFORM_IOTDEV)
#include "iotdev_common/include/iotdev_common.h"
#include "iotdev_utils/iotdev_filesystem_ops/iotdev_filesystem_ops.h"
#include "iotdev_utils/iotdev_datetime/iotdev_datetime.h"
#include "iotdev_utils/iotdev_protobuf/nanopb/pb_encode.h"
#include "iotdev_utils/iotdev_protobuf/nanopb/pb_decode.h"
#include "esp_crc.h"

#define LOG_LEVEL       3

/// Display Show Information Log Level Definition
#if LOG_LEVEL >= 0
// Display Show Information Log Level Definition
#define ZB_LOGS(TAG, fmt, ...)                          IOTDEV_LOGW(TAG, fmt, ##__VA_ARGS__)
// Raw buffer in HEX format 
#define ZB_LOGS_BUFFER_HEX(TAG, data, len)              IOTDEV_LOG_BUFFER_HEX(TAG, data, len)
#else
#define ZB_LOGS(TAG, fmt, ...)                          do { } while (0)
#define ZB_LOGS_BUFFER_HEX(TAG, data, len)              do { } while (0)
#endif

/// Display Show Information Log Level Definition
#if LOG_LEVEL >= 1
// Warning Log Level Definition
#define ZB_LOGW(TAG, fmt, ...)                          IOTDEV_LOGW(TAG, fmt, ##__VA_ARGS__)
#else
#define ZB_LOGW(TAG, fmt, ...)                          do { } while (0)
#endif

/// Display Show Information Log Level Definition
#if LOG_LEVEL >= 2
// Error Log Level Definition
#define ZB_LOGE(TAG, fmt, ...)                          IOTDEV_LOGE(TAG, fmt, ##__VA_ARGS__)
#else
#define ZB_LOGE(TAG, fmt, ...)                          do { } while (0)
#endif

/// Display Show Information Log Level Definition
#if LOG_LEVEL >= 3
// Info Log Level Definition
#define ZB_LOGI(TAG, fmt, ...)                          IOTDEV_LOGI(TAG, fmt, ##__VA_ARGS__)
// Raw buffer in HEX format 
#define ZB_LOG_BUFFER_HEX(TAG, data, len)               do { } while (0)
// #define ZB_LOG_BUFFER_HEX(TAG, data, len)               ESP_LOG_BUFFER_HEX(TAG, data, len)
#else
#define ZB_LOGI(TAG, fmt, ...)                          do { } while (0)
#define ZB_LOG_BUFFER_HEX(TAG, data, len)               do { } while (0)
#endif

/// Display Show Information Log Level Definition
#if LOG_LEVEL >= 4
// Debug Log Level Definition
#define ZB_LOGD(TAG, fmt, ...)                          IOTDEV_LOGD(TAG, fmt, ##__VA_ARGS__)
#else
#define ZB_LOGD(TAG, fmt, ...)                          do { } while (0)
#endif

#define ZB_MEM_MALLOC                                   IOTDEV_MEM_MALLOC
#define ZB_MEM_CALLOC                                   IOTDEV_MEM_CALLOC
#define ZB_MEM_REALLOC                                  IOTDEV_MEM_REALLOC
#define ZB_MEM_FREE                                     IOTDEV_MEM_FREE

#define TYPEDEF_STRUCT_PACKED                           typedef struct __attribute__((packed))

#define ZB_TASK_DELAY_MS(x)                             IOTDEV_TASK_DELAY_MS(x)

#define SYS_TIME_NOW_MS()                               IOTDEV_SYS_TIME_NOW_MS()

#define SYS_TIME_NOW_US()                               IOTDEV_SYS_TIME_NOW_US()

#define SYS_TIME_NOW_TICK()                             xTaskGetTickCount()

#define TASK_GET_CURRENT_TASK_HANDLE()                  xTaskGetCurrentTaskHandle()

#define ZB_DIR                                          FS_MOUNT_POINT"/zb"

#define ZB_OTA_DIR                                      FS_MOUNT_POINT"/zb_ota"

#define ZB_DEVICE_DB_FILENAME                           ZB_DIR"/zb_device.db"

#define ZB_NETWORK_CFG_FILENAME                         ZB_DIR"/zb_network.cfg"

#define ZB_CRC32(crc, data, size)                       esp_crc32_le(crc, data, size)

#define ZB_GET_FILE_SIZE(file_path)                     iotdev_get_file_size(file_path)

#define ZB_FILE_READ(file_path, data, size, offset)     iotdev_file_read(file_path, data, size, offset)

#define ZB_FILE_WRITE(file_path, data, size, offset)    iotdev_file_write(file_path, data, size, offset)

#define ZB_IS_FILE_EXISTS(file_path)                    iotdev_is_file_exists(file_path)

#define ZB_CREATE_DIR(dir_path)                         iotdev_create_dir(dir_path)
#endif /* ZB_PLATFORM_IOTDEV */

#endif /* ZB_COMMON_IOTDEV_H_ */