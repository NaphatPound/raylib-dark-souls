#pragma once
// Global signal bus (Godot's `Events` autoload) decoupling gameplay from UI/audio.
#include "signal.h"
#include "actor.h"

struct Events {
    Signal<float, float> player_health_changed;   // (current, max)
    Signal<float, float> player_stamina_changed;
    Signal<float, float> boss_health_changed;
    Signal<int>          boss_phase_changed;
    Signal<Actor*>       lock_on_changed;          // target or nullptr
    Signal<>             boss_aggro;
    Signal<Actor*, float> hit_landed;              // (target, amount)
    Signal<int, int>     player_flask_changed;     // (current, max)
    Signal<float>        camera_shake;             // (amount)
    Signal<Actor*>       riposte_available;        // parry landed -> show takedown prompt
    Signal<>             riposte_ended;            // window closed / consumed
    Signal<float, float> boss_posture_changed;     // (current, max) Sekiro-style posture bar
    Signal<Actor*>       boss_executable;          // posture broke -> boss kneeling, show red dot
    Signal<>             boss_executable_ended;     // deathblow consumed / boss recovered
};

extern Events g_events;
