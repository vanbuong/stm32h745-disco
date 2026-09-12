#ifndef AUTO_H
#define AUTO_H

#include "err.h"
#include "svc/home.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUTO_RULE_MAX 32
#define AUTO_PEND_MAX 8
#define AUTO_RULES_PATH "/user/home/rules.bin"

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

typedef struct {
    uint8_t ieee[8];
    home_cmd_t cmd;
    uint16_t rule_id;
    uint8_t toggle;
} auto_act_req_t;

err_t auto_init(void);
void auto_reset(void);
void auto_poll(uint32_t dt_ms);

err_t auto_add(const auto_rule_t *r);
err_t auto_set_enabled(uint16_t id, uint8_t on);
err_t auto_remove(uint16_t id);
size_t auto_count(void);
err_t auto_at(size_t i, auto_rule_t *out);
err_t auto_get(uint16_t id, auto_rule_t *out);

err_t auto_eval(const home_device_t *changed);
err_t auto_take_due(auto_act_req_t *out);
uint16_t auto_last_id(void);

void auto_test_set_minutes(int minutes);

#ifdef __cplusplus
}
#endif

#endif /* AUTO_H */
