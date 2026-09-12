#include "audio_engine.h"
#include "bsp/board.h"
#include "hal/audio_out.h"
#include "ipc/ipc.h"
#include "svc/audio.h"
#include "svc/audio_pipe.h"

#include "stm32h7xx.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_gpio.h"

#include <string.h>

static volatile uint32_t g_ms;
static ipc_link_t g_link;
static uint8_t g_pl[IPC_PAYLOAD_MAX];
static audio_stream_t g_fmt;
static uint8_t g_playing;
static uint8_t g_pause;
static uint32_t g_underrun;
static uint32_t g_last_pos;

void SysTick_Handler(void)
{
    g_ms++;
    HAL_IncTick();
}

static void kick_m7(void)
{
    board_hsem_notify(BOARD_HSEM_M4_TO_M7);
}

static void wait_m7(void)
{
    const ipc_ctrl_t *ctrl = (const ipc_ctrl_t *)(void *)BOARD_SRAM4_BASE;

    while (ctrl->magic != IPC_SHM_MAGIC || ctrl->m7_ready == 0u) {
    }
}

static audio_pipe_t *pipe(void)
{
    uint8_t *base = (uint8_t *)(void *)BOARD_SRAM4_BASE;

    return (audio_pipe_t *)(void *)(base + IPC_AUDIO_PIPE_OFF);
}

static void send_pos(uint8_t ended)
{
    ipc_audio_pos_t p;
    uint32_t frames = audio_engine_frames();
    uint32_t hz = g_fmt.sample_hz;

    memset(&p, 0, sizeof(p));
    if (hz != 0u) {
        p.elapsed_ms = (uint32_t)(((uint64_t)frames * 1000u) / hz);
    }
    p.duration_ms = g_fmt.duration_ms;
    p.underruns = (uint16_t)g_underrun;
    p.state = ended             ? (uint8_t)AUDIO_ST_IDLE
              : (g_pause != 0u) ? (uint8_t)AUDIO_ST_PAUSE
                                : (uint8_t)AUDIO_ST_PLAY;
    (void)ipc_link_send(&g_link, IPC_EP_AUDIO, ended ? IPC_AUDIO_DONE : IPC_AUDIO_POS, &p,
                        (uint16_t)sizeof(p));
}

static void handle_audio(const ipc_msg_hdr_t *h)
{
    ipc_audio_fmt_t f;

    if (h->type == IPC_AUDIO_PLAY) {
        memset(&f, 0, sizeof(f));
        if (h->len >= sizeof(f)) {
            memcpy(&f, g_pl, sizeof(f));
        }
        g_fmt.kind = f.kind;
        g_fmt.channels = (f.channels != 0u) ? f.channels : 2u;
        g_fmt.bits = 16u;
        g_fmt.sample_hz = (f.sample_hz != 0u) ? f.sample_hz : 44100u;
        g_pause = 0u;
        g_underrun = 0u;
        g_playing = 1u;
        /* M7 already reset the bitstream pipe and may have pushed bytes. */
        (void)audio_engine_start(&g_fmt);
        audio_engine_set_volume(100u);
        audio_engine_set_paused(0u);
        (void)audio_out_start(g_fmt.sample_hz, g_fmt.channels);
        (void)ipc_link_send(&g_link, IPC_EP_AUDIO, IPC_AUDIO_ACK, NULL, 0u);
        return;
    }
    if (h->type == IPC_AUDIO_PAUSE) {
        g_pause = 1u;
        audio_engine_set_paused(1u);
        return;
    }
    if (h->type == IPC_AUDIO_RESUME) {
        g_pause = 0u;
        audio_engine_set_paused(0u);
        return;
    }
    if (h->type == IPC_AUDIO_STOP) {
        g_playing = 0u;
        audio_engine_reset();
        /* Keep SAI/MCLK running so the next play does not glitch the codec. */
        return;
    }
    if (h->type == IPC_AUDIO_VOLUME && h->len >= 1u) {
        /* Codec volume is on M7; keep PCM full-scale. */
        (void)g_pl[0];
    }
}

static void drain(void)
{
    ipc_msg_hdr_t h;

    while (ipc_link_recv(&g_link, &h, g_pl, sizeof(g_pl)) == ERR_OK) {
        if (h.type == IPC_SYS_PING) {
            (void)ipc_link_send(&g_link, IPC_EP_SYS, IPC_SYS_PONG, g_pl, h.len);
        } else if (h.dst == IPC_EP_AUDIO || h.type >= IPC_AUDIO_PLAY) {
            handle_audio(&h);
        }
    }
}

static void service_sai(void)
{
    unsigned n;

    /* Both HT and TC can be pending if the loop ran late; fill each half. */
    for (n = 0u; n < 2u; n++) {
        int16_t *half = NULL;
        size_t frames = 0u;
        uint32_t un = 0u;

        if (audio_out_half_ready(&half, &frames) == 0u || half == NULL) {
            return;
        }
        (void)audio_engine_fill(half, frames, pipe(), &un);
        g_underrun += un;
    }
}

int main(void)
{
    uint32_t blink_at;
    uint32_t last_hb;
    uint8_t led_on = 0u;
    const char *ready = "m4 ready";

    /*
     * CubeMX DUAL_CORE_BOOT_SYNC_SEQUENCE: park D2 in STOP before HAL_Init
     * so M7 can program PLL. M7 waits for D2CKRDY=0, then HSEM-wakes this
     * core and waits for D2CKRDY=1. After that, wait_m7() is app IPC.
     * Skip STOP if M7 already switched SYSCLK to PLL (debugger started CM7
     * first); the HSEM wake would already have been missed.
     */
    if (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_PLLCLK) {
        __HAL_RCC_HSEM_CLK_ENABLE();
        HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(BOARD_HSEM_M7_TO_M4));
        HAL_PWREx_ClearPendingEvent();
        HAL_PWREx_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFE, PWR_D2_DOMAIN);
        __HAL_HSEM_CLEAR_FLAG(__HAL_HSEM_SEMID_TO_MASK(BOARD_HSEM_M7_TO_M4));
    }

    (void)HAL_Init();
    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOJ);
    LL_GPIO_SetPinMode(GPIOJ, LL_GPIO_PIN_2, LL_GPIO_MODE_OUTPUT);

    board_hsem_init();
    wait_m7();
    (void)SysTick_Config(BOARD_M4_SYSCLK_HZ / 1000u);

    if (ipc_link_open(&g_link, (void *)BOARD_SRAM4_BASE, IPC_ROLE_M4, 0) != ERR_OK) {
        for (;;) {
        }
    }
    ipc_link_set_kick(&g_link, kick_m7);
    (void)ipc_link_send(&g_link, IPC_EP_SYS, IPC_SYS_READY, NULL, 0u);
    (void)ipc_link_send(&g_link, IPC_EP_LOG, IPC_LOG_LINE, ready, 8u);
    audio_pipe_reset(pipe());
    (void)audio_out_start(44100u, 2u);

    last_hb = g_ms;
    g_last_pos = g_ms;
    blink_at = g_ms + 250u;
    for (;;) {
        uint32_t now = g_ms;

        (void)board_hsem_poll(BOARD_HSEM_M7_TO_M4);
        drain();
        service_sai();
        if (g_playing != 0u && audio_engine_done() != 0u) {
            g_playing = 0u;
            send_pos(1u);
        }
        if ((now - last_hb) >= IPC_HB_PERIOD_MS) {
            last_hb = now;
            ipc_link_heartbeat(&g_link, now);
        }
        if (g_playing != 0u && (now - g_last_pos) >= 100u) {
            g_last_pos = now;
            send_pos(0u);
        }
        if ((int32_t)(now - blink_at) >= 0) {
            blink_at = now + 250u;
            if (led_on != 0u) {
                LL_GPIO_ResetOutputPin(GPIOJ, LL_GPIO_PIN_2);
                led_on = 0u;
            } else {
                LL_GPIO_SetOutputPin(GPIOJ, LL_GPIO_PIN_2);
                led_on = 1u;
            }
        }
    }
}
