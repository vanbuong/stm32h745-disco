#include "bsp/board.h"
#include "hal/uart.h"
#include "osal/osal.h"

#include "cube.h"

static osal_mutex_t *g_lock;

static void console_lock(void)
{
    if (osal_scheduler_running() == 0u) {
        return;
    }
    if (osal_mutex_ensure(&g_lock) != ERR_OK || g_lock == NULL) {
        return;
    }
    (void)osal_mutex_lock(g_lock, OSAL_WAIT_FOREVER);
}

static void console_unlock(void)
{
    if (g_lock == NULL) {
        return;
    }
    (void)osal_mutex_unlock(g_lock);
}

void board_console_init(uint32_t pclk1_hz)
{
    if (pclk1_hz == 0u) {
        pclk1_hz = HAL_RCC_GetPCLK1Freq();
        if (pclk1_hz == 0u) {
            pclk1_hz = BOARD_HSI_HZ;
        }
    }

    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);

    cube_gpio_af(GPIOB, GPIO_PIN_10, GPIO_AF7_USART3, GPIO_NOPULL);
    cube_gpio_af(GPIOB, GPIO_PIN_11, GPIO_AF7_USART3, GPIO_PULLUP);

    LL_USART_Disable(USART3);
    LL_USART_SetTransferDirection(USART3, LL_USART_DIRECTION_TX);
    LL_USART_SetDataWidth(USART3, LL_USART_DATAWIDTH_8B);
    LL_USART_SetParity(USART3, LL_USART_PARITY_NONE);
    LL_USART_SetStopBitsLength(USART3, LL_USART_STOPBITS_1);
    LL_USART_SetOverSampling(USART3, LL_USART_OVERSAMPLING_16);
    LL_USART_SetBaudRate(USART3, pclk1_hz, LL_USART_PRESCALER_DIV1, LL_USART_OVERSAMPLING_16,
                         BOARD_UART_BAUD);
    LL_USART_Enable(USART3);
}

void board_console_puts(const char *s)
{
    if (s == NULL) {
        return;
    }
    console_lock();
    while (*s != '\0') {
        uart_rx_pump();
        while (LL_USART_IsActiveFlag_TXE(USART3) == 0u) {
            uart_rx_pump();
        }
        LL_USART_TransmitData8(USART3, (uint8_t)*s++);
    }
    console_unlock();
}

void board_console_put_hex32(uint32_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[9];
    int i;

    for (i = 7; i >= 0; i--) {
        buf[i] = hex[v & 0xFu];
        v >>= 4;
    }
    buf[8] = '\0';
    board_console_puts(buf);
}
