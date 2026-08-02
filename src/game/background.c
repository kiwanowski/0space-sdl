#include <math.h>

#include "engine/engine.h"
#include "game/game.h"

static void bg_create(Instance *self)
{
    self->s.gen.hspeed = 0.0f;
    self->visible = true;
    self->depth = 500;
}

static void bg_draw(Instance *self)
{
    const RoomDef *rd = &room_defs[world.room];
    if (rd->background_count < 2) return;

    int cw, ch;
    render_canvas_size(&cw, &ch);

    float x0 = (float)world.view_x, y0 = (float)world.view_y;
    float x1 = x0 + cw,             y1 = y0 + ch;

    float vx = world.cam_x - world.view_w / 2.0f;
    float vy = world.cam_y - world.view_h / 2.0f;

    for (int layer = 0; layer < 2; layer++) {
        float divv = (layer == 0) ? 2.0f : 3.0f;
        int   bg   = rd->backgrounds[layer].bg;
        if (bg < 0 || bg >= BACKGROUND_COUNT) continue;

        float bw = background_defs[bg].w;
        float bh = background_defs[bg].h;
        if (bw <= 0 || bh <= 0) continue;

        float xp = (self->s.gen.hspeed / divv) + vx / divv;
        float yp = vy / divv;

        xp = x0 + fmodf(xp - x0, bw);
        if (xp > x0) xp -= bw;
        yp = y0 + fmodf(yp - y0, bh);
        if (yp > y0) yp -= bh;

        for (float py = yp; py < y1; py += bh)
            for (float px = xp; px < x1; px += bw)
                draw_background_part(bg, 0, 0, (int)bw, (int)bh, px, py);
    }

    if (world.room == ROOM_BAY) self->s.gen.hspeed += 6.0f;
}

void background_register(void)
{
    object_vtable[OBJ_BG].create = bg_create;
    object_vtable[OBJ_BG].draw   = bg_draw;
}
