#ifndef AUDIO_PIPE_H
#define AUDIO_PIPE_H

#include "ipc/ipc_msg.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_PIPE_CAP 16384u

typedef struct {
    volatile uint32_t wr;
    volatile uint32_t rd;
    uint8_t data[AUDIO_PIPE_CAP];
} audio_pipe_t;

void audio_pipe_reset(audio_pipe_t *p);
size_t audio_pipe_used(const audio_pipe_t *p);
size_t audio_pipe_space(const audio_pipe_t *p);
size_t audio_pipe_push(audio_pipe_t *p, const uint8_t *src, size_t n);
size_t audio_pipe_pop(audio_pipe_t *p, uint8_t *dst, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PIPE_H */
