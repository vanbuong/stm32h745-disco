#ifndef THEME_H
#define THEME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THEME_BG 0x12141Au
#define THEME_SURFACE 0x1C212Cu
#define THEME_SURFACE_2 0x262C3Au
#define THEME_ACCENT 0x3D8BFFu
#define THEME_OK 0x3DDC97u
#define THEME_WARN 0xF5A524u
#define THEME_ERR 0xF25F5Cu
#define THEME_TEXT 0xE8ECF1u
#define THEME_MUTED 0x8B93A7u
#define THEME_HAIRLINE 0x3A4154u

#define THEME_HIT_MIN_PX 40
#define THEME_ROW_H 44
#define THEME_STATUS_H 32
#define THEME_APPBAR_H 40
#define THEME_RADIUS_PX 8
#define THEME_PANEL_W 480
#define THEME_PANEL_H 272
#define THEME_CONTENT_H (THEME_PANEL_H - THEME_STATUS_H)

#ifdef __cplusplus
}
#endif

#endif /* THEME_H */
