#include "unity.h"

#include "svc/text_view.h"
#include "svc/vfs.h"

#include <string.h>

static void test_crlf_and_utf8(void)
{
    const uint8_t crlf[] = "a\r\nb\rc";
    const uint8_t bad[] = {'o', 'k', (uint8_t)0xFF, 'z', (uint8_t)0xC0, (uint8_t)0x80, 'e'};
    const uint8_t trunc[] = {'A', (uint8_t)0xE2, (uint8_t)0x82};

    TEST_ASSERT_TRUE(text_view_open_mem(crlf, (uint32_t)sizeof(crlf) - 1u) == ERR_OK);
    TEST_ASSERT_TRUE(strcmp(text_view_text(), "a\nb\nc") == 0);
    TEST_ASSERT_TRUE(text_view_status() == ERR_OK);
    text_view_close();

    TEST_ASSERT_TRUE(text_view_open_mem(bad, (uint32_t)sizeof(bad)) == ERR_OK);
    TEST_ASSERT_TRUE(strcmp(text_view_text(), "ok?z??e") == 0);
    text_view_close();

    TEST_ASSERT_TRUE(text_view_open_mem(trunc, (uint32_t)sizeof(trunc)) == ERR_OK);
    TEST_ASSERT_TRUE(strcmp(text_view_text(), "A?") == 0);
    text_view_close();

    {
        const uint8_t vn[] = {0xE1u, 0xBAu, 0xBFu, ' ', 0xF0u, 0x9Fu, 0x98u, 0x80u};
        TEST_ASSERT_TRUE(text_view_open_mem(vn, (uint32_t)sizeof(vn)) == ERR_OK);
        TEST_ASSERT_TRUE((uint8_t)text_view_text()[0] == 0xE1u);
        TEST_ASSERT_TRUE(strstr(text_view_text(), " ") != NULL);
        text_view_close();
    }
    {
        const uint8_t sur[] = {0xEDu, 0xA0u, 0x80u, 'x'};
        TEST_ASSERT_TRUE(text_view_open_mem(sur, (uint32_t)sizeof(sur)) == ERR_OK);
        TEST_ASSERT_TRUE(text_view_text()[0] == '?');
        text_view_close();
    }
}

static void test_window_1mb(void)
{
    static uint8_t big[1024u * 1024u];
    uint32_t i;
    uint32_t off;
    const char *t;

    for (i = 0u; i < sizeof(big); i++) {
        if ((i % 32u) == 31u) {
            big[i] = (uint8_t)'\n';
        } else {
            big[i] = (uint8_t)('0' + (i % 10u));
        }
    }
    TEST_ASSERT_TRUE(text_view_open_mem(big, (uint32_t)sizeof(big)) == ERR_OK);
    TEST_ASSERT_TRUE(text_view_size() == (uint32_t)sizeof(big));
    TEST_ASSERT_TRUE(text_view_win_bytes() <= TEXT_WIN_MAX);
    TEST_ASSERT_TRUE(strlen(text_view_text()) <= TEXT_WIN_MAX);
    TEST_ASSERT_TRUE(text_view_offset() == 0u);
    TEST_ASSERT_TRUE(text_view_progress() < 100u);
    TEST_ASSERT_TRUE(text_view_text()[0] == (char)big[0]);

    off = (uint32_t)((sizeof(big) * 9u) / 10u);
    TEST_ASSERT_TRUE(text_view_set_window(off) == ERR_OK);
    t = text_view_text();
    TEST_ASSERT_TRUE(t != NULL && t[0] != '\0');
    TEST_ASSERT_TRUE(text_view_offset() >= off);
    TEST_ASSERT_TRUE(text_view_win_bytes() <= TEXT_WIN_MAX);
    TEST_ASSERT_TRUE((uint8_t)t[0] == big[text_view_offset()]);
    TEST_ASSERT_TRUE(text_view_progress() >= 80u);

    TEST_ASSERT_TRUE(text_view_page(1) == ERR_OK);
    TEST_ASSERT_TRUE(text_view_offset() > off);
    TEST_ASSERT_TRUE(text_view_page(-1) == ERR_OK);
    text_view_close();
    TEST_ASSERT_TRUE(text_view_text()[0] == '\0');
}

static void test_vfs_and_edges(void)
{
    TEST_ASSERT_TRUE(vfs_mount() == ERR_OK);
    TEST_ASSERT_TRUE(text_view_open("/user/hello.txt") == ERR_OK);
    TEST_ASSERT_TRUE(strcmp(text_view_name(), "hello.txt") == 0);
    TEST_ASSERT_TRUE(strstr(text_view_text(), "hello") != NULL);
    TEST_ASSERT_TRUE(text_view_page(-1) == ERR_OK);
    TEST_ASSERT_TRUE(text_view_page(1) == ERR_OK);
    TEST_ASSERT_TRUE(text_view_page(0) == ERR_OK);
    text_view_close();

    TEST_ASSERT_TRUE(text_view_open(NULL) == ERR_INVAL);
    TEST_ASSERT_TRUE(text_view_open("/user/nope.txt") == ERR_NOENT);
    TEST_ASSERT_TRUE(text_view_open_mem(NULL, 4u) == ERR_INVAL);
    TEST_ASSERT_TRUE(text_view_open_mem((const uint8_t *)"", 0u) == ERR_OK);
    TEST_ASSERT_TRUE(text_view_progress() == 100u);
    text_view_close();
}

void test_text_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_crlf_and_utf8);
    RUN_TEST(test_window_1mb);
    RUN_TEST(test_vfs_and_edges);
}
