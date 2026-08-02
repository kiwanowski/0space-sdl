#pragma once

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>

#include "gen_assets.h"

#define MAX_INSTANCES 4096
#define MAX_TILES     1024

/* ------------------------------------------------------------------ */
/* Instances                                                           */
/* ------------------------------------------------------------------ */

typedef struct Instance Instance;

typedef struct {
    float xc, yc;
    float xspeed, yspeed;
    float dir;
    float showdir;
    float gundir;
    float aimdir;
    float maxspeed, accel, fric;

    int   onground, inair;
    float jumpspd;
    int   jumphold;

    int   animation, oldanimation;
    float image;
    int   image2, img2, img22;
    float bounce;

    int   hs;
    float hss;

    int   reload, reloadset;
    int   knife, knifeset, knifewait, knifeclash;
    int   shot, shott, land, noact;
    int   ammo, bombammo, powershot;
    float bombspeed;

    float shake;
    float camx, camy;
    float xplat, yplat;

    int   player, team, skin;

    int   left, right, up, down, shoot, jump, bomb, knifeb;
} CharState;

typedef struct {
    float xc, yc;
    float hspeed, vspeed;
    float friction;
    int   owner;
    int   life;
    int   timer;
    bool  wallhit;
    uint32_t color;
} MoverState;

typedef struct {
    float xc, yc;
    float hspeed, vspeed;
    float dir, spin;
    float anim, imgspd;
    int   timer;
    int   state;
} GenericState;

typedef struct {
    float xc, yc;
    float xspeed, yspeed;
    float rot, spin;
    float flash;
    int   flash2;
    int   t, tmax;
    int   team, owner;
    int   dead, deadtime;
    int   settime, set;
    float image_single;
} BombState;

typedef struct {
    float x1, y1, x2, y2;
    float xx1, yy1, xx2, yy2;
    float ax, ay, bx, by;
    float maxtime, time, t;
    float anim;
    bool  started;
} LavaState;

struct Instance {
    bool    active;
    int     obj;
    int     id;

    float   x, y;
    float   xprevious, yprevious;
    float   xstart, ystart;

    int     sprite_index;
    int     mask_index;
    float   image_index;
    float   image_speed;
    float   image_xscale, image_yscale;
    float   image_angle;
    float   image_alpha;
    uint32_t image_blend;

    int     depth;
    bool    visible;
    bool    solid;

    int     alarm[12];

    union {
        CharState    ch;
        MoverState   mv;
        GenericState gen;
        LavaState    lava;
        BombState    bomb;
    } s;
};

typedef struct {
    void (*create)(Instance *);
    void (*begin_step)(Instance *);
    void (*step)(Instance *);
    void (*end_step)(Instance *);
    void (*draw)(Instance *);
    void (*destroy)(Instance *);
    void (*set_var)(Instance *, const char *name, float value);
    void (*room_start)(Instance *);
} ObjectVTable;

extern ObjectVTable object_vtable[OBJ_MAX_ID];
void objects_register(void);

typedef struct {
    Instance instances[MAX_INSTANCES];
    int      instance_count;
    int      next_id;

    int      room;
    int      room_width, room_height;
    int      view_x, view_y, view_w, view_h;
    int      hborder, vborder;
    uint32_t clear_color;

    float    cam_x, cam_y;
    float    view_angle;
    bool     camera_manual;
    bool     hwrap, vwrap;
    int      view_count;

    uint8_t  tile_deleted[MAX_TILES];

    int      pending_room;
    bool     restart_pending;
} World;

extern World world;

Instance *instance_create(float x, float y, int obj);
void      instance_destroy(Instance *inst);
Instance *instance_find(int obj);
int       instance_number(int obj);
Instance *instance_nearest(float x, float y, int obj);
bool      object_is(int obj, int parent);

void room_goto(int room);
void room_restart(void);
void world_load_room(int room);
void world_step(void);

void tile_layer_delete_at(int depth, float x, float y);

void      instance_bbox(const Instance *inst, float px, float py,
                        float *l, float *t, float *r, float *b);
bool      instance_meets(const Instance *a, const Instance *b);
Instance *instance_place(const Instance *self, float px, float py, int obj);
bool      place_meeting(const Instance *self, float px, float py, int obj);
Instance *collision_rectangle(float x1, float y1, float x2, float y2,
                              int obj, const Instance *ignore);
Instance *collision_circle(float cx, float cy, float radius,
                           int obj, const Instance *ignore);
Instance *collision_line(float x1, float y1, float x2, float y2,
                         int obj, const Instance *ignore);

bool render_init(const char *asset_dir, int scale, bool fullscreen);
void render_shutdown(void);
void render_begin(void);
void render_end(void);
void render_present(void);

void draw_sprite(int sprite, int frame, float x, float y);
void draw_sprite_ext(int sprite, int frame, float x, float y,
                     float xscale, float yscale, float angle,
                     uint32_t blend, float alpha);
void draw_background_tiled(int bg, float x, float y);
void draw_background_part(int bg, int sx, int sy, int sw, int sh,
                          float x, float y);
void draw_rectangle(float x1, float y1, float x2, float y2, bool outline);
void draw_circle(float cx, float cy, float r, bool outline);

typedef enum { BLEND_NORMAL, BLEND_ADD, BLEND_GLOW } BlendMode;
void draw_set_blend(BlendMode mode);
void draw_line_col(float x1, float y1, float x2, float y2);
void draw_set_color(uint32_t rgb);
void draw_set_alpha(float a);
uint32_t merge_color(uint32_t a, uint32_t b, float amount);

void draw_text_small(float x, float y, const char *text);

SDL_Renderer *render_get(void);
void render_canvas_size(int *w, int *h);
bool render_screenshot(const char *path);

typedef enum {
    IN_LEFT, IN_RIGHT, IN_UP, IN_DOWN,
    IN_JUMP, IN_SHOOT, IN_BOMB, IN_KNIFE,
    IN_START, IN_QUIT,
    IN_COUNT
} InputAction;

#define MAX_PLAYERS 4

void input_init(void);
void input_shutdown(void);
void input_new_frame(void);
void input_handle_event(const SDL_Event *ev);
bool input_held(int player, InputAction a);
bool input_pressed(int player, InputAction a);
bool input_quit_requested(void);
void input_request_quit(void);
void input_force(int player, InputAction a, bool down);

bool audio_init(const char *asset_dir);
void audio_shutdown(void);
void sound_play(int snd);
void sound_play_pitch(int snd, float pitch, float volume);
void sound_stop(int snd);
void music_play(const char *file, bool loop);
void music_stop(void);

float gm_lengthdir_x(float len, float dir);
float gm_lengthdir_y(float len, float dir);
float gm_point_direction(float x1, float y1, float x2, float y2);
float gm_point_distance(float x1, float y1, float x2, float y2);
int   gm_round(float v);
int   gm_irandom(int n);
float gm_random(float n);
float gm_sign(float v);

extern const char *g_asset_dir;
