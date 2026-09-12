#ifndef ZB_HOST_H
#define ZB_HOST_H

#include "err.h"
#include "svc/home.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZB_CLUSTER_ONOFF 0x0006u
#define ZB_CLUSTER_LEVEL 0x0008u
#define ZB_CLUSTER_TEMP 0x0402u
#define ZB_CLUSTER_OCC 0x0406u
#define ZB_CLUSTER_IAS 0x0500u

#define ZB_DEV_PATH "/user/home/devices.bin"
#define ZB_NET_PATH "/user/home/network.bin"
#define ZB_HOME_DIR "/user/home"

typedef struct {
    uint8_t channel;
    uint16_t pan;
} zb_net_cfg_t;

typedef struct {
    uint8_t formed;
    uint8_t channel;
    uint16_t pan;
    uint8_t ext_pan[8];
    uint8_t persist_ok;
    uint8_t radio_ok;
    uint8_t mock;
    uint8_t permit_left;
    char znp_ver[16];
} zb_net_info_t;

typedef struct {
    uint8_t used;
    uint8_t ieee[8];
    uint16_t nwk;
    uint8_t ep;
    char name[HOME_NAME_MAX];
    char room_id[HOME_ROOM_MAX];
    home_kind_t kind;
    uint8_t on;
    uint8_t level;
    uint8_t lqi;
    uint32_t last_seen_ms;
    uint8_t interviewing;
} zb_dev_t;

err_t zb_host_init(void);
void zb_host_reset(void);
void zb_host_poll(uint32_t dt_ms);

err_t zb_form(const zb_net_cfg_t *cfg);
err_t zb_permit_join(uint8_t seconds);
err_t zb_leave(const uint8_t ieee[8]);
err_t zb_interview(const uint8_t ieee[8]);

size_t zb_host_device_count(void);
const zb_dev_t *zb_host_device_at(size_t i);
zb_dev_t *zb_host_device_mut(size_t i);
err_t zb_host_find(const uint8_t ieee[8], size_t *idx);

void zb_host_net(zb_net_info_t *out);
uint8_t zb_host_cmd_allowed(void);
uint8_t zb_host_dirty(void);

err_t zb_host_add(const uint8_t ieee[8], uint16_t nwk, home_kind_t kind, const char *name,
                  const char *room);
err_t zb_host_set_meta(const uint8_t ieee[8], const char *name, const char *room);
err_t zb_host_apply_announce(uint16_t nwk, const uint8_t ieee[8]);
err_t zb_host_apply_clusters(const uint8_t ieee[8], const uint16_t *in, uint8_t n);
err_t zb_host_apply_report(const uint8_t ieee[8], uint16_t cluster, uint8_t on, uint8_t level);
err_t zb_host_apply_cmd(const uint8_t ieee[8], const home_cmd_t *cmd);

void zb_host_test_set_flags(uint8_t radio_ok, uint8_t mock);
home_kind_t zb_host_kind_from_clusters(const uint16_t *in, uint8_t n);

#ifdef __cplusplus
}
#endif

#endif /* ZB_HOST_H */
