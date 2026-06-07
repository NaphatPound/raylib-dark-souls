#include "arena.h"
#include "assets.h"
#include "render.h"
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
            s_crystal_lights.push_back(Vector4{ p.x, water_level, p.z, 4.0f + cs * 2.4f });
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
    // blood moon: a camera-facing quad shaded as a sphere in moon.fs (no UV-sphere
    // pole pinching), mapping the crater texture and masking to a circular disc.
    Mesh disc = GenMeshPlane(48.0f, 48.0f, 1, 1);
    s_moon = LoadModelFromMesh(disc);
    s_moon.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = assets::texture("textures/moon/blood_moon.png");
    s_moon_shader = LoadShader(assets::path("shaders/moon.vs").c_str(),
                               assets::path("shaders/moon.fs").c_str());
    s_moon.materials[0].shader = s_moon_shader;
    s_has_moon = true;

    // night-sky dome: a unit sphere drawn centred on the camera (scaled large) so
    // its inside fills the background with a gradient + moon glow, in both the main
    // view and the reflection pass.
    Mesh dome = GenMeshSphere(1.0f, 16, 32);
    s_sky = LoadModelFromMesh(dome);
    s_sky_shader = LoadShader(assets::path("shaders/sky.vs").c_str(),
                              assets::path("shaders/sky.fs").c_str());
    s_sky.materials[0].shader = s_sky_shader;
    s_loc_sky_moon = GetShaderLocation(s_sky_shader, "uMoonDir");

    // reflective water plane (subdivided so the vertex waves have shape)
    Mesh plane = GenMeshPlane(320.0f, 320.0f, 80, 80);
    s_water = LoadModelFromMesh(plane);
    s_water_shader = LoadShader(assets::path("shaders/water.vs").c_str(),
                                assets::path("shaders/water.fs").c_str());
    s_water_shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(s_water_shader, "matModel");
    s_loc_time = GetShaderLocation(s_water_shader, "uTime");
    s_loc_view = GetShaderLocation(s_water_shader, "uViewPos");
    s_loc_reflect = GetShaderLocation(s_water_shader, "uReflect");
    s_loc_screen = GetShaderLocation(s_water_shader, "uScreen");
    s_loc_moonpos = GetShaderLocation(s_water_shader, "uMoonPos");
    s_loc_cry_n = GetShaderLocation(s_water_shader, "uCrystalN");
    s_loc_cry_arr = GetShaderLocation(s_water_shader, "uCrystals");
    s_water.materials[0].shader = s_water_shader;

    // obstacle collision for the rocks that intrude into the playable ring
    obstacles.push_back({ { -12.0f, 0, -18.0f }, 3.2f });   // Rock_E
    obstacles.push_back({ {  15.0f, 0, -22.0f }, 3.6f });   // Rock_F

    build_crystals();

    // upload the static crystal lights to the water shader (positions never move)
    int cn = (int)s_crystal_lights.size();
    SetShaderValue(s_water_shader, s_loc_cry_n, &cn, SHADER_UNIFORM_INT);
    if (cn > 0)
        SetShaderValueV(s_water_shader, s_loc_cry_arr, s_crystal_lights.data(),
                        SHADER_UNIFORM_VEC4, cn);
}

void update(float dt) { s_time += dt; }

// ---------------------------------------------------------------- draw
static void draw_crystals();

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
    // face the disc toward the arena (+Z): rotate the XZ plane up about X
    if (s_has_moon) DrawModelEx(s_moon, MOON_POS, Vector3{ 1, 0, 0 }, 90.0f, Vector3{ 1, 1, 1 }, WHITE);
    if (s_has_model)
        for (int i = 0; i < s_model.meshCount; i++)
            if (!s_skip_mesh[i])
                DrawMesh(s_model.meshes[i], s_model.materials[s_model.meshMaterial[i]], s_model.transform);
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
    // cloudy red gem bodies + soft facet lines (mostly opaque -> depth write stays on)
    BeginBlendMode(BLEND_ALPHA);
    for (auto& c : s_crystals) {
        for (auto& g : c.gems) {
            place_gem(g);
            float s = g.shade;
            Color body{ (unsigned char)(165 * s), (unsigned char)(30 * s),
                        (unsigned char)(27 * s), 212 };               // cloudy deep red
            DrawCubeV(Vector3{ 0, 0, 0 }, g.size, body);
            Color edge{ (unsigned char)(95 * s), (unsigned char)(18 * s),
                        (unsigned char)(16 * s), 225 };               // soft dark-red facets
            DrawCubeWiresV(Vector3{ 0, 0, 0 }, g.size, edge);
            rlPopMatrix();
        }
    }
    EndBlendMode();

    // soft static red light halo
    BeginBlendMode(BLEND_ADDITIVE);
    for (auto& c : s_crystals) {
        DrawSphereEx(c.light_pos, 0.42f, 8, 8, Color{ 200, 30, 24, 85 });
        DrawSphereEx(c.light_pos, 0.95f, 8, 8, Color{ 165, 24, 18, 30 });
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
