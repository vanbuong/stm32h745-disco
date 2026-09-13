/*
 * zb_iotdev_peripheral.h
 *
 * Platform abstraction for the low-level peripheral primitives the ZNP
 * transport needs that are not already covered by the iotdev_* HAL
 * (UART). Keeps the direct ESP-IDF dependencies (driver/gpio.h,
 * esp_rom_sys.h) confined to a single place so the protocol driver
 * (zb_znp.c) stays board / SoC agnostic and unit-testable through a mock.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_IOTDEV_PERIPHERAL_H
#define ZB_IOTDEV_PERIPHERAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if defined(ZB_PLATFORM_IOTDEV)

#include "driver/gpio.h"
#include "esp_rom_sys.h"

/**
 * @brief Enable the internal pull-up on the ZNP UART RX line.
 *
 * Keeps the RX line idling high while the coprocessor is held in reset or has
 * not yet started driving its TX pin, preventing spurious framing on bring-up.
 *
 * @param rx_gpio RX GPIO number, or a negative value to skip (no RX pin).
 */
static inline void
zb_plat_uart_rx_pullup_en(int rx_gpio)
{
    if (rx_gpio >= 0)
    {
        gpio_pullup_en((gpio_num_t)rx_gpio);
    }
}

/**
 * @brief Busy-wait microsecond delay used for coprocessor RESET pulse timing.
 */
static inline void
zb_plat_delay_us(uint32_t us)
{
    esp_rom_delay_us(us);
}

#else /* !ZB_PLATFORM_IOTDEV — host / test build */

static inline void zb_plat_uart_rx_pullup_en(int rx_gpio) { (void)rx_gpio; }
static inline void zb_plat_delay_us(uint32_t us) { (void)us; }

#endif /* ZB_PLATFORM_IOTDEV */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_IOTDEV_PERIPHERAL_H */
