#include "svc/audio.h"

#include <stddef.h>
#include <stdint.h>

uint8_t audio_volume_to_codec(uint8_t pct)
{
    if (pct > 100u) {
        pct = 100u;
    }
    return (uint8_t)(((uint16_t)pct * 63u) / 100u);
}

int16_t audio_sat16(int32_t x)
{
    if (x > 32767) {
        return (int16_t)32767;
    }
    if (x < -32768) {
        return (int16_t)-32768;
    }
    return (int16_t)x;
}

void audio_mix_add(int16_t *dst, const int16_t *src, size_t n)
{
    size_t i;

    if (dst == NULL || src == NULL) {
        return;
    }
    for (i = 0u; i < n; i++) {
        dst[i] = audio_sat16((int32_t)dst[i] + (int32_t)src[i]);
    }
}

void audio_apply_volume(int16_t *pcm, size_t n, uint8_t pct)
{
    size_t i;
    int32_t g;

    if (pcm == NULL) {
        return;
    }
    if (pct > 100u) {
        pct = 100u;
    }
    if (pct == 100u) {
        return;
    }
    g = (int32_t)pct;
    for (i = 0u; i < n; i++) {
        pcm[i] = (int16_t)(((int32_t)pcm[i] * g) / 100);
    }
}
