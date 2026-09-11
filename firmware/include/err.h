#ifndef ERR_H
#define ERR_H

#include <stdint.h>

typedef int32_t err_t;

#define ERR_OK 0
#define ERR_BUSY (-1)
#define ERR_TIMEOUT (-2)
#define ERR_NOMEM (-3)
#define ERR_NOENT (-4)
#define ERR_INVAL (-5)
#define ERR_IO (-6)
#define ERR_NOSPC (-7)
#define ERR_DENIED (-8)
#define ERR_CORRUPT (-9)
#define ERR_UNSUPPORTED (-10)
#define ERR_OVERFLOW (-11)

#endif /* ERR_H */
