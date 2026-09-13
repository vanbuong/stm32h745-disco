/*
 * zb_af.c
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#include "af/zb_af.h"
#include "common/zb_common.h"
#include "znp/zb_znp.h"
#include "znp/zb_znp_mt_af.h"

/* Global transaction ID management */
static uint8_t g_global_trans_id = 1;

static int
send_af_data_store_cmd(uint16_t index, uint8_t length, uint8_t *data)
{
    int status = ZB_OK;
    s_zb_znp_mt_af_data_store_cmd_t af_data_store_cmd;
    af_data_store_cmd.index = index;
    af_data_store_cmd.length = length;
    af_data_store_cmd.data = data;

    status = zb_znp_mt_af_data_store(&af_data_store_cmd);
    return status;
}

int
zb_af_data_req(
    s_zb_af_address_t *dst_addr, uint16_t src_endpoint,
    uint16_t cluster_id, uint8_t *buff, uint16_t buff_len,
    uint8_t options, uint8_t radius)
{
    int status = ZB_OK;
    uint16_t msg_len = 0;
    s_zb_znp_mt_af_data_request_ext_cmd_t *af_ext_cmd = NULL;
    uint16_t data_len = 0;
    uint8_t sent_frags = 0;
    bool fragmented = (buff_len > (ZNP_MT_DATA_MAX_LEN - AF_DATA_REQ_HDR_LEN));

    if (fragmented)
    {
        // First build and send the AF_DATA_REQ_EXT message (no data)
        // Then send the data is fragmented
        data_len = buff_len;
        msg_len = AF_DATA_REQ_HDR_LEN;
    }
    else
    {
        // Send the data and header in one packet
        msg_len = buff_len + AF_DATA_REQ_HDR_LEN;
    }

    af_ext_cmd = (s_zb_znp_mt_af_data_request_ext_cmd_t *)ZB_MEM_MALLOC(msg_len);

    if (af_ext_cmd)
    {
        memset(af_ext_cmd, 0, msg_len);
        af_ext_cmd->dst_addr_mode = dst_addr->address_mode;
        if (dst_addr->address_mode == AF_ADDRESS_64BIT)
        {
            memcpy(af_ext_cmd->dst_addr, (uint8_t *)&dst_addr->long_addr, AF_LONG_ADDR_LEN);
        }
        else
        {
            af_ext_cmd->dst_addr[0] = LO_UINT16(dst_addr->short_addr);
            af_ext_cmd->dst_addr[1] = HI_UINT16(dst_addr->short_addr);
        }

        af_ext_cmd->dst_endpoint = dst_addr->endpoint;
        af_ext_cmd->dst_pan_id = dst_addr->pan_id;
        af_ext_cmd->src_endpoint = src_endpoint;
        af_ext_cmd->cluster_id = cluster_id;

        af_ext_cmd->trans_id = g_global_trans_id;
        g_global_trans_id++;
        if (g_global_trans_id == 0)
        {
            /* Never use 0: it is the "no transaction" sentinel and would
             * make AF_DATA_CONFIRM correlation ambiguous. */
            g_global_trans_id = 1;
        }

        af_ext_cmd->options = options;
        af_ext_cmd->radius = radius;

        af_ext_cmd->len = buff_len;

        if (data_len == 0)
        {
            memcpy(af_ext_cmd->data, buff, buff_len);
        }

        status = zb_znp_mt_af_data_request_ext(af_ext_cmd);
        ZB_MEM_FREE(af_ext_cmd);

        while (data_len)
        {
            uint8_t frag_size;

            // Calculate the fragment size for this packet
            if ((data_len - (sent_frags * AF_DATA_STORE_MAX_LEN)) > AF_DATA_STORE_MAX_LEN)
            {
                frag_size = AF_DATA_STORE_MAX_LEN;
            }
            else
            {
                frag_size = data_len - (sent_frags * AF_DATA_STORE_MAX_LEN);
                data_len = 0;
            }
            
            status = send_af_data_store_cmd(
                        sent_frags * AF_DATA_STORE_MAX_LEN + frag_size, frag_size, buff + sent_frags * AF_DATA_STORE_MAX_LEN);

            if (data_len == 0)
            {
                // Send the "special" message to end the fragmentation
                status = send_af_data_store_cmd(sent_frags * AF_DATA_STORE_MAX_LEN + frag_size, 0, NULL);
            }

            sent_frags++;
        }
        
    }
    else
    {
        status = AF_STATUS_MEM_ERROR;
    }
    
    return status;
}

void
zb_af_incoming_msg_free(s_zb_af_incoming_msg_t *msg)
{
    if (msg == NULL)
    {
        return;
    }
    if (msg->command.data != NULL)
    {
        ZB_MEM_FREE(msg->command.data);
        msg->command.data = NULL;
    }
    ZB_MEM_FREE(msg);
}

/**
 * @brief Initialize the AF/APS transaction ID counter
 */
void
zb_af_trans_id_init(void)
{
    g_global_trans_id = 1; // Start from 1, avoid 0
}
