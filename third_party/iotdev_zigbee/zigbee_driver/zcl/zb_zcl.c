/*
 * zb_zcl.c
 * 
 * Created on: 26 May 2025
 *     Author: Vo Van Buong (BRT-SG)
 */

#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_manu.h"
#include "zcl/zb_zcl_general.h"
#include "af/zb_af.h"
#include "common/zb_common.h"

/**************************************************************************************************
 * Macros
 **************************************************************************************************/

#define TAG "ZCL"

// Frame Control
#define ZCL_FC_TYPE(a)                  ((a) & ZCL_FRAME_CONTROL_TYPE)
#define ZCL_FC_MANU_SPECIFIC(a)         ((a) & ZCL_FRAME_CONTROL_MANU_SPECIFIC)
#define ZCL_FC_DIRECTION(a)             ((a) & ZCL_FRAME_CONTROL_DIRECTION)
#define ZCL_FC_DISABLE_DEFAULT_RSP(a)   ((a) & ZCL_FRAME_CONTROL_DISABLE_DEFAULT_RSP)

// Attribute Access Control
#define ZCL_ACCESS_CTRL_READ(a)         ((a) & ACCESS_CONTROL_READ)
#define ZCL_ACCESS_CTRL_WRITE(a)        ((a) & ACCESS_CONTROL_WRITE)
#define ZCL_ACCESS_CTRL_CMD(a)          ((a) & ACCESS_CONTROL_COMMAND)
#define ZCL_ACCESS_CTRL_AUTH_READ(a)    ((a) & ACCESS_CONTROL_AUTH_READ)
#define ZCL_ACCESS_CTRL_AUTH_WRITE(a)   ((a) & ACCESS_CONTROL_AUTH_WRITE)

#define ZCL_PARSE_CMD(a, b)             g_zcl_cmd_table[(a)].pfn_parse_in_profile((b))
#define ZCL_PROCESS_CMD(a, b)           g_zcl_cmd_table[(a)].pfn_process_in_profile((b))

#define ZCL_DEFAULT_RSP_CMD(zcl_hdr)    (ZCL_PROFILE_CMD((zcl_hdr).fc.type) && \
                                         (zcl_hdr).fc.manu_specific == 0    && \
                                         (zcl_hdr).command_id == ZCL_CMD_DEFAULT_RSP)

// Commands that have corresponding responses
#define ZCL_CMD_HAS_RSP(cmd)            ((cmd) == ZCL_CMD_READ                      || \
                                         (cmd) == ZCL_CMD_WRITE                     || \
                                         (cmd) == ZCL_CMD_WRITE_UNDIVIDED           || \
                                         (cmd) == ZCL_CMD_CONFIG_REPORT             || \
                                         (cmd) == ZCL_CMD_READ_REPORT_CFG           || \
                                         (cmd) == ZCL_CMD_DISCOVER_ATTRS            || \
                                         (cmd) == ZCL_CMD_DISCOVER_CMDS_RECEIVED    || \
                                         (cmd) == ZCL_CMD_DISCOVER_CMDS_GEN         || \
                                         (cmd) == ZCL_CMD_DISCOVER_ATTRS_EXT        || \
                                         (cmd) == ZCL_CMD_DEFAULT_RSP)

// GreenPower
#define GP_CLUSTER_ID                   0x0021
#define ZCD_NV_PROXY_TABLE_START        0x0310
#define GP_PROXY_TABLE                  0x0011

/**************************************************************************************************
 * Type Definitions
 **************************************************************************************************/

typedef struct s_zb_zcl_lib_plugin
{
    struct s_zb_zcl_lib_plugin *next;
    uint16_t start_cluster_id;
    uint16_t end_cluster_id;
    pfn_zcl_incoming_msg_handler_t pfn_incoming_msg_handler;
} s_zb_zcl_lib_plugin_t;

// Command record list
typedef struct s_zb_zcl_cmd_rec_list
{
    struct s_zb_zcl_cmd_rec_list *next;
    uint8_t endpoint;
    uint8_t num_cmds;
    const s_zb_zcl_command_rec_t *cmds;
} s_zb_zcl_cmd_rec_list_t;

// Attribute record list item
typedef struct s_zb_zcl_attr_rec_list
{
    struct s_zb_zcl_attr_rec_list *next;
    uint8_t endpoint;
    pfn_zcl_read_write_callback_t pfn_read_write_cb;
    pfn_zcl_authorize_callback_t pfn_authorize_cb;
    uint8_t num_attrs;
    const s_zb_zcl_attr_rec_t *attrs;
} s_zb_zcl_attr_rec_list_t;

// Cluster option record list item
typedef struct s_zb_zcl_cluster_option_list
{
    struct s_zb_zcl_cluster_option_list *next;
    uint8_t endpoint;
    uint8_t num_options;
    const s_zb_zcl_option_rec_t *options;
} s_zb_zcl_cluster_option_list_t;

typedef void *(*pfn_zcl_parse_in_profile_cmd_t)(s_zb_zcl_parse_cmd_t *cmd);
typedef uint8_t (*pfn_zcl_process_in_profile_cmd_t)(s_zb_zcl_incoming_msg_t *msg);

typedef struct
{
    pfn_zcl_parse_in_profile_cmd_t pfn_parse_in_profile;
    pfn_zcl_process_in_profile_cmd_t pfn_process_in_profile;
} s_zb_zcl_cmd_items_t;

typedef struct s_zb_zcl_handle_external_list
{
    void *next;
    uint8_t endpoint;
    pfn_zcl_unhandled_cmd_handler_t pfn_handler;
} s_zb_zcl_handle_external_list_t;

/**************************************************************************************************
 * Local Variables
 **************************************************************************************************/
/* Outgoing ZCL Transaction Sequence Number allocator.  This is the number
 * carried in the ZCL frame header so a remote device can match its response
 * (Read Rsp, Default Rsp, …) to our request.  It is end-to-end (host ↔ remote
 * device) and deliberately independent of the AF/APS transaction id used by
 * zb_af_data_req() to correlate the local AF_DATA_CONFIRM. */
static uint8_t g_zcl_trans_id = 1;
static uint8_t g_saved_zcl_trans_id;
static pfn_zcl_manu_spec_profile_wide_cmd_callback_t g_pfn_mspw_cmd_cb;
static s_zb_zcl_lib_plugin_t *g_zcl_plugins;
static s_zb_zcl_cmd_rec_list_t *g_zcl_cmd_list;
static s_zb_zcl_attr_rec_list_t *g_zcl_attr_list;
static s_zb_zcl_cluster_option_list_t *g_zcl_cluster_option_list;
static s_zb_af_incoming_msg_t *g_zcl_raw_af_msg;
static s_zb_zcl_handle_external_list_t *g_zcl_handle_external_list;

/**************************************************************************************************
 * Local Functions
 **************************************************************************************************/
static uint8_t *zcl_build_header(s_zb_zcl_frame_header_t *header, uint8_t *data);
static uint8_t zcl_calc_header_size(s_zb_zcl_frame_header_t *header);
static s_zb_zcl_lib_plugin_t *zcl_find_zcl_plugin(uint16_t cluster_id);
static uint8_t *zcl_serialize_attr_data(uint8_t data_type, void *data, uint8_t *buf);

static s_zb_zcl_attr_rec_list_t *zcl_find_attr_recs_list(uint8_t endpoint);
static s_zb_zcl_option_rec_t *zcl_find_cluster_option(uint8_t endpoint, uint16_t cluster_id);
static void zcl_set_security_option(uint8_t endpoint, uint16_t cluster_id, uint8_t enable);
static uint8_t zcl_device_operational(uint8_t src_ep, uint16_t cluster_id,  uint8_t frame_type, uint8_t cmd);

// Function for getting the read/write callback
static pfn_zcl_read_write_callback_t zcl_get_read_write_callback(uint8_t endpoint);
static pfn_zcl_authorize_callback_t zcl_get_authorize_callback(uint8_t endpoint);

// Function for ZCL read command
static zb_status_t zcl_read_attr_data(uint8_t *attr_data, s_zb_zcl_attr_rec_t *attr_rec, uint16_t *data_len);
static uint16_t zcl_get_attr_data_len_using_cb(uint8_t endpoint, uint16_t cluster_id, uint16_t attr_id);
static zb_status_t zcl_read_attr_data_using_cb(uint8_t endpoint, uint16_t cluster_id, uint16_t attr_id, uint8_t *attr_data, uint16_t *data_len);
static zb_status_t zcl_authorize_read(uint8_t endpoint, s_zb_af_address_t *src_addr, s_zb_zcl_attr_rec_t *attr_rec);
static void *zcl_parse_in_read_rsp_cmd(s_zb_zcl_parse_cmd_t *cmd);
static void *zcl_parse_in_read_cmd(s_zb_zcl_parse_cmd_t *cmd);
static uint8_t zcl_process_in_read_cmd(s_zb_zcl_incoming_msg_t *msg);

// Function for ZCL write command
static void *zcl_parse_in_write_rsp_cmd(s_zb_zcl_parse_cmd_t *cmd);

// Function for ZCL report command
static void *zcl_parse_in_config_report_rsp_cmd(s_zb_zcl_parse_cmd_t *cmd);
static void *zcl_parse_in_read_report_cfg_rsp_cmd(s_zb_zcl_parse_cmd_t *cmd);
static void *zcl_parse_in_report_cmd(s_zb_zcl_parse_cmd_t *cmd);

// Function for ZCL default response command
static void *zcl_parse_in_default_rsp_cmd(s_zb_zcl_parse_cmd_t *cmd);

// Callback function to handle messages externally
static uint8_t zcl_handle_external_cmd(s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "ZCL handle external cmd %02x from ep %d", msg->hdr.command_id, msg->msg->dst_endpoint);
    s_zb_zcl_handle_external_list_t *find = g_zcl_handle_external_list;
    while (find)
    {
        if ((find->endpoint == msg->msg->dst_endpoint) && (find->pfn_handler))
        {
            return find->pfn_handler(msg);
        }
        find = find->next;
    }
    return true;
}

/**************************************************************************************************
 * Parse Profile Command Function Table
 **************************************************************************************************/
static const s_zb_zcl_cmd_items_t g_zcl_cmd_table[] = {
    /* ZCL_CMD_READ */                          { zcl_parse_in_read_cmd,                zcl_process_in_read_cmd                },
    /* ZCL_CMD_READ_RSP */                      { zcl_parse_in_read_rsp_cmd,            zcl_handle_external_cmd                },
    /* ZCL_CMD_WRITE */                         { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* ZCL_CMD_WRITE_UNDIVIDED */               { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* ZCL_CMD_WRITE_RSP */                     { zcl_parse_in_write_rsp_cmd,           zcl_handle_external_cmd                },
    /* ZCL_CMD_WRITE_NO_RSP */                  { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* ZCL_CMD_CONFIG_REPORT */                 { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* ZCL_CMD_CONFIG_REPORT_RSP */             { zcl_parse_in_config_report_rsp_cmd,   zcl_handle_external_cmd                },
    /* ZCL_CMD_READ_REPORT_CFG */               { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* ZCL_CMD_READ_REPORT_CFG_RSP */           { zcl_parse_in_read_report_cfg_rsp_cmd, zcl_handle_external_cmd                },
    /* ZCL_CMD_REPORT */                        { zcl_parse_in_report_cmd,              zcl_handle_external_cmd                },
    /* ZCL_CMD_DEFAULT_RSP */                   { zcl_parse_in_default_rsp_cmd,         zcl_handle_external_cmd                },
    /* ZCL_CMD_DISCOVER_ATTRS */                { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* ZCL_CMD_DISCOVER_ATTRS_RSP */            { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* *not supported* READ_ATTRS_STRCT */      { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* *not supported* WRITE_ATTRS_STRCT */     { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* *not supported* WRITE_ATTRS_STRCT_RSP */ { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* ZCL_CMD_DISCOVER_CMDS_RECEIVED */        { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* ZCL_CMD_DISCOVER_CMDS_RECEIVED_RSP */    { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* ZCL_CMD_DISCOVER_CMDS_GEN */             { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* ZCL_CMD_DISCOVER_CMDS_GEN_RSP */         { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* ZCL_CMD_DISCOVER_ATTRS_EXT */            { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
    /* ZCL_CMD_DISCOVER_ATTRS_EXT_RSP */        { (pfn_zcl_parse_in_profile_cmd_t)NULL, (pfn_zcl_process_in_profile_cmd_t)NULL },
};

/**************************************************************************************************
 * Public Functions
 **************************************************************************************************/
/**
 * @brief Get the length of the data type
 * 
 * @param[in] data_type Data type
 * @return uint8_t Length of the data type
 */
uint8_t
zb_zcl_get_data_type_length(uint8_t data_type)
{
    uint8_t length = 0;

    switch (data_type)
    {
        case ZCL_DATATYPE_DATA8:
        case ZCL_DATATYPE_BOOLEAN:
        case ZCL_DATATYPE_BITMAP8:
        case ZCL_DATATYPE_INT8:
        case ZCL_DATATYPE_UINT8:
        case ZCL_DATATYPE_ENUM8:
            length = 1;
            break;
        case ZCL_DATATYPE_DATA16:
        case ZCL_DATATYPE_BITMAP16:
        case ZCL_DATATYPE_INT16:
        case ZCL_DATATYPE_UINT16:
        case ZCL_DATATYPE_ENUM16:
        case ZCL_DATATYPE_SEMI_PREC:
        case ZCL_DATATYPE_CLUSTER_ID:
        case ZCL_DATATYPE_ATTR_ID:
            length = 2;
            break;
        case ZCL_DATATYPE_DATA24:
        case ZCL_DATATYPE_BITMAP24:
        case ZCL_DATATYPE_INT24:
        case ZCL_DATATYPE_UINT24:
            length = 3;
            break;
        case ZCL_DATATYPE_DATA32:
        case ZCL_DATATYPE_BITMAP32:
        case ZCL_DATATYPE_INT32:
        case ZCL_DATATYPE_UINT32:
        case ZCL_DATATYPE_SINGLE_PREC:
        case ZCL_DATATYPE_TOD:
        case ZCL_DATATYPE_DATE:
        case ZCL_DATATYPE_UTC:
        case ZCL_DATATYPE_BAC_OID:
            length = 4;
            break;
        case ZCL_DATATYPE_DATA40:
        case ZCL_DATATYPE_BITMAP40:
        case ZCL_DATATYPE_INT40:
        case ZCL_DATATYPE_UINT40:
            length = 5;
            break;
        case ZCL_DATATYPE_DATA48:
        case ZCL_DATATYPE_BITMAP48:
        case ZCL_DATATYPE_INT48:
        case ZCL_DATATYPE_UINT48:
            length = 6;
            break;
        case ZCL_DATATYPE_DATA56:
        case ZCL_DATATYPE_BITMAP56:
        case ZCL_DATATYPE_INT56:
        case ZCL_DATATYPE_UINT56:
            length = 7;
            break;
        case ZCL_DATATYPE_DATA64:
        case ZCL_DATATYPE_BITMAP64:
        case ZCL_DATATYPE_INT64:
        case ZCL_DATATYPE_UINT64:
        case ZCL_DATATYPE_DOUBLE_PREC:
        case ZCL_DATATYPE_IEEE_ADDR:
            length = 8;
            break;
        case ZCL_DATATYPE_128_BIT_SEC_KEY:
            length = 16;
            break;
        case ZCL_DATATYPE_NO_DATA:
        case ZCL_DATATYPE_UNKNOWN:
            // Fall through
        default:
            length = 0;
            break;
    }

    return length;
}

/**
 * @brief Get the length of the attribute data
 * 
 * @param[in] data_type Data type
 * @param[in] data Pointer to the data
 * @return uint16_t Length of the attribute data
 */
uint16_t
zb_zcl_get_attr_data_len(uint8_t data_type, uint8_t *data)
{
    uint16_t data_len = 0;

    if (data_type == ZCL_DATATYPE_LONG_CHAR_STR || data_type == ZCL_DATATYPE_LONG_OCTET_STR)
    {
        data_len = BUILD_UINT16(data[0], data[1]) + 2; // Long string length + 2 bytes for length field
    }
    else if (data_type == ZCL_DATATYPE_CHAR_STR || data_type == ZCL_DATATYPE_OCTET_STR)
    {
        data_len = *data + 1; // String length + 1 byte for length field
    }
    else
    {
        data_len = zb_zcl_get_data_type_length(data_type); // Single byte data
    }

    return data_len;
}

/**
 * @brief Bounds-checked variant of zb_zcl_get_attr_data_len().
 *
 * zb_zcl_get_attr_data_len() dereferences @p data to read the length prefix of
 * string types, and returns 0 for any data type it has no rule for. Both are
 * fatal when walking an attribute-record list: the first reads past the end of
 * the AF payload, the second makes the walk advance by only the record header
 * so every following record is decoded from garbage. Manufacturer-specific
 * frames (Lumi/Aqara STRUCT and ARRAY types, truncated vendor blobs) hit both.
 *
 * @param data_type ZCL data type byte.
 * @param data      First byte of the attribute value.
 * @param end       One past the last valid byte of the payload.
 * @param out_len   Receives the value length on success.
 * @return true when the value is a known type and fits entirely before @p end.
 */
static bool
zcl_safe_attr_data_len(uint8_t data_type, const uint8_t *data, const uint8_t *end, uint16_t *out_len)
{
    uint16_t len;

    if (data > end)
    {
        return false;
    }

    if ((data_type == ZCL_DATATYPE_LONG_CHAR_STR) || (data_type == ZCL_DATATYPE_LONG_OCTET_STR))
    {
        if ((end - data) < 2)
        {
            return false; // no room for the 2-byte length prefix
        }
        len = (uint16_t)(BUILD_UINT16(data[0], data[1]) + 2);
    }
    else if ((data_type == ZCL_DATATYPE_CHAR_STR) || (data_type == ZCL_DATATYPE_OCTET_STR))
    {
        if ((end - data) < 1)
        {
            return false; // no room for the 1-byte length prefix
        }
        len = (uint16_t)(*data + 1);
    }
    else
    {
        len = zb_zcl_get_data_type_length(data_type);
        if ((len == 0) &&
            (data_type != ZCL_DATATYPE_NO_DATA) && (data_type != ZCL_DATATYPE_UNKNOWN))
        {
            // Unsupported type (STRUCT/ARRAY/SET/BAG...): length is not derivable,
            // so the rest of the list cannot be walked. Reject the whole frame.
            return false;
        }
    }

    if (len > (uint16_t)(end - data))
    {
        return false;
    }

    *out_len = len;
    return true;
}


/**
 * @brief Calculate the size of the ZCL frame header
 * 
 * @param[in] frame_header Pointer to the frame header structure
 * @return uint8_t Size of the ZCL frame header
 */
static uint8_t
zcl_calc_header_size(s_zb_zcl_frame_header_t *frame_header)
{
    uint8_t size = ZCL_FRAME_HEADER_FRAME_CTRL_SIZE
                    + ZCL_FRAME_HEADER_TRANS_SEQ_NUM_SIZE
                    + ZCL_FRAME_HEADER_COMMAND_ID_SIZE;

    if (frame_header->fc.manu_specific)
    {
        size += ZCL_FRAME_HEADER_MANUF_CODE_SIZE;
    }

    return size;
}

/**
 * @brief Build the ZCL frame header
 * 
 * @param[in] frame_header Pointer to the frame header structure
 * @param[in] msg_buff Pointer to the message buffer
 * @return uint8_t* Pointer to the message buffer
 */
static uint8_t*
zcl_build_header(s_zb_zcl_frame_header_t *frame_header, uint8_t *msg_buff)
{
    *msg_buff++ = frame_header->fc.byte;

    if (frame_header->fc.manu_specific)
    {
        *msg_buff++ = LO_UINT16(frame_header->manuf_code);
        *msg_buff++ = HI_UINT16(frame_header->manuf_code);
    }

    *msg_buff++ = frame_header->trans_seq_num;
    *msg_buff++ = frame_header->command_id;

    return msg_buff;
}

static s_zb_zcl_lib_plugin_t*
zcl_find_zcl_plugin(uint16_t cluster_id)
{
    s_zb_zcl_lib_plugin_t *p_plugin = g_zcl_plugins;
    while (p_plugin)
    {
        if ((cluster_id >= p_plugin->start_cluster_id) && (cluster_id <= p_plugin->end_cluster_id))
        {
            return p_plugin;
        }
        p_plugin = p_plugin->next;
    }
    return NULL;
}

static s_zb_zcl_attr_rec_list_t*
zcl_find_attr_recs_list(uint8_t endpoint)
{
    s_zb_zcl_attr_rec_list_t *p_attr = g_zcl_attr_list;
    while (p_attr)
    {
        if (p_attr->endpoint == endpoint)
        {
            return p_attr;
        }
        p_attr = p_attr->next;
    }
    return NULL;
}

static s_zb_zcl_option_rec_t*
zcl_find_cluster_option(uint8_t endpoint, uint16_t cluster_id)
{
    s_zb_zcl_cluster_option_list_t *p_option = g_zcl_cluster_option_list;
    while (p_option)
    {
        if (p_option->endpoint == endpoint)
        {
            for (uint8_t i = 0; i < p_option->num_options; i++)
            {
                if (p_option->options[i].cluster_id == cluster_id)
                {
                    return (s_zb_zcl_option_rec_t*)&p_option->options[i];
                }
            }
        }
        p_option = p_option->next;
    }
    return NULL;
}

static void
zcl_set_security_option(uint8_t endpoint, uint16_t cluster_id, uint8_t enable)
{
    s_zb_zcl_option_rec_t *p_option = zcl_find_cluster_option(endpoint, cluster_id);
    if (p_option != NULL)
    {
        if (enable)
        {
            p_option->option |= AF_EN_SECURITY;
        }
        else
        {
            p_option->option &= (AF_EN_SECURITY ^ 0xFF);
        }
    }
}

static uint8_t
zcl_device_operational(uint8_t src_ep, uint16_t cluster_id,  uint8_t frame_type, uint8_t cmd)
{
    s_zb_zcl_attr_rec_t attr_rec;
    uint8_t device_enabled = DEVICE_ENABLED;

    // If the device is Disabled (DeviceEnabled attribute is set to Disabled), it
    // cannot send or respond to application level commands, other than commands
    // to read or write attributes. Note that the Identify cluster cannot be disabled,
    // and remains functional regardless of this setting.
    if (ZCL_PROFILE_CMD(frame_type) && cmd <= ZCL_CMD_WRITE_NO_RSP)
    {
        return true;
    }
    if (cluster_id == ZCL_CLUSTER_ID_GENERAL_IDENTIFY)
    {
        return true;
    }
    if (zb_zcl_find_attr_rec(src_ep, ZCL_CLUSTER_ID_GENERAL_BASIC, ATTRID_BASIC_DEVICE_ENABLED, &attr_rec))
    {
        zcl_read_attr_data(&device_enabled, &attr_rec, NULL);
    }
    return (device_enabled == DEVICE_ENABLED) ? true : false;
}

static pfn_zcl_read_write_callback_t
zcl_get_read_write_callback(uint8_t endpoint)
{
    s_zb_zcl_attr_rec_list_t *p_attr = zcl_find_attr_recs_list(endpoint);
    if (p_attr != NULL)
    {
        return p_attr->pfn_read_write_cb;
    }
    return NULL;
}

static pfn_zcl_authorize_callback_t
zcl_get_authorize_callback(uint8_t endpoint)
{
    s_zb_zcl_attr_rec_list_t *p_attr = zcl_find_attr_recs_list(endpoint);
    if (p_attr != NULL)
    {
        return p_attr->pfn_authorize_cb;
    }
    return NULL;
}

static zb_status_t
zcl_read_attr_data(uint8_t *attr_data, s_zb_zcl_attr_rec_t *attr_rec, uint16_t *data_len)
{
    uint16_t len;

    if (attr_rec->attr.attr_data == NULL)
    {
        return ZCL_STATUS_FAILURE;
    }

    len = zb_zcl_get_attr_data_len(attr_rec->attr.data_type, attr_rec->attr.attr_data);
    memcpy(attr_data, attr_rec->attr.attr_data, len);

    if (data_len)
    {
        *data_len = len;
    }

    return ZCL_STATUS_SUCCESS;
}

static uint16_t
zcl_get_attr_data_len_using_cb(uint8_t endpoint, uint16_t cluster_id, uint16_t attr_id)
{
    uint16_t data_len = 0;
    pfn_zcl_read_write_callback_t pfn_read_write_cb = zcl_get_read_write_callback(endpoint);

    if (pfn_read_write_cb != NULL)
    {
        (*pfn_read_write_cb)(cluster_id, attr_id, ZCL_OPER_LEN, NULL, &data_len);
    }
    return data_len;
}

static zb_status_t
zcl_read_attr_data_using_cb(uint8_t endpoint, uint16_t cluster_id, uint16_t attr_id, uint8_t *attr_data, uint16_t *data_len)
{
    pfn_zcl_read_write_callback_t pfn_read_write_cb = zcl_get_read_write_callback(endpoint);

    if (data_len != NULL)
    {
        *data_len = 0;
    }

    if (pfn_read_write_cb != NULL)
    {
        return (*pfn_read_write_cb)(cluster_id, attr_id, ZCL_OPER_READ, attr_data, data_len);
    }
    return ZCL_STATUS_SOFTWARE_FAILURE;
}

static zb_status_t
zcl_authorize_read(uint8_t endpoint, s_zb_af_address_t *src_addr, s_zb_zcl_attr_rec_t *attr_rec)
{
    if (ZCL_ACCESS_CTRL_READ(attr_rec->attr.access_control))
    {
        pfn_zcl_authorize_callback_t pfn_authorize_cb = zcl_get_authorize_callback(endpoint);
        if (pfn_authorize_cb != NULL)
        {
            return (*pfn_authorize_cb)(src_addr, attr_rec, ZCL_OPER_READ);
        }
    }
    return ZCL_STATUS_SUCCESS;
}

static void*
zcl_parse_in_read_cmd(s_zb_zcl_parse_cmd_t *cmd)
{
    s_zb_zcl_read_attr_cmd_t *read_cmd;
    uint8_t *pbuf = cmd->data;

    read_cmd = (s_zb_zcl_read_attr_cmd_t *)ZB_MEM_MALLOC(sizeof(s_zb_zcl_read_attr_cmd_t) + cmd->data_len);
    if (read_cmd)
    {
        read_cmd->num_attr = cmd->data_len / 2; // Attribute ID
        for (uint8_t i = 0; i < read_cmd->num_attr; i++)
        {
            read_cmd->attr_id[i] = BUILD_UINT16(pbuf[0], pbuf[1]);
            pbuf += 2;
        }
    }
    return ((void *)read_cmd);
}

static void*
zcl_parse_in_read_rsp_cmd(s_zb_zcl_parse_cmd_t *cmd)
{
    s_zb_zcl_read_attr_rsp_cmd_t *read_rsp_cmd;
    uint8_t *pbuf = cmd->data;
    uint8_t *data_ptr;
    uint8_t num_attr = 0;
    uint8_t hdr_len;
    uint16_t attr_data_len;
    uint16_t data_len = 0;
    const uint8_t *end = cmd->data + cmd->data_len;

    // Find out the  number of attributes and the length of attribute data
    while (pbuf < end)
    {
        uint8_t status;

        // Every record needs at least attr_id(2) + status(1).
        if ((end - pbuf) < 3)
        {
            return NULL;
        }

        pbuf += 2; // move pass attribute id
        status = *pbuf++;

        if (status == ZCL_STATUS_SUCCESS)
        {
            uint8_t data_type;

            if ((end - pbuf) < 1)
            {
                return NULL; // SUCCESS record without a data type byte
            }
            data_type = *pbuf++;

            // calculated data len exceeded actual buffer length, or the data
            // type has no derivable length: invalid command
            if (!zcl_safe_attr_data_len(data_type, pbuf, end, &attr_data_len))
            {
                return NULL;
            }
            pbuf += attr_data_len; // move pass attribute data
            data_len += attr_data_len;
        }

        num_attr++;
    }

    if (num_attr == 0)
    {
        return NULL;
    }

    // calculate the length of the response header
    hdr_len = sizeof(s_zb_zcl_read_attr_rsp_cmd_t) + num_attr * sizeof(s_zb_zcl_read_attr_rsp_info_t);

    read_rsp_cmd = (s_zb_zcl_read_attr_rsp_cmd_t *)ZB_MEM_MALLOC(hdr_len + data_len);
    if (read_rsp_cmd)
    {
        pbuf = cmd->data;
        data_ptr = (uint8_t *)((uint8_t *)read_rsp_cmd + hdr_len);

        read_rsp_cmd->num_attr = num_attr;
        for (uint8_t i = 0; i < num_attr; i++)
        {
            s_zb_zcl_read_attr_rsp_info_t *rsp_info = &read_rsp_cmd->attr_list[i];

            rsp_info->attr_id = BUILD_UINT16(pbuf[0], pbuf[1]);
            pbuf += 2;

            rsp_info->status = *pbuf++;
            if (rsp_info->status == ZCL_STATUS_SUCCESS)
            {
                rsp_info->data_type = *pbuf++;

                // Validated by the sizing pass above; use the checked call so
                // both passes advance pbuf identically.
                attr_data_len = 0;
                (void)zcl_safe_attr_data_len(rsp_info->data_type, pbuf, end, &attr_data_len);
                memcpy(data_ptr, pbuf, attr_data_len);
                rsp_info->data = data_ptr;

                pbuf += attr_data_len;

                data_ptr += attr_data_len;
            }
        }
    }
    return ((void *)read_rsp_cmd);
}

static uint8_t
zcl_process_in_read_cmd(s_zb_zcl_incoming_msg_t *msg)
{
    s_zb_zcl_read_attr_cmd_t *read_cmd;
    s_zb_zcl_read_attr_rsp_cmd_t *read_rsp_cmd;
    s_zb_zcl_attr_rec_t attr_rec;
    uint16_t len;
    uint8_t attr_found;

    read_cmd = (s_zb_zcl_read_attr_cmd_t *)msg->attr_cmd;

    // Calculate the length of the response status record
    len = sizeof(s_zb_zcl_read_attr_rsp_cmd_t) + read_cmd->num_attr * sizeof(s_zb_zcl_read_attr_rsp_info_t);

    read_rsp_cmd = (s_zb_zcl_read_attr_rsp_cmd_t *)ZB_MEM_MALLOC(len);
    if (read_rsp_cmd == NULL)
    {
        return false;
    }

    read_rsp_cmd->num_attr = read_cmd->num_attr;
    for (uint8_t i = 0; i < read_cmd->num_attr; i++)
    {
        s_zb_zcl_read_attr_rsp_info_t *rsp_info = &read_rsp_cmd->attr_list[i];

        rsp_info->attr_id = read_cmd->attr_id[i];
        attr_found = zb_zcl_find_attr_rec(msg->msg->dst_endpoint, msg->msg->cluster_id,
                                                     read_cmd->attr_id[i], &attr_rec);

        // Validate the attribute is found and the access control
        if (attr_found &&
            ((attr_rec.attr.access_control & ACCESS_GLOBAL) ||
             ((attr_rec.attr.access_control & (1 << ACCESS_CONTROL_MASK)) == msg->hdr.fc.direction)))
        {
            if (ZCL_ACCESS_CTRL_READ(attr_rec.attr.access_control))
            {
                if (ZCL_ACCESS_CTRL_AUTH_READ(attr_rec.attr.access_control))
                {
                    rsp_info->status = zcl_authorize_read(msg->msg->dst_endpoint, &msg->msg->src_addr, &attr_rec);
                }
                else
                {
                    rsp_info->status = zcl_read_attr_data_using_cb(
                                                msg->msg->dst_endpoint, attr_rec.cluster_id,
                                                attr_rec.attr.attr_id, attr_rec.attr.attr_data, &len);
                }

                if (rsp_info->status == ZCL_STATUS_SUCCESS)
                {
                    rsp_info->data = attr_rec.attr.attr_data;
                    rsp_info->data_type = attr_rec.attr.data_type;
                }
            }
            else
            {
                rsp_info->status = ZCL_STATUS_WRITE_ONLY;
            }
        }
        else
        {
            rsp_info->status = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
        }
    }

    // Build and send Read Response command
    zb_zcl_send_read_rsp(
        msg->msg->dst_endpoint, &msg->msg->src_addr, msg->msg->cluster_id, read_rsp_cmd,
        !msg->hdr.fc.direction, true, msg->hdr.trans_seq_num);
    ZB_MEM_FREE(read_rsp_cmd);

    return true;
}

// Function for ZCL write command
static void*
zcl_parse_in_write_rsp_cmd(s_zb_zcl_parse_cmd_t *cmd)
{
    s_zb_zcl_write_attr_rsp_cmd_t *write_rsp_cmd;
    uint8_t *pbuf = cmd->data;
    uint8_t i = 0;

    // Validate tthat the incoming payload is a valid size. if data_len == 1, writes were successful
    // otherwise, data_len should be a multiple of sizeof(s_zb_zcl_write_attr_rsp_info_t)
    if ((cmd->data_len > 1) && (cmd->data_len % sizeof(s_zb_zcl_write_attr_rsp_info_t) != 0))
    {
        return (void *)NULL;
    }

    write_rsp_cmd = (s_zb_zcl_write_attr_rsp_cmd_t *)
                        ZB_MEM_MALLOC(sizeof(s_zb_zcl_write_attr_rsp_cmd_t) + cmd->data_len);
    if (write_rsp_cmd)
    {
        if (cmd->data_len == 1)
        {
            // Special case when all writes were successful
            write_rsp_cmd->attr_list[i++].status = *pbuf;
        }
        else
        {
            while (pbuf < (cmd->data + cmd->data_len))
            {
                write_rsp_cmd->attr_list[i].status = *pbuf++;
                write_rsp_cmd->attr_list[i++].attr_id = BUILD_UINT16(pbuf[0], pbuf[1]);
                pbuf += 2;
            }
        }

        write_rsp_cmd->num_attr = i;
    }

    return ((void *)write_rsp_cmd);
}

// Function for ZCL report command
static void*
zcl_parse_in_config_report_rsp_cmd(s_zb_zcl_parse_cmd_t *cmd)
{
    s_zb_zcl_config_report_rsp_cmd_t *cfg_report_rsp_cmd;
    uint8_t *pbuf = cmd->data;
    uint8_t num_attr;

    if ((cmd->data_len > 1) && (cmd->data_len % sizeof(s_zb_zcl_config_report_rsp_info_t) != 0))
    {
        return (void *)NULL;
    }
    else if (cmd->data_len == 1)
    {
        num_attr = 1;
    }
    else
    {
        num_attr = cmd->data_len / sizeof(s_zb_zcl_config_report_rsp_info_t);
    }

    cfg_report_rsp_cmd = (s_zb_zcl_config_report_rsp_cmd_t *)ZB_MEM_MALLOC(
        sizeof(s_zb_zcl_config_report_rsp_cmd_t) +
        (num_attr * sizeof(s_zb_zcl_config_report_rsp_info_t)));

    if (cfg_report_rsp_cmd)
    {
        if (cmd->data_len == 1)
        {
            cfg_report_rsp_cmd->attr_list[0].status = *pbuf;
        }
        else
        {
            cfg_report_rsp_cmd->num_attr = num_attr;
            for (uint8_t i = 0; i < cfg_report_rsp_cmd->num_attr; i++)
            {
                cfg_report_rsp_cmd->attr_list[i].status = *pbuf++;
                cfg_report_rsp_cmd->attr_list[i].direction = *pbuf++;
                cfg_report_rsp_cmd->attr_list[i].attr_id = BUILD_UINT16(pbuf[0], pbuf[1]);
                pbuf += 2;
            }
        }
    }

    return ((void *)cfg_report_rsp_cmd);

}

static void
zcl_build_analog_data(uint8_t data_type, uint8_t *attr_data, uint8_t *buf)
{
    int current_byte_index = 0;
    int remaining_bytes;

    remaining_bytes = zb_zcl_get_attr_data_len(data_type, attr_data);

    while (remaining_bytes--)
    {
        attr_data[current_byte_index] = *buf++;
        current_byte_index++;
    }
}

static void*
zcl_parse_in_read_report_cfg_rsp_cmd(s_zb_zcl_parse_cmd_t *cmd)
{
    s_zb_zcl_read_report_cfg_rsp_cmd_t *read_report_cfg_rsp_cmd;
    uint8_t report_change_len;
    uint8_t *pbuf = cmd->data;
    uint8_t *data_ptr;
    uint8_t num_attr = 0;
    uint8_t hdr_len;
    uint16_t data_len = 0;
    uint16_t calculated_buf_len = 0;
    uint16_t actual_data_len = cmd->data_len;

    // Calculate the length of the response command
    while (pbuf < (cmd->data + cmd->data_len))
    {
        uint8_t status;
        uint8_t direction;

        num_attr++;
        status = *pbuf++;
        direction = *pbuf++;
        pbuf += 2; // move pass the attribute ID
        calculated_buf_len += 4; // status, direction, and attribute ID

        if (status == ZCL_STATUS_SUCCESS)
        {
            if (direction == ZCL_SEND_ATTR_REPORTS)
            {
                uint8_t data_type = *pbuf++;
                pbuf += 4; // move pass the Min and Max Reporting interval
                calculated_buf_len += 1 + 2 + 2; // data type, Min and Max Reporting interval

                // For attributes of 'discrete' data types this field is omitted
                if (zb_zcl_is_analog_data_type(data_type))
                {
                    report_change_len = zb_zcl_get_data_type_length(data_type);
                    if ((report_change_len + calculated_buf_len) > actual_data_len)
                    {
                        return (void *)NULL;
                    }
                    pbuf += report_change_len; // move pass attribute data
                    calculated_buf_len += report_change_len;
                    data_len += report_change_len;
                }
            }
            else
            {
                pbuf += 2; // move pass timeout field
                calculated_buf_len += 2; // timeout period
            }
        }
    } // end while loop

    hdr_len = sizeof(s_zb_zcl_read_report_cfg_rsp_cmd_t) +
              (num_attr * sizeof(s_zb_zcl_read_report_cfg_rsp_info_t));
    read_report_cfg_rsp_cmd = (s_zb_zcl_read_report_cfg_rsp_cmd_t *)ZB_MEM_MALLOC(hdr_len + data_len);
    if (read_report_cfg_rsp_cmd)
    {
        pbuf = cmd->data;
        data_ptr = (uint8_t *)((uint8_t *)read_report_cfg_rsp_cmd + hdr_len);

        read_report_cfg_rsp_cmd->num_attr = num_attr;
        for (uint8_t i = 0; i < num_attr; i++)
        {
            s_zb_zcl_read_report_cfg_rsp_info_t *report_rsp_info = &read_report_cfg_rsp_cmd->attr_list[i];

            report_rsp_info->status = *pbuf++;
            report_rsp_info->direction = *pbuf++;
            report_rsp_info->attr_id = BUILD_UINT16(pbuf[0], pbuf[1]);
            pbuf += 2;

            if (report_rsp_info->status == ZCL_STATUS_SUCCESS)
            {
                if (report_rsp_info->direction == ZCL_SEND_ATTR_REPORTS)
                {
                    report_rsp_info->data_type = *pbuf++;
                    report_rsp_info->min_report_int = BUILD_UINT16(pbuf[0], pbuf[1]);
                    pbuf += 2;
                    report_rsp_info->max_report_int = BUILD_UINT16(pbuf[0], pbuf[1]);
                    pbuf += 2;

                    if (zb_zcl_is_analog_data_type(report_rsp_info->data_type))
                    {
                        zcl_build_analog_data(report_rsp_info->data_type, data_ptr, pbuf);
                        report_rsp_info->reportable_change = data_ptr;

                        report_change_len = zb_zcl_get_data_type_length(report_rsp_info->data_type);
                        pbuf += report_change_len;
                        data_ptr += report_change_len;
                    }
                }
                else
                {
                    report_rsp_info->timeout_period = BUILD_UINT16(pbuf[0], pbuf[1]);
                    pbuf += 2;
                }
            }
        }
    }

    return ((void *)read_report_cfg_rsp_cmd);
}

static void *
zcl_parse_in_report_cmd(s_zb_zcl_parse_cmd_t *cmd)
{
    s_zb_zcl_report_attr_cmd_t *report_cmd;
    uint8_t *pbuf = cmd->data;
    uint16_t attr_data_len;
    uint8_t *data_ptr;
    uint8_t num_attr = 0;
    uint8_t hdr_len;
    uint16_t data_len = 0;
    const uint8_t *end = cmd->data + cmd->data_len;

    while (pbuf < end)
    {
        uint8_t data_type;

        // Every record needs at least attr_id(2) + data_type(1).
        if ((end - pbuf) < 3)
        {
            return (void *)NULL;
        }

        pbuf += 2; // move pass attribute id
        data_type = *pbuf++;

        if (!zcl_safe_attr_data_len(data_type, pbuf, end, &attr_data_len))
        {
            return (void *)NULL;
        }

        num_attr++;
        pbuf += attr_data_len; // move pass attribute data
        data_len += attr_data_len;
    }

    if (num_attr == 0)
    {
        return (void *)NULL;
    }

    hdr_len = sizeof(s_zb_zcl_report_attr_cmd_t) + num_attr * sizeof(s_zb_zcl_report_attr_info_t);

    report_cmd = (s_zb_zcl_report_attr_cmd_t *)ZB_MEM_MALLOC(hdr_len + data_len);
    if (report_cmd)
    {
        pbuf = cmd->data;
        data_ptr = (uint8_t *)((uint8_t *)report_cmd + hdr_len);

        report_cmd->num_attr = num_attr;
        for (uint8_t i = 0; i < num_attr; i++)
        {
            s_zb_zcl_report_attr_info_t *attr_info = &report_cmd->attr_list[i];

            attr_info->attr_id = BUILD_UINT16(pbuf[0], pbuf[1]);
            pbuf += 2;
            attr_info->data_type = *pbuf++;

            // The sizing pass above already validated every record, so this
            // cannot fail; keep the checked call so both passes stay in step.
            attr_data_len = 0;
            (void)zcl_safe_attr_data_len(attr_info->data_type, pbuf, end, &attr_data_len);
            memcpy(data_ptr, pbuf, attr_data_len);
            attr_info->attr_data = data_ptr;

            pbuf += attr_data_len;
            data_ptr += attr_data_len;
        }
    }

    return ((void *)report_cmd);
}

// Function for ZCL default response command
static void*
zcl_parse_in_default_rsp_cmd(s_zb_zcl_parse_cmd_t *cmd)
{
    s_zb_zcl_default_rsp_cmd_t *default_rsp_cmd;
    uint8_t *pbuf = cmd->data;

    default_rsp_cmd = (s_zb_zcl_default_rsp_cmd_t *)
                        ZB_MEM_MALLOC(sizeof(s_zb_zcl_default_rsp_cmd_t));
    if (default_rsp_cmd)
    {
        default_rsp_cmd->command_id = *pbuf++;
        default_rsp_cmd->status_code = *pbuf;
    }

    return ((void *)default_rsp_cmd);
}

/**
 * @brief Send a ZCL command
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cluster_id Cluster ID
 * @param[in] command_id Command ID
 * @param[in] specific Whether the command is cluster specific
 * @param[in] direction Direction of the command
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] manuf_code Manufacturer code
 * @param[in] seq_num Sequence number
 * @param[in] cmd_len Length of the command data
 * @param[in] cmd_data Pointer to the command data
 * @return int Status of the command
 */
zb_status_t
zb_zcl_send_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, uint8_t command_id, uint8_t specific,
    uint8_t direction, uint8_t disable_default_rsp, uint16_t manuf_code,
    uint8_t seq_num, uint16_t cmd_len, uint8_t *cmd_data)
{
    s_zb_zcl_frame_header_t frame_header = {0};
    zb_status_t status;
    uint16_t msg_len = 0;
    uint8_t *msg_buff = NULL;
    uint8_t *pbuf = NULL;
    uint8_t options = 0;

    options = zb_zcl_get_cluster_option(src_ep, cluster_id);
    // The cluster might not have been defined to use security but if this message
    // is in response to another message that was using APS security, this message
    // will be sent with APS security
    if (!(options & AF_EN_SECURITY))
    {
        s_zb_af_incoming_msg_t *orig_msg = zb_zcl_get_raw_af_incoming_msg();

        if ((orig_msg && orig_msg->security_use))
        {
            options |= AF_EN_SECURITY;
        }
    }

    /* A ZCL Default Response is itself a reply to a received command. The target
     * is often a sleepy end device that is asleep by the time we answer and thus
     * cannot APS-ACK — requesting an ACK there only yields spurious delivery
     * failures and pointless retries. Never request APS ACK for a default
     * response (the cluster's registered options may otherwise set it). */
    if (!specific && command_id == ZCL_CMD_DEFAULT_RSP)
    {
        options &= ~AF_ACK_REQUEST;
    }

    if (specific)
    {
        frame_header.fc.type = ZCL_FRAME_TYPE_SPECIFIC_CMD;
    }
    else
    {
        frame_header.fc.type = ZCL_FRAME_TYPE_PROFILE_CMD;
    }

    if (manuf_code)
    {
        frame_header.fc.manu_specific = 1;
        frame_header.manuf_code = manuf_code;
    }

    if (direction)
    {
        frame_header.fc.direction = ZCL_FRAME_SERVER_CLIENT_DIR;
    }
    else
    {
        frame_header.fc.direction = ZCL_FRAME_CLIENT_SERVER_DIR;
    }

    if (disable_default_rsp)
    {
        frame_header.fc.disable_default_rsp = true;
    }
    
    frame_header.trans_seq_num = seq_num;
    frame_header.command_id = command_id;
    
    msg_len = zcl_calc_header_size(&frame_header) + cmd_len;

    msg_buff = (uint8_t *)ZB_MEM_MALLOC(msg_len);
    if (msg_buff)
    {
        pbuf = zcl_build_header(&frame_header, msg_buff);
        if (cmd_len)
        {
            memcpy(pbuf, cmd_data, cmd_len);
        }
        else
        {
            *pbuf = 0;
        }

        status = zb_af_data_req(
            dst_addr, src_ep, cluster_id, msg_buff, msg_len,
            options, AF_DEFAULT_RADIUS);
        ZB_MEM_FREE(msg_buff);
    }
    else
    {
        status = ZB_MEM_ERROR;
    }
    return status;
}

/**
 * @brief Send a ZCL read command
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cluster_id Cluster ID
 * @param[in] read_cmd Pointer to the read command structure
 * @param[in] direction Direction of the command
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return int Status of the command
 */
zb_status_t
zb_zcl_send_read_manu(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_read_attr_cmd_t *read_cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint16_t manuf_code, uint8_t seq_num)
{
    uint16_t data_len;
    uint8_t *buf;
    uint8_t *pbuf;
    int status;

    data_len = read_cmd->num_attr * 2;

    buf = (uint8_t *)ZB_MEM_MALLOC(data_len);
    if (buf)
    {
        pbuf = buf;
        for (uint8_t i = 0; i < read_cmd->num_attr; i++)
        {
            *pbuf++ = LO_UINT16(read_cmd->attr_id[i]);
            *pbuf++ = HI_UINT16(read_cmd->attr_id[i]);
        }

        status = zb_zcl_send_cmd(
                    src_ep, dst_addr, cluster_id, ZCL_CMD_READ, 0,
                    direction, disable_default_rsp, manuf_code, seq_num, data_len, buf);
        ZB_MEM_FREE(buf);
    }
    else
    {
        status = ZB_MEM_ERROR;
    }
    return status;
}

zb_status_t
zb_zcl_send_read(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_read_attr_cmd_t *read_cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint8_t seq_num)
{
    return zb_zcl_send_read_manu(src_ep, dst_addr, cluster_id, read_cmd,
                                 direction, disable_default_rsp, 0, seq_num);
}

zb_status_t
zb_zcl_send_read_rsp(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_read_attr_rsp_cmd_t *read_rsp,
    uint8_t direction, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t *buf;
    uint16_t len = 0;
    zb_status_t status;

    // Calculate the size of the command
    for (uint8_t i = 0; i < read_rsp->num_attr; i++)
    {
        s_zb_zcl_read_attr_rsp_info_t *rsp_info = &read_rsp->attr_list[i];

        len += 2 + 1; // Attribute ID + status

        if (rsp_info->status == ZCL_STATUS_SUCCESS)
        {
            len++;

            if (rsp_info->data)
            {
                len += zb_zcl_get_attr_data_len(rsp_info->data_type, rsp_info->data);
            }
            else
            {
                len += zcl_get_attr_data_len_using_cb(src_ep, cluster_id, rsp_info->attr_id);
            }
        }
    } // for loop

    buf = (uint8_t *)ZB_MEM_MALLOC(len);
    if (buf)
    {
        uint8_t *pbuf = buf;

        for (uint8_t i = 0; i < read_rsp->num_attr; i++)
        {
            s_zb_zcl_read_attr_rsp_info_t *rsp_info = &read_rsp->attr_list[i];

            *pbuf++ = LO_UINT16(rsp_info->attr_id);
            *pbuf++ = HI_UINT16(rsp_info->attr_id);
            *pbuf++ = rsp_info->status;

            if (rsp_info->status == ZCL_STATUS_SUCCESS)
            {
                *pbuf++ = rsp_info->data_type;
                if (rsp_info->data)
                {
                    pbuf = zcl_serialize_attr_data(rsp_info->data_type, rsp_info->data, pbuf);
                }
                else
                {
                    uint16_t data_len;

                    zcl_read_attr_data_using_cb(src_ep, cluster_id, rsp_info->attr_id, pbuf, &data_len);
                    pbuf += data_len;
                }
            }
        } // for loop

        status = zb_zcl_send_cmd(
                    src_ep, dst_addr, cluster_id, ZCL_CMD_READ_RSP, 0,
                    direction, disable_default_rsp, 0, seq_num, len, buf);
        ZB_MEM_FREE(buf);
    }
    else
    {
        status = ZB_MEM_ERROR;
    }

    return status;
}

/**
 * @brief Serialize the attribute data to the buffer
 * 
 * @param[in] data_type Data type
 * @param[in] data Pointer to the data
 * @param[in] buf Pointer to the buffer
 * @return uint8_t* Pointer to the buffer
 */
static uint8_t *
zcl_serialize_attr_data(uint8_t data_type, void *attr_data, uint8_t *buf)
{
    uint16_t data_len;

    if (attr_data == NULL)
    {
        return buf;
    }

    switch (data_type)
    {
        case ZCL_DATATYPE_DATA8:
        case ZCL_DATATYPE_BOOLEAN:
        case ZCL_DATATYPE_BITMAP8:
        case ZCL_DATATYPE_INT8:
        case ZCL_DATATYPE_UINT8:
        case ZCL_DATATYPE_ENUM8:
            *buf++ = *(uint8_t *)attr_data;
            break;
        case ZCL_DATATYPE_DATA16:
        case ZCL_DATATYPE_BITMAP16:
        case ZCL_DATATYPE_INT16:
        case ZCL_DATATYPE_UINT16:
        case ZCL_DATATYPE_ENUM16:
        case ZCL_DATATYPE_SEMI_PREC:
        case ZCL_DATATYPE_CLUSTER_ID:
        case ZCL_DATATYPE_ATTR_ID:
            *buf++ = LO_UINT16(*(uint16_t *)attr_data);
            *buf++ = HI_UINT16(*(uint16_t *)attr_data);
            break;
        case ZCL_DATATYPE_DATA24:
        case ZCL_DATATYPE_BITMAP24:
        case ZCL_DATATYPE_INT24:
        case ZCL_DATATYPE_UINT24:
            *buf++ = BREAK_UINT32(*(uint32_t *)attr_data, 0);
            *buf++ = BREAK_UINT32(*(uint32_t *)attr_data, 1);
            *buf++ = BREAK_UINT32(*(uint32_t *)attr_data, 2);
            break;
        case ZCL_DATATYPE_DATA32:
        case ZCL_DATATYPE_BITMAP32:
        case ZCL_DATATYPE_INT32:
        case ZCL_DATATYPE_UINT32:
        case ZCL_DATATYPE_SINGLE_PREC:
        case ZCL_DATATYPE_TOD:
        case ZCL_DATATYPE_DATE:
        case ZCL_DATATYPE_UTC:
        case ZCL_DATATYPE_BAC_OID:
            *buf++ = BREAK_UINT32(*(uint32_t *)attr_data, 0);
            *buf++ = BREAK_UINT32(*(uint32_t *)attr_data, 1);
            *buf++ = BREAK_UINT32(*(uint32_t *)attr_data, 2);
            *buf++ = BREAK_UINT32(*(uint32_t *)attr_data, 3);
            break;
        case ZCL_DATATYPE_DATA40:
        case ZCL_DATATYPE_BITMAP40:
        case ZCL_DATATYPE_INT40:
        case ZCL_DATATYPE_UINT40:
            memcpy(buf, attr_data, 5);
            buf += 5;
            break;
        case ZCL_DATATYPE_DATA48:
        case ZCL_DATATYPE_BITMAP48:
        case ZCL_DATATYPE_INT48:
        case ZCL_DATATYPE_UINT48:
            memcpy(buf, attr_data, 6);
            buf += 6;
            break;
        case ZCL_DATATYPE_DATA56:
        case ZCL_DATATYPE_BITMAP56:
        case ZCL_DATATYPE_INT56:
        case ZCL_DATATYPE_UINT56:
            memcpy(buf, attr_data, 7);
            buf += 7;
            break;
        case ZCL_DATATYPE_DATA64:
        case ZCL_DATATYPE_BITMAP64:
        case ZCL_DATATYPE_INT64:
        case ZCL_DATATYPE_UINT64:
        case ZCL_DATATYPE_DOUBLE_PREC:
        case ZCL_DATATYPE_IEEE_ADDR:
            memcpy(buf, attr_data, 8);
            buf += 8;
            break;
        case ZCL_DATATYPE_CHAR_STR:
        case ZCL_DATATYPE_OCTET_STR:
            data_len = *(uint8_t *)attr_data;
            memcpy(buf, attr_data, data_len + 1); // String length + 1 byte for length field
            buf += data_len + 1;
            break;
        case ZCL_DATATYPE_LONG_CHAR_STR:
        case ZCL_DATATYPE_LONG_OCTET_STR:
            data_len = BUILD_UINT16(*(uint8_t *)attr_data, *(uint8_t *)(attr_data + 1));  // Long string length
            memcpy(buf, attr_data, data_len + 2); // Long string length + 2 bytes for length field
            buf += data_len + 2;
            break;
        case ZCL_DATATYPE_128_BIT_SEC_KEY:
            memcpy(buf, attr_data, 16);
            buf += 16;
            break;
        case ZCL_DATATYPE_NO_DATA:
        case ZCL_DATATYPE_UNKNOWN:
            // Fall through
        default:
            break;
    }

    return buf;
}

/**
 * @brief Send a ZCL write command
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cluster_id Cluster ID
 * @param[in] write_cmd Pointer to the write command structure
 * @param[in] cmd Command ID
 * @param[in] direction Direction of the command
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return int Status of the command
 */
zb_status_t
zb_zcl_send_write_manu(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint16_t cluster_id,
    s_zb_zcl_write_attr_cmd_t *write_cmd, uint8_t cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint16_t manuf_code, uint8_t seq_num)
{
    uint16_t data_len = 0;
    uint8_t *buf;
    uint8_t *pbuf;
    int status;

    for (uint8_t i = 0; i < write_cmd->num_attr; i++)
    {
        s_zb_zcl_write_attr_info_t *attr_info = &write_cmd->attr_list[i];

        data_len += 2 + 1; // Attribute ID and Attribute data type
        data_len += zb_zcl_get_attr_data_len(attr_info->data_type, attr_info->attr_data);
    }

    buf = (uint8_t *)ZB_MEM_MALLOC(data_len);
    if (buf)
    {
        pbuf = buf;
        for (uint8_t i = 0; i < write_cmd->num_attr; i++)
        {
            s_zb_zcl_write_attr_info_t *attr_info = &write_cmd->attr_list[i];
            *pbuf++ = LO_UINT16(attr_info->attr_id);
            *pbuf++ = HI_UINT16(attr_info->attr_id);
            *pbuf++ = attr_info->data_type;
            pbuf = zcl_serialize_attr_data(attr_info->data_type, attr_info->attr_data, pbuf);
        }

        status = zb_zcl_send_cmd(
                    src_ep, dst_addr, cluster_id, cmd, 0,
                    direction, disable_default_rsp, manuf_code, seq_num, data_len, buf);
        ZB_MEM_FREE(buf);
    }
    else
    {
        status = ZB_MEM_ERROR;
    }

    return status;
}

zb_status_t
zb_zcl_send_write(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint16_t cluster_id,
    s_zb_zcl_write_attr_cmd_t *write_cmd, uint8_t cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint8_t seq_num)
{
    return zb_zcl_send_write_manu(src_ep, dst_addr, cluster_id, write_cmd, cmd,
                                  direction, disable_default_rsp, 0, seq_num);
}

/**
 * @brief Check if the data type is analog
 * 
 * @param[in] data_type Data type
 * @return uint8_t 1 if the data type is analog, 0 otherwise
 */
uint8_t
zb_zcl_is_analog_data_type(uint8_t data_type)
{
  uint8_t analog;

  switch ( data_type )
  {
    case ZCL_DATATYPE_UINT8:
    case ZCL_DATATYPE_UINT16:
    case ZCL_DATATYPE_UINT24:
    case ZCL_DATATYPE_UINT32:
    case ZCL_DATATYPE_UINT40:
    case ZCL_DATATYPE_UINT48:
    case ZCL_DATATYPE_UINT56:
    case ZCL_DATATYPE_UINT64:
    case ZCL_DATATYPE_INT8:
    case ZCL_DATATYPE_INT16:
    case ZCL_DATATYPE_INT24:
    case ZCL_DATATYPE_INT32:
    case ZCL_DATATYPE_INT40:
    case ZCL_DATATYPE_INT48:
    case ZCL_DATATYPE_INT56:
    case ZCL_DATATYPE_INT64:
    case ZCL_DATATYPE_SEMI_PREC:
    case ZCL_DATATYPE_SINGLE_PREC:
    case ZCL_DATATYPE_DOUBLE_PREC:
    case ZCL_DATATYPE_TOD:
    case ZCL_DATATYPE_DATE:
    case ZCL_DATATYPE_UTC:
      analog = 1;
      break;

    default:
      analog = 0;
      break;
  }

  return ( analog );
}

/**
 * @brief Send a ZCL config report command
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cluster_id Cluster ID
 * @param[in] config_report_cmd Pointer to the config report command structure
 * @param[in] direction Direction of the command
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return int Status of the command
 */
zb_status_t
zb_zcl_send_config_report_cmd_manu(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_config_report_cmd_t *config_report_cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint16_t manuf_code, uint8_t seq_num)
{
    uint16_t data_len = 0;
    uint8_t *buf;
    int status;

    // Find out the data length
    for (uint8_t i = 0; i < config_report_cmd->num_attr; i++)
    {
        s_zb_zcl_config_report_info_t *attr_info = &config_report_cmd->attr_list[i];
        data_len += 1 + 2; // Direction and Attribute ID
        
        if (attr_info->direction == ZCL_SEND_ATTR_REPORTS)
        {
            data_len += 1 + 2 + 2; // Data type + Min + Max Reporting interval

            // Find out the size of the Reportable Change field (for Analog data types)
            if (zb_zcl_is_analog_data_type(attr_info->data_type))
            {
                data_len += zb_zcl_get_data_type_length(attr_info->data_type);
            }
        }
        else
        {
            data_len += 2; // Timeout period
        }
    }

    buf = (uint8_t *)ZB_MEM_MALLOC(data_len);
    if (buf)
    {
        uint8_t *pbuf = buf;
        for (uint8_t i = 0; i < config_report_cmd->num_attr; i++)
        {
            s_zb_zcl_config_report_info_t *attr_info = &config_report_cmd->attr_list[i];

            *pbuf++ = attr_info->direction;
            *pbuf++ = LO_UINT16(attr_info->attr_id);
            *pbuf++ = HI_UINT16(attr_info->attr_id);
            if (attr_info->direction == ZCL_SEND_ATTR_REPORTS)
            {
                *pbuf++ = attr_info->data_type;
                *pbuf++ = LO_UINT16(attr_info->min_report_int);
                *pbuf++ = HI_UINT16(attr_info->min_report_int);
                *pbuf++ = LO_UINT16(attr_info->max_report_int);
                *pbuf++ = HI_UINT16(attr_info->max_report_int);
                if (zb_zcl_is_analog_data_type(attr_info->data_type))
                {
                    pbuf = zcl_serialize_attr_data(attr_info->data_type, attr_info->reportable_change, pbuf);
                }
            }
            else
            {
                *pbuf++ = LO_UINT16(attr_info->timeout_period);
                *pbuf++ = HI_UINT16(attr_info->timeout_period);
            }
        }

        status = zb_zcl_send_cmd(
                    src_ep, dst_addr, cluster_id, ZCL_CMD_CONFIG_REPORT, 0,
                    direction, disable_default_rsp, manuf_code, seq_num, data_len, buf);
        ZB_MEM_FREE(buf);
    }
    else
    {
        status = ZB_MEM_ERROR;
    }

    return status;
}

zb_status_t
zb_zcl_send_config_report_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_config_report_cmd_t *config_report_cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint8_t seq_num)
{
    return zb_zcl_send_config_report_cmd_manu(src_ep, dst_addr, cluster_id, config_report_cmd,
                                              direction, disable_default_rsp, 0, seq_num);
}

/**
 * @brief Send a ZCL read report configuration command
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cluster_id Cluster ID
 * @param[in] read_report_cfg_cmd Pointer to the read report configuration command structure
 * @param[in] direction Direction of the command
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t
zb_zcl_send_read_report_cfg_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_read_report_cfg_cmd_t *read_report_cfg_cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint16_t data_len = 0;
    uint8_t *buf;
    uint8_t *pbuf;
    zb_status_t status;

    data_len = read_report_cfg_cmd->num_attr * (1 + 2); // Direction and Attribute ID

    buf = (uint8_t *)ZB_MEM_MALLOC(data_len);
    if (buf)
    {
        pbuf = buf;

        for (uint8_t i = 0; i < read_report_cfg_cmd->num_attr; i++)
        {
            *pbuf++ = read_report_cfg_cmd->attr_list[i].direction;
            *pbuf++ = LO_UINT16(read_report_cfg_cmd->attr_list[i].attr_id);
            *pbuf++ = HI_UINT16(read_report_cfg_cmd->attr_list[i].attr_id);
        }

        status = zb_zcl_send_cmd(
                    src_ep, dst_addr, cluster_id, ZCL_CMD_READ_REPORT_CFG, 0,
                    direction, disable_default_rsp, 0, seq_num, data_len, buf);
        ZB_MEM_FREE(buf);
    }
    else
    {
        status = ZB_MEM_ERROR;
    }

    return status;
}

zb_status_t
zb_zcl_send_default_rsp_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t cluster_id, s_zb_zcl_default_rsp_cmd_t *default_rsp_cmd,
    uint8_t direction, uint8_t disable_default_rsp, uint16_t manuf_code, uint8_t seq_num)
{
    uint8_t buf[2]; // Command ID and status
    zb_status_t status;

    buf[0] = default_rsp_cmd->command_id;
    buf[1] = default_rsp_cmd->status_code;

    status = zb_zcl_send_cmd(
                src_ep, dst_addr, cluster_id, ZCL_CMD_DEFAULT_RSP, 0,
                direction, disable_default_rsp, manuf_code, seq_num, 2, buf);

    return status;
}

void *
zb_zcl_parse_in_read_cmd(s_zb_zcl_parse_cmd_t *cmd)
{
    s_zb_zcl_read_attr_cmd_t *read_cmd;
    uint8_t *pbuf = cmd->data;

    if (cmd->data_len % sizeof(uint16_t) != 0)
    {
        return (void *)NULL;
    }

    read_cmd = (s_zb_zcl_read_attr_cmd_t *)
                    ZB_MEM_MALLOC(sizeof(s_zb_zcl_read_attr_cmd_t) + cmd->data_len);
    if (read_cmd)
    {
        read_cmd->num_attr = cmd->data_len / sizeof(uint16_t);
        for (uint8_t i = 0; i < read_cmd->num_attr; i++)
        {
            read_cmd->attr_id[i] = BUILD_UINT16(pbuf[0], pbuf[1]);
            pbuf += sizeof(uint16_t);
        }
    }
    return ((void *)read_cmd);
}

uint8_t*
zb_zcl_parse_header(s_zb_zcl_frame_header_t *header, uint8_t *data)
{
    // Clear the header
    memset((uint8_t *)header, 0, sizeof(s_zb_zcl_frame_header_t));

    // Parse the frame control
    header->fc.type = ZCL_FC_TYPE(*data);
    header->fc.manu_specific = ZCL_FC_MANU_SPECIFIC(*data) ? 1 : 0;
    if (ZCL_FC_DIRECTION(*data))
    {
        header->fc.direction = ZCL_FRAME_SERVER_CLIENT_DIR;
    }
    else
    {
        header->fc.direction = ZCL_FRAME_CLIENT_SERVER_DIR;
    }

    header->fc.disable_default_rsp = ZCL_FC_DISABLE_DEFAULT_RSP(*data) ? 1 : 0;
    data++;

    if (header->fc.manu_specific)
    {
        header->manuf_code = BUILD_UINT16(data[0], data[1]);
        data += 2;
    }

    // Parse the transaction sequence number
    header->trans_seq_num = *data++;

    // Parse the command ID
    header->command_id = *data++;

    // Return the pointer to the next byte after the header
    return (data);
}

uint8_t
zb_zcl_find_attr_rec(
    uint8_t endpoint, uint16_t cluster_id,
    uint16_t attr_id, s_zb_zcl_attr_rec_t *attr_rec)
{
    s_zb_zcl_attr_rec_list_t *p_attr = zcl_find_attr_recs_list(endpoint);

    if (p_attr != NULL)
    {
        for (uint8_t i = 0; i < p_attr->num_attrs; i++)
        {
            if ((p_attr->attrs[i].cluster_id == cluster_id) &&
                (p_attr->attrs[i].attr.attr_id == attr_id))
            {
                *attr_rec = p_attr->attrs[i];
                return true;
            }
        }
    }
    return false;
}

zb_status_t
zb_zcl_register_plugin(
    uint16_t start_cluster_id, uint16_t end_cluster_id,
    pfn_zcl_incoming_msg_handler_t pfn_incoming_msg_handler)
{
    s_zb_zcl_lib_plugin_t *p_new_item;
    s_zb_zcl_lib_plugin_t *p_loop;

    p_new_item = (s_zb_zcl_lib_plugin_t *)
                    ZB_MEM_MALLOC(sizeof(s_zb_zcl_lib_plugin_t));
    if (p_new_item == NULL)
    {
        return ZB_MEM_ERROR;
    }

    p_new_item->next = NULL;
    p_new_item->start_cluster_id = start_cluster_id;
    p_new_item->end_cluster_id = end_cluster_id;
    p_new_item->pfn_incoming_msg_handler = pfn_incoming_msg_handler;

    if (g_zcl_plugins == NULL)
    {
        g_zcl_plugins = p_new_item;
    }
    else
    {
        p_loop = g_zcl_plugins;
        while (p_loop->next != NULL)
        {
            p_loop = p_loop->next;
        }
        p_loop->next = p_new_item;
    }
    return ZB_SUCCESS;
}

zb_status_t
zb_zcl_register_command_list(
    uint8_t endpoint, uint8_t cmds_list_size, const s_zb_zcl_command_rec_t cmds_list[])
{
    s_zb_zcl_cmd_rec_list_t *p_new_item;
    s_zb_zcl_cmd_rec_list_t *p_loop;

    p_new_item = (s_zb_zcl_cmd_rec_list_t *)
                    ZB_MEM_MALLOC(sizeof(s_zb_zcl_cmd_rec_list_t));
    if (p_new_item == NULL)
    {
        return ZB_MEM_ERROR;
    }

    p_new_item->next = NULL;
    p_new_item->endpoint = endpoint;
    p_new_item->num_cmds = cmds_list_size;
    p_new_item->cmds = cmds_list;

    if (g_zcl_cmd_list == NULL)
    {
        g_zcl_cmd_list = p_new_item;
    }
    else
    {
        p_loop = g_zcl_cmd_list;
        while (p_loop->next != NULL)
        {
            p_loop = p_loop->next;
        }
        p_loop->next = p_new_item;
    }
    return ZB_SUCCESS;
}

zb_status_t
zb_zcl_register_attr_list(
    uint8_t endpoint, uint8_t num_attrs, const s_zb_zcl_attr_rec_t attr_list[])
{
    s_zb_zcl_attr_rec_list_t *p_new_item;
    s_zb_zcl_attr_rec_list_t *p_loop;

    p_new_item = (s_zb_zcl_attr_rec_list_t *)
                    ZB_MEM_MALLOC(sizeof(s_zb_zcl_attr_rec_list_t));
    if (p_new_item == NULL)
    {
        return ZB_MEM_ERROR;
    }

    p_new_item->next = NULL;
    p_new_item->endpoint = endpoint;
    p_new_item->pfn_read_write_cb = NULL;
    p_new_item->pfn_authorize_cb = NULL;
    p_new_item->num_attrs = num_attrs;
    p_new_item->attrs = attr_list;

    if (g_zcl_attr_list == NULL)
    {
        g_zcl_attr_list = p_new_item;
    }
    else
    {
        p_loop = g_zcl_attr_list;
        while (p_loop->next != NULL)
        {
            p_loop = p_loop->next;
        }
        p_loop->next = p_new_item;
    }
    return ZB_SUCCESS;
}

zb_status_t
zb_zcl_register_cluster_option_list(
    uint8_t endpoint, uint8_t num_options, const s_zb_zcl_option_rec_t option_list[])
{
    s_zb_zcl_cluster_option_list_t *p_new_item;
    s_zb_zcl_cluster_option_list_t *p_loop;

    p_new_item = (s_zb_zcl_cluster_option_list_t *)
                    ZB_MEM_MALLOC(sizeof(s_zb_zcl_cluster_option_list_t));
    if (p_new_item == NULL)
    {
        return ZB_MEM_ERROR;
    }

    p_new_item->next = NULL;
    p_new_item->endpoint = endpoint;
    p_new_item->num_options = num_options;
    p_new_item->options = option_list;

    if (g_zcl_cluster_option_list == NULL)
    {
        g_zcl_cluster_option_list = p_new_item;
    }
    else
    {
        p_loop = g_zcl_cluster_option_list;
        while (p_loop->next != NULL)
        {
            p_loop = p_loop->next;
        }
        p_loop->next = p_new_item;
    }
    return ZB_SUCCESS;
}

uint8_t
zb_zcl_get_cluster_option(uint8_t endpoint, uint16_t cluster_id)
{
    uint8_t option;
    s_zb_zcl_option_rec_t *p_option = zcl_find_cluster_option(endpoint, cluster_id);
    if (p_option != NULL)
    {
        option = p_option->option;
        if (!ZG_SECURE_ENABLED)
        {
            option &= (AF_EN_SECURITY ^ 0xFF);
        }
        return option;
    }
    return AF_TX_OPTION_NONE;
}

zb_status_t
zb_zcl_register_read_write_callback(
    uint8_t endpoint,
    pfn_zcl_read_write_callback_t pfn_read_write_callback,
    pfn_zcl_authorize_callback_t pfn_authorize_callback)
{
    s_zb_zcl_attr_rec_list_t *p_attr = zcl_find_attr_recs_list(endpoint);
    if (p_attr != NULL)
    {
        p_attr->pfn_read_write_cb = pfn_read_write_callback;
        p_attr->pfn_authorize_cb = pfn_authorize_callback;
        return ZB_SUCCESS;
    }
    return ZB_FAILURE;
}

zb_status_t
zb_zcl_register_external_cmd_handler(
    uint8_t endpoint, pfn_zcl_unhandled_cmd_handler_t pfn_unhandled_cmd_handler)
{
    s_zb_zcl_handle_external_list_t *p_new_item;
    s_zb_zcl_handle_external_list_t *p_loop;

    p_new_item = (s_zb_zcl_handle_external_list_t *)
                    ZB_MEM_MALLOC(sizeof(s_zb_zcl_handle_external_list_t));
    if (p_new_item)
    {
        p_new_item->next = NULL;
        p_new_item->endpoint = endpoint;
        p_new_item->pfn_handler = pfn_unhandled_cmd_handler;

        if (g_zcl_handle_external_list == NULL)
        {
            g_zcl_handle_external_list = p_new_item;
        }
        else
        {
            p_loop = g_zcl_handle_external_list;
            while (p_loop->next != NULL)
            {
                p_loop = p_loop->next;
            }
            p_loop->next = p_new_item;
        }
        return ZB_SUCCESS;
    }
    return ZB_MEM_ERROR;
}

zb_status_t
zb_zcl_register_manu_spec_profile_wide_cmd_handler(
    pfn_zcl_manu_spec_profile_wide_cmd_callback_t pfn_manu_spec_profile_wide_cmd_callback)
{
    g_pfn_mspw_cmd_cb = pfn_manu_spec_profile_wide_cmd_callback;
    return ZB_SUCCESS;
}

e_zcl_proc_msg_status_t
zb_zcl_process_message(s_zb_af_incoming_msg_t *msg)
{
    s_zb_zcl_incoming_msg_t in_msg;
    s_zb_zcl_lib_plugin_t *p_in_plugin;
    s_zb_zcl_default_rsp_cmd_t default_rsp_cmd;
    uint8_t options;
    uint8_t security_enable;
    uint8_t inter_pan_msg = false;
    zb_status_t status = ZB_FAILURE;
    uint8_t default_response_sent = false;

    if (msg->command.data_length == 0)
    {
        return ZCL_PROC_MSG_INVALID;
    }

    g_zcl_raw_af_msg = (s_zb_af_incoming_msg_t *)msg;
    in_msg.msg = msg;
    in_msg.attr_cmd = NULL;
    in_msg.data = NULL;
    in_msg.data_len = 0;

    in_msg.data = zb_zcl_parse_header(&in_msg.hdr, msg->command.data);
    in_msg.data_len = msg->command.data_length;
    in_msg.data_len -= (uint16_t)(in_msg.data - msg->command.data);

    g_saved_zcl_trans_id = in_msg.hdr.trans_seq_num;

    options = zb_zcl_get_cluster_option(msg->src_endpoint, msg->cluster_id);

    // Find the appropriate plugin
    p_in_plugin = zcl_find_zcl_plugin(msg->cluster_id);

    // Local and remote Security options must match except for Default Response command
    if ((p_in_plugin != NULL) && !ZCL_DEFAULT_RSP_CMD(in_msg.hdr))
    {
        security_enable = (options & AF_EN_SECURITY) ? true : false;

        // Make sure that Clusters specifically defined to use security are received secure,
        // any other cluster that wants to use APS security will be allowed
        if ((security_enable) && (!msg->security_use))
        {
            if (UNICAST_MSG(in_msg.msg))
            {
                // Send a Default Response command back with no Application Link Key security
                zcl_set_security_option(msg->dst_endpoint, msg->cluster_id, 0);
                default_rsp_cmd.status_code = status;
                default_rsp_cmd.command_id = in_msg.hdr.command_id;
                zb_zcl_send_default_rsp_cmd(
                    msg->dst_endpoint, &msg->src_addr,
                    msg->cluster_id, &default_rsp_cmd,
                    !in_msg.hdr.fc.direction, true,
                    in_msg.hdr.manuf_code, in_msg.hdr.trans_seq_num);

                zcl_set_security_option(msg->dst_endpoint, msg->cluster_id, true);
            }

            g_zcl_raw_af_msg = NULL;
            return ZCL_PROC_MSG_NOT_SECURE; // Error, ignore the message
        }
    }

    // Is this a foundation type message
    if (!inter_pan_msg && ZCL_PROFILE_CMD(in_msg.hdr.fc.type))
    {
        if (in_msg.hdr.fc.manu_specific)
        {
            /* Give a registered manufacturer-specific handler first refusal on
             * the RAW payload. This must happen BEFORE the generic parse: some
             * vendors (e.g. Lumi/Aqara 0x115F on the Basic cluster) use payload
             * layouts and data types the generic attribute-record parser cannot
             * walk, and letting it try is what crashes the firmware. */
            const s_zb_zcl_manu_handler_t *manu_entry =
                zb_zcl_manu_find_handler(in_msg.hdr.manuf_code, msg->cluster_id, in_msg.hdr.command_id);

            if (manu_entry != NULL)
            {
                status = manu_entry->handler(&in_msg);
            }

            /* No entry, or the entry declined the frame -> common handler. */
            if ((manu_entry == NULL) || (status == ZCL_STATUS_UNSUP_MANU_GENERAL_COMMAND))
            {
                if (g_pfn_mspw_cmd_cb != NULL)
                {
                    /* Parse the profile-wide payload (e.g. Report Attributes /
                     * Read Attributes Response) so the handler can dispatch
                     * manufacturer-specific reports the same way as standard ones
                     * (needed for the Develco/frient VOC cluster 0xFC03). */
                    if ((in_msg.hdr.command_id <= ZCL_CMD_MAX) &&
                        (g_zcl_cmd_table[in_msg.hdr.command_id].pfn_parse_in_profile != NULL))
                    {
                        s_zb_zcl_parse_cmd_t parse_cmd;
                        parse_cmd.endpoint = msg->dst_endpoint;
                        parse_cmd.data_len = in_msg.data_len;
                        parse_cmd.data = in_msg.data;
                        in_msg.attr_cmd = ZCL_PARSE_CMD(in_msg.hdr.command_id, &parse_cmd);
                    }

                    status = g_pfn_mspw_cmd_cb(&in_msg);

                    if (in_msg.attr_cmd)
                    {
                        ZB_MEM_FREE(in_msg.attr_cmd);
                        in_msg.attr_cmd = NULL;
                    }
                }
                else
                {
                    status = ZCL_STATUS_UNSUP_MANU_GENERAL_COMMAND;
                }
            }
        }
        else if ((in_msg.hdr.command_id <= ZCL_CMD_MAX) &&
                 (g_zcl_cmd_table[in_msg.hdr.command_id].pfn_parse_in_profile != NULL))
        {
            s_zb_zcl_parse_cmd_t parse_cmd;
            parse_cmd.endpoint = msg->dst_endpoint;
            parse_cmd.data_len = in_msg.data_len;
            parse_cmd.data = in_msg.data;

            // Parse the command, remember that the return value is a pointer to allocated memory
            in_msg.attr_cmd = ZCL_PARSE_CMD(in_msg.hdr.command_id, &parse_cmd);
            if ((in_msg.attr_cmd != NULL) && (g_zcl_cmd_table[in_msg.hdr.command_id].pfn_process_in_profile != NULL))
            {
                // Process the command
                if (ZCL_PROCESS_CMD(in_msg.hdr.command_id, &in_msg) == false)
                {
                    // Couldn't find attribute in the table.
                }
            }

            // Free the buffer
            if (in_msg.attr_cmd)
            {
                ZB_MEM_FREE(in_msg.attr_cmd);
            }

            if (ZCL_CMD_HAS_RSP(in_msg.hdr.command_id))
            {
                g_zcl_raw_af_msg = NULL;
                return ZCL_PROC_MSG_SUCCESS;
            }

            status = ZB_SUCCESS;
        }
        else
        {
            status = ZCL_STATUS_UNSUP_GENERAL_COMMAND;
        }
    }
    else // Not foundation type message, so it must be specific to the cluster ID
    {
        if (p_in_plugin && p_in_plugin->pfn_incoming_msg_handler)
        {
            // The return value of the plugin function will be
            // ZB_SUCCESS - Supported and need default response
            // ZB_FAILURE - Not supported
            // ZCL_STATUS_CMD_HAS_RSP - Supported and do not need default response
            // ZCL_STATUS_INVALID_FIELD - Supported, but the incoming msg is wrong formatted
            // ZCL_STATUS_INVALID_VALUE - SUpported, but the request not achievable by the h/w
            // ZCL_STATUS_SOFTWARE_FAILURE - Supported, but ZStack memory allocation fails
            status = p_in_plugin->pfn_incoming_msg_handler(&in_msg);
            if (status == ZCL_STATUS_CMD_HAS_RSP || (inter_pan_msg && status == ZB_SUCCESS))
            {
                g_zcl_raw_af_msg = NULL;
                return ZCL_PROC_MSG_SUCCESS;
            }
        }
        if (status == ZB_FAILURE)
        {
            // Not supported
            if (in_msg.hdr.fc.manu_specific)
            {
                status = ZCL_STATUS_UNSUP_MANU_CLUSTER_COMMAND;
            }
            else
            {
                status = ZCL_STATUS_UNSUP_CLUSTER_COMMAND;
            }
        }
    }

    if (UNICAST_MSG(in_msg.msg) && in_msg.hdr.fc.disable_default_rsp == 0)
    {
        // Send a Default Response command back
        default_rsp_cmd.status_code = status;
        default_rsp_cmd.command_id = in_msg.hdr.command_id;
        zb_zcl_send_default_rsp_cmd(
            msg->dst_endpoint, &msg->src_addr,
            msg->cluster_id, &default_rsp_cmd,
            !in_msg.hdr.fc.direction, true,
            in_msg.hdr.manuf_code, in_msg.hdr.trans_seq_num);
        default_response_sent = true;
    }

    g_zcl_raw_af_msg = NULL;
    if (status == ZB_SUCCESS)
    {
        return ZCL_PROC_MSG_SUCCESS;
    }
    else if (status == ZCL_STATUS_UNSUP_MANU_GENERAL_COMMAND)
    {
        if (default_response_sent)
        {
            return ZCL_PROC_MSG_MANUFACTURER_SPECIFIC_DR;
        }
        else
        {
            return ZCL_PROC_MSG_MANUFACTURER_SPECIFIC;
        }
    }
    else
    {
        if (default_response_sent)
        {
            return ZCL_PROC_MSG_NOT_HANDLED_DR;
        }
        else
        {
            return ZCL_PROC_MSG_NOT_HANDLED;
        }
    }
    return status;
}

uint8_t
zb_zcl_get_parsed_trans_seq_num(void)
{
    return g_saved_zcl_trans_id;
}

uint8_t
zb_zcl_next_seq_num(void)
{
    /* Allocate the next outgoing ZCL Transaction Sequence Number.  Skips 0 so
     * callers keep the historical 1..255 range.  Distinct from the AF/APS
     * transaction id assigned inside zb_af_data_req(); a ZCL *response* must
     * instead echo the request's sequence number (see zcl_process_in_read_cmd
     * → zb_zcl_send_read_rsp), not allocate a fresh one. */
    if (g_zcl_trans_id == 0)
    {
        g_zcl_trans_id = 1;
    }
    return g_zcl_trans_id++;
}

s_zb_af_incoming_msg_t *
zb_zcl_get_raw_af_incoming_msg(void)
{
    return g_zcl_raw_af_msg;
}

zb_status_t
zb_zcl_send_read_attr_req(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint16_t cluster_id, uint16_t *attr_ids, uint8_t attr_count)
{
    zb_status_t status = ZB_OK;
    uint8_t *buf = (uint8_t *)ZB_MEM_MALLOC(sizeof(s_zb_zcl_read_attr_cmd_t) + attr_count * 2);
    if (buf)
    {
        s_zb_zcl_read_attr_cmd_t *read_attr_cmd = (s_zb_zcl_read_attr_cmd_t *)&buf[0];
        read_attr_cmd->num_attr = attr_count;
        for (uint8_t i = 0; i < attr_count; i++)
        {
            buf[i * 2] = LO_UINT16(attr_ids[i]);
            buf[i * 2 + 1] = HI_UINT16(attr_ids[i]);
        }
        uint8_t seq_num = zb_zcl_next_seq_num();
        status = zb_zcl_send_read(src_ep, dst_addr, cluster_id, read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, seq_num);
        ZB_MEM_FREE(buf);
    }
    else
    {
        ZB_LOGE(TAG, "%s() failed to allocate memory for read attribute request", __func__);
        status = ZB_MEM_ERROR;
    }
    return status;
}
