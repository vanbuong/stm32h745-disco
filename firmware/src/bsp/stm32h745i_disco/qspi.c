#include "bsp/board.h"

#include "cube.h"

#include <string.h>

/*
 * Dual QSPI NOR on STM32H745I-DISCO (ST BSP pin map). BK2 PH2/PH3 is the
 * Ethernet CRS/COL mux; Sprint 1 keeps dual-flash. Mmap smoke is 1-1-1 READ
 * of bank 1. Do not execute blank 0xFF NOR.
 */

static QSPI_HandleTypeDef g_qspi;

static void qspi_cmd_defaults(QSPI_CommandTypeDef *c)
{
    memset(c, 0, sizeof(*c));
    c->InstructionMode = QSPI_INSTRUCTION_1_LINE;
    c->AddressMode = QSPI_ADDRESS_NONE;
    c->AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    c->DataMode = QSPI_DATA_NONE;
    c->DummyCycles = 0;
    c->DdrMode = QSPI_DDR_MODE_DISABLE;
    c->DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    c->SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
}

static err_t qspi_instr(uint8_t instr)
{
    QSPI_CommandTypeDef c;

    qspi_cmd_defaults(&c);
    c.Instruction = instr;
    (void)HAL_QSPI_Abort(&g_qspi);
    return cube_err(HAL_QSPI_Command(&g_qspi, &c, HAL_QSPI_TIMEOUT_DEFAULT_VALUE));
}

void HAL_QSPI_MspInit(QSPI_HandleTypeDef *hqspi)
{
    (void)hqspi;
    __HAL_RCC_QSPI_CLK_ENABLE();
    __HAL_RCC_QSPI_FORCE_RESET();
    __HAL_RCC_QSPI_RELEASE_RESET();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    cube_gpio_af(GPIOF, GPIO_PIN_10, GPIO_AF9_QUADSPI, GPIO_NOPULL); /* CLK */
    cube_gpio_af(GPIOG, GPIO_PIN_6, GPIO_AF10_QUADSPI, GPIO_PULLUP); /* NCS */
    cube_gpio_af(GPIOD, GPIO_PIN_11, GPIO_AF9_QUADSPI, GPIO_NOPULL); /* BK1 D0 */
    cube_gpio_af(GPIOF, GPIO_PIN_9, GPIO_AF10_QUADSPI, GPIO_NOPULL); /* BK1 D1 */
    cube_gpio_af(GPIOF, GPIO_PIN_7, GPIO_AF9_QUADSPI, GPIO_NOPULL);  /* BK1 D2 */
    cube_gpio_af(GPIOF, GPIO_PIN_6, GPIO_AF9_QUADSPI, GPIO_NOPULL);  /* BK1 D3 */
    cube_gpio_af(GPIOH, GPIO_PIN_2, GPIO_AF9_QUADSPI, GPIO_NOPULL);  /* BK2 D0 */
    cube_gpio_af(GPIOH, GPIO_PIN_3, GPIO_AF9_QUADSPI, GPIO_NOPULL);  /* BK2 D1 */
    cube_gpio_af(GPIOG, GPIO_PIN_9, GPIO_AF9_QUADSPI, GPIO_NOPULL);  /* BK2 D2 */
    cube_gpio_af(GPIOG, GPIO_PIN_14, GPIO_AF9_QUADSPI, GPIO_NOPULL); /* BK2 D3 */
}

err_t board_qspi_init(void)
{
    err_t e;
    QSPI_CommandTypeDef c;
    QSPI_MemoryMappedTypeDef mmap = {0};

    g_qspi.Instance = QUADSPI;
    g_qspi.Init.ClockPrescaler = 3;
    g_qspi.Init.FifoThreshold = 4;
    g_qspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    g_qspi.Init.FlashSize = 25;
    g_qspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_4_CYCLE;
    g_qspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
    g_qspi.Init.FlashID = QSPI_FLASH_ID_1;
    g_qspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
    if (HAL_QSPI_Init(&g_qspi) != HAL_OK) {
        return ERR_IO;
    }

    e = qspi_instr(0x66u);
    if (e != ERR_OK) {
        return e;
    }
    e = qspi_instr(0x99u);
    if (e != ERR_OK) {
        return e;
    }
    HAL_Delay(1);

    qspi_cmd_defaults(&c);
    c.Instruction = 0x03u;
    c.AddressMode = QSPI_ADDRESS_1_LINE;
    c.AddressSize = QSPI_ADDRESS_24_BITS;
    c.DataMode = QSPI_DATA_1_LINE;
    mmap.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    (void)HAL_QSPI_Abort(&g_qspi);
    return cube_err(HAL_QSPI_MemoryMapped(&g_qspi, &c, &mmap));
}

err_t board_qspi_read_id(uint8_t id[3])
{
    QSPI_CommandTypeDef c;
    err_t e;

    if (id == NULL) {
        return ERR_INVAL;
    }

    qspi_cmd_defaults(&c);
    c.Instruction = 0x9Fu;
    c.DataMode = QSPI_DATA_1_LINE;
    c.NbData = 3;
    (void)HAL_QSPI_Abort(&g_qspi);
    e = cube_err(HAL_QSPI_Command(&g_qspi, &c, HAL_QSPI_TIMEOUT_DEFAULT_VALUE));
    if (e != ERR_OK) {
        return e;
    }
    return cube_err(HAL_QSPI_Receive(&g_qspi, id, HAL_QSPI_TIMEOUT_DEFAULT_VALUE));
}

err_t board_qspi_mmap_probe(uint32_t *first_word)
{
    QSPI_CommandTypeDef c;
    QSPI_MemoryMappedTypeDef mmap = {0};
    volatile uint32_t *q = (volatile uint32_t *)BOARD_QSPI_BASE;
    err_t e;

    qspi_cmd_defaults(&c);
    c.Instruction = 0x03u;
    c.AddressMode = QSPI_ADDRESS_1_LINE;
    c.AddressSize = QSPI_ADDRESS_24_BITS;
    c.DataMode = QSPI_DATA_1_LINE;
    mmap.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    (void)HAL_QSPI_Abort(&g_qspi);
    e = cube_err(HAL_QSPI_MemoryMapped(&g_qspi, &c, &mmap));
    if (e != ERR_OK) {
        return e;
    }
    if (first_word != NULL) {
        *first_word = *q;
    } else {
        (void)*q;
    }
    return ERR_OK;
}
