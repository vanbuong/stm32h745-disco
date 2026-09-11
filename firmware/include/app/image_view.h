#ifndef IMAGE_VIEW_H
#define IMAGE_VIEW_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

err_t image_view_open(const char *path);
void image_view_close(void);
err_t image_view_next(int dir);

const uint16_t *image_view_pixels(void);
uint16_t image_view_w(void);
uint16_t image_view_h(void);
uint16_t image_view_stride(void);
const char *image_view_name(void);
const char *image_view_err_str(void);
err_t image_view_status(void);
uint32_t image_view_gen(void);

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_VIEW_H */
