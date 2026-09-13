/*
 * zb_plat_serial.h
 *
 * Board / SoC-neutral serial-transport interface for the ZNP protocol driver.
 *
 * zb_znp.c speaks only in terms of these four primitives (open / close / write
 * / flush) plus an RX delivery callback; it has no direct dependency on the
 * concrete UART HAL. The iotdev/ESP backend (zb_iotdev_serial.c) adapts this to
 * the iotdev_uart driver and is selected for the firmware build via
 * ZB_PLATFORM_IOTDEV; host unit tests provide their own stub backend. This
 * mirrors the OSAL split (zb_osal.h + zb_osal_freertos.c / zb_osal_host.c).
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_PLAT_SERIAL_H
#define ZB_PLAT_SERIAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 * @brief Board-variable wiring for the ZNP serial link. Protocol-level framing
 *        (8N1, no flow control) and buffer sizes are owned by the backend.
 */
typedef struct s_zb_serial_cfg
{
    uint8_t  port;      /**< UART port (e_iotdev_uart_num_t value)        */
    uint32_t baud;      /**< baud rate (e.g. 921600)                      */
    int8_t   tx_pin;
    int8_t   rx_pin;
    int8_t   rts_pin;   /**< -1 if unused                                 */
    int8_t   cts_pin;   /**< -1 if unused                                 */
} s_zb_serial_cfg_t;

/**
 * @brief Sink for bytes received on the serial link.
 *
 * Invoked by the backend's RX path. The buffer is only valid for the duration
 * of the call. Keep it short and non-blocking.
 */
typedef void (*zb_serial_rx_cb_t)(const uint8_t *data, uint16_t len);

/**
 * @brief Open the serial transport and start delivering RX bytes to @p on_rx.
 *
 * Applies @p cfg, registers the RX callback, brings the link up and enables the
 * RX-line pull-up. Idempotency / re-open semantics follow the backend.
 */
void zb_plat_serial_open(const s_zb_serial_cfg_t *cfg, zb_serial_rx_cb_t on_rx);

/**
 * @brief Close the serial transport. Safe to call when already closed.
 */
void zb_plat_serial_close(void);

/**
 * @brief Write @p len bytes to the serial link.
 * @return number of bytes written (== @p len on success), or < 0 on failure.
 */
int zb_plat_serial_write(const uint8_t *data, uint16_t len);

/**
 * @brief Discard any buffered, not-yet-delivered RX bytes.
 */
void zb_plat_serial_flush_rx(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_PLAT_SERIAL_H */
