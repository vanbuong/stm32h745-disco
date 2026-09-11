#ifndef MEDIA_H
#define MEDIA_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MEDIA_KIND_NONE = 0,
    MEDIA_KIND_TEXT,
    MEDIA_KIND_IMAGE,
    MEDIA_KIND_AUDIO
} media_kind_t;

media_kind_t media_probe_ext(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_H */
