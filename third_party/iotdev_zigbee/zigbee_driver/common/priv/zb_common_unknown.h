#ifndef ZB_COMMON_UNKNOWN_H_
#define ZB_COMMON_UNKNOWN_H_

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

/* Host builds have no system clock wired here; these resolve to 0 so timing
 * expressions compile. Suites needing real time use the OSAL clock directly. */
#define SYS_TIME_NOW_MS()                       (0)

#define SYS_TIME_NOW_US()                       (0)

#define ZB_DIR                                  "/zb"

#define ZB_OTA_DIR                              ZB_DIR"/zb_ota"

#define ZB_DEVICE_DB_FILENAME                   ZB_DIR"/zb_device.db"

#define ZB_NETWORK_CFG_FILENAME                 ZB_DIR"/zb_network.cfg"

/* Host (off-target) builds have no flash filesystem or CRC accelerator. These
 * are benign stubs so persistence-using code compiles and links for unit tests;
 * suites that need real behaviour route the underlying iotdev_* calls to their
 * own stubs instead. */
#define ZB_CRC32(crc, data, size)               (0u)
#define ZB_GET_FILE_SIZE(file_path)             (0)
#define ZB_FILE_READ(file_path, data, size, offset)   (0)
#define ZB_FILE_WRITE(file_path, data, size, offset)  (0)
#define ZB_IS_FILE_EXISTS(file_path)            (-1)
#define ZB_CREATE_DIR(dir_path)                 (0)

#endif /* ZB_COMMON_UNKNOWN_H_ */