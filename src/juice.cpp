#include "juice.h"
#include "events.h"
#include "fx.h"
#include "mathx.h"

Juice g_juice;

void Juice::init() {
    g_events.hit_landed.connect([this](Actor* t, float a) { on_hit_landed(t, a); });
}

void Juice::update(float real_dt) {
    if (hitstop_timer > 0.0f) hitstop_timer = fmaxf(0.0f, hitstop_timer - real_dt);
}

void Juice::on_hit_landed(Actor* target, float amount) {
    if (!enabled) return;
    g_events.camera_shake.emit(fminf(fmaxf(0.5f + amount * 0.02f, 0.5f), 1.6f));
    if (target) {
        Vector3 hp = target->position() + Vector3{ 0, 1.1f, 0 };
        g_fx.hit(hp, amount);
        g_fx.impact_wave(hp);
    }
    if (hitstop_timer <= 0.0f) hitstop_timer = 0.07f;   // don't extend an active freeze (juice.gd _stopping guard)
}
