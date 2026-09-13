#ifndef SVC_LOG_H
#define SVC_LOG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { LOG_ERROR = 0, LOG_WARN = 1, LOG_INFO = 2, LOG_DEBUG = 3 } log_lvl_t;

#define LOG_LINE_MAX 192

/* Line format: core,ts_ms,lvl,mod,msg  (ts_ms is monotonic milliseconds). */

void log_init(void);
void log_reset(void);
void log_set_level(log_lvl_t max);
log_lvl_t log_level(void);
void log_set_core(const char *core);
void log_set_clock(uint32_t (*millis)(void));
void log_set_sink(void (*fn)(const char *line, size_t n));
void log_write(log_lvl_t lvl, const char *mod, const char *fmt, ...);
void log_hex(log_lvl_t lvl, const char *mod, const void *data, size_t n);
const char *log_last(void);
uint32_t log_count(void);

#ifdef __cplusplus
}
#endif

#endif /* SVC_LOG_H */
