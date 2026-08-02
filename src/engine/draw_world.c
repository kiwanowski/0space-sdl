#include <stdlib.h>

#include "engine.h"

typedef struct {
    int   depth;
    int   seq;
    bool  is_tile;
    const void *ptr;
} DrawItem;

static int compare_items(const void *a, const void *b)
{
    const DrawItem *x = a, *y = b;
    if (x->depth != y->depth) return y->depth - x->depth;
    return x->seq - y->seq;
}

void world_draw(void)
{
    static DrawItem items[MAX_INSTANCES + 4096];
    int n = 0;

    const RoomDef *rd = &room_defs[world.room];
    for (int i = 0; i < rd->tile_count && n < (int)(sizeof items / sizeof *items); i++) {
        if (i < MAX_TILES && world.tile_deleted[i]) continue;
        items[n].depth = rd->tiles[i].depth;
        items[n].seq = n;
        items[n].is_tile = true;
        items[n].ptr = &rd->tiles[i];
        n++;
    }
    for (int i = 0; i < world.instance_count && n < (int)(sizeof items / sizeof *items); i++) {
        Instance *in = &world.instances[i];
        if (!in->active || !in->visible) continue;
        items[n].depth = in->depth;
        items[n].seq = n;
        items[n].is_tile = false;
        items[n].ptr = in;
        n++;
    }

    qsort(items, n, sizeof *items, compare_items);

    for (int i = 0; i < n; i++) {
        if (items[i].is_tile) {
            const RoomTile *t = items[i].ptr;
            draw_background_part(t->bg, t->sx, t->sy, t->w, t->h,
                                 (float)t->x, (float)t->y);
        } else {
            Instance *in = (Instance *)items[i].ptr;
            if (object_vtable[in->obj].draw) {
                object_vtable[in->obj].draw(in);
            } else if (in->sprite_index >= 0) {
                float ox[4] = { 0 }, oy[4] = { 0 };
                int n = 1;
                if (world.vwrap) {
                    if (in->y < world.view_h)                        { ox[n] = 0; oy[n++] =  world.room_height; }
                    if (in->y > world.room_height - world.view_h)    { ox[n] = 0; oy[n++] = -world.room_height; }
                }
                if (world.hwrap) {
                    if (in->x < world.view_w)                        { oy[n] = 0; ox[n++] =  world.room_width; }
                    if (in->x > world.room_width - world.view_w)     { oy[n] = 0; ox[n++] = -world.room_width; }
                }
                for (int k = 0; k < n; k++)
                    draw_sprite_ext(in->sprite_index, (int)in->image_index,
                                    in->x + ox[k], in->y + oy[k],
                                    in->image_xscale, in->image_yscale,
                                    in->image_angle, in->image_blend, in->image_alpha);
            }
        }
    }
}
