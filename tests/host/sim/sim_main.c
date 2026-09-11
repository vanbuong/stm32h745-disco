#include "app/files.h"
#include "bsp/board.h"
#include "svc/vfs.h"
#include "ui/backend.h"
#include "ui/shell.h"
#include "vfs_host.h"

#include "lvgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <time.h>
#endif

static void sleep_ms(uint32_t ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000u);
    ts.tv_nsec = (long)(ms % 1000u) * 1000000L;
    (void)nanosleep(&ts, NULL);
#endif
}

static void default_root(char *out, size_t n, const char *argv0)
{
    size_t i;
    size_t last = 0u;

    if (out == NULL || n < 8u) {
        return;
    }
    if (argv0 == NULL || argv0[0] == '\0') {
        (void)snprintf(out, n, "user");
        return;
    }
    for (i = 0u; argv0[i] != '\0' && i + 1u < n; i++) {
        out[i] = argv0[i];
        if (argv0[i] == '/' || argv0[i] == '\\') {
            last = i;
        }
    }
    if (last == 0u) {
        (void)snprintf(out, n, "user");
        return;
    }
    (void)snprintf(out + last + 1u, n - (last + 1u), "user");
}

static void host_mkdir(const char *path)
{
#ifdef _WIN32
    (void)_mkdir(path);
#else
    (void)mkdir(path, 0755);
#endif
}

static void host_write(const char *dir, const char *rel, const char *payload)
{
    char path[VFS_PATH_MAX * 2u];
    FILE *f;

    (void)snprintf(path, sizeof(path), "%s/%s", dir, rel);
    f = fopen(path, "wb");
    if (f == NULL) {
        return;
    }
    (void)fwrite(payload, 1u, strlen(payload), f);
    (void)fclose(f);
}

static void seed_user(const char *root)
{
    char sub[VFS_PATH_MAX * 2u];

    host_mkdir(root);
    (void)snprintf(sub, sizeof(sub), "%s/sub", root);
    host_mkdir(sub);
    (void)snprintf(sub, sizeof(sub), "%s/empty", root);
    host_mkdir(sub);
    host_write(root, "hello.txt", "hello\n");
    host_write(root, "readme.md", "# hi\n");
    host_write(root, "data.bin", "BIN\n");
    host_write(root, "photo.png", "PNG");
    host_write(root, "song.wav", "RIFF");
    host_write(root, "sub/a.txt", "a");
}

int main(int argc, char **argv)
{
    char root[VFS_PATH_MAX];
    const char *env;
    err_t e;
    uint32_t last;
    uint32_t now;
    uint32_t dt;

    env = getenv("H745_SIM_USER");
    if (argc > 1 && argv[1] != NULL && argv[1][0] != '\0') {
        (void)snprintf(root, sizeof(root), "%s", argv[1]);
    } else if (env != NULL && env[0] != '\0') {
        (void)snprintf(root, sizeof(root), "%s", env);
    } else {
        default_root(root, sizeof(root), (argc > 0) ? argv[0] : NULL);
    }

    if (vfs_host_set_root(root) != ERR_OK) {
        fprintf(stderr, "sim: bad user root\n");
        return 1;
    }
    seed_user(root);
    e = vfs_mount();
    if (e != ERR_OK) {
        fprintf(stderr, "sim: vfs_mount failed (%d) root=%s\n", (int)e, root);
        return 1;
    }

    board_console_puts("host-sim stm32h745-disco s6\n");
    shell_init();
    shell_status_set_storage(1u);
    e = ui_backend_init();
    if (e != ERR_OK) {
        fprintf(stderr, "sim: ui_backend_init failed (%d) — need a display/SDL\n", (int)e);
        return 1;
    }
    board_console_puts("shell ready\n");
    files_load();

    last = lv_tick_get();
    for (;;) {
        now = lv_tick_get();
        dt = now - last;
        last = now;
        if (dt > 100u) {
            dt = 100u;
        }
        shell_tick(dt);
        ui_backend_handler();
        sleep_ms(5u);
    }
}
