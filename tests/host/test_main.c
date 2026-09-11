#include "test.h"

#include <stdio.h>

int g_fails;
int g_checks;

void test_ipc_run(void);
void test_vfs_run(void);
void test_znp_run(void);
void test_media_auto_run(void);
void test_mem_run(void);
void test_disp_run(void);

int main(void)
{
    test_ipc_run();
    test_vfs_run();
    test_znp_run();
    test_media_auto_run();
    test_mem_run();
    test_disp_run();
    printf("%d checks, %d failed\n", g_checks, g_fails);
    return g_fails == 0 ? 0 : 1;
}
