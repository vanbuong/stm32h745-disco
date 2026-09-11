#ifndef UI_EVENT_H
#define UI_EVENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UI_EV_BACK = 0, UI_EV_HOME, UI_EV_TAP } ui_ev_kind_t;

typedef struct {
    ui_ev_kind_t kind;
    int16_t x;
    int16_t y;
} ui_event_t;

#ifdef __cplusplus
}
#endif

#endif /* UI_EVENT_H */
