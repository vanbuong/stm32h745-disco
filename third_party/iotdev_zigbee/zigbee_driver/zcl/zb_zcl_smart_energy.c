#include "common/zb_common.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"
#include "zcl/zb_zcl_smart_energy.h"

/*********************************************************************
 * CONSTANTS
 */
#define TAG "ZCL_SE"

// ZCL_CLUSTER_ID_SE_METERING:
#define ZCL_SE_METERING_SP_TOU_SET_LEN                 24
#define ZCL_SE_METERING_SP_TOU_SET_NO_BILL_LEN         7
#define ZCL_SE_METERING_SP_BLOCK_TIER_SET_LEN          25
#define ZCL_SE_METERING_SP_BLOCK_TIER_SET_NO_BILL_LEN  8
#define ZCL_SE_METERING_GET_PROFILE_RSP_LEN            7
#define ZCL_SE_METERING_REQ_FAST_POLL_MODE_RSP_LEN     5
#define ZCL_SE_METERING_SCHEDULE_SNAPSHOT_RSP_LEN      4
#define ZCL_SE_METERING_TAKE_SNAPSHOT_RSP_LEN          5
#define ZCL_SE_METERING_PUBLISH_SNAPSHOT_LEN           16
#define ZCL_SE_METERING_GET_SAMPLED_DATA_RSP_LEN       11
#define ZCL_SE_METERING_CFG_MIRROR_LEN                 9
#define ZCL_SE_METERING_CFG_NOTIF_SCHEME_LEN           9
#define ZCL_SE_METERING_CFG_NOTIF_FLAGS_LEN            12
#define ZCL_SE_METERING_GET_NOTIF_MSG_LEN              7
#define ZCL_SE_METERING_SUPPLY_STATUS_RSP_LEN          13
#define ZCL_SE_METERING_START_SAMPLING_RSP_LEN         2
#define ZCL_SE_METERING_GET_PROFILE_LEN                6
#define ZCL_SE_METERING_REQ_MIRROR_RSP_LEN             2
#define ZCL_SE_METERING_MIRROR_REMOVED_LEN             2
#define ZCL_SE_METERING_REQ_FAST_POLL_MODE_LEN         2
#define ZCL_SE_METERING_SCHEDULE_SNAPSHOT_LEN          6
#define ZCL_SE_METERING_SNAPSHOT_SCHEDULE_LEN          13
#define ZCL_SE_METERING_TAKE_SNAPSHOT_LEN              4
#define ZCL_SE_METERING_GET_SNAPSHOT_LEN               13
#define ZCL_SE_METERING_START_SAMPLING_LEN             13
#define ZCL_SE_METERING_GET_SAMPLED_DATA_LEN           9
#define ZCL_SE_METERING_MIRROR_REPORT_ATTR_RSP_LEN     1
#define ZCL_SE_METERING_RESET_LOAD_LIMIT_CNTR_LEN      8
#define ZCL_SE_METERING_CHANGE_SUPPLY_LEN              18
#define ZCL_SE_METERING_LOCAL_CHANGE_SUPPLY_LEN        1
#define ZCL_SE_METERING_SET_SUPPLY_STATUS_LEN          8
#define ZCL_SE_METERING_SET_UNCTRLD_FLOW_THRESHOLD_LEN 18

// /*********************************************************************
//  * TYPEDEFS
//  */
// typedef struct s_zb_zcl_se_callback_rec_type
// {
//     struct s_zb_zcl_se_callback_rec_type *next;
//     uint8_t endpoint;
//     s_zb_zcl_se_app_callbacks_t *cb;
// } s_zb_zcl_se_callback_rec_t;

// /*********************************************************************
//  * FUNCTION PROTOTYPES
//  */

//  /*********************************************************************
//  * LOCAL VARIABLES
//  */
// static s_zb_zcl_se_callback_rec_t *zcl_se_callbacks = NULL;
// static uint8_t zcl_se_plugin_registered = false;

// /**
//  * @fn     zb_zcl_se_find_callbacks
//  * 
//  * @brief  Find the callbacks for an application endpoint
//  * 
//  * @param  endpoint - the endpoint to find the callbacks for
//  * 
//  * @return pointer to the callbacks, or NULL if not found
//  */
// static s_zb_zcl_se_app_callbacks_t *
// zb_zcl_se_find_callbacks(uint8_t endpoint)
// {
//     s_zb_zcl_se_callback_rec_t *p_loop = zcl_se_callbacks;
//     while (p_loop != NULL)
//     {
//         if (p_loop->endpoint == endpoint)
//         {
//             return p_loop->cb;
//         }
//         p_loop = p_loop->next;
//     }
//     return NULL;
// }

// /**
//  * @fn      zb_zcl_se_handle_incoming
//  * 
//  * @brief   Callback from ZCL to process incoming commands specific to this cluster library
//  *          or Profile commands for attributes that aren't in the attribute list
//  * 
//  * @param   msg - pointer to the incoming message
//  * 
//  * @return  zb_status_t - ZB_SUCCESS if the command was processed successfully, otherwise an error code
//  */
// static zb_status_t
// zb_zcl_se_handle_incoming(s_zb_zcl_incoming_msg_t *msg)
// {
//     zb_status_t status = ZB_SUCCESS;

//     if (ZCL_CLUSTER_CMD(msg->hdr.fc.type))
//     {
//         // Is this a manufacturer specific command?
//         if (msg->hdr.fc.manu_specific == 0)
//         {
//             // Handle commands specific to this cluster library
//             status = zb_zcl_se_handle_in_specific_commands(msg);
//         }
//         else
//         {
//             // We don't support any manufacturer specific commands
//             status = ZB_FAILURE;
//         }
//     }
//     else
//     {
//         // Handle all the normal commands (Read, Write, etc.) -- should never get here
//         status = ZB_FAILURE;
//     }
//     return status;
// }

// /**
//  * @fn      zb_zcl_se_handle_specific_client_commands
//  * 
//  * @brief   Handle incoming Server-to-Client commands specific to this cluster library
//  * 
//  * @param   msg - pointer to the incoming message
//  * @param   cb - pointer to the application callbacks
//  * 
//  * @return  zb_status_t - ZB_SUCCESS if the command was processed successfully, otherwise an error code
//  */
// static zb_status_t
// zb_zcl_se_handle_specific_client_commands(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_se_app_callbacks_t *cb)
// {
//     zb_status_t status;
//     s_zb_zcl_se_app_callbacks_t *cb = zb_zcl_se_find_callbacks(msg->msg->dst_endpoint);

//     switch (msg->msg->cluster_id)
//     {
//         case ZCL_CLUSTER_ID_SE_METERING:
//             status = zcl_se_metering_hand_client_commands(msg, cb);
//             break;
//         default:
//             status = ZB_FAILURE;
//             break;
//     }

//     return status;
// }