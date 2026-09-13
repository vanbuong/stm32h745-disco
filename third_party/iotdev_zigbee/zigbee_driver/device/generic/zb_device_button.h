#ifndef DEVICE_ZB_DEVICE_BUTTON_H_
#define DEVICE_ZB_DEVICE_BUTTON_H_

#include "device/zb_device.h"

/*
 * Momentary multi-action button on a Multistate Input cluster (0x0012).
 * The action arrives as the MultistateInput PresentValue (0x0055) attribute;
 * this driver decodes single / double / long press and emits
 * ZB_EVENT_BUTTON_ACTION for each press.
 */
typedef struct s_zb_device_button_ctx {
    uint8_t last_action;   /* e_zb_button_action_t of the most recent press */
    bool    has_action;    /* false until the first press is seen           */
} s_zb_device_button_ctx_t;

extern s_zb_function_ops_t zb_device_button_ops;

#endif /* DEVICE_ZB_DEVICE_BUTTON_H_ */
