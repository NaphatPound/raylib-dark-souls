#pragma once
// Souls-like player controller: camera-relative movement, sprint, third-person
// camera with lock-on, stamina, i-frame dodge, light/heavy combos, block,
// parry→riposte, estus heal, hit reactions and death. Ports player.gd.
#include "raylib.h"
#include "actor.h"
#include "combat.h"
#include "anim.h"
#include <vector>
#include <string>

struct AttackDef {
    std::string anim;
    float dur, h0, h1, dmg, poise, cost;
};

struct SwordTrailSeg { Vector3 base, tip; float age; };   // one sampled blade pose

class Player : public Actor {
public:
    enum S { IDLE, MOVE, DODGE, ATTACK, BLOCK, HIT, DEAD, HEAL, RIPOSTE };

    void init(Vector3 spawn);
    void unload();
    void handle_input();                                       // _unhandled_input (pressed events + mouse look)
    void update(float dt, const std::vector<Hurtbox*>& targets); // _physics_process
    void draw();
    void reset_state();
    void apply_shader(Shader s);

    // Actor
    Vector3 position() const override { return pos; }
    bool is_dead() const override { return health.is_dead(); }
    void on_hurt(const Hit& hit) override;

    void set_mouse_sensitivity(float v) { mouse_sensitivity = v; }
    float get_mouse_sensitivity() const { return mouse_sensitivity; }

    Camera3D camera{};
    Hurtbox hurtbox;
    Hitbox  hitbox;
    Health  health;
    Stamina stamina;

    Vector3 pos{ 0, 0, 16 };
    int state = IDLE;
    int flask_charges = 4;
    bool locked = false;
    bool sword_enabled = true;    // attach decorative sword to right hand
    bool autopilot = false;       // verification only: walk into the boss and attack
    float ap_timer = 0.0f;

private:
    void set_state(int s);
    Vector3 wish_dir();
    void process_ground(float dt);
    void try_dodge();
    void process_dodge(float dt);
    Vector3 facing_forward();
    void try_attack(bool heavy);
    void begin_attack(bool heavy, int index);
    void process_attack(float dt);
    void end_attack_hit();
    void enter_block();
    void process_block(float dt);
    void try_heal();
    void process_heal(float dt);
    void do_parry(Actor* source);
    void update_riposte_window(float dt);
    void try_riposte();
    void do_riposte();
    void process_riposte(float dt);
    void stagger();
    void process_hit(float dt);
    void on_died();
    void toggle_lock_on();
    void update_lock_target();
    void face_instant(Vector3 dir);
    void update_visual_facing(float dt);
    void update_camera(float dt);
    void move_and_collide(float dt);
    void draw_sword();
    void update_sword_trail(float dt);
    void draw_sword_trail();

    Model model{};
    AnimController anim;

    // tuning
    float walk_speed = 3.0f, run_speed = 6.0f, rotation_speed = 12.0f, gravity = 22.0f;
    float mouse_sensitivity = 0.0032f, sprint_drain = 18.0f;
    float dodge_cost = 25.0f, dodge_speed = 9.0f, dodge_duration = 0.6f;
    float dodge_iframe_start = 0.10f, dodge_iframe_end = 0.45f;
    float block_soak = 0.6f, facing_offset = 0.0f;
    int   flask_max = 4;
    float flask_heal = 45.0f, flask_duration = 1.1f, flask_apply_at = 0.55f;
    float parry_window = 0.28f, riposte_window = 2.2f, riposte_range = 3.0f;
    float riposte_damage = 80.0f, riposte_duration = 0.95f, riposte_apply_at = 0.4f;
    float cam_distance = 2.0f, cam_height = 1.5f, cam_follow_speed = 2.5f, cam_lerp = 16.0f;

    std::vector<AttackDef> light_combo;
    AttackDef heavy;

    // runtime
    Vector3 velocity{ 0, 0, 0 };
    bool on_floor = true;
    float state_time = 0.0f;
    float cam_yaw = 0.0f, cam_pitch = 0.2f, target_yaw = 0.0f, visual_yaw = 0.0f;
    Vector3 move_dir{ 0, 0, 0 };
    Vector3 aim_dir{ 0, 0, -1 };
    int   attack_index = 0;
    AttackDef attack_def{};
    bool  attack_buffered = false, buffered_heavy = false;
    bool  hit_active = false, hit_done = false;
    Actor* lock_target = nullptr;
    bool  cam_started = false;
    float mouse_idle = 999.0f, shake_amt = 0.0f;
    bool  flask_done = false;
    float parry_timer = 0.0f;
    Actor* riposte_target = nullptr;
    float riposte_timer = 0.0f;
    Actor* riposte_pending = nullptr;
    bool  riposte_done = false;

    std::vector<SwordTrailSeg> sword_trail_;
    float trail_life_ = 0.08f;       // short: born and gone fast (white near blade -> clear tail)

    const std::vector<Hurtbox*>* targets_ = nullptr;
};
