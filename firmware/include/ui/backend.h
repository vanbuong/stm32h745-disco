#ifndef UI_BACKEND_H
#define UI_BACKEND_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

err_t ui_backend_init(void);
void ui_backend_handler(void);
void ui_backend_invalidate(void);
uint32_t ui_backend_frames(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_BACKEND_H */
