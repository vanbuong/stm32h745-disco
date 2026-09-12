#include "app/player.h"

#include "svc/audio.h"
#include "svc/cfg.h"
#include "svc/media.h"
#include "svc/vfs.h"

#include <string.h>

#define PLAYER_LIST_MAX 32

static char g_path[VFS_PATH_MAX];
static char g_err[40];
static err_t g_st;
static uint32_t g_gen;

static void bump(void)
{
    g_gen++;
}

static void set_err(err_t e)
{
    const char *m;

    if (e == ERR_OK) {
        g_err[0] = '\0';
        return;
    }
    if (e == ERR_CORRUPT) {
        m = "truncated or corrupt";
    } else if (e == ERR_UNSUPPORTED) {
        m = "format not supported";
    } else if (e == ERR_NOENT) {
        m = "file not found";
    } else {
        m = "can't open audio";
    }
    strncpy(g_err, m, sizeof(g_err) - 1u);
    g_err[sizeof(g_err) - 1u] = '\0';
}

static void sort_names(char list[][VFS_NAME_MAX], int n)
{
    int i;
    int j;
    char tmp[VFS_NAME_MAX];

    for (i = 1; i < n; i++) {
        memcpy(tmp, list[i], VFS_NAME_MAX);
        j = i;
        while (j > 0 && strcmp(tmp, list[j - 1]) < 0) {
            memcpy(list[j], list[j - 1], VFS_NAME_MAX);
            j--;
        }
        memcpy(list[j], tmp, VFS_NAME_MAX);
    }
}

static int collect_audio(const char *parent, char list[][VFS_NAME_MAX])
{
    vfs_dir_t d = -1;
    vfs_dirent_t ent;
    int n = 0;
    err_t e;

    e = vfs_opendir(parent, &d);
    if (e != ERR_OK) {
        return -1;
    }
    for (;;) {
        e = vfs_readdir(d, &ent);
        if (e == ERR_NOENT) {
            break;
        }
        if (e != ERR_OK) {
            (void)vfs_closedir(d);
            return -1;
        }
        if (ent.is_dir != 0u || media_probe_ext(ent.name) != MEDIA_KIND_AUDIO) {
            continue;
        }
        if (n < PLAYER_LIST_MAX) {
            memcpy(list[n], ent.name, VFS_NAME_MAX);
            n++;
        }
    }
    (void)vfs_closedir(d);
    sort_names(list, n);
    return n;
}

err_t player_open(const char *path)
{
    size_t n;

    if (path == NULL || path[0] == '\0') {
        g_st = ERR_INVAL;
        set_err(g_st);
        bump();
        return g_st;
    }
    n = strlen(path);
    if (n >= VFS_PATH_MAX) {
        g_st = ERR_NOSPC;
        set_err(g_st);
        bump();
        return g_st;
    }
    memcpy(g_path, path, n + 1u);
    g_st = audio_play(path);
    set_err(g_st);
    bump();
    return g_st;
}

void player_close(void)
{
    g_path[0] = '\0';
    g_err[0] = '\0';
    g_st = ERR_OK;
    bump();
}

err_t player_next(int dir)
{
    char parent[VFS_PATH_MAX];
    char list[PLAYER_LIST_MAX][VFS_NAME_MAX];
    char next_path[VFS_PATH_MAX];
    const char *cur;
    err_t e;
    int n;
    int i;
    int idx = -1;

    cur = (g_path[0] != '\0') ? g_path : audio_path();
    e = vfs_path_parent(cur, parent, sizeof(parent));
    if (e != ERR_OK) {
        return e;
    }
    n = collect_audio(parent, list);
    if (n <= 0) {
        return ERR_NOENT;
    }
    {
        const char *slash = strrchr(cur, '/');
        slash = (slash != NULL) ? (slash + 1) : cur;
        for (i = 0; i < n; i++) {
            if (strcmp(list[i], slash) == 0) {
                idx = i;
                break;
            }
        }
    }
    if (idx < 0) {
        idx = 0;
    } else if (dir < 0) {
        idx = (idx == 0) ? (n - 1) : (idx - 1);
    } else {
        idx = (idx + 1) % n;
    }
    e = vfs_path_join(parent, list[idx], next_path, sizeof(next_path));
    if (e != ERR_OK) {
        return e;
    }
    return player_open(next_path);
}

void player_toggle(void)
{
    if (audio_state() == AUDIO_ST_PLAY) {
        (void)audio_pause();
    } else if (audio_state() == AUDIO_ST_PAUSE) {
        (void)audio_resume();
    } else if (g_path[0] != '\0') {
        (void)audio_play(g_path);
    }
    bump();
}

const char *player_path(void)
{
    return (g_path[0] != '\0') ? g_path : audio_path();
}

const char *player_title(void)
{
    const char *t = audio_title();

    if (t != NULL && t[0] != '\0') {
        return t;
    }
    return "No track";
}

const char *player_err_str(void)
{
    return g_err;
}

err_t player_status(void)
{
    return g_st;
}

uint8_t player_playing(void)
{
    return (audio_state() == AUDIO_ST_PLAY) ? 1u : 0u;
}

uint32_t player_elapsed_ms(void)
{
    return audio_elapsed_ms();
}

uint32_t player_duration_ms(void)
{
    return audio_duration_ms();
}

uint8_t player_volume(void)
{
    return audio_volume();
}

void player_set_volume(uint8_t pct)
{
    (void)audio_set_volume(pct);
    (void)cfg_set_volume(pct);
    bump();
}

uint32_t player_gen(void)
{
    return g_gen + audio_gen();
}
