#pragma once
#include <stdint.h>

typedef struct { int16_t x, y, w, h; } AtlasRect;

typedef struct {
    const char *name;
    int16_t frame_start, frame_count;
    int16_t w, h, origin_x, origin_y;
    int16_t bb_left, bb_right, bb_top, bb_bottom;
    uint8_t precise;
} SpriteDef;

typedef struct { const char *name; int16_t w, h; } BackgroundDef;
typedef struct { const char *name; const char *file; float volume; } SoundDef;

typedef struct { const char *name; float value; } InstanceVar;
typedef struct {
    int32_t x, y; int16_t obj; int32_t inst_id;
    const InstanceVar *vars; int32_t var_count;
} RoomInstance;
typedef struct { int32_t x, y; int16_t bg; int16_t sx, sy, w, h; int32_t depth; } RoomTile;

typedef struct {
    int16_t bg; uint8_t visible, foreground, tile_h, tile_v, stretch;
    int32_t x, y, h_speed, v_speed;
} RoomBackground;
typedef struct {
    const char *name;
    int32_t width, height, speed;
    uint32_t clear_color;
    int32_t view_w, view_h, hborder, vborder;
    int32_t control_open;   /* Control.open, from room creation code */
    int32_t view_count;     /* editor visible-view flags; runtime count comes from screenset() */
    const RoomInstance *instances; int32_t instance_count;
    const RoomTile *tiles; int32_t tile_count;
    const RoomBackground *backgrounds; int32_t background_count;
} RoomDef;

enum {
    SPR_BLOCK = 0,
    SPR_CHAR = 1,
    SPR_GUN = 2,
    SPR_BULLET = 3,
    SPR_BULLETHIT = 4,
    SPR_BULLETPART = 5,
    SPR_BLAST = 6,
    SPR_BLOODONWALL = 7,
    SPR_BLOODGUSH = 8,
    SPR_FLASH = 9,
    SPR_SPAWN = 10,
    SPR_CHARMASK = 11,
    SPR_ASH = 12,
    SPR_GIBS = 13,
    SPR_KNIFE = 14,
    SPR_KNIFECLASH = 15,
    SPR_SPACEBOMB = 16,
    SPR_SPACEMINE = 17,
    SPR_BOMBMASK = 18,
    SPR_BOMBARM = 19,
    SPR_SHOCKWAVE = 20,
    SPR_SMOKEBLAST = 21,
    SPR_MOON = 22,
    SPR_CHARRED = 23,
    SPR_FLAME = 24,
    SPR_FONT2 = 25,
    SPR_FONT = 26,
    SPR_AIMER = 27,
    SPR_LAVA = 28,
    SPR_OBLAVA = 29,
    SPR_SPRITE34 = 30,
    SPR_SPECBULLET = 31,
    SPR_AMMOPACK = 32,
    SPR_KNIFEONLY = 33,
    SPR_FONTBG = 34,
    SPR_ATOM = 35,
    SPR_TROPHY = 36,
    SPR_COMET = 37,
    SPR_SAT = 38,
    SPR_FONTBIG = 39,
    SPR_JUMPDUST = 40,
    SPR_DEATHSPIKES = 41,
    SPR_OBSPIKES = 42,
    SPR_BIGBLOCK = 43,
    SPR_LASER = 44,
    SPR_JUMPAD = 45,
    SPR_LASERH = 46,
    SPR_BURNUP = 47,
    SPR_SANDFALL = 48,
    SPR_DESTRUCTABLE = 49,
    SPR_DESROCK = 50,
    SPR_SAND = 51,
    SPR_ENERGY = 52,
    SPR_MOONS = 53,
    SPR_BAYDOOR = 54,
    SPR_WARNING = 55,
    SPR_WINDPART = 56,
    SPR_ENGINEFLAME = 57,
    SPR_ICON = 58,
    SPRITE_COUNT = 59
};

enum {
    BG_BG_BLACK = 0,
    BG_BG_BLUE = 1,
    BG_BG_BLUESPACE = 2,
    BG_BG_BLUESPACE2 = 3,
    BG_SPR_FLOATBLUE = 4,
    BG_TITLESCREEN = 5,
    BG_SPR_FLOATER = 6,
    BG_SPR_DUSKSPACE = 7,
    BG_BG_STARSNOW = 8,
    BG_BG_LAVA = 9,
    BG_SPR_LAVABG = 10,
    BG_LAVAFG = 11,
    BG_BG_TUNNEL = 12,
    BG_BG_TOWER = 13,
    BG_BG_LAVATUNNELFG = 14,
    BG_BG_LAVATUNNELBG = 15,
    BG_BG_STARSCAPE = 16,
    BG_FG_STARSCAPE = 17,
    BG_BG_SPACECLOUD = 18,
    BG_BG_CONTROLS = 19,
    BG_SPRING_FG = 20,
    BG_SPRING_BG = 21,
    BG_AFLOAT_FG = 22,
    BG_AFLOAT_BG = 23,
    BG_POCKET_BG = 24,
    BG_POCKET_FG = 25,
    BG_SHOES_FG = 26,
    BG_SHOES_BG = 27,
    BG_LAVA_FG = 28,
    BG_LAVA_BG = 29,
    BG_STARS_BG = 30,
    BG_STARS_FG = 31,
    BG_BG_GIRDER = 32,
    BG_GIRDER_BG = 33,
    BG_GIRDER_FG = 34,
    BG_BG_SANDES = 35,
    BG_BG_GREENSPACE = 36,
    BG_BG_GREENSPACE2 = 37,
    BG_SANDES_FG = 38,
    BG_SANDES_BG = 39,
    BG_BG_BLUE2 = 40,
    BG_STARTER_FG = 41,
    BG_STARTER_BG = 42,
    BG_BACKGROUND108 = 43,
    BG_BG_SHIP = 44,
    BG_BACKGROUND110 = 45,
    BG_BG_XTUBE = 46,
    BACKGROUND_COUNT = 47
};

enum {
    SND_S_METALSTEP6 = 0,
    SND_S_METALSTEP5 = 1,
    SND_S_METALSTEP4 = 2,
    SND_S_METALHIT5 = 3,
    SND_S_METALHIT4 = 4,
    SND_S_METALLAND5 = 5,
    SND_S_METALHIT3 = 6,
    SND_S_METALHIT2 = 7,
    SND_S_METALHIT1 = 8,
    SND_S_METALSTEP3 = 9,
    SND_S_METALLAND4 = 10,
    SND_S_METALLAND3 = 11,
    SND_S_METALLAND2 = 12,
    SND_S_METALLAND1 = 13,
    SND_S_METALSTEP2 = 14,
    SND_S_METALSTEP1 = 15,
    SND_S_LASER = 16,
    SND_S_LASER2 = 17,
    SND_S_HITWALL = 18,
    SND_S_HITWALL2 = 19,
    SND_S_HITWALL3 = 20,
    SND_S_RELOAD = 21,
    SND_S_RELOAD2 = 22,
    SND_S_LOADED = 23,
    SND_S_KILL = 24,
    SND_S_CROUCH = 25,
    SND_S_JUMP = 26,
    SND_S_JUMP2 = 27,
    SND_S_KNIFE = 28,
    SND_S_KNIFE2 = 29,
    SND_S_KNIFECLASH = 30,
    SND_S_EXPLODE = 31,
    SND_S_BOMBTHROW = 32,
    SND_S_BOMBEEP = 33,
    SND_S_EXPLODE2 = 34,
    SND_S_MINEWALL = 35,
    SND_S_KNIFEWOUND = 36,
    SND_S_FLAMEBURST = 37,
    SND_S_LITTLEFLAME = 38,
    SND_S_HITBOMB = 39,
    SND_S_MOVE = 40,
    SND_S_SELECT = 41,
    SND_S_FINISH = 42,
    SND_S_SNARE = 43,
    SND_S_MEGAJUMP = 44,
    SND_S_CRUMBLE = 45,
    SND_S_CRUMBLE2 = 46,
    SND_S_WIND = 47,
    SND_S_ALARM = 48,
    SOUND_COUNT = 49
};

enum {
    ROOM_POCKETCAVE = 0,
    ROOM_SCREEN_INIT = 1,
    ROOM_AFLOAT = 2,
    ROOM_TITLE = 3,
    ROOM_STARS = 4,
    ROOM_LAVAR = 5,
    ROOM_TUNNELBG = 6,
    ROOM_LAVATUNNEL = 7,
    ROOM_STATROOM = 8,
    ROOM_FARJUMP = 9,
    ROOM_SPACESAND = 10,
    ROOM_RANDOMROOM = 11,
    ROOM_MOTION = 12,
    ROOM_RM_CONTROLS = 13,
    ROOM_FOUR = 14,
    ROOM_SANDES = 15,
    ROOM_DEATHRING = 16,
    ROOM_STARTER = 17,
    ROOM_BAY = 18,
    ROOM_XTUBE = 19,
    ROOM_COUNT = 20
};

enum {
    OBJ_WALL = 0,
    OBJ_CHAR = 1,
    OBJ_SCREEN = 2,
    OBJ_CONTROL = 3,
    OBJ_BULLET = 4,
    OBJ_BULLETPART = 5,
    OBJ_BLOOD = 7,
    OBJ_BLOODONWALL = 8,
    OBJ_SPAWN = 9,
    OBJ_ASH = 10,
    OBJ_GIBS = 11,
    OBJ_BG = 12,
    OBJ_SPACEBOMB = 13,
    OBJ_SPACEMINE = 14,
    OBJ_SHOCKWAVE = 15,
    OBJ_MOON = 16,
    OBJ_CHARRED = 17,
    OBJ_OBJ_TITLE = 18,
    OBJ_WORD = 19,
    OBJ_DEATHLAVA = 20,
    OBJ_MOVINGBLOCK = 21,
    OBJ_BLOCK = 22,
    OBJ_SUPERLAVA = 23,
    OBJ_GREENLAVA = 24,
    OBJ_ATOM = 25,
    OBJ_STATSCREEN = 26,
    OBJ_COMET = 27,
    OBJ_SAT = 28,
    OBJ_JUMPDUST = 29,
    OBJ_SPARKLINE = 30,
    OBJ_DEATHSPIKES = 31,
    OBJ_MOVABLE = 32,
    OBJ_CONTROLSSHOW = 33,
    OBJ_LASER = 141,
    OBJ_DEADLY = 142,
    OBJ_LASERH = 143,
    OBJ_JUMPAD = 144,
    OBJ_DESTRUCTABLE = 145,
    OBJ_MEGABLOCK = 146,
    OBJ_SANDFALLDOWN = 147,
    OBJ_SANDFALL = 148,
    OBJ_SANDFALLUP = 149,
    OBJ_ROCKDEBRI = 150,
    OBJ_SAND = 151,
    OBJ_CHARGEUP = 152,
    OBJ_MOONS = 153,
    OBJ_BAYDOORCONTROL = 154,
    OBJ_A = 155,
    OBJ_BAYDOOR = 156,
    OBJ_PUSHER = 157,
    OBJ_WARNINGLIGHTS = 158,
    OBJ_WINDPART = 159,
    OBJ_ENGINEFLAME = 160,
    OBJ_DEADOB = 161,
    OBJ_MAX_ID = 162
};

extern const AtlasRect atlas_frames[];
extern const int atlas_frame_count;
extern const SpriteDef sprite_defs[SPRITE_COUNT];
extern const BackgroundDef background_defs[BACKGROUND_COUNT];
extern const SoundDef sound_defs[SOUND_COUNT];
extern const RoomDef room_defs[ROOM_COUNT];
extern const int16_t object_parent[OBJ_MAX_ID];
extern const int16_t object_sprite[OBJ_MAX_ID];
extern const int16_t object_mask[OBJ_MAX_ID];
extern const int32_t object_depth[OBJ_MAX_ID];
extern const uint8_t object_solid[OBJ_MAX_ID];
extern const uint8_t object_visible[OBJ_MAX_ID];
extern const char *const object_names[OBJ_MAX_ID];
extern const int room_order[ROOM_COUNT];
