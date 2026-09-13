#ifndef ZB_APS_GROUP_H
#define ZB_APS_GROUP_H

#include "common/zb_common.h"

#define APS_GROUP_NAME_LEN      16

typedef struct s_zb_aps_group
{
    uint16_t id;
    uint8_t name[APS_GROUP_NAME_LEN];
} s_zb_aps_group_t;

#endif /* ZB_APS_GROUP_H */