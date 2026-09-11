#include "ipc/ipc.h"

#include <string.h>

#define SLOT_SIZE (sizeof(ipc_msg_hdr_t) + IPC_PAYLOAD_MAX)

static uint8_t *slot_at(ipc_ring_t *r, uint16_t idx)
{
    return r->storage + (size_t)idx * SLOT_SIZE;
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
    r->head = 0;
    r->tail = 0;
    r->count = 0;
    return ERR_OK;
}

err_t ipc_ring_push(ipc_ring_t *r, const ipc_msg_hdr_t *hdr, const void *payload)
{
    uint8_t *slot;
    err_t e;

    if (r == NULL || hdr == NULL) {
        return ERR_INVAL;
    }
    e = ipc_hdr_validate(hdr);
    if (e != ERR_OK) {
        return e;
    }
    if (hdr->len > 0u && payload == NULL) {
        return ERR_INVAL;
    }
    if (r->count >= r->nslots) {
        return ERR_NOSPC;
    }
    slot = slot_at(r, r->head);
    memcpy(slot, hdr, sizeof(*hdr));
    if (hdr->len > 0u) {
        memcpy(slot + sizeof(*hdr), payload, hdr->len);
    }
    r->head = (uint16_t)((r->head + 1u) % r->nslots);
    r->count++;
    return ERR_OK;
}

err_t ipc_ring_pop(ipc_ring_t *r, ipc_msg_hdr_t *hdr, void *payload, size_t payload_max)
{
    uint8_t *slot;
    ipc_msg_hdr_t tmp;

    if (r == NULL || hdr == NULL) {
        return ERR_INVAL;
    }
    if (r->count == 0u) {
        return ERR_NOENT;
    }
    slot = slot_at(r, r->tail);
    memcpy(&tmp, slot, sizeof(tmp));
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
    r->tail = (uint16_t)((r->tail + 1u) % r->nslots);
    r->count--;
    return ERR_OK;
}
