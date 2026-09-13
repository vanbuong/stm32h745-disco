/*
 * zb_znp.h
 *
 * This file contains the definitions and function prototypes for the ZNP MT Interface.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_ZNP_H_
#define ZB_ZNP_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"
#include "osal/zb_osal.h"
#include "znp/zb_znp_sbl.h"

// SOF (Start of Frame) indicator byte
#define ZNP_MT_SOF                   (0xFE)

// The 3 MSB's of the 1st command field byte (Cmd0) are for command type
#define ZNP_MT_CMD_TYPE_MASK         (0xE0)

// The 5 LSB's of the 1st command field byte (Cmd0) are for the subsystem
#define ZNP_MT_SUBSYSTEM_MASK        (0x1F)

// Maximum length of RPC data field
// (1 byte length + 2 bytes command + 0-250 bytes data)
#define ZNP_MT_DATA_MAX_LEN          (250)

#define ZNP_MT_MAX_LEN               (256)

// RPC Frame field lengths
#define ZNP_MT_UART_SOF_LEN          (1)
#define ZNP_MT_UART_FCS_LEN          (1)

#define ZNP_MT_UART_FRAME_START_IDX  (1)

#define ZNP_MT_LEN_FIELD_LEN         (1)
#define ZNP_MT_CMD_FIELD_LEN         (2)

#define ZNP_MT_HDR_LEN               (ZNP_MT_LEN_FIELD_LEN + ZNP_MT_CMD_FIELD_LEN)

#define ZNP_MT_UART_HDR_LEN          (ZNP_MT_UART_SOF_LEN + ZNP_MT_HDR_LEN)

#define ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS      (1000)
#define ZNP_MT_SRSP_BDB_FORMING_TIMEOUT_MS  (20000)
#define ZNP_MT_SYS_RESET_REQ_TIMEOUT_MS     (5000)

/* Extra time a requester waits on top of its own request timeout. The znp task
 * uses the raw timeout to expire a pending request and post its completion, so
 * the guard guarantees that timeout lands first: a requester only gives up on
 * its own once the znp task is starved, never in a normal race with it. */
#define ZNP_REQ_WAIT_GUARD_MS               (100)

// Coprocessor hardware RESET pulse timing (CC253x/CC26xx ZNP reset spec).
#define ZNP_RESET_ASSERT_US                 (1000)  // RESET held low (pulse width)
#define ZNP_RESET_SETTLE_US                 (1000)  // settle after RESET deassert

#define ZNP_NOTIFY_ZNP_SRSP_BIT      (1u << 31)
#define ZNP_NOTIFY_SBL_NACK_BIT      (1u << 30)
#define ZNP_NOTIFY_SBL_ACK_BIT       (1u << 29)
#define ZNP_NOTIFY_SBL_DATA_BIT      (1u << 28)
#define ZNP_NOTIFY_CHIP_REBOOT_BIT   (1u << 27)
#define ZNP_NOTIFY_ERROR_BIT         (1u << 26)
#define ZNP_NOTIFY_WRONG_MODE_BIT    (1u << 25)
#define ZNP_NOTIFY_EXPIRED_BIT       (1u << 24)
#define ZNP_NOTIFY_BUSY_BIT          (1u << 23)
#define ZNP_NOTIFY_FLAGS_MASK        (0xFF000000u)
#define ZNP_NOTIFY_REQ_ID_MASK       (0x000000FFu)
#define ZNP_GET_STATUS(v)            ((v) & 0xFF)

typedef int (*zb_znp_set_reset_pin_callback_t)(bool state);
typedef int (*zb_znp_set_sbl_pin_callback_t)(bool state);

/**
 * @brief ZNP transport configuration.
 *
 * Carries the board-variable wiring (serial port / baud / pins) and the
 * RESET & SBL/boot pin-control callbacks so the protocol driver itself holds
 * no board-specific hardware knowledge. Populate from the board/integration
 * layer (see zb_core_init) and pass to zb_znp_init().
 */
typedef struct s_zb_znp_config
{
    uint8_t  uart_port;       // e_iotdev_uart_num_t value
    uint32_t uart_baud;       // e_iotdev_uart_baud_t value (e.g. 921600)
    int8_t   uart_tx_pin;
    int8_t   uart_rx_pin;
    int8_t   uart_rts_pin;    // GPIO_NUM_NC (-1) if unused
    int8_t   uart_cts_pin;    // GPIO_NUM_NC (-1) if unused
    uint16_t rx_stream_size;  // RX stream-buffer size in bytes; 0 = driver default
    bool     reset_on_init;   // drive a known-state power-on reset during init

    zb_znp_set_reset_pin_callback_t pfn_set_reset_pin;
    zb_znp_set_sbl_pin_callback_t   pfn_set_sbl_pin;
} s_zb_znp_config_t;

// TaskNotify status codes
typedef enum e_zb_znp_dev_state
{
    ZNP_DEV_STATE_OK = 0,
    ZNP_DEV_STATE_BUSY,
    ZNP_DEV_STATE_TIMEOUT,
    ZNP_DEV_STATE_MODE_CHANGE,
    ZNP_DEV_STATE_RESET,
    ZNP_DEV_STATE_SBL,
} e_zb_znp_dev_state_t;

typedef enum e_zb_znp_mode
{
    ZNP_MODE_ZNP = 0,
    ZNP_MODE_SBL,
} e_zb_znp_mode_t;

typedef enum e_zb_znp_cmd_req_type
{
    ZNP_CMD_REQ_TYPE_ZNP = 0,
    ZNP_CMD_REQ_TYPE_SBL,
} e_zb_znp_cmd_req_type_t;

// Cmd0 Command Type
typedef enum e_zb_znp_mt_cmd_type
{
    ZNP_MT_CMD_POLL = 0x00, // POLL command
    ZNP_MT_CMD_SREQ = 0x20, // Synchronous Request command
    ZNP_MT_CMD_AREQ = 0x40, // Asynchronous Request command
    ZNP_MT_CMD_SRSP = 0x60, // Synchronous Response command
} e_zb_znp_mt_cmd_type_t;

// Cmd0 Subsystem
typedef enum e_zb_znp_mt_subsystem
{
    ZNP_MT_SYS_RES0,   // Reserved for future use
    ZNP_MT_SYS_SYS,    // System commands
    ZNP_MT_SYS_MAC,    // MAC commands
    ZNP_MT_SYS_NWK,    // Zigbee Network commands
    ZNP_MT_SYS_AF,     // Application Framework commands
    ZNP_MT_SYS_ZDO,    // Zigbee Device Object commands
    ZNP_MT_SYS_SAPI,   // SAPI commands
    ZNP_MT_SYS_UTIL,   // Utility commands
    ZNP_MT_SYS_DBG,    // Debug commands
    ZNP_MT_SYS_APP,    // Application commands
    ZNP_MT_SYS_OTA,    // Over-the-Air commands
    ZNP_MT_SYS_ZNP,    // ZNP commands
    ZNP_MT_SYS_SPARE_12,
    ZNP_MT_SYS_UBL = 13,    // 13 to be compatible with existing RemoTI
    ZNP_MT_SYS_RES14,
    ZNP_MT_SYS_APP_CFG = 15,// 15 is APP CONFIG Sub-system
    ZNP_MT_SYS_RES16,
    ZNP_MT_SYS_PROTOBUF,
    ZNP_MT_SYS_RES18,
    ZNP_MT_SYS_GP = 21,
    ZNP_MT_SYS_MAX
} e_zb_znp_mt_subsystem_t;

typedef struct s_zb_znp_mode_req
{
    e_zb_znp_mode_t mode;
    bool wants_response;
    uint32_t timeout_ms;
    uint8_t request_id;     /* same id space as s_zb_znp_req_t: a mode change
                               shares req_signal with the SREQ path, so its
                               waiter must be able to reject stale posts too. */
} s_zb_znp_mode_req_t;

typedef struct s_zb_znp_req
{
    e_zb_znp_cmd_req_type_t type;
    uint32_t start_tick;
    bool wants_response;
    uint32_t timeout_ms;
    uint8_t request_id;

    union
    {
        struct
        {
            uint8_t cmd0;
            uint8_t cmd1;
            uint8_t payload_len;
            uint16_t resp_len;
            uint8_t *payload;
            uint8_t *resp_data;
        } znp;
        struct
        {
            uint8_t cmd;
            uint8_t data_len;
            uint8_t resp_len;
            uint8_t *data;
            uint8_t *resp_data;
        } sbl;
    };
} s_zb_znp_req_t;

typedef enum e_zb_znp_state
{
    ZNP_STATE_SOF = 0,
    ZNP_STATE_LENGTH,
    ZNP_STATE_CMD0,
    ZNP_STATE_CMD1,
    ZNP_STATE_PAYLOAD,
    ZNP_STATE_FCS
} e_zb_znp_state_t;

typedef enum e_zb_znp_sbl_state
{
    ZNP_SBL_STATE_WAIT_LEN = 0,
    ZNP_SBL_STATE_READ_CHECKSUM,
    ZNP_SBL_STATE_READ_DATA,
    ZNP_SBL_STATE_WAIT_ACK_NACK,
} e_zb_znp_sbl_state_t;

typedef struct s_zb_znp_parser
{
    e_zb_znp_state_t state;
    uint8_t *buf;
    uint16_t buf_len;
    uint16_t pos;
    uint8_t fcs;
} s_zb_znp_parser_t;

typedef struct s_zb_znp_sbl_parser
{
    e_zb_znp_sbl_state_t state;
    uint8_t buf[SBL_MAX_RX_BUFFER_SIZE];
    uint8_t buf_len;
    uint8_t pos;
    uint8_t checksum;
} s_zb_znp_sbl_parser_t;

typedef enum e_zb_znp_sbl_event_type
{
    ZNP_SBL_EVT_NONE = 0,
    ZNP_SBL_EVT_ACK,
    ZNP_SBL_EVT_NACK,
    ZNP_SBL_EVT_DATA
} e_zb_znp_sbl_event_type_t;

typedef struct s_zb_znp_sbl_event
{
    e_zb_znp_sbl_event_type_t type;
    uint8_t data[SBL_MAX_RX_BUFFER_SIZE];
    uint8_t data_len;
} s_zb_znp_sbl_event_t;

typedef struct s_zb_znp_context
{
    zb_os_stream_t recv_buffer;
    zb_os_queue_t req_queue;
    zb_os_queue_t mode_req_queue;
    zb_os_signal_t req_signal;      /* completion signal: znp task sets, requester waits */
    zb_os_mutex_t req_mutex;        /* serializes the issue+wait critical section across
                                       requester tasks (SREQ + mode/reset). The context and
                                       req_signal hold a single outstanding request, so
                                       concurrent requesters (e.g. Zigbee task vs a direct
                                       get_core_temperature / RS485 call) must not overlap. */

    e_zb_znp_mode_t mode;
    e_zb_znp_dev_state_t dev_state;
    s_zb_znp_parser_t znp_parser;
    s_zb_znp_sbl_parser_t sbl_parser;

    bool znp_waiting;
    uint8_t znp_cmd0;
    uint8_t znp_cmd1;
    uint8_t znp_req_id;
    bool znp_wants_response;

    bool sbl_waiting;
    uint8_t sbl_cmd;
    uint8_t sbl_req_id;
    bool sbl_wants_response;

    /* Response staging. The znp task copies SRSP / SBL payloads in here and the
     * requester copies them out after its completion arrives. The znp task must
     * never write through a requester-supplied pointer: a requester that has
     * given up has already unwound the stack frame its response buffer lives
     * in, so a late response would corrupt an unrelated task's stack. Only one
     * synchronous request is outstanding at a time (req_mutex), so a single
     * staging buffer suffices. */
    uint8_t resp_stage[ZNP_MT_DATA_MAX_LEN];
    uint16_t resp_stage_len;

    uint32_t req_tick;
    uint32_t req_timeout_ms;

    zb_znp_set_reset_pin_callback_t pfn_set_reset_pin_cb;
    zb_znp_set_sbl_pin_callback_t pfn_set_sbl_pin_cb;
} s_zb_znp_context_t;

/**
 * @brief Initialise the ZNP transport (RTOS objects, UART, pin callbacks).
 *
 * @param cfg  Transport configuration; must be non-NULL with both pin
 *             callbacks set.
 * @return ZB_OK on success;
 *         ZB_INVALID_PARAMETER if cfg or a required callback is NULL;
 *         ZB_MEM_ERROR if an RTOS object could not be allocated.
 */
zb_status_t zb_znp_init(const s_zb_znp_config_t *cfg);
void zb_znp_deinit(void);
int zb_znp_send_cmd_req(s_zb_znp_req_t *req);
int zb_znp_set_mode_znp(uint32_t timeout_ms);
int zb_znp_set_mode_sbl(uint32_t timeout_ms);

void zb_znp_task(void);
int zb_znp_awaiting_reply(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZNP_H_ */
