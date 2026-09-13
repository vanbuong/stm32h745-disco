/*
 * zb_manu_develco.h
 *
 * Develco / frient (manufacturer code 0x1015) vendor definitions.
 *
 * Develco's manufacturer-specific frames follow the standard ZCL
 * attribute-record layout, so unlike Lumi (see manu/lumi/) there is no raw
 * payload decoder and nothing to register in the zb_zcl_manu dispatch table -
 * the generic parser handles the frames and the owning device function checks
 * msg->hdr.manuf_code. Only the identifiers live here.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_MANU_DEVELCO_H_
#define ZB_MANU_DEVELCO_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/** Develco / frient manufacturer code. */
#define ZB_MANUFACTURER_CODE_DEVELCO                        0x1015

/* Develco/frient air-quality VOC cluster (AQSZB-110). Attribute reads and
 * reports on this cluster require the Develco manufacturer code; 0xFC03 is
 * inside the manufacturer-specific range and means something else on other
 * vendors' devices, so the code is the real discriminator. */
#define ZCL_CLUSTER_ID_MS_DEVELCO_VOC                       0xFC03

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_MANU_DEVELCO_H_ */
