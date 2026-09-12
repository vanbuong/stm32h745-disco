#include "audio_engine.h"

#include "mp3dec.h"
#include "svc/audio.h"

#include <string.h>

#define ENG_QMAX 4608u
#define ENG_INMAX 2048u
#define ENG_PCM_MAX 2304u

#define SEQ_HZ 44100u
#define SEQ_BEAT_SAMPLES 24500u /* 44100 * 60 / 108 */
#define SEQ_ATK 529u            /* 12 ms */
#define SEQ_REL 1984u           /* 45 ms */
#define SEQ_LEAD_VOL 12000
#define SEQ_BASS_VOL 7000

typedef struct {
    uint8_t midi;
    uint8_t beats4;
} seq_note_t;

typedef struct {
    const seq_note_t *notes;
    uint8_t count;
    uint8_t times;
} seq_part_t;

typedef struct {
    const seq_part_t *parts;
    unsigned nparts;
    unsigned part;
    unsigned rep;
    unsigned idx;
    uint32_t pos;
    uint32_t len;
    uint32_t phase;
    uint32_t inc;
    int vol;
} seq_voice_t;

static audio_stream_t g_s;
static uint8_t g_open;
static uint8_t g_pause;
static uint8_t g_vol = AUDIO_VOL_DEFAULT;
static uint8_t g_seq_done;
static uint32_t g_frames;
static int16_t g_q[ENG_QMAX];
static unsigned g_qrd;
static unsigned g_qwr;
static uint8_t g_in[ENG_INMAX];
static unsigned g_in_n;
static HMP3Decoder g_mp3;
static seq_voice_t g_lead;
static seq_voice_t g_bass;

/* Quarter-sine 0..90 degrees inclusive. */
static const uint16_t g_qsin[65] = {
    0,     804,   1608,  2410,  3212,  4011,  4808,  5602,  6393,  7179,  7962,  8739,  9512,
    10278, 11039, 11793, 12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530, 18204, 18868,
    19519, 20159, 20787, 21403, 22005, 22594, 23170, 23731, 24279, 24811, 25329, 25832, 26319,
    26790, 27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956, 30273, 30571, 30852, 31113,
    31356, 31580, 31785, 31971, 32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757, 32767};

/* MIDI C2..C5 Hz, rounded. */
static const uint16_t g_midi_hz[37] = {
    65,  69,  73,  78,  82,  87,  92,  98,  104, 110, 117, 123, 131, 139, 147, 156, 165, 175, 185,
    196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523};

#define M_R 0u
#define M_C2 36u
#define M_E2 40u
#define M_F2 41u
#define M_G2 43u
#define M_A2 45u
#define M_C3 48u
#define M_G3 55u
#define M_C4 60u
#define M_D4 62u
#define M_E4 64u
#define M_F4 65u
#define M_G4 67u
#define M_C5 72u

static const seq_note_t g_intro[] = {
    {M_G3, 4}, {M_C4, 4}, {M_E4, 4}, {M_G4, 8}, {M_R, 4},
};

static const seq_note_t g_theme[] = {
    {M_E4, 4}, {M_E4, 4}, {M_F4, 4}, {M_G4, 4}, {M_G4, 4}, {M_F4, 4}, {M_E4, 4}, {M_D4, 4},
    {M_C4, 4}, {M_C4, 4}, {M_D4, 4}, {M_E4, 4}, {M_E4, 6}, {M_D4, 2}, {M_D4, 8}, {M_E4, 4},
    {M_E4, 4}, {M_F4, 4}, {M_G4, 4}, {M_G4, 4}, {M_F4, 4}, {M_E4, 4}, {M_D4, 4}, {M_C4, 4},
    {M_C4, 4}, {M_D4, 4}, {M_E4, 4}, {M_D4, 6}, {M_C4, 2}, {M_C4, 8}, {M_D4, 4}, {M_D4, 4},
    {M_E4, 4}, {M_C4, 4}, {M_D4, 4}, {M_E4, 2}, {M_F4, 2}, {M_E4, 4}, {M_C4, 4}, {M_D4, 4},
    {M_E4, 2}, {M_F4, 2}, {M_E4, 4}, {M_D4, 4}, {M_C4, 4}, {M_D4, 4}, {M_G3, 8}, {M_E4, 4},
    {M_E4, 4}, {M_F4, 4}, {M_G4, 4}, {M_G4, 4}, {M_F4, 4}, {M_E4, 4}, {M_D4, 4}, {M_C4, 4},
    {M_C4, 4}, {M_D4, 4}, {M_E4, 4}, {M_D4, 6}, {M_C4, 2}, {M_C4, 8},
};

static const seq_note_t g_coda[] = {
    {M_C4, 4}, {M_E4, 4}, {M_G4, 4}, {M_C5, 16}, {M_R, 4},
};

static const seq_note_t g_intro_b[] = {
    {M_G2, 8},
    {M_C3, 8},
    {M_G2, 8},
};

static const seq_note_t g_bass_notes[] = {
    {M_C3, 8}, {M_G2, 8}, {M_C3, 8}, {M_G2, 8}, {M_A2, 8}, {M_E2, 8}, {M_F2, 8}, {M_G2, 8},
    {M_C3, 8}, {M_G2, 8}, {M_C3, 8}, {M_G2, 8}, {M_A2, 8}, {M_E2, 8}, {M_F2, 8}, {M_C3, 8},
    {M_G2, 8}, {M_C3, 8}, {M_G2, 8}, {M_C3, 8}, {M_G2, 8}, {M_C3, 8}, {M_G2, 8}, {M_C3, 8},
    {M_C3, 8}, {M_G2, 8}, {M_C3, 8}, {M_G2, 8}, {M_A2, 8}, {M_F2, 8}, {M_G2, 8}, {M_C3, 8},
};

static const seq_note_t g_coda_b[] = {
    {M_C3, 8},
    {M_G2, 8},
    {M_C2, 16},
};

static const seq_part_t g_lead_parts[] = {
    {g_intro, (uint8_t)(sizeof(g_intro) / sizeof(g_intro[0])), 1u},
    {g_theme, (uint8_t)(sizeof(g_theme) / sizeof(g_theme[0])), 2u},
    {g_coda, (uint8_t)(sizeof(g_coda) / sizeof(g_coda[0])), 1u},
};

static const seq_part_t g_bass_parts[] = {
    {g_intro_b, (uint8_t)(sizeof(g_intro_b) / sizeof(g_intro_b[0])), 1u},
    {g_bass_notes, (uint8_t)(sizeof(g_bass_notes) / sizeof(g_bass_notes[0])), 2u},
    {g_coda_b, (uint8_t)(sizeof(g_coda_b) / sizeof(g_coda_b[0])), 1u},
};

static int16_t lut_sin(uint32_t phase)
{
    unsigned i = (unsigned)(phase >> 24);
    unsigned q = i >> 6;
    unsigned n = i & 63u;
    uint16_t v;

    if (q == 0u) {
        v = g_qsin[n];
    } else if (q == 1u) {
        v = g_qsin[64u - n];
    } else if (q == 2u) {
        return -(int16_t)g_qsin[n];
    } else {
        return -(int16_t)g_qsin[64u - n];
    }
    return (int16_t)v;
}

static uint32_t midi_inc(uint8_t midi)
{
    uint16_t hz;

    if (midi < 36u || midi > 72u) {
        return 0u;
    }
    hz = g_midi_hz[midi - 36u];
    return (uint32_t)(((uint64_t)hz << 32) / (uint64_t)SEQ_HZ);
}

static uint32_t seq_duration_ms(void)
{
    unsigned p;
    unsigned k;
    uint32_t beats4 = 0u;

    for (p = 0u; p < (unsigned)(sizeof(g_lead_parts) / sizeof(g_lead_parts[0])); p++) {
        uint32_t part = 0u;
        for (k = 0u; k < g_lead_parts[p].count; k++) {
            part += g_lead_parts[p].notes[k].beats4;
        }
        beats4 += part * (uint32_t)g_lead_parts[p].times;
    }
    return (uint32_t)(((uint64_t)beats4 * (uint64_t)SEQ_BEAT_SAMPLES * 1000u) /
                      ((uint64_t)SEQ_HZ * 4u));
}

static void voice_init(seq_voice_t *v, const seq_part_t *parts, unsigned nparts, int vol)
{
    memset(v, 0, sizeof(*v));
    v->parts = parts;
    v->nparts = nparts;
    v->vol = vol;
}

static int voice_load(seq_voice_t *v)
{
    const seq_part_t *part;
    const seq_note_t *note;

    while (v->part < v->nparts) {
        part = &v->parts[v->part];
        if (v->idx < part->count) {
            note = &part->notes[v->idx];
            v->len = ((uint32_t)note->beats4 * SEQ_BEAT_SAMPLES) / 4u;
            v->pos = 0u;
            v->phase = 0u;
            v->inc = midi_inc(note->midi);
            return 1;
        }
        v->idx = 0u;
        v->rep++;
        if (v->rep < part->times) {
            continue;
        }
        v->rep = 0u;
        v->part++;
    }
    return 0;
}

static int32_t voice_tick(seq_voice_t *v)
{
    uint32_t rel;
    int32_t env;
    int32_t s;

    if (v->len == 0u && voice_load(v) == 0) {
        return 0;
    }
    if (v->pos >= v->len) {
        v->idx++;
        if (voice_load(v) == 0) {
            v->len = 0u;
            return 0;
        }
    }
    if (v->inc == 0u) {
        v->pos++;
        return 0;
    }
    if (v->pos < SEQ_ATK) {
        env = (int32_t)((v->pos * (uint32_t)v->vol) / SEQ_ATK);
    } else {
        env = (v->vol * 7) / 10;
        if (v->len > SEQ_REL) {
            rel = v->len - SEQ_REL;
            if (v->pos >= rel) {
                env = (int32_t)(((v->len - 1u - v->pos) * (uint32_t)env) / SEQ_REL);
            }
        }
    }
    s = ((int32_t)lut_sin(v->phase) * env) / 32767;
    v->phase += v->inc;
    v->pos++;
    return s;
}

static void seq_reset(void)
{
    voice_init(&g_lead, g_lead_parts, (unsigned)(sizeof(g_lead_parts) / sizeof(g_lead_parts[0])),
               SEQ_LEAD_VOL);
    voice_init(&g_bass, g_bass_parts, (unsigned)(sizeof(g_bass_parts) / sizeof(g_bass_parts[0])),
               SEQ_BASS_VOL);
    g_seq_done = 0u;
}

static void seq_fill(int16_t *dst, size_t frames)
{
    size_t i;
    uint8_t alive = 0u;

    for (i = 0u; i < frames; i++) {
        int32_t mix = voice_tick(&g_lead) + voice_tick(&g_bass);
        int16_t s = audio_sat16(mix);
        dst[i * 2u] = s;
        dst[i * 2u + 1u] = s;
        if (g_lead.len != 0u || g_bass.len != 0u) {
            alive = 1u;
        }
    }
    if (alive == 0u) {
        g_seq_done = 1u;
    }
}

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
    memset(&g_lead, 0, sizeof(g_lead));
    memset(&g_bass, 0, sizeof(g_bass));
    g_open = 0u;
    g_pause = 0u;
    g_seq_done = 0u;
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
    if (g_s.kind == AUDIO_KIND_SEQ) {
        seq_reset();
        if (g_s.sample_hz == 0u) {
            g_s.sample_hz = SEQ_HZ;
        }
        if (g_s.duration_ms == 0u) {
            g_s.duration_ms = seq_duration_ms();
        }
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
    if (g_s.kind == AUDIO_KIND_SEQ) {
        seq_fill(dst, frames);
        audio_apply_volume(dst, frames * 2u, g_vol);
        if (g_seq_done == 0u) {
            g_frames += (uint32_t)frames;
        } else if (underrun != NULL) {
            (*underrun)++;
        }
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

uint8_t audio_engine_done(void)
{
    return g_seq_done;
}
