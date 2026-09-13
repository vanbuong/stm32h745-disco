/*
 * zb_manu.c
 *
 * Vendor support layer: registers every vendor's frame decoders.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#include "manu/zb_manu.h"

#define TAG "ZB_MANU"

zb_status_t
zb_manu_register_all(void)
{
    zb_status_t status = ZB_SUCCESS;
    zb_status_t vendor_status;

    /* Every vendor is attempted even if an earlier one failed: a full dispatch
     * table must not silently cost the remaining vendors their decoders. */
    vendor_status = zb_manu_lumi_register();
    if (vendor_status != ZB_SUCCESS)
    {
        ZB_LOGE(TAG, "Lumi handler registration failed (%d)", vendor_status);
        status = (status == ZB_SUCCESS) ? vendor_status : status;
    }

    vendor_status = zb_manu_tuya_register();
    if (vendor_status != ZB_SUCCESS)
    {
        ZB_LOGE(TAG, "Tuya handler registration failed (%d)", vendor_status);
        status = (status == ZB_SUCCESS) ? vendor_status : status;
    }

    /* Develco/frient needs no decoder - see manu/develco/zb_manu_develco.h. */

    return status;
}
