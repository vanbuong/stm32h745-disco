#ifndef CFG_H
#define CFG_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_PATH "/user/cfg.bin"
#define CFG_BRIGHT_DEFAULT 100u
#define CFG_VOL_DEFAULT 70u
#define CFG_ZB_CH_DEFAULT 15u
#define CFG_JOIN_S_DEFAULT 60u
#define CFG_ZB_CH_MIN 11u
#define CFG_ZB_CH_MAX 26u
#define CFG_BRIGHT_MIN 10u

err_t cfg_init(void);
void cfg_reset(void);
void cfg_poll(void);

uint8_t cfg_brightness(void);
uint8_t cfg_volume(void);
uint8_t cfg_zb_channel(void);
uint8_t cfg_join_s(void);

err_t cfg_set_brightness(uint8_t pct);
err_t cfg_set_volume(uint8_t pct);
err_t cfg_set_zb_channel(uint8_t ch);
err_t cfg_set_join_s(uint8_t seconds);

#ifdef __cplusplus
}
#endif

#endif /* CFG_H */
