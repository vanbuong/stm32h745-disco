#include "unity.h"

#include "svc/log.h"

#include <stdint.h>
#include <string.h>

static char g_sink[LOG_LINE_MAX];
static size_t g_sink_n;
static uint32_t g_ms;

static void on_line(const char *line, size_t n)
{
    size_t i;

    g_sink_n = n;
    if (line == NULL) {
        g_sink[0] = '\0';
        return;
    }
    i = 0u;
    while (line[i] != '\0' && i + 1u < sizeof(g_sink)) {
        g_sink[i] = line[i];
        i++;
    }
    g_sink[i] = '\0';
}

static uint32_t fake_ms(void)
{
    return g_ms;
}

static void boot_log(void)
{
    log_reset();
    log_init();
    log_set_core("host");
    log_set_sink(on_line);
    g_sink[0] = '\0';
    g_sink_n = 0u;
    g_ms = 0u;
}

static void test_tagged_line_and_sink(void)
{
    boot_log();
    log_write(LOG_INFO, "ZB_CORE", "hello %s", "dev");
    TEST_ASSERT_EQUAL_STRING("host,0,I,ZB_CORE,hello dev", log_last());
    TEST_ASSERT_EQUAL_STRING(log_last(), g_sink);
    TEST_ASSERT_EQUAL_UINT32(strlen(log_last()), (uint32_t)g_sink_n);
    TEST_ASSERT_EQUAL_UINT32(1u, log_count());
    TEST_ASSERT_EQUAL_INT(LOG_INFO, (int)log_level());
}

static void test_timestamp_clock(void)
{
    boot_log();
    g_ms = 1234u;
    log_set_clock(fake_ms);
    log_write(LOG_INFO, "ZB_CORE", "hello %s", "dev");
    TEST_ASSERT_EQUAL_STRING("host,1234,I,ZB_CORE,hello dev", log_last());
    g_ms = 99u;
    log_write(LOG_WARN, "zb", "retry");
    TEST_ASSERT_EQUAL_STRING("host,99,W,zb,retry", log_last());
    log_set_clock(NULL);
    log_write(LOG_INFO, "zb", "def");
    TEST_ASSERT_EQUAL_STRING("host,0,I,zb,def", log_last());
}

static void test_level_filter(void)
{
    boot_log();
    log_write(LOG_DEBUG, "zb", "hidden");
    TEST_ASSERT_EQUAL_STRING("", log_last());
    TEST_ASSERT_EQUAL_UINT32(0u, log_count());
    log_set_level(LOG_DEBUG);
    log_write(LOG_DEBUG, "zb", "shown");
    TEST_ASSERT_EQUAL_STRING("host,0,D,zb,shown", log_last());
    log_set_level((log_lvl_t)9);
    TEST_ASSERT_EQUAL_INT(LOG_DEBUG, (int)log_level());
}

static void test_format_numbers(void)
{
    const uint8_t raw[3] = {0x0Au, 0xBBu, 0x01u};

    boot_log();
    log_write(LOG_ERROR, "ZB_ZNP", "nwk 0x%04X ieee 0x%llx %% %d", 0x12ABu, 0xAABBCCDDEELL, -42);
    TEST_ASSERT_EQUAL_STRING("host,0,E,ZB_ZNP,nwk 0x12AB ieee 0xaabbccddee % -42", log_last());

    log_write(LOG_WARN, "zb", "%c %u %lu %lld %08X", 'Z', 7u, 9ul, -3ll, 0xFFu);
    TEST_ASSERT_NOT_NULL(strstr(log_last(), "host,0,W,zb,Z 7 9 -3 000000FF"));

    log_write(LOG_INFO, NULL, "%s %p", NULL, (void *)0);
    TEST_ASSERT_TRUE(strstr(log_last(), "host,0,I,app,(null) 0x") != NULL);

    log_hex(LOG_INFO, "ZB", raw, 3u);
    TEST_ASSERT_EQUAL_STRING("host,0,I,ZB,0A BB 01", log_last());
    log_hex(LOG_INFO, "ZB", NULL, 4u);
    TEST_ASSERT_EQUAL_STRING("host,0,I,ZB,hex (null)", log_last());
}

static void test_auto_init_and_reset(void)
{
    log_reset();
    log_write(LOG_INFO, "boot", "up");
    TEST_ASSERT_EQUAL_STRING("host,0,I,boot,up", log_last());
    log_reset();
    TEST_ASSERT_EQUAL_STRING("", log_last());
    TEST_ASSERT_EQUAL_UINT32(0u, log_count());
}

void test_log_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_tagged_line_and_sink);
    RUN_TEST(test_timestamp_clock);
    RUN_TEST(test_level_filter);
    RUN_TEST(test_format_numbers);
    RUN_TEST(test_auto_init_and_reset);
}
