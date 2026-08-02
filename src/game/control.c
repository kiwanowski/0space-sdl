#include "engine/engine.h"
#include "game/game.h"

#define PLAYERS 1

void wrap_position(Instance *self)
{
    if (room_defs[world.room].control_open < 2) return;
    if (self->x < 0)                  self->x += world.room_width;
    if (self->y < 0)                  self->y += world.room_height;
    if (self->x > world.room_width)   self->x -= world.room_width;
    if (self->y > world.room_height)  self->y -= world.room_height;
}

static void control_room_start(Instance *self)
{
    world.view_count = (globals.players <= 2) ? 2 : globals.players;

    int open = room_defs[world.room].control_open;
    globals.vwrap = world.vwrap = (open == 2 || open == 3);
    globals.hwrap = world.hwrap = (open == 3);

    if (world.room != ROOM_TITLE)
        instance_create(self->x, self->y, OBJ_BG);

    Instance *spawns[64];
    int n = 0;
    for (int i = 0; i < world.instance_count && n < 64; i++) {
        Instance *in = &world.instances[i];
        if (in->active && in->obj == OBJ_SPAWN) spawns[n++] = in;
    }
    if (n == 0) return;

    for (int p = 0; p < PLAYERS; p++) {
        Instance *sp = NULL;
        for (int tries = 0; tries < 64 && !sp; tries++) {
            Instance *cand = spawns[gm_irandom(n - 1)];
            if (!cand->s.gen.state) sp = cand;
        }
        if (!sp) break;
        sp->s.gen.state = 1;

        Instance *player = instance_create(sp->x + 8, sp->y + 8, OBJ_CHAR);
        if (!player) break;

        CharState *c = &player->s.ch;
        c->player = p;
        c->dir = sp->s.gen.dir + 90.0f;
        c->showdir = c->dir;
        c->gundir = c->dir;
        c->camx = player->x;
        c->camy = player->y;
        world.view_angle = -c->dir;
        world.cam_x = player->x;
        world.cam_y = player->y;
    }
}

static void control_draw(Instance *self)
{
    const RoomDef *rd = &room_defs[world.room];
    float rw = (float)world.room_width, rh = (float)world.room_height;

    if (rd->control_open == 0) {
        const float border = 480.0f;
        uint32_t col = (world.room == ROOM_SANDES) ? 0x020030u
                                                   : 0x000000u;
        draw_set_alpha(1.0f);
        draw_set_color(col);
        draw_rectangle(-border, -border, rw + border, 1, false);
        draw_rectangle(-border, 0, 1, rh + border, false);
        draw_rectangle(rw + border, 0, rw - 1, rh + border, false);
        draw_rectangle(0, rh - 1, rw, rh + border, false);
    }

    if (world.room == ROOM_LAVATUNNEL) {
        const float border = 800.0f;
        draw_set_alpha(1.0f);
        draw_set_color(0x000000u);
        draw_rectangle(-border, -border, 1, rh + border, false);
        draw_rectangle(rw + border, -border, rw - 1, rh + border, false);
    }
    (void)self;
}

void control_register(void)
{
    object_vtable[OBJ_CONTROL].room_start = control_room_start;
    object_vtable[OBJ_CONTROL].draw       = control_draw;
}
