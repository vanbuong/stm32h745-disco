#ifndef AUDIO_OUT_H
#define AUDIO_OUT_H

#include "err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

err_t audio_out_start(uint32_t sample_hz, uint8_t channels);
err_t audio_out_write(const int16_t *pcm, size_t samples);
err_t audio_out_stop(void);
uint8_t audio_out_half_ready(int16_t **pcm, size_t *frames);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_OUT_H */
