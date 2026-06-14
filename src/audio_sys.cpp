#include "audio_sys.h"
#include "assets.h"
#include "events.h"
#include "game.h"
#include "mathx.h"
#include <cmath>

AudioSys g_audio;

static float db_to_lin(float db) { return powf(10.0f, db / 20.0f); }
static const int VOICES = 8;     // matches the original's 8-player SFX pool

void AudioSys::init() {
    const char* names[] = { "swing", "dodge", "hit", "block", "heal", "roar", "death", "victory" };
    for (const char* n : names) {
        std::string p = assets::path(std::string("audio/sfx/") + n + ".wav");
        if (!FileExists(p.c_str())) continue;
        SfxVoices v;
        v.base = LoadSound(p.c_str());
        v.loaded = true;
        for (int i = 0; i < VOICES; i++) v.aliases.push_back(LoadSoundAlias(v.base));
        sfx[n] = std::move(v);
    }
    std::string amb = assets::path("audio/music/ambient.wav");
    std::string fgt = assets::path("audio/music/fight.wav");
    std::string fir = assets::path("audio/sfx/fire.wav");
    if (FileExists(amb.c_str())) { ambient = LoadMusicStream(amb.c_str()); ambient.looping = true; has_ambient = true; }
    if (FileExists(fgt.c_str())) { fight = LoadMusicStream(fgt.c_str()); fight.looping = true; has_fight = true; }
    if (FileExists(fir.c_str())) { fire = LoadMusicStream(fir.c_str()); fire.looping = true; has_fire = true; }

    amb_db = amb_target_db = -7.0f;
    fight_db = fight_target_db = -60.0f;
    fire_db = fire_target_db = -60.0f;
    if (has_ambient) { SetMusicVolume(ambient, db_to_lin(amb_db)); PlayMusicStream(ambient); }
    if (has_fight)   { SetMusicVolume(fight, db_to_lin(fight_db)); PlayMusicStream(fight); }
    if (has_fire)    { SetMusicVolume(fire, db_to_lin(fire_db)); PlayMusicStream(fire); }

    g_events.hit_landed.connect([this](Actor*, float) { play("hit", 0.10f, -2.0f); });
    g_events.boss_aggro.connect([this]() { play("roar", 0.05f, 0.0f, 0.95f); });
    g_game.state_changed.connect([this](int s) { on_state(s); });
}

void AudioSys::update(float dt) {
    if (has_ambient) UpdateMusicStream(ambient);
    if (has_fight)   UpdateMusicStream(fight);
    if (has_fire)    UpdateMusicStream(fire);
    // constant-rate dB ramp (matches audio.gd move_toward at 40 dB/s)
    amb_db   = move_toward(amb_db, amb_target_db, 40.0f * dt);
    fight_db = move_toward(fight_db, fight_target_db, 40.0f * dt);
    fire_db  = move_toward(fire_db, fire_target_db, 40.0f * dt);
    if (has_ambient) SetMusicVolume(ambient, db_to_lin(amb_db) * music * master);
    if (has_fight)   SetMusicVolume(fight, db_to_lin(fight_db) * music * master);
    if (has_fire)    SetMusicVolume(fire, db_to_lin(fire_db) * sfx_vol * master);
}

void AudioSys::play(const std::string& name, float pitch_var, float vol_db, float base_pitch) {
    auto it = sfx.find(name);
    if (it == sfx.end() || it->second.aliases.empty()) return;
    SfxVoices& v = it->second;
    Sound& s = v.aliases[v.next];
    v.next = (v.next + 1) % (int)v.aliases.size();
    float pitch = fmaxf(0.05f, base_pitch + rand_range(-pitch_var, pitch_var));
    SetSoundPitch(s, pitch);
    SetSoundVolume(s, db_to_lin(vol_db) * sfx_vol * master);
    PlaySound(s);
}

void AudioSys::on_state(int state) {
    switch (state) {
        case Game::FIGHT:   amb_target_db = -60.0f; fight_target_db = -5.0f; break;
        case Game::DEAD:    play("death", 0.0f, -1.0f); amb_target_db = -10.0f; fight_target_db = -60.0f; break;
        case Game::VICTORY: play("victory", 0.0f, -2.0f); amb_target_db = -8.0f; fight_target_db = -60.0f; break;
        default:            amb_target_db = -7.0f; fight_target_db = -60.0f; break;
    }
}

void AudioSys::shutdown() {
    for (auto& kv : sfx) {
        for (Sound& a : kv.second.aliases) UnloadSoundAlias(a);
        if (kv.second.loaded) UnloadSound(kv.second.base);
    }
    sfx.clear();
    if (has_ambient) UnloadMusicStream(ambient);
    if (has_fight)   UnloadMusicStream(fight);
    if (has_fire)    UnloadMusicStream(fire);
}
