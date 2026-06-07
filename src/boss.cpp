#include "boss.h"
#include "game.h"
#include "events.h"
#include "audio_sys.h"
#include "fx.h"
#include "assets.h"
#include "arena.h"
#include "mathx.h"
#include <cmath>

static const float MODEL_FACE_OFFSET = 0.0f;
static const float BODY_RADIUS = 0.45f;

static const BossAttack* find_attack(const std::vector<std::pair<std::string, BossAttack>>& v, const std::string& k) {
    for (auto& p : v) if (p.first == k) return &p.second;
    return nullptr;
}

void Boss::init(Vector3 spawn) {
    pos = spawn;
    attacks = {
        { "swipe", { "zombie_attack",    1.7f, 0.55f, 0.85f, 18.0f, 25.0f, 3.2f, false } },
        { "bite",  { "zombie_biting",    1.9f, 0.60f, 0.95f, 26.0f, 30.0f, 2.8f, false } },
        { "neck",  { "zombie_neck_bite", 2.1f, 0.70f, 1.10f, 32.0f, 38.0f, 2.6f, true  } },
        { "bite2", { "zombie_biting_2",  1.6f, 0.55f, 0.90f, 24.0f, 28.0f, 3.0f, false } },
    };

    model = LoadModel(assets::path("characters/enemy.glb").c_str());
    anim.load(&model, assets::path("characters/enemy.glb").c_str());

    hurtbox.owner = this; hurtbox.center_y = 0.9f; hurtbox.radius = 0.6f; hurtbox.height = 1.7f;
    hitbox.owner = this;  hitbox.radius = 1.1f; hitbox.fwd = 1.4f; hitbox.up = 1.0f;

    poise = max_poise;
    health.changed.connect([](float c, float m) { g_events.boss_health_changed.emit(c, m); });
    health.died.connect([this]() { on_died(); });
    health.init(max_health);
    g_game.boss = this;
    g_events.boss_phase_changed.emit(phase);
    g_events.boss_aggro.connect([this]() { on_aggro(); });

    // face the player on spawn (player is at +Z, boss at -Z)
    visual_yaw = 0.0f;
    set_state(SLEEP);
    anim.set_anim("zombie_idle");
}

void Boss::unload() { anim.unload(); UnloadModel(model); }

void Boss::apply_shader(Shader s) {
    for (int i = 0; i < model.materialCount; i++) model.materials[i].shader = s;
}

void Boss::set_state(int s) { state = s; state_time = 0.0f; }
Actor* Boss::player() { return (g_game.player && !g_game.player->is_dead()) ? g_game.player : g_game.player; }

void Boss::update(float dt, const std::vector<Hurtbox*>& targets) {
    state_time += dt;
    if (cooldown > 0.0f) cooldown -= dt;
    if (!on_floor) velocity.y -= gravity * dt;
    else velocity.y = fmaxf(velocity.y, -0.1f);
    if (poise < max_poise && state != STAGGER && state != DEAD)
        poise = fminf(max_poise, poise + poise_regen * dt);
    if (state != ATTACK && tele_energy > 0.0f)
        tele_energy = move_toward(tele_energy, 0.0f, 10.0f * dt);

    switch (state) {
        case SLEEP: process_sleep(dt); break;
        case ROAR:  process_roar(dt); break;
        case IDLE: case CHASE: process_chase(dt); break;
        case ATTACK:  process_attack(dt); break;
        case STAGGER: process_stagger(dt); break;
        case PHASE:   process_phase(dt); break;
        case DEAD:    process_dead(); break;
    }
    move_and_collide(dt);
    if (on_floor) g_fx.track_movement(this, pos, Vector2Length({ velocity.x, velocity.z }), dt);
    anim.update(dt);
    if (hitbox.active) hitbox.scan(pos, aim_dir, targets);
}

// ---------------------------------------------------------------- sleep/aggro
void Boss::process_sleep(float) {
    velocity.x = 0; velocity.z = 0;
    Actor* p = g_game.player;
    if (p && Vector3Distance(pos, p->position()) <= aggro_range)
        g_game.start_fight();
}
void Boss::on_aggro() {
    if (state == SLEEP) {
        set_state(ROAR);
        anim.play_fitted("zombie_scream", 2.0f);
    }
}
void Boss::process_roar(float dt) {
    velocity.x = move_toward(velocity.x, 0.0f, 10.0f * dt);
    velocity.z = move_toward(velocity.z, 0.0f, 10.0f * dt);
    face_player(dt);
    if (state_time >= 2.0f) set_state(CHASE);
}

// ---------------------------------------------------------------- chase
void Boss::process_chase(float dt) {
    Actor* p = g_game.player;
    if (!p) { anim.set_anim("zombie_idle"); velocity.x = 0; velocity.z = 0; return; }
    face_player(dt);
    Vector3 to = flat(p->position() - pos);
    float dist = Vector3Length(to);
    if (dist <= attack_range && cooldown <= 0.0f) { choose_attack(); return; }
    if (dist <= attack_range) {
        velocity.x = move_toward(velocity.x, 0.0f, 12.0f * dt);
        velocity.z = move_toward(velocity.z, 0.0f, 12.0f * dt);
        anim.set_anim("zombie_idle");
        return;
    }
    float spd = (phase == 2) ? chase_speed_p2 : chase_speed;
    Vector3 dir = Vector3Normalize(to);
    velocity.x = dir.x * spd;
    velocity.z = dir.z * spd;
    if (phase == 2) anim.set_anim(spd > 3.5f ? "zombie_run" : "zombie_walk");
    else anim.set_anim(dist > 6.0f ? "zombie_run" : "zombie_walk");
}

// ---------------------------------------------------------------- attack
void Boss::choose_attack() {
    // deterministic, timing-derived combo length (matches boss.gd _choose_attack)
    combo_left = (int)fabsf(state_time * 1300.0f) % (phase == 2 ? 3 : 2);
    start_attack(pick_attack_key());
}
std::string Boss::pick_attack_key() {
    std::vector<std::string> pool = (phase == 1)
        ? std::vector<std::string>{ "swipe", "bite" }
        : std::vector<std::string>{ "swipe", "bite", "neck", "bite2" };
    int idx = (int)fabsf(state_time * 1000.0f) % (int)pool.size();
    return pool[idx];
}
void Boss::start_attack(const std::string& key) {
    const BossAttack* def = find_attack(attacks, key);
    if (!def) return;
    attack_def = *def;
    hit_active = false; hit_done = false;
    hitbox.damage = def->dmg;
    hitbox.poise_damage = def->poise;
    hitbox.unblockable = def->unblock;
    set_state(ATTACK);
    anim.play_fitted(def->anim, def->dur);
    face_instant();
    aim_at_player();
    g_audio.play("swing", 0.08f, -2.0f, 0.8f);
}
void Boss::process_attack(float dt) {
    velocity.x = move_toward(velocity.x, 0.0f, 14.0f * dt);
    velocity.z = move_toward(velocity.z, 0.0f, 14.0f * dt);
    BossAttack& def = attack_def;
    if (state_time < def.dur * 0.3f) { face_player(dt * 0.5f); aim_at_player(); }
    // telegraph: a single light blink during the wind-up so the player can react,
    // then a faint colour sustain until the strike. Colour = attack type:
    //   orange = normal attack (can parry),  red = heavy attack (cannot parry).
    if (state_time < def.h0) {
        float w = Clamp(state_time / def.h0, 0.0f, 1.0f);
        float blink = sinf(PI * Clamp(w / 0.75f, 0.0f, 1.0f));   // one swell, peaks early
        tele_energy = fmaxf(blink, 0.10f);
        tele_color = def.unblock ? Color{ 255, 35, 25, 255 }     // red  = unblockable
                                 : Color{ 255, 150, 30, 255 };   // orange = parryable
    } else {
        tele_energy = move_toward(tele_energy, 0.0f, 14.0f * dt);
    }
    if (!hit_done && state_time >= def.h0) {
        hit_done = true; hit_active = true;
        hitbox.begin(def.dmg, def.poise);
    } else if (hit_active && state_time >= def.h1) {
        end_hit();
    }
    if (state_time >= def.dur) {
        end_hit();
        Actor* p = g_game.player;
        bool inrange = p && Vector3Distance(pos, p->position()) <= attack_range + 0.8f && !p->is_dead();
        if (combo_left > 0 && inrange) {
            combo_left -= 1;
            face_instant();
            start_attack(pick_attack_key());
            return;
        }
        cooldown = attack_cooldown * (phase == 2 ? 0.6f : 1.0f);
        set_state(CHASE);
    }
}
void Boss::end_hit() {
    if (hit_active) { hit_active = false; hitbox.end(); }
}

// ---------------------------------------------------------------- damage/poise
void Boss::on_hurt(const Hit& hit) {
    if (state == DEAD) return;
    health.take_damage(hit.damage);
    if (health.is_dead()) return;
    if (phase == 1 && health.fraction() <= phase2_threshold) { enter_phase2(); return; }
    if (state != STAGGER) {
        poise -= hit.poise_damage;
        if (poise <= 0.0f) { poise = max_poise; enter_stagger(); }
    }
}
void Boss::enter_stagger() {
    end_hit();
    parry_stun = false;
    set_state(STAGGER);
    anim.set_anim("zombie_idle");
    anim.set_speed_scale(0.25f);
}
void Boss::process_stagger(float dt) {
    velocity.x = move_toward(velocity.x, 0.0f, 20.0f * dt);
    velocity.z = move_toward(velocity.z, 0.0f, 20.0f * dt);
    if (state_time >= (parry_stun ? parry_stun_time : stagger_time)) {
        anim.set_speed_scale(1.0f);
        parry_stun = false;
        set_state(CHASE);
    }
}
void Boss::parried() {
    if (state == DEAD) return;
    end_hit();
    combo_left = 0;
    poise = max_poise;
    tele_energy = 0.0f;
    hurtbox.invulnerable = false;
    parry_stun = true;
    set_state(STAGGER);
    anim.set_anim("zombie_idle");
    anim.set_speed_scale(0.2f);
}
void Boss::take_riposte(float dmg) {
    if (state == DEAD) return;
    health.take_damage(dmg);
    g_events.hit_landed.emit(this, dmg);
    if (health.is_dead()) return;
    if (phase == 1 && health.fraction() <= phase2_threshold) { enter_phase2(); return; }
    parry_stun = false;
    anim.set_speed_scale(1.0f);
    cooldown = attack_cooldown;
    set_state(CHASE);
}

// ---------------------------------------------------------------- phase 2
void Boss::enter_phase2() {
    phase = 2;
    poise = max_poise;
    end_hit();
    hurtbox.invulnerable = true;
    set_state(PHASE);
    anim.play_fitted("zombie_scream", 2.2f);
    g_audio.play("roar", 0.04f, 0.0f, 1.1f);
    g_fx.enrage(pos + Vector3{ 0, 1.0f, 0 });
    g_events.boss_phase_changed.emit(phase);
}
void Boss::process_phase(float dt) {
    velocity.x = move_toward(velocity.x, 0.0f, 10.0f * dt);
    velocity.z = move_toward(velocity.z, 0.0f, 10.0f * dt);
    face_player(dt * 0.5f);
    if (state_time >= 2.2f) {
        hurtbox.invulnerable = false;
        set_state(CHASE);
    }
}

// ---------------------------------------------------------------- death
void Boss::process_dead() {
    velocity.x = 0; velocity.z = 0;
    if (state_time >= 2.6f && anim.current() != "zombie_death" && anim.has("zombie_death"))
        anim.play_once("zombie_death");   // natural speed, holds the resting pose (boss.gd)
}
void Boss::on_died() {
    end_hit();
    hurtbox.invulnerable = true;
    anim.set_speed_scale(1.0f);
    set_state(DEAD);
    anim.play_fitted("zombie_dying", 3.0f);
    g_game.on_boss_died();
}

// ---------------------------------------------------------------- facing/aim
void Boss::face_player(float dt) {
    Actor* p = g_game.player;
    if (!p) return;
    Vector3 to = flat(p->position() - pos);
    if (Vector3Length(to) < 0.05f) return;
    float target = atan2f(to.x, to.z) + facing_offset;
    visual_yaw = lerp_angle(visual_yaw, target, rotation_speed * dt);
}
void Boss::face_instant() {
    Actor* p = g_game.player;
    if (!p) return;
    Vector3 to = flat(p->position() - pos);
    if (Vector3Length(to) < 0.05f) return;
    visual_yaw = atan2f(to.x, to.z) + facing_offset;
}
void Boss::aim_at_player() {
    Actor* p = g_game.player;
    if (!p) return;
    Vector3 to = flat(p->position() - pos);
    if (Vector3Length(to) < 0.05f) return;
    aim_dir = Vector3Normalize(to);
}

void Boss::move_and_collide(float dt) {
    pos = pos + velocity * dt;
    arena::resolve(pos, BODY_RADIUS);
    on_floor = pos.y <= arena::floor_y + 0.001f;
}

// ---------------------------------------------------------------- draw
void Boss::draw() {
    DrawModelEx(model, pos, Vector3{ 0, 1, 0 }, visual_yaw * RAD2DEG + MODEL_FACE_OFFSET * RAD2DEG,
                Vector3{ 1, 1, 1 }, WHITE);
    if (tele_energy > 0.02f) {
        // a small blinking warning light above the boss (no longer a big growing orb)
        Vector3 lp = pos + Vector3{ 0, 1.62f, 0 };
        float e = Clamp(tele_energy, 0.0f, 1.0f);
        BeginBlendMode(BLEND_ADDITIVE);
        Color core = tele_color; core.a = (unsigned char)(235 * e);
        DrawSphere(lp, 0.05f + 0.035f * e, core);
        Color halo = tele_color; halo.a = (unsigned char)(80 * e);
        DrawSphere(lp, 0.13f + 0.16f * e, halo);
        EndBlendMode();
    }
}

void Boss::reset_state() {
    phase = 1;
    poise = max_poise;
    health.heal_full();
    hurtbox.invulnerable = false;
    anim.set_speed_scale(1.0f);
    cooldown = 0.0f;
    combo_left = 0;
    parry_stun = false;
    tele_energy = 0.0f;
    velocity = { 0, 0, 0 };
    set_state(SLEEP);
    anim.set_anim("zombie_idle");
    g_events.boss_phase_changed.emit(phase);
}
