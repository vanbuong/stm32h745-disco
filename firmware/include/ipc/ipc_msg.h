#ifndef IPC_MSG_H
#define IPC_MSG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPC_MAGIC 0xA55Au
#define IPC_VERSION 1u
#define IPC_PAYLOAD_MAX 256u

#define IPC_EP_SYS 1u
#define IPC_EP_AUDIO 2u
#define IPC_EP_NET 3u
#define IPC_EP_LOG 4u
#define IPC_EP_ZB 5u

#define IPC_FLAG_ACK 0x01u
#define IPC_FLAG_NAK 0x02u
#define IPC_FLAG_MORE 0x04u

#define IPC_SYS_HEARTBEAT 1u
#define IPC_SYS_READY 2u
#define IPC_SYS_PANIC 3u

#define IPC_AUDIO_PLAY 10u
#define IPC_AUDIO_PAUSE 11u
#define IPC_AUDIO_RESUME 12u
#define IPC_AUDIO_STOP 13u
#define IPC_AUDIO_VOLUME 14u
#define IPC_AUDIO_POS 15u
#define IPC_AUDIO_ACK 16u
#define IPC_AUDIO_NAK 17u
#define IPC_AUDIO_UNDERRUN 18u

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t ver;
    uint8_t src;
    uint8_t dst;
    uint8_t flags;
    uint16_t type;
    uint16_t seq;
    uint16_t len;
} ipc_msg_hdr_t;

#ifdef __cplusplus
}
#endif

#endif /* IPC_MSG_H */
