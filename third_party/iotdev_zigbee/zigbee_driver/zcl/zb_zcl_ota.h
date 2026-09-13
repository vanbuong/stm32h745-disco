/*
 * zb_zcl_ota.h
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_ZCL_OTA_H_
#define ZB_ZCL_OTA_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "zcl/zb_zcl.h"
#include "ota/zb_ota_common.h"

/*********************************************************************
 * CONSTANTS
 */
#define ZCL_OTA_MAX_MTU_BYTES                         64
// OTA Cluster Command Frame Payload Lengths
#define ZCL_OTA_PAYLOAD_MAX_LEN_IMAGE_NOTIFY                  10
#define ZCL_OTA_PAYLOAD_MAX_LEN_QUERY_NEXT_IMAGE_REQ          11
#define ZCL_OTA_PAYLOAD_MAX_LEN_QUERY_NEXT_IMAGE_RSP          13
#define ZCL_OTA_PAYLOAD_MAX_LEN_IMAGE_BLOCK_REQ               24
#define ZCL_OTA_PAYLOAD_MAX_LEN_IMAGE_PAGE_REQ                26
#define ZCL_OTA_PAYLOAD_MAX_LEN_IMAGE_BLOCK_RSP               14
#define ZCL_OTA_PAYLOAD_MAX_LEN_UPGRADE_END_REQ               9
#define ZCL_OTA_PAYLOAD_MAX_LEN_UPGRADE_END_RSP               16
#define ZCL_OTA_PAYLOAD_MAX_LEN_QUERY_SPECIFIC_FILE_REQ       18
#define ZCL_OTA_PAYLOAD_MAX_LEN_QUERY_SPECIFIC_FILE_RSP       13
#define ZCL_OTA_PAYLOAD_MIN_LEN_IMAGE_NOTIFY                  2
#define ZCL_OTA_PAYLOAD_MIN_LEN_QUERY_NEXT_IMAGE_REQ          9
#define ZCL_OTA_PAYLOAD_MIN_LEN_QUERY_NEXT_IMAGE_RSP          1
#define ZCL_OTA_PAYLOAD_MIN_LEN_IMAGE_BLOCK_REQ               14
#define ZCL_OTA_PAYLOAD_MIN_LEN_IMAGE_PAGE_REQ                18
#define ZCL_OTA_PAYLOAD_MIN_LEN_IMAGE_BLOCK_RSP               14
#define ZCL_OTA_PAYLOAD_MIN_LEN_IMAGE_BLOCK_WAIT              11
#define ZCL_OTA_PAYLOAD_MIN_LEN_UPGRADE_END_REQ               1
#define ZCL_OTA_PAYLOAD_MIN_LEN_UPGRADE_END_RSP               16
#define ZCL_OTA_PAYLOAD_MIN_LEN_QUERY_SPECIFIC_FILE_REQ       18
#define ZCL_OTA_PAYLOAD_MIN_LEN_QUERY_SPECIFIC_FILE_RSP       1
#define ZCL_OTA_PAYLOAD_LEN_QUERY_RSP_ADDR_MODE_OFFSET                    8
#define ZCL_OTA_PAYLOAD_FILE_READ_RSP_PAYLOAD_LEN_16BIT_ADDR_MODE        20     //This len is counting address mode of 16 bit
#define ZCL_OTA_PAYLOAD_FILE_READ_RSP_PAYLOAD_OFFSET_16BIT_ADDR_MODE     19     //This offset is counting address mode of 16 bit
#define ZCL_OTA_PAYLOAD_FILE_READ_RSP_PAYLOAD_LEN_64BIT_ADDR_MODE        26     //This len is counting address mode of 64 bit
#define ZCL_OTA_PAYLOAD_FILE_READ_RSP_PAYLOAD_OFFSET_64BIT_ADDR_MODE     25     //This offset is counting address mode of 64 bit

/**
 * @brief OTA Attribute IDs
 */
#define ZCL_OTA_ATTRID_OTA_UPGRADE_SERVER_ID                          0x0000
#define ZCL_OTA_ATTRID_OTA_UPGRADE_FILE_OFFSET                        0x0001
#define ZCL_OTA_ATTRID_OTA_UPGRADE_CURRENT_FILE_VERSION               0x0002
#define ZCL_OTA_ATTRID_OTA_UPGRADE_CURRENT_ZIGBEE_STACK_VERSION       0x0003
#define ZCL_OTA_ATTRID_OTA_UPGRADE_DOWNLOADED_FILE_VERSION            0x0004
#define ZCL_OTA_ATTRID_OTA_UPGRADE_DOWNLOADED_ZIGBEE_STACK_VERSION    0x0005
#define ZCL_OTA_ATTRID_OTA_UPGRADE_STATUS                             0x0006
#define ZCL_OTA_ATTRID_OTA_UPGRADE_MANUFACTURER_ID                    0x0007
#define ZCL_OTA_ATTRID_OTA_UPGRADE_IMAGE_TYPE_ID                      0x0008
#define ZCL_OTA_ATTRID_OTA_UPGRADE_MINIMUM_BLOCK_PERIOD               0x0009
#define ZCL_OTA_ATTRID_OTA_UPGRADE_IMAGE_STAMP                        0x000A
#define ZCL_OTA_ATTRID_OTA_UPGRADE_UPGRADE_ACTIVATION_POLICY          0x000B
#define ZCL_OTA_ATTRID_OTA_UPGRADE_UPGRADE_TIMEOUT_POLICY             0x000C

/**
 * @brief OTA Upgrade Status
 */
#define ZB_OTA_UPGRADE_STATUS_NORMAL         0x00
#define ZB_OTA_UPGRADE_STATUS_IN_PROGRESS    0x01
#define ZB_OTA_UPGRADE_STATUS_COMPLETE       0x02
#define ZB_OTA_UPGRADE_STATUS_UPGRADE_WAIT   0x03
#define ZB_OTA_UPGRADE_STATUS_COUNTDOWN      0x04
#define ZB_OTA_UPGRADE_STATUS_WAIT_FOR_MORE  0x05

/**
 * @brief ZCL OTA Status
 */
#define ZCL_OTA_STATUS_SUCCESS            0x00
#define ZCL_OTA_STATUS_NOT_AUTHORIZED     0x7E
#define ZCL_OTA_STATUS_MALFORMED_COMMAND  0x80
#define ZCL_OTA_STATUS_UNSUP_COMMAND      0x81
#define ZCL_OTA_STATUS_ABORT              0x95
#define ZCL_OTA_STATUS_INVALID_IMAGE      0x96
#define ZCL_OTA_STATUS_WAIT_FOR_DATA      0x97
#define ZCL_OTA_STATUS_NO_IMAGE_AVAILABLE 0x98
#define ZCL_OTA_STATUS_REQUIRE_MORE_IMAGE 0x99

/**
 * @brief OTA Upgrade Time Wait for upgrade
 */
#define ZCL_OTA_UPGRADE_TIME_WAIT         0xFFFFFFFF

/**
 * @brief OTA Cluster Commands
 */
// The purpose of sending Image Notify command is so the server has a way to notify the
// client devices of when the OTA upgrade images are available for them.
#define ZCL_OTA_COMMAND_OTA_UPGRADE_IMAGE_NOTIFY                          0x00
// Client devices SHALL send Query Next Image Request command to the server to see if
// there is new OTA upgrade image available.
#define ZCL_OTA_COMMAND_OTA_UPGRADE_QUERY_NEXT_IMAGE_REQUEST              0x01
// The upgrade server sends a Query Next Image Response with one of the following status:
// SUCCESS, NO_IMAGE_AVAILABLE or NOT_AUTHORIZED. When a SUCCESS status is sent, it is
// considered to be the explicit authorization to a device by the upgrade server that
// the device MAY upgrade to a specific software image.
#define ZCL_OTA_COMMAND_OTA_UPGRADE_QUERY_NEXT_IMAGE_RESPONSE             0x02
// The client device requests the iamge data at its leisure by sending Image Block Request
// command to the upgrade server.
#define ZCL_OTA_COMMAND_OTA_UPGRADE_IMAGE_BLOCK_REQUEST                   0x03
// The support for the command is optional. The client device MAY choose to request OTA
// upgrade data in one page size at a time from upgrade server.
#define ZCL_OTA_COMMAND_OTA_UPGRADE_IMAGE_PAGE_REQUEST                    0x04
// Upon receipt of an Image Block Request command the server SHALL generate an 
// Image Block Response.
#define ZCL_OTA_COMMAND_OTA_UPGRADE_IMAGE_BLOCK_RESPONSE                  0x05
// Upon reception all the image data, the client SHOULD verify the image to ensure
// its integrity and validity.
#define ZCL_OTA_COMMAND_OTA_UPGRADE_UPGRADE_END_REQUEST                   0x06
// When an upgrade server receives an Upgrade End Request command with a status of
// INVALID_IMAGE, REQUIRE_MOR_IMAGE, or ABORT, no additional processing SHALL be done
// in its part. If the upgrade server receives an Upgrade End Request command with a status
// of SUCCESS, it SHALL generate an Upgrade End Response 17245 with the manufacturer code
// and image type received in the Upgrade End Request along with the times indicating when
// the device SHOULD upgrade to the new image.
#define ZCL_OTA_COMMAND_OTA_UPGRADE_UPGRADE_END_RESPONSE                  0x07
// Client devices SHALL send a Query Device Specific File Request command to the server
// to request for a file that is specific and unique to it.
#define ZCL_OTA_COMMAND_OTA_UPGRADE_QUERY_DEVICE_SPECIFIC_FILE_REQUEST    0x08
// The server sends Query Device Specific File Response after receiving Query Device
// Specific File Request from a client.
#define ZCL_OTA_COMMAND_OTA_UPGRADE_QUERY_DEVICE_SPECIFIC_FILE_RESPONSE   0x09

/**
 * @brief OTA Upgrade Image Block Request Field Control Bitmask
 */
#define ZCL_OTA_BLOCK_FC_GENERIC              0x00
#define ZCL_OTA_BLOCK_FC_NODES_IEEE_PRESENT   0x01
#define ZCL_OTA_BLOCK_FC_REQ_DELAY_PRESENT    0x02

/**
 * @brief OTA Upgrade Image Notify Command Payload Type
 */
#define ZCL_OTA_IMAGE_NOTIFY_PAYLOAD_JITTER                   0x00
#define ZCL_OTA_IMAGE_NOTIFY_PAYLOAD_JITTER_MFG               0x01
#define ZCL_OTA_IMAGE_NOTIFY_PAYLOAD_JITTER_MFG_TYPE          0x02
#define ZCL_OTA_IMAGE_NOTIFY_PAYLOAD_JITTER_MFG_TYPE_VERSION  0x03

/**
 * @brief OTA Upgrade delay is the number of seconds before the client
 * should wait before switching to the upgrade image.
 */
#define ZCL_OTA_UPGRADE_DELAY_NONE            60
#define ZCL_OTA_SEND_BLOCK_WAIT               0

/*********************************************************************
 * TYPEDEFS
 */
typedef struct s_zb_zcl_ota_image_notify_params
{
    uint8_t payload_type;
    uint8_t query_jitter;
    s_zb_zcl_ota_file_info_t file_info;
} s_zb_zcl_ota_image_notify_params_t;

typedef struct s_zb_zcl_ota_query_next_image_req_params
{
    uint8_t field_control;
    s_zb_zcl_ota_file_info_t file_info;
    uint16_t hardware_version;
} s_zb_zcl_ota_query_next_image_req_params_t;

typedef struct s_zb_zcl_ota_query_image_rsp_params
{
    uint8_t status;
    s_zb_zcl_ota_file_info_t file_info;
    uint32_t image_size;
} s_zb_zcl_ota_query_image_rsp_params_t;

typedef struct s_zb_zcl_ota_image_block_req_params
{
    uint8_t field_control;
    s_zb_zcl_ota_file_info_t file_info;
    uint32_t file_offset;
    uint8_t max_data_size;
    uint64_t node_addr;
    uint16_t block_req_delay;
} s_zb_zcl_ota_image_block_req_params_t;

typedef struct s_zb_zcl_ota_image_page_req_params
{
    uint8_t field_control;
    s_zb_zcl_ota_file_info_t file_info;
    uint32_t file_offset;
    uint8_t max_data_size;
    uint16_t page_size;
    uint16_t response_spacing;
    uint64_t node_addr;
} s_zb_zcl_ota_image_page_req_params_t;

typedef struct s_zb_zcl_ota_image_block_rsp_success
{
    s_zb_zcl_ota_file_info_t file_info;
    uint32_t file_offset;
    uint8_t data_size;
    uint8_t *data;
} s_zb_zcl_ota_image_block_rsp_success_t;

typedef struct s_zb_zcl_ota_image_block_rsp_wait
{
    uint32_t current_time;
    uint32_t request_time;
    uint16_t block_req_delay;
} s_zb_zcl_ota_image_block_rsp_wait_t;

typedef union s_zb_zcl_ota_image_block_rsp
{
    s_zb_zcl_ota_image_block_rsp_success_t success;
    s_zb_zcl_ota_image_block_rsp_wait_t wait;
} s_zb_zcl_ota_image_block_rsp_t;

typedef struct s_zb_zcl_ota_image_block_rsp_params
{
    uint8_t status;
    s_zb_zcl_ota_image_block_rsp_t rsp;
} s_zb_zcl_ota_image_block_rsp_params_t;

TYPEDEF_STRUCT_PACKED s_zb_zcl_ota_upgrade_end_req_params
{
    uint8_t status;
    s_zb_zcl_ota_file_info_t file_info;
} s_zb_zcl_ota_upgrade_end_req_params_t;

typedef struct s_zb_zcl_ota_upgrade_end_rsp_params
{
    s_zb_zcl_ota_file_info_t file_info;
    uint32_t current_time;
    uint32_t upgrade_time;
} s_zb_zcl_ota_upgrade_end_rsp_params_t;

typedef struct s_zb_zcl_ota_query_device_specific_file_req_params
{
    uint64_t node_addr;
    s_zb_zcl_ota_file_info_t file_info;
    uint16_t stack_version;
} s_zb_zcl_ota_query_device_specific_file_req_params_t;

typedef struct s_zb_zcl_ota_query_device_specific_file_rsp_params
{
    uint8_t status;
    s_zb_zcl_ota_file_info_t file_info;
} s_zb_zcl_ota_query_device_specific_file_rsp_params_t;

/*********************************************************************
 * FUNCTIONS
 */
int zb_zcl_ota_send_image_notify(
    uint8_t src_endpoint, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ota_image_notify_params_t *params);

int zb_zcl_ota_send_query_specific_file_rsp(
    uint8_t src_endpoint, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ota_query_image_rsp_params_t *params, uint8_t seq_num);

int zb_zcl_ota_send_query_next_image_rsp(
    uint8_t src_endpoint, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ota_query_image_rsp_params_t *params, uint8_t seq_num);

int zb_zcl_ota_send_image_block_rsp(
    uint8_t src_endpoint, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ota_image_block_rsp_params_t *params, uint8_t seq_num);

int zb_zcl_ota_send_upgrade_end_rsp(
    uint8_t src_endpoint, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ota_upgrade_end_rsp_params_t *params, uint8_t seq_num);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZCL_OTA_H_ */
