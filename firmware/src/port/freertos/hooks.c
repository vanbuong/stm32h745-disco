#include "FreeRTOS.h"
#include "task.h"

#include "bsp/board.h"

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    board_console_puts("rtos stack ");
    board_console_puts(name != NULL ? name : "?");
    board_console_puts("\r\n");
    for (;;) {
    }
}

void vApplicationMallocFailedHook(void)
{
    board_console_puts("rtos oom\r\n");
    for (;;) {
    }
}
