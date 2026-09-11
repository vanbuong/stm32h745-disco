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
#define IPC_SYS_PING 4u
#define IPC_SYS_PONG 5u

#define IPC_LOG_LINE 1u

#define IPC_AUDIO_PLAY 10u
#define IPC_AUDIO_PAUSE 11u
#define IPC_AUDIO_RESUME 12u
#define IPC_AUDIO_STOP 13u
#define IPC_AUDIO_VOLUME 14u
#define IPC_AUDIO_POS 15u
#define IPC_AUDIO_ACK 16u
#define IPC_AUDIO_NAK 17u
#define IPC_AUDIO_UNDERRUN 18u
#define IPC_AUDIO_DONE 19u

#define IPC_AUDIO_PIPE_OFF (IPC_SHM_CTRL + (2u * IPC_SHM_RING))
#define IPC_AUDIO_PIPE_BYTES 16384u

typedef struct __attribute__((packed)) {
    uint8_t kind;
    uint8_t channels;
    uint8_t bits;
    uint8_t volume;
    uint32_t sample_hz;
} ipc_audio_fmt_t;

typedef struct __attribute__((packed)) {
    uint32_t elapsed_ms;
    uint32_t duration_ms;
    uint16_t underruns;
    uint8_t state;
    uint8_t pad;
} ipc_audio_pos_t;

#define IPC_SHM_MAGIC 0x31435049u /* 'IPC1' */
#define IPC_SHM_CTRL 256u
#define IPC_SHM_RING 16384u
#define IPC_SHM_BYTES (64u * 1024u)
#define IPC_DOWN_OFF IPC_SHM_CTRL
#define IPC_UP_OFF (IPC_SHM_CTRL + IPC_SHM_RING)
#define IPC_RING_SLOTS 60u
#define IPC_HB_TIMEOUT_MS 500u
#define IPC_HB_PERIOD_MS 100u
#define IPC_ROLE_M7 0u
#define IPC_ROLE_M4 1u

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
