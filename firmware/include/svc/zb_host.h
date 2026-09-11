#ifndef ZB_HOST_H
#define ZB_HOST_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t channel_mask;
    uint16_t pan_id;
} zb_net_cfg_t;

err_t zb_form(const zb_net_cfg_t *cfg);
err_t zb_permit_join(uint8_t seconds);
err_t zb_leave(const uint8_t ieee[8]);
err_t zb_interview(const uint8_t ieee[8]);

#ifdef __cplusplus
}
#endif

#endif /* ZB_HOST_H */
