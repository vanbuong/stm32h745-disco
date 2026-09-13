#ifndef ZB_ZNP_SBL_H_
#define ZB_ZNP_SBL_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"

/* Bootloader commands definition */
#define SBL_COMMAND_PING                0x20
#define SBL_COMMAND_DOWNLOAD            0x21
#define SBL_COMMAND_GET_STATUS          0x23
#define SBL_COMMAND_SEND_DATA           0x24
#define SBL_COMMAND_RESET               0x25
#define SBL_COMMAND_SECTOR_ERASE        0x26
#define SBL_COMMAND_CRC32               0x27
#define SBL_COMMAND_GET_CHIP_ID         0x28
#define SBL_COMMAND_MEMORY_READ         0x2A
#define SBL_COMMAND_MEMORY_WRITE        0x2B
#define SBL_COMMAND_BANK_ERASE          0x2C
#define SBL_COMMAND_SET_CCFG            0x2D
#define SBL_COMMAND_DOWNLOAD_CRC        0x2F

#define SBL_COMMAND_PING_LEN            3
#define SBL_COMMAND_DOWNLOAD_LEN        11
#define SBL_COMMAND_GET_STATUS_LEN      3
#define SBL_COMMAND_STATUS_RSP_LEN      2
#define SBL_COMMAND_SEND_DATA_LEN_MIN   3
#define SBL_COMMAND_RESET_LEN           3
#define SBL_COMMAND_SECTOR_ERASE_LEN    7
#define SBL_COMMAND_CRC32_LEN           15
#define SBL_COMMAND_CRC32_RSP_LEN       6
#define SBL_COMMAND_GET_CHIP_ID_LEN     3
#define SBL_COMMAND_MEMORY_READ_LEN     9
#define SBL_COMMAND_MEMORY_WRITE_LEN    9
#define SBL_COMMAND_BANK_ERASE_LEN      3
#define SBL_COMMAND_SET_CCFG_LEN        11
#define SBL_COMMAND_DOWNLOAD_CRC_LEN    15
#define SBL_COMMAND_SYNC_LEN            2

/* Bootloader defined status values definition */
#define SBL_COMMAND_RET_SUCCESS         0x40
#define SBL_COMMAND_RET_UNKNOWN_CMD     0x41
#define SBL_COMMAND_RET_INVALID_CMD     0x42
#define SBL_COMMAND_RET_INVALID_ADR     0x43
#define SBL_COMMAND_RET_FLASH_FAIL      0x44

#define SBL_COMMAND_SYNC                0x55

/* Bootloader defined response values definition */
#define SBL_COMMAND_ACK                 0xCC
#define SBL_COMMAND_NACK                0x33
#define SBL_COMMAND_ACK_LEN             2
#define SBL_COMMAND_NACK_LEN            2

#define SBL_COMMAND_TRY_COUNT_MAX       3
#define SBL_COMMAND_DELAY_MS            100

#define SBL_MAX_RX_BUFFER_SIZE          8

typedef void (*zb_znp_sbl_progress_cb_t)(uint8_t percent, void *ctx);

zb_status_t zb_znp_sbl_init(void);
zb_status_t zb_znp_sbl_deinit(void);
uint8_t zb_znp_sbl_cal_crc(uint8_t *data, uint8_t len);
zb_status_t zb_znp_sbl_send_ping_cmd(uint32_t timeout);
zb_status_t zb_znp_sbl_upgrade_firmware(const char *file_path, bool erase_nv_data,
                                        zb_znp_sbl_progress_cb_t progress_cb, void *progress_ctx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZNP_SBL_H_ */