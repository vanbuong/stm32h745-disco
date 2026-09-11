#include "bsp/board.h"

#include "cube.h"
#include "wm8994.h"

#include "svc/audio.h"

#include <string.h>

static WM8994_Object_t g_codec;
static uint8_t g_ready;

static int32_t codec_tick(void)
{
    return (int32_t)HAL_GetTick();
}

static int32_t codec_write(uint16_t addr, uint16_t reg, uint8_t *data, uint16_t len)
{
    return board_i2c4_write16(addr, reg, data, len);
}

static int32_t codec_read(uint16_t addr, uint16_t reg, uint8_t *data, uint16_t len)
{
    return board_i2c4_read16(addr, reg, data, len);
}

err_t board_codec_init(uint32_t sample_hz, uint8_t vol_pct)
{
    WM8994_IO_t io;
    WM8994_Init_t init;
    uint32_t id = 0u;
    uint32_t hz = sample_hz;

    if (hz == 0u) {
        hz = 44100u;
    }
    (void)board_i2c4_init();
    memset(&io, 0, sizeof(io));
    io.Address = BOARD_WM8994_ADDR;
    io.WriteReg = codec_write;
    io.ReadReg = codec_read;
    io.GetTick = codec_tick;
    if (WM8994_RegisterBusIO(&g_codec, &io) != WM8994_OK) {
        return ERR_IO;
    }
    if (WM8994_ReadID(&g_codec, &id) != WM8994_OK || id != WM8994_ID) {
        return ERR_NOENT;
    }
    memset(&init, 0, sizeof(init));
    init.InputDevice = WM8994_IN_NONE;
    init.OutputDevice = WM8994_OUT_HEADPHONE;
    init.Frequency = hz;
    init.Resolution = WM8994_RESOLUTION_16b;
    init.Volume = audio_volume_to_codec(vol_pct);
    if (WM8994_Init(&g_codec, &init) != WM8994_OK) {
        return ERR_IO;
    }
    g_ready = 1u;
    return ERR_OK;
}

err_t board_codec_volume(uint8_t pct)
{
    if (g_ready == 0u) {
        return ERR_INVAL;
    }
    if (WM8994_SetVolume(&g_codec, VOLUME_OUTPUT, audio_volume_to_codec(pct)) != WM8994_OK) {
        return ERR_IO;
    }
    return ERR_OK;
}

err_t board_codec_play(void)
{
    if (g_ready == 0u) {
        return ERR_INVAL;
    }
    return (WM8994_Play(&g_codec) == WM8994_OK) ? ERR_OK : ERR_IO;
}

err_t board_codec_pause(void)
{
    if (g_ready == 0u) {
        return ERR_INVAL;
    }
    return (WM8994_Pause(&g_codec) == WM8994_OK) ? ERR_OK : ERR_IO;
}

err_t board_codec_stop(void)
{
    if (g_ready == 0u) {
        return ERR_OK;
    }
    return (WM8994_Stop(&g_codec, WM8994_PDWN_SW) == WM8994_OK) ? ERR_OK : ERR_IO;
}

err_t board_audio_clock_init(uint32_t sample_hz)
{
    RCC_PeriphCLKInitTypeDef p = {0};

    (void)sample_hz;
    /* H745 SAI2/SAI3 share Sai23ClockSelection (there is no Sai2ClockSelection). */
    p.PeriphClockSelection = RCC_PERIPHCLK_SAI23;
    p.Sai23ClockSelection = RCC_SAI2CLKSOURCE_PLL2;
    p.PLL2.PLL2M = 25;
    p.PLL2.PLL2N = 429;
    p.PLL2.PLL2P = 38;
    p.PLL2.PLL2Q = 2;
    p.PLL2.PLL2R = 2;
    p.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_0;
    p.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
    p.PLL2.PLL2FRACN = 0;
    if (HAL_RCCEx_PeriphCLKConfig(&p) != HAL_OK) {
        return ERR_IO;
    }
    return ERR_OK;
}
