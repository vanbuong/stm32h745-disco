#ifndef DISP_GEOM_H
#define DISP_GEOM_H

#include "hal/disp.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int disp_clip_rect(disp_rect_t *r, uint16_t max_w, uint16_t max_h);
uint16_t disp_rgb565(uint8_t r, uint8_t g, uint8_t b);

#ifdef __cplusplus
}
#endif

#endif /* DISP_GEOM_H */
