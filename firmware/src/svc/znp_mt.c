#include "svc/znp_mt.h"

#include <string.h>

uint8_t znp_mt_fcs(uint8_t len, uint8_t cmd0, uint8_t cmd1, const uint8_t *payload)
{
    uint8_t fcs = (uint8_t)(len ^ cmd0 ^ cmd1);
    uint8_t i;
    for (i = 0; i < len; i++) {
        fcs = (uint8_t)(fcs ^ payload[i]);
    }
    return fcs;
}

err_t znp_mt_encode(uint8_t cmd0, uint8_t cmd1, const uint8_t *payload, uint8_t len, uint8_t *out,
                    size_t out_sz, size_t *out_len)
{
    size_t need = 5u + (size_t)len;
    if (out == NULL || out_len == NULL) {
        return ERR_INVAL;
    }
    if (len > 0u && payload == NULL) {
        return ERR_INVAL;
    }
    if (out_sz < need) {
        return ERR_NOSPC;
    }
    out[0] = ZNP_SOF;
    out[1] = len;
    out[2] = cmd0;
    out[3] = cmd1;
    if (len > 0u) {
        memcpy(&out[4], payload, len);
    }
    out[4u + len] = znp_mt_fcs(len, cmd0, cmd1, payload);
    *out_len = need;
    return ERR_OK;
}

err_t znp_mt_decode(const uint8_t *in, size_t in_len, uint8_t *cmd0, uint8_t *cmd1,
                    uint8_t *payload, uint8_t payload_max, uint8_t *len)
{
    uint8_t plen;
    uint8_t fcs;
    const uint8_t *pl = NULL;

    if (in == NULL || cmd0 == NULL || cmd1 == NULL || len == NULL) {
        return ERR_INVAL;
    }
    if (in_len < 5u) {
        return ERR_CORRUPT;
    }
    if (in[0] != ZNP_SOF) {
        return ERR_CORRUPT;
    }
    plen = in[1];
    if ((size_t)plen + 5u > in_len) {
        return ERR_CORRUPT;
    }
    if (plen > payload_max) {
        return ERR_NOSPC;
    }
    if (plen > 0u) {
        pl = &in[4];
        if (payload == NULL) {
            return ERR_INVAL;
        }
    }
    fcs = in[4u + plen];
    if (fcs != znp_mt_fcs(plen, in[2], in[3], pl)) {
        return ERR_CORRUPT;
    }
    *cmd0 = in[2];
    *cmd1 = in[3];
    *len = plen;
    if (plen > 0u) {
        memcpy(payload, pl, plen);
    }
    return ERR_OK;
}
