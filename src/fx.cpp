#include "fx.h"
#include "assets.h"
#include "mathx.h"
#include "rlgl.h"
#include <cmath>

Fx g_fx;

void Fx::init() {
    const char* names[] = { "glow", "spark", "ring", "smoke", "slash",
                            "blood_drop", "blood_splat", "ripple", "splash" };
    for (const char* n : names) tcache_[n] = assets::texture(std::string("textures/vfx/") + n + ".png");
}

Texture2D Fx::tex(const std::string& name) {
    auto it = tcache_.find(name);
    if (it != tcache_.end()) return it->second;
    return tcache_["spark"];
}

static Vector3 rand_unit() {
    Vector3 v{ rand_range(-1, 1), rand_range(-1, 1), rand_range(-1, 1) };
    float l = Vector3Length(v);
    return (l > 0.001f) ? v * (1.0f / l) : Vector3{ 0, 1, 0 };
}

void Fx::burst(Vector3 pos, const std::string& tname, Color color, int count,
               float life, float vmin, float vmax, float spread_deg,
               Vector3 dir, Vector3 grav, float smin, float smax, bool additive) {
    Texture2D t = tex(tname);
    float dl = Vector3Length(dir);
    Vector3 d = (dl > 0.001f) ? dir * (1.0f / dl) : Vector3{ 0, 1, 0 };
    float mix = fminf(1.0f, spread_deg / 180.0f);
    for (int i = 0; i < count; i++) {
        Vector3 rd = rand_unit();
        Vector3 vd = d * (1.0f - mix) + rd * mix;
        float vl = Vector3Length(vd);
        if (vl > 0.001f) vd = vd * (1.0f / vl);
        FxParticle p;
        p.pos = pos;
        p.vel = vd * rand_range(vmin, vmax);
        p.grav = grav;
        p.life = life * rand_range(0.8f, 1.1f);
        p.size = rand_range(smin, smax);
        p.color = color;
        p.tex = t;
        p.additive = additive;
        parts_.push_back(p);
    }
}

void Fx::ring(Vector3 pos, Color color, float end_scale, float dur, bool ground) {
    FxRing r;
    r.pos = pos;
    r.color = color;
    r.scale0 = end_scale * (ground ? 0.3f : 0.25f);
    r.scale1 = end_scale;
    r.dur = dur;
    r.tex = ground ? (tcache_.count("ripple") ? tcache_["ripple"] : tcache_["ring"]) : tcache_["ring"];
    r.ground = ground;
    rings_.push_back(r);
}

// ---------------------------------------------------------------- effects
void Fx::hit(Vector3 pos, float amount) {
    float s = fminf(fmaxf(amount / 26.0f, 0.5f), 1.6f);
    float t = fminf(fmaxf((amount - 6.0f) / 40.0f, 0.0f), 1.0f);
    int drops = (int)(12 + (28 - 12) * t);
    float shade = rand_range(-0.06f, 0.06f);
    Color col = { (unsigned char)(255 * (0.78f + shade)), (unsigned char)(255 * (0.04f + fmaxf(0.0f, shade * 0.2f))), 14, 255 };
    float dmin = rand_range(0.07f, 0.10f) * s, dmax = rand_range(0.16f, 0.24f) * s;
    const char* drop = (GetRandomValue(0, 99) < 25) ? "blood_splat" : "blood_drop";
    burst(pos, drop, col, drops, rand_range(0.5f, 0.7f), 2.2f, 6.5f, 85.0f,
          Vector3{ 0, 1, 0 }, Vector3{ 0, -13, 0 }, dmin, dmax, false);
    burst(pos, "blood_splat", Color{ 150, 14, 14, 240 }, (int)fmaxf(4, 6 * s),
          rand_range(0.45f, 0.6f), 0.4f, 1.8f, 140.0f, Vector3{ 0, 1, 0 }, Vector3{ 0, -3, 0 },
          0.16f * s, 0.34f * s, false);
    // a brief bright spark at the impact so the hit reads even on a dark target
    burst(pos, "spark", Color{ 255, 120, 90, 255 }, 8, 0.32f, 3.0f, 7.0f, 180.0f,
          Vector3{ 0, 1, 0 }, Vector3{ 0, -6, 0 }, 0.12f, 0.26f, true);
}
// a quick pale shockwave ring on impact (camera-facing) for a bit of "wave" punch
void Fx::impact_wave(Vector3 pos) {
    ring(pos, Color{ 255, 246, 232, 200 }, 2.0f, 0.26f, false);
    ring(pos, Color{ 255, 210, 170, 150 }, 1.1f, 0.16f, false);
}
void Fx::parry(Vector3 pos) {
    burst(pos, "spark", Color{ 255, 242, 153, 255 }, 26, 0.5f, 4.0f, 8.5f, 180.0f,
          Vector3{ 0, 1, 0 }, Vector3{ 0, -3, 0 }, 0.22f, 0.42f, true);
    ring(pos, Color{ 255, 230, 140, 230 }, 2.6f, 0.32f, false);
    ring(pos, Color{ 255, 255, 242, 178 }, 1.4f, 0.18f, false);
}
void Fx::riposte(Vector3 pos) {
    burst(pos, "spark", Color{ 255, 204, 77, 255 }, 34, 0.6f, 4.5f, 10.0f, 180.0f,
          Vector3{ 0, 1, 0 }, Vector3{ 0, -7, 0 }, 0.3f, 0.6f, true);
    burst(pos, "glow", Color{ 255, 140, 38, 255 }, 12, 0.55f, 1.0f, 3.0f, 180.0f,
          Vector3{ 0, 1, 0 }, Vector3{ 0, -3, 0 }, 0.5f, 1.0f, true);
    ring(pos, Color{ 255, 191, 77, 230 }, 3.4f, 0.4f, false);
}
void Fx::enrage(Vector3 pos) {
    burst(pos, "spark", Color{ 255, 64, 38, 255 }, 32, 0.6f, 3.0f, 7.5f, 180.0f,
          Vector3{ 0, 1, 0 }, Vector3{ 0, -3, 0 }, 0.28f, 0.55f, true);
    ring(pos, Color{ 255, 51, 31, 230 }, 3.4f, 0.45f, false);
    ring(pos, Color{ 255, 102, 51, 178 }, 2.0f, 0.3f, false);
}
void Fx::dodge(Vector3 pos) {
    ripple(Vector3{ pos.x, WATER_Y, pos.z }, 5.0f);
    burst(pos + Vector3{ 0, 0.15f, 0 }, "smoke", Color{ 179, 184, 199, 128 }, 8, 0.5f,
          1.2f, 2.6f, 60.0f, Vector3{ 0, 1, 0 }, Vector3{ 0, 1.5f, 0 }, 0.35f, 0.8f, false);
}
void Fx::heal(Vector3 pos) {
    for (int i = 0; i < 24; i++) {
        FxParticle p;
        p.pos = pos + Vector3{ rand_range(-0.45f, 0.45f), rand_range(0.2f, 1.0f), rand_range(-0.45f, 0.45f) };
        p.vel = Vector3{ 0, rand_range(0.4f, 1.1f), 0 };
        p.grav = Vector3{ 0, 1.6f, 0 };
        p.life = rand_range(0.8f, 1.3f);
        p.size = rand_range(0.12f, 0.26f);
        p.color = Color{ 128, 255, 128, 255 };
        p.tex = tex("glow");
        p.additive = true;
        parts_.push_back(p);
    }
    ring(pos + Vector3{ 0, 1.0f, 0 }, Color{ 140, 255, 140, 178 }, 1.8f, 0.4f, false);
}
void Fx::ripple(Vector3 pos, float speed) {
    float end_scale = fminf(fmaxf(1.6f + speed * 0.22f, 1.6f), 3.8f);
    ring(Vector3{ pos.x, WATER_Y, pos.z }, Color{ 190, 224, 255, 210 }, end_scale, 0.75f, true);
    if (speed > 3.0f)
        burst(pos + Vector3{ 0, 0.05f, 0 }, "splash", Color{ 200, 230, 255, 220 }, 6, 0.45f,
              0.8f, 1.8f, 55.0f, Vector3{ 0, 1, 0 }, Vector3{ 0, -5, 0 }, 0.06f, 0.13f, false);
}
void Fx::track_movement(void* id, Vector3 pos, float h_speed, float dt) {
    if (h_speed < 0.4f) return;
    float d = step_[id] + h_speed * dt;
    if (d >= STEP_DIST) {
        d = 0.0f;
        ripple(Vector3{ pos.x, WATER_Y, pos.z }, h_speed);
    }
    step_[id] = d;
}

// ---------------------------------------------------------------- sim/draw
void Fx::update(float dt) {
    for (size_t i = 0; i < parts_.size();) {
        FxParticle& p = parts_[i];
        p.age += dt;
        if (p.age >= p.life) { parts_[i] = parts_.back(); parts_.pop_back(); continue; }
        p.vel = p.vel + p.grav * dt;
        p.pos = p.pos + p.vel * dt;
        i++;
    }
    for (size_t i = 0; i < rings_.size();) {
        FxRing& r = rings_[i];
        r.age += dt;
        if (r.age >= r.dur) { rings_[i] = rings_.back(); rings_.pop_back(); continue; }
        i++;
    }
}

static void draw_ground_quad(Texture2D t, Vector3 c, float size, Color col) {
    float h = size * 0.5f;
    rlSetTexture(t.id);
    rlBegin(RL_QUADS);
    rlColor4ub(col.r, col.g, col.b, col.a);
    rlNormal3f(0, 1, 0);
    rlTexCoord2f(0, 0); rlVertex3f(c.x - h, c.y, c.z - h);
    rlTexCoord2f(0, 1); rlVertex3f(c.x - h, c.y, c.z + h);
    rlTexCoord2f(1, 1); rlVertex3f(c.x + h, c.y, c.z + h);
    rlTexCoord2f(1, 0); rlVertex3f(c.x + h, c.y, c.z - h);
    rlEnd();
    rlSetTexture(0);
}

void Fx::draw(const Camera3D& cam) {
    // alpha pass
    BeginBlendMode(BLEND_ALPHA);
    for (auto& p : parts_) {
        if (p.additive) continue;
        float a = 1.0f - p.age / p.life;
        Color c = p.color; c.a = (unsigned char)(c.a * a);
        DrawBillboard(cam, p.tex, p.pos, p.size, c);
    }
    for (auto& r : rings_) {
        float t = r.age / r.dur;
        float sc = r.scale0 + (r.scale1 - r.scale0) * t;
        Color c = r.color; c.a = (unsigned char)(c.a * (1.0f - t));
        if (r.ground) draw_ground_quad(r.tex, r.pos, sc, c);
    }
    EndBlendMode();
    // additive pass
    BeginBlendMode(BLEND_ADDITIVE);
    for (auto& p : parts_) {
        if (!p.additive) continue;
        float a = 1.0f - p.age / p.life;
        Color c = p.color; c.a = (unsigned char)(c.a * a);
        DrawBillboard(cam, p.tex, p.pos, p.size, c);
    }
    for (auto& r : rings_) {
        if (r.ground) continue;
        float t = r.age / r.dur;
        float sc = r.scale0 + (r.scale1 - r.scale0) * t;
        Color c = r.color; c.a = (unsigned char)(c.a * (1.0f - t));
        DrawBillboard(cam, r.tex, r.pos, sc, c);
    }
    EndBlendMode();
}
