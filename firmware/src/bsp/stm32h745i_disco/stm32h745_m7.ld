ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    AXIRAM (rwx): ORIGIN = 0x24000000, LENGTH = 512K
    SRAM3 (rwx) : ORIGIN = 0x30040000, LENGTH = 32K
}

_estack = ORIGIN(DTCM) + LENGTH(DTCM);

SECTIONS
{
    .isr_vector :
    {
        KEEP(*(.isr_vector))
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH

    .data : ALIGN(4)
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > DTCM AT > FLASH

    _sidata = LOADADDR(.data);

    .bss (NOLOAD) : ALIGN(4)
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > DTCM

    .dma_buf (NOLOAD) : ALIGN(32)
    {
        KEEP(*(.dma_buf))
        . = ALIGN(32);
    } > AXIRAM

    .eth_dma (NOLOAD) : ALIGN(32)
    {
        KEEP(*(.eth_dma))
        . = ALIGN(32);
    } > SRAM3
}
