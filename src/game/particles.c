#include <math.h>
#include <stdlib.h>

#include "engine/engine.h"
#include "game/game.h"

bool mover_step(Instance *self, float dx, float dy, int solid,
                void (*on_hit_x)(Instance *), void (*on_hit_y)(Instance *))
{
    MoverState *m = &self->s.mv;
    bool hit = false;

    m->xc += dx;
    m->yc += dy;
    int h = gm_round(m->xc);
    int v = gm_round(m->yc);
    m->xc -= h;
    m->yc -= v;

    int sx = (h > 0) - (h < 0);
    for (int i = 0; i < abs(h); i++) {
        if (!place_meeting(self, self->x + sx, self->y, solid)) {
            self->x += sx;
        } else {
            hit = true;
            if (on_hit_x) on_hit_x(self);
            break;
        }
    }
    if (!self->active) return true;

    int sy = (v > 0) - (v < 0);
    for (int i = 0; i < abs(v); i++) {
        if (!place_meeting(self, self->x, self->y + sy, solid)) {
            self->y += sy;
        } else {
            hit = true;
            if (on_hit_y) on_hit_y(self);
            break;
        }
    }
    return hit;
}

static float mirand(float v) { return v - gm_random(v * 2.0f); }

static void bullet_hit(Instance *self)
{
    self->s.mv.wallhit = true;
    instance_destroy(self);
}

static void bullet_step(Instance *self)
{
    self->s.mv.life++;
    wrap_position(self);
    mover_step(self, self->s.mv.hspeed, self->s.mv.vspeed, OBJ_BLOCK,
               bullet_hit, bullet_hit);
}

static void bullet_destroy(Instance *self)
{
    MoverState *m = &self->s.mv;
    float dir = gm_point_direction(0, 0, m->hspeed, m->vspeed);

    if (m->wallhit) {
        static const int hits[] = { SND_S_HITWALL, SND_S_HITWALL2, SND_S_HITWALL3 };
        sound_play(hits[gm_irandom(2)]);
    }

    for (int i = 0; i < 3; i++) {
        Instance *p = instance_create(self->x, self->y, OBJ_BULLETPART);
        if (!p) continue;
        float d = dir - 180.0f + mirand(10.0f);
        float s = 1.0f + gm_random(2.0f);
        p->s.mv.hspeed = gm_lengthdir_x(s, d);
        p->s.mv.vspeed = gm_lengthdir_y(s, d);
        p->s.mv.friction = 0.02f;
        p->image_speed = 0.5f + gm_random(0.3f);
        p->image_angle = d - 180.0f;
    }

    for (int i = 0; i < 5; i++) {
        Instance *p = instance_create(self->x, self->y, OBJ_BULLETPART);
        if (!p) continue;
        float d = dir - 180.0f + mirand(180.0f);
        float s = 5.0f + gm_random(3.0f);
        p->sprite_index  = SPR_BULLETPART;
        p->image_speed   = 0.2f + gm_random(0.5f);
        p->s.mv.hspeed   = gm_lengthdir_x(s, d);
        p->s.mv.vspeed   = gm_lengthdir_y(s, d);
        p->s.mv.friction = 0.2f;
        p->image_angle   = 90.0f * (float)gm_irandom(3);
    }

    Instance *puff = instance_create(self->x, self->y, OBJ_BULLETPART);
    if (puff) puff->sprite_index = SPR_BLAST;
}

static void bulletpart_step(Instance *self)
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

    if (self->sprite_index >= 0) {
        int n = sprite_defs[self->sprite_index].frame_count;
        if (n > 0 && self->image_index + self->image_speed >= n)
            instance_destroy(self);
    }
}

static void sparkline_create(Instance *self)
{
    MoverState *m = &self->s.mv;
    m->color = 0x00FF00;
    if (gm_irandom(3) == 1) m->color = 0x00FFFF;
    if (gm_irandom(3) == 1) m->color = 0xFFFFFF;
    m->timer = 5 + gm_irandom(10);
    m->friction = 1.2f + gm_random(0.1f);
    self->visible = true;
}

static void sparkline_step(Instance *self)
{
    MoverState *m = &self->s.mv;
    if (--m->timer < 1) { instance_destroy(self); return; }

    self->x += m->hspeed;
    self->y += m->vspeed;
    m->hspeed /= m->friction;
    m->vspeed /= m->friction;
}

static void sparkline_draw(Instance *self)
{
    MoverState *m = &self->s.mv;
    draw_set_color(m->color);
    draw_set_alpha(1.0f);
    draw_line_col(self->x, self->y, self->x - m->hspeed, self->y - m->vspeed);
}

static void chargeup_create(Instance *self)
{
    self->image_speed = 0.7f + gm_random(0.5f);
    self->s.gen.state = -1;
}

static void chargeup_step(Instance *self)
{
    GenericState *g = &self->s.gen;

    Instance *target = NULL;
    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (in->active && in->id == g->state) { target = in; break; }
    }

    if (target) {
        float bx = target->x + gm_lengthdir_x(8.0f, target->s.ch.gundir);
        float by = target->y + gm_lengthdir_y(8.0f, target->s.ch.gundir);
        if (gm_point_distance(self->x, self->y, bx, by) < 2.0f) {
            instance_destroy(self);
            return;
        }
        g->dir = gm_point_direction(self->x, self->y, bx, by);
    }

    g->hspeed += gm_lengthdir_x(0.5f, g->dir);
    g->vspeed += gm_lengthdir_y(0.5f, g->dir);
    self->x += g->hspeed;
    self->y += g->vspeed;

    if (self->sprite_index >= 0) {
        int n = sprite_defs[self->sprite_index].frame_count;
        if (n > 0 && self->image_index + self->image_speed >= n)
            instance_destroy(self);
    }
}

void spawn_chargeup(Instance *player, float gundir, float xspeed, float yspeed)
{
    for (int i = 0; i < 3; i++) {
        float scatter = gundir + mirand(80.0f);
        float out     = 3.0f + gm_random(6.0f);
        float px = player->x + gm_lengthdir_x(8.0f, gundir) + gm_lengthdir_x(out, scatter);
        float py = player->y + gm_lengthdir_y(8.0f, gundir) + gm_lengthdir_y(out, scatter);

        Instance *c = instance_create(px, py, OBJ_CHARGEUP);
        if (!c) continue;
        c->s.gen.hspeed = xspeed / 2.0f;
        c->s.gen.vspeed = yspeed / 2.0f;
        c->s.gen.dir    = gm_point_direction(px, py,
                                             player->x + gm_lengthdir_x(8.0f, gundir),
                                             player->y + gm_lengthdir_y(8.0f, gundir));
        c->s.gen.state  = player->id;
    }
}

static void jumpdust_create(Instance *self)
{
    self->image_speed = 0.4f;
}

static void jumpdust_step(Instance *self)
{
    if (self->sprite_index >= 0) {
        int n = sprite_defs[self->sprite_index].frame_count;
        if (n > 0 && self->image_index + self->image_speed >= n)
            instance_destroy(self);
    }
}

void particles_register(void)
{
    object_vtable[OBJ_BULLET].step        = bullet_step;
    object_vtable[OBJ_BULLET].destroy     = bullet_destroy;

    object_vtable[OBJ_BULLETPART].step    = bulletpart_step;

    object_vtable[OBJ_SPARKLINE].create   = sparkline_create;
    object_vtable[OBJ_SPARKLINE].step     = sparkline_step;
    object_vtable[OBJ_SPARKLINE].draw     = sparkline_draw;

    object_vtable[OBJ_CHARGEUP].create    = chargeup_create;
    object_vtable[OBJ_CHARGEUP].step      = chargeup_step;

    object_vtable[OBJ_JUMPDUST].create    = jumpdust_create;
    object_vtable[OBJ_JUMPDUST].step      = jumpdust_step;
}
