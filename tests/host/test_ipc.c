#include "test.h"

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

    CHECK(ipc_ring_init(&r, storage, NSLOTS) == ERR_OK);
    CHECK(ipc_ring_used(&r) == 0u);
    CHECK(ipc_ring_credits(&r) == NSLOTS);
    for (i = 0; i < NSLOTS; i++) {
        CHECK(ipc_hdr_init(&h, IPC_EP_SYS, IPC_EP_AUDIO, IPC_SYS_HEARTBEAT, i, 2) == ERR_OK);
        pl[0] = (uint8_t)i;
        pl[1] = 0xAAu;
        CHECK(ipc_ring_push(&r, &h, pl) == ERR_OK);
    }
    CHECK(ipc_ring_used(&r) == NSLOTS);
    CHECK(ipc_ring_credits(&r) == 0u);
    CHECK(ipc_hdr_init(&h, IPC_EP_SYS, IPC_EP_AUDIO, IPC_SYS_HEARTBEAT, 99, 2) == ERR_OK);
    CHECK(ipc_ring_push(&r, &h, pl) == ERR_NOSPC);
    CHECK(ipc_ring_credits(&r) == 0u);
    for (i = 0; i < NSLOTS; i++) {
        CHECK(ipc_ring_pop(&r, &out, got, sizeof(got)) == ERR_OK);
        CHECK(out.seq == i);
        CHECK(got[0] == (uint8_t)i);
        CHECK(got[1] == 0xAAu);
    }
    CHECK(ipc_ring_credits(&r) == NSLOTS);
    CHECK(ipc_ring_pop(&r, &out, got, sizeof(got)) == ERR_NOENT);
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
    CHECK(ipc_ring_bind(&prod, storage, NSLOTS, &head, &tail) == ERR_OK);
    CHECK(ipc_ring_bind(&cons, storage, NSLOTS, &head, &tail) == ERR_OK);
    CHECK(ipc_ring_bind(NULL, storage, NSLOTS, &head, &tail) == ERR_INVAL);
    CHECK(ipc_ring_bind(&prod, storage, NSLOTS, NULL, &tail) == ERR_INVAL);
    for (i = 0; i < NSLOTS; i++) {
        CHECK(ipc_hdr_init(&h, IPC_EP_SYS, IPC_EP_SYS, IPC_SYS_PING, i, 1) == ERR_OK);
        pl[0] = (uint8_t)(0x10u + i);
        CHECK(ipc_ring_push(&prod, &h, pl) == ERR_OK);
    }
    CHECK(ipc_ring_push(&prod, &h, pl) == ERR_NOSPC);
    CHECK(head == NSLOTS);
    for (i = 0; i < NSLOTS; i++) {
        CHECK(ipc_ring_pop(&cons, &out, got, sizeof(got)) == ERR_OK);
        CHECK(out.seq == i);
        CHECK(got[0] == (uint8_t)(0x10u + i));
    }
    CHECK(ipc_hdr_init(&h, IPC_EP_SYS, IPC_EP_SYS, IPC_SYS_PING, 7, 1) == ERR_OK);
    pl[0] = 0x77u;
    CHECK(ipc_ring_push(&prod, &h, pl) == ERR_OK);
    CHECK(ipc_ring_pop(&cons, &out, got, sizeof(got)) == ERR_OK);
    CHECK(out.seq == 7u);
    CHECK(got[0] == 0x77u);
}

static void test_ipc_hdr(void)
{
    ipc_msg_hdr_t h;
    CHECK(ipc_hdr_init(NULL, 1, 2, 3, 0, 0) == ERR_INVAL);
    CHECK(ipc_hdr_init(&h, 1, 2, 3, 0, IPC_PAYLOAD_MAX + 1u) == ERR_INVAL);
    CHECK(ipc_hdr_init(&h, 1, 2, 3, 1, 0) == ERR_OK);
    CHECK(ipc_hdr_validate(NULL) == ERR_INVAL);
    CHECK(ipc_hdr_validate(&h) == ERR_OK);
    h.magic = 0;
    CHECK(ipc_hdr_validate(&h) == ERR_CORRUPT);
    h.magic = IPC_MAGIC;
    h.ver = 99;
    CHECK(ipc_hdr_validate(&h) == ERR_UNSUPPORTED);
    h.ver = IPC_VERSION;
    h.len = (uint16_t)(IPC_PAYLOAD_MAX + 1u);
    CHECK(ipc_hdr_validate(&h) == ERR_INVAL);
}

static void test_ipc_errors(void)
{
    ipc_ring_t r;
    ipc_msg_hdr_t h;
    uint8_t tiny[1];
    uint8_t two[2];
    CHECK(ipc_ring_init(NULL, storage, NSLOTS) == ERR_INVAL);
    CHECK(ipc_ring_init(&r, NULL, NSLOTS) == ERR_INVAL);
    CHECK(ipc_ring_init(&r, storage, 0) == ERR_INVAL);
    CHECK(ipc_ring_init(&r, storage, NSLOTS) == ERR_OK);
    CHECK(ipc_ring_used(NULL) == 0u);
    CHECK(ipc_ring_credits(NULL) == 0u);
    CHECK(ipc_hdr_init(&h, 1, 2, 3, 0, 2) == ERR_OK);
    CHECK(ipc_ring_push(&r, &h, NULL) == ERR_INVAL);
    CHECK(ipc_ring_push(NULL, &h, tiny) == ERR_INVAL);
    CHECK(ipc_hdr_init(&h, 1, 2, 3, 0, 0) == ERR_OK);
    CHECK(ipc_ring_push(&r, &h, NULL) == ERR_OK);
    CHECK(ipc_ring_pop(&r, &h, tiny, sizeof(tiny)) == ERR_OK);
    CHECK(ipc_ring_pop(NULL, &h, tiny, 1) == ERR_INVAL);
    CHECK(ipc_hdr_init(&h, 1, 2, 3, 0, 2) == ERR_OK);
    two[0] = 1;
    two[1] = 2;
    CHECK(ipc_ring_push(&r, &h, two) == ERR_OK);
    CHECK(ipc_ring_pop(&r, &h, tiny, 1) == ERR_NOSPC);
    CHECK(ipc_ring_pop(&r, &h, two, sizeof(two)) == ERR_OK);
    CHECK(h.len == 2u);
    CHECK(two[0] == 1u);
    CHECK(ipc_hdr_init(&h, 1, 2, 3, 0, 2) == ERR_OK);
    two[0] = 3;
    two[1] = 4;
    CHECK(ipc_ring_push(&r, &h, two) == ERR_OK);
    CHECK(ipc_ring_pop(&r, &h, NULL, sizeof(two)) == ERR_INVAL);
    h.magic = 0;
    CHECK(ipc_ring_push(&r, &h, two) == ERR_CORRUPT);
    r.local_head = 20u;
    r.local_tail = 0u;
    CHECK(ipc_ring_used(&r) == NSLOTS);
}

static void test_ipc_corrupt_slot(void)
{
    ipc_ring_t r;
    ipc_msg_hdr_t h;
    ipc_msg_hdr_t out;
    uint8_t pl[2];
    uint8_t got[8];

    CHECK(ipc_ring_init(&r, storage, NSLOTS) == ERR_OK);
    CHECK(ipc_hdr_init(&h, 1, 2, 3, 1, 1) == ERR_OK);
    pl[0] = 0x11u;
    CHECK(ipc_ring_push(&r, &h, pl) == ERR_OK);
    CHECK(ipc_hdr_init(&h, 1, 2, 3, 2, 1) == ERR_OK);
    pl[0] = 0x22u;
    CHECK(ipc_ring_push(&r, &h, pl) == ERR_OK);
    storage[0] = 0u;
    storage[1] = 0u;
    CHECK(ipc_ring_pop(&r, &out, got, sizeof(got)) == ERR_CORRUPT);
    CHECK(ipc_ring_pop(&r, &out, got, sizeof(got)) == ERR_OK);
    CHECK(out.seq == 2u);
    CHECK(got[0] == 0x22u);
}

static void test_ipc_watch(void)
{
    ipc_watch_t w;

    ipc_watch_reset(NULL);
    ipc_watch_beat(NULL, 0);
    CHECK(ipc_watch_alive(NULL, 0) == 0u);
    ipc_watch_reset(&w);
    CHECK(ipc_watch_alive(&w, 0) == 0u);
    ipc_watch_beat(&w, 100u);
    CHECK(ipc_watch_alive(&w, 100u) == 1u);
    CHECK(ipc_watch_alive(&w, 600u) == 1u);
    CHECK(ipc_watch_alive(&w, 601u) == 0u);
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
    CHECK(ipc_link_open(NULL, shm, IPC_ROLE_M7, 1) == ERR_INVAL);
    CHECK(ipc_link_open(&m7, NULL, IPC_ROLE_M7, 1) == ERR_INVAL);
    CHECK(ipc_link_open(&m7, shm, 9u, 1) == ERR_INVAL);
    CHECK(ipc_link_open(&m4, shm, IPC_ROLE_M4, 1) == ERR_INVAL);
    CHECK(ipc_link_open(&m4, shm, IPC_ROLE_M4, 0) == ERR_CORRUPT);

    CHECK(ipc_link_open(&m7, shm, IPC_ROLE_M7, 1) == ERR_OK);
    CHECK(m7.ctrl != NULL);
    CHECK(m7.ctrl->magic == IPC_SHM_MAGIC);
    CHECK(m7.ctrl->m7_ready == 1u);
    ipc_link_observe(&m7, 10u);
    CHECK(ipc_link_peer_alive(&m7, 10u) == 0u);
    CHECK(ipc_link_open(&m4, shm, IPC_ROLE_M4, 0) == ERR_OK);
    CHECK(m4.ctrl->m4_ready == 1u);

    g_kicks = 0u;
    ipc_link_set_kick(&m4, count_kick);
    CHECK(ipc_link_send(&m4, IPC_EP_LOG, IPC_LOG_LINE, "hi", 2) == ERR_OK);
    CHECK(g_kicks == 1u);
    CHECK(ipc_link_recv(&m7, &h, got, sizeof(got)) == ERR_OK);
    CHECK(h.type == IPC_LOG_LINE);
    CHECK(h.seq == 0u);
    CHECK(h.len == 2u);
    CHECK(got[0] == 'h');
    CHECK(got[1] == 'i');

    CHECK(ipc_link_send(&m7, IPC_EP_SYS, IPC_SYS_PING, "ab", 2) == ERR_OK);
    CHECK(ipc_link_recv(&m4, &h, got, sizeof(got)) == ERR_OK);
    CHECK(h.type == IPC_SYS_PING);
    CHECK(ipc_link_send(&m4, IPC_EP_SYS, IPC_SYS_PONG, got, h.len) == ERR_OK);
    CHECK(ipc_link_recv(&m7, &h, got, sizeof(got)) == ERR_OK);
    CHECK(h.type == IPC_SYS_PONG);
    CHECK(h.seq == 1u);
    CHECK(got[0] == 'a');

    CHECK(ipc_link_peer_alive(&m7, 0) == 0u);
    ipc_link_heartbeat(&m4, 50u);
    ipc_link_observe(&m7, 1000u);
    CHECK(ipc_link_peer_alive(&m7, 1000u) == 1u);
    CHECK(ipc_link_recv(&m7, &h, got, sizeof(got)) == ERR_OK);
    CHECK(h.type == IPC_SYS_HEARTBEAT);
    CHECK(ipc_link_peer_alive(&m7, 1000u + IPC_HB_TIMEOUT_MS) == 1u);
    CHECK(ipc_link_peer_alive(&m7, 1000u + IPC_HB_TIMEOUT_MS + 1u) == 0u);

    for (i = 0; i < IPC_RING_SLOTS; i++) {
        CHECK(ipc_link_send(&m7, IPC_EP_SYS, IPC_SYS_PING, NULL, 0) == ERR_OK);
    }
    CHECK(ipc_link_send(&m7, IPC_EP_SYS, IPC_SYS_PING, NULL, 0) == ERR_NOSPC);
    CHECK(ipc_ring_credits(&m7.tx) == 0u);

    ctrl = (ipc_ctrl_t *)(void *)shm;
    ctrl->ver = 99u;
    CHECK(ipc_link_open(&m4, shm, IPC_ROLE_M4, 0) == ERR_UNSUPPORTED);
    ctrl->ver = IPC_VERSION;
    ctrl->nslots = 0u;
    CHECK(ipc_link_open(&m4, shm, IPC_ROLE_M4, 0) == ERR_INVAL);
    ctrl->nslots = (uint16_t)(IPC_RING_SLOTS + 1u);
    CHECK(ipc_link_open(&m4, shm, IPC_ROLE_M4, 0) == ERR_INVAL);
    ctrl->nslots = IPC_RING_SLOTS;
    ctrl->magic = 0u;
    CHECK(ipc_link_open(&m4, shm, IPC_ROLE_M4, 0) == ERR_CORRUPT);
    CHECK(ipc_link_recv(&m4, &h, got, sizeof(got)) == ERR_INVAL);

    CHECK(ipc_link_send(&m7, IPC_EP_SYS, IPC_SYS_PING, NULL, (uint16_t)(IPC_PAYLOAD_MAX + 1u)) ==
          ERR_INVAL);
    CHECK(ipc_link_send(NULL, 1, 1, NULL, 0) == ERR_INVAL);
    CHECK(ipc_link_recv(&m7, &h, got, sizeof(got)) == ERR_NOENT);
    CHECK(ipc_link_peer_alive(NULL, 0) == 0u);
    ipc_link_observe(NULL, 0);
    ipc_link_heartbeat(NULL, 0);
    ipc_link_set_kick(NULL, count_kick);
    (void)pl;
}

void test_ipc_run(void)
{
    test_ipc_wrap();
    test_ipc_bind_spsc();
    test_ipc_hdr();
    test_ipc_errors();
    test_ipc_corrupt_slot();
    test_ipc_watch();
    test_ipc_link();
}
