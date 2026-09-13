#ifndef IOTDEV_CONFIG_H
#define IOTDEV_CONFIG_H

#include <stdint.h>

#define CFG_ZIGBEE_CHANNEL 1
#define CFG_ZIGBEE_CHANNEL_MASK 2
#define CFG_ZIGBEE_TX_POWER 3

int iotdev_config_set_int(int key, int32_t value);
int iotdev_compare_config_int(int key, int32_t value);
int iotdev_config_save(void);

#endif /* IOTDEV_CONFIG_H */
