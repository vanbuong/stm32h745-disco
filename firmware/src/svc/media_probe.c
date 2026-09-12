#include "svc/media.h"

#include <string.h>

static int ascii_eq_ci(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        char ca = *a;
        char cb = *b;
        if (ca >= 'A' && ca <= 'Z') {
            ca = (char)(ca - 'A' + 'a');
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = (char)(cb - 'A' + 'a');
        }
        if (ca != cb) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

media_kind_t media_probe_ext(const char *path)
{
    const char *dot;
    if (path == NULL) {
        return MEDIA_KIND_NONE;
    }
    dot = strrchr(path, '.');
    if (dot == NULL || dot[1] == '\0') {
        return MEDIA_KIND_NONE;
    }
    dot++;
    if (ascii_eq_ci(dot, "txt") || ascii_eq_ci(dot, "md") || ascii_eq_ci(dot, "c") ||
        ascii_eq_ci(dot, "h") || ascii_eq_ci(dot, "log")) {
        return MEDIA_KIND_TEXT;
    }
    if (ascii_eq_ci(dot, "jpg") || ascii_eq_ci(dot, "jpeg") || ascii_eq_ci(dot, "png") ||
        ascii_eq_ci(dot, "bmp")) {
        return MEDIA_KIND_IMAGE;
    }
    if (ascii_eq_ci(dot, "mp3") || ascii_eq_ci(dot, "wav")) {
        return MEDIA_KIND_AUDIO;
    }
    if (ascii_eq_ci(dot, "ch8") || ascii_eq_ci(dot, "c8")) {
        return MEDIA_KIND_GAME;
    }
    return MEDIA_KIND_NONE;
}
