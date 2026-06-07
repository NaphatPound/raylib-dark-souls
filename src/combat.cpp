#include "combat.h"
#include "events.h"
#include "mathx.h"
#include <algorithm>

// ---------------------------------------------------------------- Health
void Health::init(float mx) {
    max_health = mx;
    current = mx;
    dead = false;
    changed.emit(current, max_health);
}
void Health::take_damage(float amount) {
    if (dead || amount <= 0.0f) return;
    current = fmaxf(0.0f, current - amount);
    damaged.emit(amount);
    changed.emit(current, max_health);
    if (current <= 0.0f) {
        dead = true;
        died.emit();
    }
}
void Health::heal(float amount) {
    if (dead || amount <= 0.0f) return;
    current = fminf(max_health, current + amount);
    changed.emit(current, max_health);
}
void Health::heal_full() {
    dead = false;
    current = max_health;
    changed.emit(current, max_health);
}

// ---------------------------------------------------------------- Stamina
void Stamina::init(float mx) {
    max_stamina = mx;
    current = mx;
    regen_timer = 0.0f;
    changed.emit(current, max_stamina);
}
void Stamina::update(float dt) {
    if (regen_timer > 0.0f) {
        regen_timer = fmaxf(0.0f, regen_timer - dt);
    } else if (current < max_stamina) {
        current = fminf(max_stamina, current + regen_rate * dt);
        changed.emit(current, max_stamina);
    }
}
bool Stamina::spend(float a) {
    if (current < a) return false;
    current -= a;
    regen_timer = regen_delay;
    changed.emit(current, max_stamina);
    return true;
}
void Stamina::drain(float a) {
    if (a <= 0.0f) return;
    current = fmaxf(0.0f, current - a);
    regen_timer = regen_delay;
    changed.emit(current, max_stamina);
}
void Stamina::refill() {
    current = max_stamina;
    regen_timer = 0.0f;
    changed.emit(current, max_stamina);
}

// ---------------------------------------------------------------- Hurtbox
bool Hurtbox::take_hit(const Hit& h) {
    if (invulnerable) return false;
    if (owner) owner->on_hurt(h);
    return true;
}

// ---------------------------------------------------------------- Hitbox
void Hitbox::begin(float dmg, float poise) {
    if (dmg >= 0.0f) damage = dmg;
    if (poise >= 0.0f) poise_damage = poise;
    already_hit.clear();
    active = true;
}
void Hitbox::end() {
    active = false;
    already_hit.clear();
}
Vector3 Hitbox::world_center(Vector3 owner_pos, Vector3 aim_dir) const {
    return owner_pos + Vector3{ 0, up, 0 } + aim_dir * fwd;
}

// distance from point p to a vertical segment [a, b] (a.y < b.y, same x/z)
static float dist_point_vsegment(Vector3 p, Vector3 a, Vector3 b) {
    float y = fminf(fmaxf(p.y, a.y), b.y);
    float dx = p.x - a.x, dy = p.y - y, dz = p.z - a.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

void Hitbox::scan(Vector3 owner_pos, Vector3 aim_dir, const std::vector<Hurtbox*>& targets) {
    if (!active) return;
    Vector3 center = world_center(owner_pos, aim_dir);
    for (Hurtbox* hb : targets) {
        if (!hb || hb->owner == owner) continue;
        if (std::find(already_hit.begin(), already_hit.end(), hb) != already_hit.end()) continue;
        Vector3 hp = hb->owner ? hb->owner->position() : Vector3{ 0, 0, 0 };
        Vector3 a{ hp.x, hp.y + hb->center_y - hb->height * 0.5f, hp.z };
        Vector3 b{ hp.x, hp.y + hb->center_y + hb->height * 0.5f, hp.z };
        if (dist_point_vsegment(center, a, b) > radius + hb->radius) continue;

        Vector3 dir = flat(hp - owner_pos);
        if (Vector3Length(dir) > 0.001f) dir = Vector3Normalize(dir);
        else dir = { 0, 0, 0 };
        Hit hit;
        hit.damage = damage;
        hit.poise_damage = poise_damage;
        hit.knockback = dir * knockback_force;
        hit.source = owner;
        hit.unblockable = unblockable;
        // only consume / report the hit if it actually connected (i-frames return false)
        if (hb->take_hit(hit)) {
            already_hit.push_back(hb);
            g_events.hit_landed.emit(hb->owner, damage);
        }
    }
}
