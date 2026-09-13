/*
 * zb_device_state_cache.c
 *
 * See zb_device_state_cache.h for what this is and why it is a separate file
 * from the device database.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#include "device/zb_device_state_cache.h"

#include "common/zb_common.h"
#include "device/zb_device_manager.h"

#include <stdlib.h>
#include <string.h>

#if defined(ZB_PLATFORM_IOTDEV)
#include "esp_heap_caps.h"
/* Ask for external RAM explicitly, falling back to internal so a board without
 * PSRAM (or one whose PSRAM is full) still gets a working cache. */
static inline void *
zb_state_cache_alloc(size_t size)
{
    void *p = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    return (p != NULL) ? p : heap_caps_malloc(size, MALLOC_CAP_8BIT);
}
#define ZB_STATE_CACHE_ALLOC(sz)    zb_state_cache_alloc(sz)
#else
#define ZB_STATE_CACHE_ALLOC(sz)    malloc(sz)
#endif

#define TAG "ZB_STATE_CACHE"

#define ZB_STATE_CACHE_FILE         ZB_DIR"/zb_state.db"
#define ZB_STATE_CACHE_TMP          ZB_DIR"/zb_state.tmp"
#define ZB_STATE_CACHE_MAGIC        0x5A425354u   /* "ZBST" */
#define ZB_STATE_CACHE_VERSION      1u

/* Matches zigbee2mqtt's state.json cadence. Long enough that flash wear is
 * irrelevant (a 7.5 MB partition gives centuries at this rate), short enough
 * that an unplanned reset loses only a few minutes of readings. */
#define ZB_STATE_CACHE_FLUSH_MS     (5u * 60u * 1000u)

/* One device's last-known state. Written to flash verbatim, so every field is
 * fixed-width and the whole record is position-independent. */
typedef struct s_zb_state_record
{
    uint64_t ieee_addr;
    uint64_t last_seen;                 /* UTC epoch seconds                  */
    uint8_t  online;
    uint8_t  lqi;
    uint8_t  value_count;
    uint8_t  reserved;                  /* keep the values 4-byte aligned     */
    s_zb_func_value_t values[ZB_STATE_CACHE_MAX_VALUES];
} s_zb_state_record_t;

typedef struct s_zb_state_file_header
{
    uint32_t magic;
    uint32_t version;
    uint32_t count;
    uint32_t crc;                       /* over the records that follow       */
} s_zb_state_file_header_t;

/* The record table is ~84 KB - too much to leave sitting in internal DRAM for
 * the life of the process, and it is touched only on restore and on the
 * five-minute flush, so external RAM's slower access costs nothing here. One
 * allocation at init, never freed: the cache lives as long as the driver.
 * heap_caps_malloc() names the intent rather than relying on
 * CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL happening to route a block this size
 * outwards; the internal fallback keeps a board without PSRAM working. */
static s_zb_state_record_t *s_records = NULL;
static uint16_t s_record_count = 0;
static bool     s_dirty = false;
static uint32_t s_next_flush_ms = 0;

/* ------------------------------------------------------------------------ */

bool
zb_device_state_cache_is_cacheable(e_zb_function_type_t type)
{
    switch (type)
    {
        /* Momentary: an action is an event, not a state. */
        case ZB_FUNC_BUTTON:
        /* Alarm-like: restoring these fires automations for something that is
         * no longer happening. They come back unknown and wait for the device. */
        case ZB_FUNC_OCCUPANCY:
        case ZB_FUNC_IAS_MOTION_SENSOR:
        case ZB_FUNC_IAS_CONTACT_SWITCH:
        case ZB_FUNC_IAS_DOOR_WINDOW_HANDLE:
        case ZB_FUNC_IAS_FIRE_SENSOR:
        case ZB_FUNC_IAS_WATER_SENSOR:
        case ZB_FUNC_IAS_CO_SENSOR:
        case ZB_FUNC_IAS_PERSONAL_EMERGENCY:
        case ZB_FUNC_IAS_VIBRATION_SENSOR:
        case ZB_FUNC_IAS_GENERIC_SENSOR:
        /* No readable state to begin with. */
        case ZB_FUNC_BASIC_INFO:
        case ZB_FUNC_IAS_WARNING:
            return false;

        /* Sensor readings - the whole point of the cache. A sleepy device
         * cannot be read back (zb_core_query_device_task skips end devices),
         * so without this its row is blank until it next reports. */
        case ZB_FUNC_TEMPERATURE:
        case ZB_FUNC_HUMIDITY:
        case ZB_FUNC_PRESSURE:
        case ZB_FUNC_FLOW:
        case ZB_FUNC_ILLUMINANCE:
        case ZB_FUNC_PM25:
        case ZB_FUNC_PM10:
        case ZB_FUNC_PM1:
        case ZB_FUNC_CO2:
        case ZB_FUNC_ECO2:
        case ZB_FUNC_TVOC:
        case ZB_FUNC_FORMALDEHYDE:
        case ZB_FUNC_IAQ:
        case ZB_FUNC_BATTERY:

        /* Actuator position - a lamp's on/off, level and colour, a plug's
         * relay. A mains actuator is re-read within seconds of the network
         * coming up (poll_interval_ms == 0 => one read while !synced), so the
         * restored value is short-lived; it is there to cover those seconds,
         * during which every lamp and plug in the house would otherwise show a
         * blank row on the app. Nothing is commanded on the strength of it -
         * the value only populates the state table, and is flagged stale (see
         * zb_device_manager_value_is_stale()) until the device next publishes
         * a value of its own. Note a device that confirms the value unchanged
         * publishes nothing, so the flag can outlive the confirmation; that
         * costs nothing today, as no consumer reads it. */
        case ZB_FUNC_ONOFF_LIGHT:
        case ZB_FUNC_ONOFF_PLUGIN_UNIT:
        case ZB_FUNC_ONOFF_SMART_PLUG:
        case ZB_FUNC_MAINS_POWER_OUTLET:
        case ZB_FUNC_RELAY:
        case ZB_FUNC_ONOFF_SWITCH:
        case ZB_FUNC_DIMMABLE_LIGHT:
        case ZB_FUNC_DIMMABLE_PLUGIN_UNIT:
        case ZB_FUNC_DIMMER_SWITCH:
        case ZB_FUNC_COLOR_TEMP_LIGHT:
        case ZB_FUNC_COLOR_LIGHT:
        case ZB_FUNC_EXTENDED_COLOR_LIGHT:

        /* Metering that rides along with an actuator (smart plug, outlet).
         * Energy is a cumulative counter - showing 0 kWh after a reboot is
         * plainly wrong rather than merely blank - and the electrical
         * quantities are readings like any other sensor's. */
        case ZB_FUNC_ELECTRICAL:
        case ZB_FUNC_ENERGY:
            return true;

        default:
            /* Anything not named above: unknown to this policy, so not cached.
             * A new function type has to be added here deliberately - and to
             * zb_device_manager_write_function_value(), or nothing it caches
             * can be restored. */
            return false;
    }
}

static s_zb_state_record_t *
state_cache_find(uint64_t ieee_addr, bool create)
{
    if (s_records == NULL)
    {
        return NULL;
    }
    for (uint16_t i = 0; i < s_record_count; i++)
    {
        if (s_records[i].ieee_addr == ieee_addr)
        {
            return &s_records[i];
        }
    }
    if (!create || (s_record_count >= ZB_STATE_CACHE_MAX_DEVICES))
    {
        return NULL;
    }
    s_zb_state_record_t *rec = &s_records[s_record_count++];
    memset(rec, 0, sizeof(*rec));
    rec->ieee_addr = ieee_addr;
    return rec;
}

/* ------------------------------------------------------------------------ */

void
zb_device_state_cache_init(void)
{
    s_record_count = 0;
    s_dirty = false;
    s_next_flush_ms = 0;

    if (s_records == NULL)
    {
        s_records = ZB_STATE_CACHE_ALLOC(sizeof(s_zb_state_record_t) * ZB_STATE_CACHE_MAX_DEVICES);
        if (s_records == NULL)
        {
            /* Not fatal: without the table the cache is simply disabled and
             * every device comes up blank, which is the pre-cache behaviour. */
            ZB_LOGE(TAG, "State cache alloc failed (%u bytes), cache disabled",
                    (unsigned)(sizeof(s_zb_state_record_t) * ZB_STATE_CACHE_MAX_DEVICES));
            return;
        }
        memset(s_records, 0, sizeof(s_zb_state_record_t) * ZB_STATE_CACHE_MAX_DEVICES);
    }

    uint32_t size = (uint32_t)ZB_GET_FILE_SIZE(ZB_STATE_CACHE_FILE);
    if (size < sizeof(s_zb_state_file_header_t))
    {
        ZB_LOGI(TAG, "No state cache yet (%s)", ZB_STATE_CACHE_FILE);
        return;
    }

    s_zb_state_file_header_t hdr = {0};
    if (ZB_FILE_READ(ZB_STATE_CACHE_FILE, (uint8_t *)&hdr, sizeof(hdr), 0) != (int)sizeof(hdr))
    {
        ZB_LOGW(TAG, "State cache header unreadable, ignoring");
        return;
    }
    if ((hdr.magic != ZB_STATE_CACHE_MAGIC) || (hdr.version != ZB_STATE_CACHE_VERSION))
    {
        /* A version bump deliberately discards the file rather than migrating:
         * this is a cache, and losing it costs one reporting interval. */
        ZB_LOGW(TAG, "State cache magic/version mismatch (0x%08lX v%lu), discarding",
                (unsigned long)hdr.magic, (unsigned long)hdr.version);
        return;
    }
    if (hdr.count > ZB_STATE_CACHE_MAX_DEVICES)
    {
        ZB_LOGW(TAG, "State cache claims %lu devices, capping at %u",
                (unsigned long)hdr.count, (unsigned)ZB_STATE_CACHE_MAX_DEVICES);
        hdr.count = ZB_STATE_CACHE_MAX_DEVICES;
    }

    uint32_t bytes = hdr.count * (uint32_t)sizeof(s_zb_state_record_t);
    if ((bytes + sizeof(hdr)) > size)
    {
        ZB_LOGW(TAG, "State cache truncated, discarding");
        return;
    }

    /* Read in record-sized chunks: ZB_FILE_READ takes a uint16_t length, and
     * the whole table can exceed 64 KB. */
    for (uint32_t i = 0; i < hdr.count; i++)
    {
        uint32_t offset = (uint32_t)sizeof(hdr) + i * (uint32_t)sizeof(s_zb_state_record_t);
        if (ZB_FILE_READ(ZB_STATE_CACHE_FILE, (uint8_t *)&s_records[i],
                         (uint16_t)sizeof(s_zb_state_record_t), offset)
            != (int)sizeof(s_zb_state_record_t))
        {
            ZB_LOGW(TAG, "State cache record %lu unreadable, keeping %lu",
                    (unsigned long)i, (unsigned long)i);
            s_record_count = (uint16_t)i;
            return;
        }
    }

    uint32_t crc = 0;
    crc = ZB_CRC32(crc, (const uint8_t *)s_records, bytes);
    if (crc != hdr.crc)
    {
        ZB_LOGW(TAG, "State cache CRC mismatch, discarding");
        s_record_count = 0;
        return;
    }

    s_record_count = (uint16_t)hdr.count;
    ZB_LOGI(TAG, "State cache loaded: %u device(s)", (unsigned)s_record_count);
}

void
zb_device_state_cache_restore(s_zb_device_t *device)
{
    if (device == NULL)
    {
        return;
    }
    const s_zb_state_record_t *rec = state_cache_find(device->ieee_addr, false);
    if (rec == NULL)
    {
        return;
    }

    uint8_t restored = 0;
    for (uint8_t v = 0; v < rec->value_count && v < ZB_STATE_CACHE_MAX_VALUES; v++)
    {
        const s_zb_func_value_t *val = &rec->values[v];
        /* Match on sensor_id AND type: a re-interviewed device can reuse an
         * endpoint/cluster pair for a different function. */
        for (uint8_t f = 0; f < device->function_count && f < ZB_MAX_FUNCTIONS; f++)
        {
            s_zb_function_t *func = &device->functions[f];
            if ((func->sensor_id != val->sensor_id) || (func->type != val->type))
            {
                continue;
            }
            if (zb_device_manager_write_function_value(func, val))
            {
                func->value_stale = true;   /* last known, not live */
                restored++;
            }
            break;
        }
    }

    device->last_seen = rec->last_seen;
    device->lqi = rec->lqi;

    /* Liveness is re-derived, not replayed. The cached `online` flag says what
     * was true when the snapshot was taken, which may be many hours ago: a
     * device that left the network while the hub was down would otherwise come
     * back announced as online and stay that way until its timeout expires.
     * Feeding the cached last_seen through the same rule the sweep uses gives
     * the right answer for both cases - and yields UNKNOWN when the RTC has not
     * been set yet, so nothing is claimed on a clockless boot. This is what
     * zigbee2mqtt does when it was down longer than the availability timeout. */
    device->status = rec->online
                       ? zb_device_manager_status_from_last_seen(device, rec->last_seen)
                       : ZB_DEVICE_STATUS_OFFLINE;

    ZB_LOGI(TAG, "0x%llx - restored %u cached value(s), last seen %llu, status %d",
            device->ieee_addr, (unsigned)restored,
            (unsigned long long)rec->last_seen, (int)device->status);
}

void
zb_device_state_cache_mark_dirty(void)
{
    s_dirty = true;
}

void
zb_device_state_cache_forget(uint64_t ieee_addr)
{
    for (uint16_t i = 0; i < s_record_count; i++)
    {
        if (s_records[i].ieee_addr == ieee_addr)
        {
            s_records[i] = s_records[--s_record_count];
            s_dirty = true;
            return;
        }
    }
}

/* Snapshot every known device into the record table. */
static void
state_cache_collect(void)
{
    s_record_count = 0;

    if (s_records == NULL)
    {
        return;
    }

    uint16_t idx = 0;
    s_zb_device_t *device;
    while ((device = zb_device_manager_find_device_from_start_index(&idx)) != NULL)
    {
        if (s_record_count >= ZB_STATE_CACHE_MAX_DEVICES)
        {
            ZB_LOGW(TAG, "State cache full at %u devices", (unsigned)s_record_count);
            break;
        }
        s_zb_func_value_t values[ZB_MAX_FUNCTIONS];
        uint8_t count = zb_device_manager_read_function_values(device->ieee_addr, values);

        s_zb_state_record_t *rec = &s_records[s_record_count];
        memset(rec, 0, sizeof(*rec));
        rec->ieee_addr = device->ieee_addr;
        rec->last_seen = device->last_seen;
        rec->lqi       = device->lqi;
        rec->online    = (device->status == ZB_DEVICE_STATUS_ONLINE) ? 1u : 0u;

        for (uint8_t i = 0; (i < count) && (rec->value_count < ZB_STATE_CACHE_MAX_VALUES); i++)
        {
            if (!zb_device_state_cache_is_cacheable(values[i].type))
            {
                continue;
            }
            rec->values[rec->value_count++] = values[i];
        }
        idx++;
        s_record_count++;
    }
}

void
zb_device_state_cache_flush(void)
{
    if (s_records == NULL)
    {
        return;
    }

    ZB_LOGI(TAG, "Cache flush");
    state_cache_collect();

    s_zb_state_file_header_t hdr = {
        .magic   = ZB_STATE_CACHE_MAGIC,
        .version = ZB_STATE_CACHE_VERSION,
        .count   = s_record_count,
        .crc     = 0,
    };
    uint32_t bytes = (uint32_t)s_record_count * (uint32_t)sizeof(s_zb_state_record_t);
    hdr.crc = ZB_CRC32(0, (const uint8_t *)s_records, bytes);

    /* Write to a temporary and rename, so a reset mid-write leaves the previous
     * snapshot intact rather than a half-file the CRC would reject. */
    remove(ZB_STATE_CACHE_TMP);
    if (ZB_FILE_WRITE(ZB_STATE_CACHE_TMP, (const uint8_t *)&hdr, sizeof(hdr), 0) != (int)sizeof(hdr))
    {
        ZB_LOGE(TAG, "State cache header write failed");
        remove(ZB_STATE_CACHE_TMP);
        return;
    }
    for (uint16_t i = 0; i < s_record_count; i++)
    {
        uint32_t offset = (uint32_t)sizeof(hdr) + (uint32_t)i * (uint32_t)sizeof(s_zb_state_record_t);
        if (ZB_FILE_WRITE(ZB_STATE_CACHE_TMP, (const uint8_t *)&s_records[i],
                          (uint16_t)sizeof(s_zb_state_record_t), offset)
            != (int)sizeof(s_zb_state_record_t))
        {
            ZB_LOGE(TAG, "State cache record %u write failed", (unsigned)i);
            remove(ZB_STATE_CACHE_TMP);
            return;
        }
    }

    remove(ZB_STATE_CACHE_FILE);
    if (rename(ZB_STATE_CACHE_TMP, ZB_STATE_CACHE_FILE) != 0)
    {
        ZB_LOGE(TAG, "State cache rename failed");
        remove(ZB_STATE_CACHE_TMP);
        return;
    }

    s_dirty = false;
    ZB_LOGI(TAG, "State cache saved: %u device(s), %lu bytes",
            (unsigned)s_record_count, (unsigned long)(bytes + sizeof(hdr)));
}

void
zb_device_state_cache_tick(uint32_t now_ms)
{
    if (!s_dirty)
    {
        return;
    }
    if ((s_next_flush_ms != 0) && ((int32_t)(now_ms - s_next_flush_ms) < 0))
    {
        return;
    }
    zb_device_state_cache_flush();
    s_next_flush_ms = now_ms + ZB_STATE_CACHE_FLUSH_MS;
}
