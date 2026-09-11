#include "svc/audio_pipe.h"

#include <string.h>

_Static_assert((AUDIO_PIPE_CAP & (AUDIO_PIPE_CAP - 1u)) == 0u, "pipe cap pow2");
_Static_assert((IPC_AUDIO_PIPE_OFF + sizeof(audio_pipe_t)) <= IPC_SHM_BYTES, "pipe in sram4");

void audio_pipe_reset(audio_pipe_t *p)
{
    if (p == NULL) {
        return;
    }
    p->wr = 0u;
    p->rd = 0u;
}

size_t audio_pipe_used(const audio_pipe_t *p)
{
    if (p == NULL) {
        return 0u;
    }
    return (size_t)(p->wr - p->rd);
}

size_t audio_pipe_space(const audio_pipe_t *p)
{
    size_t used;

    if (p == NULL) {
        return 0u;
    }
    used = audio_pipe_used(p);
    if (used >= AUDIO_PIPE_CAP) {
        return 0u;
    }
    return AUDIO_PIPE_CAP - used;
}

size_t audio_pipe_push(audio_pipe_t *p, const uint8_t *src, size_t n)
{
    size_t i;
    size_t room;

    if (p == NULL || src == NULL || n == 0u) {
        return 0u;
    }
    room = audio_pipe_space(p);
    if (n > room) {
        n = room;
    }
    for (i = 0u; i < n; i++) {
        p->data[p->wr & (AUDIO_PIPE_CAP - 1u)] = src[i];
        p->wr++;
    }
    return n;
}

size_t audio_pipe_pop(audio_pipe_t *p, uint8_t *dst, size_t n)
{
    size_t i;
    size_t have;

    if (p == NULL || dst == NULL || n == 0u) {
        return 0u;
    }
    have = audio_pipe_used(p);
    if (n > have) {
        n = have;
    }
    for (i = 0u; i < n; i++) {
        dst[i] = p->data[p->rd & (AUDIO_PIPE_CAP - 1u)];
        p->rd++;
    }
    return n;
}
