#include "iotdev_config/include/iotdev_config.h"
#include "iotdev_gpio/include/iotdev_gpio.h"

#include "hal/uart.h"
#include "svc/cfg.h"

static int32_t g_mask;
static int32_t g_tx = 5;
static uint8_t g_mask_set;

int iotdev_config_set_int(int key, int32_t value)
{
    if (key == CFG_ZIGBEE_CHANNEL) {
        return (cfg_set_zb_channel((uint8_t)value) == ERR_OK) ? 0 : -1;
    }
    if (key == CFG_ZIGBEE_CHANNEL_MASK) {
        g_mask = value;
        g_mask_set = 1u;
        return 0;
    }
    if (key == CFG_ZIGBEE_TX_POWER) {
        g_tx = value;
        return 0;
    }
    return -1;
}

int iotdev_compare_config_int(int key, int32_t value)
{
    if (key == CFG_ZIGBEE_CHANNEL) {
        return (cfg_zb_channel() == (uint8_t)value) ? 1 : 0;
    }
    if (key == CFG_ZIGBEE_CHANNEL_MASK) {
        if (g_mask_set == 0u) {
            return 0;
        }
        return (g_mask == value) ? 1 : 0;
    }
    if (key == CFG_ZIGBEE_TX_POWER) {
        return (g_tx == value) ? 1 : 0;
    }
    return 0;
}

int iotdev_config_save(void)
{
    cfg_poll();
    return 1;
}

int iotdev_gpio_set_coprocessor_reset_pin(bool level)
{
    return (uart_set_gpio(UART_ID_ZNP, UART_PIN_RESET, level ? 1 : 0) == ERR_OK) ? 0 : -1;
}

int iotdev_gpio_set_coprocessor_boot_pin(bool level)
{
    return (uart_set_gpio(UART_ID_ZNP, UART_PIN_BOOT, level ? 1 : 0) == ERR_OK) ? 0 : -1;
}
