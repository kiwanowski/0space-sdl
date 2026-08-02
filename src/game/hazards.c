#include <math.h>
#include <string.h>

#include "engine/engine.h"
#include "game/game.h"

static float mirand(float v) { return v - gm_random(v * 2.0f); }
static float rchunk(float v, float n) { return (float)gm_round(v / n) * n; }

static void deadly_scenery_create(Instance *self)
{
    GenericState *g = &self->s.gen;

    Instance *target = instance_nearest(self->x, self->y, OBJ_BLOCK);
    g->dir = target ? gm_point_direction(self->x, self->y, target->x, target->y) : 0.0f;

    self->x = rchunk(self->x, 16.0f);
    self->y = rchunk(self->y, 16.0f);
    g->anim = 0.0f;
    g->imgspd = 0.1f + gm_random(0.2f);
    self->image_speed = 0.0f;
}

static void deathspikes_draw(Instance *self)
{
    GenericState *g = &self->s.gen;
    draw_sprite_ext(SPR_DEATHSPIKES, (int)g->anim,
                    self->x + 8 + gm_lengthdir_x(1, g->dir),
                    self->y + 8 + gm_lengthdir_y(1, g->dir),
                    1.0f, 1.0f, g->dir, 0xFFFFFF, 1.0f);
    g->anim += g->imgspd;
}

static void laser_create(Instance *self)
{
    self->x = rchunk(self->x, 16.0f);
    self->y = rchunk(self->y, 16.0f);
}

static void jumpad_create(Instance *self)
{
    if (place_meeting(self, self->x, self->y - 16, OBJ_BLOCK)) {
        self->y += 16;
        self->image_yscale = -1.0f;
    }
    self->image_speed = 0.5f + gm_random(0.2f);
}

static void pusher_step(Instance *self)
{
    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active || in->obj != OBJ_CHAR) continue;
        if (!instance_place(in, in->x, in->y, OBJ_PUSHER)) continue;
        if (in->s.ch.onground == 0 && in->s.ch.xspeed < 8.0f)
            in->s.ch.xspeed += 0.5f;
    }
    (void)self;
}

static void destructable_break(Instance *self, float hitdir)
{
    tile_layer_delete_at(-6, self->x, self->y);

    sound_play(gm_irandom(1) == 0 ? SND_S_CRUMBLE : SND_S_CRUMBLE2);

    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active || in->obj != OBJ_CHAR) continue;
        if (gm_point_distance(in->x, in->y, self->x + 8, self->y + 8) < 18.0f) {
            float d = gm_point_direction(in->x, in->y, self->x + 8, self->y + 8);
            in->s.ch.xspeed += gm_lengthdir_x(-1.0f, d);
            in->s.ch.yspeed += gm_lengthdir_y(-1.0f, d);
        }
    }

    for (int i = 0; i < 5; i++) {
        Instance *r = instance_create(self->x + 8 + mirand(3), self->y + 8 + mirand(3),
                                      OBJ_ROCKDEBRI);
        if (r) {
            float d = hitdir + mirand(40.0f);
            float s = 3.0f + gm_random(3.0f);
            r->s.mv.hspeed   = gm_lengthdir_x(s, d);
            r->s.mv.vspeed   = gm_lengthdir_y(s, d);
            r->s.mv.friction = 5.0f + gm_random(20.0f);
        }
        Instance *sd = instance_create(self->x + 8 + mirand(3), self->y + 8 + mirand(3),
                                       OBJ_SAND);
        if (sd) {
            float d = hitdir + mirand(40.0f);
            float s = 1.0f + gm_random(4.0f);
            sd->s.mv.hspeed = gm_lengthdir_x(s, d);
            sd->s.mv.vspeed = gm_lengthdir_y(s, d);
        }
    }
    instance_destroy(self);
}

static void destructable_step(Instance *self)
{
    Instance *b = instance_place(self, self->x, self->y, OBJ_BULLET);
    if (b) {
        float hitdir = gm_point_direction(0, 0, b->s.mv.hspeed, b->s.mv.vspeed);
        instance_destroy(b);
        destructable_break(self, hitdir);
        return;
    }
    Instance *sw = instance_place(self, self->x, self->y, OBJ_SHOCKWAVE);
    if (sw) {
        float hitdir = gm_point_direction(sw->x, sw->y, self->x, self->y);
        destructable_break(self, hitdir);
    }
}

static void movingblock_create(Instance *self)
{
    GenericState *g = &self->s.gen;
    g->hspeed = 0.0f;
    g->vspeed = 0.0f;
    g->dir = 1.0f;
    g->anim = 0.0f;
    self->image_speed = 0.0f;
}

static void movingblock_end_step(Instance *self)
{
    GenericState *g = &self->s.gen;

    if (g->hspeed < g->dir) g->hspeed += 0.1f;
    if (g->hspeed > g->dir) g->hspeed -= 0.1f;
    if (g->anim > 0) g->anim -= 0.2f;

    int steps = (int)fabsf(g->hspeed);
    int sx = (g->hspeed > 0) - (g->hspeed < 0);
    for (int i = 0; i < steps; i++) {
        for (int j = 0; j < world.instance_count; j++) {
            Instance *ch = &world.instances[j];
            if (!ch->active || ch->obj != OBJ_CHAR) continue;
            if (instance_place(self, self->x + sx, self->y, OBJ_CHAR) != ch) continue;
            if (!place_meeting(ch, ch->x + sx, ch->y, OBJ_BLOCK))
                ch->x += sx;
        }
        if (!place_meeting(self, self->x + sx, self->y, OBJ_WALL)) {
            self->x += sx;
        } else {
            g->hspeed = 0.0f;
            g->dir = -g->dir;
            g->anim = 3.0f;
            break;
        }
    }
}

static void lava_consume(Instance *self);

static void lava_set_var(Instance *self, const char *name, float value)
{
    LavaState *l = &self->s.lava;
    if      (!strcmp(name, "x1"))  l->x1 = value;
    else if (!strcmp(name, "y1"))  l->y1 = value;
    else if (!strcmp(name, "x2"))  l->x2 = value;
    else if (!strcmp(name, "y2"))  l->y2 = value;
    else if (!strcmp(name, "xx1")) l->xx1 = value;
    else if (!strcmp(name, "yy1")) l->yy1 = value;
    else if (!strcmp(name, "xx2")) l->xx2 = value;
    else if (!strcmp(name, "yy2")) l->yy2 = value;
    else if (!strcmp(name, "maxtime")) l->maxtime = value;
}

static void lava_create(Instance *self)
{
    LavaState *l = &self->s.lava;
    l->x1 = self->x;
    l->y1 = self->y;
    l->x2 = self->x;
    l->y2 = self->y;
    l->maxtime = 90.0f;
    l->time = 0.0f;
    self->visible = true;
    self->image_speed = 0.0f;
}

static void lava_step(Instance *self)
{
    LavaState *l = &self->s.lava;
    if (l->maxtime <= 0.0f) l->maxtime = 90.0f;

    if (!l->started) {
        l->started = true;
        l->time = -l->maxtime - 80.0f;
        l->ax = l->x1;  l->ay = l->y1;
        l->bx = l->xx1; l->by = l->yy1;
    }

    l->ax = l->x1  + (l->x2  - l->x1)  * l->t;
    l->ay = l->y1  + (l->y2  - l->y1)  * l->t;
    l->bx = l->xx1 + (l->xx2 - l->xx1) * l->t;
    l->by = l->yy1 + (l->yy2 - l->yy1) * l->t;

    l->t = (1.0f + sinf(l->time * (1.0f / l->maxtime))) / 2.0f;
    if (l->t < 0.1f || l->t > 0.9f)      l->time += 0.5f;
    else if (l->t < 0.2f || l->t > 0.8f) l->time += 0.8f;
    else                                 l->time += 1.0f;

    l->anim += 0.5f;

    self->x = l->ax;
    self->y = l->ay;

    lava_consume(self);
}

static void lava_draw(Instance *self)
{
    LavaState *l = &self->s.lava;

    uint32_t color = (((int)l->anim) & 1)
                   ? 0x0000FFu
                   : merge_color(0x0000FFu, 0x00FFFFu, 0.1f + gm_random(0.1f));

    draw_set_alpha(1.0f);
    draw_set_color(color);
    draw_rectangle(l->ax, l->ay, l->bx, l->by, false);

    const int spr = SPR_LAVA;

    int across = gm_round(fabsf(l->bx - l->ax) / 16.0f);
    int down   = gm_round(fabsf(l->by - l->ay) / 16.0f);

    for (int i = 0; i < across; i++) {
        float px = l->ax + i * 16.0f + 8.0f;
        draw_sprite_ext(spr, (int)(l->anim + i * 16 / 5.0f), px, l->ay,
                        1, 1, 0.0f, color, 1.0f);
        draw_sprite_ext(spr, (int)(l->anim + i * 16 / 5.0f), px, l->by,
                        1, 1, 180.0f, color, 1.0f);
    }
    for (int i = 0; i < down; i++) {
        float py = l->ay + i * 16.0f + 8.0f;
        draw_sprite_ext(spr, (int)(l->anim + i * 16 / 5.0f), l->ax, py,
                        1, 1, 90.0f, color, 1.0f);
        draw_sprite_ext(spr, (int)(l->anim + i * 16 / 5.0f), l->bx, py,
                        1, 1, -90.0f, color, 1.0f);
    }
}

bool lava_hits(const Instance *self, const Instance *other)
{
    const LavaState *l = &self->s.lava;
    float lx1 = SDL_min(l->ax, l->bx), lx2 = SDL_max(l->ax, l->bx);
    float ly1 = SDL_min(l->ay, l->by), ly2 = SDL_max(l->ay, l->by);

    float bl, bt, br, bb;
    instance_bbox(other, other->x, other->y, &bl, &bt, &br, &bb);
    return bl <= lx2 && lx1 <= br && bt <= ly2 && ly1 <= bb;
}

static bool is_lava(int obj)
{
    return obj == OBJ_DEATHLAVA || obj == OBJ_SUPERLAVA || obj == OBJ_GREENLAVA;
}

bool lava_hits_any(const Instance *other)
{
    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active || !is_lava(in->obj)) continue;
        if (lava_hits(in, other)) return true;
    }
    return false;
}

static void lava_consume(Instance *self)
{
    Instance *gib = NULL, *bullet = NULL;

    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active) continue;
        if (in->obj == OBJ_GIBS) {
            if (!gib && lava_hits(self, in)) gib = in;
        } else if (in->obj == OBJ_BULLET) {
            if (!bullet && lava_hits(self, in)) bullet = in;
        }
    }

    if (gib) {
        spawn_flame(gib->x, gib->y, gib->s.mv.hspeed, gib->s.mv.vspeed);
        instance_destroy(gib);
    }
    if (bullet) instance_destroy(bullet);
}

void hazards_register(void)
{
    object_vtable[OBJ_DEATHSPIKES].create = deadly_scenery_create;
    object_vtable[OBJ_DEATHSPIKES].draw   = deathspikes_draw;

    object_vtable[OBJ_LASER].create  = laser_create;
    object_vtable[OBJ_LASERH].create = laser_create;

    object_vtable[OBJ_JUMPAD].create = jumpad_create;
    object_vtable[OBJ_PUSHER].step   = pusher_step;

    object_vtable[OBJ_DESTRUCTABLE].step = destructable_step;

    object_vtable[OBJ_MOVINGBLOCK].create   = movingblock_create;
    object_vtable[OBJ_MOVINGBLOCK].end_step = movingblock_end_step;

    const int lavas[] = { OBJ_DEATHLAVA, OBJ_SUPERLAVA, OBJ_GREENLAVA };
    for (unsigned i = 0; i < sizeof lavas / sizeof *lavas; i++) {
        object_vtable[lavas[i]].create  = lava_create;
        object_vtable[lavas[i]].step    = lava_step;
        object_vtable[lavas[i]].draw    = lava_draw;
        object_vtable[lavas[i]].set_var = lava_set_var;
    }
}
