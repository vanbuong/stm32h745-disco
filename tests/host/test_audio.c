#include "unity.h"

#include "app/player.h"
#include "svc/audio.h"
#include "svc/audio_pipe.h"
#include "svc/media.h"
#include "svc/vfs.h"

#include "audio_engine.h"

#include <stdint.h>
#include <string.h>

err_t vfs_ram_add_file(const char *name, const void *data, uint16_t n);

static void put_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static unsigned make_wav(uint8_t *out, unsigned frames, uint32_t hz, uint16_t ch)
{
    unsigned i;
    unsigned data = frames * (unsigned)ch * 2u;

    memcpy(out, "RIFF", 4);
    put_le32(out + 4, 36u + data);
    memcpy(out + 8, "WAVEfmt ", 8);
    put_le32(out + 16, 16u);
    put_le16(out + 20, 1u);
    put_le16(out + 22, ch);
    put_le32(out + 24, hz);
    put_le32(out + 28, hz * (uint32_t)ch * 2u);
    put_le16(out + 32, (uint16_t)(ch * 2u));
    put_le16(out + 34, 16u);
    memcpy(out + 36, "data", 4);
    put_le32(out + 40, data);
    for (i = 0u; i < frames * (unsigned)ch; i++) {
        int16_t s = (i & 1u) ? (int16_t)1000 : (int16_t)-1000;
        put_le16(out + 44u + i * 2u, (uint16_t)s);
    }
    return 44u + data;
}

static void test_mixer(void)
{
    int16_t z[8];
    int16_t a[8];
    int16_t b[8];
    unsigned i;

    TEST_ASSERT_EQUAL_UINT8(0u, audio_volume_to_codec(0u));
    TEST_ASSERT_EQUAL_UINT8(63u, audio_volume_to_codec(100u));
    TEST_ASSERT_EQUAL_UINT8(31u, audio_volume_to_codec(50u));
    TEST_ASSERT_EQUAL_UINT8(63u, audio_volume_to_codec(200u));
    TEST_ASSERT_EQUAL_INT16(32767, audio_sat16(40000));
    TEST_ASSERT_EQUAL_INT16(-32768, audio_sat16(-40000));
    TEST_ASSERT_EQUAL_INT16(0, audio_sat16(0));

    for (i = 0u; i < 8u; i++) {
        z[i] = 0;
        a[i] = 32767;
        b[i] = 32767;
    }
    audio_mix_add(z, a, 8u);
    audio_mix_add(z, b, 8u);
    for (i = 0u; i < 8u; i++) {
        TEST_ASSERT_EQUAL_INT16(32767, z[i]);
    }
    memset(z, 0, sizeof(z));
    memset(a, 0, sizeof(a));
    audio_mix_add(z, a, 8u);
    for (i = 0u; i < 8u; i++) {
        TEST_ASSERT_EQUAL_INT16(0, z[i]);
    }
    a[0] = 20000;
    audio_apply_volume(a, 1u, 0u);
    TEST_ASSERT_EQUAL_INT16(0, a[0]);
    a[0] = 20000;
    audio_apply_volume(a, 1u, 50u);
    TEST_ASSERT_EQUAL_INT16(10000, a[0]);
}

static void test_pipe(void)
{
    uint8_t src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t dst[8];
    uint8_t bulk[8];
    static audio_pipe_t p;

    audio_pipe_reset(&p);
    TEST_ASSERT_EQUAL_UINT(0u, audio_pipe_used(&p));
    TEST_ASSERT_EQUAL_UINT(AUDIO_PIPE_CAP, audio_pipe_space(&p));
    TEST_ASSERT_EQUAL_UINT(8u, audio_pipe_push(&p, src, 8u));
    TEST_ASSERT_EQUAL_UINT(8u, audio_pipe_used(&p));
    TEST_ASSERT_EQUAL_UINT(3u, audio_pipe_pop(&p, dst, 3u));
    TEST_ASSERT_EQUAL_HEX8(1, dst[0]);
    TEST_ASSERT_EQUAL_HEX8(3, dst[2]);
    TEST_ASSERT_EQUAL_UINT(5u, audio_pipe_used(&p));
    TEST_ASSERT_EQUAL_UINT(5u, audio_pipe_pop(&p, dst, 8u));
    TEST_ASSERT_EQUAL_UINT(0u, audio_pipe_used(&p));
    memset(bulk, 0x5A, sizeof(bulk));
    TEST_ASSERT_EQUAL_UINT(sizeof(bulk), audio_pipe_push(&p, bulk, sizeof(bulk)));
    TEST_ASSERT_EQUAL_UINT(sizeof(bulk), audio_pipe_pop(&p, bulk, sizeof(bulk)));
    TEST_ASSERT_EQUAL_HEX8(0x5A, bulk[0]);
}

static void test_wav_mem(void)
{
    uint8_t wav[256];
    unsigned n;
    audio_stream_t s;
    uint8_t mp3hdr[4] = {0xFFu, 0xFBu, 0x90u, 0x00u};

    n = make_wav(wav, 16u, 8000u, 2u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, media_open_audio_mem(wav, n, &s));
    TEST_ASSERT_EQUAL_UINT8(AUDIO_KIND_PCM, s.kind);
    TEST_ASSERT_EQUAL_UINT8(2u, s.channels);
    TEST_ASSERT_EQUAL_UINT8(16u, s.bits);
    TEST_ASSERT_EQUAL_UINT32(8000u, s.sample_hz);
    TEST_ASSERT_EQUAL_UINT32(44u, s.data_off);
    TEST_ASSERT_EQUAL_UINT32(64u, s.data_bytes);
    TEST_ASSERT_EQUAL_UINT32(2u, s.duration_ms);

    TEST_ASSERT_EQUAL_INT(ERR_INVAL, media_open_audio_mem(NULL, n, &s));
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT, media_open_audio_mem(wav, 8u, &s));
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT,
                          media_open_audio_mem((const uint8_t *)"RIFFXXXXWAVE", 12u, &s));

    TEST_ASSERT_EQUAL_INT(ERR_OK, media_open_audio_mem(mp3hdr, sizeof(mp3hdr), &s));
    TEST_ASSERT_EQUAL_UINT8(AUDIO_KIND_MP3, s.kind);
    TEST_ASSERT_EQUAL_UINT32(44100u, s.sample_hz);
    TEST_ASSERT_EQUAL_UINT8(2u, s.channels);
}

static void test_play_wav(void)
{
    uint8_t wav[256];
    unsigned n;
    unsigned i;

    n = make_wav(wav, 32u, 8000u, 2u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_ram_add_file("beep.wav", wav, (uint16_t)n));
    TEST_ASSERT_EQUAL_INT(ERR_OK, audio_play("/user/beep.wav"));
    TEST_ASSERT_EQUAL_INT(AUDIO_ST_PLAY, audio_state());
    TEST_ASSERT_EQUAL_UINT8(1u, audio_active());
    TEST_ASSERT_EQUAL_STRING("beep.wav", audio_title());
    TEST_ASSERT_EQUAL_INT(ERR_OK, audio_pause());
    TEST_ASSERT_EQUAL_INT(AUDIO_ST_PAUSE, audio_state());
    TEST_ASSERT_EQUAL_INT(ERR_OK, audio_resume());
    TEST_ASSERT_EQUAL_INT(AUDIO_ST_PLAY, audio_state());
    for (i = 0u; i < 8u; i++) {
        audio_poll(i * 10u);
    }
    TEST_ASSERT_GREATER_THAN_UINT32(0u, audio_decoded_frames());
    TEST_ASSERT_EQUAL_INT(ERR_OK, audio_set_volume(40u));
    TEST_ASSERT_EQUAL_UINT8(40u, audio_volume());
    TEST_ASSERT_EQUAL_INT(ERR_OK, audio_stop());
    TEST_ASSERT_EQUAL_INT(AUDIO_ST_IDLE, audio_state());
    TEST_ASSERT_EQUAL_UINT8(0u, audio_active());
}

static void test_player(void)
{
    uint8_t wav[256];
    unsigned n;

    n = make_wav(wav, 16u, 8000u, 1u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_ram_add_file("tone.wav", wav, (uint16_t)n));
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_ram_add_file("zing.wav", wav, (uint16_t)n));
    TEST_ASSERT_EQUAL_INT(ERR_OK, player_open("/user/tone.wav"));
    TEST_ASSERT_EQUAL_INT(ERR_OK, player_status());
    TEST_ASSERT_EQUAL_STRING("tone.wav", player_title());
    TEST_ASSERT_EQUAL_UINT8(1u, player_playing());
    player_toggle();
    TEST_ASSERT_EQUAL_UINT8(0u, player_playing());
    player_toggle();
    TEST_ASSERT_EQUAL_UINT8(1u, player_playing());
    TEST_ASSERT_EQUAL_INT(ERR_OK, player_next(1));
    TEST_ASSERT_EQUAL_STRING("zing.wav", player_title());
    player_close();
    TEST_ASSERT_EQUAL_INT(ERR_OK, audio_stop());
    TEST_ASSERT_EQUAL_INT(AUDIO_ST_IDLE, audio_state());
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, audio_pause());
}

static void test_engine_and_errors(void)
{
    uint8_t wav[256];
    uint8_t bad[64];
    uint8_t mp3[8] = {0xFFu, 0xFBu, 0x90u, 0x00u, 0u, 0u, 0u, 0u};
    audio_stream_t s;
    static audio_pipe_t p;
    int16_t pcm[16];
    uint32_t un = 0u;
    unsigned n;

    TEST_ASSERT_EQUAL_INT(ERR_INVAL, audio_play(NULL));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, audio_play(""));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, media_open_audio(NULL, &s));
    TEST_ASSERT_EQUAL_INT(ERR_UNSUPPORTED, media_open_audio("/user/hello.txt", &s));

    n = make_wav(wav, 8u, 8000u, 2u);
    wav[20] = 3u; /* IEEE float, not PCM */
    TEST_ASSERT_EQUAL_INT(ERR_UNSUPPORTED, media_open_audio_mem(wav, n, &s));

    memset(bad, 0, sizeof(bad));
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT, media_open_audio_mem(bad, sizeof(bad), &s));

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT, media_open_audio("/user/song.wav", &s));
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_ram_add_file("clip.mp3", mp3, (uint16_t)sizeof(mp3)));
    TEST_ASSERT_EQUAL_INT(ERR_OK, media_open_audio("/user/clip.mp3", &s));
    TEST_ASSERT_EQUAL_UINT8(AUDIO_KIND_MP3, s.kind);
    TEST_ASSERT_EQUAL_INT(ERR_OK, audio_play("/user/clip.mp3"));
    audio_poll(0u);
    audio_poll(10u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, audio_stop());

    n = make_wav(wav, 8u, 8000u, 1u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, media_open_audio_mem(wav, n, &s));
    audio_pipe_reset(&p);
    TEST_ASSERT_EQUAL_UINT((size_t)(n - 44u), audio_pipe_push(&p, wav + 44, (size_t)(n - 44u)));
    TEST_ASSERT_EQUAL_UINT(0u, audio_pipe_push(NULL, wav, 4u));
    TEST_ASSERT_EQUAL_UINT(0u, audio_pipe_pop(NULL, wav, 4u));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, audio_engine_start(NULL));
    TEST_ASSERT_EQUAL_INT(ERR_OK, audio_engine_start(&s));
    audio_engine_set_volume(80u);
    un = 0u;
    TEST_ASSERT_EQUAL_UINT(0u, audio_engine_fill(NULL, 4u, &p, &un));
    TEST_ASSERT_EQUAL_UINT(4u, audio_engine_fill(pcm, 4u, &p, &un));
    TEST_ASSERT_GREATER_THAN_UINT32(0u, audio_engine_frames());
    TEST_ASSERT_EQUAL_UINT(8u, audio_engine_fill(pcm, 8u, &p, &un));
    TEST_ASSERT_GREATER_THAN_UINT32(0u, un);
    audio_engine_set_paused(1u);
    TEST_ASSERT_EQUAL_UINT(4u, audio_engine_fill(pcm, 4u, &p, NULL));
    audio_engine_reset();

    audio_on_peer(1500u, 3u, 0u);
    TEST_ASSERT_EQUAL_UINT32(1500u, audio_elapsed_ms());
    TEST_ASSERT_EQUAL_UINT32(3u, audio_underruns());
    n = make_wav(wav, 8u, 8000u, 2u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_ram_add_file("x.wav", wav, (uint16_t)n));
    TEST_ASSERT_EQUAL_INT(ERR_OK, audio_play("/user/x.wav"));
    audio_on_peer(0u, 0u, 1u);
    TEST_ASSERT_EQUAL_INT(AUDIO_ST_IDLE, audio_state());
}

static void test_player_volume(void)
{
    uint8_t wav[256];
    unsigned n;

    n = make_wav(wav, 8u, 8000u, 2u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_ram_add_file("vol.wav", wav, (uint16_t)n));
    TEST_ASSERT_EQUAL_INT(ERR_OK, player_open("/user/vol.wav"));
    player_set_volume(10u);
    TEST_ASSERT_EQUAL_UINT8(10u, player_volume());
    player_set_volume(200u);
    TEST_ASSERT_EQUAL_UINT8(100u, player_volume());
    TEST_ASSERT_EQUAL_UINT32(0u, player_elapsed_ms());
    TEST_ASSERT_GREATER_THAN_UINT32(0u, player_duration_ms());
    TEST_ASSERT_EQUAL_STRING("/user/vol.wav", player_path());
    player_close();
    TEST_ASSERT_EQUAL_INT(ERR_OK, audio_stop());
}

void test_audio_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_mixer);
    RUN_TEST(test_pipe);
    RUN_TEST(test_wav_mem);
    RUN_TEST(test_play_wav);
    RUN_TEST(test_player);
    RUN_TEST(test_engine_and_errors);
    RUN_TEST(test_player_volume);
}
