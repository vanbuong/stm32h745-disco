#include "unity.h"

#include "svc/text_view.h"
#include "svc/vfs.h"

#include <string.h>

static void test_crlf_and_utf8(void)
{
    const uint8_t crlf[] = "a\r\nb\rc";
    const uint8_t bad[] = {'o', 'k', (uint8_t)0xFF, 'z', (uint8_t)0xC0, (uint8_t)0x80, 'e'};
    const uint8_t trunc[] = {'A', (uint8_t)0xE2, (uint8_t)0x82};

    TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_open_mem(crlf, (uint32_t)sizeof(crlf) - 1u));
    TEST_ASSERT_EQUAL_STRING("a\nb\nc", text_view_text());
    TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_status());
    text_view_close();

    TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_open_mem(bad, (uint32_t)sizeof(bad)));
    TEST_ASSERT_EQUAL_STRING("ok?z??e", text_view_text());
    text_view_close();

    TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_open_mem(trunc, (uint32_t)sizeof(trunc)));
    TEST_ASSERT_EQUAL_STRING("A?", text_view_text());
    text_view_close();

    {
        const uint8_t vn[] = {0xE1u, 0xBAu, 0xBFu, ' ', 0xF0u, 0x9Fu, 0x98u, 0x80u};
        TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_open_mem(vn, (uint32_t)sizeof(vn)));
        TEST_ASSERT_EQUAL_HEX8(0xE1u, (uint8_t)text_view_text()[0]);
        TEST_ASSERT_NOT_NULL(strstr(text_view_text(), " "));
        text_view_close();
    }
    {
        const uint8_t sur[] = {0xEDu, 0xA0u, 0x80u, 'x'};
        TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_open_mem(sur, (uint32_t)sizeof(sur)));
        TEST_ASSERT_EQUAL_CHAR('?', text_view_text()[0]);
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
    TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_open_mem(big, (uint32_t)sizeof(big)));
    TEST_ASSERT_EQUAL_UINT32((uint32_t)sizeof(big), text_view_size());
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(TEXT_WIN_MAX, text_view_win_bytes());
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(TEXT_WIN_MAX, (uint32_t)strlen(text_view_text()));
    TEST_ASSERT_EQUAL_UINT32(0u, text_view_offset());
    TEST_ASSERT_LESS_THAN_UINT32(100u, text_view_progress());
    TEST_ASSERT_EQUAL_CHAR((char)big[0], text_view_text()[0]);

    off = (uint32_t)((sizeof(big) * 9u) / 10u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_set_window(off));
    t = text_view_text();
    TEST_ASSERT_NOT_NULL(t);
    TEST_ASSERT_NOT_EQUAL_CHAR('\0', t[0]);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(off, text_view_offset());
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(TEXT_WIN_MAX, text_view_win_bytes());
    TEST_ASSERT_EQUAL_HEX8(big[text_view_offset()], (uint8_t)t[0]);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(80u, text_view_progress());

    TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_page(1));
    TEST_ASSERT_GREATER_THAN_UINT32(off, text_view_offset());
    TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_page(-1));
    text_view_close();
    TEST_ASSERT_EQUAL_CHAR('\0', text_view_text()[0]);
}

static void test_vfs_and_edges(void)
{
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_open("/user/hello.txt"));
    TEST_ASSERT_EQUAL_STRING("hello.txt", text_view_name());
    TEST_ASSERT_NOT_NULL(strstr(text_view_text(), "hello"));
    TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_page(-1));
    TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_page(1));
    TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_page(0));
    text_view_close();

    TEST_ASSERT_EQUAL_INT(ERR_INVAL, text_view_open(NULL));
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, text_view_open("/user/nope.txt"));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, text_view_open_mem(NULL, 4u));
    TEST_ASSERT_EQUAL_INT(ERR_OK, text_view_open_mem((const uint8_t *)"", 0u));
    TEST_ASSERT_EQUAL_UINT32(100u, text_view_progress());
    text_view_close();
}

void test_text_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_crlf_and_utf8);
    RUN_TEST(test_window_1mb);
    RUN_TEST(test_vfs_and_edges);
}
