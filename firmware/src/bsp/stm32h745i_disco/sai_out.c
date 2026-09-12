#include "hal/audio_out.h"

#include "cube.h"

#include <string.h>

#define SAI_HALF_FRAMES 512u
#define SAI_CH 2u

static SAI_HandleTypeDef g_sai;
static DMA_HandleTypeDef g_dma;
static int16_t g_pcm[2][SAI_HALF_FRAMES * SAI_CH] __attribute__((aligned(32)));
static uint8_t g_run;
static uint32_t g_hz;

static uint32_t sai_freq(uint32_t hz)
{
    if (hz <= 8000u) {
        return SAI_AUDIO_FREQUENCY_8K;
    }
    if (hz <= 11025u) {
        return SAI_AUDIO_FREQUENCY_11K;
    }
    if (hz <= 16000u) {
        return SAI_AUDIO_FREQUENCY_16K;
    }
    if (hz <= 22050u) {
        return SAI_AUDIO_FREQUENCY_22K;
    }
    if (hz <= 32000u) {
        return SAI_AUDIO_FREQUENCY_32K;
    }
    if (hz <= 44100u) {
        return SAI_AUDIO_FREQUENCY_44K;
    }
    return SAI_AUDIO_FREQUENCY_48K;
}

void HAL_SAI_MspInit(SAI_HandleTypeDef *hsai)
{
    GPIO_InitTypeDef g = {0};

    if (hsai->Instance != SAI2_Block_A) {
        return;
    }
    __HAL_RCC_SAI2_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    g.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF10_SAI2;
    HAL_GPIO_Init(GPIOI, &g);

    g_dma.Instance = DMA2_Stream1;
    g_dma.Init.Request = DMA_REQUEST_SAI2_A;
    g_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    g_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    g_dma.Init.MemInc = DMA_MINC_ENABLE;
    g_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    g_dma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    g_dma.Init.Mode = DMA_CIRCULAR;
    g_dma.Init.Priority = DMA_PRIORITY_HIGH;
    /* FIFO mode scrambled 16-bit samples when HT/TC are polled without IRQs. */
    g_dma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    (void)HAL_DMA_Init(&g_dma);
    __HAL_LINKDMA(hsai, hdmatx, g_dma);
}

err_t audio_out_start(uint32_t sample_hz, uint8_t channels)
{
    (void)channels;
    if (sample_hz == 0u) {
        sample_hz = 44100u;
    }
    /* Keep MCLK running across tracks. Restarting SAI pops the codec. */
    if (g_run != 0u && g_hz == sample_hz) {
        return ERR_OK;
    }
    memset(g_pcm, 0, sizeof(g_pcm));

    if (g_run != 0u) {
        (void)audio_out_stop();
    }

    memset(&g_sai, 0, sizeof(g_sai));
    g_sai.Instance = SAI2_Block_A;
    g_sai.Init.AudioMode = SAI_MODEMASTER_TX;
    g_sai.Init.Synchro = SAI_ASYNCHRONOUS;
    g_sai.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
    g_sai.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
    g_sai.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
    g_sai.Init.AudioFrequency = sai_freq(sample_hz);
    g_sai.Init.Mckdiv = 0u;
    g_sai.Init.MonoStereoMode = SAI_STEREOMODE;
    g_sai.Init.CompandingMode = SAI_NOCOMPANDING;
    g_sai.Init.TriState = SAI_OUTPUT_NOTRELEASED;
    g_sai.Init.MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE;
    g_sai.Init.MckOutput = SAI_MCK_OUTPUT_ENABLE;
    /*
     * WM8994 I2S 16-bit wants 32-bit slots (64 BCLKs/frame). Tight 16-bit
     * slots (32 BCLKs/frame) leave the codec sampling the next word as hiss.
     */
    if (HAL_SAI_InitProtocol(&g_sai, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_16BITEXTENDED, 2u) !=
        HAL_OK) {
        return ERR_IO;
    }
    if (HAL_SAI_Transmit_DMA(&g_sai, (uint8_t *)g_pcm[0],
                             (uint16_t)(SAI_HALF_FRAMES * SAI_CH * 2u)) != HAL_OK) {
        return ERR_IO;
    }
    /* Poll HT/TC; do not enable NVIC (Sprint 7 vector table stays the 16 exceptions). */
    HAL_NVIC_DisableIRQ(DMA2_Stream1_IRQn);
    HAL_NVIC_DisableIRQ(SAI2_IRQn);
    g_hz = sample_hz;
    g_run = 1u;
    return ERR_OK;
}

err_t audio_out_write(const int16_t *pcm, size_t samples)
{
    (void)pcm;
    (void)samples;
    return ERR_OK;
}

err_t audio_out_stop(void)
{
    if (g_run != 0u) {
        (void)HAL_SAI_DMAStop(&g_sai);
        (void)HAL_SAI_DeInit(&g_sai);
        g_run = 0u;
        g_hz = 0u;
    }
    return ERR_OK;
}

uint8_t audio_out_half_ready(int16_t **pcm, size_t *frames)
{
    if (g_run == 0u || pcm == NULL || frames == NULL) {
        return 0u;
    }
    if (__HAL_DMA_GET_FLAG(&g_dma, DMA_FLAG_HTIF1_5) != 0u) {
        __HAL_DMA_CLEAR_FLAG(&g_dma, DMA_FLAG_HTIF1_5);
        *pcm = g_pcm[0];
        *frames = SAI_HALF_FRAMES;
        return 1u;
    }
    if (__HAL_DMA_GET_FLAG(&g_dma, DMA_FLAG_TCIF1_5) != 0u) {
        __HAL_DMA_CLEAR_FLAG(&g_dma, DMA_FLAG_TCIF1_5);
        *pcm = g_pcm[1];
        *frames = SAI_HALF_FRAMES;
        return 2u;
    }
    return 0u;
}
