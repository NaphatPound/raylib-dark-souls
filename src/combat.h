#pragma once
// Combat framework: Hit payload, Health/Stamina components, and Hitbox/Hurtbox
// resolved geometrically (sphere vs vertical capsule) instead of Godot Area3D.
#include "raylib.h"
#include "signal.h"
#include "actor.h"
#include <vector>

struct Hit {
    float   damage = 0.0f;
    float   poise_damage = 0.0f;
    Vector3 knockback{ 0, 0, 0 };
    Actor*  source = nullptr;
    bool    unblockable = false;
};

struct Health {
    float max_health = 100.0f;
    float current = 0.0f;
    bool  dead = false;
    Signal<float, float> changed;   // (current, max)
    Signal<float>        damaged;   // (amount)
    Signal<>             died;

    void init(float mx);
    bool is_dead() const { return dead; }
    void take_damage(float amount);
    void heal(float amount);
    void heal_full();
    float fraction() const { return max_health > 0.0f ? current / max_health : 0.0f; }
};

struct Stamina {
    float max_stamina = 100.0f;
    float current = 0.0f;
    float regen_rate = 25.0f;
    float regen_delay = 0.6f;
    float regen_timer = 0.0f;
    Signal<float, float> changed;

    void init(float mx);
    void update(float dt);
    bool can_spend(float a) const { return current >= a; }
    bool spend(float a);
    void drain(float a);
    void refill();
};

struct Hurtbox {
    Actor* owner = nullptr;
    float  center_y = 0.9f;   // capsule mid-height above the actor origin
    float  radius = 0.5f;
    float  height = 1.7f;
    bool   invulnerable = false;

    // Returns true if the hit connected (false if i-frames swallowed it).
    bool take_hit(const Hit& h);
};

struct Hitbox {
    Actor* owner = nullptr;
    float  radius = 1.0f;
    float  fwd = 1.2f;        // reach in front of the actor (aim -Z)
    float  up = 1.0f;         // height above origin
    float  damage = 10.0f;
    float  poise_damage = 10.0f;
    float  knockback_force = 4.0f;
    bool   unblockable = false;

    bool active = false;
    std::vector<Hurtbox*> already_hit;

    void begin(float dmg = -1.0f, float poise = -1.0f);
    void end();
    // Poll opposing hurtboxes; owner_pos is the actor origin, aim_dir a unit XZ
    // vector toward the target. Mirrors hitbox.gd's per-frame _scan/_try_hit.
    void scan(Vector3 owner_pos, Vector3 aim_dir, const std::vector<Hurtbox*>& targets);
    Vector3 world_center(Vector3 owner_pos, Vector3 aim_dir) const;
};
