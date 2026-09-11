#ifndef IPC_H
#define IPC_H

#include "err.h"
#include "ipc/ipc_msg.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPC_SLOT_SIZE (sizeof(ipc_msg_hdr_t) + IPC_PAYLOAD_MAX)

typedef struct {
    uint8_t *storage;
    uint16_t nslots;
    uint16_t local_head;
    uint16_t local_tail;
    volatile uint16_t *head;
    volatile uint16_t *tail;
} ipc_ring_t;

typedef struct {
    uint32_t last_ms;
    uint8_t seen;
} ipc_watch_t;

typedef struct {
    uint32_t magic;
    uint16_t ver;
    uint16_t nslots;
    volatile uint32_t m7_ready;
    volatile uint32_t m4_ready;
    volatile uint32_t m4_hb_ms;
    volatile uint32_t m4_hb_seq;
    volatile uint32_t kick_m7;
    volatile uint32_t kick_m4;
    uint8_t pad[IPC_SHM_CTRL - 32u];
} ipc_ctrl_t;

typedef struct {
    volatile uint16_t head;
    volatile uint16_t tail;
} ipc_meta_t;

typedef struct {
    ipc_ring_t tx;
    ipc_ring_t rx;
    ipc_ctrl_t *ctrl;
    uint8_t role;
    uint16_t seq;
    uint32_t seen_hb_seq;
    ipc_watch_t watch;
    void (*kick)(void);
} ipc_link_t;

err_t ipc_hdr_init(ipc_msg_hdr_t *hdr, uint8_t src, uint8_t dst, uint16_t type, uint16_t seq,
                   uint16_t len);
err_t ipc_hdr_validate(const ipc_msg_hdr_t *hdr);

err_t ipc_ring_init(ipc_ring_t *r, void *storage, uint16_t nslots);
err_t ipc_ring_bind(ipc_ring_t *r, void *storage, uint16_t nslots, volatile uint16_t *head,
                    volatile uint16_t *tail);
err_t ipc_ring_push(ipc_ring_t *r, const ipc_msg_hdr_t *hdr, const void *payload);
err_t ipc_ring_pop(ipc_ring_t *r, ipc_msg_hdr_t *hdr, void *payload, size_t payload_max);
uint16_t ipc_ring_used(const ipc_ring_t *r);
uint16_t ipc_ring_credits(const ipc_ring_t *r);

void ipc_watch_reset(ipc_watch_t *w);
void ipc_watch_beat(ipc_watch_t *w, uint32_t now_ms);
uint8_t ipc_watch_alive(const ipc_watch_t *w, uint32_t now_ms);

err_t ipc_link_open(ipc_link_t *l, void *base, uint8_t role, int format);
err_t ipc_link_send(ipc_link_t *l, uint8_t dst, uint16_t type, const void *payload, uint16_t len);
err_t ipc_link_recv(ipc_link_t *l, ipc_msg_hdr_t *hdr, void *payload, size_t payload_max);
void ipc_link_set_kick(ipc_link_t *l, void (*kick)(void));
void ipc_link_heartbeat(ipc_link_t *l, uint32_t now_ms);
void ipc_link_observe(ipc_link_t *l, uint32_t now_ms);
uint8_t ipc_link_peer_alive(const ipc_link_t *l, uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* IPC_H */
