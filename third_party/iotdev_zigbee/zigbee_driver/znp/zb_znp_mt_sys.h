/*
 * zb_znp_mt_sys.h
 * 
 * This module contains the definitions and function prototypes for the ZNP MT SYS Interface.
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_ZNP_MT_SYS_H_
#define ZB_ZNP_MT_SYS_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"

/**************************************************************************************************
 * SYS COMMANDS
 *************************************************************************************************/

/* AREQ from host */
#define ZNP_SYS_RESET_REQ                     0x00
#define ZNP_RESET_TYPE_HARDWARE               0x00
#define ZNP_RESET_TYPE_SOFTWARE               0x01

/* SREQ/SRSP */
#define ZNP_SYS_PING                          0x01
#define ZNP_SYS_VERSION                       0x02
#define ZNP_SYS_SET_EXTADDR                   0x03
#define ZNP_SYS_GET_EXTADDR                   0x04
#define ZNP_SYS_RAM_READ                      0x05
#define ZNP_SYS_RAM_WRITE                     0x06
#define ZNP_SYS_OSAL_NV_ITEM_INIT             0x07
#define ZNP_SYS_OSAL_NV_READ                  0x08
#define ZNP_SYS_OSAL_NV_WRITE                 0x09
#define ZNP_SYS_OSAL_START_TIMER              0x0A
#define ZNP_SYS_OSAL_STOP_TIMER               0x0B
#define ZNP_SYS_RANDOM                        0x0C
#define ZNP_SYS_ADC_READ                      0x0D
#define ZNP_SYS_GPIO                          0x0E
#define ZNP_SYS_STACK_TUNE                    0x0F
#define ZNP_SYS_SET_TIME                      0x10
#define ZNP_SYS_GET_TIME                      0x11
#define ZNP_SYS_OSAL_NV_DELETE                0x12
#define ZNP_SYS_OSAL_NV_LENGTH                0x13
#define ZNP_SYS_SET_TX_POWER                  0x14
#define ZNP_SYS_GET_TX_POWER                  0x15
#define ZNP_SYS_GET_TEMPERATURE               0x1E
#define ZNP_SYS_GET_HEAP_STATISTICS           0x1F

/* AREQ to host */
#define ZNP_SYS_RESET_IND                     0x80
#define ZNP_SYS_OSAL_TIMER_EXPIRED            0x81

// OSAL NV item IDs
#define ZNP_ZCD_NV_EXTADDR                    0x0001
#define ZNP_ZCD_NV_BOOTCOUNTER                0x0002
#define ZNP_ZCD_NV_STARTUP_OPTION             0x0003
#define ZNP_ZCD_NV_START_DELAY                0x0004

// NWK Layer NV item IDs
#define ZNP_ZCD_NV_NIB                        0x0021
#define ZNP_ZCD_NV_DEVICE_LIST                0x0022
#define ZNP_ZCD_NV_ADDRMGR                    0x0023
#define ZNP_ZCD_NV_POLL_RATE                  0x0024
#define ZNP_ZCD_NV_QUEUED_POLL_RATE           0x0025
#define ZNP_ZCD_NV_RESPONSE_POLL_RATE         0x0026
#define ZNP_ZCD_NV_REJOIN_POLL_RATE           0x0027
#define ZNP_ZCD_NV_DATA_RETRIES               0x0028
#define ZNP_ZCD_NV_POLL_FAILURE_RETRIES       0x0029
#define ZNP_ZCD_NV_STACK_PROFILE              0x002A
#define ZNP_ZCD_NV_INDIRECT_MSG_TIMEOUT       0x002B
#define ZNP_ZCD_NV_ROUTE_EXPIRY_TIME          0x002C
#define ZNP_ZCD_NV_EXTENDED_PAN_ID            0x002D
#define ZNP_ZCD_NV_BCAST_RETRIES              0x002E
#define ZNP_ZCD_NV_PASSIVE_ACK_TIMEOUT        0x002F
#define ZNP_ZCD_NV_BCAST_DELIVERY_TIME        0x0030
#define ZNP_ZCD_NV_NWK_MODE                   0x0031
#define ZNP_ZCD_NV_CONCENTRATOR_ENABLE        0x0032
#define ZNP_ZCD_NV_CONCENTRATOR_DISCOVERY     0x0033
#define ZNP_ZCD_NV_CONCENTRATOR_RADIUS        0x0034
#define ZNP_ZCD_NV_CONCENTRATOR_RC            0x0036
#define ZNP_ZCD_NV_NWK_MGR_MODE               0x0037
#define ZNP_ZCD_NV_SRC_RTG_EXPIRY_TIME        0x0038
#define ZNP_ZCD_NV_ROUTE_DISCOVERY_TIME       0x0039
#define ZNP_ZCD_NV_NWK_ACTIVE_KEY_INFO        0x003A
#define ZNP_ZCD_NV_NWK_ALTERN_KEY_INFO        0x003B
#define ZNP_ZCD_NV_ROUTER_OFF_ASSOC_CLEANUP   0x003C
#define ZNP_ZCD_NV_NWK_LEAVE_REQ_ALLOWED      0x003D
#define ZNP_ZCD_NV_NWK_CHILD_AGE_ENABLE       0x003E
#define ZNP_ZCD_NV_DEVICE_LIST_KA_TIMEOUT     0x003F

// APS Layer NV item IDs
#define ZNP_ZCD_NV_BINDING_TABLE              0x0041
#define ZNP_ZCD_NV_GROUP_TABLE                0x0042
#define ZNP_ZCD_NV_APS_FRAME_RETRIES          0x0043
#define ZNP_ZCD_NV_APS_ACK_WAIT_DURATION      0x0044
#define ZNP_ZCD_NV_APS_ACK_WAIT_MULTIPLIER    0x0045
#define ZNP_ZCD_NV_BINDING_TIME               0x0046
#define ZNP_ZCD_NV_APS_USE_EXT_PANID          0x0047
#define ZNP_ZCD_NV_APS_USE_INSECURE_JOIN      0x0048
#define ZNP_ZCD_NV_COMMISSIONED_NWK_ADDR      0x0049

#define ZNP_ZCD_NV_APS_NONMEMBER_RADIUS       0x004B     // Multicast non_member radius
#define ZNP_ZCD_NV_APS_LINK_KEY_TABLE         0x004C
#define ZNP_ZCD_NV_APS_DUPREJ_TIMEOUT_INC     0x004D
#define ZNP_ZCD_NV_APS_DUPREJ_TIMEOUT_COUNT   0x004E
#define ZNP_ZCD_NV_APS_DUPREJ_TABLE_SIZE      0x004F

// Security NV Item IDs
#define ZNP_ZCD_NV_SECURITY_LEVEL             0x0061
#define ZNP_ZCD_NV_PRECFGKEY                  0x0062
#define ZNP_ZCD_NV_PRECFGKEYS_ENABLE          0x0063
#define ZNP_ZCD_NV_SECURITY_MODE              0x0064
#define ZNP_ZCD_NV_SECURE_PERMIT_JOIN         0x0065
#define ZNP_ZCD_NV_APS_LINK_KEY_TYPE          0x0066
#define ZNP_ZCD_NV_APS_ALLOW_R19_SECURITY     0x0067

#define ZNP_ZCD_NV_IMPLICIT_CERTIFICATE       0x0069
#define ZNP_ZCD_NV_DEVICE_PRIVATE_KEY         0x006A
#define ZNP_ZCD_NV_CA_PUBLIC_KEY              0x006B

#define ZNP_ZCD_NV_USE_DEFAULT_TCLK           0x006D
#define ZNP_ZCD_NV_TRUSTCENTER_ADDR           0x006E
#define ZNP_ZCD_NV_RNG_COUNTER                0x006F
#define ZNP_ZCD_NV_RANDOM_SEED                0x0070

// ZDO NV Item IDs
#define ZNP_ZCD_NV_USERDESC                   0x0081
#define ZNP_ZCD_NV_NWKKEY                     0x0082
#define ZNP_ZCD_NV_PANID                      0x0083
#define ZNP_ZCD_NV_CHANLIST                   0x0084
#define ZNP_ZCD_NV_LEAVE_CTRL                 0x0085
#define ZNP_ZCD_NV_SCAN_DURATION              0x0086
#define ZNP_ZCD_NV_LOGICAL_TYPE               0x0087
#define ZNP_ZCD_NV_NWKMGR_MIN_TX              0x0088
#define ZNP_ZCD_NV_NWKMGR_ADDR                0x0089

#define ZNP_ZCD_NV_ZDO_DIRECT_CB              0x008F

// ZCL NV item IDs
#define ZNP_ZCD_NV_SCENE_TABLE                0x0091
#define ZNP_ZCD_NV_MIN_FREE_NWK_ADDR          0x0092
#define ZNP_ZCD_NV_MAX_FREE_NWK_ADDR          0x0093
#define ZNP_ZCD_NV_MIN_FREE_GRP_ID            0x0094
#define ZNP_ZCD_NV_MAX_FREE_GRP_ID            0x0095
#define ZNP_ZCD_NV_MIN_GRP_IDS                0x0096
#define ZNP_ZCD_NV_MAX_GRP_IDS                0x0097

// Non-standard NV item IDs
#define ZNP_ZCD_NV_SAPI_ENDPOINT              0x00A1

// NV Items Reserved for Commissioning Cluster Startup Attribute Set (SAS):
// 0x00B1 - 0x00BF: Parameters related to APS and NWK layers
// 0x00C1 - 0x00CF: Parameters related to Security
// 0x00D1 - 0x00DF: Current key parameters
#define ZNP_ZCD_NV_SAS_SHORT_ADDR             0x00B1
#define ZNP_ZCD_NV_SAS_EXT_PANID              0x00B2
#define ZNP_ZCD_NV_SAS_PANID                  0x00B3
#define ZNP_ZCD_NV_SAS_CHANNEL_MASK           0x00B4
#define ZNP_ZCD_NV_SAS_PROTOCOL_VER           0x00B5
#define ZNP_ZCD_NV_SAS_STACK_PROFILE          0x00B6
#define ZNP_ZCD_NV_SAS_STARTUP_CTRL           0x00B7

#define ZNP_ZCD_NV_SAS_TC_ADDR                0x00C1
#define ZNP_ZCD_NV_SAS_TC_MASTER_KEY          0x00C2
#define ZNP_ZCD_NV_SAS_NWK_KEY                0x00C3
#define ZNP_ZCD_NV_SAS_USE_INSEC_JOIN         0x00C4
#define ZNP_ZCD_NV_SAS_PRECFG_LINK_KEY        0x00C5
#define ZNP_ZCD_NV_SAS_NWK_KEY_SEQ_NUM        0x00C6
#define ZNP_ZCD_NV_SAS_NWK_KEY_TYPE           0x00C7
#define ZNP_ZCD_NV_SAS_NWK_MGR_ADDR           0x00C8

#define ZNP_ZCD_NV_SAS_CURR_TC_MASTER_KEY     0x00D1
#define ZNP_ZCD_NV_SAS_CURR_NWK_KEY           0x00D2
#define ZNP_ZCD_NV_SAS_CURR_PRECFG_LINK_KEY   0x00D3

// NV Items Reserved for Trust Center Link Key Table entries
// 0x0101 - 0x01FF
#define ZNP_ZCD_NV_TCLK_TABLE_START           0x0101
#define ZNP_ZCD_NV_TCLK_TABLE_END             0x01FF

// NV Items Reserved for APS Link Key Table entries
// 0x0201 - 0x02FF
#define ZNP_ZCD_NV_APS_LINK_KEY_DATA_START    0x0201     // APS key data
#define ZNP_ZCD_NV_APS_LINK_KEY_DATA_END      0x02FF

// NV Items Reserved for Master Key Table entries
// 0x0301 - 0x03FF
#define ZNP_ZCD_NV_MASTER_KEY_DATA_START      0x0301     // Master key data
#define ZNP_ZCD_NV_MASTER_KEY_DATA_END        0x03FF

// NV Items Reserved for applications (user applications)
// 0x0401 - 0x0FFF

// ZNP_ZCD_STARTUP_OPTION values
//   These are bit weighted - you can OR these together.
//   Setting one of these bits will set their associated NV items
//   to code initialized values.
#define ZNP_ZCD_STARTOPT_DEFAULT_CONFIG_STATE  0x01
#define ZNP_ZCD_STARTOPT_DEFAULT_NETWORK_STATE 0x02
#define ZNP_ZCD_STARTOPT_AUTO_START            0x04
#define ZNP_ZCD_STARTOPT_CLEAR_CONFIG          ZNP_ZCD_STARTOPT_DEFAULT_CONFIG_STATE
#define ZNP_ZCD_STARTOPT_CLEAR_STATE           ZNP_ZCD_STARTOPT_DEFAULT_NETWORK_STATE
#define ZNP_ZCD_STARTOPT_CLEAR_NWK_FRAME_COUNTER 0x80

// ZNP_ZCD_LOGICAL_TYPE values
#define ZNP_ZCD_DEVICETYPE_COORDINATOR         0x00
#define ZNP_ZCD_DEVICETYPE_ROUTER              0x01
#define ZNP_ZCD_DEVICETYPE_ENDDEVICE           0x02

#define ZNP_ZCL_KE_IMPLICIT_CERTIFICATE_LEN    48
#define ZNP_ZCL_KE_CA_PUBLIC_KEY_LEN           22
#define ZNP_ZCL_KE_DEVICE_PRIVATE_KEY_LEN      21

/**************************************************************************************************
 * TYPEDEFS
 *************************************************************************************************/

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_reset_req
{
    uint8_t reset_type;
} s_zb_znp_mt_sys_reset_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_reset_ind
{
    uint8_t reason;
    uint8_t transport_revision;
    uint8_t product_id;
    uint8_t major_rel;
    uint8_t minor_rel;
    uint8_t maint_rel;
    uint8_t fw_major_rel;
    uint8_t fw_minor_rel;
    uint8_t fw_maint_rel;
} s_zb_znp_mt_sys_reset_ind_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_ping_srsp
{
    uint16_t capabilities;
} s_zb_znp_mt_sys_ping_srsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_version_srsp
{
    uint8_t transport_revision;
    uint8_t product_id;
    uint8_t major_rel;
    uint8_t minor_rel;
    uint8_t maint_rel;
    uint8_t fw_major_rel;
    uint8_t fw_minor_rel;
    uint8_t fw_maint_rel;
} s_zb_znp_mt_sys_version_srsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_set_extaddr_cmd
{
    uint8_t ext_addr[8];
} s_zb_znp_mt_sys_set_extaddr_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_get_extaddr_srsp
{
    uint64_t ext_addr;
} s_zb_znp_mt_sys_get_extaddr_srsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_ram_read_cmd
{
    uint16_t address;
    uint8_t length;
} s_zb_znp_mt_sys_ram_read_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_ram_read_srsp
{
    uint8_t len;
    uint8_t value[128];
} s_zb_znp_mt_sys_ram_read_srsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_ram_write_cmd
{
    uint16_t address;
    uint8_t len;
    uint8_t value[128];
} s_zb_znp_mt_sys_ram_write_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_osal_nv_read_cmd
{
    uint16_t id;
    uint8_t offset;
} s_zb_znp_mt_sys_osal_nv_read_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_osal_nv_read_srsp
{
    uint8_t len;
    uint8_t value[248];
} s_zb_znp_mt_sys_osal_nv_read_srsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_osal_nv_write_cmd
{
    uint16_t id;
    uint8_t offset;
    uint8_t len;
    uint8_t value[246];
} s_zb_znp_mt_sys_osal_nv_write_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_osal_nv_item_init_cmd
{
    uint16_t id;
    uint16_t item_len;
    uint8_t init_len;
    uint8_t init_data[245];
} s_zb_znp_mt_sys_osal_nv_item_init_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_osal_nv_delete_cmd
{
    uint16_t id;
    uint16_t item_len;
} s_zb_znp_mt_sys_osal_nv_delete_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_osal_nv_length_cmd
{
    uint16_t id;
} s_zb_znp_mt_sys_osal_nv_length_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_osal_nv_length_srsp
{
    uint16_t item_len;
} s_zb_znp_mt_sys_osal_nv_length_srsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_osal_start_timer_cmd
{
    uint8_t timer_id;
    uint16_t timeout;
} s_zb_znp_mt_sys_osal_start_timer_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_osal_stop_timer_cmd
{
    uint8_t timer_id;
} s_zb_znp_mt_sys_osal_stop_timer_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_osal_timer_expired_ind
{
    uint8_t timer_id;
} s_zb_znp_mt_sys_osal_timer_expired_ind_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_random_srsp
{
    uint16_t value;
} s_zb_znp_mt_sys_random_srsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_adc_read_cmd
{
    uint8_t channel;
    uint8_t resolution;
} s_zb_znp_mt_sys_adc_read_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_adc_read_srsp
{
    uint16_t value;
} s_zb_znp_mt_sys_adc_read_srsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_gpio_cmd
{
    uint8_t operation;
    uint8_t value;
} s_zb_znp_mt_sys_gpio_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_gpio_srsp
{
    uint8_t value;
} s_zb_znp_mt_sys_gpio_srsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_stack_tune_cmd
{
    uint8_t operation;
    uint8_t value;
} s_zb_znp_mt_sys_stack_tune_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_stack_tune_srsp
{
    uint8_t value;
} s_zb_znp_mt_sys_stack_tune_srsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_set_time_cmd
{
    uint8_t utc_time[4];
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t month;
    uint8_t day;
    uint8_t year;
} s_zb_znp_mt_sys_set_time_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_get_time_srsp
{
    uint32_t utc_time;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t month;
    uint8_t day;
    uint8_t year;
} s_zb_znp_mt_sys_get_time_srsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_set_tx_power_cmd
{
    int8_t tx_power;
} s_zb_znp_mt_sys_set_tx_power_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_sys_get_tx_power_srsp
{
    int8_t tx_power;
} s_zb_znp_mt_sys_get_tx_power_srsp_t;

/**************************************************************************************************
 * CALLBACKS DEFINITION
 *************************************************************************************************/
typedef int (*zb_znp_mt_sys_reset_ind_callback_t)(const s_zb_znp_mt_sys_reset_ind_t *msg);
typedef int (*zb_znp_mt_sys_osal_timer_expired_ind_callback_t)(const s_zb_znp_mt_sys_osal_timer_expired_ind_t *msg);

typedef struct s_zb_znp_mt_sys_cb
{
    zb_znp_mt_sys_reset_ind_callback_t pfn_sys_reset_ind_cb;
    zb_znp_mt_sys_osal_timer_expired_ind_callback_t pfn_sys_osal_timer_expired_ind_cb;
} s_zb_znp_mt_sys_cb_t;

/**************************************************************************************************
 * FUNCTION PROTOTYPES
 *************************************************************************************************/
void zb_znp_mt_sys_register_callback(s_zb_znp_mt_sys_cb_t callback);
void zb_znp_mt_sys_unregister_callback(void);
void zb_znp_mt_sys_process(const uint8_t *rpc_buff, uint8_t rpc_len);
int zb_znp_mt_sys_reset_req(const s_zb_znp_mt_sys_reset_req_t *req_cmd);
int zb_znp_mt_sys_ping(s_zb_znp_mt_sys_ping_srsp_t *rsp);
int zb_znp_mt_sys_version(s_zb_znp_mt_sys_version_srsp_t *rsp);
int zb_znp_mt_sys_set_extaddr(const s_zb_znp_mt_sys_set_extaddr_cmd_t *req_cmd);
int zb_znp_mt_sys_get_extaddr(s_zb_znp_mt_sys_get_extaddr_srsp_t *rsp);
int zb_znp_mt_sys_ram_read(
    const s_zb_znp_mt_sys_ram_read_cmd_t *req_cmd,
    s_zb_znp_mt_sys_ram_read_srsp_t *rsp);
int zb_znp_mt_sys_ram_write(const s_zb_znp_mt_sys_ram_write_cmd_t *req_cmd);
int zb_znp_mt_sys_osal_nv_item_init(const s_zb_znp_mt_sys_osal_nv_item_init_cmd_t *req_cmd);
int zb_znp_mt_sys_osal_nv_read(
    const s_zb_znp_mt_sys_osal_nv_read_cmd_t *req_cmd,
    s_zb_znp_mt_sys_osal_nv_read_srsp_t *rsp);
int zb_znp_mt_sys_osal_nv_write(const s_zb_znp_mt_sys_osal_nv_write_cmd_t *req_cmd);
int zb_znp_mt_sys_osal_nv_delete(const s_zb_znp_mt_sys_osal_nv_delete_cmd_t *req_cmd);
int zb_znp_mt_sys_osal_nv_length(
    const s_zb_znp_mt_sys_osal_nv_length_cmd_t *req_cmd,
    s_zb_znp_mt_sys_osal_nv_length_srsp_t *rsp);
int zb_znp_mt_sys_osal_start_timer(const s_zb_znp_mt_sys_osal_start_timer_cmd_t *req_cmd);
int zb_znp_mt_sys_osal_stop_timer(const s_zb_znp_mt_sys_osal_stop_timer_cmd_t *req_cmd);
int zb_znp_mt_sys_random(s_zb_znp_mt_sys_random_srsp_t *rsp);
int zb_znp_mt_sys_adc_read(
    const s_zb_znp_mt_sys_adc_read_cmd_t *req_cmd,
    s_zb_znp_mt_sys_adc_read_srsp_t *rsp);
int zb_znp_mt_sys_gpio(
    const s_zb_znp_mt_sys_gpio_cmd_t *req_cmd,
    s_zb_znp_mt_sys_gpio_srsp_t *rsp);
int zb_znp_mt_sys_stack_tune(
    const s_zb_znp_mt_sys_stack_tune_cmd_t *req_cmd,
    s_zb_znp_mt_sys_stack_tune_srsp_t *rsp);
int zb_znp_mt_sys_set_time(const s_zb_znp_mt_sys_set_time_cmd_t *req_cmd);
int zb_znp_mt_sys_get_time(s_zb_znp_mt_sys_get_time_srsp_t *rsp);
int zb_znp_mt_sys_set_tx_power(const s_zb_znp_mt_sys_set_tx_power_cmd_t *req_cmd);
int zb_znp_mt_sys_get_tx_power(s_zb_znp_mt_sys_get_tx_power_srsp_t *rsp);
int zb_znp_mt_sys_get_temperature(int16_t *rsp);
int zb_znp_mt_sys_get_heap_statistics(uint32_t *free_heap, uint32_t *total_heap);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZNP_MT_SYS_H_ */

