#include "audio_engine.h"

#include "mp3dec.h"
#include "svc/audio.h"

#include <string.h>

#define ENG_QMAX 4608u
#define ENG_INMAX 2048u
#define ENG_PCM_MAX 2304u

static audio_stream_t g_s;
static uint8_t g_open;
static uint8_t g_pause;
static uint8_t g_vol = AUDIO_VOL_DEFAULT;
static uint32_t g_frames;
static int16_t g_q[ENG_QMAX];
static unsigned g_qrd;
static unsigned g_qwr;
static uint8_t g_in[ENG_INMAX];
static unsigned g_in_n;
static HMP3Decoder g_mp3;

static unsigned q_used(void)
{
    return g_qwr - g_qrd;
}

static void q_push(const int16_t *pcm, unsigned n)
{
    unsigned i;

    for (i = 0u; i < n; i++) {
        if ((g_qwr - g_qrd) >= ENG_QMAX) {
            break;
        }
        g_q[g_qwr % ENG_QMAX] = pcm[i];
        g_qwr++;
    }
}

static unsigned q_pop_stereo(int16_t *dst, unsigned frames, uint8_t ch)
{
    unsigned got = 0u;

    while (got < frames) {
        if (ch == 1u) {
            if (q_used() < 1u) {
                break;
            }
            dst[got * 2u] = g_q[g_qrd % ENG_QMAX];
            dst[got * 2u + 1u] = dst[got * 2u];
            g_qrd++;
        } else {
            int16_t l;
            int16_t r;
            if (q_used() < 2u) {
                break;
            }
            l = g_q[g_qrd % ENG_QMAX];
            g_qrd++;
            r = g_q[g_qrd % ENG_QMAX];
            g_qrd++;
            dst[got * 2u] = l;
            dst[got * 2u + 1u] = r;
        }
        got++;
    }
    return got;
}

static void decode_mp3(audio_pipe_t *pipe)
{
    int16_t out[ENG_PCM_MAX];
    unsigned char *inptr;
    int left;
    int err;
    int skip;
    MP3FrameInfo fi;

    if (g_mp3 == 0) {
        g_mp3 = MP3InitDecoder();
        if (g_mp3 == 0) {
            return;
        }
    }
    if (g_in_n < ENG_INMAX) {
        g_in_n += (unsigned)audio_pipe_pop(pipe, g_in + g_in_n, ENG_INMAX - g_in_n);
    }
    if (g_in_n < 4u) {
        return;
    }
    skip = MP3FindSyncWord(g_in, (int)g_in_n);
    if (skip < 0) {
        g_in_n = 0u;
        return;
    }
    if (skip > 0) {
        memmove(g_in, g_in + skip, g_in_n - (unsigned)skip);
        g_in_n -= (unsigned)skip;
    }
    inptr = g_in;
    left = (int)g_in_n;
    err = MP3Decode(g_mp3, &inptr, &left, out, 0);
    if (left > 0 && left <= (int)g_in_n) {
        memmove(g_in, inptr, (size_t)left);
        g_in_n = (unsigned)left;
    } else {
        g_in_n = 0u;
    }
    if (err != ERR_MP3_NONE) {
        return;
    }
    MP3GetLastFrameInfo(g_mp3, &fi);
    if (fi.outputSamps > 0 && fi.outputSamps <= (int)ENG_PCM_MAX) {
        q_push(out, (unsigned)fi.outputSamps);
        if (fi.nChans == 1 || fi.nChans == 2) {
            g_s.channels = (uint8_t)fi.nChans;
        }
    }
}

static void decode_pcm(audio_pipe_t *pipe)
{
    uint8_t raw[256];
    int16_t tmp[128];
    size_t n;
    unsigned i;

    n = audio_pipe_pop(pipe, raw, sizeof(raw));
    n &= ~(size_t)1u;
    for (i = 0u; i < n / 2u; i++) {
        tmp[i] = (int16_t)((uint16_t)raw[i * 2u] | ((uint16_t)raw[i * 2u + 1u] << 8));
    }
    q_push(tmp, (unsigned)(n / 2u));
}

void audio_engine_reset(void)
{
    if (g_mp3 != 0) {
        MP3FreeDecoder(g_mp3);
        g_mp3 = 0;
    }
    memset(&g_s, 0, sizeof(g_s));
    g_open = 0u;
    g_pause = 0u;
    g_frames = 0u;
    g_qrd = 0u;
    g_qwr = 0u;
    g_in_n = 0u;
}

err_t audio_engine_start(const audio_stream_t *s)
{
    audio_engine_reset();
    if (s == NULL) {
        return ERR_INVAL;
    }
    g_s = *s;
    g_open = 1u;
    g_pause = 0u;
    if (g_s.channels == 0u) {
        g_s.channels = 2u;
    }
    return ERR_OK;
}

void audio_engine_set_volume(uint8_t pct)
{
    g_vol = (pct > 100u) ? 100u : pct;
}

void audio_engine_set_paused(uint8_t on)
{
    g_pause = (on != 0u) ? 1u : 0u;
}

size_t audio_engine_fill(int16_t *dst, size_t frames, audio_pipe_t *pipe, uint32_t *underrun)
{
    size_t got = 0u;
    uint8_t ch;
    unsigned guard = 0u;

    if (dst == NULL || frames == 0u) {
        return 0u;
    }
    memset(dst, 0, frames * 2u * sizeof(int16_t));
    if (g_open == 0u || g_pause != 0u) {
        return frames;
    }
    ch = (g_s.channels == 1u) ? 1u : 2u;
    while (got < frames && guard < 64u) {
        unsigned need = (unsigned)(frames - got);
        unsigned take;
        unsigned before;

        take = q_pop_stereo(dst + got * 2u, need, ch);
        got += take;
        if (got >= frames) {
            break;
        }
        if (pipe == NULL) {
            break;
        }
        before = q_used();
        if (g_s.kind == AUDIO_KIND_MP3) {
            decode_mp3(pipe);
        } else {
            decode_pcm(pipe);
        }
        if (q_used() == before) {
            break;
        }
        guard++;
    }
    if (got < frames && underrun != NULL) {
        (*underrun)++;
    }
    audio_apply_volume(dst, frames * 2u, g_vol);
    g_frames += (uint32_t)got;
    return frames;
}

uint32_t audio_engine_frames(void)
{
    return g_frames;
}
