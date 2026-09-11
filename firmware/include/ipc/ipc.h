#ifndef IPC_H
#define IPC_H

#include "err.h"
#include "ipc/ipc_msg.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *storage;
    uint16_t nslots;
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} ipc_ring_t;

err_t ipc_hdr_init(ipc_msg_hdr_t *hdr, uint8_t src, uint8_t dst, uint16_t type, uint16_t seq,
                   uint16_t len);
err_t ipc_hdr_validate(const ipc_msg_hdr_t *hdr);

err_t ipc_ring_init(ipc_ring_t *r, void *storage, uint16_t nslots);
err_t ipc_ring_push(ipc_ring_t *r, const ipc_msg_hdr_t *hdr, const void *payload);
err_t ipc_ring_pop(ipc_ring_t *r, ipc_msg_hdr_t *hdr, void *payload, size_t payload_max);

#ifdef __cplusplus
}
#endif

#endif /* IPC_H */
