#include "unity.h"

#include "app/home.h"
#include "app/settings.h"
#include "fw_version.h"
#include "hal/wdog.h"
#include "svc/audio.h"
#include "svc/cfg.h"
#include "svc/health.h"
#include "svc/home.h"
#include "svc/vfs.h"

#include <string.h>

static void boot_cfg(void)
{
    cfg_reset();
    health_init();
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_init());
}

static void test_cfg_defaults_and_clamp(void)
{
    boot_cfg();
    TEST_ASSERT_EQUAL_UINT8(CFG_BRIGHT_DEFAULT, cfg_brightness());
    TEST_ASSERT_EQUAL_UINT8(CFG_VOL_DEFAULT, cfg_volume());
    TEST_ASSERT_EQUAL_UINT8(CFG_ZB_CH_DEFAULT, cfg_zb_channel());
    TEST_ASSERT_EQUAL_UINT8(CFG_JOIN_S_DEFAULT, cfg_join_s());
    TEST_ASSERT_TRUE(vfs_in_user_jail(CFG_PATH) != 0);

    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_set_brightness(200u));
    TEST_ASSERT_EQUAL_UINT8(100u, cfg_brightness());
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_set_brightness(1u));
    TEST_ASSERT_EQUAL_UINT8(CFG_BRIGHT_MIN, cfg_brightness());
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_set_volume(200u));
    TEST_ASSERT_EQUAL_UINT8(100u, cfg_volume());
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_set_zb_channel(5u));
    TEST_ASSERT_EQUAL_UINT8(CFG_ZB_CH_MIN, cfg_zb_channel());
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_set_zb_channel(40u));
    TEST_ASSERT_EQUAL_UINT8(CFG_ZB_CH_MAX, cfg_zb_channel());
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_set_join_s(0u));
    TEST_ASSERT_EQUAL_UINT8(1u, cfg_join_s());
}

static void test_cfg_persist_and_remount(void)
{
    vfs_stat_t st;

    boot_cfg();
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_set_brightness(55u));
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_set_volume(40u));
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_set_zb_channel(20u));
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_set_join_s(30u));
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_stat(CFG_PATH, &st));
    TEST_ASSERT_EQUAL_UINT32(8u, st.size);

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_unmount());
    TEST_ASSERT_EQUAL_INT(0, vfs_mounted());
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_remount());
    TEST_ASSERT_EQUAL_INT(1, vfs_mounted());
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_stat("/user/hello.txt", &st));

    cfg_reset();
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_init());
    TEST_ASSERT_EQUAL_UINT8(55u, cfg_brightness());
    TEST_ASSERT_EQUAL_UINT8(40u, cfg_volume());
    TEST_ASSERT_EQUAL_UINT8(20u, cfg_zb_channel());
    TEST_ASSERT_EQUAL_UINT8(30u, cfg_join_s());
}

static void test_cfg_bad_magic(void)
{
    vfs_file_t fd = -1;
    size_t put = 0u;
    const char junk[] = "XXXX----";

    boot_cfg();
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_open(CFG_PATH, VFS_O_WR | VFS_O_CREAT | VFS_O_TRUNC, &fd));
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_write(fd, junk, 8u, &put));
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_close(fd));
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_init());
    TEST_ASSERT_EQUAL_UINT8(CFG_BRIGHT_DEFAULT, cfg_brightness());
}

static void test_health_watchdog_and_oom(void)
{
    TEST_ASSERT_EQUAL_INT(ERR_OK, health_init());
    TEST_ASSERT_EQUAL_UINT8(1u, health_ready());
    TEST_ASSERT_EQUAL_UINT8(0u, health_expired());
    TEST_ASSERT_EQUAL_UINT8(1u, health_peer_ok());
    TEST_ASSERT_EQUAL_INT(HEALTH_REASON_NONE, health_reason());
    TEST_ASSERT_EQUAL_STRING("ok", health_reason_text(HEALTH_REASON_NONE));
    TEST_ASSERT_EQUAL_STRING("watchdog", health_reason_text(HEALTH_REASON_WDOG));
    TEST_ASSERT_EQUAL_STRING("brown-out", health_reason_text(HEALTH_REASON_BOR));
    TEST_ASSERT_EQUAL_STRING("out of memory", health_reason_text(HEALTH_REASON_OOM));

    health_kick();
    health_poll(100u);
    TEST_ASSERT_EQUAL_UINT8(0u, health_expired());
    health_poll(HEALTH_WDOG_MS + 1u);
    TEST_ASSERT_EQUAL_UINT8(1u, health_expired());
    TEST_ASSERT_EQUAL_INT(HEALTH_REASON_WDOG, health_reason());

    TEST_ASSERT_EQUAL_INT(ERR_OK, health_init());
    health_mark_oom();
    TEST_ASSERT_EQUAL_UINT8(1u, health_oom());
    TEST_ASSERT_EQUAL_INT(HEALTH_REASON_OOM, health_reason());
    health_note_peer(0u);
    TEST_ASSERT_EQUAL_UINT8(0u, health_peer_ok());
    health_note_peer(1u);
    TEST_ASSERT_EQUAL_UINT8(1u, health_peer_ok());
}

static void test_health_selftest_pack(void)
{
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_init());
    TEST_ASSERT_EQUAL_INT(ERR_OK, health_init());
    TEST_ASSERT_EQUAL_INT(ERR_OK, wdog_start());
    TEST_ASSERT_EQUAL_UINT8(1u, wdog_started());
    health_kick();
    TEST_ASSERT_EQUAL_INT(ERR_OK, health_selftest());
    TEST_ASSERT_EQUAL_INT(1, vfs_mounted());
    {
        vfs_stat_t st;
        TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_stat("/user/hello.txt", &st));
        TEST_ASSERT_EQUAL_UINT8(0u, st.is_dir);
    }
}

static void test_cfg_poll_after_unmount(void)
{
    vfs_stat_t st;

    boot_cfg();
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_unmount());
    TEST_ASSERT_EQUAL_INT(ERR_IO, cfg_set_brightness(70u));
    TEST_ASSERT_EQUAL_UINT8(70u, cfg_brightness());
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_remount());
    cfg_poll();
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_stat(CFG_PATH, &st));
    TEST_ASSERT_EQUAL_UINT32(8u, st.size);
    cfg_reset();
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_init());
    TEST_ASSERT_EQUAL_UINT8(70u, cfg_brightness());
}

static void test_health_not_ready_and_selftest_fail(void)
{
    cfg_reset();
    health_reset();
    health_kick();
    health_poll(100u);
    TEST_ASSERT_EQUAL_UINT8(0u, health_ready());
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    TEST_ASSERT_EQUAL_INT(ERR_OK, health_selftest());
    TEST_ASSERT_EQUAL_UINT8(1u, health_ready());

    health_kick();
    health_poll(HEALTH_WDOG_MS + 5u);
    TEST_ASSERT_EQUAL_UINT8(1u, health_expired());
    TEST_ASSERT_EQUAL_INT(ERR_IO, health_selftest());
}

static void test_settings_about_and_nudge(void)
{
    const char *about;
    const char *line;

    boot_cfg();
    (void)audio_set_volume(CFG_VOL_DEFAULT);
    settings_open();
    about = settings_about();
    TEST_ASSERT_NOT_NULL(about);
    TEST_ASSERT_NOT_NULL(strstr(about, "M7 " FW_VERSION_M7));
    TEST_ASSERT_NOT_NULL(strstr(about, "M4 " FW_VERSION_M4));
    TEST_ASSERT_EQUAL_STRING(FW_VERSION_M7, FW_VERSION_M4);

    settings_nudge_brightness(-20);
    TEST_ASSERT_EQUAL_UINT8(80u, settings_brightness());
    settings_nudge_volume(10);
    TEST_ASSERT_EQUAL_UINT8(80u, settings_volume());
    settings_nudge_zb_channel(1);
    TEST_ASSERT_EQUAL_UINT8(16u, settings_zb_channel());
    settings_nudge_join_s(-10);
    TEST_ASSERT_EQUAL_UINT8(50u, settings_join_s());

    line = settings_bright_line();
    TEST_ASSERT_NOT_NULL(strstr(line, "80"));
    line = settings_vol_line();
    TEST_ASSERT_NOT_NULL(strstr(line, "80"));
    line = settings_zb_line();
    TEST_ASSERT_NOT_NULL(strstr(line, "16"));
    TEST_ASSERT_NOT_NULL(strstr(line, "50"));

    settings_nudge_brightness(-100);
    TEST_ASSERT_EQUAL_UINT8(CFG_BRIGHT_MIN, settings_brightness());
    settings_nudge_brightness(200);
    TEST_ASSERT_EQUAL_UINT8(100u, settings_brightness());
    settings_nudge_volume(-200);
    TEST_ASSERT_EQUAL_UINT8(0u, settings_volume());
    settings_nudge_zb_channel(-20);
    TEST_ASSERT_EQUAL_UINT8(CFG_ZB_CH_MIN, settings_zb_channel());
    settings_nudge_zb_channel(40);
    TEST_ASSERT_EQUAL_UINT8(CFG_ZB_CH_MAX, settings_zb_channel());
    settings_nudge_join_s(-300);
    TEST_ASSERT_EQUAL_UINT8(1u, settings_join_s());
    settings_nudge_join_s(300);
    TEST_ASSERT_EQUAL_UINT8(254u, settings_join_s());
    TEST_ASSERT_EQUAL_STRING("", settings_banner());

    health_mark_oom();
    settings_tick(10u);
    TEST_ASSERT_NOT_NULL(strstr(settings_banner(), "memory"));
    settings_close();
    TEST_ASSERT_TRUE(settings_gen() > 0u);

    home_reset();
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_init());
    TEST_ASSERT_EQUAL_INT(ERR_OK, cfg_set_join_s(12u));
    home_app_pair();
    {
        home_net_t n;
        home_net(&n);
        TEST_ASSERT_EQUAL_UINT8(12u, n.permit_left);
    }
}

void test_health_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_cfg_defaults_and_clamp);
    RUN_TEST(test_cfg_persist_and_remount);
    RUN_TEST(test_cfg_bad_magic);
    RUN_TEST(test_cfg_poll_after_unmount);
    RUN_TEST(test_health_not_ready_and_selftest_fail);
    RUN_TEST(test_health_watchdog_and_oom);
    RUN_TEST(test_health_selftest_pack);
    RUN_TEST(test_settings_about_and_nudge);
}
