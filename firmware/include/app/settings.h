#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void settings_open(void);
void settings_close(void);
void settings_tick(uint32_t dt_ms);

void settings_nudge_brightness(int delta);
void settings_nudge_volume(int delta);
void settings_nudge_zb_channel(int delta);
void settings_nudge_join_s(int delta);

uint8_t settings_brightness(void);
uint8_t settings_volume(void);
uint8_t settings_zb_channel(void);
uint8_t settings_join_s(void);

const char *settings_banner(void);
const char *settings_bright_line(void);
const char *settings_vol_line(void);
const char *settings_zb_line(void);
const char *settings_about(void);
uint32_t settings_gen(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SETTINGS_H */
