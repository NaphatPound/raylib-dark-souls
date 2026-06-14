#pragma once
// SFX pool + crossfading ambient/fight music (Godot's `Audio` autoload),
// driven by the Events bus and direct play() calls.
#include "raylib.h"
#include "actor.h"
#include <string>
#include <vector>
#include <unordered_map>

struct SfxVoices {
    Sound base{};
    std::vector<Sound> aliases;   // round-robin so repeats overlap with independent pitch
    int next = 0;
    bool loaded = false;
};

struct AudioSys {
    std::unordered_map<std::string, SfxVoices> sfx;
    Music ambient{};
    Music fight{};
    Music fire{};                 // looping bonfire crackle (intro), gated by fire_on/off
    bool has_ambient = false, has_fight = false, has_fire = false;

    // music volume tracked in dB (ramped at a constant 40 dB/s like the original)
    float amb_db = -7.0f, fight_db = -60.0f, fire_db = -60.0f;
    float amb_target_db = -7.0f, fight_target_db = -60.0f, fire_target_db = -60.0f;
    float master = 1.0f, music = 1.0f, sfx_vol = 1.0f;

    void init();
    void update(float dt);
    void play(const std::string& name, float pitch_var = 0.0f,
              float vol_db = 0.0f, float base_pitch = 1.0f);
    void on_state(int state);
    void fire_on()  { fire_target_db = -7.0f; }   // bonfire audible
    void fire_off() { fire_target_db = -60.0f; }  // silence (ramped)
    void shutdown();
};

extern AudioSys g_audio;
