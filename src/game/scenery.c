#include <math.h>
#include <string.h>

#include "engine/engine.h"
#include "game/game.h"

Globals globals = { .players = 1, .bayopen = 0 };

static float mirand(float v) { return v - gm_random(v * 2.0f); }

static float approach(float from, float to, float step)
{
    if (from < to) return SDL_min(from + step, to);
    return SDL_max(from - step, to);
}

static float mid_value(float a, float b, float amount)
{
    return a + amount * (b - a);
}

static void parallax_draw(Instance *self)
{
    float vx = world.cam_x - world.view_w / 2.0f;
    float vy = world.cam_y - world.view_h / 2.0f;
    float size = self->s.gen.imgspd > 0 ? self->s.gen.imgspd : 1.0f;
    if (self->sprite_index < 0) return;
    draw_sprite_ext(self->sprite_index, 0,
                    self->x + vx / 3.0f, self->y + vy / 3.0f,
                    size, size, 0.0f, 0xFFFFFF, 1.0f);
}

static void parallax_set_var(Instance *self, const char *name, float value)
{
    if (!strcmp(name, "size")) self->s.gen.imgspd = value;
}

static void sat_create(Instance *self) { self->s.gen.imgspd = 1.0f; }

static void sandfalldown_create(Instance *self)
{
    self->s.gen.dir = 1.0f;
}

static void sandfallup_create(Instance *self)
{
    self->s.gen.dir = -1.0f;
    self->image_speed = -1.0f;
}

void pushed_by_sand(Instance *self, float *yspeed, int reps)
{
    Instance *col = collision_circle(self->x, self->y, 4.0f, OBJ_SANDFALL, self);
    if (!col) return;

    float push = col->s.gen.dir;
    if (reps <= 0) reps = 3;

    for (int i = 0; i < reps; i++) {
        Instance *sd = instance_create(self->x + mirand(5), self->y + mirand(5), OBJ_SAND);
        if (!sd) continue;
        sd->s.mv.hspeed = mirand(3);
        sd->s.mv.vspeed = mirand(1) + push * 3.0f;
    }

    if (!place_meeting(self, self->x, self->y + push * 3.0f, OBJ_MEGABLOCK))
        *yspeed = approach(*yspeed, push * 3.0f, fabsf(push) / 3.0f);
}

static void baydoor_create(Instance *self)
{
    Instance *a = instance_find(OBJ_A);
    GenericState *g = &self->s.gen;
    if (a && self->y > a->y) {
        self->image_index = 0;
        g->anim = self->y + 32.0f;
    } else {
        self->image_index = 1;
        g->anim = self->y - 32.0f;
    }
    self->image_speed = 0.0f;
}

static void baydoor_step(Instance *self)
{
    GenericState *g = &self->s.gen;
    self->y = mid_value(self->y, globals.bayopen ? g->anim : self->ystart, 0.2f);
}

static void baydoor_end_step(Instance *self)
{
    Instance *a = instance_find(OBJ_A);
    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active || in->obj != OBJ_CHAR) continue;
        if (!instance_place(in, in->x, in->y, OBJ_BAYDOOR)) continue;
        float d = a ? gm_point_direction(in->x, in->y, a->x, a->y) - 180.0f + mirand(30)
                    : gm_random(360.0f);
        char_kill(in, d);
    }
}

static void baydoorcontrol_create(Instance *self)
{
    self->s.gen.timer = 0;
    globals.bayopen = 0;
    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (in->active && in->obj == OBJ_WARNINGLIGHTS) in->visible = false;
    }
}

static void baydoorcontrol_step(Instance *self)
{
    GenericState *g = &self->s.gen;
    Instance *a = instance_find(OBJ_A);
    g->timer++;

    if (g->timer == 200 - 60) {
        for (int i = 0; i < world.instance_count; i++) {
            Instance *in = &world.instances[i];
            if (in->active && in->obj == OBJ_WARNINGLIGHTS) in->visible = true;
        }
        sound_play(SND_S_ALARM);
    }

    if (g->timer >= 200 && g->timer <= 400) {
        instance_create(128 + gm_random(400), 160 + gm_random(200), OBJ_WINDPART);
        globals.bayopen = 1;
        if (g->timer == 200) {
            sound_stop(SND_S_ALARM);
            sound_play(SND_S_WIND);
        }
        for (int i = 0; i < world.instance_count && a; i++) {
            Instance *in = &world.instances[i];
            if (!in->active || in->obj != OBJ_CHAR) continue;
            CharState *c = &in->s.ch;
            if (c->onground != 0) continue;
            if (in->x < a->x && fabsf(c->xspeed) < 10.0f) c->xspeed -= 0.2f;
            else                                          c->xspeed += 0.2f;
            if (in->y < a->y) c->yspeed += 0.1f;
            else              c->yspeed -= 0.1f;
        }
    } else if (g->timer > 400) {
        globals.bayopen = 0;
        sound_stop(SND_S_WIND);
    }
}

static void atom_create(Instance *self)
{
    self->s.gen.vspeed = 1.0f;
}

static void atom_step(Instance *self)
{
    GenericState *g = &self->s.gen;
    float d = gm_point_direction(self->x, self->y, 80 + 9 + 8, 64);
    g->hspeed += gm_lengthdir_x(0.4f, d);
    g->vspeed += gm_lengthdir_y(0.4f, d);
    self->x += g->hspeed;
    self->y += g->vspeed;
}

static void comet_create(Instance *self)
{
    GenericState *g = &self->s.gen;
    float s = 4.0f + gm_random(3.0f);
    g->hspeed = gm_lengthdir_x(s, 270 - 45);
    g->vspeed = gm_lengthdir_y(s, 270 - 45);
}

static void comet_step(Instance *self)
{
    GenericState *g = &self->s.gen;
    self->x += g->hspeed;
    self->y += g->vspeed;
    if (self->sprite_index >= 0) {
        int n = sprite_defs[self->sprite_index].frame_count;
        if (n > 0 && self->image_index + self->image_speed >= n)
            instance_destroy(self);
    }
}

static void windpart_create(Instance *self)
{
    GenericState *g = &self->s.gen;
    self->image_speed = 0.1f + gm_random(1.0f);
    g->timer = (int)(40 + gm_random(100));
    g->hspeed = mirand(0.5f);
    g->vspeed = mirand(0.5f);
}

static void windpart_step(Instance *self)
{
    GenericState *g = &self->s.gen;
    if (place_meeting(self, self->x, self->y, OBJ_BLOCK)) g->timer -= 100;
    if (--g->timer < 0) { instance_destroy(self); return; }
    self->x += g->hspeed;
    self->y += g->vspeed;
}

static void charred_create(Instance *self)
{
    for (int i = 0; i < 3; i++) {
        Instance *p = instance_create(self->x + mirand(2), self->y + mirand(2), OBJ_BULLETPART);
        if (!p) continue;
        p->sprite_index = SPR_BLAST;
        p->image_speed = 0.7f + gm_random(0.3f);
        float d = gm_random(360.0f), s = 3.0f + gm_random(1.0f);
        p->s.mv.hspeed = gm_lengthdir_x(s, d);
        p->s.mv.vspeed = gm_lengthdir_y(s, d);
    }
    for (int i = 0; i < 6; i++) {
        Instance *p = instance_create(self->x + mirand(2), self->y + mirand(2), OBJ_BULLETPART);
        if (!p) continue;
        p->sprite_index = SPR_BULLETPART;
        p->image_speed = 0.2f + gm_random(0.5f);
        float d = gm_random(360.0f), s = 1.0f + gm_random(7.0f);
        p->s.mv.hspeed = gm_lengthdir_x(s, d);
        p->s.mv.vspeed = gm_lengthdir_y(s, d);
    }
}

void spawn_charred(float x, float y, float showdir, float spin,
                   float xspeed, float yspeed)
{
    Instance *c = instance_create(x, y, OBJ_CHARRED);
    if (!c) return;
    c->image_angle   = showdir;
    c->s.mv.hspeed   = xspeed;
    c->s.mv.vspeed   = yspeed;
    c->s.mv.friction = spin;
}

static void charred_bounce_x(Instance *self)
{
    self->s.mv.hspeed = -self->s.mv.hspeed / 1.5f;
    self->s.mv.friction /= 1.5f;
}

static void charred_bounce_y(Instance *self)
{
    self->s.mv.vspeed = -self->s.mv.vspeed / 1.5f;
    self->s.mv.friction /= 1.5f;
}

static void charred_step(Instance *self)
{
    MoverState *m = &self->s.mv;
    m->life++;
    mover_step(self, m->hspeed, m->vspeed, OBJ_BLOCK, charred_bounce_x, charred_bounce_y);
    if (!self->active) return;

    if (m->life > 100) { instance_destroy(self); return; }

    if (++m->timer > 2) {
        m->timer = 0;
        spawn_flame(self->x, self->y, m->hspeed, m->vspeed);
    }
    self->image_angle += m->friction;
}

static void engineflame_create(Instance *self)
{
    if (self->sprite_index >= 0)
        self->image_index = (float)gm_irandom(sprite_defs[self->sprite_index].frame_count - 1);
}

static void bloodonwall_step(Instance *self)
{
    if (!instance_place(self, self->x, self->y, OBJ_MEGABLOCK))
        instance_destroy(self);
}

static void animation_end_destroy(Instance *self)
{
    if (self->sprite_index < 0) return;
    int n = sprite_defs[self->sprite_index].frame_count;
    if (n > 0 && self->image_index + self->image_speed >= n)
        instance_destroy(self);
}

void scenery_register(void)
{
    object_vtable[OBJ_SANDFALLDOWN].create = sandfalldown_create;
    object_vtable[OBJ_SANDFALLUP].create   = sandfallup_create;

    object_vtable[OBJ_BAYDOOR].create   = baydoor_create;
    object_vtable[OBJ_BAYDOOR].step     = baydoor_step;
    object_vtable[OBJ_BAYDOOR].end_step = baydoor_end_step;

    object_vtable[OBJ_BAYDOORCONTROL].create = baydoorcontrol_create;
    object_vtable[OBJ_BAYDOORCONTROL].step   = baydoorcontrol_step;

    object_vtable[OBJ_ATOM].create = atom_create;
    object_vtable[OBJ_ATOM].step   = atom_step;

    object_vtable[OBJ_COMET].create = comet_create;
    object_vtable[OBJ_COMET].step   = comet_step;

    object_vtable[OBJ_MOON].draw     = parallax_draw;
    object_vtable[OBJ_MOON].set_var  = parallax_set_var;
    object_vtable[OBJ_MOONS].draw    = parallax_draw;
    object_vtable[OBJ_MOONS].set_var = parallax_set_var;
    object_vtable[OBJ_SAT].create    = sat_create;
    object_vtable[OBJ_SAT].draw      = parallax_draw;
    object_vtable[OBJ_SAT].set_var   = parallax_set_var;

    object_vtable[OBJ_WINDPART].create = windpart_create;
    object_vtable[OBJ_WINDPART].step   = windpart_step;

    object_vtable[OBJ_CHARRED].create = charred_create;
    object_vtable[OBJ_CHARRED].step   = charred_step;

    object_vtable[OBJ_ENGINEFLAME].create = engineflame_create;
    object_vtable[OBJ_BLOODONWALL].step   = bloodonwall_step;
    object_vtable[OBJ_ASH].step           = animation_end_destroy;
    object_vtable[OBJ_JUMPDUST].step      = animation_end_destroy;
}
