#ifndef TEXT_VIEW_H
#define TEXT_VIEW_H

#include "err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Peak RAM for the resident text window (REQ-TXT-02). Files larger than
 * TEXT_FILE_WINDOW_MIN are still shown; only one window is loaded at a time.
 */
#define TEXT_WIN_MAX 32768u
#define TEXT_FILE_WINDOW_MIN (256u * 1024u)

err_t text_view_open(const char *path);
err_t text_view_open_mem(const uint8_t *data, uint32_t size);
void text_view_close(void);

err_t text_view_set_window(uint32_t byte_off);
err_t text_view_page(int dir);

const char *text_view_text(void);
uint32_t text_view_size(void);
uint32_t text_view_offset(void);
uint32_t text_view_win_bytes(void);
uint32_t text_view_progress(void);
const char *text_view_name(void);
err_t text_view_status(void);
uint32_t text_view_gen(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXT_VIEW_H */
