#include <string.h>

#include "engine/engine.h"
#include "game/game.h"

static void wall_create(Instance *self)
{
    self->image_speed = 0.0f;
}

static void spawn_set_var(Instance *self, const char *name, float value)
{
    if (!strcmp(name, "dir")) self->s.gen.dir = value;
}

void objects_register(void)
{
    object_vtable[OBJ_WALL].create = wall_create;
    object_vtable[OBJ_SPAWN].set_var = spawn_set_var;

    char_register();
    particles_register();
    control_register();
    hazards_register();
    debris_register();
    bombs_register();
    scenery_register();
    background_register();
}
