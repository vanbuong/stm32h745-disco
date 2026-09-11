#ifndef AUTO_H
#define AUTO_H

#include "err.h"
#include "svc/home.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUTO_RULE_MAX 32

typedef enum {
    AUTO_TRIG_ON = 0,
    AUTO_TRIG_OCCUPIED,
    AUTO_TRIG_TEMP_GT,
    AUTO_TRIG_TIME
} auto_trig_t;

typedef enum { AUTO_ACT_ON = 0, AUTO_ACT_OFF, AUTO_ACT_TOGGLE } auto_act_t;

typedef struct {
    uint16_t id;
    uint8_t enabled;
    char name[HOME_NAME_MAX];
    uint8_t trig_ieee[8];
    auto_trig_t trig;
    int16_t thresh;
    uint8_t action_ieee[8];
    auto_act_t action;
    uint32_t delay_ms;
} auto_rule_t;

err_t auto_add(const auto_rule_t *r);
err_t auto_eval(const home_device_t *changed);
void auto_reset(void);
uint16_t auto_last_id(void);

#ifdef __cplusplus
}
#endif

#endif /* AUTO_H */
