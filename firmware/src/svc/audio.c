#include "svc/audio.h"

#include "svc/audio_pipe.h"
#include "svc/media.h"
#include "svc/vfs.h"

#include <string.h>

#if defined(CORE_CM7)
#include "bsp/board.h"
#include "ipc/ipc.h"
#else
#include "audio_engine.h"
#endif

static audio_state_t g_st;
static uint8_t g_vol = AUDIO_VOL_DEFAULT;
static char g_path[AUDIO_PATH_MAX];
static char g_title[AUDIO_TITLE_MAX];
static audio_stream_t g_info;
static vfs_file_t g_fd = -1;
static uint32_t g_elapsed_ms;
static uint32_t g_underrun;
static uint32_t g_gen;
static uint32_t g_decoded;
static uint32_t g_file_off;
static uint8_t g_eof;
#if !defined(CORE_CM7)
static audio_pipe_t g_pipe;
#endif

static audio_pipe_t *pipe(void)
{
#if defined(CORE_CM7)
    uint8_t *base = (uint8_t *)board_ipc_base();

    if (base == NULL) {
        return NULL;
    }
    return (audio_pipe_t *)(void *)(base + IPC_AUDIO_PIPE_OFF);
#else
    return &g_pipe;
#endif
}

static void bump(void)
{
    g_gen++;
}

static void set_elapsed(uint32_t ms)
{
    uint32_t prev = g_elapsed_ms;

    g_elapsed_ms = ms;
    if ((prev / 1000u) != (ms / 1000u)) {
        bump();
    }
}

static void set_title(const char *path)
{
    const char *slash;
    size_t n;

    g_title[0] = '\0';
    if (path == NULL) {
        return;
    }
    slash = strrchr(path, '/');
    slash = (slash != NULL) ? (slash + 1) : path;
    n = strlen(slash);
    if (n >= AUDIO_TITLE_MAX) {
        n = AUDIO_TITLE_MAX - 1u;
    }
    memcpy(g_title, slash, n);
    g_title[n] = '\0';
}

static void close_fd(void)
{
    if (g_fd >= 0) {
        (void)vfs_close(g_fd);
        g_fd = -1;
    }
}

static void idle(void)
{
    close_fd();
    g_st = AUDIO_ST_IDLE;
    g_eof = 1u;
    audio_pipe_reset(pipe());
#if !defined(CORE_CM7)
    audio_engine_reset();
#endif
}

#if defined(CORE_CM7)
static void send_fmt(uint16_t type)
{
    ipc_audio_fmt_t f;

    memset(&f, 0, sizeof(f));
    f.kind = g_info.kind;
    f.channels = g_info.channels;
    f.bits = g_info.bits;
    f.volume = audio_volume_to_codec(g_vol);
    f.sample_hz = g_info.sample_hz;
    (void)board_ipc_send(IPC_EP_AUDIO, type, &f, (uint16_t)sizeof(f));
}
#endif

err_t audio_play(const char *path)
{
    err_t e;
    audio_pipe_t *p;
    size_t n;

    audio_stop();
    if (path == NULL || path[0] == '\0') {
        return ERR_INVAL;
    }
    n = strlen(path);
    if (n >= AUDIO_PATH_MAX) {
        return ERR_NOSPC;
    }
    e = media_open_audio(path, &g_info);
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_open(path, VFS_O_RD, &g_fd);
    if (e != ERR_OK) {
        return e;
    }
    if (g_info.data_off != 0u) {
        e = vfs_seek(g_fd, g_info.data_off);
        if (e != ERR_OK) {
            close_fd();
            return e;
        }
    }
    memcpy(g_path, path, n + 1u);
    set_title(path);
    g_file_off = g_info.data_off;
    g_elapsed_ms = 0u;
    g_underrun = 0u;
    g_decoded = 0u;
    g_eof = 0u;
    p = pipe();
    audio_pipe_reset(p);
#if defined(CORE_CM7)
    send_fmt(IPC_AUDIO_PLAY);
    e = board_codec_init(g_info.sample_hz, g_vol);
    if (e == ERR_OK) {
        e = board_codec_play();
    }
    if (e != ERR_OK) {
        board_console_puts("codec play fail\r\n");
    }
#else
    (void)audio_engine_start(&g_info);
    audio_engine_set_volume(g_vol);
    audio_engine_set_paused(0u);
#endif
    g_st = AUDIO_ST_PLAY;
    bump();
    return ERR_OK;
}

err_t audio_pause(void)
{
    if (g_st != AUDIO_ST_PLAY) {
        return ERR_INVAL;
    }
    g_st = AUDIO_ST_PAUSE;
#if defined(CORE_CM7)
    (void)board_ipc_send(IPC_EP_AUDIO, IPC_AUDIO_PAUSE, NULL, 0u);
    (void)board_codec_pause();
#else
    audio_engine_set_paused(1u);
#endif
    bump();
    return ERR_OK;
}

err_t audio_resume(void)
{
    if (g_st != AUDIO_ST_PAUSE) {
        return ERR_INVAL;
    }
    g_st = AUDIO_ST_PLAY;
#if defined(CORE_CM7)
    (void)board_ipc_send(IPC_EP_AUDIO, IPC_AUDIO_RESUME, NULL, 0u);
    (void)board_codec_play();
#else
    audio_engine_set_paused(0u);
#endif
    bump();
    return ERR_OK;
}

err_t audio_stop(void)
{
#if defined(CORE_CM7)
    (void)board_ipc_send(IPC_EP_AUDIO, IPC_AUDIO_STOP, NULL, 0u);
    (void)board_codec_stop();
#endif
    idle();
    g_path[0] = '\0';
    g_title[0] = '\0';
    memset(&g_info, 0, sizeof(g_info));
    bump();
    return ERR_OK;
}

err_t audio_set_volume(uint8_t pct)
{
    if (pct > 100u) {
        pct = 100u;
    }
    g_vol = pct;
#if defined(CORE_CM7)
    (void)board_ipc_send(IPC_EP_AUDIO, IPC_AUDIO_VOLUME, &g_vol, 1u);
    (void)board_codec_volume(pct);
#else
    audio_engine_set_volume(pct);
#endif
    bump();
    return ERR_OK;
}

static void feed(void)
{
    uint8_t buf[256];
    size_t n = 0u;
    size_t put;
    audio_pipe_t *p = pipe();

    if (g_st != AUDIO_ST_PLAY || g_fd < 0 || g_eof != 0u || p == NULL) {
        return;
    }
    while (audio_pipe_space(p) >= 64u) {
        if (vfs_read(g_fd, buf, sizeof(buf), &n) != ERR_OK) {
            g_eof = 1u;
            break;
        }
        if (n == 0u) {
            g_eof = 1u;
            break;
        }
        put = audio_pipe_push(p, buf, n);
        g_file_off += (uint32_t)put;
        if (put < n) {
            (void)vfs_seek(g_fd, g_file_off);
            break;
        }
    }
}

void audio_poll(uint32_t now_ms)
{
    int16_t pcm[256];
    uint32_t un = 0u;

    (void)now_ms;
    feed();
#if !defined(CORE_CM7)
    if (g_st == AUDIO_ST_PLAY || g_st == AUDIO_ST_PAUSE) {
        (void)audio_engine_fill(pcm, 128u, pipe(), &un);
        g_decoded = audio_engine_frames();
        if (g_info.sample_hz != 0u) {
            set_elapsed((uint32_t)(((uint64_t)g_decoded * 1000u) / g_info.sample_hz));
        }
        g_underrun += un;
        if (g_eof != 0u && audio_pipe_used(pipe()) == 0u && g_st == AUDIO_ST_PLAY) {
            idle();
            bump();
        }
    }
#else
    (void)pcm;
    (void)un;
#endif
}

void audio_on_peer(uint32_t elapsed_ms, uint16_t underruns, uint8_t ended)
{
    set_elapsed(elapsed_ms);
    g_underrun = underruns;
    if (ended != 0u && g_st != AUDIO_ST_IDLE) {
        idle();
        bump();
    }
}

audio_state_t audio_state(void)
{
    return g_st;
}

uint8_t audio_active(void)
{
    return (g_st == AUDIO_ST_PLAY || g_st == AUDIO_ST_PAUSE) ? 1u : 0u;
}

uint8_t audio_volume(void)
{
    return g_vol;
}

const char *audio_path(void)
{
    return g_path;
}

const char *audio_title(void)
{
    return g_title;
}

uint32_t audio_elapsed_ms(void)
{
    return g_elapsed_ms;
}

uint32_t audio_duration_ms(void)
{
    return g_info.duration_ms;
}

uint32_t audio_underruns(void)
{
    return g_underrun;
}

uint32_t audio_gen(void)
{
    return g_gen;
}

uint32_t audio_decoded_frames(void)
{
    return g_decoded;
}
