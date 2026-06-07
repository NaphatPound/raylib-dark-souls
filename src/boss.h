#pragma once
// Multi-phase zombie boss. FSM: sleep -> roar -> chase -> attack, with poise/
// stagger, a phase transition at 50% HP, telegraphs, and a death sequence.
// Ports boss.gd.
#include "raylib.h"
#include "actor.h"
#include "combat.h"
#include "anim.h"
#include <vector>
#include <string>

struct BossAttack {
    std::string anim;
    float dur, h0, h1, dmg, poise, range;
    bool unblock;
};

class Boss : public Actor {
public:
    enum S { SLEEP, ROAR, IDLE, CHASE, ATTACK, STAGGER, PHASE, DEAD };

    void init(Vector3 spawn);
    void unload();
    void update(float dt, const std::vector<Hurtbox*>& targets);
    void draw();
    void reset_state();
    void apply_shader(Shader s);

    Vector3 position() const override { return pos; }
    bool is_dead() const override { return health.is_dead(); }
    void on_hurt(const Hit& hit) override;
    void parried() override;
    void take_riposte(float dmg) override;

    Hurtbox hurtbox;
    Hitbox  hitbox;
    Health  health;

    Vector3 pos{ 0, 0, -10 };
    int phase = 1;
    int state = SLEEP;

private:
    void set_state(int s);
    Actor* player();
    void process_sleep(float dt);
    void on_aggro();
    void process_roar(float dt);
    void process_chase(float dt);
    void choose_attack();
    std::string pick_attack_key();
    void start_attack(const std::string& key);
    void process_attack(float dt);
    void end_hit();
    void enter_stagger();
    void process_stagger(float dt);
    void enter_phase2();
    void process_phase(float dt);
    void process_dead();
    void on_died();
    void face_player(float dt);
    void face_instant();
    void aim_at_player();
    void move_and_collide(float dt);

    Model model{};
    AnimController anim;

    float max_health = 600.0f, walk_speed = 1.8f, chase_speed = 3.2f, chase_speed_p2 = 5.0f;
    float gravity = 22.0f, aggro_range = 24.0f, attack_range = 2.8f, rotation_speed = 7.0f;
    float facing_offset = 0.0f, phase2_threshold = 0.5f, max_poise = 70.0f, poise_regen = 18.0f;
    float stagger_time = 2.2f, parry_stun_time = 2.8f, attack_cooldown = 0.6f;

    std::vector<std::pair<std::string, BossAttack>> attacks;

    Vector3 velocity{ 0, 0, 0 };
    bool on_floor = true;
    float state_time = 0.0f;
    float poise = 70.0f;
    float visual_yaw = 0.0f;
    Vector3 aim_dir{ 0, 0, 1 };
    BossAttack attack_def{};
    bool hit_active = false, hit_done = false;
    float cooldown = 0.0f;
    int combo_left = 0;
    bool parry_stun = false;
    float tele_energy = 0.0f;
    Color tele_color{ 255, 30, 25, 255 };
};
