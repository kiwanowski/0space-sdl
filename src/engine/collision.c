#include <math.h>

#include "engine.h"

void instance_bbox(const Instance *inst, float px, float py,
                   float *l, float *t, float *r, float *b)
{
    int spr = (inst->mask_index >= 0) ? inst->mask_index : inst->sprite_index;
    if (spr < 0 || spr >= SPRITE_COUNT) {
        *l = *r = px;
        *t = *b = py;
        return;
    }

    px = SDL_roundf(px);
    py = SDL_roundf(py);

    const SpriteDef *sd = &sprite_defs[spr];
    *l = px + (sd->bb_left  - sd->origin_x);
    *r = px + (sd->bb_right - sd->origin_x);
    *t = py + (sd->bb_top    - sd->origin_y);
    *b = py + (sd->bb_bottom - sd->origin_y);
}

static bool boxes_overlap(float al, float at, float ar, float ab,
                          float bl, float bt, float br, float bb)
{
    return al <= br && bl <= ar && at <= bb && bt <= ab;
}

bool instance_meets(const Instance *a, const Instance *b)
{
    if (!a || !b || a == b || !a->active || !b->active) return false;

    float al, at, ar, ab;
    float bl, bt, br, bb;
    instance_bbox(a, a->x, a->y, &al, &at, &ar, &ab);
    instance_bbox(b, b->x, b->y, &bl, &bt, &br, &bb);
    return boxes_overlap(al, at, ar, ab, bl, bt, br, bb);
}

Instance *instance_place(const Instance *self, float px, float py, int obj)
{
    float al, at, ar, ab;
    instance_bbox(self, px, py, &al, &at, &ar, &ab);

    for (int i = 0; i < world.instance_count; i++) {
        Instance *other = &world.instances[i];
        if (!other->active || other == self) continue;
        if (!object_is(other->obj, obj)) continue;

        float bl, bt, br, bb;
        instance_bbox(other, other->x, other->y, &bl, &bt, &br, &bb);
        if (boxes_overlap(al, at, ar, ab, bl, bt, br, bb)) return other;
    }
    return NULL;
}

bool place_meeting(const Instance *self, float px, float py, int obj)
{
    return instance_place(self, px, py, obj) != NULL;
}

Instance *collision_rectangle(float x1, float y1, float x2, float y2,
                              int obj, const Instance *ignore)
{
    if (x1 > x2) { float t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { float t = y1; y1 = y2; y2 = t; }

    for (int i = 0; i < world.instance_count; i++) {
        Instance *other = &world.instances[i];
        if (!other->active || other == ignore) continue;
        if (!object_is(other->obj, obj)) continue;

        float bl, bt, br, bb;
        instance_bbox(other, other->x, other->y, &bl, &bt, &br, &bb);
        if (boxes_overlap(x1, y1, x2, y2, bl, bt, br, bb)) return other;
    }
    return NULL;
}

Instance *collision_circle(float cx, float cy, float radius,
                           int obj, const Instance *ignore)
{
    for (int i = 0; i < world.instance_count; i++) {
        Instance *other = &world.instances[i];
        if (!other->active || other == ignore) continue;
        if (!object_is(other->obj, obj)) continue;

        float bl, bt, br, bb;
        instance_bbox(other, other->x, other->y, &bl, &bt, &br, &bb);

        float nx = cx < bl ? bl : (cx > br ? br : cx);
        float ny = cy < bt ? bt : (cy > bb ? bb : cy);
        float dx = cx - nx, dy = cy - ny;
        if (dx * dx + dy * dy <= radius * radius) return other;
    }
    return NULL;
}

Instance *collision_line(float x1, float y1, float x2, float y2,
                         int obj, const Instance *ignore)
{
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    int steps = (int)len + 1;

    for (int s = 0; s <= steps; s++) {
        float t = steps ? (float)s / steps : 0.0f;
        float px = x1 + dx * t, py = y1 + dy * t;
        Instance *hit = collision_rectangle(px, py, px, py, obj, ignore);
        if (hit) return hit;
    }
    return NULL;
}
