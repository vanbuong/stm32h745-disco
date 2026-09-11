#ifndef ZNP_MT_H
#define ZNP_MT_H

#include "err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZNP_SOF 0xFEu
#define ZNP_PAYLOAD_MAX 250u

uint8_t znp_mt_fcs(uint8_t len, uint8_t cmd0, uint8_t cmd1, const uint8_t *payload);

err_t znp_mt_encode(uint8_t cmd0, uint8_t cmd1, const uint8_t *payload, uint8_t len, uint8_t *out,
                    size_t out_sz, size_t *out_len);

err_t znp_mt_decode(const uint8_t *in, size_t in_len, uint8_t *cmd0, uint8_t *cmd1,
                    uint8_t *payload, uint8_t payload_max, uint8_t *len);

#ifdef __cplusplus
}
#endif

#endif /* ZNP_MT_H */
