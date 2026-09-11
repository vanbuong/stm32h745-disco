/**
 * LVGL v9.5.0 overrides. Unset options take defaults from lv_conf_internal.h.
 * Heap is AXI SRAM (not DTCM .bss). Do not clang-format this file.
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16

#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN

#define LV_MEM_SIZE (96u * 1024u)
#define LV_MEM_ADR 0x24010000u

#define LV_DEF_REFR_PERIOD 33
#define LV_DPI_DEF 128
#define LV_USE_OS LV_OS_NONE

#define LV_USE_DRAW_SW 1
#define LV_USE_DRAW_DMA2D 0
#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1

#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_USE_ANIMIMG 0

#define LV_USE_BUTTON 1
#define LV_USE_LABEL 1
#define LV_USE_LIST 1
#define LV_USE_IMAGE 1
#define LV_USE_BAR 0
#define LV_USE_SLIDER 0
#define LV_USE_SWITCH 0
#define LV_USE_CHECKBOX 0
#define LV_USE_DROPDOWN 0
#define LV_USE_ROLLER 0
#define LV_USE_TEXTAREA 0
#define LV_USE_KEYBOARD 0
#define LV_USE_BUTTONMATRIX 0
#define LV_USE_CALENDAR 0
#define LV_USE_CHART 0
#define LV_USE_TABLE 0
#define LV_USE_TABVIEW 0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0
#define LV_USE_MENU 0
#define LV_USE_MSGBOX 0
#define LV_USE_SPAN 0
#define LV_USE_SPINBOX 0
#define LV_USE_SPINNER 0
#define LV_USE_LED 0
#define LV_USE_LINE 0
#define LV_USE_ARC 0
#define LV_USE_ARCLABEL 0
#define LV_USE_CANVAS 0
#define LV_USE_ANIMIMG 0
#define LV_USE_SCALE 0
#define LV_USE_GIF 0
#define LV_USE_IMAGEBUTTON 0
#define LV_USE_IME_PINYIN 0
#define LV_USE_LOTTIE 0
#define LV_USE_3DTEXTURE 0

#define LV_USE_THORVG_INTERNAL 0
#define LV_USE_THORVG_EXTERNAL 0
#define LV_USE_VECTOR_GRAPHIC 0
#define LV_USE_TINY_TTF 0
#define LV_USE_FREETYPE 0
#define LV_USE_FFMPEG 0
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_XML 0
#define LV_USE_TEST 0
#define LV_USE_MONKEY 0
#define LV_BUILD_EXAMPLES 0
#define LV_USE_DEMO_WIDGETS 0

#endif /* LV_CONF_H */
