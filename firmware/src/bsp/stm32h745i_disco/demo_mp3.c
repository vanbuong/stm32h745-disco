#include "bsp/board.h"

/*
 * Public-domain Ode to Joy (~80 s, 44.1 kHz stereo, 48 kb/s).
 * Bytes live in demo.mp3; GAS .incbin pulls them into .rodata.
 * Regenerated with scripts/gen_demo_mp3.py so the clip still fits in 1 MiB flash.
 */
#define DEMO_MP3_STR_(x) #x
#define DEMO_MP3_STR(x) DEMO_MP3_STR_(x)
#ifdef DEMO_MP3_FILE
#define DEMO_MP3_PATH DEMO_MP3_STR(DEMO_MP3_FILE)
#else
#define DEMO_MP3_PATH "demo.mp3"
#endif

extern const uint8_t demo_mp3_start[];
extern const uint8_t demo_mp3_end[];

__asm__(".section .rodata.demo_mp3,\"a\"\n"
        ".balign 4\n"
        ".global demo_mp3_start\n"
        "demo_mp3_start:\n"
        ".incbin \"" DEMO_MP3_PATH "\"\n"
        ".global demo_mp3_end\n"
        "demo_mp3_end:\n"
        ".previous\n");

const uint8_t *board_demo_mp3(uint32_t *len)
{
    if (len != NULL) {
        *len = (uint32_t)(demo_mp3_end - demo_mp3_start);
    }
    return demo_mp3_start;
}
