#ifndef HOME_H
#define HOME_H

#include "err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HOME_NAME_MAX 24
#define HOME_ROOM_MAX 16
#define HOME_DEV_MAX 32
#define HOME_ROOM_CAP 8

typedef enum { HOME_LIGHT = 0, HOME_SWITCH, HOME_BINARY_SENSOR, HOME_CLIMATE } home_kind_t;

typedef struct {
    char id[HOME_NAME_MAX];
    char name[HOME_NAME_MAX];
} home_room_t;

typedef struct {
    char id[HOME_NAME_MAX];
    uint8_t ieee[8];
    uint16_t nwk;
    char name[HOME_NAME_MAX];
    char room_id[HOME_ROOM_MAX];
    home_kind_t kind;
    uint8_t on;
    uint8_t level;
    uint8_t lqi;
    uint32_t last_seen_ms;
} home_device_t;

typedef struct {
    uint8_t on;
    uint8_t has_level;
    uint8_t level;
} home_cmd_t;

typedef struct {
    uint8_t formed;
    uint8_t channel;
    uint16_t pan;
    uint8_t radio_ok;
    uint8_t persist_ok;
    uint8_t mock;
    uint8_t permit_left;
    char znp_ver[16];
} home_net_t;

err_t home_init(void);
void home_reset(void);
void home_poll(uint32_t dt_ms);

size_t home_devices(const char *room_id, home_device_t *out, size_t max);
err_t home_device(const char *device_id, home_device_t *out);
err_t home_device_at(size_t i, home_device_t *out);
size_t home_device_count(void);
size_t home_rooms(home_room_t *out, size_t max);

err_t home_cmd(const char *device_id, const home_cmd_t *cmd);
err_t home_set_meta(const char *device_id, const char *name, const char *room_id);
void home_on_change(void (*cb)(const home_device_t *));

err_t home_form(uint8_t channel, uint16_t pan);
err_t home_permit_join(uint8_t seconds);
err_t home_remove(const char *device_id);
void home_net(home_net_t *out);
uint32_t home_gen(void);
uint8_t home_bar_level(void);
const char *home_cluster_text(home_kind_t kind);
uint32_t home_now_ms(void);

err_t home_test_announce(uint16_t nwk, const uint8_t ieee[8]);
err_t home_test_clusters(const uint8_t ieee[8], const uint16_t *in, uint8_t n);
err_t home_test_report(const uint8_t ieee[8], uint16_t cluster, uint8_t on, uint8_t level);
void home_test_force_radio(uint8_t radio_ok, uint8_t mock);

#ifdef __cplusplus
}
#endif

#endif /* HOME_H */
