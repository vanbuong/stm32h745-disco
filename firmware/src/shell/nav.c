#include "ui/nav.h"

#include <stddef.h>

void nav_init(nav_stack_t *s)
{
    unsigned i;

    if (s == NULL) {
        return;
    }
    for (i = 0u; i < NAV_STACK_MAX; i++) {
        s->frames[i].id = NULL;
        s->frames[i].args = NULL;
    }
    s->depth = 0u;
    s->gen = 0u;
}

err_t nav_push(nav_stack_t *s, const char *id, void *args)
{
    if (s == NULL || id == NULL || id[0] == '\0') {
        return ERR_INVAL;
    }
    if (s->depth >= NAV_STACK_MAX) {
        return ERR_NOSPC;
    }
    s->frames[s->depth].id = id;
    s->frames[s->depth].args = args;
    s->depth++;
    s->gen++;
    return ERR_OK;
}

err_t nav_pop(nav_stack_t *s)
{
    if (s == NULL) {
        return ERR_INVAL;
    }
    if (s->depth == 0u) {
        return ERR_NOENT;
    }
    s->depth--;
    s->frames[s->depth].id = NULL;
    s->frames[s->depth].args = NULL;
    s->gen++;
    return ERR_OK;
}

void nav_home(nav_stack_t *s)
{
    if (s == NULL || s->depth == 0u) {
        return;
    }
    while (s->depth > 0u) {
        s->depth--;
        s->frames[s->depth].id = NULL;
        s->frames[s->depth].args = NULL;
    }
    s->gen++;
}

const nav_frame_t *nav_top(const nav_stack_t *s)
{
    if (s == NULL || s->depth == 0u) {
        return NULL;
    }
    return &s->frames[s->depth - 1u];
}
