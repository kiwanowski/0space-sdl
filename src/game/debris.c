#include <math.h>

#include "engine/engine.h"
#include "game/game.h"

static float mirand(float v) { return v - gm_random(v * 2.0f); }

static void gibs_create(Instance *self)
{
    self->image_speed = 0.0f;
}

static void gib_bounce_x(Instance *self)
{
    self->s.mv.hspeed   = -self->s.mv.hspeed / 1.5f;
    self->s.mv.friction /= 1.5f;
}

static void gib_bounce_y(Instance *self)
{
    self->s.mv.vspeed   = -self->s.mv.vspeed / 1.5f;
    self->s.mv.friction /= 1.5f;
}

static void gibs_step(Instance *self)
{
    MoverState *m = &self->s.mv;
    pushed_by_sand(self, &m->vspeed, 1);
    wrap_position(self);
    mover_step(self, m->hspeed, m->vspeed, OBJ_MEGABLOCK, gib_bounce_x, gib_bounce_y);
    if (!self->active) return;

    self->image_angle += m->friction;
    m->hspeed *= 0.99f;
    m->vspeed *= 0.99f;

    if (++m->timer > 600) instance_destroy(self);
}

static void rockdebri_create(Instance *self)
{
    self->image_speed = 0.1f + gm_random(0.6f);
    self->image_angle = gm_random(360.0f);
}

static void rockdebri_step(Instance *self)
{
    MoverState *m = &self->s.mv;
    wrap_position(self);
    mover_step(self, m->hspeed, m->vspeed, OBJ_MEGABLOCK, gib_bounce_x, gib_bounce_y);
    if (!self->active) return;

    self->image_angle += m->friction;

    if (self->sprite_index >= 0) {
        int n = sprite_defs[self->sprite_index].frame_count;
        if (n > 0 && self->image_index + self->image_speed >= n)
            instance_destroy(self);
    }
}

static void blood_create(Instance *self)
{
    self->image_speed = 0.4f + gm_random(0.4f);
    self->s.mv.friction = 0.05f;
}

static void blood_splat(Instance *self)
{
    Instance *w = instance_create(self->x, self->y, OBJ_BLOODONWALL);
    if (w) {
        w->image_angle = self->image_angle;
        w->image_speed = 0.0f;
        w->image_index = (float)gm_irandom(3);
    }
    instance_destroy(self);
}

static void blood_step(Instance *self)
{
    MoverState *m = &self->s.mv;
    float spd = gm_point_distance(0, 0, m->hspeed, m->vspeed);
    if (spd > 0) {
        float dir = gm_point_direction(0, 0, m->hspeed, m->vspeed);
        spd = SDL_max(0.0f, spd - m->friction);
        m->hspeed = gm_lengthdir_x(spd, dir);
        m->vspeed = gm_lengthdir_y(spd, dir);
    }
    mover_step(self, m->hspeed, m->vspeed, OBJ_BLOCK, blood_splat, blood_splat);
    if (!self->active) return;

    if (++m->timer > 90 || spd <= 0.05f) instance_destroy(self);
}

static void sand_create(Instance *self)
{
    self->s.mv.friction = 0.08f;
}

static void sand_step(Instance *self)
{
    MoverState *m = &self->s.mv;
    float spd = gm_point_distance(0, 0, m->hspeed, m->vspeed);
    if (spd > 0) {
        float dir = gm_point_direction(0, 0, m->hspeed, m->vspeed);
        spd = SDL_max(0.0f, spd - m->friction);
        m->hspeed = gm_lengthdir_x(spd, dir);
        m->vspeed = gm_lengthdir_y(spd, dir);
    }
    self->x += m->hspeed;
    self->y += m->vspeed;
    if (++m->timer > 120 || spd <= 0.02f) instance_destroy(self);
}

static void deadob_create(Instance *self)
{
    self->visible = false;
    self->s.gen.timer = 0;
}

static void deadob_step(Instance *self)
{
    GenericState *g = &self->s.gen;
    world.camera_manual = true;
    world.cam_x = g->hspeed;
    world.cam_y = g->vspeed;
    if (++g->timer > 60) room_restart();
}

void spawn_death_effect(float x, float y, float dir, float otherdir, int skin)
{
    sound_play(SND_S_KILL);

    for (int side = 0; side < 2; side++) {
        float a = (side == 0) ? dir + 180.0f : dir;
        Instance *g = instance_create(x + gm_lengthdir_x(4, a),
                                      y + gm_lengthdir_y(4, a), OBJ_GIBS);
        if (!g) continue;
        float d = dir + (side == 0 ? 40.0f : -40.0f) + mirand(30.0f);
        float s = 1.0f + gm_random(3.0f);
        g->s.mv.hspeed = gm_lengthdir_x(s, d);
        g->s.mv.vspeed = gm_lengthdir_y(s, d);
        g->s.mv.friction = (5.0f + gm_random(10.0f)) * (gm_irandom(1) ? -1.0f : 1.0f);
        g->image_index = (float)(side + skin * 2);
    }

    for (int i = 0; i < 25; i++) {
        Instance *g = instance_create(x, y, OBJ_GIBS);
        if (g) {
            float d = otherdir + mirand(20.0f);
            float s = 1.0f + gm_random(5.0f);
            g->s.mv.hspeed = gm_lengthdir_x(s, d);
            g->s.mv.vspeed = gm_lengthdir_y(s, d);
            g->s.mv.friction = (5.0f + gm_random(10.0f)) * (gm_irandom(1) ? -1.0f : 1.0f);
            g->image_index = (float)(8 + gm_irandom(4));
        }
        Instance *b = instance_create(x, y, OBJ_BLOOD);
        if (b) {
            float d = otherdir + mirand(40.0f);
            float s = 1.0f + gm_random(5.0f);
            b->s.mv.hspeed = gm_lengthdir_x(s, d);
            b->s.mv.vspeed = gm_lengthdir_y(s, d);
            b->image_angle = d;
        }
    }
}

void debris_register(void)
{
    object_vtable[OBJ_GIBS].create      = gibs_create;
    object_vtable[OBJ_GIBS].step        = gibs_step;
    object_vtable[OBJ_ROCKDEBRI].create = rockdebri_create;
    object_vtable[OBJ_ROCKDEBRI].step   = rockdebri_step;

    object_vtable[OBJ_BLOOD].create = blood_create;
    object_vtable[OBJ_BLOOD].step   = blood_step;

    object_vtable[OBJ_SAND].create = sand_create;
    object_vtable[OBJ_SAND].step   = sand_step;

    object_vtable[OBJ_DEADOB].create = deadob_create;
    object_vtable[OBJ_DEADOB].step   = deadob_step;
}
