#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include "err.h"
#include "svc/audio_pipe.h"
#include "svc/media.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void audio_engine_reset(void);
err_t audio_engine_start(const audio_stream_t *s);
void audio_engine_set_volume(uint8_t pct);
void audio_engine_set_paused(uint8_t on);
size_t audio_engine_fill(int16_t *dst, size_t frames, audio_pipe_t *pipe, uint32_t *underrun);
uint32_t audio_engine_frames(void);
uint8_t audio_engine_done(void);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_ENGINE_H */
