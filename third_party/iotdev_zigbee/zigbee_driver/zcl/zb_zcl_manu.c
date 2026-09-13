/*
 * zb_zcl_manu.c
 *
 * Manufacturer-specific ZCL profile-wide command dispatch table.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#include "zcl/zb_zcl_manu.h"

#define TAG "ZB_ZCL_MANU"

/* Small fixed table: the number of vendors needing bespoke parsing is tiny and
 * a static array keeps the lookup allocation-free on the RX path. */
#ifndef ZB_ZCL_MANU_MAX_HANDLERS
#define ZB_ZCL_MANU_MAX_HANDLERS  8
#endif

static s_zb_zcl_manu_handler_t s_manu_handlers[ZB_ZCL_MANU_MAX_HANDLERS];
static uint8_t s_manu_handler_count;

zb_status_t
zb_zcl_manu_register_handler(const s_zb_zcl_manu_handler_t *entry)
{
    if ((entry == NULL) || (entry->handler == NULL))
    {
        return ZB_INVALID_PARAMETER;
    }

    /* Replace an identical match key rather than shadowing it, so a module can
     * be re-initialised without leaking table slots. */
    for (uint8_t i = 0; i < s_manu_handler_count; i++)
    {
        if ((s_manu_handlers[i].manuf_code == entry->manuf_code) &&
            (s_manu_handlers[i].cluster_id == entry->cluster_id) &&
            (s_manu_handlers[i].command_id == entry->command_id))
        {
            s_manu_handlers[i] = *entry;
            ZB_LOGI(TAG, "Replaced handler mfr=0x%04x cluster=0x%04x cmd=0x%02x (%s)",
                entry->manuf_code, entry->cluster_id, entry->command_id,
                entry->name ? entry->name : "-");
            return ZB_SUCCESS;
        }
    }

    if (s_manu_handler_count >= ZB_ZCL_MANU_MAX_HANDLERS)
    {
        ZB_LOGE(TAG, "Handler table full (%d), cannot add mfr=0x%04x",
            ZB_ZCL_MANU_MAX_HANDLERS, entry->manuf_code);
        return ZB_MEM_ERROR;
    }

    s_manu_handlers[s_manu_handler_count++] = *entry;
    ZB_LOGI(TAG, "Registered handler mfr=0x%04x cluster=0x%04x cmd=0x%02x (%s)",
        entry->manuf_code, entry->cluster_id, entry->command_id,
        entry->name ? entry->name : "-");
    return ZB_SUCCESS;
}

const s_zb_zcl_manu_handler_t *
zb_zcl_manu_find_handler(uint16_t manuf_code, uint16_t cluster_id, uint8_t command_id)
{
    for (uint8_t i = 0; i < s_manu_handler_count; i++)
    {
        const s_zb_zcl_manu_handler_t *e = &s_manu_handlers[i];

        if (e->manuf_code != manuf_code)
        {
            continue;
        }
        if ((e->cluster_id != ZB_ZCL_MANU_ANY_CLUSTER) && (e->cluster_id != cluster_id))
        {
            continue;
        }
        if ((e->command_id != ZB_ZCL_MANU_ANY_CMD) && (e->command_id != command_id))
        {
            continue;
        }
        return e;
    }
    return NULL;
}

void
zb_zcl_manu_clear_handlers(void)
{
    s_manu_handler_count = 0;
}
