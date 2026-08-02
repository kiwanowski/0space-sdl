#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/engine.h"

void world_draw(void);

int main(int argc, char **argv)
{
    const char *asset_dir = getenv("ZEROSPACE_ASSETS");
    if (!asset_dir) asset_dir = DEFAULT_ASSET_DIR;

    int  scale       = 3;
    bool fullscreen  = false;
    bool no_audio    = false;
    int  start_room  = -1;
    int  run_frames  = -1;
    const char *shot = NULL;
    bool  trace      = false;
    int   face       = 0;
    int   bomb_at    = -1;
    float bomb_x = -1, bomb_y = -1;
    int   reload_at  = -1;
    int   shot_at    = -1;
    const char *hold[6]; int hold_count = 0;
    int   fire_at[8]; int fire_count = 0;
    int   bullet_at  = -1;
    int   jump_at[8]; int jump_count = 0;
    float warp_x = -1, warp_y = -1;
    bool  census     = false;
    float impulse_x = 0, impulse_y = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--scale") && i + 1 < argc)        scale = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--fullscreen"))              fullscreen = true;
        else if (!strcmp(argv[i], "--no-audio"))                no_audio = true;
        else if (!strcmp(argv[i], "--room") && i + 1 < argc)    start_room = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc)  run_frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc) shot = argv[++i];
        else if (!strcmp(argv[i], "--trace"))                   trace = true;
        else if (!strcmp(argv[i], "--face") && i + 1 < argc)     face = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--bomb") && i + 1 < argc)     bomb_at = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--bomb-at") && i + 2 < argc) {
            bomb_x = (float)atof(argv[++i]);
            bomb_y = (float)atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "--census"))                  census = true;
        else if (!strcmp(argv[i], "--reload") && i + 1 < argc)   reload_at = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--shot") && i + 1 < argc)     shot_at = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--hold") && i + 1 < argc) {
            if (hold_count < 6) hold[hold_count++] = argv[++i];
            else i++;
        }
        else if (!strcmp(argv[i], "--bullet") && i + 1 < argc)   bullet_at = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--fire") && i + 1 < argc) {
            if (fire_count < 8) fire_at[fire_count++] = atoi(argv[++i]);
            else i++;
        }
        else if (!strcmp(argv[i], "--jump") && i + 1 < argc) {
            if (jump_count < 8) jump_at[jump_count++] = atoi(argv[++i]);
            else i++;
        }
        else if (!strcmp(argv[i], "--warp") && i + 2 < argc) {
            warp_x = (float)atof(argv[++i]);
            warp_y = (float)atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "--impulse") && i + 2 < argc) {
            impulse_x = (float)atof(argv[++i]);
            impulse_y = (float)atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "--list-rooms")) {
            for (int r = 0; r < ROOM_COUNT; r++)
                printf("%2d %s (%dx%d)\n", r, room_defs[r].name,
                       room_defs[r].width, room_defs[r].height);
            return 0;
        } else {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    if (!render_init(asset_dir, scale, fullscreen)) return 1;
    input_init();
    if (!no_audio) audio_init(asset_dir);

    objects_register();

    for (int k = 0; k < hold_count; k++) {
        const char *h = hold[k];
        if      (!strcmp(h, "right")) input_force(0, IN_RIGHT, true);
        else if (!strcmp(h, "left"))  input_force(0, IN_LEFT,  true);
        else if (!strcmp(h, "up"))    input_force(0, IN_UP,    true);
        else if (!strcmp(h, "down"))  input_force(0, IN_DOWN,  true);
        else if (!strcmp(h, "bomb"))  input_force(0, IN_BOMB,  true);
        else if (!strcmp(h, "jump"))  input_force(0, IN_JUMP,  true);
        else if (!strcmp(h, "knife")) input_force(0, IN_KNIFE, true);
        else { fprintf(stderr, "unknown --hold button: %s\n", h); return 1; }
    }

    world.pending_room = -1;
    world_load_room(start_room >= 0 ? start_room : room_order[1]);

    const double step_ms = 1000.0 / 30.0;
    double next = (double)SDL_GetTicks();
    int frame = 0;

    while (!input_quit_requested()) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) input_handle_event(&ev);

        input_force(0, IN_SHOOT, false);
        for (int k = 0; k < fire_count; k++)
            if (frame == fire_at[k]) input_force(0, IN_SHOOT, true);
        input_force(0, IN_JUMP, false);
        for (int k = 0; k < jump_count; k++)
            if (frame == jump_at[k]) input_force(0, IN_JUMP, true);

        input_new_frame();
        if (input_held(0, IN_QUIT)) input_request_quit();

        if (frame == 1 && warp_x >= 0) {
            Instance *p = instance_find(OBJ_CHAR);
            if (p) { p->x = warp_x; p->y = warp_y; p->s.ch.camx = warp_x; p->s.ch.camy = warp_y; }
        }
        if (frame == 1 && (impulse_x != 0 || impulse_y != 0)) {
            Instance *p = instance_find(OBJ_CHAR);
            if (p) { p->s.ch.xspeed = impulse_x; p->s.ch.yspeed = impulse_y; }
        }
        if (face != 0) {
            Instance *p = instance_find(OBJ_CHAR);
            if (p) {
                p->s.ch.hs = face;
                p->s.ch.aimdir = (face < 0) ? -180.0f : 0.0f;
            }
        }

        if (frame == 1 && reload_at >= 0) {
            Instance *p = instance_find(OBJ_CHAR);
            if (p) p->s.ch.reload = reload_at;
        }

        if (frame == shot_at) {
            Instance *p = instance_find(OBJ_CHAR);
            if (p) p->s.ch.shot = 10;
        }

        if (frame == bullet_at) {
            Instance *p = instance_find(OBJ_CHAR);
            if (p) {
                Instance *b = instance_create(p->x, p->y, OBJ_BULLET);
                if (b) { b->s.mv.life = 100; b->s.mv.hspeed = 12.0f; }
            }
        }

        if (frame == bomb_at) {
            Instance *p = instance_find(OBJ_CHAR);
            if (p) {
                float bx = (bomb_x >= 0) ? bomb_x : p->x;
                float by = (bomb_y >= 0) ? bomb_y : p->y;
                Instance *b = instance_create(bx, by, OBJ_SPACEBOMB);
                if (b) { b->s.bomb.team = 0; b->s.bomb.spin = 4.0f; }
                printf("dropped a bomb at frame %d\n", frame);
            }
        }

        world_step();

        if (trace) {
            Instance *bb = instance_find(OBJ_SPACEBOMB);
            if (bb) printf("f%-4d bomb x=%7.2f y=%7.2f spd=(%6.2f,%6.2f) t=%3d hitid=%d\n",
                           frame, bb->x, bb->y, bb->s.bomb.xspeed, bb->s.bomb.yspeed,
                           bb->s.bomb.t, bb->s.bomb.hitid);
            Instance *p = instance_find(OBJ_CHAR);
            if (p)
                printf("f%-4d x=%7.2f y=%7.2f dir=%7.2f ground=%d air=%-4d "
                       "spd=(%6.2f,%6.2f) anim=%d img=%.3f frame=%d\n",
                       frame, p->x, p->y, p->s.ch.dir, p->s.ch.onground,
                       p->s.ch.inair, p->s.ch.xspeed, p->s.ch.yspeed,
                       p->s.ch.animation, p->s.ch.image, (int)p->s.ch.image);
        }

        render_begin();
        world_draw();
        render_end();

        frame++;
        bool last = (run_frames >= 0 && frame >= run_frames);

        if (last && shot) {
            if (render_screenshot(shot)) printf("wrote %s\n", shot);
            else                         fprintf(stderr, "screenshot failed\n");
        }

        render_present();
        if (last) break;

        next += step_ms;
        double now = (double)SDL_GetTicks();
        if (next > now) SDL_Delay((uint32_t)(next - now));
        else            next = now;
    }

    if (census) {
        int counts[OBJ_MAX_ID];
        for (int i = 0; i < OBJ_MAX_ID; i++) counts[i] = 0;
        for (int i = 0; i < world.instance_count; i++)
            if (world.instances[i].active) counts[world.instances[i].obj]++;
        int gone = 0;
        const RoomDef *rd = &room_defs[world.room];
        for (int i = 0; i < rd->tile_count && i < MAX_TILES; i++)
            if (world.tile_deleted[i]) gone++;
        printf("--- tiles deleted: %d/%d ---\n", gone, rd->tile_count);

        printf("--- live instances ---\n");
        for (int i = 0; i < OBJ_MAX_ID; i++)
            if (counts[i]) printf("  %-16s %d\n", object_names[i] ? object_names[i] : "?", counts[i]);
    }

    audio_shutdown();
    input_shutdown();
    render_shutdown();
    SDL_Quit();
    return 0;
}
