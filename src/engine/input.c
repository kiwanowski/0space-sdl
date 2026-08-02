#include <string.h>

#include "engine.h"

typedef struct {
    SDL_Scancode keys[IN_COUNT];
} KeyMap;

static KeyMap        g_keymap[MAX_PLAYERS];
static SDL_Gamepad  *g_pads[MAX_PLAYERS];
static bool          g_held[MAX_PLAYERS][IN_COUNT];
static bool          g_prev[MAX_PLAYERS][IN_COUNT];
static bool          g_force[MAX_PLAYERS][IN_COUNT];
static bool          g_quit;

void input_force(int player, InputAction a, bool down)
{
    if (player >= 0 && player < MAX_PLAYERS && a >= 0 && a < IN_COUNT)
        g_force[player][a] = down;
}

static void set_defaults(void)
{
    KeyMap *p0 = &g_keymap[0];
    p0->keys[IN_LEFT]  = SDL_SCANCODE_LEFT;
    p0->keys[IN_RIGHT] = SDL_SCANCODE_RIGHT;
    p0->keys[IN_UP]    = SDL_SCANCODE_UP;
    p0->keys[IN_DOWN]  = SDL_SCANCODE_DOWN;
    p0->keys[IN_JUMP]  = SDL_SCANCODE_Z;
    p0->keys[IN_SHOOT] = SDL_SCANCODE_X;
    p0->keys[IN_BOMB]  = SDL_SCANCODE_C;
    p0->keys[IN_KNIFE] = SDL_SCANCODE_V;
    p0->keys[IN_START] = SDL_SCANCODE_RETURN;
    p0->keys[IN_QUIT]  = SDL_SCANCODE_ESCAPE;

    KeyMap *p1 = &g_keymap[1];
    p1->keys[IN_LEFT]  = SDL_SCANCODE_A;
    p1->keys[IN_RIGHT] = SDL_SCANCODE_D;
    p1->keys[IN_UP]    = SDL_SCANCODE_W;
    p1->keys[IN_DOWN]  = SDL_SCANCODE_S;
    p1->keys[IN_JUMP]  = SDL_SCANCODE_G;
    p1->keys[IN_SHOOT] = SDL_SCANCODE_H;
    p1->keys[IN_BOMB]  = SDL_SCANCODE_J;
    p1->keys[IN_KNIFE] = SDL_SCANCODE_K;
    p1->keys[IN_START] = SDL_SCANCODE_TAB;
    p1->keys[IN_QUIT]  = SDL_SCANCODE_UNKNOWN;
}

void input_init(void)
{
    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
        SDL_Log("gamepad subsystem unavailable: %s", SDL_GetError());
    set_defaults();

    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    if (ids) {
        for (int i = 0; i < count && i < MAX_PLAYERS; i++)
            g_pads[i] = SDL_OpenGamepad(ids[i]);
        SDL_free(ids);
    }
}

void input_shutdown(void)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
        if (g_pads[i]) SDL_CloseGamepad(g_pads[i]);
}

void input_request_quit(void)  { g_quit = true; }
bool input_quit_requested(void) { return g_quit; }

void input_handle_event(const SDL_Event *ev)
{
    switch (ev->type) {
    case SDL_EVENT_QUIT:
        g_quit = true;
        break;
    case SDL_EVENT_GAMEPAD_ADDED:
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (!g_pads[i]) { g_pads[i] = SDL_OpenGamepad(ev->gdevice.which); break; }
        }
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (g_pads[i] &&
                SDL_GetGamepadID(g_pads[i]) == ev->gdevice.which) {
                SDL_CloseGamepad(g_pads[i]);
                g_pads[i] = NULL;
            }
        }
        break;
    default:
        break;
    }
}

static bool pad_action(SDL_Gamepad *pad, InputAction a)
{
    if (!pad) return false;
    const int DEADZONE = 22000;
    switch (a) {
    case IN_LEFT:
        return SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_LEFT) ||
               SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX) < -DEADZONE;
    case IN_RIGHT:
        return SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT) ||
               SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX) > DEADZONE;
    case IN_UP:
        return SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_UP) ||
               SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY) < -DEADZONE;
    case IN_DOWN:
        return SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_DOWN) ||
               SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY) > DEADZONE;
    case IN_JUMP:  return SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH);
    case IN_SHOOT: return SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_WEST);
    case IN_BOMB:  return SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_EAST);
    case IN_KNIFE: return SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_NORTH);
    case IN_START: return SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_START);
    default:       return false;
    }
}

void input_new_frame(void)
{
    memcpy(g_prev, g_held, sizeof g_held);

    const bool *keys = SDL_GetKeyboardState(NULL);
    for (int p = 0; p < MAX_PLAYERS; p++) {
        for (int a = 0; a < IN_COUNT; a++) {
            SDL_Scancode sc = g_keymap[p].keys[a];
            bool down = (sc != SDL_SCANCODE_UNKNOWN) && keys[sc];
            if (!down) down = pad_action(g_pads[p], (InputAction)a);
            g_held[p][a] = down || g_force[p][a];
        }
    }
}

bool input_held(int player, InputAction a)
{
    if (player < 0 || player >= MAX_PLAYERS) return false;
    return g_held[player][a];
}

bool input_pressed(int player, InputAction a)
{
    if (player < 0 || player >= MAX_PLAYERS) return false;
    return g_held[player][a] && !g_prev[player][a];
}
