/*
 * zb_znp.c
 *
 * This file implements the functions for the main MT module.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#include "znp/zb_znp.h"
#include "znp/zb_znp_mt_af.h"
#include "znp/zb_znp_mt_util.h"
#include "znp/zb_znp_mt_app.h"
#include "znp/zb_znp_mt_app_cfg.h"
#include "znp/zb_znp_mt_sys.h"
#include "znp/zb_znp_mt_zdo.h"
#include "znp/zb_znp_sbl.h"
#include "platform/iotdev/zb_iotdev_peripheral.h"
#include "platform/iotdev/zb_plat_serial.h"
#define TAG                         "ZNP"

#define ZIGBEE_ZNP_QUEUE_SIZE       16
#define ZIGBEE_ZNP_MODE_QUEUE_SIZE  1
#define ZIGBEE_ZNP_MSG_BUFFER_SIZE  256

typedef struct s_zb_znp_frame
{
    uint8_t *buf;
    uint16_t buf_len;
} s_zb_znp_frame_t;

static void znp_process_incoming_data(const uint8_t *data, uint16_t len);
static int znp_requester_send_sync(s_zb_znp_req_t *req, uint32_t success_notify_flags);
static void znp_full_reset_state(bool enter_bootloader);
static void znp_destroy_rtos_objects(void);

// queue for ZNP requests that will be sent out to UART
static s_zb_znp_context_t g_znp_context;
static uint8_t g_znp_req_id_gen = 0;
static bool g_znp_initialised = false;

static inline uint32_t
znp_make_notify_value(uint32_t flags, uint8_t request_id)
{
    return (flags & ZNP_NOTIFY_FLAGS_MASK) | (request_id & ZNP_NOTIFY_REQ_ID_MASK);
}

static inline void
znp_notify_req(uint32_t flags, uint8_t request_id)
{
    /* Single completion signal: the znp task posts, the requester (app) task
     * consumes. request_id in the value lets the waiter reject stale posts. */
    zb_os_signal_set(g_znp_context.req_signal, znp_make_notify_value(flags, request_id));
}

static int
znp_queue_add(const s_zb_znp_req_t *req)
{
    if (g_znp_context.req_queue)
    {
        if (zb_os_queue_send(g_znp_context.req_queue, req, 1))
        {
            return ZB_OK;
        }
    }
    return ZB_FAIL;
}

/*
 * Wait on req_signal until a completion carrying @p request_id arrives, the
 * deadline passes, or the signal times out. Late posts from an already
 * abandoned request are ignored so they can't satisfy or fail this wait.
 */
static int
znp_wait_completion(uint8_t request_id, uint32_t wait_ms,
                    uint32_t success_notify_flags)
{
    uint32_t start_tick = zb_os_now_ms();
    uint32_t notify_value = 0;
    uint32_t elapsed_ms;

    while ((elapsed_ms = zb_os_now_ms() - start_tick) < wait_ms)
    {
        if (!zb_os_signal_wait(g_znp_context.req_signal, &notify_value,
                               wait_ms - elapsed_ms))
        {
            return ZNP_DEV_STATE_TIMEOUT;
        }
        if ((notify_value & ZNP_NOTIFY_REQ_ID_MASK) != request_id)
        {
            // Late completion from an old request. Ignore and keep waiting.
            continue;
        }
        return (notify_value & success_notify_flags) ? ZB_OK : ZB_FAIL;
    }
    return ZNP_DEV_STATE_TIMEOUT;
}

/*
 * Copy the staged response out into the requester's buffer. Called only after a
 * matching completion, i.e. while the requester frame is provably still alive.
 * Any tail the coprocessor did not supply is zeroed so callers never decode
 * uninitialised stack.
 */
static void
znp_copy_staged_response(uint8_t *dst, uint16_t dst_len)
{
    if (!dst || dst_len == 0)
    {
        return;
    }
    uint16_t copy_len = g_znp_context.resp_stage_len;
    if (copy_len > dst_len)
    {
        copy_len = dst_len;
    }
    memcpy(dst, g_znp_context.resp_stage, copy_len);
    if (copy_len < dst_len)
    {
        memset(dst + copy_len, 0, dst_len - copy_len);
    }
}

static int
znp_requester_send_sync(s_zb_znp_req_t *req, uint32_t success_notify_flags)
{
    int result;

    /* Serialize the whole issue+wait against every other requester: the context
     * and req_signal only track ONE outstanding request, so a concurrent caller
     * (e.g. a direct get_core_temperature/RS485 call while the Zigbee task is
     * doing a Set-Mode/reset) would clear this one's completion and make it
     * spuriously expire. The znp task never takes this mutex, so it keeps
     * posting completions and the holder always releases within its timeout. */
    zb_os_mutex_lock(g_znp_context.req_mutex, ZB_OSAL_WAIT_FOREVER);

    if (++g_znp_req_id_gen == 0)
    {
        g_znp_req_id_gen = 1;
    }
    req->request_id = g_znp_req_id_gen;
    req->start_tick = zb_os_now_ms();

    // Drain any stale pending notification before issuing a new request.
    zb_os_signal_clear(g_znp_context.req_signal);

    if (znp_queue_add(req) != ZB_OK)
    {
        ZB_LOGE(TAG, "%s(): Failed to add ZNP request to queue", __func__);
        /* Never queued, so the znp task will not free the request payload;
         * BUSY tells the caller it still owns that buffer. */
        result = ZNP_DEV_STATE_BUSY;
        goto out;
    }

    result = znp_wait_completion(req->request_id,
                                 req->timeout_ms + ZNP_REQ_WAIT_GUARD_MS,
                                 success_notify_flags);
    if (result == ZB_OK)
    {
        if (req->type == ZNP_CMD_REQ_TYPE_ZNP)
        {
            znp_copy_staged_response(req->znp.resp_data, req->znp.resp_len);
        }
        else
        {
            znp_copy_staged_response(req->sbl.resp_data, req->sbl.resp_len);
        }
    }
out:
    zb_os_mutex_unlock(g_znp_context.req_mutex);
    return result;
}

static inline void
znp_parser_reset(s_zb_znp_parser_t *parser)
{
    if (parser->buf)
    {
        ZB_MEM_FREE(parser->buf);
    }
    memset(parser, 0, sizeof(s_zb_znp_parser_t));
}

static inline void
znp_sbl_parser_reset(s_zb_znp_sbl_parser_t *parser)
{
    memset(parser, 0, sizeof(s_zb_znp_sbl_parser_t));
}

static inline void
znp_reset_pending_znp_req(void)
{
    g_znp_context.znp_waiting = false;
    g_znp_context.znp_req_id = 0;
    g_znp_context.znp_wants_response = false;
    /* Clear the correlation key too, otherwise a duplicate or late frame with
     * the same CMD would still match once the request is no longer pending. */
    g_znp_context.znp_cmd0 = 0;
    g_znp_context.znp_cmd1 = 0;
}

static inline void
znp_reset_pending_sbl_req(void)
{
    g_znp_context.sbl_waiting = false;
    g_znp_context.sbl_req_id = 0;
    g_znp_context.sbl_wants_response = false;
    g_znp_context.sbl_cmd = 0;
}

/* Stage a response payload for the requester to pick up after its completion. */
static void
znp_stage_response(const uint8_t *data, uint16_t len)
{
    if (len > sizeof(g_znp_context.resp_stage))
    {
        ZB_LOGW(TAG, "%s(): response truncated %d -> %d", __func__,
                (int)len, (int)sizeof(g_znp_context.resp_stage));
        len = sizeof(g_znp_context.resp_stage);
    }
    if (len)
    {
        memcpy(g_znp_context.resp_stage, data, len);
    }
    g_znp_context.resp_stage_len = len;
}

/*
 * Fail whatever request is currently in flight. Used when the link is torn down
 * underneath it (mode change / hardware reset) so its requester learns straight
 * away instead of sitting out the full timeout.
 */
static void
znp_cancel_pending_req(void)
{
    if (g_znp_context.znp_waiting)
    {
        if (g_znp_context.znp_wants_response && g_znp_context.znp_req_id != 0)
        {
            znp_notify_req(ZNP_NOTIFY_ERROR_BIT, g_znp_context.znp_req_id);
        }
        znp_reset_pending_znp_req();
    }
    if (g_znp_context.sbl_waiting)
    {
        if (g_znp_context.sbl_wants_response && g_znp_context.sbl_req_id != 0)
        {
            znp_notify_req(ZNP_NOTIFY_ERROR_BIT, g_znp_context.sbl_req_id);
        }
        znp_reset_pending_sbl_req();
    }
}

/*
 * Empty the request queue, releasing the payload each entry owns and failing
 * any requester still blocked on one. A plain queue reset would leak both.
 */
static void
znp_flush_request_queue(void)
{
    s_zb_znp_req_t req;
    while (zb_os_queue_recv(g_znp_context.req_queue, &req, ZB_OSAL_NO_WAIT))
    {
        if (req.type == ZNP_CMD_REQ_TYPE_ZNP && req.znp.payload)
        {
            ZB_MEM_FREE(req.znp.payload);
        }
        else if (req.type == ZNP_CMD_REQ_TYPE_SBL && req.sbl.data)
        {
            ZB_MEM_FREE(req.sbl.data);
        }
        /* request_id 0 marks a fire-and-forget request: nobody is waiting. */
        if (req.request_id != 0)
        {
            znp_notify_req(ZNP_NOTIFY_ERROR_BIT, req.request_id);
        }
    }
}

static inline uint8_t
znp_calc_fcs(uint8_t fcs, const uint8_t *data, uint16_t len)
{
    while (len--)
    {
        fcs ^= *data++;
    }
    return fcs;
}

static bool
znp_parse_frame(s_zb_znp_parser_t *parser, uint8_t data)
{
    switch (parser->state)
    {
        case ZNP_STATE_SOF:
            if (data == ZNP_MT_SOF)
            {
                parser->state = ZNP_STATE_LENGTH;
                parser->fcs = 0;
            }
            return false;
        case ZNP_STATE_LENGTH:
            parser->buf_len = data + ZNP_MT_CMD_FIELD_LEN;
            parser->pos = 0;
            parser->fcs = znp_calc_fcs(parser->fcs, &data, 1);
            parser->buf = (uint8_t *)ZB_MEM_MALLOC(parser->buf_len);
            if (!parser->buf)
            {
                ZB_LOGE(TAG, "%s(): Mem alloc failed (1) len %d", __func__, (int)parser->buf_len);
            }
            parser->state = ZNP_STATE_CMD0;
            return false;
        case ZNP_STATE_CMD0:
            if (!parser->buf)
            {
                parser->buf = (uint8_t *)ZB_MEM_MALLOC(parser->buf_len);
                if (!parser->buf)
                {
                    ZB_LOGE(TAG, "%s(): Mem alloc failed (2) len %d", __func__, (int)parser->buf_len);
                }
            }
            if (parser->buf)
            {
                parser->buf[parser->pos++] = data;
            }
            parser->fcs = znp_calc_fcs(parser->fcs, &data, 1);
            parser->state = ZNP_STATE_CMD1;
            return false;
        case ZNP_STATE_CMD1:
            if (parser->buf)
            {
                parser->buf[parser->pos++] = data;
            }
            parser->fcs = znp_calc_fcs(parser->fcs, &data, 1);
            /* A zero-length-payload frame (LEN==0) is already complete after
             * CMD1; go straight to FCS. Otherwise the FCS byte would be written
             * past the end of buf and consumed as payload. */
            parser->state = (parser->pos >= parser->buf_len)
                                ? ZNP_STATE_FCS
                                : ZNP_STATE_PAYLOAD;
            return false;
        case ZNP_STATE_PAYLOAD:
            if (parser->buf)
            {
                parser->buf[parser->pos++] = data;
            }
            parser->fcs = znp_calc_fcs(parser->fcs, &data, 1);
            if (parser->pos >= parser->buf_len)
            {
                parser->state = ZNP_STATE_FCS;
            }
            return false;
        case ZNP_STATE_FCS:
            if (parser->fcs == data)
            {
                if (!parser->buf)
                {
                    ZB_LOGW(TAG, "%s(): Valid frame, no mem", __func__);
                    znp_parser_reset(parser);
                    break;
                }
                uint8_t cmd0 = parser->buf[0];
                uint8_t cmd1 = parser->buf[1];
                if ((cmd0 & ZNP_MT_CMD_TYPE_MASK) == ZNP_MT_CMD_SRSP)
                {
                    if (g_znp_context.znp_waiting
                        && g_znp_context.znp_cmd0 == (cmd0 & ZNP_MT_SUBSYSTEM_MASK)
                        && g_znp_context.znp_cmd1 == cmd1)
                    {
                        ZB_LOGI(TAG, "SRSP received: CMD: %02X%02X", cmd0, cmd1);
                        ZB_LOG_BUFFER_HEX(TAG, parser->buf, parser->buf_len);
                        znp_stage_response(parser->buf + ZNP_MT_CMD_FIELD_LEN,
                                           parser->buf_len - ZNP_MT_CMD_FIELD_LEN);
                        if (g_znp_context.znp_wants_response)
                        {
                            ZB_LOGD(TAG, "Notifying SRSP CMD: %02X%02X", cmd0, cmd1);
                            znp_notify_req(ZNP_NOTIFY_ZNP_SRSP_BIT, g_znp_context.znp_req_id);
                        }
                        znp_reset_pending_znp_req();
                    }
                    else
                    {
                        ZB_LOGW(TAG, "Unexpected SRSP received: CMD: %02X%02X", cmd0, cmd1);
                    }
                }
                else
                {
                    if (g_znp_context.znp_waiting
                        && g_znp_context.znp_cmd0 == (cmd0 & ZNP_MT_SUBSYSTEM_MASK)
                        && g_znp_context.znp_cmd1 == cmd1
                        && g_znp_context.znp_wants_response)
                    {
                        ZB_LOGD(TAG, "Notifying AREQ CMD: %02X%02X", cmd0, cmd1);
                        znp_notify_req(ZNP_NOTIFY_CHIP_REBOOT_BIT, g_znp_context.znp_req_id);
                        znp_reset_pending_znp_req();
                    }
                    ZB_LOGI(TAG, "ARSP received: CMD: %02X%02X", parser->buf[0], parser->buf[1]);
                    ZB_LOG_BUFFER_HEX(TAG, parser->buf, parser->buf_len);
                    znp_process_incoming_data(parser->buf, parser->buf_len);
                }
            }
            else
            {
                ZB_LOGE(TAG, "FCS error recv %x: calc %x", data, parser->fcs);
            }
            znp_parser_reset(parser);
            break;
    }
    return false;
}

static void
znp_sbl_send_sync(s_zb_znp_mode_req_t *mode_req)
{
    uint8_t sync[SBL_COMMAND_SYNC_LEN] = {SBL_COMMAND_SYNC, SBL_COMMAND_SYNC};
    zb_plat_serial_write(sync, SBL_COMMAND_SYNC_LEN);
    g_znp_context.sbl_waiting = true;
    g_znp_context.sbl_cmd = SBL_COMMAND_SYNC;
    g_znp_context.sbl_wants_response = mode_req->wants_response;
}

static void
znp_sbl_send_ack(void)
{
    uint8_t ack[SBL_COMMAND_ACK_LEN] = {0, SBL_COMMAND_ACK};
    zb_plat_serial_write(ack, SBL_COMMAND_ACK_LEN);
}

static void
znp_sbl_send_nack(void)
{
    uint8_t nack[SBL_COMMAND_NACK_LEN] = {0, SBL_COMMAND_NACK};
    zb_plat_serial_write(nack, SBL_COMMAND_NACK_LEN);
}

static bool
znp_sbl_parse_frame(s_zb_znp_sbl_parser_t *parser, uint8_t data, s_zb_znp_sbl_event_t *event)
{
    event->type = ZNP_SBL_EVT_NONE;
    switch (parser->state)
    {
        case ZNP_SBL_STATE_WAIT_LEN:
            if (data == 0x00)
            {
                parser->state = ZNP_SBL_STATE_WAIT_ACK_NACK;
                return false;
            }
            /* data is the wire length byte; the payload it announces must fit
             * the fixed parser buffer or the reads below run off the end. */
            if (data < 3 || (uint8_t)(data - 2) > SBL_MAX_RX_BUFFER_SIZE)
            {
                znp_sbl_parser_reset(parser);
                return false;
            }
            parser->buf_len = data - 2;
            parser->pos = 0;
            parser->state = ZNP_SBL_STATE_READ_CHECKSUM;
            return false;
        case ZNP_SBL_STATE_READ_CHECKSUM:
            parser->checksum = data;
            parser->state = ZNP_SBL_STATE_READ_DATA;
            return false;
        case ZNP_SBL_STATE_READ_DATA:
            parser->buf[parser->pos++] = data;
            if (parser->pos == parser->buf_len)
            {
                if (zb_znp_sbl_cal_crc(parser->buf, parser->buf_len) == parser->checksum)
                {
                    event->type = ZNP_SBL_EVT_DATA;
                    memcpy(event->data, parser->buf, parser->buf_len);
                    event->data_len = parser->buf_len;
                    znp_sbl_parser_reset(parser);
                    znp_sbl_send_ack();
                    return event->type != ZNP_SBL_EVT_NONE;
                }
                else
                {
                    znp_sbl_parser_reset(parser);
                    znp_sbl_send_nack();
                    return false;
                }
            }
            return false;
        case ZNP_SBL_STATE_WAIT_ACK_NACK:
            if (data == SBL_COMMAND_ACK)
            {
                event->type = ZNP_SBL_EVT_ACK;
            }
            if (data == SBL_COMMAND_NACK)
            {
                event->type = ZNP_SBL_EVT_NACK;
            }
            znp_sbl_parser_reset(parser);
            return event->type != ZNP_SBL_EVT_NONE;
        default:
            znp_sbl_parser_reset(parser);
            return false;
    }
}

static void
znp_serial_rx_cb(const uint8_t *data, uint16_t length)
{
    size_t sent = zb_os_stream_send(g_znp_context.recv_buffer, data, length, ZB_OSAL_NO_WAIT);
    if (sent != length)
    {
        ZB_LOGE(TAG, "Failed to send data to message buffer %d", length);
    }
}

static void
znp_destroy_rtos_objects(void)
{
    if (g_znp_context.req_queue)
    {
        zb_os_queue_delete(g_znp_context.req_queue);
        g_znp_context.req_queue = NULL;
    }
    if (g_znp_context.mode_req_queue)
    {
        zb_os_queue_delete(g_znp_context.mode_req_queue);
        g_znp_context.mode_req_queue = NULL;
    }
    if (g_znp_context.recv_buffer)
    {
        zb_os_stream_delete(g_znp_context.recv_buffer);
        g_znp_context.recv_buffer = NULL;
    }
    if (g_znp_context.req_signal)
    {
        zb_os_signal_delete(g_znp_context.req_signal);
        g_znp_context.req_signal = NULL;
    }
    if (g_znp_context.req_mutex)
    {
        zb_os_mutex_delete(g_znp_context.req_mutex);
        g_znp_context.req_mutex = NULL;
    }
}

zb_status_t
zb_znp_init(const s_zb_znp_config_t *cfg)
{
    if (!cfg || !cfg->pfn_set_reset_pin || !cfg->pfn_set_sbl_pin)
    {
        ZB_LOGE(TAG, "%s(): invalid config", __func__);
        return ZB_INVALID_PARAMETER;
    }
    if (g_znp_initialised)
    {
        ZB_LOGW(TAG, "%s(): already initialised", __func__);
        return ZB_OK;
    }

    // queues for passing ZNP/mode requests from application thread to UART thread
    memset(&g_znp_context, 0, sizeof(g_znp_context));

    uint16_t rx_size = cfg->rx_stream_size ? cfg->rx_stream_size : ZIGBEE_ZNP_MSG_BUFFER_SIZE;
    g_znp_context.req_queue = zb_os_queue_create(ZIGBEE_ZNP_QUEUE_SIZE, sizeof(s_zb_znp_req_t));
    g_znp_context.mode_req_queue = zb_os_queue_create(ZIGBEE_ZNP_MODE_QUEUE_SIZE, sizeof(s_zb_znp_mode_req_t));
    g_znp_context.req_signal = zb_os_signal_create();
    g_znp_context.req_mutex = zb_os_mutex_create(false);
    g_znp_context.recv_buffer = zb_os_stream_create(rx_size);
    if (!g_znp_context.req_queue || !g_znp_context.mode_req_queue ||
        !g_znp_context.recv_buffer || !g_znp_context.req_signal || !g_znp_context.req_mutex)
    {
        ZB_LOGE(TAG, "%s(): failed to allocate RTOS objects", __func__);
        znp_destroy_rtos_objects();
        return ZB_MEM_ERROR;
    }

    g_znp_context.mode = ZNP_MODE_ZNP;
    g_znp_context.pfn_set_reset_pin_cb = cfg->pfn_set_reset_pin;
    g_znp_context.pfn_set_sbl_pin_cb = cfg->pfn_set_sbl_pin;

    // Open the serial transport. The platform backend owns the UART framing,
    // RX buffering and RX-line pull-up; we only hand it the board wiring and
    // the byte sink that feeds the receive stream.
    s_zb_serial_cfg_t serial_cfg =
    {
        .port = cfg->uart_port,
        .baud = cfg->uart_baud,
        .tx_pin = cfg->uart_tx_pin,
        .rx_pin = cfg->uart_rx_pin,
        .rts_pin = cfg->uart_rts_pin,
        .cts_pin = cfg->uart_cts_pin,
    };
    zb_plat_serial_open(&serial_cfg, znp_serial_rx_cb);

    g_znp_initialised = true;

    // Optionally bring the coprocessor up in a known state (normal/ZNP mode) so
    // host and ZNP start from a defined point regardless of prior pin state.
    if (cfg->reset_on_init)
    {
        znp_full_reset_state(false);
    }

    return ZB_OK;
}

void
zb_znp_deinit(void)
{
    if (!g_znp_initialised)
    {
        return;
    }

    zb_plat_serial_close();

    /* Release the payloads still owned by the queue and fail anyone waiting on
     * them before the RTOS objects they live in go away. */
    znp_cancel_pending_req();
    znp_flush_request_queue();

    znp_parser_reset(&g_znp_context.znp_parser);
    znp_destroy_rtos_objects();

    memset(&g_znp_context, 0, sizeof(g_znp_context));
    g_znp_initialised = false;
}

static void
znp_process_incoming_data(const uint8_t *data, uint16_t len)
{
    switch (data[0] & ZNP_MT_SUBSYSTEM_MASK)
    {
        case ZNP_MT_SYS_AF:
            zb_znp_mt_af_process(data, len);
            break;
        case ZNP_MT_SYS_ZDO:
            zb_znp_mt_zdo_process(data, len);
            break;
        case ZNP_MT_SYS_SYS:
            zb_znp_mt_sys_process(data, len);
            break;
        case ZNP_MT_SYS_UTIL:
            zb_znp_mt_util_process(data, len);
            break;
        case ZNP_MT_SYS_APP_CFG:
            zb_znp_mt_app_cfg_process(data, len);
            break;
        case ZNP_MT_SYS_APP:
            zb_znp_mt_app_process(data, len);
            break;
        default:
            ZB_LOGW(TAG, "%s(): not handled CMD: %02X%02X", __func__, data[0], data[1]);
            break;
    }
}

static int
znp_build_frame(s_zb_znp_req_t *req, s_zb_znp_frame_t *frame)
{
    if (!req || !frame)
    {
        ZB_LOGE(TAG, "Invalid parameters");
        return ZB_INVALID_PARAMETER;
    }
    /* payload_len doubles as the wire length byte and, once the frame is built,
     * holds the frame length — so anything over the MT maximum would wrap the
     * uint8_t field and put a bogus length on the wire. */
    if (req->znp.payload_len > ZNP_MT_DATA_MAX_LEN ||
        (req->znp.payload_len > 0 && !req->znp.payload))
    {
        ZB_LOGE(TAG, "%s(): invalid payload len %d", __func__, (int)req->znp.payload_len);
        return ZB_INVALID_PARAMETER;
    }
    frame->buf_len = ZNP_MT_UART_HDR_LEN + req->znp.payload_len + ZNP_MT_UART_FCS_LEN;
    frame->buf = (uint8_t *)ZB_MEM_MALLOC(frame->buf_len);
    if (!frame->buf)
    {
        ZB_LOGE(TAG, "%s(): Mem alloc failed len %d", __func__, (int)frame->buf_len);
        return ZB_MEM_ERROR;
    }
    frame->buf[0] = ZNP_MT_SOF;
    frame->buf[1] = req->znp.payload_len;
    frame->buf[2] = req->znp.cmd0;
    frame->buf[3] = req->znp.cmd1;
    if (req->znp.payload_len)
    {
        memcpy(&frame->buf[ZNP_MT_UART_HDR_LEN], req->znp.payload, req->znp.payload_len);
    }
    frame->buf[ZNP_MT_UART_HDR_LEN + req->znp.payload_len] =
        znp_calc_fcs(0, &frame->buf[ZNP_MT_UART_FRAME_START_IDX],
                                      ZNP_MT_HDR_LEN + req->znp.payload_len);
    return ZB_OK;
}

int
zb_znp_send_cmd_req(s_zb_znp_req_t *req)
{
    int status = ZB_OK;
    if (!req)
    {
        return ZB_INVALID_PARAMETER;
    }
    if (req->type == ZNP_CMD_REQ_TYPE_ZNP)
    {
        s_zb_znp_frame_t frame;
        if (znp_build_frame(req, &frame) != ZB_OK)
        {
            return ZB_FAIL;
        }

        // Update request's payload with the znp frame
        req->znp.payload = frame.buf;
        req->znp.payload_len = frame.buf_len;

        ZB_LOGD(TAG, "ZNP request queued: CMD: %02X%02X", req->znp.cmd0, req->znp.cmd1);
        if ((req->znp.cmd0 & ZNP_MT_CMD_TYPE_MASK) == ZNP_MT_CMD_SREQ)
        {
            ZB_LOGD(TAG, "Waiting for ZNP SRSP, CMD: %02X%02X", req->znp.cmd0, req->znp.cmd1);
            status = znp_requester_send_sync(req, ZNP_NOTIFY_ZNP_SRSP_BIT);
            /* BUSY = never queued, so the znp task will not free the frame. */
            if (status == ZNP_DEV_STATE_BUSY)
            {
                ZB_MEM_FREE(frame.buf);
                req->znp.payload = NULL;
            }
            return (status == ZB_OK) ? ZB_OK : ZB_FAIL;
        }
        else if ((req->znp.cmd0 & ZNP_MT_SUBSYSTEM_MASK) == ZNP_MT_SYS_SYS && req->znp.cmd1 == ZNP_SYS_RESET_REQ)
        {
            status = znp_requester_send_sync(req, ZNP_NOTIFY_CHIP_REBOOT_BIT);
            if (status == ZNP_DEV_STATE_BUSY)
            {
                ZB_MEM_FREE(frame.buf);
                req->znp.payload = NULL;
                return ZB_FAIL;
            }
            return (status == ZB_OK) ? ZB_OK : ZB_FAIL;
        }
        req->request_id = 0;
        req->start_tick = zb_os_now_ms();
        if (znp_queue_add(req) != ZB_OK)
        {
            ZB_LOGE(TAG, "Failed to add znp request to queue");
            ZB_MEM_FREE(frame.buf);
            return ZB_FAIL;
        }
    }
    else if (req->type == ZNP_CMD_REQ_TYPE_SBL)
    {
        uint8_t *data = (uint8_t *)ZB_MEM_MALLOC(req->sbl.data_len);
        if (!data)
        {
            ZB_LOGE(TAG, "Failed to allocate memory for SBL data");
            return ZB_FAIL;
        }
        memcpy(data, req->sbl.data, req->sbl.data_len);
        req->sbl.data = data;
        if (req->wants_response)
        {
            status = znp_requester_send_sync(req, (ZNP_NOTIFY_SBL_ACK_BIT | ZNP_NOTIFY_SBL_DATA_BIT));
            if (status == ZNP_DEV_STATE_BUSY)
            {
                ZB_MEM_FREE(data);
                req->sbl.data = NULL;
            }
            return (status == ZB_OK) ? ZB_OK : ZB_FAIL;
        }
        req->request_id = 0;
        req->start_tick = zb_os_now_ms();
        if (znp_queue_add(req) != ZB_OK)
        {
            ZB_LOGE(TAG, "Failed to add sbl request to queue");
            ZB_MEM_FREE(data);
            req->sbl.data = NULL;
            return ZB_FAIL;
        }
    }

    return status;
}

static int
znp_send_mode_req(s_zb_znp_mode_req_t *mode_req)
{
    int result = ZB_OK;

    /* Same single-outstanding-request serialization as znp_requester_send_sync:
     * a mode/reset shares req_signal with the SREQ path, so hold req_mutex for
     * the whole issue+wait to keep a concurrent SREQ from clearing our
     * completion (and vice versa). */
    zb_os_mutex_lock(g_znp_context.req_mutex, ZB_OSAL_WAIT_FOREVER);

    /* A mode change tears down whatever the znp task had in flight, and that
     * teardown posts a completion for the abandoned request. Tag this request
     * so its waiter can tell that post apart from its own. */
    if (++g_znp_req_id_gen == 0)
    {
        g_znp_req_id_gen = 1;
    }
    mode_req->request_id = g_znp_req_id_gen;

    /* Drop any mode request left behind by a requester that gave up (the znp
     * task was starved past its timeout). Holding req_mutex means no other
     * requester can be waiting on one, so anything still queued is abandoned.
     * Without this the depth-1 queue stays full and every later mode change
     * fails to enqueue while the znp task eventually acts on the stale one. */
    s_zb_znp_mode_req_t abandoned;
    while (zb_os_queue_recv(g_znp_context.mode_req_queue, &abandoned, ZB_OSAL_NO_WAIT))
    {
        ZB_LOGW(TAG, "%s(): dropping abandoned mode request (id %u)", __func__,
                (unsigned)abandoned.request_id);
    }

    /* Drain any stale completion before queuing: the req_signal is shared with
     * the SREQ path, so clear it so a leftover value can't satisfy this wait
     * early. Safe against lost wakeups — the znp task only posts after it
     * dequeues this request, which happens after the send below. */
    if (mode_req->wants_response)
    {
        zb_os_signal_clear(g_znp_context.req_signal);
    }

    if (!zb_os_queue_send(g_znp_context.mode_req_queue, mode_req, 1))
    {
        ZB_LOGE(TAG, "Failed to send mode request to queue");
        result = ZB_FAIL;
        goto out;
    }

    if (mode_req->wants_response)
    {
        uint32_t success_bit = (mode_req->mode == ZNP_MODE_SBL)
                                   ? ZNP_NOTIFY_SBL_ACK_BIT
                                   : ZNP_NOTIFY_CHIP_REBOOT_BIT;
        result = znp_wait_completion(mode_req->request_id,
                                     mode_req->timeout_ms + ZNP_REQ_WAIT_GUARD_MS,
                                     success_bit);
        result = (result == ZB_OK) ? ZB_OK : ZB_FAIL;
        goto out;
    }
    result = ZB_OK;
out:
    zb_os_mutex_unlock(g_znp_context.req_mutex);
    return result;
}

int zb_znp_set_mode_znp(uint32_t timeout_ms)
{
    s_zb_znp_mode_req_t mode_req;
    memset(&mode_req, 0, sizeof(mode_req));
    mode_req.mode = ZNP_MODE_ZNP;
    mode_req.wants_response = true;
    mode_req.timeout_ms = timeout_ms;
    return znp_send_mode_req(&mode_req);
}

int zb_znp_set_mode_sbl(uint32_t timeout_ms)
{
    s_zb_znp_mode_req_t mode_req;
    memset(&mode_req, 0, sizeof(mode_req));
    mode_req.mode = ZNP_MODE_SBL;
    mode_req.wants_response = true;
    mode_req.timeout_ms = timeout_ms;
    return znp_send_mode_req(&mode_req);
}

static void
znp_full_reset_state(bool enter_bootloader)
{
    ZB_LOGI(TAG, "Full reset state, enter bootloader: %d", enter_bootloader);

    /* The coprocessor is about to reboot, so nothing in flight or queued can
     * still complete. Fail both explicitly: the queue owns heap-allocated
     * payloads, and a requester blocked on one would otherwise stall for its
     * whole timeout waiting for a response that can never arrive. */
    znp_cancel_pending_req();
    znp_flush_request_queue();

    zb_plat_serial_flush_rx();

    if (g_znp_context.pfn_set_sbl_pin_cb)
    {
        g_znp_context.pfn_set_sbl_pin_cb(enter_bootloader ? 0 : 1);
    }
    if (g_znp_context.pfn_set_reset_pin_cb)
    {
        g_znp_context.pfn_set_reset_pin_cb(0);
        zb_plat_delay_us(ZNP_RESET_ASSERT_US);
        g_znp_context.pfn_set_reset_pin_cb(1);
        zb_plat_delay_us(ZNP_RESET_SETTLE_US);
    }

    /* Discard the whole receive path AFTER the pulse, not before. The RX
     * callback runs on the UART task, which outranks this one, so it keeps
     * feeding recv_buffer with the noise the coprocessor emits while it browns
     * out and reboots. Clearing only the UART ring beforehand would leave that
     * noise staged in recv_buffer, where the next mode's parser would consume
     * it — typically eating the SBL SYNC ACK or the SYS_RESET_IND that the
     * mode change is waiting for. */
    zb_plat_serial_flush_rx();
    if (!zb_os_stream_reset(g_znp_context.recv_buffer))
    {
        ZB_LOGE(TAG, "Failed to reset stream buffer");
    }
    znp_parser_reset(&g_znp_context.znp_parser);
    znp_sbl_parser_reset(&g_znp_context.sbl_parser);
#if PROD_TEST
    printf("<ZB result=\"ZIGBEE Reset: Bootloader mode = %s\">PASS</ZB>\r\n", enter_bootloader ? "true" : "false");
#endif
}

static void
znp_process_mode_change(void)
{
    s_zb_znp_mode_req_t mode_req;
    if (!zb_os_queue_recv(g_znp_context.mode_req_queue, &mode_req, ZB_OSAL_NO_WAIT))
    {
        return;
    }
    bool enter_bootloader = mode_req.mode == ZNP_MODE_SBL;

    znp_full_reset_state(enter_bootloader);

    if (enter_bootloader)
    {
        g_znp_context.mode = ZNP_MODE_SBL;
        g_znp_context.sbl_waiting = true;
        g_znp_context.sbl_wants_response = mode_req.wants_response;
        g_znp_context.sbl_req_id = mode_req.request_id;
        g_znp_context.req_tick = zb_os_now_ms();
        g_znp_context.req_timeout_ms = mode_req.timeout_ms;
        znp_sbl_send_sync(&mode_req);
    }
    else
    {
        g_znp_context.mode = ZNP_MODE_ZNP;
        g_znp_context.znp_waiting = true;
        g_znp_context.znp_cmd0 = ZNP_MT_SYS_SYS;
        g_znp_context.znp_cmd1 = ZNP_SYS_RESET_IND;
        g_znp_context.znp_wants_response = mode_req.wants_response;
        g_znp_context.znp_req_id = mode_req.request_id;
        g_znp_context.req_tick = zb_os_now_ms();
        g_znp_context.req_timeout_ms = mode_req.timeout_ms;
    }
}

static void
znp_process_znp_req(void)
{
    s_zb_znp_req_t req;
    if (g_znp_context.sbl_waiting || g_znp_context.znp_waiting)
    {
        if (zb_os_now_ms() - g_znp_context.req_tick < g_znp_context.req_timeout_ms)
        {
            // Check the first item in the queue and remove if expired
            if (zb_os_queue_peek(g_znp_context.req_queue, &req, ZB_OSAL_NO_WAIT))
            {
                if (zb_os_now_ms() - req.start_tick > req.timeout_ms)
                {
                    ZB_LOGW(TAG, "First request expired, CMD: %02X%02X", req.znp.cmd0, req.znp.cmd1);
                    memset(&req, 0, sizeof(req));
                    if (zb_os_queue_recv(g_znp_context.req_queue, &req, ZB_OSAL_NO_WAIT))
                    {
                        znp_notify_req(ZNP_NOTIFY_ERROR_BIT, req.request_id);
                        goto exit;
                    }
                    return;
                }
            }
            return;
        }
        else if (g_znp_context.znp_waiting)
        {
            ZB_LOGI(TAG, "ZNP timeout");
            znp_notify_req(ZNP_NOTIFY_ERROR_BIT, g_znp_context.znp_req_id);
            znp_reset_pending_znp_req();
        }
        else if (g_znp_context.sbl_waiting)
        {
            ZB_LOGI(TAG, "SBL timeout");
            znp_notify_req(ZNP_NOTIFY_ERROR_BIT, g_znp_context.sbl_req_id);
            znp_reset_pending_sbl_req();
        }
    }
    if (!zb_os_queue_recv(g_znp_context.req_queue, &req, ZB_OSAL_NO_WAIT))
    {
        return;
    }

    // Check current mode and request type
    if (g_znp_context.mode == ZNP_MODE_ZNP && req.type != ZNP_CMD_REQ_TYPE_ZNP)
    {
        ZB_LOGI(TAG, "ZNP wrong mode");
        znp_notify_req(ZNP_NOTIFY_ERROR_BIT, req.request_id);
        goto exit;
    }
    if (g_znp_context.mode == ZNP_MODE_SBL && req.type != ZNP_CMD_REQ_TYPE_SBL)
    {
        ZB_LOGI(TAG, "SBL wrong mode");
        znp_notify_req(ZNP_NOTIFY_ERROR_BIT, req.request_id);
        goto exit;
    }

    // ZNP request
    if (req.type == ZNP_CMD_REQ_TYPE_ZNP)
    {
        if (((req.znp.cmd0 & ZNP_MT_CMD_TYPE_MASK) == ZNP_MT_CMD_SREQ) && (zb_os_now_ms() - req.start_tick > req.timeout_ms))
        {
            ZB_LOGW(TAG, "Request expired, CMD: %02X%02X", req.znp.cmd0, req.znp.cmd1);
            znp_notify_req(ZNP_NOTIFY_ERROR_BIT, req.request_id);
            goto exit;
        }
        if (zb_plat_serial_write(req.znp.payload, req.znp.payload_len) != req.znp.payload_len)
        {
            ZB_LOGE(TAG, "Failed to send ZNP request: CMD: %02X%02X", req.znp.cmd0, req.znp.cmd1);
            znp_notify_req(ZNP_NOTIFY_ERROR_BIT, req.request_id);
            goto exit;
        }
        ZB_LOGI(TAG, "Sent ZNP request: CMD: %02X%02X", req.znp.cmd0, req.znp.cmd1);
        ZB_LOG_BUFFER_HEX(TAG, req.znp.payload, req.znp.payload_len);
        // If SREQ, wait for SRSP
        if ((req.znp.cmd0 & ZNP_MT_CMD_TYPE_MASK) == ZNP_MT_CMD_SREQ)
        {
            ZB_LOGD(TAG, "Sending SREQ, CMD: %02X%02X, waiting for SRSP", req.znp.cmd0, req.znp.cmd1);
            g_znp_context.znp_waiting = true;
            g_znp_context.znp_cmd0 = req.znp.cmd0 & ZNP_MT_SUBSYSTEM_MASK;
            g_znp_context.znp_cmd1 = req.znp.cmd1;
            g_znp_context.znp_wants_response = req.wants_response;
            g_znp_context.znp_req_id = req.request_id;
            g_znp_context.req_tick = req.start_tick;
            g_znp_context.req_timeout_ms = req.timeout_ms;
        }
        else if ((req.znp.cmd0 & ZNP_MT_CMD_TYPE_MASK) == ZNP_MT_CMD_AREQ && (req.znp.cmd1 == ZNP_SYS_RESET_REQ))
        {
            ZB_LOGD(TAG, "Sending AREQ, CMD: %02X%02X, waiting for AIND", req.znp.cmd0, req.znp.cmd1);
            g_znp_context.znp_waiting = true;
            g_znp_context.znp_cmd0 = req.znp.cmd0 & ZNP_MT_SUBSYSTEM_MASK;
            g_znp_context.znp_cmd1 = ZNP_SYS_RESET_IND;
            g_znp_context.znp_wants_response = req.wants_response;
            g_znp_context.znp_req_id = req.request_id;
            g_znp_context.req_tick = req.start_tick;
            g_znp_context.req_timeout_ms = req.timeout_ms;
        }
    }
    // SBL request
    else if (req.type == ZNP_CMD_REQ_TYPE_SBL)
    {
        if (zb_os_now_ms() - req.start_tick > req.timeout_ms)
        {
            ZB_LOGW(TAG, "SBL request expired, CMD: %02X", req.sbl.cmd);
            znp_notify_req(ZNP_NOTIFY_ERROR_BIT, req.request_id);
            goto exit;
        }
        ZB_LOGD(TAG, "Sending SBL request: CMD: %02X", req.sbl.cmd);
        zb_plat_serial_write(req.sbl.data, req.sbl.data_len);
        g_znp_context.sbl_waiting = true;
        g_znp_context.sbl_cmd = req.sbl.cmd;
        g_znp_context.sbl_wants_response = req.wants_response;
        g_znp_context.sbl_req_id = req.request_id;
        g_znp_context.req_tick = req.start_tick;
        g_znp_context.req_timeout_ms = req.timeout_ms;
    }
exit:
    if (req.type == ZNP_CMD_REQ_TYPE_ZNP && req.znp.payload)
    {
        ZB_MEM_FREE(req.znp.payload);
        req.znp.payload = NULL;
    }
    if (req.type == ZNP_CMD_REQ_TYPE_SBL && req.sbl.data)
    {
        ZB_MEM_FREE(req.sbl.data);
        req.sbl.data = NULL;
    }
}

void
zb_znp_task(void)
{
    uint8_t parse_buffer[ZIGBEE_ZNP_MSG_BUFFER_SIZE];

    /* The task loop outlives zb_znp_deinit() (the app task tears the driver
     * down on its way out), so re-check before touching RTOS objects that
     * deinit has already destroyed. */
    if (!g_znp_initialised)
    {
        zb_os_delay_ms(10);
        return;
    }

    znp_process_mode_change();
    znp_process_znp_req();
    // Process the message buffer
    size_t received = zb_os_stream_recv(g_znp_context.recv_buffer, parse_buffer, ZIGBEE_ZNP_MSG_BUFFER_SIZE, 1);
    if (received > 0)
    {
        for (uint16_t i = 0; i < received; i++)
        {
            if (g_znp_context.mode == ZNP_MODE_ZNP)
            {
                znp_parse_frame(&g_znp_context.znp_parser, parse_buffer[i]);
            }
            else if (g_znp_context.mode == ZNP_MODE_SBL)
            {
                s_zb_znp_sbl_event_t event;
                memset(&event, 0, sizeof(event));
                znp_sbl_parse_frame(&g_znp_context.sbl_parser, parse_buffer[i], &event);
                if (event.type == ZNP_SBL_EVT_NACK)
                {
                    ZB_LOG_BUFFER_HEX(TAG, parse_buffer, received);
                    if (g_znp_context.sbl_waiting && g_znp_context.sbl_wants_response)
                    {
                        znp_notify_req(ZNP_NOTIFY_SBL_NACK_BIT, g_znp_context.sbl_req_id);
                        znp_reset_pending_sbl_req();
                    }
                }
                else if (event.type == ZNP_SBL_EVT_ACK)
                {
                    // For status and crc32 commands, wait for the next response data
                    if (g_znp_context.sbl_cmd != SBL_COMMAND_GET_STATUS && g_znp_context.sbl_cmd != SBL_COMMAND_CRC32)
                    {
                        if (g_znp_context.sbl_waiting && g_znp_context.sbl_wants_response)
                        {
                            znp_notify_req(ZNP_NOTIFY_SBL_ACK_BIT, g_znp_context.sbl_req_id);
                            znp_reset_pending_sbl_req();
                        }
                    }
                }
                else if (event.type == ZNP_SBL_EVT_DATA)
                {
                    znp_stage_response(event.data, event.data_len);
                    if (g_znp_context.sbl_waiting && g_znp_context.sbl_wants_response)
                    {
                        znp_notify_req(ZNP_NOTIFY_SBL_DATA_BIT, g_znp_context.sbl_req_id);
                        znp_reset_pending_sbl_req();
                    }
                }
            }
        }
    }
}
