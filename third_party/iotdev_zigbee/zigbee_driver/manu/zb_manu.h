/*
 * zb_manu.h
 *
 * Vendor (manufacturer) support layer.
 *
 * One subdirectory per vendor under manu/. Each owns its manufacturer code,
 * its manufacturer-specific cluster and attribute ids, and - when the vendor
 * ships frames the generic ZCL parser cannot walk - a raw-payload decoder plus
 * the rows that register it in the zb_zcl_manu dispatch table.
 *
 * Including this header pulls in every vendor's identifiers, which is what the
 * device schema and device functions want; the decoders themselves are private
 * to their own translation unit.
 *
 * To add a vendor: create manu/<vendor>/zb_manu_<vendor>.[ch], add the source
 * to the component CMakeLists, include the header here, and call its
 * zb_manu_<vendor>_register() from zb_manu_register_all() if it has decoders.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_MANU_H_
#define ZB_MANU_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"

#include "manu/develco/zb_manu_develco.h"
#include "manu/lumi/zb_manu_lumi.h"
#include "manu/tuya/zb_manu_tuya.h"

/**
 * @brief Register every vendor's manufacturer-specific frame handlers.
 *
 * Called once from zb_core_zcl_application_init(), after the ZCL layer is up.
 * Registration order across vendors does not matter (rows are keyed by
 * manufacturer code); order within a vendor does - see
 * zb_zcl_manu_register_handler().
 *
 * @return ZB_SUCCESS when every vendor registered cleanly, otherwise the first
 *         failure (the remaining vendors are still attempted).
 */
zb_status_t zb_manu_register_all(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_MANU_H_ */
