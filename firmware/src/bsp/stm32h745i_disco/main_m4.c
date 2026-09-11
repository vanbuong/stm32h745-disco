#include "bsp/board.h"
#include "ipc/ipc.h"

#include "stm32h7xx.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_gpio.h"

static volatile uint32_t g_ms;
static ipc_link_t g_link;
static uint8_t g_pl[IPC_PAYLOAD_MAX];

void SysTick_Handler(void)
{
    g_ms++;
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

static void drain(void)
{
    ipc_msg_hdr_t h;

    while (ipc_link_recv(&g_link, &h, g_pl, sizeof(g_pl)) == ERR_OK) {
        if (h.type == IPC_SYS_PING) {
            (void)ipc_link_send(&g_link, IPC_EP_SYS, IPC_SYS_PONG, g_pl, h.len);
        }
    }
}

int main(void)
{
    uint32_t blink_at;
    uint32_t last_hb;
    uint8_t led_on = 0u;
    const char *ready = "m4 ready";

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

    last_hb = g_ms;
    blink_at = g_ms + 250u;
    for (;;) {
        uint32_t now = g_ms;

        (void)board_hsem_poll(BOARD_HSEM_M7_TO_M4);
        drain();
        if ((now - last_hb) >= IPC_HB_PERIOD_MS) {
            last_hb = now;
            ipc_link_heartbeat(&g_link, now);
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
