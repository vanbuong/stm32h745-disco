#ifndef APP_PLAYER_H
#define APP_PLAYER_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

err_t player_open(const char *path);
void player_close(void);
err_t player_next(int dir);
void player_toggle(void);

const char *player_path(void);
const char *player_title(void);
const char *player_err_str(void);
err_t player_status(void);
uint8_t player_playing(void);
uint32_t player_elapsed_ms(void);
uint32_t player_duration_ms(void);
uint8_t player_volume(void);
void player_set_volume(uint8_t pct);
uint32_t player_gen(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_PLAYER_H */
