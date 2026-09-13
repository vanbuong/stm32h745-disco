#ifndef IOTDEV_GPIO_H
#define IOTDEV_GPIO_H

#include <stdbool.h>

int iotdev_gpio_set_coprocessor_reset_pin(bool level);
int iotdev_gpio_set_coprocessor_boot_pin(bool level);

#endif /* IOTDEV_GPIO_H */
