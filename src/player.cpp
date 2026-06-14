#include "player.h"
#include "sword.h"
#include "game.h"
#include "events.h"
#include "audio_sys.h"
#include "fx.h"
#include "input.h"
#include "assets.h"
#include "arena.h"
#include "mathx.h"
#include "rlgl.h"
#include <cmath>

// The Godot->glTF export keeps the mixamo art facing +Z; tune here if the model
// renders facing the wrong way (see notes — verified on first run).
static const float MODEL_FACE_OFFSET = 0.0f;
static const float BODY_RADIUS = 0.4f;

void Player::init(Vector3 spawn) {
    pos = spawn;
    light_combo = {
        { "standing_melee_attack_horizontal", 0.85f, 0.28f, 0.52f, 16.0f, 20.0f, 15.0f },
        { "standing_melee_attack_backhand",   0.90f, 0.30f, 0.55f, 18.0f, 22.0f, 15.0f },
        { "standing_melee_attack_downward",   1.00f, 0.34f, 0.60f, 24.0f, 30.0f, 18.0f },
    };
    heavy = { "standing_melee_attack_360_high", 1.30f, 0.45f, 0.80f, 38.0f, 55.0f, 30.0f };

    model = LoadModel(assets::path("characters/player.glb").c_str());
    anim.load(&model, assets::path("characters/player.glb").c_str());

    hurtbox.owner = this; hurtbox.center_y = 0.9f; hurtbox.radius = 0.5f; hurtbox.height = 1.7f;
    hitbox.owner = this;  hitbox.radius = 1.0f; hitbox.fwd = 1.2f; hitbox.up = 1.0f;

    health.init(100.0f);
    stamina.init(100.0f);
    flask_charges = flask_max;

    health.changed.connect([](float c, float m) { g_events.player_health_changed.emit(c, m); });
    stamina.changed.connect([](float c, float m) { g_events.player_stamina_changed.emit(c, m); });
    health.died.connect([this]() { on_died(); });
    g_events.camera_shake.connect([this](float a) { shake_amt = fmaxf(shake_amt, a); });

    g_game.player = this;
    g_events.player_health_changed.emit(health.current, health.max_health);
    g_events.player_stamina_changed.emit(stamina.current, stamina.max_stamina);
    g_events.player_flask_changed.emit(flask_charges, flask_max);

    // face the boss (-Z) on spawn
    Vector3 fwd = yaw_forward(cam_yaw); fwd = Vector3{ -fwd.x, 0, -fwd.z }; // FORWARD(-Z) rotated by cam_yaw
    target_yaw = atan2f(fwd.x, fwd.z) + facing_offset;
    visual_yaw = target_yaw;

    camera.up = { 0, 1, 0 };
    camera.fovy = 50.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camera.position = pos + Vector3{ 0, cam_height + 1.0f, 3.0f };
    camera.target = pos + Vector3{ 0, cam_height, 0 };

    set_state(IDLE);
}

void Player::unload() { anim.unload(); UnloadModel(model); }

void Player::apply_shader(Shader s) {
    for (int i = 0; i < model.materialCount; i++) model.materials[i].shader = s;
}

void Player::set_state(int s) { state = s; state_time = 0.0f; }

// ---------------------------------------------------------------- input
void Player::handle_input() {
    if (state == DEAD) return;
    if (autopilot) {
        // verification driver: approach and swing when in reach
        ap_timer -= GetFrameTime();
        if (g_game.boss && (state == IDLE || state == MOVE) && ap_timer <= 0.0f) {
            if (Vector3Distance(pos, g_game.boss->position()) <= 2.6f) { try_attack(false); ap_timer = 0.8f; }
        }
        return;
    }
    Vector2 md = GetMouseDelta();
    if (IsCursorHidden() && (md.x != 0.0f || md.y != 0.0f)) {
        mouse_idle = 0.0f;
        cam_yaw -= md.x * mouse_sensitivity;
        cam_pitch = Clamp(cam_pitch + md.y * mouse_sensitivity, 0.05f, 1.3f);
    }
    if (in::act_pressed_attack_light()) {
        if (!IsCursorHidden()) { DisableCursor(); return; }
        try_attack(false);
    } else if (in::act_pressed_attack_heavy()) {
        try_attack(true);
    } else if (in::act_pressed_dodge()) {
        try_dodge();
    } else if (in::act_pressed_heal()) {
        try_heal();
    } else if (in::act_pressed_interact()) {
        try_riposte();
    } else if (in::act_pressed_lock_on()) {
        toggle_lock_on();
    }
}

// ---------------------------------------------------------------- main loop
void Player::update(float dt, const std::vector<Hurtbox*>& targets) {
    targets_ = &targets;
    state_time += dt;
    mouse_idle += dt;
    update_riposte_window(dt);

    if (!on_floor) velocity.y -= gravity * dt;
    else velocity.y = fmaxf(velocity.y, -0.1f);

    update_lock_target();
    switch (state) {
        case IDLE: case MOVE: process_ground(dt); break;
        case DODGE:   process_dodge(dt); break;
        case ATTACK:  process_attack(dt); break;
        case BLOCK:   process_block(dt); break;
        case HIT:     process_hit(dt); break;
        case HEAL:    process_heal(dt); break;
        case RIPOSTE: process_riposte(dt); break;
        case DEAD:    velocity.x = 0; velocity.z = 0; break;
    }
    move_and_collide(dt);
    if (on_floor) g_fx.track_movement(this, pos, Vector2Length({ velocity.x, velocity.z }), dt);
    update_visual_facing(dt);
    update_camera(dt);
    stamina.update(dt);
    anim.update(dt);
    update_sword_trail(dt);

    // keep the live hitbox scanning while an attack window is open
    if (hitbox.active) hitbox.scan(pos, aim_dir, targets);
}

// ---------------------------------------------------------------- ground/move
Vector3 Player::wish_dir() {
    if (autopilot && g_game.boss) {
        Vector3 d = flat(g_game.boss->position() - pos);
        if (Vector3Length(d) > 2.2f) return Vector3Normalize(d);
        return { 0, 0, 0 };
    }
    Vector2 iv = in::move_vector();
    if (iv.x == 0.0f && iv.y == 0.0f) return { 0, 0, 0 };
    Vector3 dir = rotate_y(Vector3{ iv.x, 0, iv.y }, cam_yaw);
    dir.y = 0;
    return Vector3Normalize(dir);
}

void Player::process_ground(float dt) {
    move_dir = wish_dir();
    bool blocking = in::act_down_block();
    if (blocking && Vector3Length(move_dir) < 0.1f) { enter_block(); return; }
    bool sprinting = in::act_down_sprint() && Vector3Length(move_dir) > 0.1f && stamina.current > 1.0f;
    float speed = walk_speed;
    if (sprinting) {
        speed = run_speed;
        stamina.drain(sprint_drain * dt);
        if (stamina.current <= 0.0f) speed = walk_speed;
    }
    velocity.x = move_dir.x * speed;
    velocity.z = move_dir.z * speed;
    if (Vector3Length(move_dir) > 0.1f) {
        anim.set_anim(sprinting ? "standing_run_forward" : "standing_walk_forward");
        if (state != MOVE) set_state(MOVE);
    } else {
        anim.set_anim("standing_idle");
        if (state != IDLE) set_state(IDLE);
    }
}

// ---------------------------------------------------------------- dodge
void Player::try_dodge() {
    if (state == DODGE || state == HIT || state == DEAD) return;
    if (!stamina.spend(dodge_cost)) return;
    Vector3 dir = wish_dir();
    if (Vector3Length(dir) < 0.1f) dir = facing_forward() * -1.0f;   // no input -> dodge back
    move_dir = Vector3Normalize(dir);
    // choose the directional dodge clip from the dodge dir relative to the player's facing
    Vector3 fwd = facing_forward();
    Vector3 rightv{ -fwd.z, 0.0f, fwd.x };
    float fdot = Vector3DotProduct(move_dir, fwd);
    float rdot = Vector3DotProduct(move_dir, rightv);
    const char* clip = (fabsf(fdot) >= fabsf(rdot))
        ? (fdot >= 0.0f ? "dodge_forward" : "dodge_backward")
        : (rdot >= 0.0f ? "dodge_right" : "dodge_left");
    set_state(DODGE);
    anim.play_fitted(clip, dodge_duration);
    g_audio.play("dodge", 0.06f);
    g_fx.dodge(pos);
    end_attack_hit();
}

void Player::process_dodge(float dt) {
    hurtbox.invulnerable = (state_time >= dodge_iframe_start && state_time <= dodge_iframe_end);
    float t = Clamp(state_time / dodge_duration, 0.0f, 1.0f);
    float falloff = 1.0f - t;
    velocity.x = move_dir.x * dodge_speed * falloff;
    velocity.z = move_dir.z * dodge_speed * falloff;
    if (state_time >= dodge_duration) {
        hurtbox.invulnerable = false;
        set_state(IDLE);
    }
}

Vector3 Player::facing_forward() {
    float a = visual_yaw - facing_offset;
    return Vector3Normalize(Vector3{ sinf(a), 0, cosf(a) });
}

// ---------------------------------------------------------------- attacks
void Player::try_attack(bool heavy_) {
    if (state == DODGE || state == HIT || state == DEAD) return;
    if (state == ATTACK) { attack_buffered = true; buffered_heavy = heavy_; return; }
    begin_attack(heavy_, 0);
}

void Player::begin_attack(bool heavy_, int index) {
    AttackDef def = heavy_ ? heavy : light_combo[index];
    if (!stamina.spend(def.cost)) { set_state(IDLE); return; }
    attack_def = def;
    attack_index = index;
    attack_buffered = false;
    hit_active = false;
    hit_done = false;
    hitbox.damage = def.dmg;
    hitbox.poise_damage = def.poise;
    hitbox.unblockable = false;
    set_state(ATTACK);
    anim.play_fitted(def.anim, def.dur);
    g_audio.play("swing", 0.10f, -1.0f, heavy_ ? 0.9f : 1.1f);
    Vector3 ad;
    if (locked && lock_target) ad = lock_target->position() - pos;
    else ad = yaw_forward(cam_yaw) * -1.0f;     // FORWARD(-Z) rotated by cam_yaw
    ad.y = 0;
    if (Vector3Length(ad) > 0.01f) {
        aim_dir = Vector3Normalize(ad);
        face_instant(aim_dir);
    }
}

void Player::process_attack(float dt) {
    velocity.x = move_toward(velocity.x, 0.0f, 20.0f * dt);
    velocity.z = move_toward(velocity.z, 0.0f, 20.0f * dt);
    AttackDef& def = attack_def;
    if (!hit_done && state_time >= def.h0) {
        hit_done = true; hit_active = true;
        hitbox.begin(def.dmg, def.poise);
    } else if (hit_active && state_time >= def.h1) {
        end_attack_hit();
    }
    if (state_time >= def.dur) {
        if (attack_buffered) {
            int next_index = attack_index + 1;
            if (buffered_heavy) begin_attack(true, 0);
            else if (next_index < (int)light_combo.size()) begin_attack(false, next_index);
            else begin_attack(false, 0);
        } else {
            set_state(IDLE);
        }
    }
}

void Player::end_attack_hit() {
    if (hit_active) { hit_active = false; hitbox.end(); }
}

// ---------------------------------------------------------------- block
void Player::enter_block() {
    if (state != BLOCK) {
        set_state(BLOCK);
        anim.set_anim("standing_block_idle");
        parry_timer = parry_window;
    }
}

void Player::process_block(float dt) {
    parry_timer = fmaxf(0.0f, parry_timer - dt);
    velocity.x = move_toward(velocity.x, 0.0f, 30.0f * dt);
    velocity.z = move_toward(velocity.z, 0.0f, 30.0f * dt);
    if (!anim.playing()) anim.set_anim("standing_block_idle");
    if (!in::act_down_block()) set_state(IDLE);
}

// ---------------------------------------------------------------- estus flask
void Player::try_heal() {
    if (state != IDLE && state != MOVE) return;
    if (!on_floor || flask_charges <= 0) return;
    if (health.current >= health.max_health) return;
    flask_charges -= 1;
    g_events.player_flask_changed.emit(flask_charges, flask_max);
    flask_done = false;
    set_state(HEAL);
    g_audio.play("heal", 0.04f);
    g_fx.heal(pos + Vector3{ 0, 0.9f, 0 });
    anim.play_fitted("standing_taunt_battlecry", flask_duration);
}

void Player::process_heal(float dt) {
    velocity.x = move_toward(velocity.x, 0.0f, 24.0f * dt);
    velocity.z = move_toward(velocity.z, 0.0f, 24.0f * dt);
    if (!flask_done && state_time >= flask_duration * flask_apply_at) {
        flask_done = true;
        health.heal(flask_heal);
    }
    if (state_time >= flask_duration) set_state(IDLE);
}

// ---------------------------------------------------------------- parry / riposte
void Player::do_parry(Actor* source) {
    parry_timer = 0.0f;
    if (source) source->parried();   // fills the boss posture bar; a full bar -> kneel (executable)
    g_audio.play("block", 0.05f, 2.0f, 1.5f);
    g_events.camera_shake.emit(0.7f);
    g_fx.parry(pos + Vector3{ 0, 1.2f, 0 });
    set_state(IDLE);
    anim.set_anim("standing_idle");
}

void Player::update_riposte_window(float dt) {
    if (riposte_timer <= 0.0f) return;
    bool lost = (riposte_target == nullptr);
    if (!lost && riposte_target->is_dead()) lost = true;
    riposte_timer -= dt;
    if (lost || riposte_timer <= 0.0f) {
        riposte_timer = 0.0f;
        riposte_target = nullptr;
        g_events.riposte_ended.emit();
    }
}

void Player::try_riposte() {
    if (state == DODGE || state == DEAD || state == ATTACK || state == RIPOSTE) return;
    // deathblow only on a posture-broken (kneeling) boss
    Actor* target = (g_game.boss && g_game.boss->executable()) ? g_game.boss : nullptr;
    if (!target) return;
    if (Vector3Distance(pos, target->position()) > riposte_range) return;
    do_riposte(target);
}

void Player::do_riposte(Actor* target) {
    riposte_pending = target;
    riposte_done = false;
    g_events.boss_executable_ended.emit();   // consume the red dot immediately
    Vector3 dir = target->position() - pos;
    dir.y = 0;
    if (Vector3Length(dir) > 0.01f) {
        dir = Vector3Normalize(dir);
        face_instant(dir);
        aim_dir = dir;
        // close the gap: stand right in front of the enemy so the stab actually connects
        Vector3 tp = target->position(); tp.y = pos.y;
        pos = tp - dir * 0.95f;
        arena::resolve(pos, BODY_RADIUS);
    }
    set_state(RIPOSTE);
    anim.play_fitted("stabbing", riposte_duration);   // full stab
    g_audio.play("swing", 0.05f, 0.0f, 0.9f);
    // blood + lunge cue the moment the deathblow begins
    g_fx.hit(target->position() + Vector3{ 0, 1.0f, 0 }, 1.2f);
    g_audio.play("hit", 0.10f, 0.0f, 0.85f);
    g_events.camera_shake.emit(0.4f);
}

void Player::process_riposte(float dt) {
    velocity.x = move_toward(velocity.x, 0.0f, 22.0f * dt);
    velocity.z = move_toward(velocity.z, 0.0f, 22.0f * dt);
    // light blood from the wind-up before the blade lands
    if (!riposte_done && riposte_pending && fmodf(state_time, 0.22f) < dt)
        g_fx.hit(riposte_pending->position() + Vector3{ 0, 1.05f, 0 }, 0.8f);
    // the deathblow fires the instant the blade drives into the body (mid-thrust),
    // so the enemy falls immediately on contact rather than at the end of the stab.
    if (!riposte_done && state_time >= riposte_duration * riposte_apply_at) {
        riposte_done = true;
        if (riposte_pending) {
            Vector3 c = riposte_pending->position() + Vector3{ 0, 1.1f, 0 };
            g_fx.riposte(c);                       // takedown burst
            g_fx.hit(c, 2.4f);                     // heavy blood spray
            g_fx.impact_wave(riposte_pending->position() + Vector3{ 0, 0.5f, 0 }, 1.4f);
            g_events.camera_shake.emit(1.2f);
            g_audio.play("hit", 0.05f, 3.0f, 0.78f);   // deep, loud impact
            riposte_pending->take_riposte(riposte_damage);   // -> boss falls back now
        }
    }
    // return to the normal camera at ~80% of the enemy's falling-death (don't wait for its end)
    const float contact = riposte_duration * riposte_apply_at;
    const float fall_dur = 1.4f;   // matches the boss falling_back_death fit
    if (riposte_done && state_time >= contact + fall_dur * 0.8f) {
        riposte_pending = nullptr;
        set_state(IDLE);
    }
}

// ---------------------------------------------------------------- hit / death
void Player::on_hurt(const Hit& hit) {
    if (state == DEAD) return;
    float dmg = hit.damage;
    if (state == BLOCK && !hit.unblockable && hit.source) {
        Vector3 to_src = Vector3Normalize(hit.source->position() - pos);
        Vector3 facing = facing_forward();
        if (Vector3DotProduct(facing, to_src) > 0.25f) {
            if (parry_timer > 0.0f) { do_parry(hit.source); return; }
            float stamina_cost = dmg * 0.8f;
            bool guard_break = stamina.current < stamina_cost;
            stamina.drain(stamina_cost);
            if (guard_break) {
                health.take_damage(dmg);
                if (health.is_dead()) return;
                stagger();
            } else {
                health.take_damage(dmg * (1.0f - block_soak));
                g_audio.play("block", 0.08f);
                if (health.is_dead()) return;
                anim.play_fitted("standing_block_react_large", 0.4f);
            }
            return;
        }
    }
    health.take_damage(dmg);
    if (Vector3Length(hit.knockback) > 0.0f) velocity = velocity + hit.knockback;
    if (!health.is_dead()) stagger();
}

void Player::stagger() {
    end_attack_hit();
    set_state(HIT);
    anim.play_fitted("standing_react_large_gut", 0.45f);
}

void Player::process_hit(float dt) {
    velocity.x = move_toward(velocity.x, 0.0f, 18.0f * dt);
    velocity.z = move_toward(velocity.z, 0.0f, 18.0f * dt);
    if (state_time >= 0.45f) set_state(IDLE);
}

void Player::on_died() {
    end_attack_hit();
    hurtbox.invulnerable = true;
    set_state(DEAD);
    anim.play_fitted("standing_react_large_gut", 1.2f);
    g_game.on_player_died();
}

// ---------------------------------------------------------------- lock-on
void Player::toggle_lock_on() {
    if (locked) {
        locked = false; lock_target = nullptr;
        g_events.lock_on_changed.emit(nullptr);
        return;
    }
    Actor* boss = g_game.boss;
    if (boss) {   // matches player.gd: lock onto the boss if it exists (cleared next tick if dead)
        locked = true; lock_target = boss;
        g_events.lock_on_changed.emit(boss);
    }
}

void Player::update_lock_target() {
    if (locked && (lock_target == nullptr || lock_target->is_dead())) {
        locked = false; lock_target = nullptr;
        g_events.lock_on_changed.emit(nullptr);
    }
}

// ---------------------------------------------------------------- facing/cam
void Player::face_instant(Vector3 dir) {
    dir.y = 0;
    if (Vector3Length(dir) < 0.01f) return;
    target_yaw = atan2f(dir.x, dir.z) + facing_offset;
    visual_yaw = target_yaw;
}

void Player::update_visual_facing(float dt) {
    // hold facing during a dodge so the directional (fwd/back/left/right) clip reads correctly
    if (state == ATTACK || state == HIT || state == DEAD || state == DODGE) return;
    Vector3 face_dir{ 0, 0, 0 };
    if (locked && lock_target && !lock_target->is_dead()) face_dir = lock_target->position() - pos;
    else if (Vector3Length(move_dir) > 0.1f) face_dir = move_dir;
    if (Vector3Length(face_dir) > 0.01f) {
        face_dir.y = 0;
        target_yaw = atan2f(face_dir.x, face_dir.z) + facing_offset;
    }
    visual_yaw = lerp_angle(visual_yaw, target_yaw, rotation_speed * dt);
}

void Player::update_camera(float dt) {
    // cinematic takedown: pan to a low front 3/4 shot so we watch the stab from the front,
    // then the normal rig lerps back once the riposte ends (state -> IDLE).
    if (state == RIPOSTE && riposte_pending) {
        Vector3 e = riposte_pending->position();
        Vector3 fwd = flat(e - pos);
        if (Vector3Length(fwd) < 0.01f) fwd = aim_dir;
        fwd = Vector3Normalize(fwd);
        Vector3 right{ fwd.z, 0.0f, -fwd.x };
        Vector3 mid = Vector3Lerp(pos, e, 0.5f) + Vector3{ 0, 1.1f, 0 };
        Vector3 cpos = e + fwd * 2.0f + right * 1.5f + Vector3{ 0, 0.9f, 0 };  // beyond+beside the enemy
        float k = Clamp(cam_lerp * 0.5f * dt, 0.0f, 1.0f);
        camera.position = Vector3Lerp(camera.position, cpos, k);
        camera.target   = Vector3Lerp(camera.target, mid, k);
        return;
    }
    Vector3 pivot = pos + Vector3{ 0, cam_height, 0 };
    if (locked && lock_target && !lock_target->is_dead()) {
        Vector3 d = flat(lock_target->position() - pos);
        if (Vector3Length(d) > 0.05f) {
            d = Vector3Normalize(d);
            cam_yaw = lerp_angle(cam_yaw, atan2f(-d.x, -d.z), 8.0f * dt);
        }
        cam_pitch = Lerp(cam_pitch, 0.30f, 5.0f * dt);
    } else if (state == MOVE && Vector3Length(move_dir) > 0.1f && mouse_idle > 0.9f) {
        float follow_yaw = atan2f(-move_dir.x, -move_dir.z);
        cam_yaw = lerp_angle(cam_yaw, follow_yaw, cam_follow_speed * dt);
    }
    Vector3 back{ sinf(cam_yaw), 0, cosf(cam_yaw) };
    Vector3 target_pos = pivot + back * (cam_distance * cosf(cam_pitch)) + Vector3{ 0, cam_distance * sinf(cam_pitch), 0 };
    if (!cam_started) { camera.position = target_pos; cam_started = true; }
    else camera.position = Vector3Lerp(camera.position, target_pos, Clamp(cam_lerp * dt, 0.0f, 1.0f));
    // shake: jitter the look target so framing stays put
    shake_amt = move_toward(shake_amt, 0.0f, 4.0f * dt);
    float j = shake_amt * 0.05f;
    camera.target = pivot + Vector3{ rand_range(-j, j), rand_range(-j, j), 0 };
}

void Player::move_and_collide(float dt) {
    pos = pos + velocity * dt;
    arena::resolve(pos, BODY_RADIUS);
    on_floor = pos.y <= arena::floor_y + 0.001f;
}

// ---------------------------------------------------------------- draw
void Player::draw() {
    DrawModelEx(model, pos, Vector3{ 0, 1, 0 }, visual_yaw * RAD2DEG + MODEL_FACE_OFFSET * RAD2DEG,
                Vector3{ 1, 1, 1 }, WHITE);
    draw_sword_trail();
    draw_sword();
}

void Player::draw_sword() {
    if (!sword_enabled) return;
    draw_hand_sword(model, anim, pos, visual_yaw + MODEL_FACE_OFFSET);
}

// Sample the blade each frame during a swing and keep a short history; the segments
// connect into a faint white "wind" ribbon that fades out behind the blade.
void Player::update_sword_trail(float dt) {
    for (auto& s : sword_trail_) s.age += dt;
    while (!sword_trail_.empty() && sword_trail_.front().age >= trail_life_)
        sword_trail_.erase(sword_trail_.begin());

    bool swinging = sword_enabled && (state == ATTACK || state == RIPOSTE);
    if (!swinging) return;
    Vector3 base, tip;
    sword_blade_segment(model, anim, pos, visual_yaw + MODEL_FACE_OFFSET, base, tip);
    // only emit when the blade has actually moved, so slow wind-ups don't clump
    if (sword_trail_.empty() || Vector3Distance(tip, sword_trail_.back().tip) > 0.05f)
        sword_trail_.push_back({ base, tip, 0.0f });
}

void Player::draw_sword_trail() {
    if (sword_trail_.size() < 2) return;
    rlDisableBackfaceCulling();
    rlDisableDepthMask();                  // translucent: depth-test but don't write (no self-sort)
    // Alpha-blended pure white: washes toward white over ANY background, so it stays
    // white even crossing the bright blood-moon (additive there went pink). Solid-white
    // default texture guarantees no stray texel; alpha only ever lightens, never black.
    rlSetTexture(rlGetTextureIdDefault());
    BeginBlendMode(BLEND_ALPHA);
    rlBegin(RL_QUADS);
    // bright white at the blade, fading fast (cubic) to clear along the short tail
    for (size_t i = 1; i < sword_trail_.size(); i++) {
        const SwordTrailSeg& a = sword_trail_[i - 1];
        const SwordTrailSeg& b = sword_trail_[i];
        float fa = 1.0f - a.age / trail_life_;  fa = fa * fa * fa;   // 1 near blade -> 0 far
        float fb = 1.0f - b.age / trail_life_;  fb = fb * fb * fb;
        unsigned char ta = (unsigned char)(160.0f * fa);
        unsigned char tb = (unsigned char)(160.0f * fb);
        rlColor4ub(255, 255, 255, (unsigned char)(ta * 0.40f)); rlVertex3f(a.base.x, a.base.y, a.base.z);
        rlColor4ub(255, 255, 255, ta);                          rlVertex3f(a.tip.x, a.tip.y, a.tip.z);
        rlColor4ub(255, 255, 255, tb);                          rlVertex3f(b.tip.x, b.tip.y, b.tip.z);
        rlColor4ub(255, 255, 255, (unsigned char)(tb * 0.40f)); rlVertex3f(b.base.x, b.base.y, b.base.z);
    }
    rlEnd();
    EndBlendMode();
    rlSetTexture(0);
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

// ---------------------------------------------------------------- reset
void Player::reset_state() {
    health.heal_full();
    stamina.refill();
    flask_charges = flask_max;
    g_events.player_flask_changed.emit(flask_charges, flask_max);
    hurtbox.invulnerable = false;
    locked = false; lock_target = nullptr;
    parry_timer = 0.0f; riposte_timer = 0.0f;
    riposte_target = nullptr; riposte_pending = nullptr;
    g_events.riposte_ended.emit();
    velocity = { 0, 0, 0 };
    sword_trail_.clear();
    set_state(IDLE);
}
