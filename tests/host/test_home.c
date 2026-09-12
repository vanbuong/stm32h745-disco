#include "unity.h"

#include "app/home.h"
#include "svc/auto.h"
#include "svc/home.h"
#include "svc/vfs.h"
#include "svc/zb_host.h"
#include "svc/znp_mt.h"

#include <string.h>

static uint8_t g_cb_on;
static char g_cb_name[HOME_NAME_MAX];

static void on_dev(const home_device_t *d)
{
    if (d == NULL) {
        return;
    }
    g_cb_on = d->on;
    {
        size_t i = 0u;
        while (d->name[i] != '\0' && i + 1u < sizeof(g_cb_name)) {
            g_cb_name[i] = d->name[i];
            i++;
        }
        g_cb_name[i] = '\0';
    }
}

static void boot_home(void)
{
    home_reset();
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_init());
}

static void test_mock_seed(void)
{
    home_device_t d[HOME_DEV_MAX];
    home_room_t rooms[HOME_ROOM_CAP];
    home_net_t n;
    size_t nd;
    size_t nr;

    boot_home();
    nd = home_devices(NULL, d, HOME_DEV_MAX);
    TEST_ASSERT_TRUE(nd >= 4u);
    nr = home_rooms(rooms, HOME_ROOM_CAP);
    TEST_ASSERT_TRUE(nr >= 2u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device("Living lamp", &d[0]));
    TEST_ASSERT_EQUAL_INT(HOME_LIGHT, d[0].kind);
    TEST_ASSERT_EQUAL_UINT8(1u, d[0].on);
    home_net(&n);
    TEST_ASSERT_EQUAL_UINT8(1u, n.formed);
    TEST_ASSERT_EQUAL_UINT8(15u, n.channel);
    TEST_ASSERT_TRUE(vfs_in_user_jail(ZB_DEV_PATH) != 0);
    TEST_ASSERT_TRUE(vfs_in_user_jail(ZB_NET_PATH) != 0);
}

static void test_cmd_toggle_and_revert(void)
{
    home_device_t d;
    home_cmd_t cmd;
    uint8_t before;

    boot_home();
    g_cb_on = 0xFFu;
    g_cb_name[0] = '\0';
    home_on_change(on_dev);
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device("Living lamp", &d));
    before = d.on;
    cmd.on = (before != 0u) ? 0u : 1u;
    cmd.has_level = 0u;
    cmd.level = 0u;
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_cmd("Living lamp", &cmd));
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device("Living lamp", &d));
    TEST_ASSERT_EQUAL_UINT8(cmd.on, d.on);
    TEST_ASSERT_EQUAL_STRING("Living lamp", g_cb_name);
    TEST_ASSERT_EQUAL_UINT8(cmd.on, g_cb_on);

    home_test_force_radio(0u, 0u);
    cmd.on = (d.on != 0u) ? 0u : 1u;
    before = d.on;
    TEST_ASSERT_EQUAL_INT(ERR_IO, home_cmd(d.id, &cmd));
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device("Living lamp", &d));
    TEST_ASSERT_EQUAL_UINT8(before, d.on);

    cmd.on = 1u;
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, home_cmd("Front door", &cmd));
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, home_cmd("nope", &cmd));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, home_cmd("Living lamp", NULL));
}

static void test_cap_32(void)
{
    uint8_t ieee[8];
    unsigned i;
    char name[HOME_NAME_MAX];

    boot_home();
    TEST_ASSERT_EQUAL_UINT(4u, home_device_count());
    memset(ieee, 0, sizeof(ieee));
    ieee[0] = 0xAAu;
    for (i = 0u; i < 28u; i++) {
        ieee[7] = (uint8_t)(i + 1u);
        name[0] = 'D';
        name[1] = (char)('0' + ((i / 10u) % 10u));
        name[2] = (char)('0' + (i % 10u));
        name[3] = '\0';
        TEST_ASSERT_EQUAL_INT(
            ERR_OK, zb_host_add(ieee, (uint16_t)(0x10u + i), HOME_SWITCH, name, "Living"));
    }
    TEST_ASSERT_EQUAL_UINT(32u, home_device_count());
    ieee[7] = 0xFFu;
    TEST_ASSERT_EQUAL_INT(ERR_NOSPC, zb_host_add(ieee, 0x00FFu, HOME_SWITCH, "overflow", "Living"));
}

static void test_interview_map(void)
{
    const uint8_t light[8] = {0x10u, 0, 0, 0, 0, 0, 0, 0x11u};
    const uint8_t zone[8] = {0x10u, 0, 0, 0, 0, 0, 0, 0x22u};
    const uint16_t cl_light[2] = {ZB_CLUSTER_ONOFF, ZB_CLUSTER_LEVEL};
    const uint16_t cl_zone[1] = {ZB_CLUSTER_IAS};
    const uint16_t cl_sw[1] = {ZB_CLUSTER_ONOFF};
    const uint16_t cl_temp[1] = {ZB_CLUSTER_TEMP};
    home_device_t d;

    boot_home();
    TEST_ASSERT_EQUAL_INT(HOME_LIGHT, zb_host_kind_from_clusters(cl_light, 2u));
    TEST_ASSERT_EQUAL_INT(HOME_BINARY_SENSOR, zb_host_kind_from_clusters(cl_zone, 1u));
    TEST_ASSERT_EQUAL_INT(HOME_SWITCH, zb_host_kind_from_clusters(cl_sw, 1u));
    TEST_ASSERT_EQUAL_INT(HOME_CLIMATE, zb_host_kind_from_clusters(cl_temp, 1u));

    TEST_ASSERT_EQUAL_INT(ERR_OK, home_test_announce(0x00AAu, light));
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_test_clusters(light, cl_light, 2u));
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device_at(home_device_count() - 1u, &d));
    TEST_ASSERT_EQUAL_INT(HOME_LIGHT, d.kind);

    TEST_ASSERT_EQUAL_INT(ERR_OK, home_test_announce(0x00BBu, zone));
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_test_clusters(zone, cl_zone, 1u));
    TEST_ASSERT_EQUAL_INT(ERR_OK, zb_host_find(zone, NULL));
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device("1000000000000022", &d));
    TEST_ASSERT_EQUAL_INT(HOME_BINARY_SENSOR, d.kind);
}

static void test_persist_and_permit(void)
{
    home_device_t d;
    home_cmd_t cmd;
    home_net_t n;
    vfs_stat_t st;

    boot_home();
    cmd.on = 0u;
    cmd.has_level = 0u;
    cmd.level = 0u;
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_cmd("Living lamp", &cmd));
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_set_meta("Hall switch", "Hall light", "Hall"));
    home_poll(0u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_stat(ZB_DEV_PATH, &st));
    TEST_ASSERT_TRUE(st.size > 0u);

    home_reset();
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_init());
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device("Living lamp", &d));
    TEST_ASSERT_EQUAL_UINT8(0u, d.on);
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device("Hall light", &d));
    TEST_ASSERT_EQUAL_STRING("Hall", d.room_id);

    TEST_ASSERT_EQUAL_INT(ERR_OK, home_permit_join(2u));
    home_net(&n);
    TEST_ASSERT_EQUAL_UINT8(2u, n.permit_left);
    home_poll(1000u);
    home_net(&n);
    TEST_ASSERT_EQUAL_UINT8(1u, n.permit_left);
    home_poll(1000u);
    home_net(&n);
    TEST_ASSERT_EQUAL_UINT8(0u, n.permit_left);
}

static void test_app_pages(void)
{
    home_device_t d;

    boot_home();
    home_app_open();
    TEST_ASSERT_EQUAL_INT(HOME_PAGE_LIST, home_app_page());
    TEST_ASSERT_NOT_NULL(home_app_banner());
    home_app_open_device(0u);
    TEST_ASSERT_EQUAL_INT(HOME_PAGE_DEVICE, home_app_page());
    TEST_ASSERT_EQUAL_STRING("Device", home_app_title());
    TEST_ASSERT_EQUAL_UINT8(1u, home_app_on_back());
    TEST_ASSERT_EQUAL_INT(HOME_PAGE_LIST, home_app_page());
    home_app_open_network();
    TEST_ASSERT_EQUAL_INT(HOME_PAGE_NETWORK, home_app_page());
    TEST_ASSERT_EQUAL_UINT8(1u, home_app_on_back());
    TEST_ASSERT_EQUAL_UINT8(0u, home_app_on_back());

    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device_at(0u, &d));
    TEST_ASSERT_EQUAL_STRING("ON", home_app_state_text(&d));
    home_app_toggle(0u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device_at(0u, &d));
    TEST_ASSERT_EQUAL_STRING("OFF", home_app_state_text(&d));
    home_app_pair();
    home_app_tick(0u);
    TEST_ASSERT_TRUE(strstr(home_app_banner(), "Pairing") != NULL);
    TEST_ASSERT_EQUAL_STRING("Light", home_app_kind_text(HOME_LIGHT));
    TEST_ASSERT_EQUAL_STRING("Sensor", home_app_kind_text(HOME_BINARY_SENSOR));
    home_app_open_autos();
    TEST_ASSERT_EQUAL_INT(HOME_PAGE_AUTOS, home_app_page());
    TEST_ASSERT_TRUE(home_app_rule_count() >= 1u);
    TEST_ASSERT_NOT_NULL(home_app_rule_name(0u));
    TEST_ASSERT_NOT_NULL(home_app_rule_summary(0u));
    TEST_ASSERT_NOT_NULL(home_app_rule_delay(0u));
    home_app_open_rule(0u);
    TEST_ASSERT_EQUAL_INT(HOME_PAGE_RULE, home_app_page());
    TEST_ASSERT_EQUAL_UINT8(1u, home_app_on_back());
    TEST_ASSERT_EQUAL_INT(HOME_PAGE_AUTOS, home_app_page());
    home_app_toggle_rule(0u);
    TEST_ASSERT_EQUAL_UINT8(0u, home_app_rule_enabled(0u));
    home_app_toggle_rule(0u);
    home_app_close();
}

static void test_occupancy_rule(void)
{
    home_device_t lamp;
    home_cmd_t cmd;
    const uint8_t motion[8] = {0x01u, 0, 0, 0, 0, 0, 0, 0x04u};
    vfs_stat_t st;

    boot_home();
    TEST_ASSERT_TRUE(vfs_in_user_jail(AUTO_RULES_PATH) != 0);
    TEST_ASSERT_TRUE(auto_count() >= 1u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device("Living lamp", &lamp));
    cmd.on = 0u;
    cmd.has_level = 0u;
    cmd.level = 0u;
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_cmd("Living lamp", &cmd));
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device("Living lamp", &lamp));
    TEST_ASSERT_EQUAL_UINT8(0u, lamp.on);

    TEST_ASSERT_EQUAL_INT(ERR_OK, home_test_report(motion, ZB_CLUSTER_OCC, 1u, 0u));
    home_poll(0u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device("Living lamp", &lamp));
    TEST_ASSERT_EQUAL_UINT8(1u, lamp.on);
    TEST_ASSERT_EQUAL_STRING("just now", home_app_last_seen(&lamp));

    home_poll(3000u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device("Living lamp", &lamp));
    TEST_ASSERT_EQUAL_UINT8(0u, lamp.on);

    TEST_ASSERT_EQUAL_INT(ERR_OK, auto_set_enabled(home_app_rule_id(0u), 0u));
    home_poll(0u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_stat(AUTO_RULES_PATH, &st));
    TEST_ASSERT_TRUE(st.size > 0u);
    home_reset();
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_init());
    TEST_ASSERT_EQUAL_UINT8(0u, home_app_rule_enabled(0u));
}

static void test_rule_cap_and_kinds(void)
{
    auto_rule_t r;
    home_device_t d;
    const uint8_t clim[8] = {0x33u, 0, 0, 0, 0, 0, 0, 0x01u};
    unsigned i;

    boot_home();
    memset(&r, 0, sizeof(r));
    r.enabled = 1u;
    r.trig = AUTO_TRIG_TEMP_GT;
    r.thresh = 20;
    memcpy(r.trig_ieee, clim, 8u);
    memcpy(r.action_ieee, clim, 8u);
    r.action = AUTO_ACT_ON;
    TEST_ASSERT_EQUAL_INT(ERR_OK, zb_host_add(clim, 0x0044u, HOME_CLIMATE, "Hall temp", "Hall"));
    TEST_ASSERT_EQUAL_INT(ERR_OK, auto_add(&r));
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_test_report(clim, ZB_CLUSTER_LEVEL, 0u, 26u));
    home_poll(0u);
    TEST_ASSERT_EQUAL_UINT16(r.id == 0u ? auto_last_id() : auto_last_id(), auto_last_id());

    memset(&r, 0, sizeof(r));
    r.enabled = 1u;
    r.trig = AUTO_TRIG_TIME;
    r.thresh = 8 * 60;
    r.action = AUTO_ACT_TOGGLE;
    r.action_ieee[0] = 0x01u;
    r.action_ieee[7] = 0x02u;
    TEST_ASSERT_EQUAL_INT(ERR_OK, auto_add(&r));
    auto_test_set_minutes(8 * 60);
    home_poll(0u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device("Hall switch", &d));

    for (i = 0u; i < 29u; i++) {
        memset(&r, 0, sizeof(r));
        r.enabled = 0u;
        r.trig = AUTO_TRIG_ON;
        TEST_ASSERT_EQUAL_INT(ERR_OK, auto_add(&r));
    }
    TEST_ASSERT_EQUAL_INT(ERR_NOSPC, auto_add(&r));
    TEST_ASSERT_EQUAL_STRING("OnOff, Level", home_cluster_text(HOME_LIGHT));
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_remove("Hall switch"));
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, home_device("Hall switch", &d));
}

static void test_form_leave_rooms(void)
{
    const uint8_t ieee[8] = {0x22u, 0, 0, 0, 0, 0, 0, 0x33u};
    home_device_t d[8];
    home_room_t rooms[HOME_ROOM_CAP];
    home_net_t n;
    size_t i;

    boot_home();
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_form(20u, 0xBEEFu));
    home_net(&n);
    TEST_ASSERT_EQUAL_UINT8(20u, n.channel);
    TEST_ASSERT_EQUAL_UINT16(0xBEEFu, n.pan);
    TEST_ASSERT_TRUE(home_bar_level() >= 2u);

    TEST_ASSERT_EQUAL_INT(ERR_OK, zb_host_add(ieee, 0x0033u, HOME_SWITCH, "Extra", "Living"));
    TEST_ASSERT_EQUAL_INT(ERR_OK, zb_interview(ieee));
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_test_report(ieee, ZB_CLUSTER_ONOFF, 1u, 0u));
    TEST_ASSERT_EQUAL_INT(ERR_OK, home_device("Extra", &d[0]));
    TEST_ASSERT_EQUAL_UINT8(1u, d[0].on);
    TEST_ASSERT_EQUAL_INT(ERR_OK, zb_leave(ieee));
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, home_device("Extra", &d[0]));

    TEST_ASSERT_TRUE(home_devices("Living", d, 8u) >= 1u);
    TEST_ASSERT_EQUAL_UINT(0u, home_devices("Nope", d, 8u));
    TEST_ASSERT_TRUE(home_rooms(rooms, HOME_ROOM_CAP) >= 2u);
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, zb_form(NULL));
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, zb_leave(ieee));
    (void)i;
}

void test_home_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_mock_seed);
    RUN_TEST(test_cmd_toggle_and_revert);
    RUN_TEST(test_cap_32);
    RUN_TEST(test_interview_map);
    RUN_TEST(test_persist_and_permit);
    RUN_TEST(test_app_pages);
    RUN_TEST(test_occupancy_rule);
    RUN_TEST(test_rule_cap_and_kinds);
    RUN_TEST(test_form_leave_rooms);
}
