#include "ipc/ipc.h"

#include <string.h>

#define SLOT_SIZE (sizeof(ipc_msg_hdr_t) + IPC_PAYLOAD_MAX)

static uint8_t *slot_at(const ipc_ring_t *r, uint16_t pos)
{
    uint16_t idx = (uint16_t)(pos % r->nslots);
    return r->storage + ((size_t)idx * SLOT_SIZE);
}

static uint16_t load_head(const ipc_ring_t *r)
{
    return __atomic_load_n(r->head, __ATOMIC_ACQUIRE);
}

static uint16_t load_tail(const ipc_ring_t *r)
{
    return __atomic_load_n(r->tail, __ATOMIC_ACQUIRE);
}

static void store_head(ipc_ring_t *r, uint16_t v)
{
    __atomic_store_n(r->head, v, __ATOMIC_RELEASE);
}

static void store_tail(ipc_ring_t *r, uint16_t v)
{
    __atomic_store_n(r->tail, v, __ATOMIC_RELEASE);
}

static uint16_t used_raw(const ipc_ring_t *r)
{
    return (uint16_t)(load_head(r) - load_tail(r));
}

err_t ipc_hdr_init(ipc_msg_hdr_t *hdr, uint8_t src, uint8_t dst, uint16_t type, uint16_t seq,
                   uint16_t len)
{
    if (hdr == NULL) {
        return ERR_INVAL;
    }
    if (len > IPC_PAYLOAD_MAX) {
        return ERR_INVAL;
    }
    hdr->magic = IPC_MAGIC;
    hdr->ver = IPC_VERSION;
    hdr->src = src;
    hdr->dst = dst;
    hdr->flags = 0;
    hdr->type = type;
    hdr->seq = seq;
    hdr->len = len;
    return ERR_OK;
}

err_t ipc_hdr_validate(const ipc_msg_hdr_t *hdr)
{
    if (hdr == NULL) {
        return ERR_INVAL;
    }
    if (hdr->magic != IPC_MAGIC) {
        return ERR_CORRUPT;
    }
    if (hdr->ver != IPC_VERSION) {
        return ERR_UNSUPPORTED;
    }
    if (hdr->len > IPC_PAYLOAD_MAX) {
        return ERR_INVAL;
    }
    return ERR_OK;
}

err_t ipc_ring_init(ipc_ring_t *r, void *storage, uint16_t nslots)
{
    if (r == NULL || storage == NULL || nslots == 0u) {
        return ERR_INVAL;
    }
    r->storage = (uint8_t *)storage;
    r->nslots = nslots;
    r->local_head = 0u;
    r->local_tail = 0u;
    r->head = &r->local_head;
    r->tail = &r->local_tail;
    return ERR_OK;
}

err_t ipc_ring_bind(ipc_ring_t *r, void *storage, uint16_t nslots, volatile uint16_t *head,
                    volatile uint16_t *tail)
{
    if (r == NULL || storage == NULL || nslots == 0u || head == NULL || tail == NULL) {
        return ERR_INVAL;
    }
    r->storage = (uint8_t *)storage;
    r->nslots = nslots;
    r->local_head = 0u;
    r->local_tail = 0u;
    r->head = head;
    r->tail = tail;
    return ERR_OK;
}

err_t ipc_ring_push(ipc_ring_t *r, const ipc_msg_hdr_t *hdr, const void *payload)
{
    uint8_t *slot;
    uint16_t head;
    err_t e;

    if (r == NULL || hdr == NULL || r->head == NULL || r->tail == NULL) {
        return ERR_INVAL;
    }
    e = ipc_hdr_validate(hdr);
    if (e != ERR_OK) {
        return e;
    }
    if (hdr->len > 0u && payload == NULL) {
        return ERR_INVAL;
    }
    head = load_head(r);
    if (used_raw(r) >= r->nslots) {
        return ERR_NOSPC;
    }
    slot = slot_at(r, head);
    memcpy(slot, hdr, sizeof(*hdr));
    if (hdr->len > 0u) {
        memcpy(slot + sizeof(*hdr), payload, hdr->len);
    }
    store_head(r, (uint16_t)(head + 1u));
    return ERR_OK;
}

err_t ipc_ring_pop(ipc_ring_t *r, ipc_msg_hdr_t *hdr, void *payload, size_t payload_max)
{
    uint8_t *slot;
    ipc_msg_hdr_t tmp;
    uint16_t tail;
    err_t e;

    if (r == NULL || hdr == NULL || r->head == NULL || r->tail == NULL) {
        return ERR_INVAL;
    }
    tail = load_tail(r);
    if (used_raw(r) == 0u) {
        return ERR_NOENT;
    }
    slot = slot_at(r, tail);
    memcpy(&tmp, slot, sizeof(tmp));
    e = ipc_hdr_validate(&tmp);
    if (e != ERR_OK) {
        store_tail(r, (uint16_t)(tail + 1u));
        return e;
    }
    if (tmp.len > payload_max) {
        return ERR_NOSPC;
    }
    if (tmp.len > 0u && payload == NULL) {
        return ERR_INVAL;
    }
    memcpy(hdr, &tmp, sizeof(tmp));
    if (tmp.len > 0u) {
        memcpy(payload, slot + sizeof(tmp), tmp.len);
    }
    store_tail(r, (uint16_t)(tail + 1u));
    return ERR_OK;
}

uint16_t ipc_ring_used(const ipc_ring_t *r)
{
    uint16_t n;

    if (r == NULL || r->head == NULL || r->tail == NULL) {
        return 0u;
    }
    n = used_raw(r);
    if (n > r->nslots) {
        return r->nslots;
    }
    return n;
}

uint16_t ipc_ring_credits(const ipc_ring_t *r)
{
    if (r == NULL) {
        return 0u;
    }
    return (uint16_t)(r->nslots - ipc_ring_used(r));
}

void ipc_watch_reset(ipc_watch_t *w)
{
    if (w == NULL) {
        return;
    }
    w->last_ms = 0u;
    w->seen = 0u;
}

void ipc_watch_beat(ipc_watch_t *w, uint32_t now_ms)
{
    if (w == NULL) {
        return;
    }
    w->last_ms = now_ms;
    w->seen = 1u;
}

uint8_t ipc_watch_alive(const ipc_watch_t *w, uint32_t now_ms)
{
    uint32_t dt;

    if (w == NULL || w->seen == 0u) {
        return 0u;
    }
    dt = now_ms - w->last_ms;
    if (dt > IPC_HB_TIMEOUT_MS) {
        return 0u;
    }
    return 1u;
}
