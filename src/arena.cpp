#include "arena.h"
#include "assets.h"
#include "render.h"
#include "game.h"
#include "mathx.h"
#include "rlgl.h"
#include <vector>
#include <cmath>

namespace arena {

float   floor_y = 0.0f;
Vector3 boundary_center = { 0, 0, 2 };
float   boundary_radius = 24.0f;
float   water_level = 0.02f;
std::vector<Obstacle> obstacles;

static Model s_model{};
static bool s_has_model = false;
static std::vector<bool> s_skip_mesh;   // glb meshes the port replaces (built-in moon + water)
static Model s_column{};                // unit cube reused (lit) for the frozen colonnade
static bool s_has_column = false;
static Model s_moon{};
static Shader s_moon_shader{};
static bool s_has_moon = false;
static Model s_sky{};
static Shader s_sky_shader{};
static int s_loc_sky_moon = -1;
static Model s_water{};
static Shader s_water_shader{};
static int s_loc_time = -1, s_loc_view = -1, s_loc_reflect = -1, s_loc_screen = -1, s_loc_moonpos = -1;
static int s_loc_cry_n = -1, s_loc_cry_arr = -1;
static float s_time = 0.0f;
static const int MAX_CRYSTAL_LIGHTS = 32;

static const Vector3 MOON_POS = { 0, 30, -130 };   // world position of the blood moon disc

// A gem shard is an upright box (rotated/leaning) of a fixed varied size; it reads
// as a cloudy red gem, and the cluster emits a soft red light halo. Static (no
// animation) -- `shade` just gives each gem a fixed brightness for cluster variety.
struct Gem { Vector3 base; Vector3 size; float yaw, tilt, shade; };
struct Crystal { std::vector<Gem> gems; Vector3 light_pos; };
static std::vector<Crystal> s_crystals;
// One red point light per crystal cluster, fed to the water shader so the glow
// pools on the water surface near each crystal: xyz = world pos, w = glow radius.
static std::vector<Vector4> s_crystal_lights;

// ---------------------------------------------------------------- crystals
static void build_crystals() {
    s_crystals.clear();
    s_crystal_lights.clear();
    if (g_level == LEVEL_COLOSSEUM) {
        // no gem clusters -- 8 brazier lights at the four gateways drive the water glow
        const float gw[4] = { 0, 90, 180, 270 };
        for (int g = 0; g < 4; g++) {
            float a = gw[g] * DEG2RAD;
            Vector3 r{ sinf(a), 0, cosf(a) }, t{ cosf(a), 0, -sinf(a) };
            for (int s = -1; s <= 1; s += 2) {
                Vector3 p = boundary_center + Vector3Scale(r, 34.6f) + Vector3Scale(t, 5.4f * s);
                s_crystal_lights.push_back(Vector4{ p.x, 1.2f, p.z, 9.0f });
            }
        }
        return;
    }
    SetRandomSeed(13377);
    Vector3 spawn{ 0, 0, 16 }, boss{ 0, 0, -10 };
    int placed = 0, attempts = 0, want = 20;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float ang = rand_range(0, 2 * PI);
        float rad = sqrtf(rand_range(0, 1)) * (boundary_radius - 2.0f);
        Vector3 p = boundary_center + Vector3{ sinf(ang) * rad, 0, cosf(ang) * rad };
        if (Vector3Distance(p, spawn) < 4.5f) continue;
        if (Vector3Distance(p, boss) < 6.0f) continue;
        Crystal c;
        float cs = rand_range(0.6f, 2.0f);
        int gems = GetRandomValue(3, 6);
        float tallest = 1.0f;
        for (int i = 0; i < gems; i++) {
            float w = rand_range(0.22f, 0.62f) * cs;   // gem cross-section (varied size)
            float d = rand_range(0.22f, 0.62f) * cs;
            float h = rand_range(1.2f, 4.0f) * cs;      // gem length, leaning upward
            tallest = fmaxf(tallest, h);
            Vector3 b = p + Vector3{ rand_range(-0.9f, 0.9f) * cs, -0.2f, rand_range(-0.9f, 0.9f) * cs };
            Gem g;
            g.base = b;
            g.size = { w, h, d };
            g.yaw = rand_range(0, 2 * PI);
            g.tilt = rand_range(-0.22f, 0.22f);
            g.shade = rand_range(0.7f, 1.0f);
            c.gems.push_back(g);
        }
        c.light_pos = p + Vector3{ 0, tallest * 0.5f + 0.6f, 0 };
        s_crystals.push_back(c);
        if ((int)s_crystal_lights.size() < MAX_CRYSTAL_LIGHTS)            // glow radius scales with cluster size
            s_crystal_lights.push_back(Vector4{ p.x, 1.0f, p.z, 4.0f + cs * 2.4f });   // y lifts it onto geometry; water uses xz only
        placed++;
    }
}

// ---------------------------------------------------------------- load
void load(Shader lit) {
    std::string p = assets::path("arena/boss_arena.glb");
    if (FileExists(p.c_str())) {
        s_model = LoadModel(p.c_str());
        s_has_model = true;
        for (int i = 0; i < s_model.materialCount; i++) s_model.materials[i].shader = lit;
        // The glb ships its own dark "Moon" sphere (at MOON_POS) and a "WaterField"
        // plane; the port renders its own glowing moon + reflective water, so skip
        // those two meshes — the moon sphere was occluding our moon disc, and the
        // water plane pokes through our wave troughs. Detect them by bounding box.
        s_skip_mesh.assign(s_model.meshCount, false);
        for (int i = 0; i < s_model.meshCount; i++) {
            BoundingBox bb = GetMeshBoundingBox(s_model.meshes[i]);
            Vector3 c = { (bb.min.x + bb.max.x) * 0.5f,
                          (bb.min.y + bb.max.y) * 0.5f,
                          (bb.min.z + bb.max.z) * 0.5f };
            float hy = bb.max.y - bb.min.y;
            float wx = bb.max.x - bb.min.x, wz = bb.max.z - bb.min.z;
            bool is_moon  = Vector3Distance(c, MOON_POS) < 12.0f;
            bool is_water = hy < 2.0f && (wx > 40.0f || wz > 40.0f) && c.y < 6.0f;
            s_skip_mesh[i] = is_moon || is_water;
        }
    }
    // a plain lit cube reused for the frozen-cathedral colonnade (columns + broken caps),
    // so the columns pick up the cold lighting + fog like the rest of the scenery
    s_column = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    s_column.materials[0].shader = lit;
    s_has_column = true;

    // blood moon: a camera-facing quad shaded as a sphere in moon.fs (no UV-sphere
    // pole pinching), mapping the crater texture and masking to a circular disc.
    Mesh disc = GenMeshPlane(48.0f, 48.0f, 1, 1);
    s_moon = LoadModelFromMesh(disc);
    const char* moon_tex_by[4] = { "textures/moon/blood_moon.png", "textures/moon/moon_surface.png", "textures/moon/blood_moon.png", "textures/moon/blood_moon.png" };
    const char* moon_fs_by[4]  = { "shaders/moon.fs", "shaders/moon_crescent.fs", "shaders/moon.fs", "shaders/moon.fs" };
    s_moon.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = assets::texture(moon_tex_by[g_level]);
    const char* moon_fs = moon_fs_by[g_level];
    s_moon_shader = LoadShader(assets::path("shaders/moon.vs").c_str(),
                               assets::path(moon_fs).c_str());
    s_moon.materials[0].shader = s_moon_shader;
    s_has_moon = true;

    // night-sky dome: a unit sphere drawn centred on the camera (scaled large) so
    // its inside fills the background with a gradient + moon glow, in both the main
    // view and the reflection pass.
    Mesh dome = GenMeshSphere(1.0f, 16, 32);
    s_sky = LoadModelFromMesh(dome);
    const char* sky_fs_by[4] = { "shaders/sky.fs", "shaders/sky_ice.fs", "shaders/sky_forge.fs", "shaders/sky_storm.fs" };
    s_sky_shader = LoadShader(assets::path("shaders/sky.vs").c_str(),
                              assets::path(sky_fs_by[g_level]).c_str());
    s_sky.materials[0].shader = s_sky_shader;
    s_loc_sky_moon = GetShaderLocation(s_sky_shader, "uMoonDir");

    // reflective water plane (subdivided so the vertex waves have shape)
    Mesh plane = GenMeshPlane(320.0f, 320.0f, 80, 80);
    s_water = LoadModelFromMesh(plane);
    const char* water_fs_by[4] = { "shaders/water.fs", "shaders/water_ice.fs", "shaders/water_forge.fs", "shaders/water_storm.fs" };
    s_water_shader = LoadShader(assets::path("shaders/water.vs").c_str(),
                                assets::path(water_fs_by[g_level]).c_str());
    s_water_shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(s_water_shader, "matModel");
    s_loc_time = GetShaderLocation(s_water_shader, "uTime");
    s_loc_view = GetShaderLocation(s_water_shader, "uViewPos");
    s_loc_reflect = GetShaderLocation(s_water_shader, "uReflect");
    s_loc_screen = GetShaderLocation(s_water_shader, "uScreen");
    s_loc_moonpos = GetShaderLocation(s_water_shader, "uMoonPos");
    s_loc_cry_n = GetShaderLocation(s_water_shader, "uCrystalN");
    s_loc_cry_arr = GetShaderLocation(s_water_shader, "uCrystals");
    s_water.materials[0].shader = s_water_shader;

    // obstacle collision for the boss_arena rocks that intrude into the ring
    // (only the glb levels have them; the colosseum pit is clear)
    if (g_level != LEVEL_COLOSSEUM) {
        obstacles.push_back({ { -12.0f, 0, -18.0f }, 3.2f });   // Rock_E
        obstacles.push_back({ {  15.0f, 0, -22.0f }, 3.6f });   // Rock_F
    }

    build_crystals();

    // upload the static crystal lights to the water shader (positions never move)
    int cn = (int)s_crystal_lights.size();
    SetShaderValue(s_water_shader, s_loc_cry_n, &cn, SHADER_UNIFORM_INT);
    if (cn > 0)
        SetShaderValueV(s_water_shader, s_loc_cry_arr, s_crystal_lights.data(),
                        SHADER_UNIFORM_VEC4, cn);
    // same crystal lights illuminate the rocks/fighters via the lit shader
    Vector3 pcol_by[4] = { { 0.85f, 0.14f, 0.10f },    // blood: red
                           { 0.26f, 0.55f, 0.85f },    // frozen: ice-blue
                           { 0.95f, 0.42f, 0.10f },    // forge: ember-orange
                           { 0.95f, 0.50f, 0.18f } };  // colosseum: brazier fire
    g_lit.set_point_lights(s_crystal_lights.data(), cn, pcol_by[g_level]);
}

void update(float dt) { s_time += dt; }

// ---------------------------------------------------------------- draw
static void draw_crystals();
static void draw_level_props();
static void draw_colosseum();

// Background dome: filled before the scenery in every pass so the sky is never an
// empty void (which is what made the water reflect black). Drawn with culling off
// because the camera sits inside the sphere; restores culling for the scenery.
void draw_sky(Camera3D cam) {
    Vector3 md = Vector3Normalize(Vector3Subtract(MOON_POS, cam.position));
    SetShaderValue(s_sky_shader, s_loc_sky_moon, &md, SHADER_UNIFORM_VEC3);
    rlDisableBackfaceCulling();
    DrawModelEx(s_sky, cam.position, Vector3{ 0, 1, 0 }, 0.0f, Vector3{ 500, 500, 500 }, WHITE);
    rlEnableBackfaceCulling();
}

void draw_world(Camera3D cam) {
    (void)cam;
    // sky moon: full blood moon in the ruin, cold crescent in the cathedral; the forge
    // and the stormy colosseum have no moon
    if (s_has_moon && (g_level == LEVEL_BLOODMOON || g_level == LEVEL_FROZEN))
        DrawModelEx(s_moon, MOON_POS, Vector3{ 1, 0, 0 }, 90.0f, Vector3{ 1, 1, 1 }, WHITE);
    if (g_level == LEVEL_COLOSSEUM) {
        draw_colosseum();                 // its own procedural geometry (no boss_arena.glb)
    } else {
        if (s_has_model)
            for (int i = 0; i < s_model.meshCount; i++)
                if (!s_skip_mesh[i])
                    DrawMesh(s_model.meshes[i], s_model.materials[s_model.meshMaterial[i]], s_model.transform);
        if (g_level != LEVEL_BLOODMOON) draw_level_props();
    }
    draw_crystals();
}

// Cloudy red gem crystals: static (no animation) boxes of fixed varied sizes. Each
// is a fairly opaque deep-red body (reads as a cloudy gem, not clear glass) with soft
// dark-red facet lines; the cluster emits a soft red light halo. Drawn full-bright in
// rlgl immediate mode; rlPushMatrix/rlRotatef leans each box (vertices bake per-cube,
// so a whole pass batches under one blend mode).
static void place_gem(const Gem& g) {
    rlPushMatrix();
    rlTranslatef(g.base.x, g.base.y, g.base.z);
    rlRotatef(g.yaw * RAD2DEG, 0, 1, 0);
    rlRotatef(g.tilt * RAD2DEG, 1, 0, 0);
    rlTranslatef(0, g.size.y * 0.5f, 0);     // lift so the box base sits on the ground
}

static void draw_crystals() {
    if (g_level == LEVEL_COLOSSEUM) return;   // colosseum has braziers, not gem crystals
    // per-level gem tints: blood = cloudy red, frozen = ice-blue, forge = ember-orange.
    // bodyRGB at shade 1, edgeRGB, haloA(rgb), haloB(rgb), body alpha.
    int L = g_level;
    Vector3 bodyC[3] = { {165,30,27}, {120,175,225}, {210,90,25} };
    Vector3 edgeC[3] = { {95,18,16},  {170,225,250}, {255,170,60} };
    Vector3 h0C[3]   = { {200,30,24}, {110,200,255}, {255,120,35} };
    Vector3 h1C[3]   = { {165,24,18}, {80,160,235},  {220,90,25} };
    unsigned char bodyA = (L == LEVEL_FROZEN) ? 180 : 210;

    BeginBlendMode(BLEND_ALPHA);
    for (auto& c : s_crystals) {
        for (auto& g : c.gems) {
            place_gem(g);
            float s = g.shade;
            DrawCubeV(Vector3{ 0, 0, 0 }, g.size,
                      Color{ (unsigned char)(bodyC[L].x * s), (unsigned char)(bodyC[L].y * s), (unsigned char)(bodyC[L].z * s), bodyA });
            DrawCubeWiresV(Vector3{ 0, 0, 0 }, g.size,
                      Color{ (unsigned char)(edgeC[L].x * s), (unsigned char)(edgeC[L].y * s), (unsigned char)(edgeC[L].z * s), 215 });
            rlPopMatrix();
        }
    }
    EndBlendMode();

    // soft static additive light halo
    BeginBlendMode(BLEND_ADDITIVE);
    for (auto& c : s_crystals) {
        DrawSphereEx(c.light_pos, 0.42f, 8, 8, Color{ (unsigned char)h0C[L].x, (unsigned char)h0C[L].y, (unsigned char)h0C[L].z, 90 });
        DrawSphereEx(c.light_pos, 0.95f, 8, 8, Color{ (unsigned char)h1C[L].x, (unsigned char)h1C[L].y, (unsigned char)h1C[L].z, 32 });
    }
    EndBlendMode();
}

// Frozen/forge scenery: a ruined colonnade (lit cubes, so they take the level's light +
// fog) ringing the lake, plus jagged spires nearer the centre. Frozen = blue-grey stone +
// translucent ice spires; forge = dark obsidian + glowing ember spires. Layout follows
// design/frozen_cathedral_design.md.
static void draw_level_props() {
    if (!s_has_column) return;
    bool forge = (g_level == LEVEL_FORGE);
    Color stone = forge ? Color{ 34, 26, 28, 255 }     // dark obsidian
                        : Color{ 78, 96, 110, 255 };    // blue-grey ice-stone
    static const float colAng[10] = { 0, 35, 72, 112, 151, 204, 238, 276, 315, 342 };
    for (int i = 0; i < 10; i++) {
        float a = colAng[i] * DEG2RAD;
        float r = 27.5f;
        Vector3 p = boundary_center + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        float h = 7.0f + (float)((i * 7) % 9);                       // 7..15 varied
        float yaw = colAng[i];
        DrawModelEx(s_column, Vector3{ p.x, h * 0.5f, p.z }, Vector3{ 0, 1, 0 }, yaw, Vector3{ 1.5f, h, 1.5f }, stone);
        DrawModelEx(s_column, Vector3{ p.x, h + 0.35f, p.z }, Vector3{ 0, 1, 0 }, yaw + 22.0f, Vector3{ 2.1f, 0.7f, 2.1f }, stone);
    }
    // jagged spires (translucent ice-blue / glowing ember), inner ring
    Color spire = forge ? Color{ 230, 110, 35, 165 } : Color{ 150, 205, 240, 150 };
    Color spireGlow = forge ? Color{ 255, 130, 40, 95 } : Color{ 110, 200, 255, 80 };
    static const float spAng[7] = { 18, 66, 139, 188, 247, 292, 333 };
    BeginBlendMode(BLEND_ALPHA);
    for (int i = 0; i < 7; i++) {
        float a = spAng[i] * DEG2RAD;
        float r = 17.0f + (float)((i * 5) % 7);                      // 17..24
        Vector3 b = boundary_center + Vector3{ sinf(a) * r, -0.2f, cosf(a) * r };
        float h = 3.5f + (float)((i * 3) % 6);                       // 3.5..9
        float rad = 0.45f + 0.12f * (i % 3);
        DrawCylinderEx(b, b + Vector3{ 0.2f * sinf(a), h, 0.2f * cosf(a) }, rad, 0.04f, 5, spire);
    }
    EndBlendMode();
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 7; i++) {
        float a = spAng[i] * DEG2RAD;
        float r = 17.0f + (float)((i * 5) % 7);
        float h = 3.5f + (float)((i * 3) % 6);
        Vector3 tip = boundary_center + Vector3{ sinf(a) * r + 0.2f * sinf(a), h - 0.2f, cosf(a) * r + 0.2f * cosf(a) };
        DrawSphereEx(tip, 0.30f, 6, 6, spireGlow);
    }
    EndBlendMode();
}

// The Sunken Colosseum: an ENCLOSED amphitheater built procedurally (no boss_arena.glb),
// per design/sunken_colosseum_design.md -- six concentric stepped wall tiers with gateway
// gaps at the four cardinals, four arched gateways, a central dais, and flanking braziers.
// Wall pieces use the lit cube so they take the stormy light + fog.
static const float GW_ANG[4] = { 0.0f, 90.0f, 180.0f, 270.0f };

static void draw_colosseum() {
    if (!s_has_column) return;
    Vector3 ctr = boundary_center;
    Color wet{ 57, 77, 77, 255 }, moss{ 83, 104, 92, 255 }, dark{ 40, 55, 56, 255 };

    struct Ring { float ri, ro, top, gap; Color c; };
    Ring rings[6] = {
        { 24.0f, 25.5f, 0.35f, 14.0f, wet },
        { 26.0f, 28.5f, 0.95f, 18.0f, wet },
        { 29.2f, 31.7f, 1.65f, 18.0f, moss },
        { 32.4f, 35.0f, 2.45f, 18.0f, wet },
        { 35.8f, 38.5f, 3.35f, 18.0f, moss },
        { 39.5f, 43.5f, 5.25f, 22.0f, dark },
    };
    const int segs = 64;
    for (int ri = 0; ri < 6; ri++) {
        Ring& R = rings[ri];
        float midR = (R.ri + R.ro) * 0.5f, thick = (R.ro - R.ri);
        float segW = 2.0f * midR * sinf(PI / segs) * 1.2f;
        for (int i = 0; i < segs; i++) {
            float deg = i * 360.0f / segs;
            bool gap = false;
            for (int g = 0; g < 4; g++) {
                float dd = fabsf(deg - GW_ANG[g]); if (dd > 180.0f) dd = 360.0f - dd;
                if (dd < R.gap) { gap = true; break; }
            }
            if (gap) continue;
            float a = deg * DEG2RAD;
            Vector3 p = ctr + Vector3{ sinf(a) * midR, 0, cosf(a) * midR };
            DrawModelEx(s_column, Vector3{ p.x, R.top * 0.5f, p.z }, Vector3{ 0, 1, 0 }, deg,
                        Vector3{ segW, R.top, thick }, R.c);
        }
    }

    // four arched gateways through the wall
    for (int g = 0; g < 4; g++) {
        float a = GW_ANG[g] * DEG2RAD;
        Vector3 rad{ sinf(a), 0, cosf(a) }, tng{ cosf(a), 0, -sinf(a) };
        Vector3 base = ctr + Vector3Scale(rad, 38.0f);
        for (int s = -1; s <= 1; s += 2) {                       // pillars flanking the opening
            Vector3 pp = base + Vector3Scale(tng, 4.1f * s);
            DrawModelEx(s_column, Vector3{ pp.x, 2.6f, pp.z }, Vector3{ 0, 1, 0 }, GW_ANG[g],
                        Vector3{ 1.2f, 5.2f, 5.0f }, wet);
        }
        for (int k = -3; k <= 3; k++) {                          // stepped arch crown
            float fx = k / 3.0f;
            float h = 5.2f + (8.4f - 5.2f) * (1.0f - fx * fx);
            Vector3 pp = base + Vector3Scale(tng, k * 1.2f);
            DrawModelEx(s_column, Vector3{ pp.x, h - 0.35f, pp.z }, Vector3{ 0, 1, 0 }, GW_ANG[g],
                        Vector3{ 1.35f, 0.7f, 5.0f }, dark);
        }
    }

    // central dais (low, two-step)
    DrawModelEx(s_column, ctr + Vector3{ 0, 0.20f, 0 }, Vector3{ 0, 1, 0 }, 0, Vector3{ 8.4f, 0.40f, 8.4f }, moss);
    DrawModelEx(s_column, ctr + Vector3{ 0, 0.09f, 0 }, Vector3{ 0, 1, 0 }, 0, Vector3{ 10.0f, 0.20f, 10.0f }, wet);

    // brazier pedestals (lit) then flames (additive), two per gateway
    Vector3 bpos[8]; int nb = 0;
    for (int g = 0; g < 4; g++) {
        float a = GW_ANG[g] * DEG2RAD;
        Vector3 rad{ sinf(a), 0, cosf(a) }, tng{ cosf(a), 0, -sinf(a) };
        for (int s = -1; s <= 1; s += 2) {
            Vector3 bp = ctr + Vector3Scale(rad, 34.6f) + Vector3Scale(tng, 5.4f * s);
            bpos[nb++] = bp;
            DrawModelEx(s_column, Vector3{ bp.x, 0.9f, bp.z }, Vector3{ 0, 1, 0 }, 0, Vector3{ 0.7f, 1.8f, 0.7f }, dark);
            DrawModelEx(s_column, Vector3{ bp.x, 1.95f, bp.z }, Vector3{ 0, 1, 0 }, 0, Vector3{ 1.1f, 0.35f, 1.1f }, wet);   // bowl
        }
    }
    BeginBlendMode(BLEND_ADDITIVE);
    for (int b = 0; b < nb; b++) {
        Vector3 bp = bpos[b];
        for (int f = 0; f < 5; f++) {
            float fa = f * 1.3f + b;
            float fl = 0.5f + 0.5f * sinf(s_time * 7.0f + fa * 2.1f);
            Vector3 fp = bp + Vector3{ 0.13f * sinf(fa + s_time), 2.15f + (0.3f + 0.55f * fl) * 0.5f, 0.13f * cosf(fa + s_time) };
            DrawSphereEx(fp, 0.16f + 0.11f * fl, 6, 6, Color{ 255, (unsigned char)(130 + 90 * fl), 30, 130 });
        }
        DrawSphereEx(bp + Vector3{ 0, 2.3f, 0 }, 0.9f, 8, 8, Color{ 255, 120, 35, 48 });
    }
    EndBlendMode();
}

void draw_water(Camera3D cam, Texture2D reflectTex, Vector2 screen) {
    SetShaderValue(s_water_shader, s_loc_screen, &screen, SHADER_UNIFORM_VEC2);
    SetShaderValue(s_water_shader, s_loc_time, &s_time, SHADER_UNIFORM_FLOAT);
    Vector3 vp = cam.position;
    SetShaderValue(s_water_shader, s_loc_view, &vp, SHADER_UNIFORM_VEC3);
    Vector3 mp = MOON_POS;
    SetShaderValue(s_water_shader, s_loc_moonpos, &mp, SHADER_UNIFORM_VEC3);
    SetShaderValueTexture(s_water_shader, s_loc_reflect, reflectTex);
    BeginBlendMode(BLEND_ALPHA);
    DrawModel(s_water, Vector3{ boundary_center.x, water_level, boundary_center.z }, 1.0f, WHITE);
    EndBlendMode();
}

void unload() {
    if (s_has_model) UnloadModel(s_model);
    if (s_has_column) UnloadModel(s_column);
    if (s_has_moon) { UnloadModel(s_moon); UnloadShader(s_moon_shader); }
    UnloadModel(s_sky);   UnloadShader(s_sky_shader);
    UnloadModel(s_water); UnloadShader(s_water_shader);
    s_crystals.clear();
    s_crystal_lights.clear();
}

// ---------------------------------------------------------------- collision
void resolve(Vector3& pos, float r) {
    if (pos.y < floor_y) pos.y = floor_y;
    float dx = pos.x - boundary_center.x, dz = pos.z - boundary_center.z;
    float d = sqrtf(dx * dx + dz * dz);
    float maxd = boundary_radius - 0.6f - r;
    if (d > maxd && d > 0.0001f) {
        float s = maxd / d;
        pos.x = boundary_center.x + dx * s;
        pos.z = boundary_center.z + dz * s;
    }
    for (auto& o : obstacles) {
        float ox = pos.x - o.center.x, oz = pos.z - o.center.z;
        float od = sqrtf(ox * ox + oz * oz);
        float mind = o.radius + r;
        if (od < mind && od > 0.0001f) {
            float s = mind / od;
            pos.x = o.center.x + ox * s;
            pos.z = o.center.z + oz * s;
        }
    }
}

}
