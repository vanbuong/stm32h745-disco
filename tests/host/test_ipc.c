#include "test.h"

#include "ipc/ipc.h"

#include <stdint.h>
#include <string.h>

#define NSLOTS 4u

static uint8_t storage[NSLOTS * (sizeof(ipc_msg_hdr_t) + IPC_PAYLOAD_MAX)];

static void test_ipc_wrap(void)
{
    ipc_ring_t r;
    ipc_msg_hdr_t h;
    ipc_msg_hdr_t out;
    uint8_t pl[8];
    uint8_t got[8];
    uint16_t i;

    CHECK(ipc_ring_init(&r, storage, NSLOTS) == ERR_OK);
    for (i = 0; i < NSLOTS; i++) {
        CHECK(ipc_hdr_init(&h, IPC_EP_SYS, IPC_EP_AUDIO, IPC_SYS_HEARTBEAT, i, 2) == ERR_OK);
        pl[0] = (uint8_t)i;
        pl[1] = 0xAAu;
        CHECK(ipc_ring_push(&r, &h, pl) == ERR_OK);
    }
    CHECK(ipc_hdr_init(&h, IPC_EP_SYS, IPC_EP_AUDIO, IPC_SYS_HEARTBEAT, 99, 2) == ERR_OK);
    CHECK(ipc_ring_push(&r, &h, pl) == ERR_NOSPC);
    for (i = 0; i < NSLOTS; i++) {
        CHECK(ipc_ring_pop(&r, &out, got, sizeof(got)) == ERR_OK);
        CHECK(out.seq == i);
        CHECK(got[0] == (uint8_t)i);
        CHECK(got[1] == 0xAAu);
    }
    CHECK(ipc_ring_pop(&r, &out, got, sizeof(got)) == ERR_NOENT);
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
}

void test_ipc_run(void)
{
    test_ipc_wrap();
    test_ipc_hdr();
    test_ipc_errors();
}
