#pragma once
// Combat game-feel on a landed hit: brief hit-stop, camera-shake request, blood
// VFX. Disabled until the real game turns it on. Ports juice.gd.
#include "actor.h"

struct Juice {
    bool enabled = false;
    float hitstop_timer = 0.0f;   // counted in REAL time

    void init();                   // connect Events.hit_landed
    void update(float real_dt);    // decay hit-stop in real time
    float time_scale() const { return hitstop_timer > 0.0f ? 0.04f : 1.0f; }

    void on_hit_landed(Actor* target, float amount);
};

extern Juice g_juice;
