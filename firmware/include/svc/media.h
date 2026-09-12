#ifndef MEDIA_H
#define MEDIA_H

#include "err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MEDIA_KIND_NONE = 0,
    MEDIA_KIND_TEXT,
    MEDIA_KIND_IMAGE,
    MEDIA_KIND_AUDIO,
    MEDIA_KIND_GAME
} media_kind_t;

media_kind_t media_probe_ext(const char *path);

/* Contain-fit destination for the 480×200 content area (REQ-IMG-02). */
#define IMG_DST_MAX_W 480u
#define IMG_DST_MAX_H 200u

typedef struct {
    uint16_t w;
    uint16_t h;
    uint16_t stride; /* pixels per row */
    uint16_t *px;    /* RGB565, caller-owned */
} image_buf_t;

typedef struct {
    uint16_t max_w;
    uint16_t max_h;
} image_req_t;

err_t media_decode_image(const char *path, image_buf_t *out, const image_req_t *req);
err_t media_decode_image_mem(const uint8_t *data, uint32_t size, image_buf_t *out,
                             const image_req_t *req);

/* STM32 JPEG peripheral when present; software path if this returns ERR_UNSUPPORTED. */
err_t media_jpeg_hw_decode(const uint8_t *data, uint32_t size, image_buf_t *out,
                           const image_req_t *req);

#define AUDIO_KIND_PCM 0u
#define AUDIO_KIND_MP3 1u
#define AUDIO_KIND_SEQ 2u
#define AUDIO_SEQ_HZ 44100u
#define AUDIO_SEQ_DURATION_MS 78888u

typedef struct {
    uint8_t kind;
    uint8_t channels;
    uint8_t bits;
    uint32_t sample_hz;
    uint32_t data_off;
    uint32_t data_bytes;
    uint32_t duration_ms;
} audio_stream_t;

err_t media_open_audio(const char *path, audio_stream_t *s);
err_t media_open_audio_mem(const uint8_t *data, uint32_t size, audio_stream_t *s);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_H */
