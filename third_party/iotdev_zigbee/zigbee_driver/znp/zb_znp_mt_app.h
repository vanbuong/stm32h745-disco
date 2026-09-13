/*
 * zb_znp_mt_app.h
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_ZNP_MT_APP_H_
#define ZB_ZNP_MT_APP_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"

/**************************************************************************************************
 * APP COMMANDS
 *************************************************************************************************/
// SREQ
#define ZNP_APP_MSG                     0x00
#define ZNP_APP_USER_TEST               0x01
#define ZNP_APP_PB_ZCL_MSG              0x02
#define ZNP_APP_PB_ZCL_CFG              0x03

/* RS485 command IDs under MT_RPC_SYS_APP */
#define ZNP_APP_RS485_WRITE_REQ         0x10    /* SREQ host->ZNP: raw bytes to TX */
#define ZNP_APP_RS485_CONFIG_REQ        0x11    /* SREQ host->ZNP: set line config */
#define ZNP_APP_RS485_GET_CONFIG_REQ    0x12    /* SREQ host->ZNP: read line config */

// SRSP
#define ZNP_APP_RSP                     0x80
#define ZNP_APP_TOUCHLINK_TL_IND        0x81
#define ZNP_APP_PB_ZCL_IND              0x82

// AREQ
#define ZNP_APP_RS485_DATA_IND          0x90    /* AREQ ZNP->host: raw bytes received */
#define ZNP_APP_RS485_ERROR_IND         0x91    /* AREQ ZNP->host: line/buffer error */

#define ZNP_APP_RS485_SUCCESS           0x00
#define ZNP_APP_RS485_ERROR_LENGTH      0x01
#define ZNP_APP_RS485_ERROR_BUSY        0x02
#define ZNP_APP_RS485_ERROR_NOT_READY   0x03

#define ZNP_APP_RS485_ERROR_IND_OVERRUN 0x01
#define ZNP_APP_RS485_ERROR_IND_FRAMING 0x02
#define ZNP_APP_RS485_ERROR_IND_TX_OVERFLOW 0x03

#define ZNP_APP_RS485_PARITY_NONE       0x00
#define ZNP_APP_RS485_PARITY_EVEN       0x01
#define ZNP_APP_RS485_PARITY_ODD        0x02

typedef struct s_zb_znp_mt_app_config
{
    uint32_t baud_rate;
    uint8_t stop_bits;
    uint8_t parity;
    uint16_t idle_gap_ms;
} s_zb_znp_mt_app_config_t;

typedef struct s_zb_znp_mt_app_rs485_data_ind_t
{
    uint8_t len;
    uint8_t *data;
} s_zb_znp_mt_app_rs485_data_ind_t;

typedef int (*zb_znp_mt_app_rs485_data_ind_callback_t)(const s_zb_znp_mt_app_rs485_data_ind_t *msg);
typedef int (*zb_znp_mt_app_rs485_error_ind_callback_t)(const uint8_t *error);

typedef struct s_zb_znp_mt_app_cb
{
    zb_znp_mt_app_rs485_data_ind_callback_t pfn_rs485_data_ind_cb;
    zb_znp_mt_app_rs485_error_ind_callback_t pfn_rs485_error_ind_cb;
} s_zb_znp_mt_app_cb_t;

void zb_znp_mt_app_register_callback(s_zb_znp_mt_app_cb_t callbacks);
void zb_znp_mt_app_unregister_callback(void);
void zb_znp_mt_app_process(const uint8_t *rpc_buff, uint8_t rpc_len);
int zb_znp_mt_app_rs485_write_req(const uint8_t *data, uint8_t len);
int zb_znp_mt_app_rs485_config_req(const s_zb_znp_mt_app_config_t *config);
int zb_znp_mt_app_rs485_get_config_req(s_zb_znp_mt_app_config_t *config);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZNP_MT_APP_H_ */