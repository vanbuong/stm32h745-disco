#include "game/game_sim.h"

#include <string.h>

const game_module_t *game_module_by_id(const char *id)
{
    if (id == NULL || id[0] == '\0' || strcmp(id, "brick") == 0) {
        return game_brick_module();
    }
    if (strcmp(id, "chip8") == 0) {
        return game_chip8_module();
    }
    return NULL;
}
