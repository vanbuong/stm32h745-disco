#ifndef ZB_OTA_COMMON_H_
#define ZB_OTA_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"

// OTA Header constants
#define ZB_OTA_HDR_MAGIC_NUMBER                    0x0BEEF11E
#define ZB_OTA_HDR_STACK_VERSION                   2
#define ZB_OTA_HDR_HEADER_VERSION                  0x0100
#define ZB_OTA_HDR_FIELD_CONTROL                   0

#define ZB_OTA_HEADER_LEN_MIN                      56
#define ZB_OTA_HEADER_LEN_MAX                      69
#define ZB_OTA_HEADER_LEN_MIN_ECDSA                166
#define ZB_OTA_HEADER_STRING_LENGTH                32

#define ZB_OTA_FC_SCV_PRESENT                      (0x1 << 0)
#define ZB_OTA_FC_DSF_PRESENT                      (0x1 << 1)
#define ZB_OTA_FC_HWV_PRESENT                      (0x1 << 2)

#define ZB_OTA_SUB_ELEMENT_HDR_LEN                 6

#define ZB_OTA_UPGRADE_IMAGE_TAG_ID                0
#define ZB_OTA_ECDSA_SIGNATURE_TAG_ID              1
#define ZB_OTA_EDCSA_CERTIFICATE_TAG_ID            2

TYPEDEF_STRUCT_PACKED s_zb_zcl_ota_file_info
{
    uint16_t manufacturer_id;
    uint16_t type;
    uint32_t version;
} s_zb_zcl_ota_file_info_t;

TYPEDEF_STRUCT_PACKED s_zb_zcl_ota_sub_element_hdr
{
    uint16_t tag;
    uint32_t length;
} s_zb_zcl_ota_sub_element_hdr_t;

TYPEDEF_STRUCT_PACKED s_zb_zcl_ota_header
{
    uint32_t magic_number;
    uint16_t header_version;
    uint16_t header_length;
    uint16_t field_control;
    s_zb_zcl_ota_file_info_t file_info;
    uint16_t stack_version;
    uint8_t header_string[ZB_OTA_HEADER_STRING_LENGTH];
    uint32_t image_size;
} s_zb_zcl_ota_header_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_OTA_COMMON_H_ */