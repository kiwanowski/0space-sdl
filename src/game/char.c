#include <math.h>
#include <stdlib.h>

#include "engine/engine.h"
#include "game/game.h"

#define ROOM_SPEED 30

#define CHAR_BORDER (-100.0f)

static const struct { float start, stop, speed; } ANIM[4] = {
    { 0,  1,  0.02f },
    { 2,  5,  0.20f },
    { 6,  7,  0.04f },
    { 8, 11, -1.00f },
};

enum { ANIM_STAY, ANIM_RUN, ANIM_JUMP, ANIM_JUMPHOLD };

static float approach(float from, float to, float step)
{
    if (from < to) return SDL_min(from + step, to);
    return SDL_max(from - step, to);
}

static int fall(int t) { return t > 0 ? t - 1 : t; }

static float angle_between(float a, float b)
{
    float d = b - a;
    while (d >= 180.0f) d -= 360.0f;
    while (d < -180.0f) d += 360.0f;
    return d;
}

static float mid_angle(float a, float b, float amount)
{
    return a + angle_between(a, b) * amount;
}

static float mid_value(float a, float b, float amount)
{
    return a + amount * (b - a);
}

static float mirand(float v) { return v - gm_random(v * 2.0f); }

static int button_update(int prev, bool down)
{
    if (down) return prev > 0 ? prev + 1 : 1;
    return prev > 0 ? -1 : 0;
}

static void play_step_sound(void)
{
    static const int steps[] = {
        SND_S_METALSTEP1, SND_S_METALSTEP2, SND_S_METALSTEP3,
        SND_S_METALSTEP4, SND_S_METALSTEP5,
    };
    sound_play(steps[gm_irandom(4)]);
}

static void play_land_sound(void)
{
    static const int lands[] = {
        SND_S_METALLAND1, SND_S_METALLAND2, SND_S_METALLAND3,
        SND_S_METALLAND4, SND_S_METALLAND5,
    };
    sound_play(lands[gm_irandom(4)]);
}

static void char_create(Instance *self)
{
    CharState *c = &self->s.ch;

    c->maxspeed  = 1.5f;
    c->accel     = 0.5f;
    c->fric      = 0.3f;
    c->dir       = 0.0f;
    c->hs        = 1;
    c->hss       = 1.0f;
    c->jumpspd   = 0.5f;
    c->reloadset = 4 * ROOM_SPEED;
    c->knifeset  = 17;
    c->ammo      = 5;
    c->bombammo  = 3;
    c->noact     = 10;
    c->shoot     = 2;
    c->camx      = self->x;
    c->camy      = self->y;
    c->animation = ANIM_STAY;

    self->sprite_index = SPR_CHAR;
    self->image_speed  = 1.0f;

    world.camera_manual = true;
    world.cam_x = self->x;
    world.cam_y = self->y;
}

static void char_hit_horizontal(Instance *self)
{
    CharState *c = &self->s.ch;
    if ((c->shot == 0 && c->inair == 0) || c->inair > 0) {
        if (c->xspeed > 0) c->dir = 90.0f;
        if (c->xspeed < 0) c->dir = 270.0f;
    }
    play_land_sound();
    c->xspeed = 0.0f;
    c->land = 10;
}

static void char_hit_vertical(Instance *self)
{
    CharState *c = &self->s.ch;
    if ((c->shot == 0 && c->inair == 0) || c->inair > 0) {
        if (c->yspeed > 0) c->dir = 0.0f;
        if (c->yspeed < 0) c->dir = 180.0f;
    }
    play_land_sound();
    c->yspeed = 0.0f;
    c->land = 10;
}

static void char_move(Instance *self, float dx, float dy)
{
    CharState *c = &self->s.ch;

    c->xc += dx;
    c->yc += dy;
    int h = gm_round(c->xc);
    int v = gm_round(c->yc);
    c->xc -= h;
    c->yc -= v;

    int sx = (h > 0) - (h < 0);
    for (int i = 0; i < abs(h); i++) {
        if (!place_meeting(self, self->x + sx, self->y, OBJ_MEGABLOCK)) {
            self->x += sx;
        } else {
            char_hit_horizontal(self);
            break;
        }
    }

    int sy = (v > 0) - (v < 0);
    for (int i = 0; i < abs(v); i++) {
        if (!place_meeting(self, self->x, self->y + sy, OBJ_MEGABLOCK)) {
            self->y += sy;
        } else {
            char_hit_vertical(self);
            break;
        }
    }
}

static void char_read_input(Instance *self)
{
    CharState *c = &self->s.ch;
    int p = c->player;

    c->left   = button_update(c->left,   input_held(p, IN_LEFT));
    c->right  = button_update(c->right,  input_held(p, IN_RIGHT));
    c->up     = button_update(c->up,     input_held(p, IN_UP));
    c->down   = button_update(c->down,   input_held(p, IN_DOWN));
    c->shoot  = button_update(c->shoot,  input_held(p, IN_SHOOT));
    c->jump   = button_update(c->jump,   input_held(p, IN_JUMP));
    c->bomb   = button_update(c->bomb,   input_held(p, IN_BOMB));
    c->knifeb = button_update(c->knifeb, input_held(p, IN_KNIFE));

    if (c->noact > 0) {
        c->shoot = 0;
        c->jump = 0;
    }
}

static void char_animate(Instance *self)
{
    CharState *c = &self->s.ch;
    float start = ANIM[c->animation].start;
    float stop  = ANIM[c->animation].stop;
    float spd   = ANIM[c->animation].speed;

    if (c->animation == ANIM_JUMPHOLD)
        c->image = start + (c->jumpspd / 4.0f) * (stop - start);

    if (spd > 0) {
        c->image  += spd;
        c->bounce += spd / 2.0f;
    }
    if (c->image > stop || c->animation != c->oldanimation)
        c->image = start;
    if (c->bounce > 1.0f) c->bounce = 0.0f;

    if ((int)floorf(c->image) == 4 && c->image2 != (int)floorf(c->image))
        play_step_sound();
    c->image2 = (int)floorf(c->image);
}

static void char_shoot(Instance *self, float shotdir)
{
    CharState *c = &self->s.ch;

    sound_play(gm_irandom(1) ? SND_S_LASER : SND_S_LASER2);
    c->shake = 5.0f;
    c->reload = c->reloadset;
    c->shot = 10;

    for (int i = 0; i < 5; i++) {
        Instance *sp = instance_create(self->x, self->y, OBJ_SPARKLINE);
        if (sp) {
            float d = shotdir + mirand(10.0f);
            float s = 8.0f + gm_random(8.0f);
            sp->s.mv.hspeed = gm_lengthdir_x(s, d);
            sp->s.mv.vspeed = gm_lengthdir_y(s, d);
        }
    }

    Instance *b = instance_create(self->x, self->y, OBJ_BULLET);
    if (b) {
        b->s.mv.hspeed = gm_lengthdir_x(12.0f, shotdir);
        b->s.mv.vspeed = gm_lengthdir_y(12.0f, shotdir);
        b->s.mv.owner = c->player;
        b->image_angle = gm_point_direction(0, 0, b->s.mv.hspeed, b->s.mv.vspeed);
    }

    Instance *f = instance_create(self->x, self->y, OBJ_BULLETPART);
    if (f) {
        f->sprite_index = SPR_FLASH;
        f->image_angle  = shotdir;
    }

    /* recoil */
    c->xspeed += gm_lengthdir_x(2.0f, shotdir - 180.0f);
    c->yspeed += gm_lengthdir_y(2.0f, shotdir - 180.0f);
}

static void char_swing_knife(Instance *self, float shotdir)
{
    CharState *c = &self->s.ch;
    c->knifeclash = 0;
    c->shake = 1.0f;
    sound_play(gm_irandom(1) ? SND_S_KNIFE : SND_S_KNIFE2);
    c->knife = c->knifeset;
    c->knifewait = c->knifeset;

    if (c->onground && c->aimdir != 90.0f) {
        c->xspeed += gm_lengthdir_x(3.0f, shotdir);
        c->yspeed += gm_lengthdir_y(3.0f, shotdir);
    }
}

static void hand_camera_to_deadob(Instance *self)
{
    CharState *c = &self->s.ch;
    Instance *d = instance_create(self->x, self->y, OBJ_DEADOB);
    if (d) {
        d->s.gen.hspeed = c->camx;
        d->s.gen.vspeed = c->camy;
    }
}

void char_kill(Instance *self, float otherdir)
{
    CharState *c = &self->s.ch;

    hand_camera_to_deadob(self);
    spawn_death_effect(self->x, self->y, c->dir, otherdir, c->skin);
    instance_destroy(self);
}

static void char_burn(Instance *self, float momentum)
{
    CharState *c = &self->s.ch;

    sound_play(SND_S_FLAMEBURST);
    hand_camera_to_deadob(self);
    spawn_charred(self->x, self->y, c->showdir,
                  (fabsf(c->xspeed + c->yspeed) / 2.0f) * c->hss,
                  c->xspeed * momentum, c->yspeed * momentum);
    instance_destroy(self);
}

static void char_begin_step(Instance *self)
{
    pushed_by_sand(self, &self->s.ch.yspeed, 3);
}

static void char_step(Instance *self)
{
    CharState *c = &self->s.ch;

    if (c->noact > 0) c->noact--;

    char_animate(self);
    char_read_input(self);

    float fx = gm_lengthdir_x(1.0f, c->dir - 90.0f);
    float fy = gm_lengthdir_y(1.0f, c->dir - 90.0f);
    if (place_meeting(self, self->x + fx, self->y + fy, OBJ_MEGABLOCK)) c->onground++;
    else                                                                c->onground = 0;

    if (c->onground > 2) {
        c->inair = 0;
        c->dir = (float)gm_round(c->dir / 90.0f) * 90.0f;

        if (c->jump == 0)
            c->animation = (gm_point_distance(0, 0, c->xspeed, c->yspeed) > 0)
                         ? ANIM_RUN : ANIM_STAY;

        c->maxspeed = (c->jumphold > 20) ? 0.2f : 1.5f;

        if (c->right > 0) {
            c->xspeed = approach(c->xspeed, gm_lengthdir_x(c->maxspeed, c->dir), c->accel);
            c->yspeed = approach(c->yspeed, gm_lengthdir_y(c->maxspeed, c->dir), c->accel);
        }
        if (c->left > 0) {
            c->xspeed = approach(c->xspeed, gm_lengthdir_x(c->maxspeed, c->dir - 180.0f), c->accel);
            c->yspeed = approach(c->yspeed, gm_lengthdir_y(c->maxspeed, c->dir - 180.0f), c->accel);
        }
        if (c->left <= 0 && c->right <= 0) {
            c->xspeed = approach(c->xspeed, 0.0f, c->fric);
            c->yspeed = approach(c->yspeed, 0.0f, c->fric);
        }

        if (c->jump > 0) {
            if (c->jumphold == 0) sound_play(SND_S_CROUCH);
            c->animation = ANIM_JUMPHOLD;
            if (c->jumpspd < 4.0f && c->jumphold > 2) c->jumpspd += 0.5f;
            c->jumphold++;
        }
        if (c->jump == -1) {
            Instance *d = instance_create(self->x, self->y, OBJ_JUMPDUST);
            if (d) d->image_angle = c->dir;
            sound_play(gm_irandom(1) == 1 ? SND_S_JUMP : SND_S_JUMP2);

            if (place_meeting(self, self->x + fx, self->y + fy, OBJ_JUMPAD)) {
                sound_play(SND_S_MEGAJUMP);
                c->jumpspd = 5.0f;
            }
            c->xspeed += gm_lengthdir_x(c->jumpspd, c->dir + 90.0f) + c->xplat;
            c->yspeed += gm_lengthdir_y(c->jumpspd, c->dir + 90.0f) + c->yplat;
            c->jumpspd = 0.5f;
            c->jumphold = 0;
        }
        c->hss = (float)c->hs;
    } else {
        c->inair++;
        c->jumphold = 0;
        c->jumpspd = 0.5f;
        c->animation = ANIM_JUMP;
        c->shot = 0;

        c->dir += (gm_point_distance(0, 0, c->xspeed, c->yspeed) / 2.0f) * c->hss;

        if (c->right > 0 &&
            place_meeting(self, self->x + gm_lengthdir_x(1, c->dir),
                                self->y + gm_lengthdir_y(1, c->dir), OBJ_MEGABLOCK)) {
            c->xspeed = approach(c->xspeed, gm_lengthdir_x(c->maxspeed, c->dir - 90.0f), 0.2f);
            c->yspeed = approach(c->yspeed, gm_lengthdir_y(c->maxspeed, c->dir - 90.0f), 0.2f);
            if (gm_point_distance(0, 0, c->xspeed, c->yspeed) < 1.0f) {
                c->onground = 3;
                c->dir += 90.0f;
            }
        }
        if (c->left > 0 &&
            place_meeting(self, self->x + gm_lengthdir_x(1, c->dir + 180.0f),
                                self->y + gm_lengthdir_y(1, c->dir + 180.0f), OBJ_MEGABLOCK)) {
            c->xspeed = approach(c->xspeed, gm_lengthdir_x(c->maxspeed, c->dir - 90.0f), 0.2f);
            c->yspeed = approach(c->yspeed, gm_lengthdir_y(c->maxspeed, c->dir - 90.0f), 0.2f);
            if (gm_point_distance(0, 0, c->xspeed, c->yspeed) < 1.0f) {
                c->onground = 3;
                c->dir -= 90.0f;
            }
        }
    }

    if (c->knife == 0) {
        c->reload = fall(c->reload);
        if (c->inair > 100) c->reload = fall(c->reload);
        c->knifewait = fall(c->knifewait);
    }
    c->shot  = fall(c->shot);
    c->shott = fall(c->shott);
    c->land  = fall(c->land);
    c->knife = fall(c->knife);

    if (c->right > 0) { c->hs =  1; c->aimdir =    0.0f; }
    if (c->left  > 0) { c->hs = -1; c->aimdir = -180.0f; }
    if (c->up    > 0) c->aimdir =  90.0f;
    if (c->down  > 0) c->aimdir = -90.0f;

    if (c->bomb > 0 && c->bombammo > 0) {
        if (c->up > 0 || c->down > 0 || c->left > 0 || c->right > 0)
            c->gundir = mid_angle(c->gundir, c->showdir + c->aimdir, 0.1f);
    } else if (c->knife == 0) {
        c->gundir = mid_angle(c->gundir, c->showdir + c->aimdir, 0.4f);
    }

    if (c->bombammo > 0) {
        if (c->bomb > 0 && c->bombspeed < 5.0f)
            c->bombspeed += 0.5f;

        if (c->bomb == -1) {
            c->bombammo--;
            bool planting = fabsf(angle_between(c->gundir, c->showdir - 90.0f)) < 10.0f
                          && c->onground;
            if (planting) {
                Instance *m = instance_create(self->x, self->y, OBJ_SPACEMINE);
                if (m) {
                    m->image_angle = c->dir + c->aimdir;
                    m->s.bomb.team = c->team;
                    m->s.bomb.owner = c->player;
                }
            } else {
                sound_play(SND_S_BOMBTHROW);
                Instance *b = instance_create(self->x, self->y, OBJ_SPACEBOMB);
                if (b) {
                    b->s.bomb.team = c->team;
                    b->s.bomb.owner = c->player;
                    b->s.bomb.xspeed = gm_lengthdir_x(c->bombspeed, c->gundir);
                    b->s.bomb.yspeed = gm_lengthdir_y(c->bombspeed, c->gundir);
                    b->s.bomb.rot = c->gundir;
                    b->s.bomb.spin = c->bombspeed;
                }
                c->xspeed += gm_lengthdir_x(0.5f, c->gundir - 180.0f);
                c->yspeed += gm_lengthdir_y(0.5f, c->gundir - 180.0f);
            }
            c->bombspeed = 0.0f;
        }
    }

    if (c->shoot == 1 && c->bomb == 0) {
        float shotdir = c->dir + c->aimdir;
        if (c->reload == 0 && c->powershot == 0)
            char_shoot(self, shotdir);
        else if (c->knife == 0 && c->knifewait == 0)
            char_swing_knife(self, shotdir);
    }
    if (c->knifeb == 1 && c->knife == 0 && c->knifewait == 0)
        char_swing_knife(self, c->dir + c->aimdir);

    c->showdir = mid_angle(c->showdir, c->dir, 0.25f);

    c->xplat = c->yplat = 0.0f;
    char_move(self, c->xspeed, c->yspeed);

    c->shake /= 1.5f;

    float targx = self->x + gm_lengthdir_x(65 + 8, c->dir + c->aimdir);
    float targy = self->y + gm_lengthdir_y(65 + 8, c->dir + c->aimdir);
    c->camx = mid_value(c->camx, targx, 0.1f);
    c->camy = mid_value(c->camy, targy, 0.1f);

    if (room_defs[world.room].control_open >= 2) {
        if (self->x < 0)                 { self->x += world.room_width;  c->camx += world.room_width; }
        if (self->y < 0)                 { self->y += world.room_height; c->camy += world.room_height; }
        if (self->x > world.room_width)  { self->x -= world.room_width;  c->camx -= world.room_width; }
        if (self->y > world.room_height) { self->y -= world.room_height; c->camy -= world.room_height; }
    }

    if (room_defs[world.room].control_open == 1 && c->inair > 10 &&
        (self->x < CHAR_BORDER || self->x > world.room_width  - CHAR_BORDER ||
         self->y < CHAR_BORDER || self->y > world.room_height - CHAR_BORDER)) {
        char_burn(self, 1.0f);
        return;
    }

    world.camera_manual = true;
    world.cam_x = c->camx + mirand(floorf(c->shake));
    world.cam_y = c->camy + mirand(floorf(c->shake));
    world.view_angle = mid_angle(world.view_angle, -c->dir, 0.2f);

    if (instance_place(self, self->x, self->y, OBJ_DEADLY)) {
        char_kill(self, gm_point_direction(0, 0, c->xspeed, c->yspeed));
        return;
    }

    for (int i = 0; i < world.instance_count; i++) {
        Instance *b = &world.instances[i];
        if (!b->active || b->obj != OBJ_BULLET) continue;
        if (!instance_meets(self, b)) continue;
        if (b->s.mv.owner == c->player && b->s.mv.life <= 5) continue;
        char_kill(self, gm_point_direction(0, 0, b->s.mv.hspeed, b->s.mv.vspeed));
        if (b->s.mv.life > 80) instance_destroy(b);
        return;
    }
    Instance *wave = instance_place(self, self->x, self->y, OBJ_SHOCKWAVE);
    if (wave && wave->image_index < 6.0f) {
        char_kill(self, gm_point_direction(0, 0, wave->s.mv.hspeed, wave->s.mv.vspeed));
        return;
    }

    if (lava_hits_any(self)) {
        char_burn(self, 0.25f);
        return;
    }

    if (instance_place(self, self->x, self->y, OBJ_BLOCK))
        char_kill(self, gm_random(360.0f));
}

static void char_draw_burnup(Instance *self)
{
    const CharState *c = &self->s.ch;
    const float bdis = 40.0f;
    const float rw = (float)world.room_width, rh = (float)world.room_height;

    if (room_defs[world.room].control_open != 1) return;
    if (!(self->x < bdis || self->y < bdis ||
          self->x > rw - bdis || self->y > rh - bdis)) return;

    float dx = bdis, dy = bdis;
    if (self->x < bdis)      dx = self->x;
    if (self->y < bdis)      dy = self->y;
    if (self->x > rw - bdis) dx = rw - self->x;
    if (self->y > rh - bdis) dy = rh - self->y;

    float alpha = 1.0f - SDL_min(dx, dy) / bdis;
    alpha = SDL_clamp(alpha, 0.0f, 1.0f);

    draw_set_blend(BLEND_ADD);
    draw_sprite_ext(SPR_BURNUP, (int)self->image_index, self->x, self->y,
                    (float)c->hs, 1.0f, c->showdir, 0x0000FFu, alpha);
    draw_set_blend(BLEND_NORMAL);
}

static void char_draw(Instance *self)
{
    CharState *c = &self->s.ch;

    char_draw_burnup(self);

    draw_sprite_ext(SPR_CHAR, (int)lroundf(c->image) + c->skin * 12,
                    self->x, self->y,
                    (float)c->hs, 1.0f, c->showdir, 0xFFFFFF, 1.0f);

    float ox = roundf(gm_lengthdir_x(c->bounce, c->dir - 90.0f));
    float oy = roundf(gm_lengthdir_y(c->bounce, c->dir - 90.0f));

    if (c->bomb > 0 && c->bombammo > 0) {

        const int ba = 8;
        int f = (int)((c->bombspeed / 5.0f) * ba) + c->skin * 9;
        draw_sprite_ext(SPR_BOMBARM, f, self->x + ox, self->y + oy,
                        1.0f, (float)c->hs, c->gundir, 0xFFFFFF, 1.0f);
    } else if (c->knife > 0) {
        int ka = 13;
        c->img2 = (int)floorf(ka - ((float)c->knife / c->knifeset) * ka);
        draw_sprite_ext(SPR_KNIFE, c->img2 + 14 * c->skin,
                        self->x + ox, self->y + oy,
                        1.0f, (float)c->hs, c->gundir, 0xFFFFFF, 1.0f);
    } else {
        c->img2 = (int)floorf(42 - ((float)c->reload / c->reloadset) * 42);
        if (c->img2 == 14 && c->img22 != c->img2)
            sound_play(gm_irandom(1) ? SND_S_RELOAD : SND_S_RELOAD2);
        if (c->img2 == 39 && c->img22 != c->img2)
            spawn_chargeup(self, c->gundir, c->xspeed, c->yspeed);
        if (c->img2 == 41 && c->img22 != c->img2)
            sound_play(SND_S_LOADED);
        draw_sprite_ext(SPR_GUN, c->img2 + 43 * c->skin,
                        self->x + ox, self->y + oy,
                        1.0f, (float)c->hs, c->gundir, 0xFFFFFF, 1.0f);
    }

    c->oldanimation = c->animation;
    c->img22 = c->img2;
}

void char_register(void)
{
    object_vtable[OBJ_CHAR].create = char_create;
    object_vtable[OBJ_CHAR].begin_step = char_begin_step;
    object_vtable[OBJ_CHAR].step   = char_step;
    object_vtable[OBJ_CHAR].draw   = char_draw;
}
