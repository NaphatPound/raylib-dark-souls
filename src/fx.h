#pragma once
// One-shot action VFX (blood, parry flash, dodge dust, heal motes, takedown
// burst, enrage shockwave, water ripples) — a small CPU billboard/ring pool
// replacing Godot's CPUParticles3D. Ports fx.gd.
#include "raylib.h"
#include <vector>
#include <string>
#include <unordered_map>

struct FxParticle {
    Vector3 pos, vel, grav;
    float life, age = 0.0f, size;
    Color color;
    Texture2D tex;
    bool additive;
};

struct FxRing {
    Vector3 pos;
    Color color;
    float scale0, scale1, dur, age = 0.0f;
    Texture2D tex;
    bool ground;     // lay flat on XZ (water ripples) vs camera-facing
};

struct Fx {
    void init();
    void update(float dt);
    void draw(const Camera3D& cam);

    void hit(Vector3 pos, float amount);
    void impact_wave(Vector3 pos);
    void parry(Vector3 pos);
    void riposte(Vector3 pos);
    void enrage(Vector3 pos);
    void dodge(Vector3 pos);
    void heal(Vector3 pos);
    void ripple(Vector3 pos, float speed = 1.0f);
    void track_movement(void* id, Vector3 pos, float h_speed, float dt);

private:
    Texture2D tex(const std::string& name);
    void burst(Vector3 pos, const std::string& tname, Color color, int count,
               float life, float vmin, float vmax, float spread_deg,
               Vector3 dir, Vector3 grav, float smin, float smax, bool additive);
    void ring(Vector3 pos, Color color, float end_scale, float dur, bool ground);

    std::vector<FxParticle> parts_;
    std::vector<FxRing> rings_;
    std::unordered_map<std::string, Texture2D> tcache_;
    std::unordered_map<void*, float> step_;
    static constexpr float WATER_Y = 0.06f;   // just above the water surface (0.02)
    static constexpr float STEP_DIST = 0.9f;
};

extern Fx g_fx;
