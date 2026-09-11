#include "bsp/board.h"

#include <stdio.h>

void board_console_puts(const char *s)
{
    if (s == NULL) {
        return;
    }
    (void)fputs(s, stdout);
    (void)fflush(stdout);
}
