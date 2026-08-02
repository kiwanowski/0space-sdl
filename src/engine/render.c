#include <SDL3_image/SDL_image.h>
#include <stdio.h>
#include <string.h>

#include "engine.h"

#define VIEW_W 320
#define VIEW_H 240

#define CANVAS_W 448
#define CANVAS_H 448

static SDL_Window   *g_window;
static SDL_Renderer *g_renderer;
static SDL_Texture  *g_atlas;
static SDL_Texture  *g_canvas;
static SDL_Texture  *g_backgrounds[BACKGROUND_COUNT];

static uint32_t g_color = 0xFFFFFF;
static float    g_alpha = 1.0f;

const char *g_asset_dir = DEFAULT_ASSET_DIR;

SDL_Renderer *render_get(void) { return g_renderer; }

static void path_join(char *out, size_t n, const char *a, const char *b)
{
    SDL_snprintf(out, n, "%s/%s", a, b);
}

bool render_init(const char *asset_dir, int scale, bool fullscreen)
{
    g_asset_dir = asset_dir;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    SDL_WindowFlags flags = 0;
    if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN;

    if (!SDL_CreateWindowAndRenderer("0Space", VIEW_W * scale, VIEW_H * scale,
                                     flags, &g_window, &g_renderer)) {
        SDL_Log("CreateWindowAndRenderer failed: %s", SDL_GetError());
        return false;
    }

    SDL_SetRenderLogicalPresentation(g_renderer, VIEW_W, VIEW_H,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

    char path[1024];
    path_join(path, sizeof path, asset_dir, "atlas.png");
    g_atlas = IMG_LoadTexture(g_renderer, path);
    if (!g_atlas) {
        SDL_Log("failed to load %s: %s", path, SDL_GetError());
        return false;
    }

    SDL_SetTextureScaleMode(g_atlas, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(g_atlas, SDL_BLENDMODE_BLEND);

    g_canvas = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888,
                                 SDL_TEXTUREACCESS_TARGET, CANVAS_W, CANVAS_H);
    if (!g_canvas) {
        SDL_Log("failed to create canvas: %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode(g_canvas, SDL_SCALEMODE_NEAREST);
    return true;
}

void render_shutdown(void)
{
    for (int i = 0; i < BACKGROUND_COUNT; i++)
        if (g_backgrounds[i]) SDL_DestroyTexture(g_backgrounds[i]);
    if (g_atlas) SDL_DestroyTexture(g_atlas);
    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window) SDL_DestroyWindow(g_window);
}

static SDL_Texture *background_texture(int bg)
{
    if (bg < 0 || bg >= BACKGROUND_COUNT) return NULL;
    if (!g_backgrounds[bg]) {
        char path[1024], file[512];
        SDL_snprintf(file, sizeof file, "bg/%s.png", background_defs[bg].name);
        path_join(path, sizeof path, g_asset_dir, file);
        g_backgrounds[bg] = IMG_LoadTexture(g_renderer, path);
        if (g_backgrounds[bg])
            SDL_SetTextureScaleMode(g_backgrounds[bg], SDL_SCALEMODE_NEAREST);
    }
    return g_backgrounds[bg];
}

void render_begin(void)
{
    world.view_x = (int)(world.cam_x - CANVAS_W / 2);
    world.view_y = (int)(world.cam_y - CANVAS_H / 2);

    SDL_SetRenderTarget(g_renderer, g_canvas);
    uint32_t c = world.clear_color;
    SDL_SetRenderDrawColor(g_renderer, c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF, 255);
    SDL_RenderClear(g_renderer);
}

void render_end(void)
{
    SDL_SetRenderTarget(g_renderer, NULL);
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_renderer);

    SDL_FRect dst = {
        (VIEW_W - CANVAS_W) / 2.0f,
        (VIEW_H - CANVAS_H) / 2.0f,
        CANVAS_W, CANVAS_H,
    };
    SDL_FPoint centre = { CANVAS_W / 2.0f, CANVAS_H / 2.0f };
    SDL_RenderTextureRotated(g_renderer, g_canvas, NULL, &dst,
                             -world.view_angle, &centre, SDL_FLIP_NONE);
}

void render_canvas_size(int *w, int *h) { *w = CANVAS_W; *h = CANVAS_H; }
void render_present(void) { SDL_RenderPresent(g_renderer); }

static const AtlasRect *frame_rect(int sprite, int frame)
{
    if (sprite < 0 || sprite >= SPRITE_COUNT) return NULL;
    const SpriteDef *sd = &sprite_defs[sprite];
    if (sd->frame_count <= 0) return NULL;
    int f = frame % sd->frame_count;
    if (f < 0) f += sd->frame_count;
    return &atlas_frames[sd->frame_start + f];
}

void draw_sprite_ext(int sprite, int frame, float x, float y,
                     float xscale, float yscale, float angle,
                     uint32_t blend, float alpha)
{
    const AtlasRect *r = frame_rect(sprite, frame);
    if (!r) return;
    const SpriteDef *sd = &sprite_defs[sprite];

    SDL_FRect src = { r->x, r->y, r->w, r->h };

    SDL_FlipMode flip = SDL_FLIP_NONE;
    if (xscale < 0) { xscale = -xscale; flip |= SDL_FLIP_HORIZONTAL; }
    if (yscale < 0) { yscale = -yscale; flip |= SDL_FLIP_VERTICAL; }

    float w = r->w * xscale;
    float h = r->h * yscale;
    float ox = sd->origin_x * xscale;
    float oy = sd->origin_y * yscale;
    if (flip & SDL_FLIP_HORIZONTAL) ox = w - ox;
    if (flip & SDL_FLIP_VERTICAL)   oy = h - oy;

    SDL_FRect dst = { x - world.view_x - ox, y - world.view_y - oy, w, h };

    SDL_SetTextureColorMod(g_atlas, blend & 0xFF, (blend >> 8) & 0xFF, (blend >> 16) & 0xFF);
    SDL_SetTextureAlphaMod(g_atlas, (uint8_t)(alpha * 255.0f));

    if (angle != 0.0f || flip != SDL_FLIP_NONE) {
        SDL_FPoint centre = { ox, oy };
        SDL_RenderTextureRotated(g_renderer, g_atlas, &src, &dst,
                                 -angle, &centre, flip);
    } else {
        SDL_RenderTexture(g_renderer, g_atlas, &src, &dst);
    }

    SDL_SetTextureColorMod(g_atlas, 255, 255, 255);
    SDL_SetTextureAlphaMod(g_atlas, 255);
}

void draw_sprite(int sprite, int frame, float x, float y)
{
    draw_sprite_ext(sprite, frame, x, y, 1, 1, 0, 0xFFFFFF, 1.0f);
}

void draw_background_part(int bg, int sx, int sy, int sw, int sh,
                          float x, float y)
{
    SDL_Texture *tex = background_texture(bg);
    if (!tex) return;
    SDL_FRect src = { sx, sy, sw, sh };
    SDL_FRect dst = { x - world.view_x, y - world.view_y, sw, sh };
    SDL_RenderTexture(g_renderer, tex, &src, &dst);
}

void draw_background_tiled(int bg, float x, float y)
{
    SDL_Texture *tex = background_texture(bg);
    if (!tex) return;
    int bw = background_defs[bg].w, bh = background_defs[bg].h;
    if (bw <= 0 || bh <= 0) return;

    float ox = x - world.view_x;
    float oy = y - world.view_y;
    ox = SDL_fmodf(ox, (float)bw); if (ox > 0) ox -= bw;
    oy = SDL_fmodf(oy, (float)bh); if (oy > 0) oy -= bh;

    for (float py = oy; py < VIEW_H; py += bh) {
        for (float px = ox; px < VIEW_W; px += bw) {
            SDL_FRect dst = { px, py, bw, bh };
            SDL_RenderTexture(g_renderer, tex, NULL, &dst);
        }
    }
}

void draw_set_color(uint32_t rgb) { g_color = rgb; }
void draw_set_alpha(float a)      { g_alpha = a; }

static void apply_draw_color(void)
{
    SDL_SetRenderDrawColor(g_renderer,
                           g_color & 0xFF, (g_color >> 8) & 0xFF, (g_color >> 16) & 0xFF,
                           (uint8_t)(g_alpha * 255.0f));
}

void draw_rectangle(float x1, float y1, float x2, float y2, bool outline)
{
    apply_draw_color();
    if (x1 > x2) { float t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { float t = y1; y1 = y2; y2 = t; }
    SDL_FRect r = { x1 - world.view_x, y1 - world.view_y, x2 - x1 + 1, y2 - y1 + 1 };
    if (outline) SDL_RenderRect(g_renderer, &r);
    else         SDL_RenderFillRect(g_renderer, &r);
}

void draw_circle(float cx, float cy, float r, bool outline)
{
    apply_draw_color();
    cx -= world.view_x;
    cy -= world.view_y;
    int ri = (int)(r + 0.5f);
    for (int dy = -ri; dy <= ri; dy++) {
        float dx = SDL_sqrtf(SDL_max(0.0f, (float)ri * ri - (float)dy * dy));
        if (outline) {
            SDL_RenderPoint(g_renderer, cx - dx, cy + dy);
            SDL_RenderPoint(g_renderer, cx + dx, cy + dy);
        } else {
            SDL_RenderLine(g_renderer, cx - dx, cy + dy, cx + dx, cy + dy);
        }
    }
}

void draw_set_blend(BlendMode mode)
{
    SDL_BlendMode m = SDL_BLENDMODE_BLEND;
    if (mode == BLEND_ADD) {
        m = SDL_BLENDMODE_ADD;
    } else if (mode == BLEND_GLOW) {
        m = SDL_ComposeCustomBlendMode(
                SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_DST_ALPHA,
                SDL_BLENDOPERATION_ADD,
                SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE,
                SDL_BLENDOPERATION_ADD);
    }
    SDL_SetRenderDrawBlendMode(g_renderer, m);
    if (g_atlas) SDL_SetTextureBlendMode(g_atlas, m);
}

void draw_line_col(float x1, float y1, float x2, float y2)
{
    apply_draw_color();
    SDL_RenderLine(g_renderer,
                   x1 - world.view_x, y1 - world.view_y,
                   x2 - world.view_x, y2 - world.view_y);
}

uint32_t merge_color(uint32_t a, uint32_t b, float amount)
{
    if (amount < 0) amount = 0;
    if (amount > 1) amount = 1;
    int ar = a & 0xFF, ag = (a >> 8) & 0xFF, ab = (a >> 16) & 0xFF;
    int br = b & 0xFF, bg = (b >> 8) & 0xFF, bb = (b >> 16) & 0xFF;
    int r = (int)(ar + (br - ar) * amount);
    int g = (int)(ag + (bg - ag) * amount);
    int bl = (int)(ab + (bb - ab) * amount);
    return (uint32_t)(r | (g << 8) | (bl << 16));
}

void draw_text_small(float x, float y, const char *text)
{
    apply_draw_color();
    for (const char *p = text; *p; p++) {
        if (*p != ' ') {
            SDL_FRect r = { x - world.view_x, y - world.view_y, 3, 5 };
            SDL_RenderFillRect(g_renderer, &r);
        }
        x += 4;
    }
}

bool render_screenshot(const char *path)
{
    SDL_Surface *s = SDL_RenderReadPixels(g_renderer, NULL);
    if (!s) {
        SDL_Log("RenderReadPixels failed: %s", SDL_GetError());
        return false;
    }
    bool ok = IMG_SavePNG(s, path);
    SDL_DestroySurface(s);
    return ok;
}
