#ifndef LV_PORT_H
#define LV_PORT_H

#include "err.h"

#include <stdint.h>

err_t lv_port_init(void);
uint32_t lv_port_frames(void);

#endif /* LV_PORT_H */
