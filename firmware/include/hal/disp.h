#ifndef DISP_H
#define DISP_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { DISP_FMT_RGB565 = 0, DISP_FMT_ARGB8888 = 1 } disp_fmt_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} disp_rect_t;

typedef struct {
    uint16_t w;
    uint16_t h;
    uint16_t stride;
    disp_fmt_t fmt;
} disp_info_t;

void disp_get_info(disp_info_t *info);
void disp_flush(const disp_rect_t *r, const void *pixels);

#ifdef __cplusplus
}
#endif

#endif /* DISP_H */
