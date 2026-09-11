#include "unity.h"

#include "ipc/ipc.h"

#include <stdint.h>
#include <string.h>

#define NSLOTS 4u

static uint8_t storage[NSLOTS * (sizeof(ipc_msg_hdr_t) + IPC_PAYLOAD_MAX)];
static uint8_t shm[IPC_SHM_BYTES];
static unsigned g_kicks;

static void count_kick(void)
{
    g_kicks++;
}

static void test_ipc_wrap(void)
{
    ipc_ring_t r;
    ipc_msg_hdr_t h;
    ipc_msg_hdr_t out;
    uint8_t pl[8];
    uint8_t got[8];
    uint16_t i;

    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_init(&r, storage, NSLOTS));
    TEST_ASSERT_EQUAL_UINT16(0u, ipc_ring_used(&r));
    TEST_ASSERT_EQUAL_UINT16(NSLOTS, ipc_ring_credits(&r));
    for (i = 0; i < NSLOTS; i++) {
        TEST_ASSERT_EQUAL_INT(ERR_OK,
                              ipc_hdr_init(&h, IPC_EP_SYS, IPC_EP_AUDIO, IPC_SYS_HEARTBEAT, i, 2));
        pl[0] = (uint8_t)i;
        pl[1] = 0xAAu;
        TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_push(&r, &h, pl));
    }
    TEST_ASSERT_EQUAL_UINT16(NSLOTS, ipc_ring_used(&r));
    TEST_ASSERT_EQUAL_UINT16(0u, ipc_ring_credits(&r));
    TEST_ASSERT_EQUAL_INT(ERR_OK,
                          ipc_hdr_init(&h, IPC_EP_SYS, IPC_EP_AUDIO, IPC_SYS_HEARTBEAT, 99, 2));
    TEST_ASSERT_EQUAL_INT(ERR_NOSPC, ipc_ring_push(&r, &h, pl));
    TEST_ASSERT_EQUAL_UINT16(0u, ipc_ring_credits(&r));
    for (i = 0; i < NSLOTS; i++) {
        TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_pop(&r, &out, got, sizeof(got)));
        TEST_ASSERT_EQUAL_UINT16(i, out.seq);
        TEST_ASSERT_EQUAL_UINT8((uint8_t)i, got[0]);
        TEST_ASSERT_EQUAL_HEX8(0xAAu, got[1]);
    }
    TEST_ASSERT_EQUAL_UINT16(NSLOTS, ipc_ring_credits(&r));
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, ipc_ring_pop(&r, &out, got, sizeof(got)));
}

static void test_ipc_bind_spsc(void)
{
    ipc_ring_t prod;
    ipc_ring_t cons;
    volatile uint16_t head = 0;
    volatile uint16_t tail = 0;
    ipc_msg_hdr_t h;
    ipc_msg_hdr_t out;
    uint8_t pl[2];
    uint8_t got[2];
    uint16_t i;

    memset(storage, 0, sizeof(storage));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_bind(&prod, storage, NSLOTS, &head, &tail));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_bind(&cons, storage, NSLOTS, &head, &tail));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_ring_bind(NULL, storage, NSLOTS, &head, &tail));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_ring_bind(&prod, storage, NSLOTS, NULL, &tail));
    for (i = 0; i < NSLOTS; i++) {
        TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_hdr_init(&h, IPC_EP_SYS, IPC_EP_SYS, IPC_SYS_PING, i, 1));
        pl[0] = (uint8_t)(0x10u + i);
        TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_push(&prod, &h, pl));
    }
    TEST_ASSERT_EQUAL_INT(ERR_NOSPC, ipc_ring_push(&prod, &h, pl));
    TEST_ASSERT_EQUAL_UINT16(NSLOTS, head);
    for (i = 0; i < NSLOTS; i++) {
        TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_pop(&cons, &out, got, sizeof(got)));
        TEST_ASSERT_EQUAL_UINT16(i, out.seq);
        TEST_ASSERT_EQUAL_UINT8((uint8_t)(0x10u + i), got[0]);
    }
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_hdr_init(&h, IPC_EP_SYS, IPC_EP_SYS, IPC_SYS_PING, 7, 1));
    pl[0] = 0x77u;
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_push(&prod, &h, pl));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_pop(&cons, &out, got, sizeof(got)));
    TEST_ASSERT_EQUAL_UINT16(7u, out.seq);
    TEST_ASSERT_EQUAL_HEX8(0x77u, got[0]);
}

static void test_ipc_hdr(void)
{
    ipc_msg_hdr_t h;
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_hdr_init(NULL, 1, 2, 3, 0, 0));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_hdr_init(&h, 1, 2, 3, 0, IPC_PAYLOAD_MAX + 1u));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_hdr_init(&h, 1, 2, 3, 1, 0));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_hdr_validate(NULL));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_hdr_validate(&h));
    h.magic = 0;
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT, ipc_hdr_validate(&h));
    h.magic = IPC_MAGIC;
    h.ver = 99;
    TEST_ASSERT_EQUAL_INT(ERR_UNSUPPORTED, ipc_hdr_validate(&h));
    h.ver = IPC_VERSION;
    h.len = (uint16_t)(IPC_PAYLOAD_MAX + 1u);
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_hdr_validate(&h));
}

static void test_ipc_errors(void)
{
    ipc_ring_t r;
    ipc_msg_hdr_t h;
    uint8_t tiny[1];
    uint8_t two[2];
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_ring_init(NULL, storage, NSLOTS));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_ring_init(&r, NULL, NSLOTS));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_ring_init(&r, storage, 0));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_init(&r, storage, NSLOTS));
    TEST_ASSERT_EQUAL_UINT16(0u, ipc_ring_used(NULL));
    TEST_ASSERT_EQUAL_UINT16(0u, ipc_ring_credits(NULL));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_hdr_init(&h, 1, 2, 3, 0, 2));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_ring_push(&r, &h, NULL));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_ring_push(NULL, &h, tiny));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_hdr_init(&h, 1, 2, 3, 0, 0));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_push(&r, &h, NULL));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_pop(&r, &h, tiny, sizeof(tiny)));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_ring_pop(NULL, &h, tiny, 1));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_hdr_init(&h, 1, 2, 3, 0, 2));
    two[0] = 1;
    two[1] = 2;
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_push(&r, &h, two));
    TEST_ASSERT_EQUAL_INT(ERR_NOSPC, ipc_ring_pop(&r, &h, tiny, 1));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_pop(&r, &h, two, sizeof(two)));
    TEST_ASSERT_EQUAL_UINT16(2u, h.len);
    TEST_ASSERT_EQUAL_UINT8(1u, two[0]);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_hdr_init(&h, 1, 2, 3, 0, 2));
    two[0] = 3;
    two[1] = 4;
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_push(&r, &h, two));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_ring_pop(&r, &h, NULL, sizeof(two)));
    h.magic = 0;
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT, ipc_ring_push(&r, &h, two));
    r.local_head = 20u;
    r.local_tail = 0u;
    TEST_ASSERT_EQUAL_UINT16(NSLOTS, ipc_ring_used(&r));
}

static void test_ipc_corrupt_slot(void)
{
    ipc_ring_t r;
    ipc_msg_hdr_t h;
    ipc_msg_hdr_t out;
    uint8_t pl[2];
    uint8_t got[8];

    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_init(&r, storage, NSLOTS));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_hdr_init(&h, 1, 2, 3, 1, 1));
    pl[0] = 0x11u;
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_push(&r, &h, pl));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_hdr_init(&h, 1, 2, 3, 2, 1));
    pl[0] = 0x22u;
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_push(&r, &h, pl));
    storage[0] = 0u;
    storage[1] = 0u;
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT, ipc_ring_pop(&r, &out, got, sizeof(got)));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_ring_pop(&r, &out, got, sizeof(got)));
    TEST_ASSERT_EQUAL_UINT16(2u, out.seq);
    TEST_ASSERT_EQUAL_HEX8(0x22u, got[0]);
}

static void test_ipc_watch(void)
{
    ipc_watch_t w;

    ipc_watch_reset(NULL);
    ipc_watch_beat(NULL, 0);
    TEST_ASSERT_EQUAL_UINT8(0u, ipc_watch_alive(NULL, 0));
    ipc_watch_reset(&w);
    TEST_ASSERT_EQUAL_UINT8(0u, ipc_watch_alive(&w, 0));
    ipc_watch_beat(&w, 100u);
    TEST_ASSERT_EQUAL_UINT8(1u, ipc_watch_alive(&w, 100u));
    TEST_ASSERT_EQUAL_UINT8(1u, ipc_watch_alive(&w, 600u));
    TEST_ASSERT_EQUAL_UINT8(0u, ipc_watch_alive(&w, 601u));
}

static void test_ipc_link(void)
{
    ipc_link_t m7;
    ipc_link_t m4;
    ipc_msg_hdr_t h;
    uint8_t pl[32];
    uint8_t got[32];
    ipc_ctrl_t *ctrl;
    uint16_t i;

    memset(shm, 0, sizeof(shm));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_link_open(NULL, shm, IPC_ROLE_M7, 1));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_link_open(&m7, NULL, IPC_ROLE_M7, 1));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_link_open(&m7, shm, 9u, 1));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_link_open(&m4, shm, IPC_ROLE_M4, 1));
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT, ipc_link_open(&m4, shm, IPC_ROLE_M4, 0));

    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_link_open(&m7, shm, IPC_ROLE_M7, 1));
    TEST_ASSERT_NOT_NULL(m7.ctrl);
    TEST_ASSERT_EQUAL_HEX32(IPC_SHM_MAGIC, m7.ctrl->magic);
    TEST_ASSERT_EQUAL_UINT32(1u, m7.ctrl->m7_ready);
    ipc_link_observe(&m7, 10u);
    TEST_ASSERT_EQUAL_UINT8(0u, ipc_link_peer_alive(&m7, 10u));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_link_open(&m4, shm, IPC_ROLE_M4, 0));
    TEST_ASSERT_EQUAL_UINT32(1u, m4.ctrl->m4_ready);

    g_kicks = 0u;
    ipc_link_set_kick(&m4, count_kick);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_link_send(&m4, IPC_EP_LOG, IPC_LOG_LINE, "hi", 2));
    TEST_ASSERT_EQUAL_UINT(1u, g_kicks);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_link_recv(&m7, &h, got, sizeof(got)));
    TEST_ASSERT_EQUAL_UINT16(IPC_LOG_LINE, h.type);
    TEST_ASSERT_EQUAL_UINT16(0u, h.seq);
    TEST_ASSERT_EQUAL_UINT16(2u, h.len);
    TEST_ASSERT_EQUAL_CHAR('h', (char)got[0]);
    TEST_ASSERT_EQUAL_CHAR('i', (char)got[1]);

    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_link_send(&m7, IPC_EP_SYS, IPC_SYS_PING, "ab", 2));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_link_recv(&m4, &h, got, sizeof(got)));
    TEST_ASSERT_EQUAL_UINT16(IPC_SYS_PING, h.type);
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_link_send(&m4, IPC_EP_SYS, IPC_SYS_PONG, got, h.len));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_link_recv(&m7, &h, got, sizeof(got)));
    TEST_ASSERT_EQUAL_UINT16(IPC_SYS_PONG, h.type);
    TEST_ASSERT_EQUAL_UINT16(1u, h.seq);
    TEST_ASSERT_EQUAL_CHAR('a', (char)got[0]);

    TEST_ASSERT_EQUAL_UINT8(0u, ipc_link_peer_alive(&m7, 0));
    ipc_link_heartbeat(&m4, 50u);
    ipc_link_observe(&m7, 1000u);
    TEST_ASSERT_EQUAL_UINT8(1u, ipc_link_peer_alive(&m7, 1000u));
    TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_link_recv(&m7, &h, got, sizeof(got)));
    TEST_ASSERT_EQUAL_UINT16(IPC_SYS_HEARTBEAT, h.type);
    TEST_ASSERT_EQUAL_UINT8(1u, ipc_link_peer_alive(&m7, 1000u + IPC_HB_TIMEOUT_MS));
    TEST_ASSERT_EQUAL_UINT8(0u, ipc_link_peer_alive(&m7, 1000u + IPC_HB_TIMEOUT_MS + 1u));

    for (i = 0; i < IPC_RING_SLOTS; i++) {
        TEST_ASSERT_EQUAL_INT(ERR_OK, ipc_link_send(&m7, IPC_EP_SYS, IPC_SYS_PING, NULL, 0));
    }
    TEST_ASSERT_EQUAL_INT(ERR_NOSPC, ipc_link_send(&m7, IPC_EP_SYS, IPC_SYS_PING, NULL, 0));
    TEST_ASSERT_EQUAL_UINT16(0u, ipc_ring_credits(&m7.tx));

    ctrl = (ipc_ctrl_t *)(void *)shm;
    ctrl->ver = 99u;
    TEST_ASSERT_EQUAL_INT(ERR_UNSUPPORTED, ipc_link_open(&m4, shm, IPC_ROLE_M4, 0));
    ctrl->ver = IPC_VERSION;
    ctrl->nslots = 0u;
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_link_open(&m4, shm, IPC_ROLE_M4, 0));
    ctrl->nslots = (uint16_t)(IPC_RING_SLOTS + 1u);
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_link_open(&m4, shm, IPC_ROLE_M4, 0));
    ctrl->nslots = IPC_RING_SLOTS;
    ctrl->magic = 0u;
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT, ipc_link_open(&m4, shm, IPC_ROLE_M4, 0));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_link_recv(&m4, &h, got, sizeof(got)));

    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_link_send(&m7, IPC_EP_SYS, IPC_SYS_PING, NULL,
                                                   (uint16_t)(IPC_PAYLOAD_MAX + 1u)));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, ipc_link_send(NULL, 1, 1, NULL, 0));
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, ipc_link_recv(&m7, &h, got, sizeof(got)));
    TEST_ASSERT_EQUAL_UINT8(0u, ipc_link_peer_alive(NULL, 0));
    ipc_link_observe(NULL, 0);
    ipc_link_heartbeat(NULL, 0);
    ipc_link_set_kick(NULL, count_kick);
    (void)pl;
}

void test_ipc_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_ipc_wrap);
    RUN_TEST(test_ipc_bind_spsc);
    RUN_TEST(test_ipc_hdr);
    RUN_TEST(test_ipc_errors);
    RUN_TEST(test_ipc_corrupt_slot);
    RUN_TEST(test_ipc_watch);
    RUN_TEST(test_ipc_link);
}
