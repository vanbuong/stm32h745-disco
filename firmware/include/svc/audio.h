#ifndef AUDIO_H
#define AUDIO_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

err_t audio_play(const char *path);
err_t audio_pause(void);
err_t audio_resume(void);
err_t audio_stop(void);
err_t audio_set_volume(uint8_t pct);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_H */
