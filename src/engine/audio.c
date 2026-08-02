#include <SDL3_mixer/SDL_mixer.h>
#include <string.h>

#include "engine.h"

#define VOICES 32

static MIX_Mixer *g_mixer;
static MIX_Audio *g_sounds[SOUND_COUNT];
static MIX_Track *g_voices[VOICES];
static int        g_voice_sound[VOICES];
static int        g_next_voice;

static MIX_Audio *g_music;
static MIX_Track *g_music_track;

static bool g_enabled;

bool audio_init(const char *asset_dir)
{
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("audio subsystem unavailable: %s", SDL_GetError());
        return false;
    }
    if (!MIX_Init()) {
        SDL_Log("MIX_Init failed: %s", SDL_GetError());
        return false;
    }
    g_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (!g_mixer) {
        SDL_Log("MIX_CreateMixerDevice failed: %s", SDL_GetError());
        return false;
    }

    for (int i = 0; i < SOUND_COUNT; i++) {
        char path[1024];
        SDL_snprintf(path, sizeof path, "%s/sfx/%s", asset_dir, sound_defs[i].file);
        g_sounds[i] = MIX_LoadAudio(g_mixer, path, true);
        if (!g_sounds[i])
            SDL_Log("failed to load %s: %s", path, SDL_GetError());
    }

    for (int i = 0; i < VOICES; i++) {
        g_voices[i] = MIX_CreateTrack(g_mixer);
        g_voice_sound[i] = -1;
    }

    g_enabled = true;
    return true;
}

void audio_shutdown(void)
{
    if (!g_enabled) return;
    for (int i = 0; i < VOICES; i++)
        if (g_voices[i]) MIX_DestroyTrack(g_voices[i]);
    if (g_music_track) MIX_DestroyTrack(g_music_track);
    if (g_music) MIX_DestroyAudio(g_music);
    for (int i = 0; i < SOUND_COUNT; i++)
        if (g_sounds[i]) MIX_DestroyAudio(g_sounds[i]);
    if (g_mixer) MIX_DestroyMixer(g_mixer);
    MIX_Quit();
    g_enabled = false;
}

static int pick_voice(void)
{
    for (int i = 0; i < VOICES; i++) {
        int v = (g_next_voice + i) % VOICES;
        if (g_voices[v] && !MIX_TrackPlaying(g_voices[v])) {
            g_next_voice = (v + 1) % VOICES;
            return v;
        }
    }
    int v = g_next_voice;
    g_next_voice = (v + 1) % VOICES;
    return v;
}

void sound_play_pitch(int snd, float pitch, float volume)
{
    if (!g_enabled || snd < 0 || snd >= SOUND_COUNT || !g_sounds[snd]) return;

    int v = pick_voice();
    MIX_Track *t = g_voices[v];
    if (!t) return;

    MIX_StopTrack(t, 0);
    if (!MIX_SetTrackAudio(t, g_sounds[snd])) return;
    MIX_SetTrackFrequencyRatio(t, pitch);
    MIX_SetTrackGain(t, volume * sound_defs[snd].volume);
    g_voice_sound[v] = snd;
    MIX_PlayTrack(t, 0);
}

void sound_play(int snd)
{
    sound_play_pitch(snd, 1.0f, 1.0f);
}

void sound_stop(int snd)
{
    if (!g_enabled) return;
    for (int i = 0; i < VOICES; i++)
        if (g_voice_sound[i] == snd && g_voices[i])
            MIX_StopTrack(g_voices[i], 0);
}

void music_play(const char *file, bool loop)
{
    if (!g_enabled) return;
    music_stop();

    char path[1024];
    SDL_snprintf(path, sizeof path, "%s/music/%s", g_asset_dir, file);
    g_music = MIX_LoadAudio(g_mixer, path, false);
    if (!g_music) {
        SDL_Log("failed to load music %s: %s", path, SDL_GetError());
        return;
    }
    if (!g_music_track) g_music_track = MIX_CreateTrack(g_mixer);
    if (!g_music_track) return;

    MIX_SetTrackAudio(g_music_track, g_music);

    SDL_PropertiesID opts = SDL_CreateProperties();
    SDL_SetNumberProperty(opts, MIX_PROP_PLAY_LOOPS_NUMBER, loop ? -1 : 0);
    MIX_PlayTrack(g_music_track, opts);
    SDL_DestroyProperties(opts);
}

void music_stop(void)
{
    if (!g_enabled) return;
    if (g_music_track) MIX_StopTrack(g_music_track, 0);
    if (g_music) { MIX_DestroyAudio(g_music); g_music = NULL; }
}
