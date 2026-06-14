#include "mobs.h"
#include "game.h"
#include "events.h"
#include "audio_sys.h"
#include "fx.h"
#include "assets.h"
#include "arena.h"
#include "mathx.h"
#include <cmath>
#include <memory>

static const float BODY_RADIUS = 0.42f;

// ---------------------------------------------------------------- Mob
void Mob::init(Vector3 spawn, float yaw, Shader lit, float hp, float speed) {
    pos = spawn; spawn_pos = spawn; spawn_yaw = yaw;
    max_health = hp; chase_speed = speed;

    model = LoadModel(assets::path("characters/enemy.glb").c_str());
    anim.load(&model, assets::path("characters/enemy.glb").c_str());
    for (int i = 0; i < model.materialCount; i++) model.materials[i].shader = lit;

    hurtbox.owner = this; hurtbox.center_y = 0.85f; hurtbox.radius = 0.52f; hurtbox.height = 1.6f;
    hitbox.owner = this;  hitbox.radius = 1.0f; hitbox.fwd = 1.2f; hitbox.up = 0.95f;

    poise = max_poise;
    health.died.connect([this]() { on_died(); });
    health.init(max_health);

    visual_yaw = yaw;
    set_state(SLEEP);
    anim.set_anim("zombie_idle");
}

void Mob::unload() { anim.unload(); UnloadModel(model); }

void Mob::set_state(int s) { state = s; state_time = 0.0f; }

void Mob::update(float dt, const std::vector<Hurtbox*>& targets) {
    state_time += dt;
    if (cooldown > 0.0f) cooldown -= dt;
    if (!on_floor) velocity.y -= gravity * dt;
    else velocity.y = fmaxf(velocity.y, -0.1f);
    if (poise < max_poise && state != STAGGER && state != DEAD)
        poise = fminf(max_poise, poise + poise_regen * dt);
    if (state != ATTACK && tele_energy > 0.0f)
        tele_energy = move_toward(tele_energy, 0.0f, 8.0f * dt);

    switch (state) {
        case SLEEP:   process_sleep(dt); break;
        case CHASE:   process_chase(dt); break;
        case ATTACK:  process_attack(dt); break;
        case STAGGER: process_stagger(dt); break;
        case DEAD:    process_dead(); break;
    }
    move_and_collide(dt);
    if (on_floor) g_fx.track_movement(this, pos, Vector2Length({ velocity.x, velocity.z }), dt);
    anim.update(dt);
    if (hitbox.active) hitbox.scan(pos, aim_dir, targets);
}

// ---------------------------------------------------------------- states
void Mob::process_sleep(float) {
    velocity.x = 0; velocity.z = 0;
    Actor* p = g_game.player;
    if (p && !p->is_dead() && Vector3Distance(pos, p->position()) <= aggro_range)
        set_state(CHASE);
}

void Mob::process_chase(float dt) {
    Actor* p = g_game.player;
    if (!p || p->is_dead()) { anim.set_anim("zombie_idle"); velocity.x = 0; velocity.z = 0; return; }
    face_player(dt);
    Vector3 to = flat(p->position() - pos);
    float dist = Vector3Length(to);
    if (dist <= attack_range && cooldown <= 0.0f) { start_attack(); return; }
    if (dist <= attack_range) {
        velocity.x = move_toward(velocity.x, 0.0f, 10.0f * dt);
        velocity.z = move_toward(velocity.z, 0.0f, 10.0f * dt);
        anim.set_anim("zombie_idle");
        return;
    }
    Vector3 dir = Vector3Normalize(to);
    velocity.x = dir.x * chase_speed;
    velocity.z = dir.z * chase_speed;
    anim.set_anim(dist > 6.0f ? "zombie_run" : "zombie_walk");
}

void Mob::start_attack() {
    hit_active = false; hit_done = false;
    hitbox.damage = atk_dmg; hitbox.poise_damage = atk_poise; hitbox.unblockable = false;
    set_state(ATTACK);
    anim.play_fitted("zombie_attack", atk_dur);
    aim_at_player();
    Actor* p = g_game.player;                                  // snap-face at swing start
    if (p) { Vector3 to = flat(p->position() - pos); if (Vector3Length(to) > 0.05f) visual_yaw = atan2f(to.x, to.z); }
    g_audio.play("swing", 0.08f, -3.0f, 0.85f);
}

void Mob::process_attack(float dt) {
    velocity.x = move_toward(velocity.x, 0.0f, 12.0f * dt);
    velocity.z = move_toward(velocity.z, 0.0f, 12.0f * dt);
    if (state_time < atk_dur * 0.3f) { face_player(dt * 0.5f); aim_at_player(); }
    // a single warning blink during the wind-up, then fade (parryable -> orange-green)
    if (state_time < atk_h0) {
        float w = Clamp(state_time / atk_h0, 0.0f, 1.0f);
        float blink = sinf(PI * Clamp(w / 0.75f, 0.0f, 1.0f));
        tele_energy = fmaxf(blink, 0.10f);
    } else {
        tele_energy = move_toward(tele_energy, 0.0f, 12.0f * dt);
    }
    if (!hit_done && state_time >= atk_h0) {
        hit_done = true; hit_active = true;
        hitbox.begin(atk_dmg, atk_poise);
    } else if (hit_active && state_time >= atk_h1) {
        hit_active = false; hitbox.end();
    }
    if (state_time >= atk_dur) {
        if (hit_active) { hit_active = false; hitbox.end(); }
        cooldown = attack_cooldown;
        set_state(CHASE);
    }
}

void Mob::enter_stagger() {
    if (hit_active) { hit_active = false; hitbox.end(); }
    set_state(STAGGER);
    anim.set_anim("zombie_idle");
    anim.set_speed_scale(0.25f);
}
void Mob::process_stagger(float dt) {
    velocity.x = move_toward(velocity.x, 0.0f, 18.0f * dt);
    velocity.z = move_toward(velocity.z, 0.0f, 18.0f * dt);
    if (state_time >= stagger_time) { anim.set_speed_scale(1.0f); set_state(CHASE); }
}

void Mob::process_dead() {
    velocity.x = 0; velocity.z = 0;
    if (state_time >= 2.2f && anim.current() != "zombie_death" && anim.has("zombie_death"))
        anim.play_once("zombie_death");
}
void Mob::on_died() {
    if (hit_active) { hit_active = false; hitbox.end(); }
    hurtbox.invulnerable = true;
    anim.set_speed_scale(1.0f);
    set_state(DEAD);
    anim.play_fitted("zombie_dying", 2.4f);
    g_audio.play("roar", 0.05f, -10.0f, 1.1f);
}

// ---------------------------------------------------------------- damage
void Mob::on_hurt(const Hit& hit) {
    if (state == DEAD) return;
    health.take_damage(hit.damage);
    if (health.is_dead()) return;
    if (state != STAGGER) {
        poise -= hit.poise_damage;
        if (poise <= 0.0f) { poise = max_poise; enter_stagger(); }
    }
}

// ---------------------------------------------------------------- facing/move
void Mob::face_player(float dt) {
    Actor* p = g_game.player;
    if (!p) return;
    Vector3 to = flat(p->position() - pos);
    if (Vector3Length(to) < 0.05f) return;
    float target = atan2f(to.x, to.z);
    visual_yaw = lerp_angle(visual_yaw, target, rotation_speed * dt);
}
void Mob::aim_at_player() {
    Actor* p = g_game.player;
    if (!p) return;
    Vector3 to = flat(p->position() - pos);
    if (Vector3Length(to) < 0.05f) return;
    aim_dir = Vector3Normalize(to);
}
void Mob::move_and_collide(float dt) {
    pos = pos + velocity * dt;
    arena::resolve(pos, BODY_RADIUS);
    on_floor = pos.y <= arena::floor_y + 0.001f;
}

// ---------------------------------------------------------------- draw
void Mob::draw() {
    DrawModelEx(model, pos, Vector3{ 0, 1, 0 }, visual_yaw * RAD2DEG,
                Vector3{ 0.92f, 0.92f, 0.92f }, WHITE);   // slightly smaller than the boss
    if (tele_energy > 0.02f) {
        Vector3 lp = pos + Vector3{ 0, 1.52f, 0 };
        float e = Clamp(tele_energy, 0.0f, 1.0f);
        BeginBlendMode(BLEND_ADDITIVE);
        DrawSphere(lp, 0.045f + 0.03f * e, Color{ 205, 255, 175, (unsigned char)(220 * e) });
        DrawSphere(lp, 0.12f + 0.14f * e, Color{ 150, 255, 150, (unsigned char)(70 * e) });
        EndBlendMode();
    }
}

void Mob::reset() {
    pos = spawn_pos;
    poise = max_poise;
    health.heal_full();
    hurtbox.invulnerable = false;
    anim.set_speed_scale(1.0f);
    cooldown = 0.0f; tele_energy = 0.0f;
    hit_active = false; hit_done = false;
    velocity = { 0, 0, 0 };
    visual_yaw = spawn_yaw;
    set_state(SLEEP);
    anim.set_anim("zombie_idle");
}

// ================================================================ horde manager
namespace mobs {

static std::vector<std::unique_ptr<Mob>> s_mobs;
static std::vector<Hurtbox*> s_targets;
static float s_total_max = 1.0f;
static bool  s_fight_started = false;
static bool  s_victory = false;

void spawn(Shader lit) {
    s_mobs.clear();
    s_targets.clear();
    s_fight_started = false;
    s_victory = false;

    // Scattered across the level, clear of the central obstacle and the entrance
    // aisle (player spawns at +Z ~16). World x,z; one tougher enemy deep in.
    struct Sp { float x, z, yaw, hp, spd; };
    static const Sp grave_defs[] = {
        { -6.0f,  -1.0f,  0.4f, 52.0f, 2.7f },
        {  6.5f,  -2.5f, -0.6f, 52.0f, 2.7f },
        {  0.0f,  -8.5f,  3.14f, 70.0f, 2.4f },   // crypt guardian (tougher)
        { -9.0f,   5.0f,  1.2f, 48.0f, 2.9f },
        {  9.0f,   4.0f, -1.2f, 48.0f, 2.9f },
        {  1.0f, -15.0f,  3.14f, 60.0f, 2.5f },
    };
    static const Sp fungal_defs[] = {                 // Sporeborn: a denser, hardier cavern pack
        { -7.0f,  -2.0f,  0.5f, 58.0f, 2.5f },
        {  7.5f,  -3.0f, -0.6f, 58.0f, 2.5f },
        { -5.0f, -11.0f,  3.0f, 64.0f, 2.4f },
        {  5.0f, -12.0f, -3.0f, 64.0f, 2.4f },
        { -11.0f,  6.0f,  1.3f, 52.0f, 2.8f },
        { 11.0f,   5.0f, -1.3f, 52.0f, 2.8f },
        {  0.0f, -17.0f,  3.14f, 78.0f, 2.3f },       // grotto warden (tougher)
    };
    static const Sp desert_defs[] = {               // The Buried Dead: spread wide around the ziggurat
        { -9.0f,  -3.0f,  0.5f, 56.0f, 2.6f },
        {  9.0f,  -4.0f, -0.5f, 56.0f, 2.6f },
        { -13.0f,  7.0f,  1.3f, 50.0f, 2.9f },
        { 13.0f,   6.0f, -1.3f, 50.0f, 2.9f },
        {  0.0f, -13.0f,  3.14f, 62.0f, 2.5f },
        { -7.0f, -18.0f,  3.0f, 58.0f, 2.5f },
        {  7.0f, -18.0f, -3.0f, 58.0f, 2.5f },
    };
    static const Sp wreck_defs[] = {                // The Drowned: wading the shallows around the wrecks
        { -8.0f,  -2.0f,  0.5f, 54.0f, 2.6f },
        {  8.0f,  -3.0f, -0.5f, 54.0f, 2.6f },
        { -12.0f,  6.0f,  1.3f, 50.0f, 2.8f },
        { 12.0f,   5.0f, -1.3f, 50.0f, 2.8f },
        {  0.0f, -11.0f,  3.14f, 66.0f, 2.4f },
        { -6.0f, -17.0f,  3.0f, 56.0f, 2.5f },
        {  6.0f, -17.0f, -3.0f, 56.0f, 2.5f },
    };
    static const Sp sanctum_defs[] = {              // The Forsaken: guardians ringing the monument
        { -8.0f,  -2.0f,  0.5f, 56.0f, 2.7f },
        {  8.0f,  -3.0f, -0.5f, 56.0f, 2.7f },
        { -13.0f,  7.0f,  1.3f, 50.0f, 3.0f },
        { 13.0f,   6.0f, -1.3f, 50.0f, 3.0f },
        {  0.0f, -12.0f,  3.14f, 68.0f, 2.5f },
        { -7.0f, -18.0f,  3.0f, 56.0f, 2.6f },
        {  7.0f, -18.0f, -3.0f, 56.0f, 2.6f },
    };
    static const Sp clock_defs[] = {               // The Rusted: husks ground into the machinery
        { -8.0f,  -2.0f,  0.5f, 58.0f, 2.6f },
        {  8.0f,  -3.0f, -0.5f, 58.0f, 2.6f },
        { -13.0f,  7.0f,  1.3f, 52.0f, 2.8f },
        { 13.0f,   6.0f, -1.3f, 52.0f, 2.8f },
        {  0.0f, -12.0f,  3.14f, 72.0f, 2.4f },
        { -7.0f, -18.0f,  3.0f, 58.0f, 2.5f },
        {  7.0f, -18.0f, -3.0f, 58.0f, 2.5f },
    };
    static const Sp shrine_defs[] = {              // The Lost: pilgrims haunting the avenue + grounds
        {  0.0f,  10.0f,  3.14f, 60.0f, 2.5f },    // a guardian on the path
        { -4.0f,   6.0f,  2.6f, 54.0f, 2.6f },
        {  4.0f,   5.0f, -2.6f, 54.0f, 2.6f },
        {  0.0f,  -8.0f,  0.0f, 70.0f, 2.4f },
        { -9.0f,  -3.0f,  1.3f, 52.0f, 2.8f },
        {  9.0f,  -4.0f, -1.3f, 52.0f, 2.8f },
        {  0.0f, -14.0f,  0.0f, 60.0f, 2.5f },
    };
    static const Sp bones_defs[] = {               // The Carrion: scavengers picking the great skeleton
        {  0.0f,   8.0f,  3.14f, 58.0f, 2.6f },    // inside the ribcage
        { -4.0f,   2.0f,  2.6f, 54.0f, 2.7f },
        {  4.0f,   0.0f, -2.6f, 54.0f, 2.7f },
        {  0.0f, -10.0f,  0.0f, 72.0f, 2.4f },
        { -9.0f,  -4.0f,  1.3f, 52.0f, 2.8f },
        {  9.0f,  -5.0f, -1.3f, 52.0f, 2.8f },
        {  0.0f, -15.0f,  0.0f, 60.0f, 2.5f },
    };
    static const Sp geode_defs[] = {               // The Petrified: crystal-encrusted husks
        { -8.0f,  -2.0f,  0.5f, 58.0f, 2.6f },
        {  8.0f,  -3.0f, -0.5f, 58.0f, 2.6f },
        { -13.0f,  7.0f,  1.3f, 52.0f, 2.8f },
        { 13.0f,   6.0f, -1.3f, 52.0f, 2.8f },
        {  0.0f, -12.0f,  3.14f, 74.0f, 2.4f },
        { -7.0f, -18.0f,  3.0f, 56.0f, 2.5f },
        {  7.0f, -18.0f, -3.0f, 56.0f, 2.5f },
    };
    static const Sp temple_defs[] = {              // The Rooted: husks bound in the overgrowth
        { -8.0f,  -2.0f,  0.5f, 58.0f, 2.6f },
        {  8.0f,  -3.0f, -0.5f, 58.0f, 2.6f },
        { -13.0f,  7.0f,  1.3f, 52.0f, 2.8f },
        { 13.0f,   6.0f, -1.3f, 52.0f, 2.8f },
        {  0.0f, -12.0f,  3.14f, 72.0f, 2.4f },
        { -7.0f, -18.0f,  3.0f, 56.0f, 2.5f },
        {  7.0f, -18.0f, -3.0f, 56.0f, 2.5f },
    };
    static const Sp mine_defs[] = {                // The Delvers: husks still working the dead seam
        { -8.0f,  -2.0f,  0.5f, 58.0f, 2.6f },
        {  8.0f,  -3.0f, -0.5f, 58.0f, 2.6f },
        { -13.0f,  7.0f,  1.3f, 52.0f, 2.8f },
        { 13.0f,   6.0f, -1.3f, 52.0f, 2.8f },
        {  0.0f, -12.0f,  3.14f, 72.0f, 2.4f },
        { -7.0f, -18.0f,  3.0f, 56.0f, 2.5f },
        {  7.0f, -18.0f, -3.0f, 56.0f, 2.5f },
    };
    static const Sp obs_defs[] = {                 // The Watchers: stargazers turned husk
        { -8.0f,  -2.0f,  0.5f, 58.0f, 2.6f },
        {  8.0f,  -3.0f, -0.5f, 58.0f, 2.6f },
        { -13.0f,  7.0f,  1.3f, 52.0f, 2.8f },
        { 13.0f,   6.0f, -1.3f, 52.0f, 2.8f },
        {  0.0f, -12.0f,  3.14f, 72.0f, 2.4f },
        { -7.0f, -18.0f,  3.0f, 56.0f, 2.5f },
        {  7.0f, -18.0f, -3.0f, 56.0f, 2.5f },
    };
    static const Sp lib_defs[] = {                 // The Scholars: husks bound to the dead archive
        { -8.0f,  -2.0f,  0.5f, 58.0f, 2.6f },
        {  8.0f,  -3.0f, -0.5f, 58.0f, 2.6f },
        { -13.0f,  7.0f,  1.3f, 52.0f, 2.8f },
        { 13.0f,   6.0f, -1.3f, 52.0f, 2.8f },
        {  0.0f, -12.0f,  3.14f, 72.0f, 2.4f },
        { -7.0f, -18.0f,  3.0f, 56.0f, 2.5f },
        {  7.0f, -18.0f, -3.0f, 56.0f, 2.5f },
    };
    static const Sp camp_defs[] = {                // The Routed: a broken army's stragglers
        { -8.0f,  -2.0f,  0.5f, 60.0f, 2.7f },
        {  8.0f,  -3.0f, -0.5f, 60.0f, 2.7f },
        { -13.0f,  7.0f,  1.3f, 54.0f, 2.9f },
        { 13.0f,   6.0f, -1.3f, 54.0f, 2.9f },
        {  0.0f, -12.0f,  3.14f, 76.0f, 2.5f },    // a captain (tougher)
        { -7.0f, -18.0f,  3.0f, 58.0f, 2.6f },
        {  7.0f, -18.0f, -3.0f, 58.0f, 2.6f },
    };
    static const Sp aqua_defs[] = {                // The Wretched: things that crawled from the channel
        { -7.0f,  -2.0f,  0.5f, 58.0f, 2.6f },
        {  7.0f,  -3.0f, -0.5f, 58.0f, 2.6f },
        { -11.0f,  7.0f,  1.3f, 52.0f, 2.8f },
        { 11.0f,   6.0f, -1.3f, 52.0f, 2.8f },
        {  0.0f, -11.0f,  3.14f, 72.0f, 2.4f },
        { -6.0f, -17.0f,  3.0f, 56.0f, 2.5f },
        {  6.0f, -17.0f, -3.0f, 56.0f, 2.5f },
    };
    static const Sp court_defs[] = {               // The Faithless: husks kneeling among the sentinels
        { -8.0f,  -2.0f,  0.5f, 60.0f, 2.6f },
        {  8.0f,  -3.0f, -0.5f, 60.0f, 2.6f },
        { -13.0f,  7.0f,  1.3f, 54.0f, 2.8f },
        { 13.0f,   6.0f, -1.3f, 54.0f, 2.8f },
        {  0.0f, -12.0f,  3.14f, 78.0f, 2.4f },    // a knight-husk (tougher)
        { -7.0f, -18.0f,  3.0f, 58.0f, 2.5f },
        {  7.0f, -18.0f, -3.0f, 58.0f, 2.5f },
    };
    static const Sp garden_defs[] = {              // The Withered: gardeners gone to seed
        { -9.0f,  -2.0f,  0.5f, 56.0f, 2.7f },
        {  9.0f,  -3.0f, -0.5f, 56.0f, 2.7f },
        { -13.0f,  7.0f,  1.3f, 50.0f, 2.9f },
        { 13.0f,   6.0f, -1.3f, 50.0f, 2.9f },
        {  0.0f, -12.0f,  3.14f, 70.0f, 2.5f },
        { -7.0f, -18.0f,  3.0f, 54.0f, 2.6f },
        {  7.0f, -18.0f, -3.0f, 54.0f, 2.6f },
    };
    static const Sp bridge_defs[] = {              // The Stranded: travellers trapped on the span (kept on the deck)
        { -6.0f,  -2.0f,  0.5f, 58.0f, 2.6f },
        {  6.0f,  -3.0f, -0.5f, 58.0f, 2.6f },
        { -7.0f,   9.0f,  3.14f, 54.0f, 2.7f },
        {  7.0f,   9.0f,  3.14f, 54.0f, 2.7f },
        {  0.0f, -12.0f,  3.14f, 74.0f, 2.5f },
        { -6.0f, -19.0f,  3.0f, 56.0f, 2.6f },
        {  6.0f, -19.0f, -3.0f, 56.0f, 2.6f },
    };
    static const Sp beacon_defs[] = {              // The Marooned: castaways washed onto the rocks
        { -8.0f,  -2.0f,  0.5f, 58.0f, 2.6f },
        {  8.0f,  -3.0f, -0.5f, 58.0f, 2.6f },
        { -13.0f,  7.0f,  1.3f, 52.0f, 2.8f },
        { 13.0f,   6.0f, -1.3f, 52.0f, 2.8f },
        {  0.0f, -12.0f,  3.14f, 72.0f, 2.4f },
        { -7.0f, -18.0f,  3.0f, 56.0f, 2.5f },
        {  7.0f, -18.0f, -3.0f, 56.0f, 2.5f },
    };
    static const Sp mill_defs[] = {                // The Reaped: field-hands turned husk
        { -9.0f,  -2.0f,  0.5f, 56.0f, 2.7f },
        {  9.0f,  -3.0f, -0.5f, 56.0f, 2.7f },
        { -13.0f,  7.0f,  1.3f, 50.0f, 2.9f },
        { 13.0f,   6.0f, -1.3f, 50.0f, 2.9f },
        {  0.0f, -12.0f,  3.14f, 70.0f, 2.5f },
        { -7.0f, -18.0f,  3.0f, 54.0f, 2.6f },
        {  7.0f, -18.0f, -3.0f, 54.0f, 2.6f },
    };
    static const Sp ossuary_defs[] = {             // The Interred: the dead who would not stay
        { -8.0f,  -2.0f,  0.5f, 58.0f, 2.6f },
        {  8.0f,  -3.0f, -0.5f, 58.0f, 2.6f },
        { -12.0f,  7.0f,  1.3f, 52.0f, 2.8f },
        { 12.0f,   6.0f, -1.3f, 52.0f, 2.8f },
        {  0.0f, -12.0f,  3.14f, 74.0f, 2.4f },
        { -7.0f, -18.0f,  3.0f, 56.0f, 2.5f },
        {  7.0f, -18.0f, -3.0f, 56.0f, 2.5f },
    };
    static const Sp fair_defs[] = {                // The Revelers: the crowd that never left
        { -9.0f,  -2.0f,  0.5f, 56.0f, 2.7f },
        {  9.0f,  -3.0f, -0.5f, 56.0f, 2.7f },
        { -13.0f,  7.0f,  1.3f, 50.0f, 2.9f },
        { 13.0f,   6.0f, -1.3f, 50.0f, 2.9f },
        {  0.0f, -13.0f,  3.14f, 70.0f, 2.5f },
        { -7.0f, -18.0f,  3.0f, 54.0f, 2.6f },
        {  7.0f, -18.0f, -3.0f, 54.0f, 2.6f },
    };
    static const Sp throne_defs[] = {              // The Pretenders: those who would seize the empty throne
        { -8.0f,  -2.0f,  0.5f, 60.0f, 2.6f },
        {  8.0f,  -3.0f, -0.5f, 60.0f, 2.6f },
        { -9.0f,   9.0f,  3.14f, 54.0f, 2.7f },
        {  9.0f,   9.0f,  3.14f, 54.0f, 2.7f },
        {  0.0f, -10.0f,  0.0f, 80.0f, 2.4f },     // a usurper-knight before the throne (tougher)
        { -6.0f, -16.0f,  0.0f, 56.0f, 2.5f },
        {  6.0f, -16.0f,  0.0f, 56.0f, 2.5f },
    };
    static const Sp springs_defs[] = {             // The Scalded: those the springs claimed
        { -8.0f,  -2.0f,  0.5f, 56.0f, 2.6f },
        {  8.0f,  -3.0f, -0.5f, 56.0f, 2.6f },
        { -13.0f,  7.0f,  1.3f, 50.0f, 2.8f },
        { 13.0f,   6.0f, -1.3f, 50.0f, 2.8f },
        {  0.0f, -12.0f,  3.14f, 70.0f, 2.4f },
        { -7.0f, -18.0f,  3.0f, 54.0f, 2.5f },
        {  7.0f, -18.0f, -3.0f, 54.0f, 2.5f },
    };
    static const Sp pforest_defs[] = {             // The Calcified: those the forest turned to stone
        {  0.0f,  -3.0f,  3.14f, 60.0f, 2.7f },
        { -9.0f,  -6.0f,  0.6f, 54.0f, 2.6f },
        {  9.0f,  -6.0f, -0.6f, 54.0f, 2.6f },
        { -14.0f,  4.0f,  1.2f, 50.0f, 2.5f },
        { 14.0f,   4.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -14.0f,  3.14f, 64.0f, 2.8f },
        { -8.0f, -19.0f,  2.9f, 52.0f, 2.5f },
        {  8.0f, -19.0f, -2.9f, 52.0f, 2.5f },
    };
    static const Sp hamlet_defs[] = {              // The Afflicted: the plague-dead of the village
        {  0.0f,  -4.0f,  3.14f, 58.0f, 2.6f },
        { -10.0f, -7.0f,  0.7f, 52.0f, 2.5f },
        {  10.0f, -7.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  3.0f,  1.1f, 50.0f, 2.5f },
        { 13.0f,   3.0f, -1.1f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 62.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp henge_defs[] = {               // The Devout: the cultists bound to the stones
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -8.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -8.0f, -0.7f, 52.0f, 2.5f },
        { -12.0f, -1.0f,  1.2f, 50.0f, 2.5f },
        { 12.0f,  -1.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f, -18.0f,  2.9f, 50.0f, 2.4f },
        {  6.0f, -18.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp bog_defs[] = {                 // The Mired: the drowned dead of the swamp
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -8.0f,  -8.0f,  0.7f, 52.0f, 2.5f },
        {  8.0f,  -8.0f, -0.7f, 52.0f, 2.5f },
        { -12.0f,  2.0f,  1.2f, 50.0f, 2.5f },
        { 12.0f,   2.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -14.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp lumber_defs[] = {              // The Splintered: the sawmill's maimed dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  1.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   1.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -14.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp gaol_defs[] = {                // The Condemned: the prisoners of the gaol
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -8.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  8.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -11.0f, -2.0f,  1.2f, 50.0f, 2.5f },
        { 11.0f,  -2.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f, -18.0f,  2.9f, 50.0f, 2.4f },
        {  6.0f, -18.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp tar_defs[] = {                 // The Engulfed: the tar-drowned dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -12.0f, -1.0f,  1.2f, 50.0f, 2.5f },
        { 12.0f,  -1.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -14.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp vine_defs[] = {                // The Trodden: the vineyard's crushed dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp floe_defs[] = {                // The Frostbitten: the ice-locked dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -14.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp quarry_defs[] = {              // The Hewn: the quarry's crushed laborers
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -14.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp dock_defs[] = {                // The Becalmed: the skydock's lost crew
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp glass_defs[] = {               // The Overgrown: the conservatory's choked dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp apiary_defs[] = {              // The Swarmed: the apiary's stung dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp kiln_defs[] = {                // The Fired: the kiln yard's scorched dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp reef_defs[] = {                // The Fathomless: the reef's drowned dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp foundry_defs[] = {             // The Smelted: the foundry's molten dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp nave_defs[] = {                // The Penitent: the cathedral's faithful dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -9.0f,  0.5f, 52.0f, 2.5f },
        {  5.0f,  -9.0f, -0.5f, 52.0f, 2.5f },
        { -5.0f,  -2.0f,  1.0f, 50.0f, 2.5f },
        {  5.0f,  -2.0f, -1.0f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -5.0f, -17.0f,  2.9f, 50.0f, 2.4f },
        {  5.0f, -17.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp watermill_defs[] = {           // The Sodden: the watermill's drowned dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp salt_defs[] = {                // The Encrusted: the salt-pans' brined dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp stilt_defs[] = {               // The Brackish: the lagoon village's drowned
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp hive_defs[] = {                // The Hivebound: the hive's chitinous dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp brew_defs[] = {                // The Soused: the brewery's drowned-in-drink dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp bamboo_defs[] = {              // The Severed: the grove's blade-cut dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp collier_defs[] = {             // The Charred: the charcoal camp's burnt dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp plaza_defs[] = {               // The Bygone: the plaza's vanished townsfolk
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp pumpkin_defs[] = {             // The Harvested: the patch's reaped dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp bell_defs[] = {                // The Tolled: the bell-yard's deafened dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp aviary_defs[] = {              // The Plumed: the aviary's caged dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp web_defs[] = {                 // The Ensnared: the lair's silk-bound dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp lotus_defs[] = {               // The Stilled: the pond's drowned dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -9.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  9.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -13.0f,  0.0f,  1.2f, 50.0f, 2.5f },
        { 13.0f,   0.0f, -1.2f, 50.0f, 2.5f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -19.0f,  2.9f, 50.0f, 2.4f },
        {  7.0f, -19.0f, -2.9f, 50.0f, 2.4f },
    };
    static const Sp archtree_defs[] = {            // The Grafted: husks rooted to the sacred tree
        {  0.0f,  -7.0f,  3.14f, 58.0f, 2.6f },
        { -10.0f, -10.0f,  0.8f, 52.0f, 2.5f },
        { 10.0f, -10.0f, -0.8f, 52.0f, 2.5f },
        { -14.0f, -2.0f,  1.2f, 50.0f, 2.4f },
        { 14.0f,  -2.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f, -15.0f,  3.14f, 62.0f, 2.7f },
        { -8.0f, -20.0f,  2.8f, 50.0f, 2.4f },
        {  8.0f, -20.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp chess_defs[] = {               // The Sacrificed: pieces taken from the board
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -9.0f,  0.6f, 52.0f, 2.5f },
        {  6.0f,  -9.0f, -0.6f, 52.0f, 2.5f },
        { -12.0f, -4.0f,  1.1f, 50.0f, 2.4f },
        { 12.0f,  -4.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -14.0f,  3.14f, 60.0f, 2.7f },
        { -9.0f, -18.0f,  2.8f, 50.0f, 2.4f },
        {  9.0f, -18.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp maze_defs[] = {                // The Wayward: those lost in the hedges
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -8.0f,  0.6f, 52.0f, 2.5f },
        {  5.0f,  -8.0f, -0.6f, 52.0f, 2.5f },
        { -8.0f,  -3.0f,  1.1f, 50.0f, 2.4f },
        {  8.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -11.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f, -12.0f,  2.8f, 50.0f, 2.4f },
        {  6.0f, -12.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp falls_defs[] = {               // The Drenched: the plunge-pool's dead
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -7.0f,  0.6f, 52.0f, 2.5f },
        {  5.0f,  -7.0f, -0.6f, 52.0f, 2.5f },
        { -10.0f, -3.0f,  1.1f, 50.0f, 2.4f },
        { 10.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -10.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -11.0f,  2.8f, 50.0f, 2.4f },
        {  7.0f, -11.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp crater_defs[] = {              // The Stricken: those felled by the fallen star
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -8.0f,  0.6f, 52.0f, 2.5f },
        {  6.0f,  -8.0f, -0.6f, 52.0f, 2.5f },
        { -11.0f, -3.0f,  1.1f, 50.0f, 2.4f },
        { 11.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -12.0f,  3.14f, 60.0f, 2.7f },
        { -8.0f, -13.0f,  2.8f, 50.0f, 2.4f },
        {  8.0f, -13.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp titan_defs[] = {               // The Forgotten: the fallen god's last faithful
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -9.0f,  0.6f, 52.0f, 2.5f },
        {  6.0f,  -9.0f, -0.6f, 52.0f, 2.5f },
        { -12.0f, -2.0f,  1.1f, 50.0f, 2.4f },
        { 12.0f,  -2.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -16.0f,  2.8f, 50.0f, 2.4f },
        {  7.0f, -16.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp hoodoo_defs[] = {              // The Eroded: the badlands' weathered dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -9.0f,  0.6f, 52.0f, 2.5f },
        {  6.0f,  -9.0f, -0.6f, 52.0f, 2.5f },
        { -12.0f, -3.0f,  1.1f, 50.0f, 2.4f },
        { 12.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -8.0f, -15.0f,  2.8f, 50.0f, 2.4f },
        {  8.0f, -15.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp moai_defs[] = {                // The Vigil: the island's watching dead
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -7.0f,  0.6f, 52.0f, 2.5f },
        {  5.0f,  -7.0f, -0.6f, 52.0f, 2.5f },
        { -10.0f, -2.0f,  1.1f, 50.0f, 2.4f },
        { 10.0f,  -2.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -10.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -11.0f,  2.8f, 50.0f, 2.4f },
        {  7.0f, -11.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp cavern_defs[] = {              // The Sunken: the underground pool's drowned dead
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -8.0f,  0.6f, 52.0f, 2.5f },
        {  5.0f,  -8.0f, -0.6f, 52.0f, 2.5f },
        { -11.0f, -3.0f,  1.1f, 50.0f, 2.4f },
        { 11.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -11.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -13.0f,  2.8f, 50.0f, 2.4f },
        {  7.0f, -13.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp pines_defs[] = {               // The Snowbound: the winter wood's frozen dead
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -8.0f,  0.6f, 52.0f, 2.5f },
        {  5.0f,  -8.0f, -0.6f, 52.0f, 2.5f },
        { -10.0f, -2.0f,  1.1f, 50.0f, 2.4f },
        { 10.0f,  -2.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -11.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -13.0f,  2.8f, 50.0f, 2.4f },
        {  7.0f, -13.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp galleon_defs[] = {             // The Mutinous: the ship's drowned crew
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -8.0f,  0.6f, 52.0f, 2.5f },
        {  5.0f,  -8.0f, -0.6f, 52.0f, 2.5f },
        { -8.0f,  -3.0f,  1.1f, 50.0f, 2.4f },
        {  8.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -12.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -14.0f,  2.8f, 50.0f, 2.4f },
        {  7.0f, -14.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp sunflower_defs[] = {           // The Wilted: the field's withered dead
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -8.0f,  0.6f, 52.0f, 2.5f },
        {  5.0f,  -8.0f, -0.6f, 52.0f, 2.5f },
        { -10.0f, -3.0f,  1.1f, 50.0f, 2.4f },
        { 10.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -12.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -14.0f,  2.8f, 50.0f, 2.4f },
        {  7.0f, -14.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp sphinx_defs[] = {              // The Embalmed: the necropolis' risen dead
        {  6.0f,  -3.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -3.0f,  0.6f, 52.0f, 2.5f },
        {  11.0f,  -6.0f, -0.6f, 52.0f, 2.5f },
        { -11.0f,  -6.0f,  1.1f, 50.0f, 2.4f },
        {  7.0f,  -10.0f, -1.1f, 50.0f, 2.4f },
        { -7.0f, -10.0f,  3.14f, 60.0f, 2.7f },
        {  13.0f, -2.0f,  2.8f, 50.0f, 2.4f },
        { -13.0f, -2.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp dam_defs[] = {                 // The Submerged: the reservoir's drowned dead
        {  0.0f,  -3.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -5.0f,  0.6f, 52.0f, 2.5f },
        {  6.0f,  -5.0f, -0.6f, 52.0f, 2.5f },
        { -12.0f, -1.0f,  1.1f, 50.0f, 2.4f },
        { 12.0f,  -1.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f,  -9.0f,  3.14f, 60.0f, 2.7f },
        { -8.0f,  -8.0f,  2.8f, 50.0f, 2.4f },
        {  8.0f,  -8.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp theatre_defs[] = {             // The Chorus: the theatre's risen players
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -7.0f,  0.6f, 52.0f, 2.5f },
        {  5.0f,  -7.0f, -0.6f, 52.0f, 2.5f },
        { -8.0f,  -3.0f,  1.1f, 50.0f, 2.4f },
        {  8.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f,  -9.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f, -10.0f,  2.8f, 50.0f, 2.4f },
        {  6.0f, -10.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp wheel_defs[] = {               // The Suspended: the fairground's fallen riders
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -7.0f,  0.6f, 52.0f, 2.5f },
        {  6.0f,  -7.0f, -0.6f, 52.0f, 2.5f },
        { -11.0f, -3.0f,  1.1f, 50.0f, 2.4f },
        { 11.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -11.0f,  3.14f, 60.0f, 2.7f },
        { -8.0f, -12.0f,  2.8f, 50.0f, 2.4f },
        {  8.0f, -12.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp redwood_defs[] = {             // The Mossgrown: the ancient grove's lost dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -9.0f,  0.6f, 52.0f, 2.5f },
        {  6.0f,  -9.0f, -0.6f, 52.0f, 2.5f },
        { -12.0f, -3.0f,  1.1f, 50.0f, 2.4f },
        { 12.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -15.0f,  2.8f, 50.0f, 2.4f },
        {  7.0f, -15.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp balloon_defs[] = {             // The Untethered: the festival's lost dead
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -7.0f,  0.6f, 52.0f, 2.5f },
        {  6.0f,  -7.0f, -0.6f, 52.0f, 2.5f },
        { -11.0f, -3.0f,  1.1f, 50.0f, 2.4f },
        { 11.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -11.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -13.0f,  2.8f, 50.0f, 2.4f },
        {  7.0f, -13.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp canal_defs[] = {               // The Adrift: the canal's drowned dead
        {  0.0f,  -4.0f,  3.14f, 56.0f, 2.6f },
        { -4.0f,  -7.0f,  0.6f, 52.0f, 2.5f },
        {  4.0f,  -7.0f, -0.6f, 52.0f, 2.5f },
        { -6.0f,  -2.0f,  1.1f, 50.0f, 2.4f },
        {  6.0f,  -2.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -11.0f,  3.14f, 60.0f, 2.7f },
        { -5.0f, -13.0f,  2.8f, 50.0f, 2.4f },
        {  5.0f, -13.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp silo_defs[] = {                // The Husked: the grain-works' shambling dead
        {  0.0f,  -4.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -6.0f,  0.6f, 52.0f, 2.5f },
        {  6.0f,  -6.0f, -0.6f, 52.0f, 2.5f },
        { -12.0f, -2.0f,  1.1f, 50.0f, 2.4f },
        { 12.0f,  -2.0f, -1.1f, 50.0f, 2.4f },
        {  4.0f,  -2.0f,  3.14f, 60.0f, 2.7f },
        { -8.0f,  -9.0f,  2.8f, 50.0f, 2.4f },
        {  8.0f,  -9.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp organ_defs[] = {               // The Discordant: the organ hall's risen dead
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -7.0f,  0.6f, 52.0f, 2.5f },
        {  5.0f,  -7.0f, -0.6f, 52.0f, 2.5f },
        { -9.0f,  -3.0f,  1.1f, 50.0f, 2.4f },
        {  9.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -10.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f, -11.0f,  2.8f, 50.0f, 2.4f },
        {  6.0f, -11.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp hypo_defs[] = {                // The Engraved: the hall's risen dead
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -3.0f,  -8.0f,  0.6f, 52.0f, 2.5f },
        {  3.0f,  -8.0f, -0.6f, 52.0f, 2.5f },
        { -8.0f,  -2.0f,  1.1f, 50.0f, 2.4f },
        {  8.0f,  -2.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -10.0f,  3.14f, 60.0f, 2.7f },
        { -3.0f, -13.0f,  2.8f, 50.0f, 2.4f },
        {  3.0f, -13.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp fort_defs[] = {                // The Garrison: the fort's dead defenders
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -7.0f,  -9.0f,  0.6f, 52.0f, 2.5f },
        {  7.0f,  -9.0f, -0.6f, 52.0f, 2.5f },
        { -13.0f, -3.0f,  1.1f, 50.0f, 2.4f },
        { 13.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -9.0f, -15.0f,  2.8f, 50.0f, 2.4f },
        {  9.0f, -15.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp triumph_defs[] = {             // The Vanquished: the conquered dead
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -7.0f,  0.6f, 52.0f, 2.5f },
        {  6.0f,  -7.0f, -0.6f, 52.0f, 2.5f },
        { -11.0f, -3.0f,  1.1f, 50.0f, 2.4f },
        { 11.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -10.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f, -12.0f,  2.8f, 50.0f, 2.4f },
        {  6.0f, -12.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp orchard_defs[] = {             // The Fallow: the orchard's withered dead
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -8.0f,  0.6f, 52.0f, 2.5f },
        {  5.0f,  -8.0f, -0.6f, 52.0f, 2.5f },
        { -10.0f, -2.0f,  1.1f, 50.0f, 2.4f },
        { 10.0f,  -2.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -11.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -13.0f,  2.8f, 50.0f, 2.4f },
        {  7.0f, -13.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp loom_defs[] = {                // The Unraveled: the weaver's hall's risen dead
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -7.0f,  0.6f, 52.0f, 2.5f },
        {  5.0f,  -7.0f, -0.6f, 52.0f, 2.5f },
        { -9.0f,  -3.0f,  1.1f, 50.0f, 2.4f },
        {  9.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -10.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f, -11.0f,  2.8f, 50.0f, 2.4f },
        {  6.0f, -11.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp savanna_defs[] = {             // The Parched: the savanna's wandering dead
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -9.0f,  0.6f, 52.0f, 2.5f },
        {  6.0f,  -9.0f, -0.6f, 52.0f, 2.5f },
        { -12.0f, -3.0f,  1.1f, 50.0f, 2.4f },
        { 12.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -8.0f, -15.0f,  2.8f, 50.0f, 2.4f },
        {  8.0f, -15.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp mosque_defs[] = {              // The Prostrate: the mosque's risen faithful
        {  0.0f,  -4.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -6.0f,  0.6f, 52.0f, 2.5f },
        {  5.0f,  -6.0f, -0.6f, 52.0f, 2.5f },
        { -9.0f,  -2.0f,  1.1f, 50.0f, 2.4f },
        {  9.0f,  -2.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f,  -9.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f, -11.0f,  2.8f, 50.0f, 2.4f },
        {  6.0f, -11.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp totem_defs[] = {               // The Ancestral: the village's risen dead
        {  0.0f,  -5.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -8.0f,  0.6f, 52.0f, 2.5f },
        {  5.0f,  -8.0f, -0.6f, 52.0f, 2.5f },
        { -10.0f, -3.0f,  1.1f, 50.0f, 2.4f },
        { 10.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -11.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -13.0f,  2.8f, 50.0f, 2.4f },
        {  7.0f, -13.0f, -2.8f, 50.0f, 2.4f },
    };
    static const Sp oasis_defs[] = {               // The Thirsting: parched wanderers risen at the spring
        {  0.0f,  -4.0f,  3.14f, 54.0f, 2.5f },
        { -6.0f,  -6.0f,  0.7f, 50.0f, 2.4f },
        {  6.0f,  -6.0f, -0.7f, 50.0f, 2.4f },
        { -11.0f, -2.0f,  1.2f, 48.0f, 2.3f },
        { 11.0f,  -2.0f, -1.2f, 48.0f, 2.3f },
        {  0.0f, -12.0f,  3.14f, 58.0f, 2.6f },
        { -8.0f, -12.0f,  2.6f, 50.0f, 2.4f },
        {  8.0f, -12.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp pagoda_defs[] = {              // The Penitent: temple shades risen in the garden
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  6.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f, -4.0f,  1.1f, 50.0f, 2.4f },
        { 10.0f,  -4.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -13.0f,  3.14f, 60.0f, 2.7f },
        { -8.0f, -13.0f,  2.7f, 50.0f, 2.4f },
        {  8.0f, -13.0f, -2.7f, 50.0f, 2.4f },
    };
    static const Sp stepwell_defs[] = {            // The Drowned: shades risen from the flooded baori
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -9.0f,  0.7f, 52.0f, 2.5f },
        {  6.0f,  -9.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f, -3.0f,  1.1f, 50.0f, 2.4f },
        { 10.0f,  -3.0f, -1.1f, 50.0f, 2.4f },
        {  0.0f, -12.0f,  3.14f, 60.0f, 2.7f },
        { -9.0f, -12.0f,  2.7f, 50.0f, 2.4f },
        {  9.0f, -12.0f, -2.7f, 50.0f, 2.4f },
    };
    static const Sp jantar_defs[] = {              // The Timeless: shades drifting among the instruments
        {  0.0f,  -2.0f,  3.14f, 54.0f, 2.5f },
        { -6.0f,  -5.0f,  0.7f, 52.0f, 2.5f },
        {  6.0f,  -5.0f, -0.7f, 52.0f, 2.5f },
        { -11.0f,  1.0f,  1.2f, 50.0f, 2.4f },
        { 11.0f,   1.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -9.0f,  3.14f, 58.0f, 2.6f },
        { -8.0f, -10.0f,  2.6f, 50.0f, 2.4f },
        {  8.0f, -10.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp terracotta_defs[] = {          // The Buried: clay ranks risen from the pit
        {  0.0f,   0.0f,  3.14f, 54.0f, 2.5f },
        { -6.0f,  -3.0f,  0.7f, 52.0f, 2.5f },
        {  6.0f,  -3.0f, -0.7f, 52.0f, 2.5f },
        { -11.0f,  1.0f,  1.2f, 50.0f, 2.4f },
        { 11.0f,   1.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -6.0f,  3.14f, 58.0f, 2.6f },
        { -7.0f,  -7.0f,  2.6f, 50.0f, 2.4f },
        {  7.0f,  -7.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp ballcourt_defs[] = {           // The Sacrificed: shades risen in the ball court alley
        {  0.0f,  -2.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -5.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -5.0f, -0.7f, 52.0f, 2.5f },
        { -7.0f,   2.0f,  1.2f, 50.0f, 2.4f },
        {  7.0f,   2.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -9.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f, -10.0f,  2.6f, 50.0f, 2.4f },
        {  6.0f, -10.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp petra_defs[] = {               // The Entombed: shades risen in the Treasury forecourt
        {  0.0f,  -3.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -6.0f,  0.7f, 52.0f, 2.5f },
        {  6.0f,  -6.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f, -1.0f,  1.2f, 50.0f, 2.4f },
        { 10.0f,  -1.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f, -10.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f, -11.0f,  2.6f, 50.0f, 2.4f },
        {  7.0f, -11.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp bazaar_defs[] = {              // The Covetous: shades risen among the market stalls
        {  0.0f,  -4.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -7.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -7.0f, -0.7f, 52.0f, 2.5f },
        { -6.0f,   8.0f,  1.2f, 50.0f, 2.4f },
        {  6.0f,   8.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f, -10.0f,  3.14f, 60.0f, 2.7f },
        { -5.0f, -11.0f,  2.6f, 50.0f, 2.4f },
        {  5.0f, -11.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp siege_defs[] = {               // The Routed: a broken army among the war machines
        {  0.0f,  -2.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -5.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -5.0f, -0.7f, 52.0f, 2.5f },
        { -11.0f,  2.0f,  1.2f, 50.0f, 2.4f },
        { 11.0f,   2.0f, -1.2f, 50.0f, 2.4f },
        {  3.0f,  -9.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f, -10.0f,  2.6f, 50.0f, 2.4f },
        {  6.0f, -11.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp whaling_defs[] = {             // The Flensed: shades risen on the bloody wharf
        {  0.0f,  -2.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -5.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -5.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f,  2.0f,  1.2f, 50.0f, 2.4f },
        { 10.0f,   2.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -9.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f, -10.0f,  2.6f, 50.0f, 2.4f },
        {  6.0f, -10.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp cactus_defs[] = {              // The Withered: sun-dried husks among the cacti
        {  0.0f,  -2.0f,  3.14f, 54.0f, 2.5f },
        { -6.0f,  -5.0f,  0.7f, 52.0f, 2.5f },
        {  6.0f,  -5.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f,  1.0f,  1.2f, 50.0f, 2.4f },
        { 10.0f,   1.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -9.0f,  3.14f, 58.0f, 2.6f },
        { -7.0f, -10.0f,  2.6f, 50.0f, 2.4f },
        {  7.0f, -10.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp zimbabwe_defs[] = {            // The Dethroned: shades risen in the abandoned royal court
        {  0.0f,  -1.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -3.0f,  0.7f, 52.0f, 2.5f },
        {  6.0f,  -3.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f,  2.0f,  1.2f, 50.0f, 2.4f },
        { 10.0f,   2.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -6.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f,  -5.0f,  2.6f, 50.0f, 2.4f },
        {  7.0f,  -5.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp basil_defs[] = {               // The Devout: shades risen on the cathedral square
        {  0.0f,   6.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,   3.0f,  0.7f, 52.0f, 2.5f },
        {  6.0f,   3.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f,  7.0f,  1.2f, 50.0f, 2.4f },
        { 10.0f,   7.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,   0.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f,   1.0f,  2.6f, 50.0f, 2.4f },
        {  6.0f,   1.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp print_defs[] = {               // The Redacted: shades risen among the presses
        {  0.0f,  -4.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -2.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -2.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f,  3.0f,  1.2f, 50.0f, 2.4f },
        { 10.0f,   3.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f, -11.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f, -11.0f,  2.6f, 50.0f, 2.4f },
        {  6.0f, -11.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp ger_defs[] = {                 // The Windswept: shades risen across the steppe camp
        {  0.0f,  -1.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -4.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -4.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f,  2.0f,  1.2f, 50.0f, 2.4f },
        { 10.0f,   2.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -8.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f,  -7.0f,  2.6f, 50.0f, 2.4f },
        {  7.0f,  -7.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp alchemy_defs[] = {             // The Transmuted: shades risen amid the apparatus
        {  0.0f,  -1.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -4.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -4.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f,  3.0f,  1.2f, 50.0f, 2.4f },
        { 10.0f,   3.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -8.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f,  -8.0f,  2.6f, 50.0f, 2.4f },
        {  6.0f,  -8.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp baths_defs[] = {               // The Languid: shades risen in the steaming bath
        {  0.0f,  -2.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -5.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -5.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f,  2.0f,  1.2f, 50.0f, 2.4f },
        { 10.0f,   2.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -9.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f,  -8.0f,  2.6f, 50.0f, 2.4f },
        {  7.0f,  -8.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp float_defs[] = {               // The Adrift: shades risen on the market pier
        {  0.0f,  -2.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -5.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -5.0f, -0.7f, 52.0f, 2.5f },
        { -9.0f,   2.0f,  1.2f, 50.0f, 2.4f },
        {  9.0f,   2.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -9.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f,  -9.0f,  2.6f, 50.0f, 2.4f },
        {  6.0f,  -9.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp ishtar_defs[] = {              // The Exiled: shades risen on the processional way
        {  0.0f,  -2.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -5.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -5.0f, -0.7f, 52.0f, 2.5f },
        { -8.0f,   2.0f,  1.2f, 50.0f, 2.4f },
        {  8.0f,   2.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -9.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f,  -8.0f,  2.6f, 50.0f, 2.4f },
        {  6.0f,  -8.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp tannery_defs[] = {             // The Steeped: shades risen among the dye pits
        {  0.0f,   2.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -1.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -1.0f, -0.7f, 52.0f, 2.5f },
        { -9.0f,   3.0f,  1.2f, 50.0f, 2.4f },
        {  9.0f,   3.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -6.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f,  -6.0f,  2.6f, 50.0f, 2.4f },
        {  6.0f,  -6.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp museum_defs[] = {              // The Exhibited: shades risen among the displays
        {  0.0f,  -3.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,  -5.0f,  0.7f, 52.0f, 2.5f },
        {  6.0f,  -5.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f,  3.0f,  1.2f, 50.0f, 2.4f },
        { 10.0f,   3.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f, -10.0f,  3.14f, 60.0f, 2.7f },
        { -8.0f,  -9.0f,  2.6f, 50.0f, 2.4f },
        {  8.0f,  -9.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp gompa_defs[] = {               // The Cloistered: shades risen in the monastery court
        {  0.0f,  -2.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -5.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -5.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f,  3.0f,  1.2f, 50.0f, 2.4f },
        { 10.0f,   3.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -9.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f,  -8.0f,  2.6f, 50.0f, 2.4f },
        {  7.0f,  -8.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp buddha_defs[] = {              // The Awakened: shades risen at the Buddha's feet
        {  0.0f,  -1.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -4.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -4.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f,  3.0f,  1.2f, 50.0f, 2.4f },
        { 10.0f,   3.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -7.0f,  3.14f, 60.0f, 2.7f },
        { -7.0f,  -6.0f,  2.6f, 50.0f, 2.4f },
        {  7.0f,  -6.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp angkor_defs[] = {              // The Devoured: shades risen among the prasats
        {  0.0f,  -6.0f,  3.14f, 56.0f, 2.6f },
        { -4.0f,  -4.0f,  0.7f, 52.0f, 2.5f },
        {  4.0f,  -4.0f, -0.7f, 52.0f, 2.5f },
        { -5.0f,   3.0f,  1.2f, 50.0f, 2.4f },
        {  5.0f,   3.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,   0.0f,  3.14f, 60.0f, 2.7f },
        { -5.0f,  -8.0f,  2.6f, 50.0f, 2.4f },
        {  5.0f,  -8.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp badgir_defs[] = {              // The Sun-Struck: shades risen in the baking alleys
        {  0.0f,  -2.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -5.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -5.0f, -0.7f, 52.0f, 2.5f },
        { -7.0f,   3.0f,  1.2f, 50.0f, 2.4f },
        {  7.0f,   3.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -8.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f,  -7.0f,  2.6f, 50.0f, 2.4f },
        {  6.0f,  -7.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp topiary_defs[] = {             // The Overgrown: shades risen among the clipped hedges
        {  0.0f,  -2.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -5.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -5.0f, -0.7f, 52.0f, 2.5f },
        { -12.0f,  3.0f,  1.2f, 50.0f, 2.4f },
        { 12.0f,   3.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -9.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f,  -9.0f,  2.6f, 50.0f, 2.4f },
        {  6.0f,  -9.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp glassworks_defs[] = {          // The Vitrified: shades risen among the furnaces
        {  0.0f,  -2.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,  -5.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,  -5.0f, -0.7f, 52.0f, 2.5f },
        { -10.0f,  3.0f,  1.2f, 50.0f, 2.4f },
        { 10.0f,   3.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  -8.0f,  3.14f, 60.0f, 2.7f },
        { -6.0f,  -7.0f,  2.6f, 50.0f, 2.4f },
        {  6.0f,  -7.0f, -2.6f, 50.0f, 2.4f },
    };
    static const Sp shipyard_defs[] = {            // The Unlaunched: shipwrights who never floated their hull
        {  0.0f,   6.0f,  3.14f, 56.0f, 2.6f },
        { -6.0f,   4.0f,  0.8f, 52.0f, 2.5f },
        {  6.0f,   4.0f, -0.8f, 52.0f, 2.5f },
        { -9.0f,  -2.0f,  1.2f, 50.0f, 2.4f },
        {  9.0f,  -2.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,   9.0f,  3.14f, 60.0f, 2.7f },
        { -5.0f,  -8.0f,  2.4f, 50.0f, 2.4f },
        {  5.0f,  -8.0f, -2.4f, 50.0f, 2.4f },
    };
    static const Sp gothic_defs[] = {              // The Excommunicate: shades cast out onto the cathedral close
        {  0.0f,   6.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,   5.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,   5.0f, -0.7f, 52.0f, 2.5f },
        { -8.0f,   0.0f,  1.3f, 50.0f, 2.4f },
        {  8.0f,   0.0f, -1.3f, 50.0f, 2.4f },
        {  0.0f,   9.0f,  3.14f, 60.0f, 2.7f },
        { -4.0f,  -5.0f,  2.5f, 50.0f, 2.4f },
        {  4.0f,  -5.0f, -2.5f, 50.0f, 2.4f },
    };
    static const Sp pueblo_defs[] = {              // The Vanished: the Ancestral dead astir on the cliff ledge
        {  0.0f,   7.0f,  3.14f, 56.0f, 2.6f },
        { -5.0f,   6.0f,  0.7f, 52.0f, 2.5f },
        {  5.0f,   6.0f, -0.7f, 52.0f, 2.5f },
        { -9.0f,   1.0f,  1.2f, 50.0f, 2.4f },
        {  9.0f,   1.0f, -1.2f, 50.0f, 2.4f },
        {  0.0f,  10.0f,  3.14f, 60.0f, 2.7f },
        { -4.0f,  -3.0f,  2.5f, 50.0f, 2.4f },
        {  4.0f,  -3.0f, -2.5f, 50.0f, 2.4f },
    };
    const Sp* defs = grave_defs;
    int n = (int)(sizeof(grave_defs) / sizeof(grave_defs[0]));
    if (g_level == LEVEL_FUNGAL) { defs = fungal_defs; n = (int)(sizeof(fungal_defs) / sizeof(fungal_defs[0])); }
    else if (g_level == LEVEL_DESERT) { defs = desert_defs; n = (int)(sizeof(desert_defs) / sizeof(desert_defs[0])); }
    else if (g_level == LEVEL_WRECK) { defs = wreck_defs; n = (int)(sizeof(wreck_defs) / sizeof(wreck_defs[0])); }
    else if (g_level == LEVEL_SANCTUM) { defs = sanctum_defs; n = (int)(sizeof(sanctum_defs) / sizeof(sanctum_defs[0])); }
    else if (g_level == LEVEL_CLOCK) { defs = clock_defs; n = (int)(sizeof(clock_defs) / sizeof(clock_defs[0])); }
    else if (g_level == LEVEL_SHRINE) { defs = shrine_defs; n = (int)(sizeof(shrine_defs) / sizeof(shrine_defs[0])); }
    else if (g_level == LEVEL_BONES) { defs = bones_defs; n = (int)(sizeof(bones_defs) / sizeof(bones_defs[0])); }
    else if (g_level == LEVEL_GEODE) { defs = geode_defs; n = (int)(sizeof(geode_defs) / sizeof(geode_defs[0])); }
    else if (g_level == LEVEL_TEMPLE) { defs = temple_defs; n = (int)(sizeof(temple_defs) / sizeof(temple_defs[0])); }
    else if (g_level == LEVEL_MINE) { defs = mine_defs; n = (int)(sizeof(mine_defs) / sizeof(mine_defs[0])); }
    else if (g_level == LEVEL_OBS) { defs = obs_defs; n = (int)(sizeof(obs_defs) / sizeof(obs_defs[0])); }
    else if (g_level == LEVEL_LIB) { defs = lib_defs; n = (int)(sizeof(lib_defs) / sizeof(lib_defs[0])); }
    else if (g_level == LEVEL_CAMP) { defs = camp_defs; n = (int)(sizeof(camp_defs) / sizeof(camp_defs[0])); }
    else if (g_level == LEVEL_AQUA) { defs = aqua_defs; n = (int)(sizeof(aqua_defs) / sizeof(aqua_defs[0])); }
    else if (g_level == LEVEL_COURT) { defs = court_defs; n = (int)(sizeof(court_defs) / sizeof(court_defs[0])); }
    else if (g_level == LEVEL_GARDEN) { defs = garden_defs; n = (int)(sizeof(garden_defs) / sizeof(garden_defs[0])); }
    else if (g_level == LEVEL_BRIDGE) { defs = bridge_defs; n = (int)(sizeof(bridge_defs) / sizeof(bridge_defs[0])); }
    else if (g_level == LEVEL_BEACON) { defs = beacon_defs; n = (int)(sizeof(beacon_defs) / sizeof(beacon_defs[0])); }
    else if (g_level == LEVEL_MILL) { defs = mill_defs; n = (int)(sizeof(mill_defs) / sizeof(mill_defs[0])); }
    else if (g_level == LEVEL_OSSUARY) { defs = ossuary_defs; n = (int)(sizeof(ossuary_defs) / sizeof(ossuary_defs[0])); }
    else if (g_level == LEVEL_FAIR) { defs = fair_defs; n = (int)(sizeof(fair_defs) / sizeof(fair_defs[0])); }
    else if (g_level == LEVEL_THRONE) { defs = throne_defs; n = (int)(sizeof(throne_defs) / sizeof(throne_defs[0])); }
    else if (g_level == LEVEL_SPRINGS) { defs = springs_defs; n = (int)(sizeof(springs_defs) / sizeof(springs_defs[0])); }
    else if (g_level == LEVEL_PFOREST) { defs = pforest_defs; n = (int)(sizeof(pforest_defs) / sizeof(pforest_defs[0])); }
    else if (g_level == LEVEL_HAMLET) { defs = hamlet_defs; n = (int)(sizeof(hamlet_defs) / sizeof(hamlet_defs[0])); }
    else if (g_level == LEVEL_HENGE) { defs = henge_defs; n = (int)(sizeof(henge_defs) / sizeof(henge_defs[0])); }
    else if (g_level == LEVEL_BOG) { defs = bog_defs; n = (int)(sizeof(bog_defs) / sizeof(bog_defs[0])); }
    else if (g_level == LEVEL_SAWMILL) { defs = lumber_defs; n = (int)(sizeof(lumber_defs) / sizeof(lumber_defs[0])); }
    else if (g_level == LEVEL_GAOL) { defs = gaol_defs; n = (int)(sizeof(gaol_defs) / sizeof(gaol_defs[0])); }
    else if (g_level == LEVEL_TAR) { defs = tar_defs; n = (int)(sizeof(tar_defs) / sizeof(tar_defs[0])); }
    else if (g_level == LEVEL_VINE) { defs = vine_defs; n = (int)(sizeof(vine_defs) / sizeof(vine_defs[0])); }
    else if (g_level == LEVEL_FLOE) { defs = floe_defs; n = (int)(sizeof(floe_defs) / sizeof(floe_defs[0])); }
    else if (g_level == LEVEL_QUARRY) { defs = quarry_defs; n = (int)(sizeof(quarry_defs) / sizeof(quarry_defs[0])); }
    else if (g_level == LEVEL_DOCK) { defs = dock_defs; n = (int)(sizeof(dock_defs) / sizeof(dock_defs[0])); }
    else if (g_level == LEVEL_GLASS) { defs = glass_defs; n = (int)(sizeof(glass_defs) / sizeof(glass_defs[0])); }
    else if (g_level == LEVEL_APIARY) { defs = apiary_defs; n = (int)(sizeof(apiary_defs) / sizeof(apiary_defs[0])); }
    else if (g_level == LEVEL_KILN) { defs = kiln_defs; n = (int)(sizeof(kiln_defs) / sizeof(kiln_defs[0])); }
    else if (g_level == LEVEL_REEF) { defs = reef_defs; n = (int)(sizeof(reef_defs) / sizeof(reef_defs[0])); }
    else if (g_level == LEVEL_FOUNDRY) { defs = foundry_defs; n = (int)(sizeof(foundry_defs) / sizeof(foundry_defs[0])); }
    else if (g_level == LEVEL_NAVE) { defs = nave_defs; n = (int)(sizeof(nave_defs) / sizeof(nave_defs[0])); }
    else if (g_level == LEVEL_WATERMILL) { defs = watermill_defs; n = (int)(sizeof(watermill_defs) / sizeof(watermill_defs[0])); }
    else if (g_level == LEVEL_SALT) { defs = salt_defs; n = (int)(sizeof(salt_defs) / sizeof(salt_defs[0])); }
    else if (g_level == LEVEL_STILT) { defs = stilt_defs; n = (int)(sizeof(stilt_defs) / sizeof(stilt_defs[0])); }
    else if (g_level == LEVEL_HIVE) { defs = hive_defs; n = (int)(sizeof(hive_defs) / sizeof(hive_defs[0])); }
    else if (g_level == LEVEL_BREW) { defs = brew_defs; n = (int)(sizeof(brew_defs) / sizeof(brew_defs[0])); }
    else if (g_level == LEVEL_BAMBOO) { defs = bamboo_defs; n = (int)(sizeof(bamboo_defs) / sizeof(bamboo_defs[0])); }
    else if (g_level == LEVEL_COLLIER) { defs = collier_defs; n = (int)(sizeof(collier_defs) / sizeof(collier_defs[0])); }
    else if (g_level == LEVEL_PLAZA) { defs = plaza_defs; n = (int)(sizeof(plaza_defs) / sizeof(plaza_defs[0])); }
    else if (g_level == LEVEL_PUMPKIN) { defs = pumpkin_defs; n = (int)(sizeof(pumpkin_defs) / sizeof(pumpkin_defs[0])); }
    else if (g_level == LEVEL_BELL) { defs = bell_defs; n = (int)(sizeof(bell_defs) / sizeof(bell_defs[0])); }
    else if (g_level == LEVEL_AVIARY) { defs = aviary_defs; n = (int)(sizeof(aviary_defs) / sizeof(aviary_defs[0])); }
    else if (g_level == LEVEL_WEB) { defs = web_defs; n = (int)(sizeof(web_defs) / sizeof(web_defs[0])); }
    else if (g_level == LEVEL_LOTUS) { defs = lotus_defs; n = (int)(sizeof(lotus_defs) / sizeof(lotus_defs[0])); }
    else if (g_level == LEVEL_ARCHTREE) { defs = archtree_defs; n = (int)(sizeof(archtree_defs) / sizeof(archtree_defs[0])); }
    else if (g_level == LEVEL_CHESS) { defs = chess_defs; n = (int)(sizeof(chess_defs) / sizeof(chess_defs[0])); }
    else if (g_level == LEVEL_MAZE) { defs = maze_defs; n = (int)(sizeof(maze_defs) / sizeof(maze_defs[0])); }
    else if (g_level == LEVEL_FALLS) { defs = falls_defs; n = (int)(sizeof(falls_defs) / sizeof(falls_defs[0])); }
    else if (g_level == LEVEL_CRATER) { defs = crater_defs; n = (int)(sizeof(crater_defs) / sizeof(crater_defs[0])); }
    else if (g_level == LEVEL_TITAN) { defs = titan_defs; n = (int)(sizeof(titan_defs) / sizeof(titan_defs[0])); }
    else if (g_level == LEVEL_HOODOO) { defs = hoodoo_defs; n = (int)(sizeof(hoodoo_defs) / sizeof(hoodoo_defs[0])); }
    else if (g_level == LEVEL_MOAI) { defs = moai_defs; n = (int)(sizeof(moai_defs) / sizeof(moai_defs[0])); }
    else if (g_level == LEVEL_CAVERN) { defs = cavern_defs; n = (int)(sizeof(cavern_defs) / sizeof(cavern_defs[0])); }
    else if (g_level == LEVEL_PINES) { defs = pines_defs; n = (int)(sizeof(pines_defs) / sizeof(pines_defs[0])); }
    else if (g_level == LEVEL_GALLEON) { defs = galleon_defs; n = (int)(sizeof(galleon_defs) / sizeof(galleon_defs[0])); }
    else if (g_level == LEVEL_SUNFLOWER) { defs = sunflower_defs; n = (int)(sizeof(sunflower_defs) / sizeof(sunflower_defs[0])); }
    else if (g_level == LEVEL_SPHINX) { defs = sphinx_defs; n = (int)(sizeof(sphinx_defs) / sizeof(sphinx_defs[0])); }
    else if (g_level == LEVEL_DAM) { defs = dam_defs; n = (int)(sizeof(dam_defs) / sizeof(dam_defs[0])); }
    else if (g_level == LEVEL_THEATRE) { defs = theatre_defs; n = (int)(sizeof(theatre_defs) / sizeof(theatre_defs[0])); }
    else if (g_level == LEVEL_WHEEL) { defs = wheel_defs; n = (int)(sizeof(wheel_defs) / sizeof(wheel_defs[0])); }
    else if (g_level == LEVEL_REDWOOD) { defs = redwood_defs; n = (int)(sizeof(redwood_defs) / sizeof(redwood_defs[0])); }
    else if (g_level == LEVEL_BALLOON) { defs = balloon_defs; n = (int)(sizeof(balloon_defs) / sizeof(balloon_defs[0])); }
    else if (g_level == LEVEL_CANAL) { defs = canal_defs; n = (int)(sizeof(canal_defs) / sizeof(canal_defs[0])); }
    else if (g_level == LEVEL_SILO) { defs = silo_defs; n = (int)(sizeof(silo_defs) / sizeof(silo_defs[0])); }
    else if (g_level == LEVEL_ORGAN) { defs = organ_defs; n = (int)(sizeof(organ_defs) / sizeof(organ_defs[0])); }
    else if (g_level == LEVEL_HYPO) { defs = hypo_defs; n = (int)(sizeof(hypo_defs) / sizeof(hypo_defs[0])); }
    else if (g_level == LEVEL_FORT) { defs = fort_defs; n = (int)(sizeof(fort_defs) / sizeof(fort_defs[0])); }
    else if (g_level == LEVEL_TRIUMPH) { defs = triumph_defs; n = (int)(sizeof(triumph_defs) / sizeof(triumph_defs[0])); }
    else if (g_level == LEVEL_ORCHARD) { defs = orchard_defs; n = (int)(sizeof(orchard_defs) / sizeof(orchard_defs[0])); }
    else if (g_level == LEVEL_LOOM) { defs = loom_defs; n = (int)(sizeof(loom_defs) / sizeof(loom_defs[0])); }
    else if (g_level == LEVEL_SAVANNA) { defs = savanna_defs; n = (int)(sizeof(savanna_defs) / sizeof(savanna_defs[0])); }
    else if (g_level == LEVEL_MOSQUE) { defs = mosque_defs; n = (int)(sizeof(mosque_defs) / sizeof(mosque_defs[0])); }
    else if (g_level == LEVEL_TOTEM) { defs = totem_defs; n = (int)(sizeof(totem_defs) / sizeof(totem_defs[0])); }
    else if (g_level == LEVEL_OASIS) { defs = oasis_defs; n = (int)(sizeof(oasis_defs) / sizeof(oasis_defs[0])); }
    else if (g_level == LEVEL_PAGODA) { defs = pagoda_defs; n = (int)(sizeof(pagoda_defs) / sizeof(pagoda_defs[0])); }
    else if (g_level == LEVEL_STEPWELL) { defs = stepwell_defs; n = (int)(sizeof(stepwell_defs) / sizeof(stepwell_defs[0])); }
    else if (g_level == LEVEL_JANTAR) { defs = jantar_defs; n = (int)(sizeof(jantar_defs) / sizeof(jantar_defs[0])); }
    else if (g_level == LEVEL_TERRACOTTA) { defs = terracotta_defs; n = (int)(sizeof(terracotta_defs) / sizeof(terracotta_defs[0])); }
    else if (g_level == LEVEL_BALLCOURT) { defs = ballcourt_defs; n = (int)(sizeof(ballcourt_defs) / sizeof(ballcourt_defs[0])); }
    else if (g_level == LEVEL_PETRA) { defs = petra_defs; n = (int)(sizeof(petra_defs) / sizeof(petra_defs[0])); }
    else if (g_level == LEVEL_BAZAAR) { defs = bazaar_defs; n = (int)(sizeof(bazaar_defs) / sizeof(bazaar_defs[0])); }
    else if (g_level == LEVEL_SIEGE) { defs = siege_defs; n = (int)(sizeof(siege_defs) / sizeof(siege_defs[0])); }
    else if (g_level == LEVEL_WHALING) { defs = whaling_defs; n = (int)(sizeof(whaling_defs) / sizeof(whaling_defs[0])); }
    else if (g_level == LEVEL_CACTUS) { defs = cactus_defs; n = (int)(sizeof(cactus_defs) / sizeof(cactus_defs[0])); }
    else if (g_level == LEVEL_ZIMBABWE) { defs = zimbabwe_defs; n = (int)(sizeof(zimbabwe_defs) / sizeof(zimbabwe_defs[0])); }
    else if (g_level == LEVEL_BASIL) { defs = basil_defs; n = (int)(sizeof(basil_defs) / sizeof(basil_defs[0])); }
    else if (g_level == LEVEL_PRINT) { defs = print_defs; n = (int)(sizeof(print_defs) / sizeof(print_defs[0])); }
    else if (g_level == LEVEL_GER) { defs = ger_defs; n = (int)(sizeof(ger_defs) / sizeof(ger_defs[0])); }
    else if (g_level == LEVEL_ALCHEMY) { defs = alchemy_defs; n = (int)(sizeof(alchemy_defs) / sizeof(alchemy_defs[0])); }
    else if (g_level == LEVEL_BATHS) { defs = baths_defs; n = (int)(sizeof(baths_defs) / sizeof(baths_defs[0])); }
    else if (g_level == LEVEL_FLOAT) { defs = float_defs; n = (int)(sizeof(float_defs) / sizeof(float_defs[0])); }
    else if (g_level == LEVEL_ISHTAR) { defs = ishtar_defs; n = (int)(sizeof(ishtar_defs) / sizeof(ishtar_defs[0])); }
    else if (g_level == LEVEL_TANNERY) { defs = tannery_defs; n = (int)(sizeof(tannery_defs) / sizeof(tannery_defs[0])); }
    else if (g_level == LEVEL_MUSEUM) { defs = museum_defs; n = (int)(sizeof(museum_defs) / sizeof(museum_defs[0])); }
    else if (g_level == LEVEL_GOMPA) { defs = gompa_defs; n = (int)(sizeof(gompa_defs) / sizeof(gompa_defs[0])); }
    else if (g_level == LEVEL_BUDDHA) { defs = buddha_defs; n = (int)(sizeof(buddha_defs) / sizeof(buddha_defs[0])); }
    else if (g_level == LEVEL_ANGKOR) { defs = angkor_defs; n = (int)(sizeof(angkor_defs) / sizeof(angkor_defs[0])); }
    else if (g_level == LEVEL_BADGIR) { defs = badgir_defs; n = (int)(sizeof(badgir_defs) / sizeof(badgir_defs[0])); }
    else if (g_level == LEVEL_TOPIARY) { defs = topiary_defs; n = (int)(sizeof(topiary_defs) / sizeof(topiary_defs[0])); }
    else if (g_level == LEVEL_GLASSWORKS) { defs = glassworks_defs; n = (int)(sizeof(glassworks_defs) / sizeof(glassworks_defs[0])); }
    else if (g_level == LEVEL_SHIPYARD) { defs = shipyard_defs; n = (int)(sizeof(shipyard_defs) / sizeof(shipyard_defs[0])); }
    else if (g_level == LEVEL_GOTHIC) { defs = gothic_defs; n = (int)(sizeof(gothic_defs) / sizeof(gothic_defs[0])); }
    else if (g_level == LEVEL_PUEBLO) { defs = pueblo_defs; n = (int)(sizeof(pueblo_defs) / sizeof(pueblo_defs[0])); }
    s_total_max = 0.0f;
    for (int i = 0; i < n; i++) {
        auto m = std::make_unique<Mob>();
        m->init(Vector3{ defs[i].x, 0.0f, defs[i].z }, defs[i].yaw, lit, defs[i].hp, defs[i].spd);
        s_total_max += defs[i].hp;
        s_mobs.push_back(std::move(m));
    }
    for (auto& m : s_mobs) s_targets.push_back(&m->hurtbox);

    if (!s_mobs.empty()) g_game.boss = s_mobs[0].get();   // a valid initial lock-on target
    g_events.boss_phase_changed.emit(1);                  // reset the HUD name to phase 1
    g_events.boss_health_changed.emit(s_total_max, s_total_max);
}

void reset() {
    for (auto& m : s_mobs) m->reset();
    s_fight_started = false;
    s_victory = false;
    if (!s_mobs.empty()) g_game.boss = s_mobs[0].get();
    g_events.boss_health_changed.emit(s_total_max, s_total_max);
}

int alive() {
    int n = 0;
    for (auto& m : s_mobs) if (!m->is_dead()) n++;
    return n;
}

void update(float dt, const std::vector<Hurtbox*>& player_targets,
            Vector3& player_pos, float player_radius) {
    // 1. AI / animation / per-mob hit scan against the player
    for (auto& m : s_mobs) m->update(dt, player_targets);

    // 2. first contact -> FIGHT state (lights the HUD bar; reuses the boss path).
    //    Safe even with the inert Boss object: it is never updated or drawn here.
    if (!s_fight_started) {
        for (auto& m : s_mobs)
            if (m->state != Mob::SLEEP && m->state != Mob::DEAD) { g_game.start_fight(); s_fight_started = true; break; }
    }

    // 3. push the player out of each living Hollow
    for (auto& m : s_mobs) {
        if (m->is_dead()) continue;
        Vector3 d = player_pos - m->pos; d.y = 0;
        float dist = Vector3Length(d);
        float mind = player_radius + BODY_RADIUS;
        if (dist < mind && dist > 0.0001f) {
            Vector3 push = Vector3Scale(Vector3Normalize(d), (mind - dist) * 0.5f);
            player_pos = player_pos + push;
            m->pos = m->pos - push;
            arena::resolve(player_pos, player_radius);
            arena::resolve(m->pos, BODY_RADIUS);
        }
    }

    // 4. keep the Hollows from stacking on each other
    for (size_t i = 0; i < s_mobs.size(); i++)
        for (size_t j = i + 1; j < s_mobs.size(); j++) {
            Mob* a = s_mobs[i].get(); Mob* b = s_mobs[j].get();
            if (a->is_dead() || b->is_dead()) continue;
            Vector3 d = a->pos - b->pos; d.y = 0;
            float dist = Vector3Length(d);
            float mind = BODY_RADIUS * 2.0f;
            if (dist < mind && dist > 0.0001f) {
                Vector3 push = Vector3Scale(Vector3Normalize(d), (mind - dist) * 0.5f);
                a->pos = a->pos + push; b->pos = b->pos - push;
                arena::resolve(a->pos, BODY_RADIUS); arena::resolve(b->pos, BODY_RADIUS);
            }
        }

    // 5. lock-on target = nearest living Hollow
    Mob* nearest = nullptr; float best = 1e9f;
    for (auto& m : s_mobs) {
        if (m->is_dead()) continue;
        float dd = Vector3Distance(player_pos, m->pos);
        if (dd < best) { best = dd; nearest = m.get(); }
    }
    if (nearest) g_game.boss = nearest;

    // 6. aggregate horde health -> HUD bar
    float sum = 0.0f;
    for (auto& m : s_mobs) sum += m->health.current;
    g_events.boss_health_changed.emit(sum, s_total_max);

    // 7. the last Hollow falls -> victory (reuses the bonfire flow)
    if (!s_victory && alive() == 0) {
        s_victory = true;
        g_game.boss = nullptr;
        g_game.on_boss_died();
    }
}

void draw() { for (auto& m : s_mobs) m->draw(); }

void unload() {
    for (auto& m : s_mobs) m->unload();
    s_mobs.clear();
    s_targets.clear();
}

std::vector<Hurtbox*>& targets() { return s_targets; }

}
