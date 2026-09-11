#include "ipc/ipc.h"

#include <string.h>

_Static_assert(sizeof(ipc_ctrl_t) == (size_t)IPC_SHM_CTRL, "ipc_ctrl_t");
_Static_assert(sizeof(ipc_msg_hdr_t) == 12u, "ipc_msg_hdr_t");
_Static_assert((IPC_RING_SLOTS * IPC_SLOT_SIZE) + sizeof(ipc_meta_t) <= IPC_SHM_RING, "ring");

static uint8_t src_ep(uint8_t dst)
{
    if (dst == IPC_EP_LOG) {
        return IPC_EP_LOG;
    }
    return IPC_EP_SYS;
}

static void ring_window(uint8_t *base, size_t off, ipc_meta_t **meta, uint8_t **slots)
{
    uint8_t *win = base + off;
    *meta = (ipc_meta_t *)win;
    *slots = win + sizeof(ipc_meta_t);
}

static err_t bind_role(ipc_link_t *l, uint8_t *base)
{
    ipc_meta_t *down_meta;
    ipc_meta_t *up_meta;
    uint8_t *down_slots;
    uint8_t *up_slots;
    uint16_t n = l->ctrl->nslots;
    err_t e;

    ring_window(base, IPC_DOWN_OFF, &down_meta, &down_slots);
    ring_window(base, IPC_UP_OFF, &up_meta, &up_slots);
    if (l->role == IPC_ROLE_M7) {
        e = ipc_ring_bind(&l->tx, down_slots, n, &down_meta->head, &down_meta->tail);
        if (e != ERR_OK) {
            return e;
        }
        return ipc_ring_bind(&l->rx, up_slots, n, &up_meta->head, &up_meta->tail);
    }
    e = ipc_ring_bind(&l->tx, up_slots, n, &up_meta->head, &up_meta->tail);
    if (e != ERR_OK) {
        return e;
    }
    return ipc_ring_bind(&l->rx, down_slots, n, &down_meta->head, &down_meta->tail);
}

static err_t format_shm(uint8_t *base)
{
    ipc_ctrl_t *c = (ipc_ctrl_t *)base;

    memset(base, 0, (size_t)IPC_SHM_CTRL + (2u * (size_t)IPC_SHM_RING));
    c->magic = IPC_SHM_MAGIC;
    c->ver = IPC_VERSION;
    c->nslots = IPC_RING_SLOTS;
    c->m7_ready = 1u;
    return ERR_OK;
}

static err_t attach_shm(uint8_t *base)
{
    const ipc_ctrl_t *c = (const ipc_ctrl_t *)base;

    if (c->magic != IPC_SHM_MAGIC) {
        return ERR_CORRUPT;
    }
    if (c->ver != IPC_VERSION) {
        return ERR_UNSUPPORTED;
    }
    if (c->nslots == 0u || c->nslots > IPC_RING_SLOTS) {
        return ERR_INVAL;
    }
    return ERR_OK;
}

static void kick_peer(ipc_link_t *l)
{
    if (l->ctrl != NULL) {
        if (l->role == IPC_ROLE_M7) {
            l->ctrl->kick_m4 = l->ctrl->kick_m4 + 1u;
        } else {
            l->ctrl->kick_m7 = l->ctrl->kick_m7 + 1u;
        }
    }
    if (l->kick != NULL) {
        l->kick();
    }
}

err_t ipc_link_open(ipc_link_t *l, void *base, uint8_t role, int format)
{
    uint8_t *b = (uint8_t *)base;
    err_t e;

    if (l == NULL || base == NULL) {
        return ERR_INVAL;
    }
    if (role != IPC_ROLE_M7 && role != IPC_ROLE_M4) {
        return ERR_INVAL;
    }
    if (format != 0 && role != IPC_ROLE_M7) {
        return ERR_INVAL;
    }
    memset(l, 0, sizeof(*l));
    l->role = role;
    l->ctrl = (ipc_ctrl_t *)b;
    if (format != 0) {
        e = format_shm(b);
    } else {
        e = attach_shm(b);
    }
    if (e != ERR_OK) {
        l->ctrl = NULL;
        return e;
    }
    e = bind_role(l, b);
    if (e != ERR_OK) {
        l->ctrl = NULL;
        return e;
    }
    if (role == IPC_ROLE_M4) {
        l->ctrl->m4_ready = 1u;
    }
    return ERR_OK;
}

err_t ipc_link_send(ipc_link_t *l, uint8_t dst, uint16_t type, const void *payload, uint16_t len)
{
    ipc_msg_hdr_t h;
    err_t e;

    if (l == NULL || l->ctrl == NULL) {
        return ERR_INVAL;
    }
    e = ipc_hdr_init(&h, src_ep(dst), dst, type, l->seq, len);
    if (e != ERR_OK) {
        return e;
    }
    e = ipc_ring_push(&l->tx, &h, payload);
    if (e != ERR_OK) {
        return e;
    }
    l->seq = (uint16_t)(l->seq + 1u);
    kick_peer(l);
    return ERR_OK;
}

err_t ipc_link_recv(ipc_link_t *l, ipc_msg_hdr_t *hdr, void *payload, size_t payload_max)
{
    if (l == NULL || l->ctrl == NULL) {
        return ERR_INVAL;
    }
    return ipc_ring_pop(&l->rx, hdr, payload, payload_max);
}

void ipc_link_set_kick(ipc_link_t *l, void (*kick)(void))
{
    if (l != NULL) {
        l->kick = kick;
    }
}

void ipc_link_heartbeat(ipc_link_t *l, uint32_t now_ms)
{
    uint32_t ms = now_ms;

    if (l == NULL || l->ctrl == NULL) {
        return;
    }
    l->ctrl->m4_hb_ms = now_ms;
    l->ctrl->m4_hb_seq = l->ctrl->m4_hb_seq + 1u;
    (void)ipc_link_send(l, IPC_EP_SYS, IPC_SYS_HEARTBEAT, &ms, (uint16_t)sizeof(ms));
}

void ipc_link_observe(ipc_link_t *l, uint32_t now_ms)
{
    uint32_t seq;

    if (l == NULL || l->ctrl == NULL) {
        return;
    }
    if (l->ctrl->m4_ready == 0u) {
        return;
    }
    seq = l->ctrl->m4_hb_seq;
    if (seq != l->seen_hb_seq) {
        l->seen_hb_seq = seq;
        ipc_watch_beat(&l->watch, now_ms);
    }
}

uint8_t ipc_link_peer_alive(const ipc_link_t *l, uint32_t now_ms)
{
    if (l == NULL) {
        return 0u;
    }
    return ipc_watch_alive(&l->watch, now_ms);
}
