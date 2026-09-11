#ifndef APP_FILES_H
#define APP_FILES_H

#include "err.h"
#include "svc/media.h"
#include "svc/vfs.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FILES_ROWS_MAX 64
#define FILES_DEPTH_MAX 8

typedef enum {
    FILES_ST_OK = 0,
    FILES_ST_EMPTY,
    FILES_ST_UNMOUNTED,
    FILES_ST_IO,
    FILES_ST_PROMPT
} files_state_t;

typedef struct {
    char name[VFS_NAME_MAX];
    uint8_t is_dir;
    uint32_t size;
    media_kind_t kind;
} files_row_t;

void files_reset(void);
void files_load(void);
void files_reload(void);

const char *files_cwd(void);
files_state_t files_state(void);
unsigned files_count(void);
const files_row_t *files_row(unsigned i);
uint32_t files_view_gen(void);

int32_t files_scroll(void);
void files_set_scroll(int32_t y);

int files_on_row(unsigned i);
int files_back(void);

void files_prompt_open_text(void);
void files_prompt_cancel(void);
const char *files_prompt_name(void);
const char *files_open_path(void);

void files_format_size(uint32_t bytes, char *out, size_t out_sz);
const char *files_kind_tag(const files_row_t *r);

#ifdef __cplusplus
}
#endif

#endif /* APP_FILES_H */
