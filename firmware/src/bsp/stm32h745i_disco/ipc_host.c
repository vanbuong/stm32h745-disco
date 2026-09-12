#include "bsp/board.h"

#include "cube.h"
#include "ipc/ipc.h"
#include "svc/audio.h"

#include <string.h>

static ipc_link_t s_link;
static uint8_t s_ok;
static uint8_t s_ping_sent;

static void kick_m4(void)
{
    board_hsem_notify(BOARD_HSEM_M7_TO_M4);
}

static void dwt_enable(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0u;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void put_u32(uint32_t v)
{
    char buf[11];
    int i = 10;

    buf[10] = '\0';
    if (v == 0u) {
        board_console_puts("0");
        return;
    }
    while (v > 0u && i > 0) {
        buf[--i] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    board_console_puts(&buf[i]);
}

static int log_text_ok(const uint8_t *pl, uint16_t n)
{
    uint16_t i;

    if (pl == NULL || n == 0u) {
        return 0;
    }
    for (i = 0u; i < n; i++) {
        uint8_t c = pl[i];
        if (c == '\0') {
            return 1;
        }
        if ((c < 32u && c != '\t' && c != '\n' && c != '\r') || (c > 126u)) {
            return 0;
        }
    }
    return 1;
}

static void print_log(const uint8_t *pl, uint16_t len)
{
    char line[IPC_PAYLOAD_MAX + 1u];
    uint16_t n = len;

    if (n > IPC_PAYLOAD_MAX) {
        n = IPC_PAYLOAD_MAX;
    }
    if (!log_text_ok(pl, n)) {
        return;
    }
    memcpy(line, pl, n);
    line[n] = '\0';
    board_console_puts("m4: ");
    board_console_puts(line);
    board_console_puts("\r\n");
}

static void log_pong(const uint8_t *pl, uint16_t len)
{
    uint32_t t0;
    uint32_t dt;
    uint32_t us;
    uint32_t hz;

    if (len < 4u || pl == NULL) {
        return;
    }
    memcpy(&t0, pl, sizeof(t0));
    dt = DWT->CYCCNT - t0;
    hz = board_sysclk_hz();
    if (hz < 1000000u) {
        hz = 400000000u;
    }
    us = dt / (hz / 1000000u);
    board_console_puts("ipc_rtt ");
    put_u32(us);
    board_console_puts(" us\r\n");
}

static void handle_msg(const ipc_msg_hdr_t *h, const uint8_t *pl, uint32_t now_ms)
{
    /* IPC_LOG_LINE and IPC_SYS_HEARTBEAT are both 1 — dispatch by endpoint. */
    if (h->dst == IPC_EP_LOG) {
        if (h->type == IPC_LOG_LINE) {
            print_log(pl, h->len);
        }
        return;
    }
    if (h->type == IPC_SYS_HEARTBEAT) {
        ipc_watch_beat(&s_link.watch, now_ms);
        return;
    }
    if (h->type == IPC_SYS_READY) {
        board_console_puts("m4 ready\r\n");
        return;
    }
    if (h->type == IPC_SYS_PONG) {
        log_pong(pl, h->len);
        return;
    }
    if (h->type == IPC_AUDIO_POS || h->type == IPC_AUDIO_DONE || h->type == IPC_AUDIO_UNDERRUN ||
        h->type == IPC_AUDIO_ACK || h->type == IPC_AUDIO_NAK) {
        ipc_audio_pos_t pos;
        uint8_t ended = (h->type == IPC_AUDIO_DONE) ? 1u : 0u;
        uint16_t un = 0u;
        uint32_t elapsed = audio_elapsed_ms();

        if (h->type == IPC_AUDIO_POS && pl != NULL && h->len >= sizeof(pos)) {
            memcpy(&pos, pl, sizeof(pos));
            elapsed = pos.elapsed_ms;
            un = pos.underruns;
            ended = (pos.state == (uint8_t)AUDIO_ST_IDLE) ? 1u : 0u;
        }
        if (h->type == IPC_AUDIO_UNDERRUN) {
            un = (uint16_t)(audio_underruns() + 1u);
        }
        audio_on_peer(elapsed, un, ended);
    }
}

err_t board_ipc_init(void)
{
    err_t e;

    s_ok = 0u;
    s_ping_sent = 0u;
    board_hsem_init();
    dwt_enable();
    e = ipc_link_open(&s_link, (void *)BOARD_SRAM4_BASE, IPC_ROLE_M7, 1);
    if (e != ERR_OK) {
        return e;
    }
    ipc_link_set_kick(&s_link, kick_m4);
    kick_m4();
    board_cm4_boot();
    s_ok = 1u;
    return ERR_OK;
}

void board_ipc_poll(uint32_t now_ms)
{
    ipc_msg_hdr_t h;
    uint8_t pl[IPC_PAYLOAD_MAX];
    unsigned n = 0u;

    if (s_ok == 0u) {
        return;
    }
    (void)board_hsem_poll(BOARD_HSEM_M4_TO_M7);
    ipc_link_observe(&s_link, now_ms);
    while (n < 16u && ipc_link_recv(&s_link, &h, pl, sizeof(pl)) == ERR_OK) {
        handle_msg(&h, pl, now_ms);
        n++;
    }
    if (s_ping_sent == 0u && ipc_watch_alive(&s_link.watch, now_ms) != 0u) {
        uint32_t t0 = DWT->CYCCNT;
        if (ipc_link_send(&s_link, IPC_EP_SYS, IPC_SYS_PING, &t0, (uint16_t)sizeof(t0)) == ERR_OK) {
            s_ping_sent = 1u;
        }
    }
}

uint8_t board_ipc_peer_alive(uint32_t now_ms)
{
    if (s_ok == 0u) {
        return 0u;
    }
    return ipc_link_peer_alive(&s_link, now_ms);
}

err_t board_ipc_send(uint8_t dst, uint16_t type, const void *payload, uint16_t len)
{
    if (s_ok == 0u) {
        return ERR_IO;
    }
    return ipc_link_send(&s_link, dst, type, payload, len);
}

void *board_ipc_base(void)
{
    return (void *)BOARD_SRAM4_BASE;
}
