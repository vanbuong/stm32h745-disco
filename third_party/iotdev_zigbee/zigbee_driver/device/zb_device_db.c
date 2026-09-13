/*
 * zb_device_db.c
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#include "common/zb_common.h"
#include "device/zb_device_db.h"
#include "proto/zb_device.pb.h"
#include "iotdev_utils/iotdev_protobuf/nanopb/pb_encode.h"
#include "iotdev_utils/iotdev_protobuf/nanopb/pb_decode.h"

#define TAG "ZB_DB"

/* Local prototypes */

/**
 * @brief Write callback for encoding process of device list
 *
 * @param[in] stream - the pointer to stream for writing data to
 * @param[in] buf - the buffer array need to be written
 * @param[in] count - the number of data bytes
 * @return true - if writing count bytes successfully
 * @return false - if writing count bytes failed
 */
static bool
zb_nwkdb_write_callback(struct pb_ostream_s *stream, const uint8_t *buf, size_t count)
{
    FILE *f = (FILE *)stream->state;
    return fwrite(buf, 1, count, f) == count;
}

/**
 * @brief Read callback for decoding process of device list
 *
 * @param[in] stream - the pointer to stream for reading data from
 * @param[in] buf - the buffer array need to be read
 * @param[in] count - the number of data bytes
 * @return true - if reading count bytes successfully
 * @return false - if reading count bytes failed
 */
static bool
zb_nwkdb_read_callback(struct pb_istream_s *stream, uint8_t *buf, size_t count)
{
    FILE *f = (FILE *)stream->state;
    int result;

    if (count == 0)
    {
        return true;
    }
    result = fread(buf, 1, count, f);

    if (result <= 0)
    {
        stream->bytes_left = 0; /* EOF */
    }
    return result == count;
}

/**
 * @brief Get the output stream context for encoding process
 * 
 * @param[in] f - the pointer to the output file handle
 * @return pb_ostream_t
 */
static struct pb_ostream_s
pb_ostream_from_file(FILE *f)
{
    struct pb_ostream_s stream = {};
    stream.callback = &zb_nwkdb_write_callback;
    stream.state = (void *)f;
    stream.max_size = SIZE_MAX;
    stream.bytes_written = 0;
    return stream;
}

/**
 * @brief Get the input stream context for decoding process
 *
 * @param[in] f - the pointer to the input file handle
 * @return pb_istream_t
 */
static struct pb_istream_s
pb_istream_from_file(FILE *f)
{
    struct pb_istream_s stream = {};
    stream.callback = &zb_nwkdb_read_callback;
    stream.state = (void *)f;
    stream.bytes_left = SIZE_MAX;
    return stream;
}

/**
 * @brief Write callback for encoding process of cluster list
 *
 * @param[in] stream - the pointer to stream for writing data to
 * @param[in] buf - the buffer array need to be written
 * @param[in] count - the number of data bytes
 * @return true - if writing count bytes successfully
 * @return false - if writing count bytes failed
 */
static bool
zb_nwkdb_encode_in_cluster_list(struct pb_ostream_s *ostream, const pb_field_iter_t *field, void * const *arg)
{
    s_zb_device_endpoint_t *endpoint = (s_zb_device_endpoint_t *)(*arg);
    for (int i = 0; i < endpoint->in_cluster_count; ++i)
    {
        if (!pb_encode_tag_for_field(ostream, field))
        {
            ZB_LOGE(TAG, "Encode in cluster list tag for field error!");
            return false;
        }
        if (!pb_encode_varint(ostream, endpoint->in_clusters[i]))
        {
            ZB_LOGE(TAG, "Encode in cluster list error!");
            return false;
        }
    }
    return true;
}

/**
 * @brief Write callback for encoding process of cluster list
 *
 * @param[in] stream - the pointer to stream for writing data to
 * @param[in] buf - the buffer array need to be written
 * @param[in] count - the number of data bytes
 * @return true - if writing count bytes successfully
 * @return false - if writing count bytes failed
 */
static bool
zb_nwkdb_encode_out_cluster_list(struct pb_ostream_s *ostream, const pb_field_iter_t *field, void * const *arg)
{
    s_zb_device_endpoint_t *endpoint = (s_zb_device_endpoint_t *)(*arg);
    for (int i = 0; i < endpoint->out_cluster_count; ++i)
    {
        if (!pb_encode_tag_for_field(ostream, field))
        {
            ZB_LOGE(TAG, "Encode out cluster list tag for field error!");
            return false;
        }
        if (!pb_encode_varint(ostream, endpoint->out_clusters[i]))
        {
            ZB_LOGE(TAG, "Encode out cluster list error!");
            return false;
        }
    }
    return true;
}

/**
 * @brief Write callback for encoding process of cluster list
 *
 * @param[in] stream - the pointer to stream for writing data to
 * @param[in] buf - the buffer array need to be written
 * @param[in] count - the number of data bytes
 * @return true - if writing count bytes successfully
 * @return false - if writing count bytes failed
 */
static bool
zb_nwkdb_decode_in_cluster_list(struct pb_istream_s *istream, const pb_field_iter_t *field, void **arg)
{
    s_zb_device_endpoint_t *endpoint = (s_zb_device_endpoint_t *)(*arg);
    uint64_t val;
    if (!pb_decode_varint(istream, &val))
    {
        ZB_LOGE(TAG, "Decode in cluster list error!");
        return false;
    }
    endpoint->in_clusters[endpoint->in_cluster_count++] = (uint16_t)val;
    return true;
}

/**
 * @brief Write callback for encoding process of cluster list
 *
 * @param[in] stream - the pointer to stream for writing data to
 * @param[in] buf - the buffer array need to be written
 * @param[in] count - the number of data bytes
 * @return true - if writing count bytes successfully
 * @return false - if writing count bytes failed
 */
static bool
zb_nwkdb_decode_out_cluster_list(struct pb_istream_s *istream, const pb_field_iter_t *field, void **arg)
{
    s_zb_device_endpoint_t *endpoint = (s_zb_device_endpoint_t *)(*arg);
    uint64_t val;
    if (!pb_decode_varint(istream, &val))
    {
        ZB_LOGE(TAG, "Decode out cluster list error!");
        return false;
    }
    endpoint->out_clusters[endpoint->out_cluster_count++] = (uint16_t)val;
    return true;
}

/**
 * @brief Encode device endpoint
 *
 * @param[in] ostream - output stream to write encode data to
 * @param[in] field - the field of data
 * @param[in] arg - the pointer to device endpoint from user
 * @return true - if the encode process finishes successfully
 * @return false - if the encode process failed
 */
static bool
zb_nwkdb_encode_endpoint_info(struct pb_ostream_s *ostream, const pb_field_iter_t *field, void * const *arg)
{
    s_zb_device_t *device = (s_zb_device_t *)(*arg);
    zb_endpoint_info endpoint = zb_endpoint_info_init_zero;

    for (int i = 0; i < device->endpoint_count; ++i)
    {
        if (!pb_encode_tag_for_field(ostream, field))
        {
            ZB_LOGE(TAG, "Encode device endpoint tag for field error!");
            return false;
        }
        endpoint.endpoint_id = device->endpoints[i].endpoint_id;
        endpoint.profile_id = device->endpoints[i].profile_id;
        endpoint.device_id = device->endpoints[i].device_id;
        endpoint.ias_zone_type = device->endpoints[i].ias_zone_type;
        endpoint.color_caps = device->endpoints[i].color_caps;
        endpoint.color_caps_valid = device->endpoints[i].color_caps_valid;
        endpoint.has_color_temp_min = true;
        endpoint.color_temp_min = device->endpoints[i].color_temp_min;
        endpoint.has_color_temp_max = true;
        endpoint.color_temp_max = device->endpoints[i].color_temp_max;
        endpoint.in_cluster_list.funcs.encode = zb_nwkdb_encode_in_cluster_list;
        endpoint.in_cluster_list.arg = &device->endpoints[i];
        endpoint.out_cluster_list.funcs.encode = zb_nwkdb_encode_out_cluster_list;
        endpoint.out_cluster_list.arg = &device->endpoints[i];
        if (!pb_encode_submessage(ostream, zb_endpoint_info_fields, &endpoint))
        {
            ZB_LOGE(TAG, "Encode device endpoint error!");
            return false;
        }
    }
    return true;
}


/**
 * @brief Decode device endpoint from input stream of file
 *
 * @param[in] istream - the input stream to read data for decoding process
 * @param[in] field - the data field will be decoded (no used now)
 * @param[in] arg - the pointer to the device endpoint will be stored
 * @return true - if the decoding process success
 * @return false - if the decoding process failed
 */
static bool
zb_nwkdb_decode_endpoint_info(struct pb_istream_s *istream, const pb_field_iter_t *field, void **arg)
{
    s_zb_device_t *device = (s_zb_device_t *)(*arg);
    memset(&device->endpoints[device->endpoint_count], 0, sizeof(s_zb_device_endpoint_t));
    /* Attach cluster decode callbacks */
    zb_endpoint_info endpoint = zb_endpoint_info_init_zero;
    endpoint.in_cluster_list.funcs.decode = zb_nwkdb_decode_in_cluster_list;
    endpoint.in_cluster_list.arg = &device->endpoints[device->endpoint_count];
    endpoint.out_cluster_list.funcs.decode = zb_nwkdb_decode_out_cluster_list;
    endpoint.out_cluster_list.arg = &device->endpoints[device->endpoint_count];

    if (!pb_decode(istream, zb_endpoint_info_fields, &endpoint))
    {
        ZB_LOGE(TAG, "Decode device endpoint error!");
        return false;
    }
    device->endpoints[device->endpoint_count].endpoint_id = endpoint.endpoint_id;
    device->endpoints[device->endpoint_count].profile_id = endpoint.profile_id;
    device->endpoints[device->endpoint_count].device_id = endpoint.device_id;
    device->endpoints[device->endpoint_count].ias_zone_type = endpoint.ias_zone_type;
    device->endpoints[device->endpoint_count].color_caps = endpoint.color_caps;
    device->endpoints[device->endpoint_count].color_caps_valid = endpoint.color_caps_valid;
    device->endpoints[device->endpoint_count].color_temp_min =
        endpoint.has_color_temp_min ? (uint16_t)endpoint.color_temp_min : 0;
    device->endpoints[device->endpoint_count].color_temp_max =
        endpoint.has_color_temp_max ? (uint16_t)endpoint.color_temp_max : 0;
    device->endpoint_count++;
    return true;
}

/* Global prototypes */

/**
 * @brief Initialize database
 *
 * @return int - ZB_OK if success, otherwise ZB_FAIL
 */
zb_status_t
zb_device_db_int(void)
{
    return ZB_OK;
}

/**
 * @brief Load device list from database file in flash storage
 *
 * @return DeviceList* - pointer of device list has been loaded, NULL if load failed
 */
zb_status_t
zb_device_db_load_device(s_zb_device_t *device, char *path)
{
    int ret = ZB_FAIL;
    FILE *f = NULL;

    zb_device_info device_info = zb_device_info_init_zero;
    device_info.endpoints.funcs.decode = &zb_nwkdb_decode_endpoint_info;
    device_info.endpoints.arg = device;

    ZB_LOGI(TAG, "Loading device file: %s", path);
    f = fopen(path, "rb");

    if (f)
    {
        struct pb_istream_s input = pb_istream_from_file(f);
        if (pb_decode(&input, zb_device_info_fields, &device_info))
        {
            /* Copy device info to device list */
            memcpy(device->manufacturer, device_info.manufacturer, sizeof(device_info.manufacturer));
            memcpy(device->model, device_info.model, sizeof(device_info.model));
            device->ieee_addr = device_info.ieee_addr;
            device->parent_ieee = device_info.parent_ieee;
            device->nwk_addr = device_info.nwk_addr;
            device->manu_id = device_info.manu_id;
            device->capabilities = device_info.capabilities;
            if (device_info.has_hw_version)
            {
                device->hw_version = device_info.hw_version;
            }
            if (device_info.has_date_code)
            {
                memcpy(device->date_code, device_info.date_code, sizeof(device->date_code));
            }
            if (device_info.has_product_code)
            {
                device->product_code_len = device_info.product_code.size <= sizeof(device->product_code)
                                               ? device_info.product_code.size : sizeof(device->product_code);
                memcpy(device->product_code, device_info.product_code.bytes, device->product_code_len);
            }
            if (device_info.has_serial_number)
            {
                memcpy(device->serial_number, device_info.serial_number, sizeof(device->serial_number));
            }
            if (device_info.has_product_label)
            {
                memcpy(device->product_label, device_info.product_label, sizeof(device->product_label));
            }
            if (device_info.has_sw_build_id)
            {
                memcpy(device->sw_build_id, device_info.sw_build_id, sizeof(device->sw_build_id));
            }
            if (device_info.has_power_source)
            {
                device->power_source = (uint8_t)device_info.power_source;
            }
            ret = ZB_OK;
        }
        else
        {
            ZB_LOGE(TAG, "Decode failed: %s", PB_GET_ERROR(&input));
        }
        fclose(f);
    }
    else
    {
        ZB_LOGE(TAG, "File %s open to read failure..", path);
        ret = ZB_FAIL;
    }
    return ret;
}

/**
 * @brief Update the device list into database file in flash storage
 *
 * @param devices - pointer to device info want to store in flash
 * @return int
 */
zb_status_t
zb_device_db_update_device(s_zb_device_t *device, char *path)
{
    int ret = ZB_FAIL;
    FILE *f = NULL;

    zb_device_info device_info = zb_device_info_init_zero;

    f = fopen(path, "wb");
    if (f)
    {
        struct pb_ostream_s output = pb_ostream_from_file(f);

        /* Copy device info to protobuf structure */
        memcpy(device_info.manufacturer, device->manufacturer, sizeof(device->manufacturer));
        memcpy(device_info.model, device->model, sizeof(device->model));
        device_info.ieee_addr = device->ieee_addr;
        device_info.parent_ieee = device->parent_ieee;
        device_info.nwk_addr = device->nwk_addr;
        device_info.manu_id = device->manu_id;
        device_info.capabilities = device->capabilities;
        device_info.has_hw_version = true;
        device_info.hw_version = device->hw_version;
        device_info.has_date_code = true;
        memcpy(device_info.date_code, device->date_code, sizeof(device_info.date_code));
        device_info.has_product_code = true;
        device_info.product_code.size = device->product_code_len <= sizeof(device_info.product_code.bytes)
                                            ? device->product_code_len : sizeof(device_info.product_code.bytes);
        memcpy(device_info.product_code.bytes, device->product_code, device_info.product_code.size);
        device_info.has_serial_number = true;
        memcpy(device_info.serial_number, device->serial_number, sizeof(device_info.serial_number));
        device_info.has_product_label = true;
        memcpy(device_info.product_label, device->product_label, sizeof(device_info.product_label));
        device_info.has_sw_build_id = true;
        memcpy(device_info.sw_build_id, device->sw_build_id, sizeof(device_info.sw_build_id));
        device_info.has_power_source = true;
        device_info.power_source = device->power_source;
        device_info.endpoints.funcs.encode = zb_nwkdb_encode_endpoint_info;
        device_info.endpoints.arg = device;

        if (pb_encode(&output, zb_device_info_fields, &device_info))
        {
            ZB_LOGI(TAG, "Encode and update database done.");
            ret = ZB_OK;
        }
        else
        {
            ZB_LOGE(TAG, "Encoding failed: %s", PB_GET_ERROR(&output));
        }
        fclose(f);
    }
    else
    {
        ZB_LOGE(TAG, "File %s open for write failure..", path);
        ret = ZB_FAIL;
    }
    return ret;
}
