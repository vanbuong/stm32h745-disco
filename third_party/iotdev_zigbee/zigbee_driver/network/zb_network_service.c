/*
 * zb_network_service.c
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#include "common/zb_common.h"
#include "core/zb_core.h"
#include "osal/zb_osal.h"
#include "af/zb_af.h"
#include "device/zb_device.h"
#include "device/zb_device_manager.h"
#include "network/zb_network_service.h"
#include "zdo/zb_zdo.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"
#include "zcl/zb_zcl_ss.h"
#include "zcl/zb_zcl_lighting.h"

#define TAG "ZB_NWK_SRV"

/**
 * @brief Enum for adding new device
 * 
 */
typedef enum e_zb_nwksrv_ad_state
{
    ZB_NWKSRV_AD_STATE_AVAILABLE = 0,
    ZB_NWKSRV_AD_STATE_WAITING,
    ZB_NWKSRV_AD_STATE_GETTING_NETWORK_ADDR,
    ZB_NWKSRV_AD_STATE_GETTING_PARENT_IEEE_ADDR,
    ZB_NWKSRV_AD_STATE_GETTING_NODE_INFO,
    ZB_NWKSRV_AD_STATE_GETTING_ACTIVE_ENDPOINT,
    ZB_NWKSRV_AD_STATE_GETTING_SIMPLE_DESC,
    ZB_NWKSRV_AD_STATE_GETTING_IAS_ZONE_STATUS,
    ZB_NWKSRV_AD_STATE_WRITING_IAS_CIE_ADDRESS,
    ZB_NWKSRV_AD_STATE_IAS_ZONE_ENROLL_CHECKING,
    ZB_NWKSRV_AD_STATE_GETTING_BASIC_INFO,
    ZB_NWKSRV_AD_STATE_GETTING_EXTENDED_INFO,   /* best-effort extended identity read */
    ZB_NWKSRV_AD_STATE_GETTING_COLOR_CAPABILITIES,
    ZB_NWKSRV_AD_STATE_WRAP_UP
} e_zb_nwksrv_ad_state_t;

/* Extended identity is read in small chunks so each RESPONSE fits one APS frame
 * (simple/sleepy devices don't do fragmentation, so a big multi-string response
 * goes unanswered). The long string attributes are read one-per-request; the
 * short ones are grouped. */
#define ZB_NWKSRV_EXTENDED_INFO_CHUNK_MAX_ATTRS  3
static const struct
{
    uint8_t  count;
    uint16_t attrs[ZB_NWKSRV_EXTENDED_INFO_CHUNK_MAX_ATTRS];
} s_extended_info_chunks[] = {
    { 2, { ATTRID_BASIC_DATE_CODE, ATTRID_BASIC_PRODUCT_CODE } }, /* small: 1B + 2 short strings */
    { 1, { ATTRID_BASIC_SERIAL_NUMBER } },   /* string - read alone */
    { 1, { ATTRID_BASIC_PRODUCT_LABEL } },   /* string - read alone */
    { 1, { ATTRID_BASIC_SW_BUILD_ID } },     /* string - read alone */
};
#define ZB_NWKSRV_EXTENDED_INFO_CHUNK_COUNT \
    (sizeof(s_extended_info_chunks) / sizeof(s_extended_info_chunks[0]))

/**
 * @brief Struct for state machine of adding new device
 * 
 */
typedef struct s_zb_nwksrv_ad_state_machine
{
    e_zb_nwksrv_ad_state_t state;
    s_zb_device_info_t *device_info;                /* Allocated by the state machine, must be ZB_MEM_FREEd */
    pfn_zb_nwksrv_add_device_cb pfn_add_device_cb;
    uint8_t ep;                                     /* Which endpoint index are we getting info on? (SimpleDesc) */
    uint8_t ext_chunk;                              /* Current extended-identity read chunk index */
    uint8_t tries;                                  /* Up to 3 tries per state before timing out */
    uint32_t timeout;                               /* ms before timeout */
} s_zb_nwksrv_ad_state_machine_t;

/**
 * @brief Local Prototypes 
 *  
 */
static void zb_nwksrv_ad_free_state_machine(s_zb_nwksrv_ad_state_machine_t *state_ptr);
static void zb_nwksrv_ad_wrap_up(s_zb_nwksrv_ad_state_machine_t *state_ptr);
static s_zb_nwksrv_ad_state_machine_t *zb_nwksrv_ad_reuse_state_machine(uint64_t ieee_addr);
static s_zb_nwksrv_ad_state_machine_t *zb_nwksrv_ad_get_state_machine(void);
static void zb_nwksrv_ad_process_device_announce_handler(s_zb_nwksrv_event_element_t *element);
static void zb_nwksrv_ad_process_device_update_handler(s_zb_nwksrv_event_element_t *element);
static void zb_nwksrv_ad_process_device_remove_handler(s_zb_nwksrv_event_element_t *element);
static void zb_nwksrv_ad_process_ieee_addr_resp_handler(s_zb_nwksrv_event_element_t *element);
static void zb_nwksrv_ad_process_nwk_addr_resp_handler(s_zb_nwksrv_event_element_t *element);
static void zb_nwksrv_ad_process_node_desc_resp_handler(s_zb_nwksrv_event_element_t *element);
static void zb_nwksrv_ad_process_active_endpoint_resp_handler(s_zb_nwksrv_event_element_t *element);
static void zb_nwksrv_ad_process_simple_desc_resp_handler(s_zb_nwksrv_event_element_t *element);
static e_zb_zcl_observe_t zb_nwksrv_ad_process_read_basic_attr_resp_handler(s_zb_zcl_incoming_msg_t *msg);
static e_zb_zcl_observe_t zb_nwksrv_ad_process_read_color_attr_resp_handler(s_zb_zcl_incoming_msg_t *msg);
static e_zb_zcl_observe_t zb_nwksrv_ad_process_read_ias_zone_attr_resp_handler(s_zb_zcl_incoming_msg_t *msg);
static e_zb_zcl_observe_t zb_nwksrv_ad_process_write_ias_zone_attr_resp_handler(s_zb_zcl_incoming_msg_t *msg);
static void zb_nwksrv_ad_cache_report(s_zb_device_info_t *device_info, uint8_t endpoint,
    uint16_t cluster_id, const s_zb_zcl_report_attr_cmd_t *report);
static void zb_nwksrv_ad_process_state_machine(s_zb_nwksrv_ad_state_machine_t *state);
static void zb_nwksrv_ad_process_force_device_leave_network(s_zb_device_info_t *device_info);
static void zb_nwksrv_timer_callback(zb_os_timer_t timer, void *arg);
static inline void zb_nwksrv_reset_timeout(s_zb_nwksrv_ad_state_machine_t *state);
static inline void zb_nwksrv_reset_tries_count(s_zb_nwksrv_ad_state_machine_t *state);
static void zb_nwksrv_send_zcl_basic_info_read_req(uint16_t nwk_addr, uint8_t endpoint);
static void zb_nwksrv_send_zcl_extended_info_read_req(uint16_t nwk_addr, uint8_t endpoint, uint8_t chunk_idx);
static void zb_nwksrv_ad_start_extended_chunk(s_zb_nwksrv_ad_state_machine_t *state_ptr, uint8_t chunk_idx);
static void zb_nwksrv_ad_proceed_after_identity(s_zb_nwksrv_ad_state_machine_t *state_ptr);
static void zb_nwksrv_send_zcl_color_capabilities_read_req(uint16_t nwk_addr, uint8_t endpoint);
static void zb_nwksrv_send_zcl_ias_zone_write_attr_ias_cie_addr_req(uint16_t nwk_addr, uint8_t endpoint);
static void zb_nwksrv_send_zcl_ias_zone_enroll_response(uint16_t nwk_addr, uint8_t endpoint, uint8_t zone_id, uint8_t resp_code);
static void zb_nwksrv_send_zcl_ias_zone_enroll_checking_req(uint16_t nwk_addr, uint8_t endpoint);

/**
 * @brief Global and local variable for device state machine
 * 
 */
/* State machine data */
static s_zb_nwksrv_ad_state_machine_t g_zb_nwksrv_ad_state_machine[ZB_NWKSRV_MAX_STATE_MACHINES];
/* Timer handle for state machine */
static zb_os_timer_t g_zb_nwksrv_timer;
/* Event flags handle for state machine */
static zb_os_event_t g_zb_nwksrv_event_group;
static uint32_t SM_TIMER_TRIGGER_BIT = (1u << 0);
/* Queue handle for processing incomming event (app/zboss) */
static zb_os_queue_t g_zb_nwksrv_event_queue;

/**
 * Global function definition
 */
bool
zb_nwksrv_is_commissioning(void)
{
    /* A slot is idle only in the AVAILABLE state; anything else means a device
     * is being interviewed. Low-priority diagnostics (e.g. Mgmt_Lqi) should
     * hold off while this is true so they don't back up the ZNP's over-air
     * transaction queue and expire an interview frame (ZMacTransactionExpired). */
    for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
    {
        if (g_zb_nwksrv_ad_state_machine[i].state != ZB_NWKSRV_AD_STATE_AVAILABLE)
        {
            return true;
        }
    }
    return false;
}

int
zb_nwksrv_init(void)
{
    g_zb_nwksrv_event_group = zb_os_event_create();
    if (!g_zb_nwksrv_event_group)
    {
        ZB_LOGE(TAG, "Event group create failed");
        return ZB_FAIL;
    }
    g_zb_nwksrv_event_queue = zb_os_queue_create(ZB_NWKSRV_EVENT_QUEUE_LENGTH, sizeof(s_zb_nwksrv_event_element_t));
    if (!g_zb_nwksrv_event_queue)
    {
        ZB_LOGE(TAG, "Event queue create failed");
        return ZB_FAIL;
    }
    g_zb_nwksrv_timer = zb_os_timer_create("Zigbee_timer", ZB_NWKSRV_ONE_TICK_TIME,
        true, zb_nwksrv_timer_callback, NULL);
    if (!g_zb_nwksrv_timer)
    {
        ZB_LOGE(TAG, "Timer create failed");
        return ZB_FAIL;
    }
    if (!zb_os_timer_start(g_zb_nwksrv_timer))
    {
        ZB_LOGE(TAG, "Timer start failed");
        return ZB_FAIL;
    }

    return ZB_OK;
}

int
zb_nwksrv_deinit(void)
{
    return ZB_OK;
}

int
zb_nwksrv_event_queue_put(s_zb_nwksrv_event_element_t *element)
{
    if (!g_zb_nwksrv_event_queue)
    {
        ZB_LOGE(TAG, "Event data queue NULL");
        return ZB_FAIL;
    }
    if (!zb_os_queue_send(g_zb_nwksrv_event_queue, element, 1))
    {
        ZB_LOGE(TAG, "Send data to event queue failed");
        return ZB_FAIL;
    }
    return ZB_OK;
}

void
zb_nwksrv_task(void)
{
    uint32_t event_bits = 0;
    s_zb_nwksrv_event_element_t element = {};

    if (zb_os_queue_recv(g_zb_nwksrv_event_queue, &element, ZB_OSAL_NO_WAIT))
    {
        switch (element.event_source)
        {
        case ZB_NWKSRV_EVENT_SOURCE_DEVICE_UPDATE:
            zb_nwksrv_ad_process_device_update_handler(&element);
            break;
        case ZB_NWKSRV_EVENT_SOURCE_DEVICE_REMOVE:
            zb_nwksrv_ad_process_device_remove_handler(&element);
            break;
        case ZB_NWKSRV_EVENT_SOURCE_DEVICE_ANNCE:
            zb_nwksrv_ad_process_device_announce_handler(&element);
            break;
        case ZB_NWKSRV_EVENT_SOURCE_IEEE_ADDR:
            zb_nwksrv_ad_process_ieee_addr_resp_handler(&element);
            break;
        case ZB_NWKSRV_EVENT_SOURCE_NWK_ADDR:
            zb_nwksrv_ad_process_nwk_addr_resp_handler(&element);
            break;
        case ZB_NWKSRV_EVENT_SOURCE_NODE_DESC:
            zb_nwksrv_ad_process_node_desc_resp_handler(&element);
            break;
        case ZB_NWKSRV_EVENT_SOURCE_ACTIVE_EP:
            zb_nwksrv_ad_process_active_endpoint_resp_handler(&element);
            break;
        case ZB_NWKSRV_EVENT_SOURCE_SIMPLE_DESC:
            zb_nwksrv_ad_process_simple_desc_resp_handler(&element);
            break;
        default:
            ZB_LOGE(TAG, "Unsupported event source %d", element.event_source);
            break;
        }
    }

    event_bits = zb_os_event_wait(g_zb_nwksrv_event_group, SM_TIMER_TRIGGER_BIT, true, true, ZB_OSAL_NO_WAIT);
    /* Process timer triggered event */
    if (event_bits & SM_TIMER_TRIGGER_BIT)
    {
        for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
        {
            if (g_zb_nwksrv_ad_state_machine[i].state)
            {
                if (g_zb_nwksrv_ad_state_machine[i].timeout < 2)
                {
                    zb_nwksrv_ad_process_state_machine(&g_zb_nwksrv_ad_state_machine[i]);
                }
                else
                {
                    --g_zb_nwksrv_ad_state_machine[i].timeout;
                }
            }
        }
    }
}

/**
 * @brief Adds a device to the database. Will make a series of Zigbee calls to find out all of 
 *        the device information
 * 
 * @param ieee_addr     address of the node
 * @param tc_dev_ind    pointer to zb_znp_mt_zdo_tc_device_ind_t, or NULL if Maintenance request
 * @param pfn_add_device_cb called once the process is complete (or failed)
 */
void
zb_nwksrv_add_device(uint64_t ieee_addr, s_zb_znp_mt_zdo_tc_device_ind_t *tc_dev_ind,
    pfn_zb_nwksrv_add_device_cb pfn_add_device_cb)
{
    s_zb_nwksrv_ad_state_machine_t *state;

    ZB_LOGI(TAG, "%s(): State machine started on %llx", __func__, ieee_addr);
    
    if (tc_dev_ind)
    {
        ZB_LOGI(TAG, "New node %04x", tc_dev_ind->src_addr);
    }
    else
    {
        ZB_LOGI(TAG, "Refresh");
    }

    /* If we already have a state machine, reuse it. If not, get a new one */
    state = zb_nwksrv_ad_reuse_state_machine(ieee_addr);
    if (!state)
    {
        state = zb_nwksrv_ad_get_state_machine();
    }

    /* No state machine, give up! */
    if (!state)
    {
        if (pfn_add_device_cb)
        {
            (*pfn_add_device_cb)(NULL, ieee_addr, ZB_NWKSRV_ADS_NO_MEM);
        }
        return;
    }

    /* Setup state machine common items */
    state->device_info->ieee_addr = ieee_addr;
    state->pfn_add_device_cb = pfn_add_device_cb;
    zb_nwksrv_reset_tries_count(state);
    zb_nwksrv_reset_timeout(state);

    if (tc_dev_ind)
    {
        state->device_info->nwk_addr = tc_dev_ind->src_addr;
        state->device_info->parent_nwk_addr = tc_dev_ind->parent_addr;
        /* Waiting for device announcement */
        state->state = ZB_NWKSRV_AD_STATE_WAITING;
    }
    else
    {
        /* Have ieee_addr, get nwk_addr */    
        state->state = ZB_NWKSRV_AD_STATE_GETTING_NETWORK_ADDR;
        /* Send request directly to ZNP */
        zb_zdo_send_nwk_addr_req(ieee_addr);
    }
}

/**
 * @brief Called when device discovery state machine completed
 * 
 * @param device_info - passed device information pointer
 * @param err - state machine error codes
 */
void
zb_nwksrv_add_device_info_cb(s_zb_device_info_t *device_info, uint64_t ieee_addr, int err)
{
    ZB_LOGI(TAG, "%s(): Complete state machine with err %d for node %llx", __func__, err, ieee_addr);

    if (err == ZB_NWKSRV_ADS_OK)
    {
        s_zb_device_t *device = zb_device_manager_find_by_ieee(device_info->ieee_addr);
        if (device)
        {
            zb_device_manager_on_device_rejoined(device, device_info->nwk_addr, device_info->parent_nwk_addr);
        }
        else
        {
            if (zb_device_manager_add_device(device_info) == ZB_OK)
            {
                device = zb_device_manager_find_by_ieee(device_info->ieee_addr);
                if (zb_device_manager_save_device_file(device) != ZB_OK)
                {
                    ZB_LOGE(TAG, "%s() failed to save new device file", __func__);
                }
            }
            else
            {
                ZB_LOGE(TAG, "%s() failed to add device to database", __func__);
            }
        }
    }
    else if (err == ZB_NWKSRV_ADS_NO_RSP && !zb_device_manager_find_by_ieee(device_info->ieee_addr))
    {
        zb_nwksrv_ad_process_force_device_leave_network(device_info);
    }
}

/**
 * @brief Find the state machine by ieee_addr and clear timeout entry
 * 
 * @param ieee_addr address of device
 */
void
zb_nwksrv_ad_process_device_announce_signal(const uint64_t ieee_addr)
{
    uint64_t *ieee_addr_copy = (uint64_t *)ZB_MEM_MALLOC(sizeof(uint64_t));
    if (!ieee_addr_copy)
    {
        ZB_LOGE(TAG, "%s() failed no mem", __func__);
        return;
    }
    *ieee_addr_copy = ieee_addr;
    s_zb_nwksrv_event_element_t element = {};
    element.event_source = ZB_NWKSRV_EVENT_SOURCE_DEVICE_ANNCE;
    element.response = ieee_addr_copy;
    if (!zb_os_queue_send(g_zb_nwksrv_event_queue, &element, 1))
    {
        ZB_MEM_FREE(ieee_addr_copy);
        ZB_LOGE(TAG, "%s() failed send queue", __func__);
    }
}

static void
zb_nwksrv_ad_process_device_announce_handler(s_zb_nwksrv_event_element_t *element)
{
    uint64_t *ieee_addr = (uint64_t *)element->response;
    for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
    {
        if (g_zb_nwksrv_ad_state_machine[i].state == ZB_NWKSRV_AD_STATE_WAITING &&
            g_zb_nwksrv_ad_state_machine[i].device_info->ieee_addr == *ieee_addr)
        {
            zb_nwksrv_ad_process_state_machine(&g_zb_nwksrv_ad_state_machine[i]);
            break;
        }
    }
    ZB_MEM_FREE(ieee_addr);
}

/**
 * @brief Add new state machine for joining device to process device information discovery
 * 
 * @param dev_info - pointer to device info that contains ieee and network address address
 */
void
zb_nwksrv_ad_process_device_update_signal(s_zb_znp_mt_zdo_tc_device_ind_t *tc_dev_ind)
{
    s_zb_znp_mt_zdo_tc_device_ind_t *tc_dev_ind_copy =
        (s_zb_znp_mt_zdo_tc_device_ind_t *)ZB_MEM_MALLOC(sizeof(s_zb_znp_mt_zdo_tc_device_ind_t));
    if (tc_dev_ind_copy)
    {
        memcpy(tc_dev_ind_copy, tc_dev_ind, sizeof(s_zb_znp_mt_zdo_tc_device_ind_t));
        s_zb_nwksrv_event_element_t element = {};
        element.event_source = ZB_NWKSRV_EVENT_SOURCE_DEVICE_UPDATE;
        element.response = tc_dev_ind_copy;
        if (!zb_os_queue_send(g_zb_nwksrv_event_queue, &element, 1))
        {
            ZB_MEM_FREE(tc_dev_ind_copy);
            ZB_LOGE(TAG, "%s() failed send queue", __func__);
        }
    }
    else
    {
        ZB_LOGE(TAG, "%s() failed no mem", __func__);
    }
}

/**
 * @brief Remove device from device list
 * 
 * @param leave_ind - pointer to leave indication
 */
void
zb_nwksrv_ad_process_device_remove_signal(s_zb_znp_mt_zdo_leave_ind_t *leave_ind)
{
    s_zb_znp_mt_zdo_leave_ind_t *leave_ind_copy =
        (s_zb_znp_mt_zdo_leave_ind_t *)ZB_MEM_MALLOC(sizeof(s_zb_znp_mt_zdo_leave_ind_t));
    if (leave_ind_copy)
    {
        memcpy(leave_ind_copy, leave_ind, sizeof(s_zb_znp_mt_zdo_leave_ind_t));
        s_zb_nwksrv_event_element_t element = {};
        element.event_source = ZB_NWKSRV_EVENT_SOURCE_DEVICE_REMOVE;
        element.response = leave_ind_copy;
        if (!zb_os_queue_send(g_zb_nwksrv_event_queue, &element, 1))
        {
            ZB_MEM_FREE(leave_ind_copy);
            ZB_LOGE(TAG, "%s() failed send queue", __func__);
        }
    }
    else
    {
        ZB_LOGE(TAG, "%s() failed no mem", __func__);
    }
}

static void
zb_nwksrv_ad_process_device_remove_handler(s_zb_nwksrv_event_element_t *element)
{
    s_zb_znp_mt_zdo_leave_ind_t *leave_ind = (s_zb_znp_mt_zdo_leave_ind_t *)element->response;

    /* Device leave network, remove device from device list */
    zb_device_manager_remove_device(leave_ind->ext_addr);
    ZB_MEM_FREE(leave_ind);
}

static void
zb_nwksrv_ad_process_device_update_handler(s_zb_nwksrv_event_element_t *element)
{
    s_zb_znp_mt_zdo_tc_device_ind_t *tc_dev_ind = (s_zb_znp_mt_zdo_tc_device_ind_t *)element->response;
    s_zb_device_t *device = zb_device_manager_find_by_ieee(tc_dev_ind->src_ieee_addr);
    if (device)
    {
        zb_device_manager_on_device_rejoined(device, tc_dev_ind->src_addr, tc_dev_ind->parent_addr);
    }
    else
    {
        /* New device joins the network */
        ZB_LOGI(TAG, "%s() add new device to network", __func__);
        zb_nwksrv_add_device(tc_dev_ind->src_ieee_addr, tc_dev_ind, zb_nwksrv_add_device_info_cb);
    }
    ZB_MEM_FREE(tc_dev_ind);
}

static void
zb_nwksrv_ad_cache_report(s_zb_device_info_t *device_info, uint8_t endpoint, uint16_t cluster_id,
    const s_zb_zcl_report_attr_cmd_t *report)
{
    for (uint8_t r = 0; r < report->num_attr; r++)
    {
        const s_zb_zcl_report_attr_info_t *rec = &report->attr_list[r];
        uint16_t len = zb_zcl_get_attr_data_len(rec->data_type, rec->attr_data);
        s_zb_cached_attr_t *slot = NULL;

        if (len == 0 || len > ZB_CACHED_ATTR_MAX_LEN)
        {
            /* len == 0 means an unsupported data type (no derivable length);
             * len > max means a value too big to cache (long strings). Skip
             * rather than truncate, which would seed a wrong value. Short
             * strings that do fit are stored verbatim, length prefix included,
             * so the handler sees exactly the bytes a live report would give. */
            continue;
        }

        /* Latest-wins on (endpoint, cluster, attr): keeps the table bounded and
         * always holding the freshest value for a chatty device. */
        for (uint8_t k = 0; k < device_info->cached_attr_count; k++)
        {
            if (device_info->cached_attrs[k].endpoint == endpoint &&
                device_info->cached_attrs[k].cluster_id == cluster_id &&
                device_info->cached_attrs[k].attr_id == rec->attr_id)
            {
                slot = &device_info->cached_attrs[k];
                break;
            }
        }

        if (slot == NULL)
        {
            if (device_info->cached_attr_count >= ZB_MAX_CACHED_ATTRS)
            {
                ZB_LOGW(TAG, "Report cache full for node 0x%x, dropping attr 0x%04x",
                    device_info->nwk_addr, rec->attr_id);
                continue;
            }
            slot = &device_info->cached_attrs[device_info->cached_attr_count++];
        }

        slot->endpoint = endpoint;
        slot->cluster_id = cluster_id;
        slot->attr_id = rec->attr_id;
        slot->data_type = rec->data_type;
        slot->len = (uint8_t)len;
        memcpy(slot->value, rec->attr_data, len);

        ZB_LOGI(TAG, "Cached report from node 0x%x during interview: ep=%d cluster=0x%04x attr=0x%04x len=%d",
            device_info->nwk_addr, endpoint, cluster_id, rec->attr_id, slot->len);
    }
}

e_zb_zcl_observe_t
zb_nwksrv_zcl_observe(s_zb_zcl_incoming_msg_t *msg)
{
    if (msg == NULL || msg->msg == NULL || msg->attr_cmd == NULL)
    {
        return ZB_ZCL_OBS_IGNORED;
    }

    /* Only 16-bit-addressed frames can be matched against a state machine,
     * which tracks devices by nwk_addr. */
    if (msg->msg->src_addr.address_mode != AF_ADDRESS_16BIT)
    {
        return ZB_ZCL_OBS_IGNORED;
    }

    switch (msg->hdr.command_id)
    {
        case ZCL_CMD_READ_RSP:
            switch (msg->msg->cluster_id)
            {
                case ZCL_CLUSTER_ID_GENERAL_BASIC:
                    return zb_nwksrv_ad_process_read_basic_attr_resp_handler(msg);
                case ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL:
                    return zb_nwksrv_ad_process_read_color_attr_resp_handler(msg);
                case ZCL_CLUSTER_ID_SS_IAS_ZONE:
                    return zb_nwksrv_ad_process_read_ias_zone_attr_resp_handler(msg);
                default:
                    /* Not an interview cluster - let the normal path have it. */
                    return ZB_ZCL_OBS_IGNORED;
            }

        case ZCL_CMD_WRITE_RSP:
            if (msg->msg->cluster_id == ZCL_CLUSTER_ID_SS_IAS_ZONE)
            {
                return zb_nwksrv_ad_process_write_ias_zone_attr_resp_handler(msg);
            }
            return ZB_ZCL_OBS_IGNORED;

        case ZCL_CMD_REPORT:
        {
            /* A device being interviewed has no pool entry and no functions yet,
             * so the normal path would drop this. Cache the values on the state
             * machine's device_info; they are seeded into the functions at
             * add_device() time. Still returns OBSERVED so the frame continues
             * down the normal path (harmless if the device is already known). */
            for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
            {
                s_zb_nwksrv_ad_state_machine_t *sm = &g_zb_nwksrv_ad_state_machine[i];
                if (sm->state == ZB_NWKSRV_AD_STATE_AVAILABLE || sm->device_info == NULL)
                {
                    continue;
                }
                if (msg->msg->src_addr.short_addr != sm->device_info->nwk_addr)
                {
                    continue;
                }
                zb_nwksrv_ad_cache_report(sm->device_info, msg->msg->src_endpoint,
                    msg->msg->cluster_id, (const s_zb_zcl_report_attr_cmd_t *)msg->attr_cmd);
                return ZB_ZCL_OBS_OBSERVED;
            }
            return ZB_ZCL_OBS_IGNORED;
        }

        default:
            return ZB_ZCL_OBS_IGNORED;
    }
}

/**
 * Local function definition
 */

/**
 * @brief Update timer ticks for all inprogress state machine
 * 
 */
static void
zb_nwksrv_timer_callback(zb_os_timer_t timer, void *arg)
{
    (void)timer;
    (void)arg;
    zb_os_event_set(g_zb_nwksrv_event_group, SM_TIMER_TRIGGER_BIT);
}

/**
 * @brief Reset state timeout for the next request
 * 
 */
static void
zb_nwksrv_reset_timeout(s_zb_nwksrv_ad_state_machine_t *state)
{
    state->timeout = ZB_NWKSRV_WAITING_TICKS;
}

/**
 * @brief Reset state tries count for the next request
 * 
 */
static void
zb_nwksrv_reset_tries_count(s_zb_nwksrv_ad_state_machine_t *state)
{
    state->tries = ZB_NWKSRV_MAX_FAILED_ATTEMPTS - 1;
}

/**
 * @brief The current state has timed out. Either retry, or exit state machine with error
 * 
 * @param state     pointer to state machine  
 */
static void
zb_nwksrv_ad_process_state_machine(s_zb_nwksrv_ad_state_machine_t *state)
{
    if (!state || state->state == ZB_NWKSRV_AD_STATE_AVAILABLE)
    {
        return;
    }
    ZB_LOGW(TAG, "%s(): Node %04x, state %d, remaining tries: %d", __func__,
        state->device_info->nwk_addr, state->state, state->tries);

    if (state->tries)
    {
        --state->tries;
        zb_nwksrv_reset_timeout(state);

        /* Calling zboss api require lock for thread safe */
        switch (state->state)
        {
            case ZB_NWKSRV_AD_STATE_WAITING:
                /* Go on to getting node info */
                // state->state = ZB_NWKSRV_AD_STATE_GETTING_NODE_INFO;
                state->state = ZB_NWKSRV_AD_STATE_GETTING_PARENT_IEEE_ADDR;
                zb_nwksrv_reset_tries_count(state);
                // zb_zdo_send_node_desc_req(state->device_info->nwk_addr);
                zb_zdo_send_ieee_addr_req(state->device_info->parent_nwk_addr);
                break;
            case ZB_NWKSRV_AD_STATE_GETTING_NETWORK_ADDR:
                zb_zdo_send_nwk_addr_req(state->device_info->ieee_addr);
                break;
            case ZB_NWKSRV_AD_STATE_GETTING_PARENT_IEEE_ADDR:
                zb_zdo_send_ieee_addr_req(state->device_info->parent_nwk_addr);
                break;
            case ZB_NWKSRV_AD_STATE_GETTING_NODE_INFO:
                zb_zdo_send_node_desc_req(state->device_info->nwk_addr);
                break;
            case ZB_NWKSRV_AD_STATE_GETTING_ACTIVE_ENDPOINT:
                zb_zdo_send_active_endpoint_req(state->device_info->nwk_addr);
                break;
            case ZB_NWKSRV_AD_STATE_GETTING_SIMPLE_DESC:
                zb_zdo_send_simple_desc_req(state->device_info->nwk_addr,
                                                   state->device_info->endpoints[state->ep].endpoint_id);
                break;
            case ZB_NWKSRV_AD_STATE_GETTING_IAS_ZONE_STATUS:
                zb_nwksrv_send_zcl_ias_zone_enroll_checking_req(state->device_info->nwk_addr,
                                                                state->device_info->endpoints[0].endpoint_id);
                break;
            case ZB_NWKSRV_AD_STATE_WRITING_IAS_CIE_ADDRESS:
                zb_nwksrv_send_zcl_ias_zone_write_attr_ias_cie_addr_req(state->device_info->nwk_addr,
                                                                        state->device_info->endpoints[0].endpoint_id);
                break;
            case ZB_NWKSRV_AD_STATE_IAS_ZONE_ENROLL_CHECKING:
                zb_nwksrv_send_zcl_ias_zone_enroll_checking_req(state->device_info->nwk_addr,
                                                                state->device_info->endpoints[0].endpoint_id);
                break;
            case ZB_NWKSRV_AD_STATE_GETTING_BASIC_INFO:
                zb_nwksrv_send_zcl_basic_info_read_req(state->device_info->nwk_addr,
                                                                state->device_info->endpoints[0].endpoint_id);
                break;
            case ZB_NWKSRV_AD_STATE_GETTING_EXTENDED_INFO:
                /* Normally tries==0 so we never retry here (we fail open below),
                 * but honour a retry if one was configured - resend this chunk. */
                zb_nwksrv_ad_start_extended_chunk(state, state->ext_chunk);
                break;
            case ZB_NWKSRV_AD_STATE_GETTING_COLOR_CAPABILITIES:
                zb_nwksrv_send_zcl_color_capabilities_read_req(state->device_info->nwk_addr,
                                                                state->device_info->endpoints[0].endpoint_id);
                break;
            default:
                break;
        }
    }
    else if (state->state == ZB_NWKSRV_AD_STATE_IAS_ZONE_ENROLL_CHECKING)
    {
        /* Presume device has enrolled sucessfully, move to next state */
        zb_nwksrv_reset_timeout(state);
        zb_nwksrv_reset_tries_count(state);
        state->state = ZB_NWKSRV_AD_STATE_GETTING_BASIC_INFO;
        zb_nwksrv_send_zcl_basic_info_read_req(state->device_info->nwk_addr, state->device_info->endpoints[0].endpoint_id);
    }
    else if (state->state == ZB_NWKSRV_AD_STATE_GETTING_EXTENDED_INFO)
    {
        /* Extended identity is best-effort and read chunk by chunk. A chunk the
         * device didn't (or couldn't) answer must NOT abandon the rest: skip it
         * and try the next chunk so we still collect whatever the device does
         * return. Continue the interview only after the last chunk. */
        ZB_LOGI(TAG, "AddDevice: node %llx extended chunk %u no response, trying next",
                        state->device_info->ieee_addr, (unsigned)state->ext_chunk);
        uint8_t next_chunk = state->ext_chunk + 1;
        if (next_chunk < ZB_NWKSRV_EXTENDED_INFO_CHUNK_COUNT)
        {
            zb_nwksrv_ad_start_extended_chunk(state, next_chunk);
        }
        else
        {
            zb_nwksrv_ad_proceed_after_identity(state);
        }
    }
    else
    {
        ZB_LOGW(TAG, "AddDevice: Failed to get response from target node %llx, state %d",
                        state->device_info->ieee_addr, state->state);
        if (state->pfn_add_device_cb)
        {
            state->pfn_add_device_cb(state->device_info, state->device_info->ieee_addr, ZB_NWKSRV_ADS_NO_RSP);
        }
        zb_nwksrv_ad_free_state_machine(state);
    }
}

/**
 * @brief Allocates a state machine entry, including the Device info
 * 
 * @return Pointer to state machine, ready to be activated
 */
static s_zb_nwksrv_ad_state_machine_t*
zb_nwksrv_ad_get_state_machine(void)
{
    s_zb_nwksrv_ad_state_machine_t *state = NULL;

    /* Find a ZB_MEM_FREE entry */
    for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
    {
        if (g_zb_nwksrv_ad_state_machine[i].state == ZB_NWKSRV_AD_STATE_AVAILABLE)
        {
            ZB_LOGI(TAG, "%s(): Initiated new state machine", __func__);
            state = &g_zb_nwksrv_ad_state_machine[i];
            break;
        }
    }

    /* Found an entry, use it */
    if (state == NULL)
    {
        ZB_LOGE(TAG, "%s(): Error - State machine full. max count %d", __func__, ZB_NWKSRV_MAX_STATE_MACHINES);
        return NULL;
    }

    /* Found a ZB_MEM_FREE state machine. Also, allocate a device info for this entry */
    state->device_info = (s_zb_device_info_t *)ZB_MEM_MALLOC(sizeof(s_zb_device_info_t));
    if (!state->device_info)
    {
        ZB_LOGE(TAG, "%s(): Error - no memory for device info", __func__);
        return NULL;
    }

    memset(state->device_info, 0, sizeof(s_zb_device_info_t));
    return state;
}

/**
 * @brief Finds and reuse the state machine by ieee addr
 * 
 * @param ieee_addr device address
 * @return A pointer to state machine if found or NULL
 */
static s_zb_nwksrv_ad_state_machine_t*
zb_nwksrv_ad_reuse_state_machine(uint64_t ieee_addr)
{
    /* Find the state machine based on the ieeeAddr */
    for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
    {
        if (g_zb_nwksrv_ad_state_machine[i].state != ZB_NWKSRV_AD_STATE_AVAILABLE &&
            g_zb_nwksrv_ad_state_machine[i].device_info->ieee_addr == ieee_addr)
        {
            ZB_LOGI(TAG, "%s(): Restarting State Machine of node %llx", __func__, ieee_addr);

            /* Free the state machine */
            zb_nwksrv_ad_free_state_machine(&g_zb_nwksrv_ad_state_machine[i]);

            /* Add a new one (just like from scratch)*/
            return zb_nwksrv_ad_get_state_machine();
        }
    }
    return NULL;
}

/**
 * @brief Free the state machine and the device_info
 * 
 * @param state - pointer to specific state machine
 */
static void
zb_nwksrv_ad_free_state_machine(s_zb_nwksrv_ad_state_machine_t *state)
{
    ZB_LOGI(TAG, "%s(): State machine free for node %llx", __func__, state->device_info->ieee_addr);
    /* Reset state machine to scratch */
    memset(state, 0, sizeof(s_zb_nwksrv_ad_state_machine_t));
}

/**
 * @brief 
 * 
 */
static void
zb_nwksrv_ad_process_ieee_addr_resp_handler(s_zb_nwksrv_event_element_t *element)
{
    s_zb_znp_mt_zdo_ieee_addr_rsp_t *ieee_addr_resp = (s_zb_znp_mt_zdo_ieee_addr_rsp_t *)element->response;

    for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
    {
        if (g_zb_nwksrv_ad_state_machine[i].state == ZB_NWKSRV_AD_STATE_GETTING_PARENT_IEEE_ADDR &&
            g_zb_nwksrv_ad_state_machine[i].device_info->parent_nwk_addr == ieee_addr_resp->nwk_addr)
        {
            ZB_LOGI(TAG, "Receive ieee_addr_resp parent ieee addr %llx of node %04x" , ieee_addr_resp->ieee_addr, ieee_addr_resp->nwk_addr);

            g_zb_nwksrv_ad_state_machine[i].device_info->parent_ieee = ieee_addr_resp->ieee_addr;
            /* Reset the timer, we got the response */
            zb_nwksrv_reset_timeout(&g_zb_nwksrv_ad_state_machine[i]);
            zb_nwksrv_reset_tries_count(&g_zb_nwksrv_ad_state_machine[i]);

            /* Move on to getting node info */
            g_zb_nwksrv_ad_state_machine[i].state = ZB_NWKSRV_AD_STATE_GETTING_NODE_INFO;
            zb_zdo_send_node_desc_req(g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr);
            break;
        }
    }
    ZB_MEM_FREE(ieee_addr_resp);
}

/**
 * @brief Process a network address response which has been store in event queue
 *
 */
static void
zb_nwksrv_ad_process_nwk_addr_resp_handler(s_zb_nwksrv_event_element_t *element)
{
    s_zb_znp_mt_zdo_nwk_addr_rsp_t *nwk_addr_resp = (s_zb_znp_mt_zdo_nwk_addr_rsp_t *)element->response;

    for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
    {
        if (g_zb_nwksrv_ad_state_machine[i].state == ZB_NWKSRV_AD_STATE_GETTING_NETWORK_ADDR &&
            g_zb_nwksrv_ad_state_machine[i].device_info->ieee_addr == nwk_addr_resp->ieee_addr)
        {
            ZB_LOGI(TAG, "Receive nwk_addr_resp addr %04x of node %llx" , nwk_addr_resp->nwk_addr, nwk_addr_resp->ieee_addr);

            g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr = nwk_addr_resp->nwk_addr;
            /* Reset the timer, we got the response */
            zb_nwksrv_reset_timeout(&g_zb_nwksrv_ad_state_machine[i]);
            zb_nwksrv_reset_tries_count(&g_zb_nwksrv_ad_state_machine[i]);

            /* Device short address updated, we can wrap up the state machine */
            zb_nwksrv_ad_wrap_up(&g_zb_nwksrv_ad_state_machine[i]);
            break;
        }
    }
    ZB_MEM_FREE(nwk_addr_resp);
}

static void
zb_nwksrv_ad_process_node_desc_resp_handler(s_zb_nwksrv_event_element_t *element)
{
    s_zb_znp_mt_zdo_node_desc_rsp_t *node_desc_resp = (s_zb_znp_mt_zdo_node_desc_rsp_t *)element->response;

    for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
    {
        if (g_zb_nwksrv_ad_state_machine[i].state == ZB_NWKSRV_AD_STATE_GETTING_NODE_INFO &&
            g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr == node_desc_resp->nwk_addr)
        {
            ZB_LOGI(TAG, "Receive node_desc_resp from node %04x status %d", node_desc_resp->nwk_addr, node_desc_resp->status);
            zb_nwksrv_reset_timeout(&g_zb_nwksrv_ad_state_machine[i]);
            if (node_desc_resp->status == ZB_SUCCESS)
            {
                g_zb_nwksrv_ad_state_machine[i].device_info->manu_id = node_desc_resp->manufacturer_code;
                g_zb_nwksrv_ad_state_machine[i].device_info->device_type = node_desc_resp->logical_type;
                g_zb_nwksrv_ad_state_machine[i].device_info->battery_powered = (node_desc_resp->mac_cap_flg & ZB_DEVICE_CAP_BATTERY_POWER_MASK) ? false : true;
                g_zb_nwksrv_ad_state_machine[i].device_info->sleepy_enabled = (node_desc_resp->mac_cap_flg & ZB_DEVICE_CAP_SLEEPY_MASK) ? false : true;
                ZB_LOGI(TAG, "Node %04x manu_id %04x device_type %d battery_powered %d sleepy_enabled %d",
                    node_desc_resp->nwk_addr,
                    g_zb_nwksrv_ad_state_machine[i].device_info->manu_id,
                    g_zb_nwksrv_ad_state_machine[i].device_info->device_type,
                    g_zb_nwksrv_ad_state_machine[i].device_info->battery_powered,
                    g_zb_nwksrv_ad_state_machine[i].device_info->sleepy_enabled);
            }
            /* Move on to getting Active enpoint */
            zb_nwksrv_reset_tries_count(&g_zb_nwksrv_ad_state_machine[i]);
            g_zb_nwksrv_ad_state_machine[i].state = ZB_NWKSRV_AD_STATE_GETTING_ACTIVE_ENDPOINT;
            zb_zdo_send_active_endpoint_req(g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr);
            break;
        }
    }
    ZB_MEM_FREE(node_desc_resp);
}

static void
zb_nwksrv_ad_process_active_endpoint_resp_handler(s_zb_nwksrv_event_element_t *element)
{
    s_zb_znp_mt_zdo_active_ep_rsp_t *active_ep_resp = (s_zb_znp_mt_zdo_active_ep_rsp_t *)element->response;
    s_zb_device_info_t *device_info;

    for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
    {
        if (g_zb_nwksrv_ad_state_machine[i].state == ZB_NWKSRV_AD_STATE_GETTING_ACTIVE_ENDPOINT &&
            g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr == active_ep_resp->nwk_addr)
        {
            ZB_LOGI(TAG, "Receive active_endpoint_resp from %04x", active_ep_resp->nwk_addr);

            zb_nwksrv_reset_timeout(&g_zb_nwksrv_ad_state_machine[i]);
            device_info = g_zb_nwksrv_ad_state_machine[i].device_info;
            device_info->endpoint_count = active_ep_resp->active_ep_count;
            if (device_info->endpoint_count)
            {
                memset(device_info->endpoints, 0, device_info->endpoint_count * sizeof(s_zb_device_endpoint_t));

                for (int ep = 0; ep < device_info->endpoint_count; ++ep)
                {
                    ZB_LOGI(TAG, "ep %d", active_ep_resp->active_ep_list[ep]);
                    device_info->endpoints[ep].endpoint_id = active_ep_resp->active_ep_list[ep];
                }

                /* Move on to getting Simple descriptors */
                g_zb_nwksrv_ad_state_machine[i].ep = 0;     /* For counting endpoints in the next state */
                zb_nwksrv_reset_tries_count(&g_zb_nwksrv_ad_state_machine[i]);
                g_zb_nwksrv_ad_state_machine[i].state = ZB_NWKSRV_AD_STATE_GETTING_SIMPLE_DESC;

                /* Try to get the first one */
                zb_zdo_send_simple_desc_req(device_info->nwk_addr, device_info->endpoints[0].endpoint_id);
            }
            /* No active endpoints, go to done */
            else
            {
                // TODO: Should not add this device into device list???
                zb_nwksrv_ad_wrap_up(&g_zb_nwksrv_ad_state_machine[i]);
            }
            break;
        }
    }
    ZB_MEM_FREE(active_ep_resp);
}

static void
zb_nwksrv_ad_process_simple_desc_resp_handler(s_zb_nwksrv_event_element_t *element)
{
    s_zb_znp_mt_zdo_simple_desc_rsp_t *simple_desc_resp = (s_zb_znp_mt_zdo_simple_desc_rsp_t *)element->response;

    s_zb_device_info_t *device_info;
    s_zb_device_endpoint_t *endpoint;
    uint8_t ep_idx;

    ZB_LOGI(TAG, "Receive simple_desc_resp node %04x, endpoint %d, profile_id %04x, device_id %04x",
            simple_desc_resp->nwk_addr, simple_desc_resp->endpoint, simple_desc_resp->profile_id, simple_desc_resp->device_id);

    for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
    {
        if (g_zb_nwksrv_ad_state_machine[i].state == ZB_NWKSRV_AD_STATE_GETTING_SIMPLE_DESC &&
            g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr == simple_desc_resp->nwk_addr)
        {
            zb_nwksrv_reset_timeout(&g_zb_nwksrv_ad_state_machine[i]);

            device_info = g_zb_nwksrv_ad_state_machine[i].device_info;
            ep_idx = g_zb_nwksrv_ad_state_machine[i].ep;
            ZB_LOGI(TAG, "Endpoint %d, ep_idx %d, ep_count %d", simple_desc_resp->endpoint, ep_idx, device_info->endpoint_count);
            endpoint = &device_info->endpoints[ep_idx];
            ZB_LOGI(TAG, "Current endpoint %d", endpoint->endpoint_id);

            endpoint->profile_id = simple_desc_resp->profile_id;
            endpoint->device_id = simple_desc_resp->device_id;
            endpoint->in_cluster_count = simple_desc_resp->num_in_clusters;
            endpoint->out_cluster_count = simple_desc_resp->num_out_clusters;

            ZB_LOGI(TAG, "In clusters %d, out clusters %d", endpoint->in_cluster_count, endpoint->out_cluster_count);
            for (int cluster = 0; cluster < endpoint->in_cluster_count; ++cluster)
            {
                endpoint->in_clusters[cluster] = simple_desc_resp->in_cluster_list[cluster];
                ZB_LOGI(TAG, "CLUSTER_IN: 0x%x", endpoint->in_clusters[cluster]);
            }
            for (int cluster = 0; cluster < endpoint->out_cluster_count; ++cluster)
            {
                endpoint->out_clusters[cluster] = simple_desc_resp->out_cluster_list[cluster];
                ZB_LOGI(TAG, "CLUSTER_OUT: 0x%x", endpoint->out_clusters[cluster]);
            }
            /* On to next endpoint */
            ++g_zb_nwksrv_ad_state_machine[i].ep;
            /* If more endpoints to get, get them now */
            if (g_zb_nwksrv_ad_state_machine[i].ep < device_info->endpoint_count)
            {
                zb_zdo_send_simple_desc_req(device_info->nwk_addr, device_info->endpoints[g_zb_nwksrv_ad_state_machine[i].ep].endpoint_id);
            }
            else
            {
                s_zb_device_endpoint_t *ias_endpoint = zb_device_manager_find_endpoint_by_dev_cluster(g_zb_nwksrv_ad_state_machine[i].device_info, ZCL_CLUSTER_ID_SS_IAS_ZONE);
                s_zb_device_endpoint_t *basic_endpoint = zb_device_manager_find_endpoint_by_dev_cluster(g_zb_nwksrv_ad_state_machine[i].device_info, ZCL_CLUSTER_ID_GENERAL_BASIC);
                zb_nwksrv_reset_tries_count(&g_zb_nwksrv_ad_state_machine[i]);
                /* Check whether device has IAS Zone cluster. if yes, enroll device when needed */
                if (ias_endpoint != NULL)
                {
                    /* Move to next state to get IAS Zone status */
                    g_zb_nwksrv_ad_state_machine[i].state = ZB_NWKSRV_AD_STATE_GETTING_IAS_ZONE_STATUS;
                    zb_nwksrv_send_zcl_ias_zone_enroll_checking_req(g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, ias_endpoint->endpoint_id);
                }
                else if (basic_endpoint != NULL)
                {
                    /* Move to next state to get device model identifier */
                    g_zb_nwksrv_ad_state_machine[i].state = ZB_NWKSRV_AD_STATE_GETTING_BASIC_INFO;
                    zb_nwksrv_send_zcl_basic_info_read_req(device_info->nwk_addr, basic_endpoint->endpoint_id);
                }
                else
                {
                    ZB_LOGW(TAG, "No IAS Zone cluster and no Basic cluster, go to done");
                    /* No IAS Zone cluster and no Basic cluster, go to done */
                    zb_nwksrv_ad_wrap_up(&g_zb_nwksrv_ad_state_machine[i]);
                }
            }
            break;
        }
    }
    ZB_MEM_FREE(simple_desc_resp);
}

static e_zb_zcl_observe_t
zb_nwksrv_ad_process_read_basic_attr_resp_handler(s_zb_zcl_incoming_msg_t *msg)
{
    const s_zb_zcl_read_attr_rsp_cmd_t *rsp = (const s_zb_zcl_read_attr_rsp_cmd_t *)msg->attr_cmd;

    for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
    {
        if (g_zb_nwksrv_ad_state_machine[i].state != ZB_NWKSRV_AD_STATE_GETTING_BASIC_INFO &&
            g_zb_nwksrv_ad_state_machine[i].state != ZB_NWKSRV_AD_STATE_GETTING_EXTENDED_INFO)
        {
            continue;
        }
        s_zb_device_endpoint_t *endpoint = zb_device_manager_find_endpoint_by_dev_cluster(g_zb_nwksrv_ad_state_machine[i].device_info, msg->msg->cluster_id);
        if (endpoint == NULL ||
            msg->msg->src_addr.short_addr != g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr ||
            msg->msg->src_endpoint != endpoint->endpoint_id)
        {
            continue;
        }

        zb_nwksrv_reset_timeout(&g_zb_nwksrv_ad_state_machine[i]);

        for (uint8_t r = 0; r < rsp->num_attr; r++)
        {
            const s_zb_zcl_read_attr_rsp_info_t *attr_resp = &rsp->attr_list[r];
            /* data_type/data are only meaningful when status == SUCCESS; every
             * case below checks status before touching them. */
            const uint8_t *data = attr_resp->data;
            switch (attr_resp->attr_id)
            {
                case ATTRID_BASIC_MODEL_IDENTIFIER:
                    if (attr_resp->status == ZCL_STATUS_SUCCESS && attr_resp->data_type == ZCL_DATATYPE_CHAR_STR)
                    {
                        uint8_t model_len = data[0] < sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->model) ? data[0] : sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->model) - 1;
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->model, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->model));
                        memcpy(g_zb_nwksrv_ad_state_machine[i].device_info->model, data + 1, model_len);
                        ZB_LOGI(TAG, "Receive model of node 0x%x, len %d, model=%s", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr,
                            model_len, g_zb_nwksrv_ad_state_machine[i].device_info->model);
                    }
                    else
                    {
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->model, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->model));
                        ZB_LOGW(TAG, "Receive model of node 0x%x, status 0x%x", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, attr_resp->status);
                    }                    
                    break;
                case ATTRID_BASIC_MANUFACTURER_NAME:
                    if (attr_resp->status == ZCL_STATUS_SUCCESS && attr_resp->data_type == ZCL_DATATYPE_CHAR_STR)
                    {
                        uint8_t manu_len = data[0] < sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->manufacturer) ? data[0] : sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->manufacturer) - 1;
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->manufacturer, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->manufacturer));
                        memcpy(g_zb_nwksrv_ad_state_machine[i].device_info->manufacturer, data + 1, manu_len);
                        ZB_LOGI(TAG, "Receive manufacturer of node 0x%x, len %d, manufacturer=%s", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr,
                            manu_len, g_zb_nwksrv_ad_state_machine[i].device_info->manufacturer);
                    }
                    else
                    {
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->manufacturer, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->manufacturer));
                        ZB_LOGW(TAG, "Receive manufacturer of node 0x%x, status 0x%x", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, attr_resp->status);
                    }                
                    break;
                case ATTRID_BASIC_POWER_SOURCE:
                    if (attr_resp->status == ZCL_STATUS_SUCCESS && zb_zcl_get_data_type_length(attr_resp->data_type) == 1)
                    {
                        g_zb_nwksrv_ad_state_machine[i].device_info->power_source = data[0];
                        ZB_LOGI(TAG, "Receive power source of node 0x%x, power source=%d", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr,
                            g_zb_nwksrv_ad_state_machine[i].device_info->power_source);
                    }
                    else
                    {
                        g_zb_nwksrv_ad_state_machine[i].device_info->power_source = 0;
                        ZB_LOGW(TAG, "Receive power source of node 0x%x, status 0x%x", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, attr_resp->status);
                    }
                    break;
                case ATTRID_BASIC_ZCL_VERSION:
                    if (attr_resp->status == ZCL_STATUS_SUCCESS && zb_zcl_get_data_type_length(attr_resp->data_type) == 1)
                    {
                        g_zb_nwksrv_ad_state_machine[i].device_info->zcl_version = data[0];
                        ZB_LOGI(TAG, "Receive ZCL version of node 0x%x, ZCL version=%d", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr,
                            g_zb_nwksrv_ad_state_machine[i].device_info->zcl_version);
                    }
                    else
                    {
                        g_zb_nwksrv_ad_state_machine[i].device_info->zcl_version = 0;
                        ZB_LOGW(TAG, "Receive ZCL version of node 0x%x, status 0x%x", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, attr_resp->status);
                    }
                    break;
                case ATTRID_BASIC_APPLICATION_VERSION:
                    if (attr_resp->status == ZCL_STATUS_SUCCESS && zb_zcl_get_data_type_length(attr_resp->data_type) == 1)
                    {
                        g_zb_nwksrv_ad_state_machine[i].device_info->app_version = data[0];
                        ZB_LOGI(TAG, "Receive application version of node 0x%x, application version=%d", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr,
                            g_zb_nwksrv_ad_state_machine[i].device_info->app_version);
                    }
                    else
                    {
                        g_zb_nwksrv_ad_state_machine[i].device_info->app_version = 0;
                        ZB_LOGW(TAG, "Receive application version of node 0x%x, status 0x%x", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, attr_resp->status);
                    }
                    break;
                case ATTRID_BASIC_HW_VERSION:
                    if (attr_resp->status == ZCL_STATUS_SUCCESS && zb_zcl_get_data_type_length(attr_resp->data_type) == 1)
                    {
                        g_zb_nwksrv_ad_state_machine[i].device_info->hw_version = data[0];
                        ZB_LOGI(TAG, "Receive HW version of node 0x%x, hw_version=%d", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr,
                            g_zb_nwksrv_ad_state_machine[i].device_info->hw_version);
                    }
                    else
                    {
                        g_zb_nwksrv_ad_state_machine[i].device_info->hw_version = 0;
                        ZB_LOGW(TAG, "Receive HW version of node 0x%x, status 0x%x", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, attr_resp->status);
                    }
                    break;
                case ATTRID_BASIC_DATE_CODE:
                    if (attr_resp->status == ZCL_STATUS_SUCCESS && attr_resp->data_type == ZCL_DATATYPE_CHAR_STR)
                    {
                        uint8_t date_len = data[0] < sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->date_code) ? data[0] : sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->date_code) - 1;
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->date_code, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->date_code));
                        memcpy(g_zb_nwksrv_ad_state_machine[i].device_info->date_code, data + 1, date_len);
                        ZB_LOGI(TAG, "Receive date code of node 0x%x, len %d, date_code=%s", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr,
                            date_len, g_zb_nwksrv_ad_state_machine[i].device_info->date_code);
                    }
                    else
                    {
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->date_code, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->date_code));
                        ZB_LOGW(TAG, "Receive date code of node 0x%x, status 0x%x", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, attr_resp->status);
                    }
                    break;
                case ATTRID_BASIC_PHYSICAL_ENVIRONMENT:
                    if (attr_resp->status == ZCL_STATUS_SUCCESS && zb_zcl_get_data_type_length(attr_resp->data_type) == 1)
                    {
                        g_zb_nwksrv_ad_state_machine[i].device_info->physical_environment = data[0];
                        ZB_LOGI(TAG, "Receive physical environment of node 0x%x, physical environment=%d", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr,
                            g_zb_nwksrv_ad_state_machine[i].device_info->physical_environment);
                    }
                    else
                    {
                        g_zb_nwksrv_ad_state_machine[i].device_info->physical_environment = 0;
                        ZB_LOGW(TAG, "Receive physical environment of node 0x%x, status 0x%x", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, attr_resp->status);
                    }
                    break;
                case ATTRID_BASIC_PRODUCT_CODE:
                    if (attr_resp->status == ZCL_STATUS_SUCCESS && attr_resp->data_type == ZCL_DATATYPE_OCTET_STR)
                    {
                        uint8_t pc_len = data[0] < sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->product_code) ? data[0] : sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->product_code);
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->product_code, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->product_code));
                        memcpy(g_zb_nwksrv_ad_state_machine[i].device_info->product_code, data + 1, pc_len);
                        g_zb_nwksrv_ad_state_machine[i].device_info->product_code_len = pc_len;
                        ZB_LOGI(TAG, "Receive product code of node 0x%x, len %d", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, pc_len);
                    }
                    else
                    {
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->product_code, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->product_code));
                        g_zb_nwksrv_ad_state_machine[i].device_info->product_code_len = 0;
                        ZB_LOGW(TAG, "Receive product code of node 0x%x, status 0x%x", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, attr_resp->status);
                    }
                    break;
                case ATTRID_BASIC_SERIAL_NUMBER:
                    if (attr_resp->status == ZCL_STATUS_SUCCESS && attr_resp->data_type == ZCL_DATATYPE_CHAR_STR)
                    {
                        uint8_t sn_len = data[0] < sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->serial_number) ? data[0] : sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->serial_number) - 1;
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->serial_number, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->serial_number));
                        memcpy(g_zb_nwksrv_ad_state_machine[i].device_info->serial_number, data + 1, sn_len);
                        ZB_LOGI(TAG, "Receive serial number of node 0x%x, len %d, serial=%s", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr,
                            sn_len, g_zb_nwksrv_ad_state_machine[i].device_info->serial_number);
                    }
                    else
                    {
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->serial_number, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->serial_number));
                        ZB_LOGW(TAG, "Receive serial number of node 0x%x, status 0x%x", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, attr_resp->status);
                    }
                    break;
                case ATTRID_BASIC_PRODUCT_LABEL:
                    if (attr_resp->status == ZCL_STATUS_SUCCESS && attr_resp->data_type == ZCL_DATATYPE_CHAR_STR)
                    {
                        uint8_t pl_len = data[0] < sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->product_label) ? data[0] : sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->product_label) - 1;
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->product_label, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->product_label));
                        memcpy(g_zb_nwksrv_ad_state_machine[i].device_info->product_label, data + 1, pl_len);
                        ZB_LOGI(TAG, "Receive product label of node 0x%x, len %d, label=%s", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr,
                            pl_len, g_zb_nwksrv_ad_state_machine[i].device_info->product_label);
                    }
                    else
                    {
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->product_label, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->product_label));
                        ZB_LOGW(TAG, "Receive product label of node 0x%x, status 0x%x", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, attr_resp->status);
                    }
                    break;
                case ATTRID_BASIC_SW_BUILD_ID:
                    if (attr_resp->status == ZCL_STATUS_SUCCESS && attr_resp->data_type == ZCL_DATATYPE_CHAR_STR)
                    {
                        uint8_t sw_len = data[0] < sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->sw_build_id) ? data[0] : sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->sw_build_id) - 1;
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->sw_build_id, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->sw_build_id));
                        memcpy(g_zb_nwksrv_ad_state_machine[i].device_info->sw_build_id, data + 1, sw_len);
                        ZB_LOGI(TAG, "Receive SW build ID of node 0x%x, len %d, sw_build=%s", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr,
                            sw_len, g_zb_nwksrv_ad_state_machine[i].device_info->sw_build_id);
                    }
                    else
                    {
                        memset(g_zb_nwksrv_ad_state_machine[i].device_info->sw_build_id, 0, sizeof(g_zb_nwksrv_ad_state_machine[i].device_info->sw_build_id));
                        ZB_LOGW(TAG, "Receive SW build ID of node 0x%x, status 0x%x", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, attr_resp->status);
                    }
                    break;
                default:
                    break;
            }
        }

        if (g_zb_nwksrv_ad_state_machine[i].state == ZB_NWKSRV_AD_STATE_GETTING_BASIC_INFO)
        {
            /* Essential identity captured. Now do the best-effort extended read,
             * chunked so each response fits one frame. Every chunk is attempted
             * (a silent chunk is skipped, not fatal); the interview continues
             * from zb_nwksrv_ad_proceed_after_identity() after the last chunk,
             * whether answered or timed out. */
            zb_nwksrv_ad_start_extended_chunk(&g_zb_nwksrv_ad_state_machine[i], 0);
        }
        else /* ZB_NWKSRV_AD_STATE_GETTING_EXTENDED_INFO - this chunk answered */
        {
            uint8_t next_chunk = g_zb_nwksrv_ad_state_machine[i].ext_chunk + 1;
            if (next_chunk < ZB_NWKSRV_EXTENDED_INFO_CHUNK_COUNT)
            {
                zb_nwksrv_ad_start_extended_chunk(&g_zb_nwksrv_ad_state_machine[i], next_chunk);
            }
            else
            {
                zb_nwksrv_ad_proceed_after_identity(&g_zb_nwksrv_ad_state_machine[i]);
            }
        }
        return ZB_ZCL_OBS_CONSUMED;
    }
    return ZB_ZCL_OBS_IGNORED;
}

static e_zb_zcl_observe_t
zb_nwksrv_ad_process_read_color_attr_resp_handler(s_zb_zcl_incoming_msg_t *msg)
{
    const s_zb_zcl_read_attr_rsp_cmd_t *rsp = (const s_zb_zcl_read_attr_rsp_cmd_t *)msg->attr_cmd;

    for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
    {
        if (g_zb_nwksrv_ad_state_machine[i].state != ZB_NWKSRV_AD_STATE_GETTING_COLOR_CAPABILITIES)
        {
            continue;
        }
        s_zb_device_endpoint_t *endpoint = zb_device_manager_find_endpoint_by_dev_cluster(g_zb_nwksrv_ad_state_machine[i].device_info, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL);
        if (endpoint == NULL ||
            msg->msg->src_addr.short_addr != g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr ||
            msg->msg->src_endpoint != endpoint->endpoint_id)
        {
            continue;
        }

        zb_nwksrv_reset_timeout(&g_zb_nwksrv_ad_state_machine[i]);
        for (uint8_t r = 0; r < rsp->num_attr; r++)
        {
            const s_zb_zcl_read_attr_rsp_info_t *attr_resp = &rsp->attr_list[r];
            const uint8_t *data = attr_resp->data;
            if (attr_resp->status == ZCL_STATUS_SUCCESS)
            {
                switch (attr_resp->attr_id)
                {
                    case ATTRID_COLOR_CONTROL_CURRENT_HUE:
                        if (zb_zcl_get_data_type_length(attr_resp->data_type) == 1)
                        {
                            ZB_LOGI(TAG, "Receive current hue of node 0x%x, current hue=%d", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, data[0]);
                        }
                        break;
                    case ATTRID_COLOR_CONTROL_CURRENT_SATURATION:
                        if (zb_zcl_get_data_type_length(attr_resp->data_type) == 1)
                        {
                            ZB_LOGI(TAG, "Receive current saturation of node 0x%x, current saturation=%d", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, data[0]);
                        }
                        break;
                    case ATTRID_COLOR_CONTROL_COLOR_TEMPERATURE_MIREDS:
                        if (zb_zcl_get_data_type_length(attr_resp->data_type) == 2)
                        {
                            ZB_LOGI(TAG, "Receive color temperature mireds of node 0x%x, color temperature mireds=%d", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, BUILD_UINT16(data[0], data[1]));
                        }
                        break;
                    case ATTRID_COLOR_CONTROL_COLOR_CAPABILITIES:
                        if (zb_zcl_get_data_type_length(attr_resp->data_type) == 2)
                        {
                            endpoint->color_caps = BUILD_UINT16(data[0], data[1]);
                            endpoint->color_caps_valid = true;
                            ZB_LOGI(TAG, "Receive color capabilities of node 0x%x, color capabilities=0x%x",
                                g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, endpoint->color_caps);
                        }
                        break;
                    case ATTRID_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MIN_MIREDS:
                        if (zb_zcl_get_data_type_length(attr_resp->data_type) == 2)
                        {
                            endpoint->color_temp_min = BUILD_UINT16(data[0], data[1]);
                            ZB_LOGI(TAG, "Receive color temp physical min of node 0x%x, min=%d",
                                g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, endpoint->color_temp_min);
                        }
                        break;
                    case ATTRID_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MAX_MIREDS:
                        if (zb_zcl_get_data_type_length(attr_resp->data_type) == 2)
                        {
                            endpoint->color_temp_max = BUILD_UINT16(data[0], data[1]);
                            ZB_LOGI(TAG, "Receive color temp physical max of node 0x%x, max=%d",
                                g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, endpoint->color_temp_max);
                        }
                        break;
                    case ATTRID_COLOR_CONTROL_COLOR_MODE:
                        if (zb_zcl_get_data_type_length(attr_resp->data_type) == 1)
                        {
                            ZB_LOGI(TAG, "Receive color mode of node 0x%x, color mode=%d", g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, data[0]);
                        }
                        break;
                    default:
                        ZB_LOGW(TAG, "Unhandled color attr 0x%x of node 0x%x", attr_resp->attr_id, g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr);
                        break;
                }
            }
            else
            {
                ZB_LOGW(TAG, "Receive color attr 0x%x of node 0x%x, status 0x%x", attr_resp->attr_id, g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, attr_resp->status);
            }
        }

        zb_nwksrv_ad_wrap_up(&g_zb_nwksrv_ad_state_machine[i]);
        return ZB_ZCL_OBS_CONSUMED;
    }
    return ZB_ZCL_OBS_IGNORED;
}

static e_zb_zcl_observe_t
zb_nwksrv_ad_process_read_ias_zone_attr_resp_handler(s_zb_zcl_incoming_msg_t *msg)
{
    const s_zb_zcl_read_attr_rsp_cmd_t *rsp = (const s_zb_zcl_read_attr_rsp_cmd_t *)msg->attr_cmd;

    for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
    {
        if (g_zb_nwksrv_ad_state_machine[i].state != ZB_NWKSRV_AD_STATE_GETTING_IAS_ZONE_STATUS &&
            g_zb_nwksrv_ad_state_machine[i].state != ZB_NWKSRV_AD_STATE_IAS_ZONE_ENROLL_CHECKING)
        {
            continue;
        }
        s_zb_device_endpoint_t *endpoint = zb_device_manager_find_endpoint_by_dev_cluster(g_zb_nwksrv_ad_state_machine[i].device_info, ZCL_CLUSTER_ID_SS_IAS_ZONE);
        if (endpoint == NULL ||
            msg->msg->src_addr.short_addr != g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr ||
            msg->msg->src_endpoint != endpoint->endpoint_id)
        {
            continue;
        }
        uint64_t resp_addr = 0;
        uint8_t zone_state = 0;

        s_zb_coordinator_info_t coord_info = {};
        if (zb_core_get_coordinator_info(&coord_info) != ZB_OK)
        {
            ZB_LOGE(TAG, "%s(): Failed to get coordinator info", __func__);
            return ZB_ZCL_OBS_CONSUMED;
        }

        zb_nwksrv_reset_timeout(&g_zb_nwksrv_ad_state_machine[i]);
        for (uint8_t r = 0; r < rsp->num_attr; r++)
        {
            const s_zb_zcl_read_attr_rsp_info_t *attr_resp = &rsp->attr_list[r];
            const uint8_t *data = attr_resp->data;
            if (attr_resp->status == ZCL_STATUS_SUCCESS)
            {
                if (attr_resp->attr_id == ATTRID_SS_IAS_CIE_ADDRESS &&
                    attr_resp->data_type == ZCL_DATATYPE_IEEE_ADDR)
                {
                    /* The parsed value is not guaranteed to be 8-byte aligned,
                     * so copy it out rather than dereferencing a uint64_t*. */
                    memcpy(&resp_addr, data, sizeof(resp_addr));
                }
                else if (attr_resp->attr_id == ATTRID_IAS_ZONE_ZONE_STATE &&
                            zb_zcl_get_data_type_length(attr_resp->data_type) == 1)
                {
                    zone_state = data[0];
                }
                else if (attr_resp->attr_id == ATTRID_IAS_ZONE_ZONE_TYPE &&
                            zb_zcl_get_data_type_length(attr_resp->data_type) == 2)
                {
                    endpoint->ias_zone_type = BUILD_UINT16(data[0], data[1]);
                }
            }
        }

        ZB_LOGI(TAG, "Coord addr %llx, IAS CIE addr %llx, zone state %d", coord_info.ieee_addr, resp_addr, zone_state);
        if ((zone_state == 0 || (coord_info.ieee_addr != resp_addr)))
        {
            if (g_zb_nwksrv_ad_state_machine[i].state == ZB_NWKSRV_AD_STATE_GETTING_IAS_ZONE_STATUS)
            {
                /* Move to write IAS CIE address to start device enrollment */
                zb_nwksrv_reset_tries_count(&g_zb_nwksrv_ad_state_machine[i]);
                g_zb_nwksrv_ad_state_machine[i].state = ZB_NWKSRV_AD_STATE_WRITING_IAS_CIE_ADDRESS;
                zb_nwksrv_send_zcl_ias_zone_write_attr_ias_cie_addr_req(g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr,
                                                                        g_zb_nwksrv_ad_state_machine[i].device_info->endpoints[0].endpoint_id);
            }
        }
        else
        {
            /* Device already enrolled, move to next state to get device model identifier */
            zb_nwksrv_reset_tries_count(&g_zb_nwksrv_ad_state_machine[i]);
            g_zb_nwksrv_ad_state_machine[i].state = ZB_NWKSRV_AD_STATE_GETTING_BASIC_INFO;
            zb_nwksrv_send_zcl_basic_info_read_req(g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr,
                                                            g_zb_nwksrv_ad_state_machine[i].device_info->endpoints[0].endpoint_id);
        }
        return ZB_ZCL_OBS_CONSUMED;
    }
    return ZB_ZCL_OBS_IGNORED;
}

static e_zb_zcl_observe_t
zb_nwksrv_ad_process_write_ias_zone_attr_resp_handler(s_zb_zcl_incoming_msg_t *msg)
{
    for (int i = 0; i < ZB_NWKSRV_MAX_STATE_MACHINES; ++i)
    {
        /* Idle slots carry no device_info; check the state before dereferencing. */
        if (g_zb_nwksrv_ad_state_machine[i].state != ZB_NWKSRV_AD_STATE_WRITING_IAS_CIE_ADDRESS ||
            g_zb_nwksrv_ad_state_machine[i].device_info == NULL)
        {
            continue;
        }
        s_zb_device_endpoint_t *endpoint = zb_device_manager_find_endpoint_by_dev_cluster(g_zb_nwksrv_ad_state_machine[i].device_info, ZCL_CLUSTER_ID_SS_IAS_ZONE);
        if (endpoint == NULL)
        {
            continue;
        }
        if (g_zb_nwksrv_ad_state_machine[i].state == ZB_NWKSRV_AD_STATE_WRITING_IAS_CIE_ADDRESS &&
            msg->msg->src_addr.short_addr == g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr &&
            msg->msg->src_endpoint == endpoint->endpoint_id)
        {

            zb_nwksrv_reset_timeout(&g_zb_nwksrv_ad_state_machine[i]);
            zb_nwksrv_reset_tries_count(&g_zb_nwksrv_ad_state_machine[i]);
            /* Send enroll response and check the read back zone status */
            g_zb_nwksrv_ad_state_machine[i].state = ZB_NWKSRV_AD_STATE_IAS_ZONE_ENROLL_CHECKING;
            /* Send enroll response*/
            zb_nwksrv_send_zcl_ias_zone_enroll_response(g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, endpoint->endpoint_id,
                                                        ZB_HUB_ZONEID, SS_IAS_ZONE_STATUS_ENROLL_RESPONSE_CODE_SUCCESS);
            /* Read back the zone state */
            zb_nwksrv_send_zcl_ias_zone_enroll_checking_req(g_zb_nwksrv_ad_state_machine[i].device_info->nwk_addr, endpoint->endpoint_id);
            return ZB_ZCL_OBS_CONSUMED;
        }
    }
    return ZB_ZCL_OBS_IGNORED;
}

/* Continue the interview after the identity reads (basic + best-effort
 * extended): read colour capabilities if the device has that cluster, else
 * wrap up. Shared by the extended-read success path and its timeout. */
static void
zb_nwksrv_ad_proceed_after_identity(s_zb_nwksrv_ad_state_machine_t *state_ptr)
{
    s_zb_device_endpoint_t *endpoint =
        zb_device_manager_find_endpoint_by_dev_cluster(state_ptr->device_info, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL);
    if (endpoint != NULL)
    {
        zb_nwksrv_reset_tries_count(state_ptr);
        zb_nwksrv_reset_timeout(state_ptr);
        state_ptr->state = ZB_NWKSRV_AD_STATE_GETTING_COLOR_CAPABILITIES;
        zb_nwksrv_send_zcl_color_capabilities_read_req(state_ptr->device_info->nwk_addr, endpoint->endpoint_id);
    }
    else
    {
        zb_nwksrv_ad_wrap_up(state_ptr);
    }
}

/**
 * @brief Wrap up. We're done gather info about the state machine
 * 
 * @param state - pointer to state machine
 */
static void
zb_nwksrv_ad_wrap_up(s_zb_nwksrv_ad_state_machine_t *state)
{
    state->state = ZB_NWKSRV_AD_STATE_WRAP_UP;
    ZB_LOGI(TAG, "Service discovery complete for node %04x", state->device_info->nwk_addr);
    if (state->pfn_add_device_cb)
    {
        ZB_LOGI(TAG, "Callback function");
        state->pfn_add_device_cb(state->device_info, state->device_info->ieee_addr, ZB_NWKSRV_ADS_OK);
    }

    /* Free the state machine */
    zb_nwksrv_ad_free_state_machine(state);
}

static void
zb_nwksrv_send_zcl_basic_info_read_req(uint16_t nwk_addr, uint8_t endpoint)
{
    s_zb_af_address_t dst_addr = {
        .short_addr = nwk_addr,
        .address_mode = AF_ADDRESS_16BIT,
        .endpoint = endpoint
    };
    /* Keep this to the small, universally-supported set. A Read-Attributes
     * RESPONSE must fit in one APS frame (~66-82 bytes, and most sleepy/simple
     * end devices don't do APS fragmentation). These 5 (only 2 short strings)
     * fit; adding the extended identity attributes - DateCode/SerialNumber/
     * ProductLabel/SWBuildID are all strings - overflows a single frame, so the
     * device stays silent and the interview fails. Read the extended identity
     * separately (best-effort, chunked) if needed, never in this critical read. */
    uint8_t buf[sizeof(s_zb_zcl_read_attr_cmd_t) + 8 * sizeof(uint16_t)] = {0};
    s_zb_zcl_read_attr_cmd_t *read_attr_cmd = (s_zb_zcl_read_attr_cmd_t *)&buf[0];
    read_attr_cmd->num_attr = 7;
    read_attr_cmd->attr_id[0] = ATTRID_BASIC_ZCL_VERSION;
    read_attr_cmd->attr_id[1] = ATTRID_BASIC_APPLICATION_VERSION;
    read_attr_cmd->attr_id[2] = ATTRID_BASIC_HW_VERSION;
    read_attr_cmd->attr_id[3] = ATTRID_BASIC_MANUFACTURER_NAME;
    read_attr_cmd->attr_id[4] = ATTRID_BASIC_MODEL_IDENTIFIER;
    read_attr_cmd->attr_id[5] = ATTRID_BASIC_POWER_SOURCE;
    read_attr_cmd->attr_id[6] = ATTRID_BASIC_PHYSICAL_ENVIRONMENT;
    uint8_t seq_num = zb_zcl_next_seq_num();
    zb_zcl_send_read(ZB_HUB_ENDPOINT, &dst_addr, ZCL_CLUSTER_ID_GENERAL_BASIC,
                                read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, seq_num);
}

/* Best-effort read of one extended-identity chunk (see chunk table above).
 * A device that can't answer (unreachable, or response still too big) simply
 * stays silent; the caller treats this as optional and proceeds regardless. */
static void
zb_nwksrv_send_zcl_extended_info_read_req(uint16_t nwk_addr, uint8_t endpoint, uint8_t chunk_idx)
{
    if (chunk_idx >= ZB_NWKSRV_EXTENDED_INFO_CHUNK_COUNT)
    {
        return;
    }
    s_zb_af_address_t dst_addr = {
        .short_addr = nwk_addr,
        .address_mode = AF_ADDRESS_16BIT,
        .endpoint = endpoint
    };
    uint8_t buf[sizeof(s_zb_zcl_read_attr_cmd_t) +
                ZB_NWKSRV_EXTENDED_INFO_CHUNK_MAX_ATTRS * sizeof(uint16_t)] = {0};
    s_zb_zcl_read_attr_cmd_t *read_attr_cmd = (s_zb_zcl_read_attr_cmd_t *)&buf[0];
    read_attr_cmd->num_attr = s_extended_info_chunks[chunk_idx].count;
    for (uint8_t a = 0; a < s_extended_info_chunks[chunk_idx].count; a++)
    {
        read_attr_cmd->attr_id[a] = s_extended_info_chunks[chunk_idx].attrs[a];
    }
    uint8_t seq_num = zb_zcl_next_seq_num();
    zb_zcl_send_read(ZB_HUB_ENDPOINT, &dst_addr, ZCL_CLUSTER_ID_GENERAL_BASIC,
                                read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, seq_num);
}

/* Enter (or continue) the best-effort extended-identity read at @p chunk_idx:
 * single attempt, short window, sent to the device's Basic endpoint. */
static void
zb_nwksrv_ad_start_extended_chunk(s_zb_nwksrv_ad_state_machine_t *state_ptr, uint8_t chunk_idx)
{
    s_zb_device_endpoint_t *basic_ep =
        zb_device_manager_find_endpoint_by_dev_cluster(state_ptr->device_info, ZCL_CLUSTER_ID_GENERAL_BASIC);
    uint8_t ep_id = basic_ep ? basic_ep->endpoint_id : state_ptr->device_info->endpoints[0].endpoint_id;

    state_ptr->state = ZB_NWKSRV_AD_STATE_GETTING_EXTENDED_INFO;
    state_ptr->ext_chunk = chunk_idx;
    state_ptr->tries = 0; /* no retries - must never block the join */
    zb_nwksrv_reset_timeout(state_ptr);
    zb_nwksrv_send_zcl_extended_info_read_req(state_ptr->device_info->nwk_addr, ep_id, chunk_idx);
}

static void
zb_nwksrv_send_zcl_color_capabilities_read_req(uint16_t nwk_addr, uint8_t endpoint)
{
    s_zb_af_address_t dst_addr = {
        .short_addr = nwk_addr,
        .address_mode = AF_ADDRESS_16BIT,
        .endpoint = endpoint
    };
    uint8_t buf[sizeof(s_zb_zcl_read_attr_cmd_t) + 7 * sizeof(uint16_t)] = {0};
    s_zb_zcl_read_attr_cmd_t *read_attr_cmd = (s_zb_zcl_read_attr_cmd_t *)&buf[0];
    read_attr_cmd->num_attr = 7;
    read_attr_cmd->attr_id[0] = ATTRID_COLOR_CONTROL_CURRENT_HUE;
    read_attr_cmd->attr_id[1] = ATTRID_COLOR_CONTROL_CURRENT_SATURATION;
    read_attr_cmd->attr_id[2] = ATTRID_COLOR_CONTROL_COLOR_TEMPERATURE_MIREDS;
    read_attr_cmd->attr_id[3] = ATTRID_COLOR_CONTROL_COLOR_MODE;
    read_attr_cmd->attr_id[4] = ATTRID_COLOR_CONTROL_COLOR_CAPABILITIES;
    /* The physical limits: what colour temperatures this light can actually
     * reach, as opposed to what the ZCL attribute range permits. */
    read_attr_cmd->attr_id[5] = ATTRID_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MIN_MIREDS;
    read_attr_cmd->attr_id[6] = ATTRID_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MAX_MIREDS;
    uint8_t seq_num = zb_zcl_next_seq_num();
    zb_zcl_send_read(ZB_HUB_ENDPOINT, &dst_addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
                                read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, true, seq_num);
}

static void
zb_nwksrv_send_zcl_ias_zone_write_attr_ias_cie_addr_req(uint16_t nwk_addr, uint8_t endpoint)
{
    s_zb_coordinator_info_t coord_info = {};
    if (zb_core_get_coordinator_info(&coord_info) != ZB_OK)
    {
        ZB_LOGE(TAG, "Failed to get coordinator info");
        return;
    }

    s_zb_af_address_t dst_addr = {
        .short_addr = nwk_addr,
        .address_mode = AF_ADDRESS_16BIT,
        .endpoint = endpoint
    };

    uint8_t buf[sizeof(s_zb_zcl_write_attr_cmd_t) + sizeof(s_zb_zcl_write_attr_info_t)] = {0};
    s_zb_zcl_write_attr_cmd_t *write_attr_cmd = (s_zb_zcl_write_attr_cmd_t *)&buf[0];

    write_attr_cmd->num_attr = 1;
    write_attr_cmd->attr_list[0].attr_id = ATTRID_SS_IAS_CIE_ADDRESS;
    write_attr_cmd->attr_list[0].data_type = ZCL_DATATYPE_IEEE_ADDR;
    write_attr_cmd->attr_list[0].attr_data = (uint8_t *)&coord_info.ieee_addr;
    uint8_t seq_num = zb_zcl_next_seq_num();
    zb_zcl_send_write(ZB_HUB_ENDPOINT, &dst_addr, ZCL_CLUSTER_ID_SS_IAS_ZONE,
                                write_attr_cmd, ZCL_CMD_WRITE, ZCL_FRAME_CLIENT_SERVER_DIR, true, seq_num);
}

static void
zb_nwksrv_send_zcl_ias_zone_enroll_response(uint16_t nwk_addr, uint8_t endpoint, uint8_t zone_id, uint8_t resp_code)
{
    s_zb_af_address_t dst_addr = {
        .short_addr = nwk_addr,
        .address_mode = AF_ADDRESS_16BIT,
        .endpoint = endpoint
    };
    uint8_t seq_num = zb_zcl_next_seq_num();
    zb_zcl_ss_ias_send_zone_status_enroll_response_cmd(ZB_HUB_ENDPOINT, &dst_addr,
                                                          resp_code, zone_id, true, seq_num);
}

static void
zb_nwksrv_send_zcl_ias_zone_enroll_checking_req(uint16_t nwk_addr, uint8_t endpoint)
{
    s_zb_af_address_t dst_addr = {
        .short_addr = nwk_addr,
        .address_mode = AF_ADDRESS_16BIT,
        .endpoint = endpoint
    };
    uint8_t buf[sizeof(s_zb_zcl_read_attr_cmd_t) + 3 * sizeof(uint16_t)] = {0};
    s_zb_zcl_read_attr_cmd_t *read_attr_cmd = (s_zb_zcl_read_attr_cmd_t *)&buf[0];
    uint8_t seq_num = zb_zcl_next_seq_num();
    read_attr_cmd->num_attr = 3;
    read_attr_cmd->attr_id[0] = ATTRID_SS_IAS_CIE_ADDRESS;
    read_attr_cmd->attr_id[1] = ATTRID_IAS_ZONE_ZONE_STATE;
    read_attr_cmd->attr_id[2] = ATTRID_IAS_ZONE_ZONE_TYPE;
    zb_zcl_send_read(ZB_HUB_ENDPOINT, &dst_addr, ZCL_CLUSTER_ID_SS_IAS_ZONE,
                                read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, true, seq_num);
}

static void
zb_nwksrv_ad_process_force_device_leave_network(s_zb_device_info_t *device_info)
{
    ZB_LOGI(TAG, "Force device with ieee_addr 0x%llx nwk_addr 0x%x leave network", device_info->ieee_addr, device_info->nwk_addr);

    zb_zdo_send_mgmt_leave_req(device_info->nwk_addr, device_info->ieee_addr, 0, 0);
}
