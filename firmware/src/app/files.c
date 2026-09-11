#include "app/files.h"

#include "app/apps.h"
#include "ui/shell.h"

#include <string.h>

static char g_cwd[VFS_PATH_MAX];
static char g_open_path[VFS_PATH_MAX];
static char g_prompt_name[VFS_NAME_MAX];
static files_row_t g_rows[FILES_ROWS_MAX];
static unsigned g_n;
static files_state_t g_st;
static uint32_t g_gen;
static int32_t g_scroll;
static int32_t g_scroll_stack[FILES_DEPTH_MAX];
static uint8_t g_depth;

static void bump(void)
{
    g_gen++;
}

static void put_u32(char *out, size_t n, uint32_t v, const char *suf)
{
    char tmp[11];
    int i = 10;
    size_t o = 0u;

    if (n == 0u) {
        return;
    }
    tmp[10] = '\0';
    if (v == 0u) {
        tmp[--i] = '0';
    }
    while (v > 0u && i > 0) {
        tmp[--i] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    while (tmp[i] != '\0' && o + 1u < n) {
        out[o++] = tmp[i++];
    }
    while (suf != NULL && *suf != '\0' && o + 1u < n) {
        out[o++] = *suf++;
    }
    out[o] = '\0';
}

static void sort_rows(void)
{
    unsigned i;
    unsigned j;

    for (i = 1u; i < g_n; i++) {
        files_row_t key = g_rows[i];
        j = i;
        while (j > 0u) {
            const files_row_t *a = &g_rows[j - 1u];
            int less = 0;
            if (key.is_dir != a->is_dir) {
                less = (key.is_dir != 0u) ? 1 : 0;
            } else {
                less = (strcmp(key.name, a->name) < 0) ? 1 : 0;
            }
            if (less == 0) {
                break;
            }
            g_rows[j] = g_rows[j - 1u];
            j--;
        }
        g_rows[j] = key;
    }
}

static const char *kind_app(media_kind_t k)
{
    if (k == MEDIA_KIND_TEXT) {
        return APP_ID_TEXT;
    }
    if (k == MEDIA_KIND_IMAGE) {
        return APP_ID_IMAGE;
    }
    if (k == MEDIA_KIND_AUDIO) {
        return APP_ID_PLAYER;
    }
    return NULL;
}

void files_reset(void)
{
    g_cwd[0] = '\0';
    g_open_path[0] = '\0';
    g_prompt_name[0] = '\0';
    g_n = 0u;
    g_st = FILES_ST_OK;
    g_scroll = 0;
    g_depth = 0u;
    memset(g_scroll_stack, 0, sizeof(g_scroll_stack));
    bump();
}

void files_load(void)
{
    vfs_dir_t d = -1;
    vfs_dirent_t ent;
    err_t e;

    g_n = 0u;
    g_prompt_name[0] = '\0';
    if (g_cwd[0] == '\0') {
        memcpy(g_cwd, VFS_JAIL_PREFIX, sizeof(VFS_JAIL_PREFIX));
        g_depth = 0u;
    }
    if (vfs_mounted() == 0) {
        g_st = FILES_ST_UNMOUNTED;
        bump();
        return;
    }
    e = vfs_opendir(g_cwd, &d);
    if (e != ERR_OK) {
        g_st = FILES_ST_IO;
        bump();
        return;
    }
    for (;;) {
        e = vfs_readdir(d, &ent);
        if (e == ERR_NOENT) {
            break;
        }
        if (e != ERR_OK) {
            (void)vfs_closedir(d);
            g_st = FILES_ST_IO;
            g_n = 0u;
            bump();
            return;
        }
        if (ent.name[0] == '\0' || strcmp(ent.name, ".") == 0 || strcmp(ent.name, "..") == 0) {
            continue;
        }
        if (g_n >= FILES_ROWS_MAX) {
            break;
        }
        memset(&g_rows[g_n], 0, sizeof(g_rows[g_n]));
        memcpy(g_rows[g_n].name, ent.name, VFS_NAME_MAX);
        g_rows[g_n].is_dir = ent.is_dir;
        g_rows[g_n].size = ent.size;
        g_rows[g_n].kind = (ent.is_dir != 0u) ? MEDIA_KIND_NONE : media_probe_ext(ent.name);
        g_n++;
    }
    (void)vfs_closedir(d);
    sort_rows();
    g_st = (g_n == 0u) ? FILES_ST_EMPTY : FILES_ST_OK;
    bump();
}

void files_reload(void)
{
    files_load();
}

const char *files_cwd(void)
{
    return g_cwd;
}

files_state_t files_state(void)
{
    return g_st;
}

unsigned files_count(void)
{
    return g_n;
}

const files_row_t *files_row(unsigned i)
{
    if (i >= g_n) {
        return NULL;
    }
    return &g_rows[i];
}

uint32_t files_view_gen(void)
{
    return g_gen;
}

int32_t files_scroll(void)
{
    return g_scroll;
}

void files_set_scroll(int32_t y)
{
    g_scroll = y;
}

int files_on_row(unsigned i)
{
    const files_row_t *r = files_row(i);
    char next[VFS_PATH_MAX];
    const char *app;
    err_t e;

    if (r == NULL) {
        return 0;
    }
    if (r->is_dir != 0u) {
        e = vfs_path_join(g_cwd, r->name, next, sizeof(next));
        if (e != ERR_OK) {
            g_st = FILES_ST_IO;
            bump();
            return 1;
        }
        if (g_depth >= FILES_DEPTH_MAX) {
            return 1;
        }
        g_scroll_stack[g_depth] = g_scroll;
        g_depth++;
        memcpy(g_cwd, next, sizeof(g_cwd));
        g_scroll = 0;
        files_load();
        return 1;
    }
    e = vfs_path_join(g_cwd, r->name, g_open_path, sizeof(g_open_path));
    if (e != ERR_OK) {
        return 0;
    }
    app = kind_app(r->kind);
    if (app != NULL) {
        g_scroll_stack[g_depth] = g_scroll;
        (void)shell_push(app, g_open_path);
        return 1;
    }
    memcpy(g_prompt_name, r->name, VFS_NAME_MAX);
    g_st = FILES_ST_PROMPT;
    bump();
    return 1;
}

int files_back(void)
{
    char parent[VFS_PATH_MAX];
    err_t e;

    if (g_depth == 0u) {
        return 0;
    }
    e = vfs_path_parent(g_cwd, parent, sizeof(parent));
    if (e != ERR_OK) {
        return 0;
    }
    memcpy(g_cwd, parent, sizeof(g_cwd));
    g_depth--;
    g_scroll = g_scroll_stack[g_depth];
    files_load();
    return 1;
}

void files_prompt_open_text(void)
{
    if (g_st != FILES_ST_PROMPT || g_open_path[0] == '\0') {
        files_prompt_cancel();
        return;
    }
    g_st = FILES_ST_OK;
    g_scroll_stack[g_depth] = g_scroll;
    (void)shell_push(APP_ID_TEXT, g_open_path);
}

void files_prompt_cancel(void)
{
    g_prompt_name[0] = '\0';
    g_st = (g_n == 0u) ? FILES_ST_EMPTY : FILES_ST_OK;
    bump();
}

const char *files_prompt_name(void)
{
    return g_prompt_name;
}

const char *files_open_path(void)
{
    return g_open_path;
}

void files_format_size(uint32_t bytes, char *out, size_t out_sz)
{
    if (out == NULL || out_sz == 0u) {
        return;
    }
    if (bytes < 1024u) {
        put_u32(out, out_sz, bytes, " B");
    } else if (bytes < (1024u * 1024u)) {
        put_u32(out, out_sz, bytes / 1024u, " KB");
    } else {
        put_u32(out, out_sz, bytes / (1024u * 1024u), " MB");
    }
}

const char *files_kind_tag(const files_row_t *r)
{
    if (r == NULL) {
        return "BIN";
    }
    if (r->is_dir != 0u) {
        return "DIR";
    }
    if (r->kind == MEDIA_KIND_TEXT) {
        return "TXT";
    }
    if (r->kind == MEDIA_KIND_IMAGE) {
        return "IMG";
    }
    if (r->kind == MEDIA_KIND_AUDIO) {
        return "AUD";
    }
    return "BIN";
}
