#include "media_priv.h"

#include "svc/vfs.h"

#include <string.h>

static const uint16_t k_mp3_br_v1[16] = {0,   32,  40,  48,  56,  64,  80,  96,
                                         112, 128, 160, 192, 224, 256, 320, 0};
static const uint16_t k_mp3_br_v2[16] = {0,  8,  16, 24,  32,  40,  48,  56,
                                         64, 80, 96, 112, 128, 144, 160, 0};
static const uint16_t k_mp3_hz[3][3] = {
    {44100, 48000, 32000},
    {22050, 24000, 16000},
    {11025, 12000, 8000},
};

static int is_riff(const uint8_t *data, uint32_t size)
{
    return (size >= 12u && data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' &&
            data[8] == 'W' && data[9] == 'A' && data[10] == 'V' && data[11] == 'E');
}

static uint32_t chunk_size(const uint8_t *p)
{
    return media_le32(p);
}

static err_t wav_parse(const uint8_t *data, uint32_t size, audio_stream_t *s)
{
    uint32_t off = 12u;
    uint16_t fmt = 0u;
    uint16_t ch = 0u;
    uint16_t bits = 0u;
    uint32_t hz = 0u;
    uint8_t have_fmt = 0u;
    uint8_t have_data = 0u;

    if (s == NULL) {
        return ERR_INVAL;
    }
    memset(s, 0, sizeof(*s));
    if (!is_riff(data, size)) {
        return ERR_CORRUPT;
    }
    while (off + 8u <= size) {
        uint32_t n = chunk_size(data + off + 4u);
        const uint8_t *id = data + off;
        uint32_t body = off + 8u;
        if (id[0] == 'f' && id[1] == 'm' && id[2] == 't' && id[3] == ' ') {
            if (n < 16u || body + 16u > size) {
                return ERR_CORRUPT;
            }
            fmt = media_le16(data + body);
            ch = media_le16(data + body + 2u);
            hz = media_le32(data + body + 4u);
            bits = media_le16(data + body + 14u);
            have_fmt = 1u;
        } else if (id[0] == 'd' && id[1] == 'a' && id[2] == 't' && id[3] == 'a') {
            s->data_off = body;
            s->data_bytes = n;
            if (body + n > size) {
                s->data_bytes = size - body;
            }
            have_data = 1u;
            break;
        }
        off = body + ((n + 1u) & ~1u);
    }
    if (have_fmt == 0u || have_data == 0u) {
        return ERR_CORRUPT;
    }
    if (fmt != 1u || (ch != 1u && ch != 2u) || bits != 16u || hz < 8000u || hz > 48000u) {
        return ERR_UNSUPPORTED;
    }
    s->kind = AUDIO_KIND_PCM;
    s->channels = (uint8_t)ch;
    s->bits = 16u;
    s->sample_hz = hz;
    if (hz != 0u && ch != 0u) {
        s->duration_ms = (uint32_t)(((uint64_t)s->data_bytes * 1000u) / (hz * (uint32_t)ch * 2u));
    }
    return ERR_OK;
}

static int mp3_sync(const uint8_t *data, uint32_t size)
{
    uint32_t i;

    if (size < 4u) {
        return -1;
    }
    for (i = 0u; i + 4u <= size; i++) {
        if (data[i] == 0xFFu && (data[i + 1u] & 0xE0u) == 0xE0u) {
            unsigned layer = (unsigned)((data[i + 1u] >> 1) & 3u);
            unsigned sr = (unsigned)((data[i + 2u] >> 2) & 3u);
            unsigned ver = (unsigned)((data[i + 1u] >> 3) & 3u);
            if (layer == 1u && sr != 3u && ver != 1u) {
                return (int)i;
            }
        }
    }
    return -1;
}

static err_t mp3_parse(const uint8_t *data, uint32_t size, audio_stream_t *s)
{
    int off;
    unsigned ver;
    unsigned sr;
    unsigned br;
    unsigned mode;
    unsigned mpeg;
    uint32_t hz;
    uint32_t kbps;
    const uint16_t *brt;

    if (s == NULL) {
        return ERR_INVAL;
    }
    memset(s, 0, sizeof(*s));
    off = mp3_sync(data, size);
    if (off < 0) {
        return ERR_CORRUPT;
    }
    ver = (unsigned)((data[off + 1] >> 3) & 3u);
    sr = (unsigned)((data[off + 2] >> 2) & 3u);
    br = (unsigned)((data[off + 2] >> 4) & 15u);
    mode = (unsigned)((data[off + 3] >> 6) & 3u);
    mpeg = (ver == 3u) ? 0u : (ver == 2u) ? 1u : 2u;
    hz = k_mp3_hz[mpeg][sr];
    brt = (mpeg == 0u) ? k_mp3_br_v1 : k_mp3_br_v2;
    kbps = brt[br];
    if (hz < 8000u) {
        return ERR_UNSUPPORTED;
    }
    s->kind = AUDIO_KIND_MP3;
    s->channels = (mode == 3u) ? 1u : 2u;
    s->bits = 16u;
    s->sample_hz = hz;
    s->data_off = (uint32_t)off;
    s->data_bytes = size - (uint32_t)off;
    if (kbps != 0u) {
        s->duration_ms = (uint32_t)(((uint64_t)s->data_bytes * 8u) / kbps);
    }
    return ERR_OK;
}

err_t media_open_audio_mem(const uint8_t *data, uint32_t size, audio_stream_t *s)
{
    if (data == NULL || s == NULL || size == 0u) {
        return ERR_INVAL;
    }
    if (is_riff(data, size)) {
        return wav_parse(data, size, s);
    }
    return mp3_parse(data, size, s);
}

err_t media_open_audio(const char *path, audio_stream_t *s)
{
    vfs_file_t fd = -1;
    vfs_stat_t st;
    uint8_t head[1024];
    size_t n = 0u;
    err_t e;

    if (path == NULL || s == NULL) {
        return ERR_INVAL;
    }
    if (media_probe_ext(path) != MEDIA_KIND_AUDIO) {
        return ERR_UNSUPPORTED;
    }
    e = vfs_stat(path, &st);
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_open(path, VFS_O_RD, &fd);
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_read(fd, head, sizeof(head), &n);
    (void)vfs_close(fd);
    if (e != ERR_OK) {
        return e;
    }
    e = media_open_audio_mem(head, (uint32_t)n, s);
    if (e != ERR_OK) {
        return e;
    }
    if (st.size > s->data_off) {
        s->data_bytes = st.size - s->data_off;
        if (s->kind == AUDIO_KIND_PCM && s->sample_hz != 0u && s->channels != 0u) {
            s->duration_ms =
                (uint32_t)(((uint64_t)s->data_bytes * 1000u) / (s->sample_hz * s->channels * 2u));
        }
    }
    return ERR_OK;
}
