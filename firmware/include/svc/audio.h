#ifndef AUDIO_H
#define AUDIO_H

#include "err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_PATH_MAX 96
#define AUDIO_TITLE_MAX 32
#define AUDIO_VOL_DEFAULT 70u

typedef enum { AUDIO_ST_IDLE = 0, AUDIO_ST_PLAY = 1, AUDIO_ST_PAUSE = 2 } audio_state_t;

err_t audio_play(const char *path);
err_t audio_pause(void);
err_t audio_resume(void);
err_t audio_stop(void);
err_t audio_set_volume(uint8_t pct);

void audio_poll(uint32_t now_ms);

audio_state_t audio_state(void);
uint8_t audio_active(void);
uint8_t audio_volume(void);
const char *audio_path(void);
const char *audio_title(void);
uint32_t audio_elapsed_ms(void);
uint32_t audio_duration_ms(void);
uint32_t audio_underruns(void);
uint32_t audio_gen(void);
uint32_t audio_decoded_frames(void);
void audio_on_peer(uint32_t elapsed_ms, uint16_t underruns, uint8_t ended);

/* 0..100 percent → WM8994 output field 0..63 (REQ-AUD-06 companion). */
uint8_t audio_volume_to_codec(uint8_t pct);
int16_t audio_sat16(int32_t x);
void audio_mix_add(int16_t *dst, const int16_t *src, size_t n);
void audio_apply_volume(int16_t *pcm, size_t n, uint8_t pct);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_H */
