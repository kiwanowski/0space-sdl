#pragma once

#include "engine/engine.h"

void char_register(void);
void particles_register(void);
void control_register(void);
void hazards_register(void);
void debris_register(void);
void bombs_register(void);
void scenery_register(void);
void background_register(void);

typedef struct { int players; int bayopen; int hwrap, vwrap; } Globals;
extern Globals globals;

void pushed_by_sand(Instance *self, float *yspeed, int reps);

void wrap_position(Instance *self);

void char_kill(Instance *self, float otherdir);

void explode(float x, float y, int team);

void destructable_break(Instance *self, float hitdir);

void spawn_death_effect(float x, float y, float dir, float otherdir, int skin);
void spawn_charred(float x, float y, float showdir, float spin,
                   float xspeed, float yspeed);
void spawn_chargeup(Instance *player, float gundir, float xspeed, float yspeed);
void spawn_flame(float x, float y, float xspeed, float yspeed);

bool lava_hits(const Instance *self, const Instance *other);
bool lava_hits_any(const Instance *other);

bool mover_step(Instance *self, float dx, float dy, int solid,
                void (*on_hit_x)(Instance *), void (*on_hit_y)(Instance *));
