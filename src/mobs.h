#pragma once
// Hollow horde: a lightweight multi-enemy system for non-boss levels (the Hollow
// Graveyard). A Mob is a weaker, simpler cousin of the Boss -- one attack, low
// HP, no phases -- reusing enemy.glb plus the shared combat/anim systems. Several
// roam the level; clearing them all wins it (no boss). Levels 0-3 never touch it.
#include "raylib.h"
#include "actor.h"
#include "combat.h"
#include "anim.h"
#include <vector>

class Mob : public Actor {
public:
    enum S { SLEEP, CHASE, ATTACK, STAGGER, DEAD };

    void init(Vector3 spawn, float yaw, Shader lit, float hp, float speed);
    void unload();
    void update(float dt, const std::vector<Hurtbox*>& targets);
    void draw();
    void reset();

    Vector3 position() const override { return pos; }
    bool is_dead() const override { return health.is_dead(); }
    void on_hurt(const Hit& hit) override;

    Hurtbox hurtbox;
    Hitbox  hitbox;
    Health  health;

    Vector3 pos{ 0, 0, 0 };
    int state = SLEEP;

private:
    void set_state(int s);
    void process_sleep(float dt);
    void process_chase(float dt);
    void start_attack();
    void process_attack(float dt);
    void enter_stagger();
    void process_stagger(float dt);
    void process_dead();
    void on_died();
    void face_player(float dt);
    void aim_at_player();
    void move_and_collide(float dt);

    Model model{};
    AnimController anim;
    Vector3 spawn_pos{ 0, 0, 0 };
    float spawn_yaw = 0.0f;

    // tuning (per-instance HP / speed set in init)
    float max_health = 55.0f, chase_speed = 2.6f;
    float gravity = 22.0f, aggro_range = 15.0f, attack_range = 2.2f, rotation_speed = 6.0f;
    float max_poise = 30.0f, poise_regen = 12.0f, stagger_time = 1.4f, attack_cooldown = 1.1f;
    float atk_dur = 1.5f, atk_h0 = 0.55f, atk_h1 = 0.85f, atk_dmg = 12.0f, atk_poise = 18.0f;

    Vector3 velocity{ 0, 0, 0 };
    bool on_floor = true;
    float state_time = 0.0f;
    float poise = 30.0f;
    float visual_yaw = 0.0f;
    Vector3 aim_dir{ 0, 0, 1 };
    bool hit_active = false, hit_done = false;
    float cooldown = 0.0f;
    float tele_energy = 0.0f;
};

namespace mobs {
    void spawn(Shader lit);                         // build the level's horde
    void reset();                                   // respawn all (retry after death)
    // Drives AI + separation; player_pos is pushed out of mobs; player_targets is
    // the player hurtbox the mobs strike. Handles fight-start / lock-on / victory.
    void update(float dt, const std::vector<Hurtbox*>& player_targets,
                Vector3& player_pos, float player_radius);
    void draw();
    void unload();
    std::vector<Hurtbox*>& targets();               // the player's attack targets (mob hurtboxes)
    int  alive();
}
