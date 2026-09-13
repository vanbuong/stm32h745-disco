#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_ipc_run(void);
void test_vfs_run(void);
void test_znp_run(void);
void test_media_auto_run(void);
void test_mem_run(void);
void test_disp_run(void);
void test_shell_run(void);
void test_files_run(void);
void test_text_run(void);
void test_image_run(void);
void test_audio_run(void);
void test_net_run(void);
void test_game_run(void);
void test_home_run(void);
void test_health_run(void);
void test_log_run(void);

int main(void)
{
    UNITY_BEGIN();
    test_ipc_run();
    test_vfs_run();
    test_znp_run();
    test_media_auto_run();
    test_mem_run();
    test_disp_run();
    test_shell_run();
    test_files_run();
    test_text_run();
    test_image_run();
    test_audio_run();
    test_net_run();
    test_game_run();
    test_home_run();
    test_health_run();
    test_log_run();
    return UNITY_END();
}
