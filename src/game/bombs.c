#include <math.h>

#include "engine/engine.h"
#include "game/game.h"

#define BOMB_FUSE   75
#define SHOCKWAVES  20

static float mirand(float v) { return v - gm_random(v * 2.0f); }

static void play_land_sound(void)
{
    static const int lands[] = {
        SND_S_METALLAND1, SND_S_METALLAND2, SND_S_METALLAND3,
        SND_S_METALLAND4, SND_S_METALLAND5,
    };
    sound_play(lands[gm_irandom(4)]);
}

/* ------------------------------------------------------------------ */

void explode(float x, float y, int team)
{
    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active || in->obj != OBJ_CHAR) continue;
        float d = gm_point_distance(in->x, in->y, x, y);
        if (d < 100.0f)
            in->s.ch.shake = SDL_max(100.0f, 100.0f - d) / 4.0f;
    }

    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active || in->obj != OBJ_SPACEMINE) continue;
        if (gm_point_distance(in->x, in->y, x, y) < 60.0f &&
            !collision_line(in->x, in->y, x, y, OBJ_BLOCK, in))
            in->s.bomb.dead = 1;
    }

    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active || in->obj != OBJ_SPACEBOMB) continue;
        float dir = gm_point_direction(in->x, in->y, x, y) - 180.0f;
        if (gm_point_distance(in->x, in->y, x, y) < 60.0f) {
            in->s.bomb.xspeed += gm_lengthdir_x(5.0f, dir);
            in->s.bomb.yspeed += gm_lengthdir_y(5.0f, dir);
        }
    }

    sound_play(gm_irandom(1) ? SND_S_EXPLODE : SND_S_EXPLODE2);

    for (int i = 0; i < 14; i++) {
        Instance *p = instance_create(x, y, OBJ_BULLETPART);
        if (!p) continue;
        p->sprite_index = SPR_SMOKEBLAST;
        p->image_speed = 0.6f + gm_random(0.4f);
        float d = gm_random(360.0f), s = 0.5f + gm_random(2.5f);
        p->s.mv.hspeed = gm_lengthdir_x(s, d);
        p->s.mv.vspeed = gm_lengthdir_y(s, d);
        p->s.mv.friction = 0.02f;
    }

    for (int i = 0; i < SHOCKWAVES; i++) {
        Instance *w = instance_create(x, y, OBJ_SHOCKWAVE);
        if (!w) continue;
        float d = i * (360.0f / SHOCKWAVES);
        w->image_speed = 0.8f;
        w->image_angle = d;
        w->s.mv.hspeed = gm_lengthdir_x(5.0f, d);
        w->s.mv.vspeed = gm_lengthdir_y(5.0f, d);
        w->s.mv.friction = 0.02f;
        w->s.mv.owner = team;
    }

    for (int i = 0; i < 20; i++) {
        Instance *p = instance_create(x + mirand(2), y + mirand(2), OBJ_BULLETPART);
        if (!p) continue;
        p->sprite_index = SPR_BULLETPART;
        p->image_speed = 0.2f + gm_random(0.5f);
        float d = gm_random(360.0f), s = 1.0f + gm_random(7.0f);
        p->s.mv.hspeed = gm_lengthdir_x(s, d);
        p->s.mv.vspeed = gm_lengthdir_y(s, d);
    }

    for (int i = 0; i < 6; i++) {
        Instance *p = instance_create(x + mirand(10), y + mirand(10), OBJ_BULLETPART);
        if (!p) continue;
        p->sprite_index = SPR_BLAST;
        p->image_speed = 0.7f + gm_random(0.3f);
        float d = gm_random(360.0f), s = 3.0f + gm_random(1.0f);
        p->s.mv.hspeed = gm_lengthdir_x(s, d);
        p->s.mv.vspeed = gm_lengthdir_y(s, d);
    }
}

static void spacebomb_create(Instance *self)
{
    BombState *b = &self->s.bomb;
    b->tmax = BOMB_FUSE;
    b->t = b->tmax;
    self->image_speed = 0.0f;
}

static void spacebomb_bounce_x(Instance *self)
{
    self->s.bomb.xspeed = -self->s.bomb.xspeed / 1.1f;
    self->s.bomb.spin /= 1.5f;
    play_land_sound();
}

static void spacebomb_bounce_y(Instance *self)
{
    self->s.bomb.yspeed = -self->s.bomb.yspeed / 1.1f;
    self->s.bomb.spin /= 1.5f;
    play_land_sound();
}

static void spacebomb_begin_step(Instance *self)
{
    BombState *b = &self->s.bomb;
    pushed_by_sand(self, &b->yspeed, 3);
    if (b->t > 0) {
        b->t--;
    } else {
        explode(self->x, self->y, b->team);
        instance_destroy(self);
        return;
    }
    self->image_speed = SDL_min(1.0f - (float)b->t / b->tmax, 1.0f);

    b->flash += world.view_count * self->image_speed;
    if (b->flash > sprite_defs[SPR_SPACEBOMB].frame_count) b->flash = 0.0f;
}

static void spacebomb_step(Instance *self)
{
    BombState *b = &self->s.bomb;

    wrap_position(self);

    mover_step(self, b->xspeed, b->yspeed, OBJ_MEGABLOCK,
               spacebomb_bounce_x, spacebomb_bounce_y);
    if (!self->active) return;

    Instance *bullet = instance_place(self, self->x, self->y, OBJ_BULLET);
    if (bullet) {
        b->team = bullet->s.mv.owner;
        instance_destroy(bullet);
        explode(self->x, self->y, b->team);
        instance_destroy(self);
        return;
    }
    if (instance_place(self, self->x, self->y, OBJ_DEADLY) || lava_hits_any(self)) {
        explode(self->x, self->y, b->team);
        instance_destroy(self);
    }
}

static void spacebomb_draw(Instance *self)
{
    BombState *b = &self->s.bomb;
    int f = (int)floorf(b->flash);
    if (f == 0 || f == 2) {
        if (f == 0 && b->flash2 != f) sound_play(SND_S_BOMBEEP);
        uint32_t glow = (f == 0) ? merge_color(0x00FF00u, 0x000000u, 0.6f)
                                 : merge_color(0xFF00FFu, 0x000000u, 0.6f);
        draw_set_blend(BLEND_GLOW);
        draw_set_color(glow);
        draw_set_alpha(1.0f);
        draw_circle(self->x, self->y, 10.0f + gm_random(5.0f), false);
        draw_set_blend(BLEND_NORMAL);
    }

    draw_sprite_ext(SPR_SPACEBOMB, (int)self->image_index, self->x, self->y,
                    1.0f, 1.0f, b->rot, 0xFFFFFF, 1.0f);
    b->rot += b->spin;
    b->flash2 = f;
}

static void spacemine_create(Instance *self)
{
    sound_play(SND_S_MINEWALL);
    self->image_speed = 0.0f;
}

static void spacemine_step(Instance *self)
{
    BombState *b = &self->s.bomb;

    if (!collision_circle(self->x, self->y, 12.0f, OBJ_MEGABLOCK, self)) {
        Instance *nb = instance_create(self->x, self->y, OBJ_SPACEBOMB);
        if (nb) {
            nb->s.bomb.team = b->team;
            nb->s.bomb.xspeed = gm_lengthdir_x(-1.0f, self->image_angle);
            nb->s.bomb.yspeed = gm_lengthdir_y(-1.0f, self->image_angle);
            nb->s.bomb.spin = mirand(10.0f);
        }
        instance_destroy(self);
        return;
    }

    if (b->settime > 53) {
        b->image_single += 0.1f;
        if (b->image_single > 2.8f) b->image_single = 1.0f;
        if (b->set == 0) { sound_play(SND_S_BOMBEEP); b->set = 1; }
    } else {
        b->settime++;
    }

    Instance *target = NULL;
    float dis = 30.0f;
    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active || in->obj != OBJ_CHAR) continue;
        float d = gm_point_distance(in->x, in->y, self->x, self->y);
        if (d < dis && !collision_line(in->x, in->y, self->x, self->y, OBJ_BLOCK, in)) {
            target = in;
            dis = d;
        }
    }

    if ((target && b->set == 1) || b->deadtime > 5) {
        explode(self->x, self->y, b->team);
        instance_destroy(self);
        return;
    }

    Instance *bullet = instance_place(self, self->x, self->y, OBJ_BULLET);
    if (bullet) {
        b->team = bullet->s.mv.owner;
        b->dead = 1;
        b->deadtime = 5;
        instance_destroy(bullet);
    }

    if (b->dead == 1) b->deadtime++;
}

static void spacemine_draw(Instance *self)
{
    draw_sprite_ext(SPR_SPACEMINE, (int)self->s.bomb.image_single,
                    self->x, self->y, 1.0f, 1.0f, self->image_angle,
                    0xFFFFFF, 1.0f);
}

/* ---------------------------- shockwave --------------------------- */

static void shockwave_slow(Instance *self) { self->s.mv.hspeed /= 5.0f; self->s.mv.vspeed /= 5.0f; }

static void shockwave_step(Instance *self)
{
    MoverState *m = &self->s.mv;
    wrap_position(self);

    float spd = gm_point_distance(0, 0, m->hspeed, m->vspeed);
    if (spd > 0) {
        float dir = gm_point_direction(0, 0, m->hspeed, m->vspeed);
        spd = SDL_max(0.0f, spd - m->friction);
        m->hspeed = gm_lengthdir_x(spd, dir);
        m->vspeed = gm_lengthdir_y(spd, dir);
    }
    mover_step(self, m->hspeed, m->vspeed, OBJ_WALL, shockwave_slow, shockwave_slow);
    if (!self->active) return;

    if (self->sprite_index >= 0) {
        int n = sprite_defs[self->sprite_index].frame_count;
        if (n > 0 && self->image_index + self->image_speed >= n)
            instance_destroy(self);
    }
}

void bombs_register(void)
{
    object_vtable[OBJ_SPACEBOMB].create     = spacebomb_create;
    object_vtable[OBJ_SPACEBOMB].begin_step = spacebomb_begin_step;
    object_vtable[OBJ_SPACEBOMB].step       = spacebomb_step;
    object_vtable[OBJ_SPACEBOMB].draw       = spacebomb_draw;

    object_vtable[OBJ_SPACEMINE].create = spacemine_create;
    object_vtable[OBJ_SPACEMINE].step   = spacemine_step;
    object_vtable[OBJ_SPACEMINE].draw   = spacemine_draw;

    object_vtable[OBJ_SHOCKWAVE].step = shockwave_step;
}
