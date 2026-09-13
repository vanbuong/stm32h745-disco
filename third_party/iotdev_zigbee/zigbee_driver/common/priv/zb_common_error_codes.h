#ifndef ZB_COMMON_ERROR_CODES_H_
#define ZB_COMMON_ERROR_CODES_H_

#define ZB_OK                           0
#define ZB_FAIL                         -1

#define ZB_SUCCESS                      0x00
#define ZB_FAILURE                      0x01
#define ZB_INVALID_PARAMETER            0x02

#define ZB_MEM_ERROR                    0x10
#define ZB_BUFFER_FULL                  0x11
#define ZB_UNSUPPORTED_MODE             0x12
#define ZB_MAC_MEM_ERROR                0x13

#define ZB_NWK_NO_ROUTE                 0xCD
#define ZB_ERR_SBL_CRC32                0x30
#define ZB_ERR_SBL_TIMEOUT              0x31
#define ZB_ERR_SBL_NACK                 0x32
#define ZB_ERR_SBL_COMMAND_FAILED       0x33
#define ZB_ERR_SBL_FILE_READ            0x34
#define ZB_ERR_APS_NOT_ALLOWED          0x35

#endif /* ZB_COMMON_ERROR_CODES_H_ */