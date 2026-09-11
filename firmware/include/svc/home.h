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

typedef enum { HOME_LIGHT = 0, HOME_SWITCH, HOME_BINARY_SENSOR, HOME_CLIMATE } home_kind_t;

typedef struct {
    char id[HOME_NAME_MAX];
    char name[HOME_NAME_MAX];
} home_room_t;

typedef struct {
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

size_t home_devices(const char *room_id, home_device_t *out, size_t max);
err_t home_cmd(const char *device_id, const home_cmd_t *cmd);
err_t home_set_meta(const char *device_id, const char *name, const char *room_id);
void home_on_change(void (*cb)(const home_device_t *));

#ifdef __cplusplus
}
#endif

#endif /* HOME_H */
