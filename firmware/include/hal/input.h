#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    INPUT_PTR_DOWN = 0,
    INPUT_PTR_MOVE,
    INPUT_PTR_UP,
    INPUT_KEY,
    INPUT_BTN
} input_kind_t;

typedef struct {
    input_kind_t kind;
    int16_t x;
    int16_t y;
    uint8_t id;
    uint32_t t_ms;
} input_event_t;

bool input_poll(input_event_t *out);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_H */
