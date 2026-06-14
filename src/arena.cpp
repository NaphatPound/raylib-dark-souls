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
static Model s_cyl{};                    // unit lit cylinder (mushroom stalks, fungal grotto)
static Model s_dome{};                   // unit lit hemisphere (mushroom caps)
static Model s_cone{};                   // unit lit hex cone (crystal shards, crystal cavern)
static Model s_torus{};                  // unit lit torus (armillary rings, astral observatory)
static bool s_has_shapes = false;
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

// ---------------------------------------------------------------- graveyard data
// A leaning headstone (lit cube), a bare dead tree, and floating wisp markers.
// Built once per level load (deterministic) so nothing jitters frame to frame.
struct Tomb { Vector3 pos; float yaw, w, h, t, tilt; bool cross; Color col; };
struct Tree { Vector3 base; float h, r; int nbr; float br_yaw[3], br_len[3], br_tilt[3]; };
static std::vector<Tomb>    s_tombs;
static std::vector<Tree>    s_trees;
static std::vector<Vector3> s_wisps;   // world centres the will-o'-wisp flames bob around

// ---------------------------------------------------------------- fungal grotto data
// A giant mushroom: a leaning lit cylinder stalk + a lit hemisphere cap, with a
// bioluminescent under-cap glow. Built once per level load (deterministic).
struct Shroom { Vector3 base; float stalkH, stalkR, capR, capH, lean, leanYaw;
                Color stalkC, capC, glowC; };
static std::vector<Shroom> s_shrooms;

// ---------------------------------------------------------------- desert ziggurat data
// A tilted half-fallen obelisk (lit cube shaft + pyramidion cap) and a broken
// column (a short stack of lit-cylinder drums). Built once per level load.
struct Obelisk { Vector3 pos; float w, h, lean, leanYaw; Color col; };
struct Drum    { Vector3 pos; float r, h, lean, leanYaw; int stack; Color col; };
static std::vector<Obelisk> s_obelisks;
static std::vector<Drum>    s_drums;

// ---------------------------------------------------------------- shipwreck cove data
// A wrecked ship (long listing lit-cube hull + leaning lit-cylinder mast) and a
// small floating prop (barrel = cylinder, crate = cube). Built once per level load.
struct Wreck { Vector3 pos; float yaw, list, len, w, h, mastH; bool mast; Color col; };
struct Prop  { Vector3 pos; float yaw, s; bool crate; Color col; };
static std::vector<Wreck> s_wrecks;
static std::vector<Prop>  s_props;

// ---------------------------------------------------------------- sky sanctum data
// A floating satellite island: a lit-cylinder disc with a tapered rocky underside,
// drifting at its own height/tilt around the main platform. Built once per load.
struct Isle { Vector3 pos; float r, tilt, tiltYaw; Color col; };
static std::vector<Isle> s_isles;

// ---------------------------------------------------------------- clockwork vault data
// A giant gear: a lit-cylinder hub + lit-cube teeth + spokes, spinning about its own
// axis (flat = about Y, upright = about world Z). Built once per load; spun in draw.
struct Gear { Vector3 c; float radius, thick, speed; bool upright; int teeth; Color body, tooth, spoke; };
static std::vector<Gear> s_gears;

// ---------------------------------------------------------------- forgotten shrine data
// A torii gate (round posts + lintels), a stone lantern, and a cherry-blossom tree.
// Built once per load; the shrine stands in the reused reflecting water.
struct Torii   { Vector3 c; float yaw, w, h; Color col; };
struct Lantern { Vector3 b; };
struct Blossom { Vector3 base; float h, r; };
static std::vector<Torii>   s_torii;
static std::vector<Lantern> s_lanterns;
static std::vector<Blossom> s_blossoms;

// ---------------------------------------------------------------- dragon boneyard data
// Colossal skeletons: a ribcage (spine + arcing ribs), a horned skull, and loose
// long bones. Drawn from lit cylinder segments oriented between arbitrary endpoints
// (draw_bone_seg). Built once per load; the boneyard is a dry ash basin.
struct BonePiece { Vector3 a, b; float r; Color col; };
struct Ribcage   { Vector3 c; float yaw, len, ribH, spread; int pairs; };
struct Skull     { Vector3 c; float yaw, s; };
static std::vector<BonePiece> s_bones;
static std::vector<Ribcage>   s_ribcages;
static std::vector<Skull>     s_skulls;

// ---------------------------------------------------------------- crystal cavern data
// A faceted crystal shard: a lit hex cone, tilted, glowing. Clusters jut from rocky
// mounds (floor) or hang from the ceiling. Built once per load.
struct Crys { Vector3 base; float r, h, tilt, tiltYaw; bool ceiling; Color col, glow; };
static std::vector<Crys> s_crys;

// ---------------------------------------------------------------- overgrown temple data
// A gnarled tree root (a chained arc of cylinder segments via draw_bone_seg) and a
// patch of foliage (a green dome bush, or a trunk+canopy tree). Built once per load.
struct Root    { Vector3 a, b; float archH, r; };
struct Foliage { Vector3 pos; float r; bool tree; Color col; };
static std::vector<Root>    s_roots;
static std::vector<Foliage> s_foliage;

// ---------------------------------------------------------------- abandoned mine data
// A timber support frame (posts + beam + braces), a derelict ore cart (box on wheels),
// and a glowing ore vein (gold s_cone crystals in dark rock). Built once per load.
struct Frame   { Vector3 c; float yaw, w, h; };
struct Cart    { Vector3 c; float yaw; bool tipped; };
struct OreVein { Vector3 base; float scale; };
static std::vector<Frame>   s_frames;
static std::vector<Cart>    s_carts;
static std::vector<OreVein> s_veins;

// ---------------------------------------------------------------- astral observatory data
// A telescope (tripod + tilted tube) and a celestial globe pedestal. The armillary
// rings + orbiting planets + stars are drawn analytically in draw_obs (time-animated).
struct Scope { Vector3 c; float yaw, pitch; };
struct Globe { Vector3 c; Color col; bool ringed; };
static std::vector<Scope> s_scopes;
static std::vector<Globe> s_globes;

// ---------------------------------------------------------------- forgotten archive data
// A bookshelf (frame + shelf boards + rows of coloured book spines) and a reading
// lectern. Books are placed deterministically by index in draw. Built once per load.
struct Shelf   { Vector3 c; float yaw, w, h, lean; };
struct Lectern { Vector3 c; float yaw; };
static std::vector<Shelf>   s_shelves;
static std::vector<Lectern> s_lecterns;

// ---------------------------------------------------------------- siege camp data
// A conical canvas tent (s_cone canvas + pole + entrance) and a banner pole with a
// swaying flag. Built once per load; the camp sits on a dry muddy field.
struct Tent   { Vector3 c; float yaw, r, h; bool collapsed; Color canvas; };
struct Banner { Vector3 c; float h; Color flag; };
static std::vector<Tent>   s_tents;
static std::vector<Banner> s_banners;

// ---------------------------------------------------------------- sunken aqueduct data
// A vaulted arch: two piers + a semicircular ring of voussoir blocks + an entablature.
// Built once per load; the aqueduct stands in the reused dark water channel.
struct Arch { Vector3 c; float yaw, span, pierH, thick; };
static std::vector<Arch> s_arches;

// ---------------------------------------------------------------- sentinel court data
// A colossal guardian statue (pedestal + legs + torso + pauldrons + arms + greatsword
// + helmeted dome head), some headless. Built once per load; stands in reflecting water.
struct Statue { Vector3 c; float yaw, scale; bool headless; };
static std::vector<Statue> s_statues;

// ---------------------------------------------------------------- hanging gardens data
// A flowering bush (coloured s_dome) and a cypress topiary (tall s_cone). The
// fountains, beds, trellises and hedges are drawn analytically. Built once per load.
struct Bush    { Vector3 pos; float r; Color col; };
struct Cypress { Vector3 pos; float h, r; };
static std::vector<Bush>    s_bushes;
static std::vector<Cypress> s_cypress;

// ---------------------------------------------------------------- beacon coast data
// A jagged sea-stack (a dark tapered rock spire). The lighthouse, breakwater, boats
// and sweeping beam are drawn analytically. Built once per load; stands in the sea.
struct Stack { Vector3 base; float h, r; };
static std::vector<Stack> s_stacks;

// ---------------------------------------------------------------- windmill fields data
// A windmill (tapered tower + cap + a rotating 4-sail cross) and a hay bale/stack.
// Built once per load; spun in draw. The field is dry grass.
struct Mill { Vector3 c; float yaw, scale, speed; };
struct Hay  { Vector3 pos; float r; bool stack; };
static std::vector<Mill> s_mills;
static std::vector<Hay>  s_hay;

// ---------------------------------------------------------------- ossuary catacombs data
// A stone sarcophagus. The skull-walls, reliquary, candles and bone piles are drawn
// analytically (rows of s_dome skulls + s_cyl bone courses). Built once per load.
struct Sarco { Vector3 c; float yaw; };
static std::vector<Sarco> s_sarcos;

// ---------------------------------------------------------------- petrified forest data
// A twisted petrified tree (a gnarled chain of cylinder segments + forking branches via
// draw_bone_seg, shaped deterministically from `seed`). Built once per load; dry ground.
struct PTree { Vector3 base; float h, r; int seed; };
static std::vector<PTree> s_ptrees;

// LEVEL_HAMLET: a plague-stricken village of pitched-roof timber cottages around a
// well, with a steepled chapel. draw_cottage builds an A-frame roof from two tilted
// lit slabs inside a yawed rlgl matrix frame. Built once per load; dry dirt ground.
struct Cottage { Vector3 base; float w, d, wallH, roofH, yawDeg; int variant; };
static std::vector<Cottage> s_cottages;

// LEVEL_HENGE: a megalithic druid stone circle. A `Sarsen` is one standing stone
// (lintel=true adds a capstone, making it a trilithon when paired); tilt/fallen
// stones lean via the rlgl matrix. Built once per load; dry heath turf.
struct Sarsen { Vector3 pos; float w, h, t, yawDeg, tiltDeg; bool lintel; int variant; };
static std::vector<Sarsen> s_sarsens;

// LEVEL_BOG: a fetid swamp of weeping dead willows over murky water. A `Willow` is a
// gnarled trunk (chained draw_bone_seg) crowned with drooping branches; the level is
// WET (the reused water plane is the murky floor). Built once per load.
struct Willow { Vector3 base; float h, r; int seed; };
static std::vector<Willow> s_willows;

// LEVEL_SAWMILL: a timber yard. A `LogPile` is a pyramidal stack of horizontal logs
// (each log a draw_bone_seg cylinder). The mill shed houses a giant spinning circular
// sawblade (animated via s_time). Built once per load; dry wood-chip ground.
struct LogPile { Vector3 c; float yawDeg, logLen, logR; int rows; };
static std::vector<LogPile> s_logpiles;

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
    if (g_level == LEVEL_GRAVEYARD) return;   // wisp lights are built in build_graveyard()
    if (g_level == LEVEL_FUNGAL) return;      // spore-pod lights are built in build_fungal()
    if (g_level == LEVEL_DESERT) return;      // brazier lights are built in build_desert()
    if (g_level == LEVEL_WRECK) return;       // lantern lights are built in build_wreck()
    if (g_level == LEVEL_SANCTUM) return;     // soul-motes are built in build_sanctum()
    if (g_level == LEVEL_CLOCK) return;       // furnace/vent lights are built in build_clock()
    if (g_level == LEVEL_SHRINE) return;      // lantern lights are built in build_shrine()
    if (g_level == LEVEL_BONES) return;       // corpse-lights are built in build_bones()
    if (g_level == LEVEL_GEODE) return;       // crystal glow is built in build_geode()
    if (g_level == LEVEL_TEMPLE) return;      // firefly lights are built in build_temple()
    if (g_level == LEVEL_MINE) return;        // ore/lantern lights are built in build_mine()
    if (g_level == LEVEL_OBS) return;         // sun-orb/globe lights are built in build_obs()
    if (g_level == LEVEL_LIB) return;         // candle lights are built in build_lib()
    if (g_level == LEVEL_CAMP) return;        // campfire lights are built in build_camp()
    if (g_level == LEVEL_AQUA) return;        // algae lights are built in build_aqua()
    if (g_level == LEVEL_COURT) return;       // brazier lights are built in build_court()
    if (g_level == LEVEL_GARDEN) return;      // soft fill lights are built in build_garden()
    if (g_level == LEVEL_BRIDGE) return;      // lantern lights are built in build_bridge()
    if (g_level == LEVEL_BEACON) return;      // lamp light is built in build_beacon()
    if (g_level == LEVEL_MILL) return;        // barn-window lights are built in build_mill()
    if (g_level == LEVEL_OSSUARY) return;     // candle lights are built in build_ossuary()
    if (g_level == LEVEL_FAIR) return;        // string-lights are built in build_fair()
    if (g_level == LEVEL_THRONE) return;      // sconce lights are built in build_throne()
    if (g_level == LEVEL_SPRINGS) return;     // mineral/geyser lights are built in build_springs()
    if (g_level == LEVEL_PFOREST) return;     // amber-resin lights are built in build_pforest()
    if (g_level == LEVEL_HAMLET) return;      // plague-lantern lights are built in build_hamlet()
    if (g_level == LEVEL_HENGE) return;       // ley-line lights are built in build_henge()
    if (g_level == LEVEL_BOG) return;         // swamp-gas lights are built in build_bog()
    if (g_level == LEVEL_SAWMILL) return;     // work-lamp lights are built in build_lumber()
    if (g_level == LEVEL_GAOL) return;        // torch lights are built in build_gaol()
    if (g_level == LEVEL_TAR) return;         // sulfur-seep lights are built in build_tar()
    if (g_level == LEVEL_VINE) return;        // golden lamp lights are built in build_vine()
    if (g_level == LEVEL_FLOE) return;        // aurora / ice lights are built in build_floes()
    if (g_level == LEVEL_QUARRY) return;      // work-lamp lights are built in build_quarry()
    if (g_level == LEVEL_DOCK) return;        // signal-lamp lights are built in build_dock()
    if (g_level == LEVEL_GLASS) return;       // glasshouse lights are built in build_glass()
    if (g_level == LEVEL_APIARY) return;      // honey-glow lights are built in build_apiary()
    if (g_level == LEVEL_KILN) return;        // kiln-fire lights are built in build_kiln()
    if (g_level == LEVEL_REEF) return;        // bioluminescent lights are built in build_reef()
    if (g_level == LEVEL_FOUNDRY) return;     // molten/furnace lights are built in build_foundry()
    if (g_level == LEVEL_NAVE) return;        // candle/glass lights are built in build_nave()
    if (g_level == LEVEL_WATERMILL) return;   // lantern lights are built in build_watermill()
    if (g_level == LEVEL_SALT) return;        // brine-sheen lights are built in build_salt()
    if (g_level == LEVEL_STILT) return;       // lantern lights are built in build_stilt()
    if (g_level == LEVEL_HIVE) return;        // amber-wax lights are built in build_hive()
    if (g_level == LEVEL_BREW) return;        // copper/furnace lights are built in build_brew()
    if (g_level == LEVEL_BAMBOO) return;      // dappled/lantern lights are built in build_bamboo()
    if (g_level == LEVEL_COLLIER) return;     // ember lights are built in build_collier()
    if (g_level == LEVEL_PLAZA) return;       // civic lamp lights are built in build_plaza()
    if (g_level == LEVEL_PUMPKIN) return;     // jack-o'-lantern lights are built in build_pumpkin()
    if (g_level == LEVEL_BELL) return;        // bronze lights are built in build_bell()
    if (g_level == LEVEL_AVIARY) return;      // cage lights are built in build_aviary()
    if (g_level == LEVEL_WEB) return;         // silk-glow lights are built in build_web()
    if (g_level == LEVEL_LOTUS) return;       // lantern lights are built in build_lotus()
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
static void build_graveyard();   // defined below; called from load()
static void build_fungal();      // defined below; called from load()
static void build_desert();      // defined below; called from load()
static void build_wreck();       // defined below; called from load()
static void build_sanctum();     // defined below; called from load()
static void build_clock();       // defined below; called from load()
static void build_shrine();      // defined below; called from load()
static void build_bones();       // defined below; called from load()
static void build_geode();       // defined below; called from load()
static void build_temple();      // defined below; called from load()
static void build_mine();        // defined below; called from load()
static void build_obs();         // defined below; called from load()
static void build_lib();         // defined below; called from load()
static void build_camp();        // defined below; called from load()
static void build_aqua();        // defined below; called from load()
static void build_court();       // defined below; called from load()
static void build_garden();      // defined below; called from load()
static void build_bridge();      // defined below; called from load()
static void build_beacon();      // defined below; called from load()
static void build_mill();        // defined below; called from load()
static void build_ossuary();     // defined below; called from load()
static void build_fair();        // defined below; called from load()
static void build_throne();      // defined below; called from load()
static void build_springs();     // defined below; called from load()
static void build_pforest();     // defined below; called from load()
static void build_hamlet();      // defined below; called from load()
static void build_henge();       // defined below; called from load()
static void build_bog();         // defined below; called from load()
static void build_lumber();      // defined below; called from load()
static void build_gaol();        // defined below; called from load()
static void build_tar();         // defined below; called from load()
static void build_vine();        // defined below; called from load()
static void build_floes();       // defined below; called from load()
static void build_quarry();      // defined below; called from load()
static void build_dock();        // defined below; called from load()
static void build_glass();       // defined below; called from load()
static void build_apiary();      // defined below; called from load()
static void build_kiln();        // defined below; called from load()
static void build_reef();        // defined below; called from load()
static void build_foundry();     // defined below; called from load()
static void build_nave();        // defined below; called from load()
static void build_watermill();   // defined below; called from load()
static void build_salt();        // defined below; called from load()
static void build_stilt();       // defined below; called from load()
static void build_hive();        // defined below; called from load()
static void build_brew();        // defined below; called from load()
static void build_bamboo();      // defined below; called from load()
static void build_collier();     // defined below; called from load()
static void build_plaza();       // defined below; called from load()
static void build_pumpkin();     // defined below; called from load()
static void build_bell();        // defined below; called from load()
static void build_aviary();      // defined below; called from load()
static void build_web();         // defined below; called from load()
static void build_lotus();       // defined below; called from load()
static void build_archtree();    // defined below; called from load()
static void build_chess();       // defined below; called from load()
static void build_maze();        // defined below; called from load()
static void build_falls();       // defined below; called from load()
static void build_crater();      // defined below; called from load()
static void build_titan();       // defined below; called from load()
static void build_hoodoo();      // defined below; called from load()
static void build_moai();        // defined below; called from load()
static void build_cavern();      // defined below; called from load()
static void build_pines();       // defined below; called from load()
static void build_galleon();     // defined below; called from load()
static void build_sunflower();   // defined below; called from load()
static void build_sphinx();      // defined below; called from load()
static void build_dam();         // defined below; called from load()
static void build_theatre();     // defined below; called from load()
static void build_wheel();       // defined below; called from load()
static void build_redwood();     // defined below; called from load()
static void build_balloon();     // defined below; called from load()
static void build_canal();       // defined below; called from load()
static void build_silo();        // defined below; called from load()
static void build_organ();       // defined below; called from load()
static void build_hypo();        // defined below; called from load()
static void build_fort();        // defined below; called from load()
static void build_triumph();     // defined below; called from load()
static void build_orchard();     // defined below; called from load()
static void build_loom();        // defined below; called from load()
static void build_savanna();     // defined below; called from load()
static void build_mosque();      // defined below; called from load()
static void build_totem();       // defined below; called from load()
static void build_oasis();       // defined below; called from load()
static void build_pagoda();      // defined below; called from load()
static void build_stepwell();    // defined below; called from load()
static void build_jantar();      // defined below; called from load()
static void build_terracotta();  // defined below; called from load()
static void build_ballcourt();   // defined below; called from load()
static void build_petra();       // defined below; called from load()
static void build_bazaar();      // defined below; called from load()
static void build_siege();       // defined below; called from load()
static void build_whaling();     // defined below; called from load()
static void build_cactus();      // defined below; called from load()
static void build_zimbabwe();    // defined below; called from load()
static void build_basil();       // defined below; called from load()
static void build_print();       // defined below; called from load()
static void build_ger();         // defined below; called from load()
static void build_alchemy();     // defined below; called from load()
static void build_baths();       // defined below; called from load()
static void build_float();       // defined below; called from load()
static void build_ishtar();      // defined below; called from load()
static void build_tannery();     // defined below; called from load()
static void build_museum();      // defined below; called from load()
static void build_gompa();       // defined below; called from load()
static void build_buddha();      // defined below; called from load()
static void build_angkor();      // defined below; called from load()
static void build_badgir();      // defined below; called from load()
static void build_topiary();     // defined below; called from load()
static void build_glassworks();  // defined below; called from load()
static void build_shipyard();    // defined below; called from load()
static void build_gothic();      // defined below; called from load()
static void build_pueblo();      // defined below; called from load()
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

    // lit round primitives for the Fungal Grotto's giant mushrooms (stalk + dome cap),
    // so they take the grotto's teal light + fog like the rest of the scenery
    s_cyl = LoadModelFromMesh(GenMeshCylinder(1.0f, 1.0f, 14));
    s_cyl.materials[0].shader = lit;
    s_dome = LoadModelFromMesh(GenMeshHemiSphere(1.0f, 8, 18));
    s_dome.materials[0].shader = lit;
    s_cone = LoadModelFromMesh(GenMeshCone(1.0f, 1.0f, 6));   // hex cone = faceted crystal shard
    s_cone.materials[0].shader = lit;
    s_torus = LoadModelFromMesh(GenMeshTorus(1.0f, 0.06f, 8, 28));   // thin ring (armillary)
    s_torus.materials[0].shader = lit;
    s_has_shapes = true;

    // blood moon: a camera-facing quad shaded as a sphere in moon.fs (no UV-sphere
    // pole pinching), mapping the crater texture and masking to a circular disc.
    Mesh disc = GenMeshPlane(48.0f, 48.0f, 1, 1);
    s_moon = LoadModelFromMesh(disc);
    const char* moon_tex_by[117] = { "textures/moon/blood_moon.png", "textures/moon/moon_surface.png", "textures/moon/blood_moon.png", "textures/moon/blood_moon.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png", "textures/moon/moon_surface.png" };
    const char* moon_fs_by[117]  = { "shaders/moon.fs", "shaders/moon_crescent.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs", "shaders/moon.fs" };
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
    const char* sky_fs_by[117] = { "shaders/sky.fs", "shaders/sky_ice.fs", "shaders/sky_forge.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_forge.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_forge.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_forge.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_forge.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_ice.fs", "shaders/sky_storm.fs", "shaders/sky_storm.fs", "shaders/sky_ice.fs", "shaders/sky_forge.fs" };   // [15]..[29], [30] henge (cool twilight), [31] bog, [32] sawmill, [33] gaol, [34] tar, [35] vineyard (warm dusk), [36] floes (icy), [37] quarry, [38] skydock (high sky), [39] conservatory (bright), [40] apiary (sunny), [41] kiln (dusty), [42] reef (deep blue), [43] foundry (dark), [44] nave (interior), [45] watermill, [46] salt (blinding), [47] stilt (lagoon), [48] hive (dark amber), [49] brewery (dim), [50] bamboo (green), [51] collier (smoky), [52] plaza (sunny), [53] pumpkin (autumn dusk), [54] bell (overcast), [55] aviary (dim), [56] web (dark cavern), [57] lotus (serene), [44] nave (interior)
    s_sky_shader = LoadShader(assets::path("shaders/sky.vs").c_str(),
                              assets::path(sky_fs_by[g_level]).c_str());
    s_sky.materials[0].shader = s_sky_shader;
    s_loc_sky_moon = GetShaderLocation(s_sky_shader, "uMoonDir");

    // reflective water plane (subdivided so the vertex waves have shape)
    Mesh plane = GenMeshPlane(320.0f, 320.0f, 80, 80);
    s_water = LoadModelFromMesh(plane);
    const char* water_fs_by[117] = { "shaders/water.fs", "shaders/water_ice.fs", "shaders/water_forge.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water.fs", "shaders/water_storm.fs", "shaders/water_ice.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_ice.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_ice.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_ice.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_ice.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_ice.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_ice.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_ice.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_ice.fs", "shaders/water_storm.fs", "shaders/water_ice.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_ice.fs", "shaders/water_ice.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_ice.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs", "shaders/water_storm.fs" };   // [15]..[26], [27] springs (turquoise), [28] petrified (dry), [29] hamlet (dry), [30] henge (dry), [31] bog (murky, WET), [32] sawmill (dry), [33] gaol (dry), [34] tar (dry), [35] vineyard (dry), [36] floes (icy sea, WET), [37] quarry (dry), [38] skydock (dry), [39] conservatory (dry), [40] apiary (dry), [41] kiln (dry), [42] reef (dry seabed), [43] foundry (dry), [44] nave (dry), [45] watermill (dry), [46] salt (dry), [47] stilt (lagoon, WET), [48] hive (dry), [49] brewery (dry), [50] bamboo (dry), [51] collier (dry), [52] plaza (dry), [53] pumpkin (dry), [54] bell (dry), [55] aviary (dry), [56] web (dry), [57] lotus (pond, WET), [54] bell (dry), [55] aviary (dry), [54] bell (dry), [44] nave (dry), [41] kiln (dry)
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
    obstacles.clear();   // each level rebuilds its own obstacle set on load (avoid stale carry-over)
    if (g_level != LEVEL_COLOSSEUM && g_level != LEVEL_GRAVEYARD && g_level != LEVEL_FUNGAL && g_level != LEVEL_DESERT && g_level != LEVEL_WRECK && g_level != LEVEL_SANCTUM && g_level != LEVEL_CLOCK && g_level != LEVEL_SHRINE && g_level != LEVEL_BONES && g_level != LEVEL_GEODE && g_level != LEVEL_TEMPLE && g_level != LEVEL_MINE && g_level != LEVEL_OBS && g_level != LEVEL_LIB && g_level != LEVEL_CAMP && g_level != LEVEL_AQUA && g_level != LEVEL_COURT && g_level != LEVEL_GARDEN && g_level != LEVEL_BRIDGE && g_level != LEVEL_BEACON && g_level != LEVEL_MILL && g_level != LEVEL_OSSUARY && g_level != LEVEL_FAIR && g_level != LEVEL_THRONE && g_level != LEVEL_SPRINGS && g_level != LEVEL_PFOREST && g_level != LEVEL_HAMLET && g_level != LEVEL_HENGE && g_level != LEVEL_BOG && g_level != LEVEL_SAWMILL && g_level != LEVEL_GAOL && g_level != LEVEL_TAR && g_level != LEVEL_VINE && g_level != LEVEL_FLOE && g_level != LEVEL_QUARRY && g_level != LEVEL_DOCK && g_level != LEVEL_GLASS && g_level != LEVEL_APIARY && g_level != LEVEL_KILN && g_level != LEVEL_REEF && g_level != LEVEL_FOUNDRY && g_level != LEVEL_NAVE && g_level != LEVEL_WATERMILL && g_level != LEVEL_SALT && g_level != LEVEL_STILT && g_level != LEVEL_HIVE && g_level != LEVEL_BREW && g_level != LEVEL_BAMBOO && g_level != LEVEL_COLLIER && g_level != LEVEL_PLAZA && g_level != LEVEL_PUMPKIN && g_level != LEVEL_BELL && g_level != LEVEL_AVIARY && g_level != LEVEL_WEB && g_level != LEVEL_LOTUS) {
        obstacles.push_back({ { -12.0f, 0, -18.0f }, 3.2f });   // Rock_E
        obstacles.push_back({ {  15.0f, 0, -22.0f }, 3.6f });   // Rock_F
    }
    if (g_level == LEVEL_GRAVEYARD)
        obstacles.push_back({ boundary_center, 2.6f });          // solid central mausoleum
    if (g_level == LEVEL_FUNGAL)
        obstacles.push_back({ boundary_center, 2.4f });          // colossal central mushroom
    if (g_level == LEVEL_DESERT)
        obstacles.push_back({ boundary_center, 5.4f });          // solid stepped ziggurat
    if (g_level == LEVEL_WRECK)
        obstacles.push_back({ boundary_center, 4.2f });          // great central shipwreck hull
    if (g_level == LEVEL_SANCTUM)
        obstacles.push_back({ boundary_center, 2.8f });          // central monument
    if (g_level == LEVEL_CLOCK)
        obstacles.push_back({ boundary_center, 3.0f });          // central engine core
    if (g_level == LEVEL_SHRINE)
        obstacles.push_back({ boundary_center, 3.6f });          // central shrine pavilion
    if (g_level == LEVEL_BONES)
        obstacles.push_back({ boundary_center, 1.6f });          // central ribcage spine
    if (g_level == LEVEL_GEODE)
        obstacles.push_back({ boundary_center, 3.4f });          // giant central geode
    if (g_level == LEVEL_TEMPLE)
        obstacles.push_back({ boundary_center, 3.4f });          // ruined central temple
    if (g_level == LEVEL_MINE)
        obstacles.push_back({ boundary_center, 2.8f });          // central pit-head winch tower
    if (g_level == LEVEL_OBS)
        obstacles.push_back({ boundary_center, 3.0f });          // central armillary sphere
    if (g_level == LEVEL_LIB)
        obstacles.push_back({ boundary_center, 3.2f });          // central reading rotunda
    if (g_level == LEVEL_CAMP)
        obstacles.push_back({ boundary_center, 3.4f });          // central command pavilion
    if (g_level == LEVEL_AQUA) {                                 // grand central arch's two piers (channel stays open)
        obstacles.push_back({ boundary_center + Vector3{ -4.5f, 0, 0 }, 1.3f });
        obstacles.push_back({ boundary_center + Vector3{  4.5f, 0, 0 }, 1.3f });
    }
    if (g_level == LEVEL_COURT)
        obstacles.push_back({ boundary_center, 3.4f });          // colossal central sentinel
    if (g_level == LEVEL_GARDEN)
        obstacles.push_back({ boundary_center, 3.0f });          // central tiered fountain
    if (g_level == LEVEL_BRIDGE) {                               // midspan gate-arch piers (deck stays open between)
        obstacles.push_back({ boundary_center + Vector3{ -5.0f, 0, 0 }, 1.3f });
        obstacles.push_back({ boundary_center + Vector3{  5.0f, 0, 0 }, 1.3f });
    }
    if (g_level == LEVEL_BEACON)
        obstacles.push_back({ boundary_center, 3.0f });          // lighthouse tower base
    if (g_level == LEVEL_MILL)
        obstacles.push_back({ boundary_center, 3.0f });          // central windmill tower
    if (g_level == LEVEL_OSSUARY)
        obstacles.push_back({ boundary_center, 2.6f });          // central skull reliquary
    if (g_level == LEVEL_FAIR)
        obstacles.push_back({ boundary_center, 4.5f });          // central carousel
    if (g_level == LEVEL_THRONE)
        obstacles.push_back({ boundary_center, 3.5f });          // central throne dais
    if (g_level == LEVEL_SPRINGS)
        obstacles.push_back({ boundary_center, 2.5f });          // central geyser mound
    if (g_level == LEVEL_PFOREST)
        obstacles.push_back({ boundary_center, 3.0f });          // colossal fallen petrified log
    if (g_level == LEVEL_HAMLET)
        obstacles.push_back({ boundary_center, 1.6f });          // central village well
    if (g_level == LEVEL_HENGE)
        obstacles.push_back({ boundary_center, 2.2f });          // central altar stone
    if (g_level == LEVEL_BOG)
        obstacles.push_back({ boundary_center, 2.0f });          // central great mossy stump
    if (g_level == LEVEL_SAWMILL)
        obstacles.push_back({ boundary_center, 2.6f });          // central log stack
    if (g_level == LEVEL_GAOL)
        obstacles.push_back({ boundary_center, 2.2f });          // central gallows scaffold
    if (g_level == LEVEL_TAR)
        obstacles.push_back({ boundary_center, 2.4f });          // central tar geyser mound
    if (g_level == LEVEL_VINE)
        obstacles.push_back({ boundary_center, 2.2f });          // central wine press
    if (g_level == LEVEL_FLOE)
        obstacles.push_back({ boundary_center, 2.6f });          // central great iceberg
    if (g_level == LEVEL_QUARRY)
        obstacles.push_back({ boundary_center, 2.4f });          // central great cut block
    if (g_level == LEVEL_DOCK)
        obstacles.push_back({ boundary_center, 1.8f });          // central mooring mast
    if (g_level == LEVEL_GLASS)
        obstacles.push_back({ boundary_center, 1.6f });          // central giant palm
    if (g_level == LEVEL_APIARY)
        obstacles.push_back({ boundary_center, 2.0f });          // central hollow bee-tree
    if (g_level == LEVEL_KILN)
        obstacles.push_back({ boundary_center, 2.8f });          // central great bottle-kiln
    if (g_level == LEVEL_REEF)
        obstacles.push_back({ boundary_center, 2.6f });          // central great coral head
    if (g_level == LEVEL_FOUNDRY)
        obstacles.push_back({ boundary_center, 2.8f });          // central blast furnace
    if (g_level == LEVEL_NAVE)
        obstacles.push_back({ boundary_center, 1.4f });          // central baptismal font
    if (g_level == LEVEL_WATERMILL)
        obstacles.push_back({ boundary_center, 1.6f });          // central millstone display
    if (g_level == LEVEL_SALT)
        obstacles.push_back({ boundary_center, 1.8f });          // central great salt pile
    if (g_level == LEVEL_STILT)
        obstacles.push_back({ boundary_center, 2.4f });          // central stilt longhouse
    if (g_level == LEVEL_HIVE)
        obstacles.push_back({ boundary_center, 2.8f });          // central great paper nest
    if (g_level == LEVEL_BREW)
        obstacles.push_back({ boundary_center, 2.2f });          // central copper still
    if (g_level == LEVEL_BAMBOO)
        obstacles.push_back({ boundary_center, 1.8f });          // central zen rock arrangement
    if (g_level == LEVEL_COLLIER)
        obstacles.push_back({ boundary_center, 2.8f });          // central great charcoal mound
    if (g_level == LEVEL_PLAZA)
        obstacles.push_back({ boundary_center, 3.2f });          // central grand fountain
    if (g_level == LEVEL_PUMPKIN)
        obstacles.push_back({ boundary_center, 2.0f });          // central great pumpkin
    if (g_level == LEVEL_BELL)
        obstacles.push_back({ boundary_center, 2.4f });          // central great bell frame
    if (g_level == LEVEL_AVIARY)
        obstacles.push_back({ boundary_center, 1.8f });          // central great nest platform
    if (g_level == LEVEL_WEB)
        obstacles.push_back({ boundary_center, 1.6f });          // central web hub spire
    if (g_level == LEVEL_LOTUS)
        obstacles.push_back({ boundary_center, 2.6f });          // central water pavilion
    if (g_level == LEVEL_ARCHTREE)
        obstacles.push_back({ boundary_center, 3.2f });          // colossal central tree trunk
    if (g_level == LEVEL_CHESS)
        obstacles.push_back({ boundary_center, 2.0f });          // colossal central king piece
    if (g_level == LEVEL_MAZE)
        obstacles.push_back({ boundary_center, 2.2f });          // central grand topiary
    if (g_level == LEVEL_FALLS)
        obstacles.push_back({ boundary_center, 2.4f });          // central midstream great rock
    if (g_level == LEVEL_CRATER)
        obstacles.push_back({ boundary_center, 3.0f });          // colossal central fallen star
    if (g_level == LEVEL_TITAN)
        obstacles.push_back({ boundary_center, 2.8f });          // colossal central reaching hand
    if (g_level == LEVEL_HOODOO)
        obstacles.push_back({ boundary_center, 2.0f });          // central great hoodoo tower
    if (g_level == LEVEL_MOAI)
        obstacles.push_back({ boundary_center, 1.6f });          // central great moai
    if (g_level == LEVEL_CAVERN)
        obstacles.push_back({ boundary_center, 2.2f });          // central great dripstone column
    if (g_level == LEVEL_PINES)
        obstacles.push_back({ boundary_center, 1.2f });          // central great pine
    if (g_level == LEVEL_GALLEON)
        obstacles.push_back({ boundary_center, 1.0f });          // central mainmast
    if (g_level == LEVEL_SUNFLOWER)
        obstacles.push_back({ boundary_center, 0.8f });          // central great sunflower
    if (g_level == LEVEL_SPHINX)
        obstacles.push_back({ boundary_center, 2.6f });          // colossal central sphinx body
    if (g_level == LEVEL_DAM)
        obstacles.push_back({ boundary_center, 1.8f });          // central outlet-works valve house
    if (g_level == LEVEL_THEATRE)
        obstacles.push_back({ boundary_center, 1.4f });          // central orchestra altar (thymele)
    if (g_level == LEVEL_WHEEL)
        obstacles.push_back({ boundary_center, 2.0f });          // central wheel foundation pylon
    if (g_level == LEVEL_REDWOOD)
        obstacles.push_back({ boundary_center, 2.6f });          // central colossal redwood trunk
    if (g_level == LEVEL_BALLOON)
        obstacles.push_back({ boundary_center, 1.6f });          // central balloon basket
    if (g_level == LEVEL_CANAL)
        obstacles.push_back({ boundary_center, 1.6f });          // central mooring cluster
    if (g_level == LEVEL_SILO)
        obstacles.push_back({ boundary_center, 2.6f });          // central grain silo
    if (g_level == LEVEL_ORGAN)
        obstacles.push_back({ boundary_center, 2.2f });          // central organ console
    if (g_level == LEVEL_HYPO)
        obstacles.push_back({ boundary_center, 1.8f });          // central altar
    if (g_level == LEVEL_FORT)
        obstacles.push_back({ boundary_center, 1.4f });          // central flagpole / well
    if (g_level == LEVEL_TRIUMPH)
        obstacles.push_back({ boundary_center, 1.4f });          // central trophy (tropaeum)
    if (g_level == LEVEL_ORCHARD)
        obstacles.push_back({ boundary_center, 1.4f });          // central great blossom tree
    if (g_level == LEVEL_LOOM)
        obstacles.push_back({ boundary_center, 1.4f });          // central yarn-winding swift
    if (g_level == LEVEL_SAVANNA)
        obstacles.push_back({ boundary_center, 3.2f });          // colossal central baobab trunk
    if (g_level == LEVEL_MOSQUE)
        obstacles.push_back({ boundary_center, 2.0f });          // central ablution fountain
    if (g_level == LEVEL_TOTEM)
        obstacles.push_back({ boundary_center, 1.4f });          // central great totem pole
    if (g_level == LEVEL_OASIS)
        obstacles.push_back({ boundary_center, 1.8f });          // central spring-source rock
    if (g_level == LEVEL_PAGODA)
        obstacles.push_back({ boundary_center, 4.5f });          // base of the great pagoda
    // (stepwell builds its own confining ring + pavilion obstacles in build_stepwell)

    build_crystals();
    if (g_level == LEVEL_GRAVEYARD) build_graveyard();
    if (g_level == LEVEL_FUNGAL) build_fungal();
    if (g_level == LEVEL_DESERT) build_desert();
    if (g_level == LEVEL_WRECK) build_wreck();
    if (g_level == LEVEL_SANCTUM) build_sanctum();
    if (g_level == LEVEL_CLOCK) build_clock();
    if (g_level == LEVEL_SHRINE) build_shrine();
    if (g_level == LEVEL_BONES) build_bones();
    if (g_level == LEVEL_GEODE) build_geode();
    if (g_level == LEVEL_TEMPLE) build_temple();
    if (g_level == LEVEL_MINE) build_mine();
    if (g_level == LEVEL_OBS) build_obs();
    if (g_level == LEVEL_LIB) build_lib();
    if (g_level == LEVEL_CAMP) build_camp();
    if (g_level == LEVEL_AQUA) build_aqua();
    if (g_level == LEVEL_COURT) build_court();
    if (g_level == LEVEL_GARDEN) build_garden();
    if (g_level == LEVEL_BRIDGE) build_bridge();
    if (g_level == LEVEL_BEACON) build_beacon();
    if (g_level == LEVEL_MILL) build_mill();
    if (g_level == LEVEL_OSSUARY) build_ossuary();
    if (g_level == LEVEL_FAIR) build_fair();
    if (g_level == LEVEL_THRONE) build_throne();
    if (g_level == LEVEL_SPRINGS) build_springs();
    if (g_level == LEVEL_PFOREST) build_pforest();
    if (g_level == LEVEL_HAMLET) build_hamlet();
    if (g_level == LEVEL_HENGE) build_henge();
    if (g_level == LEVEL_BOG) build_bog();
    if (g_level == LEVEL_SAWMILL) build_lumber();
    if (g_level == LEVEL_GAOL) build_gaol();
    if (g_level == LEVEL_TAR) build_tar();
    if (g_level == LEVEL_VINE) build_vine();
    if (g_level == LEVEL_FLOE) build_floes();
    if (g_level == LEVEL_QUARRY) build_quarry();
    if (g_level == LEVEL_DOCK) build_dock();
    if (g_level == LEVEL_GLASS) build_glass();
    if (g_level == LEVEL_APIARY) build_apiary();
    if (g_level == LEVEL_KILN) build_kiln();
    if (g_level == LEVEL_REEF) build_reef();
    if (g_level == LEVEL_FOUNDRY) build_foundry();
    if (g_level == LEVEL_NAVE) build_nave();
    if (g_level == LEVEL_WATERMILL) build_watermill();
    if (g_level == LEVEL_SALT) build_salt();
    if (g_level == LEVEL_STILT) build_stilt();
    if (g_level == LEVEL_HIVE) build_hive();
    if (g_level == LEVEL_BREW) build_brew();
    if (g_level == LEVEL_BAMBOO) build_bamboo();
    if (g_level == LEVEL_COLLIER) build_collier();
    if (g_level == LEVEL_PLAZA) build_plaza();
    if (g_level == LEVEL_PUMPKIN) build_pumpkin();
    if (g_level == LEVEL_BELL) build_bell();
    if (g_level == LEVEL_AVIARY) build_aviary();
    if (g_level == LEVEL_WEB) build_web();
    if (g_level == LEVEL_LOTUS) build_lotus();
    if (g_level == LEVEL_ARCHTREE) build_archtree();
    if (g_level == LEVEL_CHESS) build_chess();
    if (g_level == LEVEL_MAZE) build_maze();
    if (g_level == LEVEL_FALLS) build_falls();
    if (g_level == LEVEL_CRATER) build_crater();
    if (g_level == LEVEL_TITAN) build_titan();
    if (g_level == LEVEL_HOODOO) build_hoodoo();
    if (g_level == LEVEL_MOAI) build_moai();
    if (g_level == LEVEL_CAVERN) build_cavern();
    if (g_level == LEVEL_PINES) build_pines();
    if (g_level == LEVEL_GALLEON) build_galleon();
    if (g_level == LEVEL_SUNFLOWER) build_sunflower();
    if (g_level == LEVEL_SPHINX) build_sphinx();
    if (g_level == LEVEL_DAM) build_dam();
    if (g_level == LEVEL_THEATRE) build_theatre();
    if (g_level == LEVEL_WHEEL) build_wheel();
    if (g_level == LEVEL_REDWOOD) build_redwood();
    if (g_level == LEVEL_BALLOON) build_balloon();
    if (g_level == LEVEL_CANAL) build_canal();
    if (g_level == LEVEL_SILO) build_silo();
    if (g_level == LEVEL_ORGAN) build_organ();
    if (g_level == LEVEL_HYPO) build_hypo();
    if (g_level == LEVEL_FORT) build_fort();
    if (g_level == LEVEL_TRIUMPH) build_triumph();
    if (g_level == LEVEL_ORCHARD) build_orchard();
    if (g_level == LEVEL_LOOM) build_loom();
    if (g_level == LEVEL_SAVANNA) build_savanna();
    if (g_level == LEVEL_MOSQUE) build_mosque();
    if (g_level == LEVEL_TOTEM) build_totem();
    if (g_level == LEVEL_OASIS) build_oasis();
    if (g_level == LEVEL_PAGODA) build_pagoda();
    if (g_level == LEVEL_STEPWELL) build_stepwell();
    if (g_level == LEVEL_JANTAR) build_jantar();
    if (g_level == LEVEL_TERRACOTTA) build_terracotta();
    if (g_level == LEVEL_BALLCOURT) build_ballcourt();
    if (g_level == LEVEL_PETRA) build_petra();
    if (g_level == LEVEL_BAZAAR) build_bazaar();
    if (g_level == LEVEL_SIEGE) build_siege();
    if (g_level == LEVEL_WHALING) build_whaling();
    if (g_level == LEVEL_CACTUS) build_cactus();
    if (g_level == LEVEL_ZIMBABWE) build_zimbabwe();
    if (g_level == LEVEL_BASIL) build_basil();
    if (g_level == LEVEL_PRINT) build_print();
    if (g_level == LEVEL_GER) build_ger();
    if (g_level == LEVEL_ALCHEMY) build_alchemy();
    if (g_level == LEVEL_BATHS) build_baths();
    if (g_level == LEVEL_FLOAT) build_float();
    if (g_level == LEVEL_ISHTAR) build_ishtar();
    if (g_level == LEVEL_TANNERY) build_tannery();
    if (g_level == LEVEL_MUSEUM) build_museum();
    if (g_level == LEVEL_GOMPA) build_gompa();
    if (g_level == LEVEL_BUDDHA) build_buddha();
    if (g_level == LEVEL_ANGKOR) build_angkor();
    if (g_level == LEVEL_BADGIR) build_badgir();
    if (g_level == LEVEL_TOPIARY) build_topiary();
    if (g_level == LEVEL_GLASSWORKS) build_glassworks();
    if (g_level == LEVEL_SHIPYARD) build_shipyard();
    if (g_level == LEVEL_GOTHIC) build_gothic();
    if (g_level == LEVEL_PUEBLO) build_pueblo();

    // upload the static crystal lights to the water shader (positions never move)
    int cn = (int)s_crystal_lights.size();
    SetShaderValue(s_water_shader, s_loc_cry_n, &cn, SHADER_UNIFORM_INT);
    if (cn > 0)
        SetShaderValueV(s_water_shader, s_loc_cry_arr, s_crystal_lights.data(),
                        SHADER_UNIFORM_VEC4, cn);
    // same crystal lights illuminate the rocks/fighters via the lit shader
    Vector3 pcol_by[117] = { { 0.85f, 0.14f, 0.10f },    // blood: red
                           { 0.26f, 0.55f, 0.85f },    // frozen: ice-blue
                           { 0.95f, 0.42f, 0.10f },    // forge: ember-orange
                           { 0.95f, 0.50f, 0.18f },    // colosseum: brazier fire
                           { 0.40f, 0.95f, 0.55f },    // graveyard: will-o'-wisp green
                           { 0.30f, 0.92f, 0.85f },    // fungal: bioluminescent teal
                           { 0.95f, 0.66f, 0.30f },    // desert: warm brazier torchlight
                           { 0.95f, 0.72f, 0.38f },    // cove: warm lantern light
                           { 0.80f, 0.88f, 1.00f },    // sanctum: pale soul-light
                           { 0.98f, 0.62f, 0.22f },    // clock: warm furnace glow
                           { 0.98f, 0.74f, 0.42f },    // shrine: warm paper-lantern light
                           { 0.66f, 0.86f, 0.40f },    // boneyard: sickly corpse-light
                           { 0.62f, 0.46f, 0.96f },    // cavern: amethyst crystal glow
                           { 0.80f, 0.92f, 0.45f },    // temple: warm firefly glow
                           { 0.98f, 0.70f, 0.30f },    // mine: gold ore / lantern glow
                           { 0.92f, 0.86f, 0.66f },    // observatory: warm sun-orb / globe glow
                           { 0.98f, 0.78f, 0.46f },    // archive: warm candlelight
                           { 0.98f, 0.60f, 0.25f },    // camp: warm campfire glow
                           { 0.40f, 0.85f, 0.55f },    // aqueduct: green algae glow
                           { 0.95f, 0.62f, 0.28f },    // court: warm brazier glow
                           { 0.92f, 0.82f, 0.60f },    // garden: warm sunlit fill
                           { 0.95f, 0.72f, 0.40f },    // bridge: warm lantern glow
                           { 0.98f, 0.86f, 0.58f },    // beacon: warm lighthouse lamp
                           { 0.95f, 0.85f, 0.58f },    // mill: warm barn-window glow
                           { 0.98f, 0.72f, 0.38f },    // ossuary: warm candle glow
                           { 0.95f, 0.70f, 0.55f },    // fairground: warm string-light glow
                           { 0.98f, 0.80f, 0.45f },    // throne: warm regal sconce
                           { 0.85f, 0.82f, 0.70f },    // springs: warm mineral fill
                           { 0.90f, 0.60f, 0.30f },    // petrified: amber resin glow
                           { 0.70f, 0.82f, 0.42f },    // hamlet: sickly plague-lantern green
                           { 0.45f, 0.85f, 0.92f },    // henge: pale ley-line cyan
                           { 0.48f, 0.78f, 0.42f },    // bog: green will-o'-wisp / swamp gas
                           { 0.95f, 0.74f, 0.42f },    // sawmill: warm work-lamp amber
                           { 1.00f, 0.58f, 0.24f },    // gaol: orange torch / brazier
                           { 0.95f, 0.66f, 0.30f },    // tar: amber-sulfur seep glow
                           { 1.00f, 0.82f, 0.50f },    // vineyard: warm golden-hour lamp
                           { 0.60f, 0.85f, 1.00f },    // floes: pale ice-cyan / aurora
                           { 1.00f, 0.80f, 0.50f },    // quarry: warm work-lamp amber
                           { 1.00f, 0.78f, 0.45f },    // skydock: warm signal-lamp glow
                           { 0.72f, 0.96f, 0.66f },    // conservatory: sun-through-glass green-gold
                           { 1.00f, 0.82f, 0.40f },    // apiary: warm honey-gold
                           { 1.00f, 0.55f, 0.20f },    // kiln: hot kiln-fire orange
                           { 0.40f, 0.80f, 0.92f },    // reef: bioluminescent cyan
                           { 1.00f, 0.50f, 0.15f },    // foundry: molten-iron orange
                           { 1.00f, 0.72f, 0.42f },    // nave: warm candle / stained-glass
                           { 1.00f, 0.86f, 0.56f },    // watermill: warm pastoral lantern
                           { 1.00f, 0.92f, 0.86f },    // salt: pale salt-white / brine sheen
                           { 1.00f, 0.80f, 0.50f },    // stilt: warm fisher-lantern glow
                           { 1.00f, 0.65f, 0.25f },    // hive: amber wax glow
                           { 1.00f, 0.68f, 0.36f },    // brewery: warm copper / furnace glow
                           { 0.60f, 0.92f, 0.50f },    // bamboo: green dappled / lantern
                           { 1.00f, 0.50f, 0.20f },    // collier: ember-red charcoal glow
                           { 1.00f, 0.85f, 0.60f },    // plaza: warm civic lamp / sun
                           { 1.00f, 0.55f, 0.15f },    // pumpkin: jack-o'-lantern orange
                           { 1.00f, 0.70f, 0.35f },    // bell: warm bronze glow
                           { 0.90f, 0.80f, 0.58f },    // aviary: warm dusty cage light
                           { 0.70f, 0.85f, 0.70f },    // web: sickly pale silk glow
                           { 1.00f, 0.74f, 0.64f },    // lotus: warm lantern / blossom pink
                           { 0.78f, 0.95f, 0.52f },    // archtree: sacred green-gold sap glow
                           { 0.70f, 0.78f, 0.95f },    // chess: cool royal blue-white
                           { 1.00f, 0.86f, 0.58f },    // maze: warm garden-lamp glow
                           { 0.62f, 0.85f, 0.95f },    // falls: cool aqua spray glow
                           { 0.80f, 0.45f, 1.00f },    // crater: eerie violet fallen-star glow
                           { 0.88f, 0.74f, 0.52f },    // titan: warm dusty amber dusk
                           { 1.00f, 0.62f, 0.34f },    // hoodoo: warm sandstone orange
                           { 0.72f, 0.78f, 0.82f },    // moai: cool coastal grey-white
                           { 0.55f, 0.80f, 0.85f },    // cavern: cool teal dripstone glow
                           { 0.70f, 0.82f, 0.95f },    // pines: cold blue-white snow glow
                           { 1.00f, 0.75f, 0.40f },    // galleon: warm deck-lantern glow
                           { 1.00f, 0.85f, 0.40f },    // sunflower: warm golden sun
                           { 1.00f, 0.82f, 0.46f },    // sphinx: warm dawn-gold sandstone
                           { 0.66f, 0.84f, 0.92f },    // dam: cool spray / water glow
                           { 1.00f, 0.86f, 0.56f },    // theatre: warm marble sun / sconce
                           { 1.00f, 0.72f, 0.42f },    // wheel: warm festive string-light glow
                           { 0.80f, 0.95f, 0.60f },    // redwood: soft green-gold grove light
                           { 1.00f, 0.70f, 0.35f },    // balloon: warm burner-flame glow
                           { 1.00f, 0.84f, 0.54f },    // canal: warm Venetian lamp / sun
                           { 1.00f, 0.85f, 0.50f },    // silo: warm grain / dusty sun
                           { 1.00f, 0.80f, 0.50f },    // organ: warm candle / gilt glow
                           { 1.00f, 0.82f, 0.45f },    // hypostyle: warm sandstone / light-shaft
                           { 1.00f, 0.66f, 0.34f },    // fort: warm cannon-fire / muzzle flash
                           { 1.00f, 0.84f, 0.50f },    // triumph: warm imperial gold / brazier
                           { 1.00f, 0.82f, 0.82f },    // orchard: soft warm blossom-pink
                           { 1.00f, 0.82f, 0.55f },    // loom: warm workshop lamp
                           { 1.00f, 0.80f, 0.42f },    // savanna: warm golden sun
                           { 1.00f, 0.85f, 0.55f },    // mosque: warm gold / lamp
                           { 1.00f, 0.58f, 0.30f },    // totem: warm fire-pit glow
                           { 1.00f, 0.80f, 0.42f },    // oasis: warm desert sun
                           { 1.00f, 0.72f, 0.40f },    // pagoda: warm lantern glow
                           { 1.00f, 0.66f, 0.34f },    // stepwell: warm torch / sandstone
                           { 1.00f, 0.80f, 0.46f },    // jantar: warm midday stone
                           { 1.00f, 0.74f, 0.42f },    // terracotta: warm dusty light
                           { 1.00f, 0.78f, 0.48f },    // ballcourt: warm tropical limestone
                           { 1.00f, 0.66f, 0.40f },    // petra: warm torch on rose rock
                           { 1.00f, 0.70f, 0.36f },    // bazaar: warm lantern light
                           { 1.00f, 0.56f, 0.28f },    // siege: warm brazier fire
                           { 1.00f, 0.52f, 0.24f },    // whaling: warm try-pot fire
                           { 1.00f, 0.78f, 0.44f },    // cactus: warm desert sun
                           { 1.00f, 0.74f, 0.42f },    // zimbabwe: warm afternoon granite
                           { 1.00f, 0.80f, 0.50f },    // basil: warm candle-window glow
                           { 1.00f, 0.74f, 0.40f },    // print: warm workshop lamp
                           { 1.00f, 0.62f, 0.30f },    // ger: warm steppe campfire
                           { 0.45f, 0.92f, 0.55f },    // alchemy: eerie green elixir glow
                           { 1.00f, 0.66f, 0.34f },    // baths: warm brazier on marble
                           { 1.00f, 0.70f, 0.36f },    // float: warm lantern over the river
                           { 1.00f, 0.74f, 0.42f },    // ishtar: warm sun on glazed brick
                           { 1.00f, 0.78f, 0.46f },    // tannery: hot sun on earth
                           { 1.00f, 0.82f, 0.56f },    // museum: soft warm gallery light
                           { 1.00f, 0.74f, 0.40f },    // gompa: warm butter-lamp
                           { 1.00f, 0.74f, 0.42f },    // buddha: warm gilded incense glow
                           { 1.00f, 0.74f, 0.42f },    // angkor: warm sun on sandstone
                           { 1.00f, 0.78f, 0.46f },    // badgir: hot sun on adobe
                           { 1.00f, 0.84f, 0.56f },    // topiary: warm garden sun
                           { 1.00f, 0.50f, 0.22f },    // glassworks: hot furnace fire
                           { 1.00f, 0.62f, 0.30f },    // shipyard: warm tar-fire / brazier glow
                           { 1.00f, 0.74f, 0.42f },    // gothic: warm stained-glass candlelight
                           { 1.00f, 0.58f, 0.26f } };  // pueblo: warm kiva-fire glow
    g_lit.set_point_lights(s_crystal_lights.data(), cn, pcol_by[g_level]);
}

void update(float dt) { s_time += dt; }

// ---------------------------------------------------------------- draw
static void draw_crystals();
static void draw_level_props();
static void draw_colosseum();
static void build_graveyard();
static void draw_graveyard();
static void draw_fungal();
static void draw_desert();
static void draw_wreck();
static void draw_sanctum();
static void draw_clock();
static void draw_shrine();
static void draw_bones();
static void draw_geode();
static void draw_temple();
static void draw_mine();
static void draw_obs();
static void draw_lib();
static void draw_camp();
static void draw_aqua();
static void draw_court();
static void draw_garden();
static void draw_bridge();
static void draw_beacon();
static void draw_mill();
static void draw_ossuary();
static void draw_fair();
static void draw_throne();
static void draw_springs();
static void draw_pforest();
static void draw_hamlet();
static void draw_henge();
static void draw_bog();
static void draw_lumber();
static void draw_gaol();
static void draw_tar();
static void draw_vine();
static void draw_floes();
static void draw_quarry();
static void draw_dock();
static void draw_glass();
static void draw_apiary();
static void draw_kiln();
static void draw_reef();
static void draw_foundry();
static void draw_nave();
static void draw_watermill();
static void draw_salt();
static void draw_stilt();
static void draw_hive();
static void draw_brew();
static void draw_bamboo();
static void draw_collier();
static void draw_plaza();
static void draw_pumpkin();
static void draw_bellyard();
static void draw_aviary();
static void draw_web_lair();
static void draw_lotus_pond();
static void draw_archtree();
static void draw_chess();
static void draw_maze();
static void draw_falls();
static void draw_crater();
static void draw_titan();
static void draw_hoodoos();
static void draw_moai_isle();
static void draw_cavern();
static void draw_pines();
static void draw_galleon();
static void draw_sunflower_field();
static void draw_sphinx_necropolis();
static void draw_dam();
static void draw_theatre();
static void draw_wheel();
static void draw_redwood_grove();
static void draw_balloon_field();
static void draw_canal();
static void draw_silo_yard();
static void draw_organ();
static void draw_hypo();
static void draw_fort();
static void draw_triumph();
static void draw_orchard();
static void draw_loom();
static void draw_savanna();
static void draw_mosque();
static void draw_totem_village();
static void draw_oasis();
static void draw_pagoda();
static void draw_stepwell();
static void draw_jantar();
static void draw_terracotta();
static void draw_ballcourt();
static void draw_petra();
static void draw_bazaar();
static void draw_siege();
static void draw_whaling();
static void draw_cactus();
static void draw_zimbabwe();
static void draw_basil();
static void draw_print();
static void draw_ger();
static void draw_alchemy();
static void draw_baths();
static void draw_float();
static void draw_ishtar();
static void draw_tannery();
static void draw_museum();
static void draw_gompa();
static void draw_buddha();
static void draw_angkor();
static void draw_badgir();
static void draw_topiary();
static void draw_glassworks();
static void draw_shipyard();
static void draw_gothic();
static void draw_pueblo();

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
    if (s_has_moon && (g_level == LEVEL_BLOODMOON || g_level == LEVEL_FROZEN || g_level == LEVEL_GRAVEYARD || g_level == LEVEL_WRECK || g_level == LEVEL_OBS || g_level == LEVEL_COURT))
        DrawModelEx(s_moon, MOON_POS, Vector3{ 1, 0, 0 }, 90.0f, Vector3{ 1, 1, 1 }, WHITE);
    if (g_level == LEVEL_COLOSSEUM) {
        draw_colosseum();                 // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_GRAVEYARD) {
        draw_graveyard();                 // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_FUNGAL) {
        draw_fungal();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_DESERT) {
        draw_desert();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_WRECK) {
        draw_wreck();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_SANCTUM) {
        draw_sanctum();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_CLOCK) {
        draw_clock();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_SHRINE) {
        draw_shrine();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BONES) {
        draw_bones();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_GEODE) {
        draw_geode();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_TEMPLE) {
        draw_temple();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_MINE) {
        draw_mine();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_OBS) {
        draw_obs();                       // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_LIB) {
        draw_lib();                       // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_CAMP) {
        draw_camp();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_AQUA) {
        draw_aqua();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_COURT) {
        draw_court();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_GARDEN) {
        draw_garden();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BRIDGE) {
        draw_bridge();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BEACON) {
        draw_beacon();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_MILL) {
        draw_mill();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_OSSUARY) {
        draw_ossuary();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_FAIR) {
        draw_fair();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_THRONE) {
        draw_throne();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_SPRINGS) {
        draw_springs();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_PFOREST) {
        draw_pforest();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_HAMLET) {
        draw_hamlet();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_HENGE) {
        draw_henge();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BOG) {
        draw_bog();                       // its own procedural geometry over the murky water plane
    } else if (g_level == LEVEL_SAWMILL) {
        draw_lumber();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_GAOL) {
        draw_gaol();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_TAR) {
        draw_tar();                       // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_VINE) {
        draw_vine();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_FLOE) {
        draw_floes();                     // its own procedural geometry over the icy water plane
    } else if (g_level == LEVEL_QUARRY) {
        draw_quarry();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_DOCK) {
        draw_dock();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_GLASS) {
        draw_glass();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_APIARY) {
        draw_apiary();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_KILN) {
        draw_kiln();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_REEF) {
        draw_reef();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_FOUNDRY) {
        draw_foundry();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_NAVE) {
        draw_nave();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_WATERMILL) {
        draw_watermill();                 // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_SALT) {
        draw_salt();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_STILT) {
        draw_stilt();                     // its own procedural geometry over the lagoon water plane
    } else if (g_level == LEVEL_HIVE) {
        draw_hive();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BREW) {
        draw_brew();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BAMBOO) {
        draw_bamboo();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_COLLIER) {
        draw_collier();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_PLAZA) {
        draw_plaza();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_PUMPKIN) {
        draw_pumpkin();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BELL) {
        draw_bellyard();                  // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_AVIARY) {
        draw_aviary();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_WEB) {
        draw_web_lair();                  // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_LOTUS) {
        draw_lotus_pond();                // its own procedural geometry over the pond water plane
    } else if (g_level == LEVEL_ARCHTREE) {
        draw_archtree();                  // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_CHESS) {
        draw_chess();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_MAZE) {
        draw_maze();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_FALLS) {
        draw_falls();                     // its own procedural geometry over the plunge-pool water plane
    } else if (g_level == LEVEL_CRATER) {
        draw_crater();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_TITAN) {
        draw_titan();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_HOODOO) {
        draw_hoodoos();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_MOAI) {
        draw_moai_isle();                 // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_CAVERN) {
        draw_cavern();                    // its own procedural geometry over the underground pool
    } else if (g_level == LEVEL_PINES) {
        draw_pines();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_GALLEON) {
        draw_galleon();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_SUNFLOWER) {
        draw_sunflower_field();           // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_SPHINX) {
        draw_sphinx_necropolis();         // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_DAM) {
        draw_dam();                       // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_THEATRE) {
        draw_theatre();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_WHEEL) {
        draw_wheel();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_REDWOOD) {
        draw_redwood_grove();             // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BALLOON) {
        draw_balloon_field();             // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_CANAL) {
        draw_canal();                     // its own procedural geometry over the canal water plane
    } else if (g_level == LEVEL_SILO) {
        draw_silo_yard();                 // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_ORGAN) {
        draw_organ();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_HYPO) {
        draw_hypo();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_FORT) {
        draw_fort();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_TRIUMPH) {
        draw_triumph();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_ORCHARD) {
        draw_orchard();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_LOOM) {
        draw_loom();                      // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_SAVANNA) {
        draw_savanna();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_MOSQUE) {
        draw_mosque();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_TOTEM) {
        draw_totem_village();             // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_OASIS) {
        draw_oasis();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_PAGODA) {
        draw_pagoda();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_STEPWELL) {
        draw_stepwell();                  // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_JANTAR) {
        draw_jantar();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_TERRACOTTA) {
        draw_terracotta();                // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BALLCOURT) {
        draw_ballcourt();                 // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_PETRA) {
        draw_petra();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BAZAAR) {
        draw_bazaar();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_SIEGE) {
        draw_siege();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_WHALING) {
        draw_whaling();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_CACTUS) {
        draw_cactus();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_ZIMBABWE) {
        draw_zimbabwe();                  // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BASIL) {
        draw_basil();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_PRINT) {
        draw_print();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_GER) {
        draw_ger();                       // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_ALCHEMY) {
        draw_alchemy();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BATHS) {
        draw_baths();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_FLOAT) {
        draw_float();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_ISHTAR) {
        draw_ishtar();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_TANNERY) {
        draw_tannery();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_MUSEUM) {
        draw_museum();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_GOMPA) {
        draw_gompa();                     // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BUDDHA) {
        draw_buddha();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_ANGKOR) {
        draw_angkor();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_BADGIR) {
        draw_badgir();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_TOPIARY) {
        draw_topiary();                   // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_GLASSWORKS) {
        draw_glassworks();                // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_SHIPYARD) {
        draw_shipyard();                  // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_GOTHIC) {
        draw_gothic();                    // its own procedural geometry (no boss_arena.glb)
    } else if (g_level == LEVEL_PUEBLO) {
        draw_pueblo();                    // its own procedural geometry (no boss_arena.glb)
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
    if (g_level == LEVEL_COLOSSEUM || g_level == LEVEL_GRAVEYARD || g_level == LEVEL_FUNGAL || g_level == LEVEL_DESERT || g_level == LEVEL_WRECK || g_level == LEVEL_SANCTUM || g_level == LEVEL_CLOCK || g_level == LEVEL_SHRINE || g_level == LEVEL_BONES || g_level == LEVEL_GEODE || g_level == LEVEL_TEMPLE || g_level == LEVEL_MINE || g_level == LEVEL_OBS || g_level == LEVEL_LIB || g_level == LEVEL_CAMP || g_level == LEVEL_AQUA || g_level == LEVEL_COURT || g_level == LEVEL_GARDEN || g_level == LEVEL_BRIDGE || g_level == LEVEL_BEACON || g_level == LEVEL_MILL || g_level == LEVEL_OSSUARY || g_level == LEVEL_FAIR || g_level == LEVEL_THRONE || g_level == LEVEL_SPRINGS || g_level == LEVEL_PFOREST || g_level == LEVEL_HAMLET || g_level == LEVEL_HENGE || g_level == LEVEL_BOG || g_level == LEVEL_SAWMILL || g_level == LEVEL_GAOL || g_level == LEVEL_TAR || g_level == LEVEL_VINE || g_level == LEVEL_FLOE || g_level == LEVEL_QUARRY || g_level == LEVEL_DOCK || g_level == LEVEL_GLASS || g_level == LEVEL_APIARY || g_level == LEVEL_KILN || g_level == LEVEL_REEF || g_level == LEVEL_FOUNDRY || g_level == LEVEL_NAVE || g_level == LEVEL_WATERMILL || g_level == LEVEL_SALT || g_level == LEVEL_STILT || g_level == LEVEL_HIVE || g_level == LEVEL_BREW || g_level == LEVEL_BAMBOO || g_level == LEVEL_COLLIER || g_level == LEVEL_PLAZA || g_level == LEVEL_PUMPKIN || g_level == LEVEL_BELL || g_level == LEVEL_AVIARY || g_level == LEVEL_WEB || g_level == LEVEL_LOTUS || g_level == LEVEL_ARCHTREE || g_level == LEVEL_CHESS || g_level == LEVEL_MAZE || g_level == LEVEL_FALLS || g_level == LEVEL_CRATER || g_level == LEVEL_TITAN || g_level == LEVEL_HOODOO || g_level == LEVEL_MOAI || g_level == LEVEL_CAVERN || g_level == LEVEL_PINES || g_level == LEVEL_GALLEON || g_level == LEVEL_SUNFLOWER || g_level == LEVEL_SPHINX || g_level == LEVEL_DAM || g_level == LEVEL_THEATRE || g_level == LEVEL_WHEEL || g_level == LEVEL_REDWOOD || g_level == LEVEL_BALLOON || g_level == LEVEL_CANAL || g_level == LEVEL_SILO || g_level == LEVEL_ORGAN || g_level == LEVEL_HYPO || g_level == LEVEL_FORT || g_level == LEVEL_TRIUMPH || g_level == LEVEL_ORCHARD || g_level == LEVEL_LOOM || g_level == LEVEL_SAVANNA || g_level == LEVEL_MOSQUE || g_level == LEVEL_TOTEM || g_level == LEVEL_OASIS || g_level == LEVEL_PAGODA || g_level == LEVEL_STEPWELL || g_level == LEVEL_JANTAR || g_level == LEVEL_TERRACOTTA || g_level == LEVEL_BALLCOURT || g_level == LEVEL_PETRA || g_level == LEVEL_BAZAAR || g_level == LEVEL_SIEGE || g_level == LEVEL_WHALING || g_level == LEVEL_CACTUS || g_level == LEVEL_ZIMBABWE || g_level == LEVEL_BASIL || g_level == LEVEL_PRINT || g_level == LEVEL_GER || g_level == LEVEL_ALCHEMY || g_level == LEVEL_BATHS || g_level == LEVEL_FLOAT || g_level == LEVEL_ISHTAR || g_level == LEVEL_TANNERY || g_level == LEVEL_MUSEUM || g_level == LEVEL_GOMPA || g_level == LEVEL_BUDDHA || g_level == LEVEL_ANGKOR || g_level == LEVEL_BADGIR || g_level == LEVEL_TOPIARY || g_level == LEVEL_GLASSWORKS || g_level == LEVEL_SHIPYARD || g_level == LEVEL_GOTHIC || g_level == LEVEL_PUEBLO) return;   // procedural levels manage their own glow, not gem crystals
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

// The Hollow Graveyard: a flooded necropolis built procedurally (no boss_arena.glb),
// per design/hollow_graveyard_design.md. A central mausoleum, leaning headstones in
// ragged rows, bare dead trees, a broken perimeter wall, and a gateway arch. Stone
// uses the lit cube so it takes the misty light + fog; wisps glow green (additive).
static void build_graveyard() {
    s_tombs.clear(); s_trees.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(90125);
    Color cols[3] = { { 96, 104, 92, 255 }, { 80, 88, 78, 255 }, { 62, 70, 62, 255 } };

    // headstones laid out in ragged rows; skip the crypt + the entrance aisle
    for (int gx = -4; gx <= 4; gx++) {
        for (int gz = -5; gz <= 3; gz++) {
            Vector3 base = ctr + Vector3{ gx * 4.3f + rand_range(-0.8f, 0.8f), 0, gz * 4.1f + rand_range(-0.8f, 0.8f) };
            float dc = Vector3Distance(base, ctr);
            if (dc < 4.8f) continue;                       // clear the mausoleum
            if (base.z > ctr.z + 11.0f) continue;          // clear the entrance aisle (player spawns +Z)
            if (dc > boundary_radius - 2.2f) continue;     // stay inside the wall
            Tomb t;
            t.pos   = base;
            t.yaw   = rand_range(-20.0f, 20.0f) + gx * 6.0f;
            t.w     = rand_range(0.6f, 0.95f);
            t.h     = rand_range(0.85f, 1.7f);
            t.t     = 0.16f;
            t.tilt  = rand_range(-10.0f, 10.0f);
            t.cross = (GetRandomValue(0, 3) == 0);
            t.col   = cols[GetRandomValue(0, 2)];
            s_tombs.push_back(t);
            if (GetRandomValue(0, 4) == 0 && (int)s_wisps.size() < 14)
                s_wisps.push_back(base + Vector3{ 0, 1.35f, 0 });
        }
    }

    // bare dead trees around the perimeter
    static const float tAng[7] = { 20, 65, 120, 165, 215, 285, 330 };
    for (int i = 0; i < 7; i++) {
        float a = tAng[i] * DEG2RAD;
        float r = boundary_radius - rand_range(3.0f, 6.0f);
        Tree tr;
        tr.base = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        tr.h = rand_range(5.0f, 8.0f);
        tr.r = rand_range(0.28f, 0.42f);
        tr.nbr = GetRandomValue(2, 3);
        for (int b = 0; b < tr.nbr; b++) {
            tr.br_yaw[b]  = rand_range(0.0f, 2 * PI);
            tr.br_len[b]  = rand_range(1.2f, 2.4f);
            tr.br_tilt[b] = rand_range(0.3f, 0.9f);
        }
        s_trees.push_back(tr);
    }

    // a couple of wisps drift by the crypt doorway too
    s_wisps.push_back(ctr + Vector3{ 1.5f, 1.1f, 2.1f });
    s_wisps.push_back(ctr + Vector3{ -1.7f, 1.3f, 1.1f });

    // feed wisp positions to the water shader (green glow pools) + the lit point lights
    for (auto& w : s_wisps) {
        if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break;
        s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 6.0f });
    }
}

static void draw_graveyard() {
    if (!s_has_column) return;
    Vector3 ctr = boundary_center;
    Color stone{ 70, 80, 68, 255 }, dark{ 45, 52, 46, 255 }, door{ 14, 17, 14, 255 };

    // central mausoleum: body, cornice, rotated roof cap, doorway recess, corner pilasters
    DrawModelEx(s_column, ctr + Vector3{ 0, 1.50f, 0 },    Vector3{ 0,1,0 }, 0,  Vector3{ 3.4f, 3.0f, 3.4f },  stone);
    DrawModelEx(s_column, ctr + Vector3{ 0, 3.15f, 0 },    Vector3{ 0,1,0 }, 0,  Vector3{ 3.9f, 0.4f, 3.9f },  dark);    // cornice
    DrawModelEx(s_column, ctr + Vector3{ 0, 3.62f, 0 },    Vector3{ 0,1,0 }, 45, Vector3{ 2.5f, 0.8f, 2.5f },  dark);    // roof cap
    DrawModelEx(s_column, ctr + Vector3{ 0, 1.05f, 1.72f },Vector3{ 0,1,0 }, 0,  Vector3{ 1.2f, 2.1f, 0.32f }, door);    // doorway recess
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            DrawModelEx(s_column, ctr + Vector3{ sx * 1.75f, 1.65f, sz * 1.75f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 3.5f, 0.5f }, dark);

    // headstones (lit cubes, leaned via the rlgl matrix so they take light + fog)
    for (auto& t : s_tombs) {
        rlPushMatrix();
        rlTranslatef(t.pos.x, 0.0f, t.pos.z);
        rlRotatef(t.yaw, 0, 1, 0);
        rlRotatef(t.tilt, 1, 0, 0);
        DrawModelEx(s_column, Vector3{ 0, t.h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ t.w, t.h, t.t }, t.col);
        DrawModelEx(s_column, Vector3{ 0, 0.10f, 0 },      Vector3{ 0,1,0 }, 0, Vector3{ t.w * 1.3f, 0.2f, t.t * 2.4f }, dark);   // plinth
        if (t.cross)
            DrawModelEx(s_column, Vector3{ 0, t.h * 0.76f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ t.w * 1.55f, t.w * 0.26f, t.t * 1.02f }, t.col);
        rlPopMatrix();
    }

    // dead trees: tapered trunk + a few angular branches
    for (auto& tr : s_trees) {
        Vector3 top = tr.base + Vector3{ 0, tr.h, 0 };
        DrawCylinderEx(tr.base, top, tr.r, tr.r * 0.18f, 6, Color{ 38, 32, 28, 255 });
        for (int b = 0; b < tr.nbr; b++) {
            Vector3 from = tr.base + Vector3{ 0, tr.h * (0.55f + 0.12f * b), 0 };
            Vector3 dir{ sinf(tr.br_yaw[b]) * cosf(tr.br_tilt[b]), sinf(tr.br_tilt[b]), cosf(tr.br_yaw[b]) * cosf(tr.br_tilt[b]) };
            Vector3 to = from + Vector3Scale(dir, tr.br_len[b]);
            DrawCylinderEx(from, to, tr.r * 0.42f, tr.r * 0.06f, 5, Color{ 32, 27, 24, 255 });
        }
    }

    // broken perimeter wall: a single low ruined ring with frequent gaps (not the
    // Colosseum's six tall tiers -- a different silhouette entirely)
    const int segs = 48;
    float wr = boundary_radius - 1.6f;
    float segW = 2.0f * wr * sinf(PI / segs) * 1.25f;
    for (int i = 0; i < segs; i++) {
        if (i % 3 == 0) continue;                          // frequent gaps
        float deg = i * 360.0f / segs;
        float a = deg * DEG2RAD;
        Vector3 p = ctr + Vector3{ sinf(a) * wr, 0, cosf(a) * wr };
        float h = 1.6f + (float)((i * 5) % 5) * 0.22f;     // 1.6..2.5 ragged
        DrawModelEx(s_column, Vector3{ p.x, h * 0.5f, p.z }, Vector3{ 0,1,0 }, deg, Vector3{ segW, h, 0.7f }, (i % 2) ? stone : dark);
    }

    // gateway arch on the +Z entrance side (toward the player spawn)
    {
        Vector3 g = ctr + Vector3{ 0, 0, boundary_radius - 1.6f };
        for (int s = -1; s <= 1; s += 2)
            DrawModelEx(s_column, g + Vector3{ 2.4f * s, 2.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 4.2f, 0.9f }, stone);
        DrawModelEx(s_column, g + Vector3{ 0, 4.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.2f, 0.8f, 0.9f }, dark);   // lintel
    }

    // will-o'-wisps: small green flames bobbing over the graves (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        float ph = s_time * 1.6f + (float)i * 1.7f;
        Vector3 fp = w + Vector3{ 0.18f * sinf(ph), 0.22f * sinf(ph * 0.7f), 0.18f * cosf(ph * 1.1f) };
        float fl = 0.55f + 0.45f * sinf(s_time * 5.0f + (float)i * 2.3f);
        DrawSphereEx(fp, 0.10f + 0.05f * fl, 6, 6, Color{ 150, 255, 150, 150 });
        DrawSphereEx(fp, 0.30f + 0.10f * fl, 8, 8, Color{ 90, 220, 120, 45 });
    }
    EndBlendMode();
}

// The Fungal Grotto: a bioluminescent underground cavern of giant mushrooms built
// procedurally (no boss_arena.glb), per design/fungal_grotto_design.md. Lit cylinder
// stalks + lit hemisphere caps (teal/violet) with additive under-cap glow, a colossal
// central mushroom landmark, ground spore pods, and drifting spore motes.
static void build_fungal() {
    s_shrooms.clear();
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(55217);

    Color stalkC{ 86, 96, 92, 255 };
    Color capTeal{ 40, 120, 120, 255 }, capViolet{ 96, 60, 130, 255 };
    Color glowTeal{ 70, 230, 210, 110 }, glowViolet{ 150, 90, 230, 95 };

    // colossal central mushroom (the landmark / obstacle)
    {
        Shroom m;
        m.base = ctr;
        m.stalkH = 9.5f; m.stalkR = 1.5f;
        m.capR = 6.0f; m.capH = 3.4f;
        m.lean = 0.0f; m.leanYaw = 0.0f;
        m.stalkC = stalkC; m.capC = capTeal; m.glowC = glowTeal;
        s_shrooms.push_back(m);
        s_wisps.push_back(ctr + Vector3{ 0, m.stalkH - 0.5f, 0 });
    }

    // a scatter of giant + medium mushrooms (entrance aisle kept clear)
    int placed = 0, attempts = 0, want = 9;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float ang = rand_range(0, 2 * PI);
        float rad = rand_range(7.0f, boundary_radius - 3.0f);
        Vector3 base = ctr + Vector3{ sinf(ang) * rad, 0, cosf(ang) * rad };
        if (base.z > ctr.z + 12.0f) continue;          // keep the +Z entrance aisle clear
        bool big = (GetRandomValue(0, 2) == 0);
        Shroom m;
        m.base = base;
        m.stalkH = big ? rand_range(5.5f, 8.5f) : rand_range(2.2f, 4.0f);
        m.stalkR = big ? rand_range(0.7f, 1.1f) : rand_range(0.35f, 0.6f);
        m.capR  = m.stalkR * rand_range(3.0f, 4.2f);
        m.capH  = m.capR * rand_range(0.45f, 0.65f);
        m.lean  = rand_range(-8.0f, 8.0f);
        m.leanYaw = rand_range(0.0f, 360.0f);
        bool violet = (GetRandomValue(0, 1) == 0);
        m.stalkC = stalkC;
        m.capC  = violet ? capViolet : capTeal;
        m.glowC = violet ? glowViolet : glowTeal;
        s_shrooms.push_back(m);
        if (big && (int)s_wisps.size() < MAX_CRYSTAL_LIGHTS)
            s_wisps.push_back(base + Vector3{ 0, m.stalkH - 0.3f, 0 });
        placed++;
    }

    // low ground spore pods (extra floor-level glow)
    static const float pAng[5] = { 30, 100, 170, 250, 320 };
    for (int i = 0; i < 5; i++) {
        float a = pAng[i] * DEG2RAD;
        float r = rand_range(6.0f, 16.0f);
        s_wisps.push_back(ctr + Vector3{ sinf(a) * r, 0.6f, cosf(a) * r });
    }

    // feed glow points to the water shader (teal pools) + the lit point lights
    for (auto& w : s_wisps) {
        if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break;
        s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 7.0f });
    }
}

static void draw_fungal() {
    if (!s_has_shapes) return;

    // pass 1: lit stalks + gill disks + dome caps (leaned via the rlgl matrix)
    for (auto& m : s_shrooms) {
        rlPushMatrix();
        rlTranslatef(m.base.x, m.base.y, m.base.z);
        rlRotatef(m.leanYaw, 0, 1, 0);
        rlRotatef(m.lean, 1, 0, 0);
        DrawModelEx(s_cyl, Vector3{ 0, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ m.stalkR, m.stalkH, m.stalkR }, m.stalkC);
        Color gill{ (unsigned char)(m.capC.r * 0.5f), (unsigned char)(m.capC.g * 0.5f), (unsigned char)(m.capC.b * 0.5f), 255 };
        DrawModelEx(s_cyl,  Vector3{ 0, m.stalkH - 0.05f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ m.capR * 0.85f, 0.16f, m.capR * 0.85f }, gill);
        DrawModelEx(s_dome, Vector3{ 0, m.stalkH, 0 },         Vector3{ 0,1,0 }, 0, Vector3{ m.capR, m.capH, m.capR }, m.capC);
        rlPopMatrix();
    }

    // pass 2: bioluminescent under-cap glow + drifting spore motes (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    for (auto& m : s_shrooms) {
        float lr = m.lean * DEG2RAD, ya = m.leanYaw * DEG2RAD;
        Vector3 up{ sinf(ya) * sinf(lr), cosf(lr), cosf(ya) * sinf(lr) };
        Vector3 cap = m.base + Vector3Scale(up, m.stalkH);
        DrawSphereEx(cap + Vector3{ 0, m.capH * 0.35f, 0 }, m.capR * 0.55f, 8, 8, m.glowC);
        DrawSphereEx(cap + Vector3{ 0, m.capH * 0.20f, 0 }, m.capR * 0.95f, 8, 8, Color{ m.glowC.r, m.glowC.g, m.glowC.b, 30 });
    }
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        float ph = s_time * 1.1f + (float)i * 1.7f;
        Vector3 fp = w + Vector3{ 0.5f * sinf(ph), 0.4f * sinf(ph * 0.6f) + 0.3f, 0.5f * cosf(ph * 0.8f) };
        float fl = 0.5f + 0.5f * sinf(s_time * 3.0f + (float)i * 2.1f);
        DrawSphereEx(fp, 0.06f + 0.04f * fl, 5, 5, Color{ 120, 245, 220, 150 });
    }
    EndBlendMode();
}

// The Desert Ziggurat: a sun-scorched ruin built procedurally (no boss_arena.glb),
// per design/desert_ziggurat_design.md. A solid stepped ziggurat, tilted half-fallen
// obelisks, broken column drums, a toppled colossus, and braziers, on DRY sand (this
// level skips the water plane and lays its own solid floor). Stone uses the lit cube/
// cylinder so it takes the warm dusk light + sandy haze.
static void build_desert() {
    s_obelisks.clear(); s_drums.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(74021);
    Color sand{ 196, 170, 120, 255 }, sandD{ 150, 126, 86, 255 };

    // tilted half-fallen obelisks scattered around (entrance aisle kept clear)
    int placed = 0, attempts = 0, want = 9;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float ang = rand_range(0, 2 * PI);
        float rad = rand_range(8.0f, boundary_radius - 3.0f);
        Vector3 p = ctr + Vector3{ sinf(ang) * rad, 0, cosf(ang) * rad };
        if (p.z > ctr.z + 12.0f) continue;
        Obelisk o;
        o.pos = p;
        o.w = rand_range(0.6f, 1.0f);
        o.h = rand_range(4.0f, 8.0f);
        o.lean = rand_range(-24.0f, 24.0f);
        o.leanYaw = rand_range(0.0f, 360.0f);
        o.col = (GetRandomValue(0, 1)) ? sand : sandD;
        s_obelisks.push_back(o);
        placed++;
    }

    // broken column drums: short stacks (1-3) with slight leans
    static const float dAng[6] = { 25, 80, 150, 205, 270, 325 };
    for (int i = 0; i < 6; i++) {
        float a = dAng[i] * DEG2RAD;
        float rad = rand_range(7.0f, boundary_radius - 4.0f);
        Drum d;
        d.pos = ctr + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (d.pos.z > ctr.z + 12.0f) d.pos.z = ctr.z + 12.0f;
        d.r = rand_range(0.6f, 0.95f);
        d.h = rand_range(0.8f, 1.4f);
        d.stack = GetRandomValue(1, 3);
        d.lean = rand_range(-6.0f, 6.0f);
        d.leanYaw = rand_range(0.0f, 360.0f);
        d.col = sand;
        s_drums.push_back(d);
    }

    // braziers at the four ziggurat corners (the warm point lights)
    s_wisps.push_back(ctr + Vector3{ -3.8f, 1.4f, -3.8f });
    s_wisps.push_back(ctr + Vector3{  3.8f, 1.4f, -3.8f });
    s_wisps.push_back(ctr + Vector3{ -3.8f, 1.4f,  3.8f });
    s_wisps.push_back(ctr + Vector3{  3.8f, 1.4f,  3.8f });
    for (auto& w : s_wisps) {
        if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break;
        s_crystal_lights.push_back(Vector4{ w.x, 1.4f, w.z, 8.0f });
    }
}

static void draw_desert() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color sand{ 196, 170, 120, 255 }, sandD{ 150, 126, 86, 255 },
          stone{ 168, 144, 104, 255 }, dark{ 120, 100, 70, 255 };

    // solid sand floor (top at y=0) + low dune mounds for relief
    DrawModelEx(s_column, Vector3{ ctr.x, -0.5f, ctr.z }, Vector3{ 0,1,0 }, 0, Vector3{ 130.0f, 1.0f, 130.0f }, sand);
    static const float duAng[6] = { 15, 70, 130, 200, 260, 320 };
    for (int i = 0; i < 6; i++) {
        float a = duAng[i] * DEG2RAD; float r = 14.0f + (i % 3) * 3.0f;
        Vector3 p = ctr + Vector3{ sinf(a) * r, -0.3f, cosf(a) * r };
        DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, (float)(i * 40), Vector3{ 6.0f + i, 1.4f, 4.5f + i }, sandD);
    }

    // central stepped ziggurat (SOLID -- unlike the Colosseum's hollow concentric rings)
    const float dh = 1.4f, dw = 1.85f, w0 = 10.0f;
    for (int t = 0; t < 5; t++) {
        float w = w0 - dw * 2.0f * t;
        DrawModelEx(s_column, ctr + Vector3{ 0, dh * 0.5f + dh * t, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, dh, w }, (t % 2) ? stone : sand);
    }
    DrawModelEx(s_column, ctr + Vector3{ 0, dh * 5 + 0.7f, 0 }, Vector3{ 0,1,0 }, 45, Vector3{ 1.6f, 1.4f, 1.6f }, dark);   // capstone shrine

    // tilted obelisks (shaft + pyramidion cap, leaned via the rlgl matrix)
    for (auto& o : s_obelisks) {
        rlPushMatrix();
        rlTranslatef(o.pos.x, 0.0f, o.pos.z);
        rlRotatef(o.leanYaw, 0, 1, 0);
        rlRotatef(o.lean, 1, 0, 0);
        DrawModelEx(s_column, Vector3{ 0, o.h * 0.5f, 0 },          Vector3{ 0,1,0 }, 0,  Vector3{ o.w, o.h, o.w }, o.col);
        DrawModelEx(s_column, Vector3{ 0, o.h + o.w * 0.5f, 0 },    Vector3{ 0,1,0 }, 45, Vector3{ o.w * 0.85f, o.w, o.w * 0.85f }, dark);   // pyramidion
        rlPopMatrix();
    }

    // broken column drums (short stacks)
    for (auto& d : s_drums) {
        rlPushMatrix();
        rlTranslatef(d.pos.x, 0.0f, d.pos.z);
        rlRotatef(d.leanYaw, 0, 1, 0);
        rlRotatef(d.lean, 1, 0, 0);
        for (int k = 0; k < d.stack; k++)
            DrawModelEx(s_cyl, Vector3{ 0, d.h * k, 0 }, Vector3{ 0,1,0 }, (float)(k * 20), Vector3{ d.r, d.h, d.r }, (k % 2) ? stone : sand);
        rlPopMatrix();
    }

    // a toppled colossus near the back: two large leaning slabs + a half-buried head
    Vector3 cx = ctr + Vector3{ -14.0f, 0, -10.0f };
    for (int s = 0; s < 2; s++) {
        rlPushMatrix();
        rlTranslatef(cx.x + s * 2.2f, 0.0f, cx.z + s * 1.4f);
        rlRotatef(28.0f + s * 18.0f, 0, 1, 0);
        rlRotatef(34.0f + s * 10.0f, 1, 0, 0);              // leaning rubble (stays above sand)
        DrawModelEx(s_column, Vector3{ 0, 2.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 5.0f, 1.7f }, (s ? sandD : stone));
        rlPopMatrix();
    }
    DrawModelEx(s_dome, cx + Vector3{ 3.0f, 0.0f, 1.2f }, Vector3{ 0,1,0 }, 30, Vector3{ 1.9f, 1.5f, 1.9f }, sandD);   // half-buried head

    // braziers: pedestal (lit) + flames (additive)
    for (auto& w : s_wisps)
        DrawModelEx(s_column, Vector3{ w.x, 0.7f, w.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.4f, 0.6f }, dark);
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        for (int f = 0; f < 5; f++) {
            float fa = f * 1.3f + (float)i;
            float fl = 0.5f + 0.5f * sinf(s_time * 7.0f + fa * 2.1f);
            Vector3 fp = w + Vector3{ 0.12f * sinf(fa + s_time), 0.7f + (0.3f + 0.5f * fl) * 0.5f, 0.12f * cosf(fa + s_time) };
            DrawSphereEx(fp, 0.15f + 0.10f * fl, 6, 6, Color{ 255, (unsigned char)(150 + 80 * fl), 40, 130 });
        }
        DrawSphereEx(w + Vector3{ 0, 1.0f, 0 }, 0.7f, 8, 8, Color{ 255, 140, 45, 45 });
    }
    EndBlendMode();
}

// The Shipwreck Cove: a foggy moonlit cove of drowned ships built procedurally (no
// boss_arena.glb), per design/shipwreck_cove_design.md. Listing lit-cube hulls with
// leaning lit-cylinder masts + crossbeams, plank piers on pilings, floating barrels/
// crates, and warm hanging lanterns, over the (reused) moonlit reflective water.
static void build_wreck() {
    s_wrecks.clear(); s_props.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(33190);
    Color hull0{ 74, 58, 44, 255 }, hull1{ 92, 72, 52, 255 };

    // great central wreck (the landmark / obstacle)
    {
        Wreck w;
        w.pos = ctr;
        w.yaw = 28.0f; w.list = 15.0f;
        w.len = 13.0f; w.w = 4.2f; w.h = 3.6f;
        w.mast = true; w.mastH = 9.5f;
        w.col = hull0;
        s_wrecks.push_back(w);
        s_wisps.push_back(ctr + Vector3{ 0, w.h + w.mastH * 0.7f, 0 });
    }

    // scattered smaller wrecks (entrance aisle kept clear)
    int placed = 0, attempts = 0, want = 3;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float ang = rand_range(0, 2 * PI);
        float rad = rand_range(9.0f, boundary_radius - 4.0f);
        Vector3 p = ctr + Vector3{ sinf(ang) * rad, 0, cosf(ang) * rad };
        if (p.z > ctr.z + 12.0f) continue;
        Wreck w;
        w.pos = p;
        w.yaw = rand_range(0.0f, 360.0f);
        w.list = rand_range(8.0f, 24.0f) * (GetRandomValue(0, 1) ? 1.0f : -1.0f);
        w.len = rand_range(7.0f, 10.0f);
        w.w = rand_range(2.8f, 3.6f);
        w.h = rand_range(2.4f, 3.2f);
        w.mast = (GetRandomValue(0, 2) != 0);
        w.mastH = rand_range(5.5f, 8.0f);
        w.col = (GetRandomValue(0, 1)) ? hull0 : hull1;
        s_wrecks.push_back(w);
        if (w.mast && (int)s_wisps.size() < MAX_CRYSTAL_LIGHTS)
            s_wisps.push_back(p + Vector3{ 0, w.h + w.mastH * 0.6f, 0 });
        placed++;
    }

    // floating barrels + crates
    static const float pAng[8] = { 20, 70, 115, 160, 210, 255, 300, 340 };
    for (int i = 0; i < 8; i++) {
        float a = pAng[i] * DEG2RAD;
        float r = rand_range(6.0f, boundary_radius - 3.0f);
        Prop pr;
        pr.pos = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        pr.yaw = rand_range(0.0f, 360.0f);
        pr.s = rand_range(0.5f, 0.9f);
        pr.crate = (i % 2 == 0);
        pr.col = (i % 2) ? hull1 : Color{ 96, 78, 58, 255 };
        s_props.push_back(pr);
    }

    // lantern glow points -> water glow + lit point lights
    for (auto& w : s_wisps) {
        if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break;
        s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 7.0f });
    }
}

static void draw_wreck() {
    if (!s_has_column || !s_has_shapes) return;
    Color deckC{ 60, 48, 36, 255 }, mastC{ 64, 50, 38, 255 }, plankC{ 84, 66, 48, 255 };

    // piers: two plank decks on pilings reaching out over the water from the shallows
    for (int pq = 0; pq < 2; pq++) {
        float sgn = pq ? 1.0f : -1.0f;
        Vector3 root = boundary_center + Vector3{ 14.0f * sgn, 0, 9.0f };
        Vector3 dir = Vector3Normalize(Vector3{ -0.4f * sgn, 0, -1.0f });
        float pierLen = 12.0f;
        Vector3 mid = root + Vector3Scale(dir, pierLen * 0.5f);
        float yaw = atan2f(dir.x, dir.z) * RAD2DEG, yr = yaw * DEG2RAD;
        DrawModelEx(s_column, mid + Vector3{ 0, 1.1f, 0 }, Vector3{ 0,1,0 }, yaw, Vector3{ 2.2f, 0.25f, pierLen }, plankC);   // deck
        for (int k = 0; k <= 5; k++) {                       // pilings (two rows)
            Vector3 pp = root + Vector3Scale(dir, pierLen * (float)k / 5.0f);
            for (int s = -1; s <= 1; s += 2)
                DrawModelEx(s_cyl, Vector3{ pp.x + cosf(yr) * 0.9f * s, 0, pp.z - sinf(yr) * 0.9f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 1.2f, 0.18f }, mastC);
        }
    }

    // ship wrecks (hull + deck + rib planks + leaning mast, all sharing the list)
    for (auto& w : s_wrecks) {
        rlPushMatrix();
        rlTranslatef(w.pos.x, 0.0f, w.pos.z);
        rlRotatef(w.yaw, 0, 1, 0);
        rlRotatef(w.list, 1, 0, 0);                          // roll about the length axis = listing
        float sink = 0.7f;
        DrawModelEx(s_column, Vector3{ 0, w.h * 0.5f - sink, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w.len, w.h, w.w }, w.col);            // hull
        DrawModelEx(s_column, Vector3{ 0, w.h - sink, 0 },       Vector3{ 0,1,0 }, 0, Vector3{ w.len * 0.92f, 0.3f, w.w * 0.78f }, deckC);  // deck
        for (int r = -2; r <= 2; r++) {                      // broken rib planks up one side
            float fx = (float)r / 2.0f;
            Vector3 rp{ fx * w.len * 0.42f, w.h * 0.7f - sink, w.w * 0.42f };
            DrawModelEx(s_column, rp, Vector3{ 0,1,0 }, 14.0f, Vector3{ 0.16f, 1.1f + 0.3f * (1.0f - fx * fx), 0.16f }, plankC);
        }
        if (w.mast) {
            DrawModelEx(s_cyl,    Vector3{ w.len * 0.28f, w.h - sink, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, w.mastH, 0.22f }, mastC);
            DrawModelEx(s_column, Vector3{ w.len * 0.28f, w.h - sink + w.mastH * 0.72f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 0.16f, w.mastH * 0.55f }, mastC);   // crossbeam
        }
        rlPopMatrix();
    }

    // floating barrels (cylinders) + crates (cubes)
    for (auto& p : s_props) {
        if (p.crate)
            DrawModelEx(s_column, p.pos + Vector3{ 0, p.s * 0.5f, 0 }, Vector3{ 0,1,0 }, p.yaw, Vector3{ p.s, p.s, p.s }, p.col);
        else
            DrawModelEx(s_cyl, p.pos + Vector3{ 0, 0.05f, 0 }, Vector3{ 0,1,0 }, p.yaw, Vector3{ p.s * 0.5f, p.s, p.s * 0.5f }, p.col);
    }

    // hanging lantern glows (additive warm)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        float fl = 0.55f + 0.45f * sinf(s_time * 4.0f + (float)i * 2.3f);
        DrawSphereEx(w, 0.16f + 0.06f * fl, 6, 6, Color{ 255, 180, 80, 160 });
        DrawSphereEx(w, 0.50f + 0.12f * fl, 8, 8, Color{ 255, 150, 60, 40 });
    }
    EndBlendMode();
}

// The Sky Sanctum: a shattered floating island high among the clouds, built
// procedurally (no boss_arena.glb), per design/sky_sanctum_design.md. A great
// circular platform (the play floor) with a tapered rocky underside, satellite
// islands drifting at various heights, arched sky-bridges, a central monument
// ringed by slowly orbiting stone shards, drifting soul-motes, and a cloud band
// far below the void. Dry (no water): this draw lays the floating platform floor.
static void draw_island(Vector3 c, float r, float tilt, float tiltYaw, Color top, Color under) {
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(tiltYaw, 0, 1, 0);
    rlRotatef(tilt, 1, 0, 0);
    DrawModelEx(s_cyl, Vector3{ 0, -1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r, 1.0f, r }, top);            // top slab (top at y=0)
    DrawModelEx(s_cyl, Vector3{ 0, -2.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 0.72f, 1.5f, r * 0.72f }, under);  // tapered
    DrawModelEx(s_cyl, Vector3{ 0, -4.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 0.42f, 1.8f, r * 0.42f }, under);  // rocky
    DrawModelEx(s_cyl, Vector3{ 0, -6.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 0.16f, 1.6f, r * 0.16f }, under);  // underside
    rlPopMatrix();
}

static void build_sanctum() {
    s_isles.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(60613);
    Color rock0{ 96, 100, 116, 255 }, rock1{ 78, 82, 98, 255 };

    // satellite islands drifting at various heights, outside the main platform
    int placed = 0, attempts = 0, want = 7;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float ang = rand_range(0, 2 * PI);
        float rad = rand_range(28.0f, 50.0f);
        Isle is;
        is.pos = ctr + Vector3{ sinf(ang) * rad, rand_range(-10.0f, 8.0f), cosf(ang) * rad };
        is.r = rand_range(3.0f, 7.0f);
        is.tilt = rand_range(-10.0f, 10.0f);
        is.tiltYaw = rand_range(0.0f, 360.0f);
        is.col = (GetRandomValue(0, 1)) ? rock0 : rock1;
        s_isles.push_back(is);
        placed++;
    }

    // soul-motes: a ring around the monument + a few scattered over the platform
    for (int i = 0; i < 6; i++) {
        float a = i * (2 * PI / 6.0f);
        s_wisps.push_back(ctr + Vector3{ sinf(a) * 5.5f, 2.5f, cosf(a) * 5.5f });
    }
    static const float mAng[4] = { 40, 130, 220, 310 };
    for (int i = 0; i < 4; i++) {
        float a = mAng[i] * DEG2RAD, r = rand_range(10.0f, 20.0f);
        s_wisps.push_back(ctr + Vector3{ sinf(a) * r, 1.2f, cosf(a) * r });
    }
    for (auto& w : s_wisps) {
        if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break;
        s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 6.0f });
    }
}

static void draw_sanctum() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color rock{ 104, 108, 124, 255 }, rockD{ 70, 74, 90, 255 }, mono{ 120, 124, 140, 255 }, dark{ 60, 63, 78, 255 };

    // main floating platform (the play floor) + satellite islands
    draw_island(ctr, 25.0f, 0.0f, 0.0f, rock, rockD);
    for (auto& is : s_isles) draw_island(is.pos, is.r, is.tilt, is.tiltYaw, is.col, rockD);

    // two arched sky-bridges reaching off the platform rim (catenary dip)
    for (int b = 0; b < 2; b++) {
        float deg = b ? 150.0f : 330.0f, a = deg * DEG2RAD;
        Vector3 dirr{ sinf(a), 0, cosf(a) };
        for (int k = -4; k <= 4; k++) {
            float fx = k / 4.0f;
            float along = 25.0f + (fx + 1.0f) * 4.0f;
            float h = -0.2f - 1.6f * (1.0f - fx * fx);
            Vector3 p = ctr + Vector3Scale(dirr, along);
            DrawModelEx(s_column, Vector3{ p.x, h, p.z }, Vector3{ 0,1,0 }, deg, Vector3{ 1.8f, 0.5f, 2.0f }, (k % 2) ? rock : rockD);
        }
    }

    // central monument: stepped pedestal + spire, ringed by slowly ORBITING shards (animated)
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 0.8f, 3.0f }, mono);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 1.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 0.7f, 2.2f }, dark);
    DrawModelEx(s_column, ctr + Vector3{ 0, 4.0f, 0 }, Vector3{ 0,1,0 }, 45, Vector3{ 0.7f, 6.0f, 0.7f }, mono);   // spire
    float spin = s_time * 0.5f;
    for (int i = 0; i < 8; i++) {
        float a = spin + i * (2 * PI / 8.0f);
        float rr = 5.0f + 0.4f * sinf(s_time + i);
        float yy = 3.0f + 0.8f * sinf(s_time * 0.8f + i * 1.3f);
        Vector3 sp = ctr + Vector3{ sinf(a) * rr, yy, cosf(a) * rr };
        DrawModelEx(s_column, sp, Vector3{ 0,1,0 }, a * RAD2DEG, Vector3{ 0.5f, 0.9f, 0.5f }, rockD);
    }

    // drifting debris over the platform (small slowly-spinning bobbing shards)
    for (int i = 0; i < 10; i++) {
        float a = i * 2.39996f;
        float r = 6.0f + (float)((i * 37) % 16);
        Vector3 p = ctr + Vector3{ sinf(a) * r, 4.5f + (i % 4) + 0.5f * sinf(s_time * 1.3f + i), cosf(a) * r };
        DrawModelEx(s_column, p, Vector3{ 0,1,0 }, (float)(i * 33) + s_time * 20.0f, Vector3{ 0.35f, 0.35f, 0.35f }, rockD);
    }

    // glowing orb atop the spire + drifting soul-motes + a faint cloud band far below
    BeginBlendMode(BLEND_ADDITIVE);
    DrawSphereEx(ctr + Vector3{ 0, 7.4f, 0 }, 0.5f + 0.06f * sinf(s_time * 3.0f), 10, 10, Color{ 180, 210, 255, 150 });
    DrawSphereEx(ctr + Vector3{ 0, 7.4f, 0 }, 1.2f, 10, 10, Color{ 120, 170, 255, 35 });
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        float ph = s_time * 0.9f + (float)i * 1.7f;
        Vector3 fp = w + Vector3{ 0.4f * sinf(ph), 0.5f * sinf(ph * 0.6f), 0.4f * cosf(ph * 0.8f) };
        float fl = 0.5f + 0.5f * sinf(s_time * 3.0f + i * 2.1f);
        DrawSphereEx(fp, 0.10f + 0.05f * fl, 6, 6, Color{ 200, 220, 255, 150 });
    }
    for (int i = 0; i < 7; i++) {
        float a = i * (2 * PI / 7.0f) + s_time * 0.02f;
        Vector3 cp = ctr + Vector3{ sinf(a) * (30.0f + i * 6), -22.0f, cosf(a) * (30.0f + i * 6) };
        DrawSphereEx(cp, 12.0f, 8, 8, Color{ 200, 210, 230, 12 });
    }
    EndBlendMode();
}

// The Clockwork Vault: a dim brass mechanism hall built procedurally (no
// boss_arena.glb), per design/clockwork_vault_design.md. Giant gears (lit-cylinder
// hub + lit-cube teeth + spokes) spin at varied speeds -- flat cogs floating at
// different heights and upright wall-cogs -- around a central glowing engine core on
// a tall axle, with brass pillars, pipes, and hissing steam vents, over the reused
// dark oily reflective water.
static void build_clock() {
    s_gears.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(81144);
    Color brass{ 182, 150, 86, 255 }, copper{ 168, 104, 66, 255 }, iron{ 96, 92, 100, 255 }, darkIron{ 60, 58, 66, 255 };

    // free-standing gears: flat cogs at varied heights + upright wall-cogs
    int placed = 0, attempts = 0, want = 7;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float ang = rand_range(0, 2 * PI);
        float rad = rand_range(9.0f, boundary_radius - 4.0f);
        Vector3 p = ctr + Vector3{ sinf(ang) * rad, 0, cosf(ang) * rad };
        if (p.z > ctr.z + 12.0f) continue;
        Gear g;
        g.upright = (GetRandomValue(0, 2) == 0);
        g.radius = rand_range(2.2f, 4.5f);
        g.thick = rand_range(0.4f, 0.7f);
        g.teeth = GetRandomValue(10, 16);
        g.speed = rand_range(18.0f, 50.0f) * (GetRandomValue(0, 1) ? 1.0f : -1.0f);
        g.c = p;
        g.c.y = g.upright ? (g.radius + 0.5f) : rand_range(0.6f, 3.5f);
        g.body = (GetRandomValue(0, 1)) ? brass : copper;
        g.tooth = iron; g.spoke = darkIron;
        s_gears.push_back(g);
        placed++;
    }

    // steam vents / furnace glow points + the core
    static const float vAng[5] = { 30, 100, 170, 250, 320 };
    for (int i = 0; i < 5; i++) {
        float a = vAng[i] * DEG2RAD, r = rand_range(8.0f, 18.0f);
        s_wisps.push_back(ctr + Vector3{ sinf(a) * r, 0.5f, cosf(a) * r });
    }
    s_wisps.push_back(ctr + Vector3{ 0, 4.5f, 0 });
    for (auto& w : s_wisps) {
        if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break;
        s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 7.0f });
    }
}

static void draw_gear(const Gear& g, float ang) {
    rlPushMatrix();
    rlTranslatef(g.c.x, g.c.y, g.c.z);
    if (g.upright) rlRotatef(90.0f, 1, 0, 0);   // stand the gear up (local Y -> world Z)
    rlRotatef(ang, 0, 1, 0);                     // spin about the gear's own axis
    DrawModelEx(s_cyl, Vector3{ 0, -g.thick * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ g.radius * 0.55f, g.thick, g.radius * 0.55f }, g.body);   // hub
    for (int i = 0; i < g.teeth; i++) {          // teeth around the rim
        rlPushMatrix();
        rlRotatef(i * 360.0f / g.teeth, 0, 1, 0);
        DrawModelEx(s_column, Vector3{ g.radius, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ g.radius * 0.20f, g.thick, g.radius * 0.12f }, g.tooth);
        rlPopMatrix();
    }
    for (int s = 0; s < 3; s++) {                // spokes across the diameter
        rlPushMatrix();
        rlRotatef(s * 60.0f, 0, 1, 0);
        DrawModelEx(s_column, Vector3{ 0, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ g.radius * 1.5f, g.thick * 0.7f, g.radius * 0.14f }, g.spoke);
        rlPopMatrix();
    }
    rlPopMatrix();
}

static void draw_clock() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color brass{ 182, 150, 86, 255 }, copper{ 168, 104, 66, 255 }, iron{ 96, 92, 100, 255 }, darkIron{ 60, 58, 66, 255 };

    // free gears (spun by time)
    for (auto& g : s_gears) draw_gear(g, s_time * g.speed);

    // central engine: tall axle + a stack of counter-rotating flat gears
    DrawModelEx(s_cyl, ctr, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 7.0f, 0.5f }, darkIron);   // axle
    Gear cg; cg.upright = false; cg.tooth = iron; cg.spoke = darkIron;
    cg.c = ctr + Vector3{ 0, 1.2f, 0 }; cg.radius = 3.2f; cg.thick = 0.6f; cg.teeth = 18; cg.body = brass;  draw_gear(cg,  s_time * 22.0f);
    cg.c = ctr + Vector3{ 0, 3.2f, 0 }; cg.radius = 2.4f; cg.thick = 0.5f; cg.teeth = 14; cg.body = copper; draw_gear(cg, -s_time * 30.0f);
    cg.c = ctr + Vector3{ 0, 5.0f, 0 }; cg.radius = 1.6f; cg.thick = 0.45f; cg.teeth = 12; cg.body = brass; draw_gear(cg,  s_time * 40.0f);

    // brass support pillars with cap fittings + a pipe stub
    static const float pAng[6] = { 0, 60, 120, 180, 240, 300 };
    for (int i = 0; i < 6; i++) {
        float a = pAng[i] * DEG2RAD, r = boundary_radius - 2.5f;
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        DrawModelEx(s_cyl, Vector3{ p.x, 0, p.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 6.5f, 0.8f }, iron);
        DrawModelEx(s_cyl, Vector3{ p.x, 6.3f, p.z }, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f, 0.5f, 1.1f }, copper);
        DrawModelEx(s_column, Vector3{ p.x, 2.0f, p.z }, Vector3{ 0,1,0 }, pAng[i], Vector3{ 2.2f, 0.4f, 0.4f }, darkIron);
    }

    // glowing core + rising steam puffs + vent embers (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    DrawSphereEx(ctr + Vector3{ 0, 4.0f, 0 }, 0.8f + 0.1f * sinf(s_time * 4.0f), 10, 10, Color{ 255, 170, 60, 150 });
    DrawSphereEx(ctr + Vector3{ 0, 4.0f, 0 }, 1.8f, 10, 10, Color{ 255, 120, 40, 35 });
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        float ph = s_time * 1.4f + (float)i * 1.9f;
        for (int p = 0; p < 3; p++) {
            float t = fmodf(ph * 0.4f + p * 0.33f, 1.0f);
            Vector3 sp = w + Vector3{ 0.2f * sinf(ph + p), t * 2.5f, 0.2f * cosf(ph + p) };
            DrawSphereEx(sp, 0.25f + t * 0.5f, 6, 6, Color{ 220, 210, 200, (unsigned char)(70 * (1.0f - t)) });
        }
        DrawSphereEx(w, 0.3f, 6, 6, Color{ 255, 150, 50, 60 });
    }
    EndBlendMode();
}

// The Forgotten Shrine: a misty mountain shrine standing in a still reflecting pool,
// built procedurally (no boss_arena.glb), per design/forgotten_shrine_design.md. A
// processional avenue of vermillion torii gates leads to a tiered pagoda pavilion,
// flanked by glowing stone lanterns, with cherry-blossom trees and one lone torii out
// in the water. Stone uses the lit primitives; reuses the reflective water as the pond.
static void draw_torii(const Torii& t) {
    rlPushMatrix();
    rlTranslatef(t.c.x, t.c.y, t.c.z);
    rlRotatef(t.yaw, 0, 1, 0);
    for (int s = -1; s <= 1; s += 2)                                            // two round posts
        DrawModelEx(s_cyl, Vector3{ t.w * s, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.34f, t.h, 0.34f }, t.col);
    DrawModelEx(s_column, Vector3{ 0, t.h, 0 },          Vector3{ 0,1,0 }, 0, Vector3{ t.w * 2.7f, 0.45f, 0.55f }, t.col);   // kasagi (top beam)
    DrawModelEx(s_column, Vector3{ 0, t.h + 0.42f, 0 },  Vector3{ 0,1,0 }, 0, Vector3{ t.w * 2.3f, 0.24f, 0.42f }, Color{ 60, 30, 26, 255 });  // ridge
    for (int s = -1; s <= 1; s += 2) {                                          // upward-swept ends
        rlPushMatrix();
        rlTranslatef(t.w * 1.32f * s, t.h + 0.1f, 0);
        rlRotatef(-16.0f * s, 0, 0, 1);
        DrawModelEx(s_column, Vector3{ 0.18f * s, 0.18f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.55f, 0.34f, 0.55f }, t.col);
        rlPopMatrix();
    }
    DrawModelEx(s_column, Vector3{ 0, t.h * 0.72f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ t.w * 2.15f, 0.30f, 0.42f }, t.col);   // nuki (lower beam)
    DrawModelEx(s_column, Vector3{ 0, t.h * 0.85f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, t.h * 0.26f, 0.22f }, Color{ 60, 30, 26, 255 });  // gakuzuka plaque
    rlPopMatrix();
}

static void draw_lantern(Vector3 b, Color stone) {
    DrawModelEx(s_cyl,    Vector3{ b.x, 0,     b.z }, Vector3{ 0,1,0 }, 0,  Vector3{ 0.5f, 0.3f, 0.5f }, stone);   // base
    DrawModelEx(s_cyl,    Vector3{ b.x, 0.3f,  b.z }, Vector3{ 0,1,0 }, 0,  Vector3{ 0.17f, 1.1f, 0.17f }, stone); // post
    DrawModelEx(s_column, Vector3{ b.x, 1.55f, b.z }, Vector3{ 0,1,0 }, 45, Vector3{ 0.72f, 0.62f, 0.72f }, stone);// light box
    DrawModelEx(s_column, Vector3{ b.x, 2.02f, b.z }, Vector3{ 0,1,0 }, 45, Vector3{ 0.95f, 0.32f, 0.95f }, stone);// cap
}

static void build_shrine() {
    s_torii.clear(); s_lanterns.clear(); s_blossoms.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    Color verm{ 200, 70, 45, 255 };

    // processional avenue of torii along the z-axis toward the central pavilion
    float zoff[6] = { 12, 8, 4, -5, -9, -13 };
    for (int i = 0; i < 6; i++) {
        Torii t;
        t.c = ctr + Vector3{ 0, 0, zoff[i] };
        t.yaw = 0.0f;
        t.w = (i == 0) ? 2.8f : 2.2f;
        t.h = (i == 0) ? 5.6f : 4.6f;
        t.col = verm;
        s_torii.push_back(t);
    }
    // iconic lone torii standing out in the water, off-axis
    { Torii t; t.c = ctr + Vector3{ -13.0f, 0, -3.0f }; t.yaw = 28.0f; t.w = 3.2f; t.h = 6.2f; t.col = verm; s_torii.push_back(t); }

    // stone lanterns flanking the avenue (their light boxes glow)
    float lz[5] = { 10, 6, 2, -2, -6 };
    for (int i = 0; i < 5; i++)
        for (int s = -1; s <= 1; s += 2) {
            Lantern L; L.b = ctr + Vector3{ 3.4f * s, 0, lz[i] };
            s_lanterns.push_back(L);
            s_wisps.push_back(L.b + Vector3{ 0, 1.55f, 0 });
        }

    // cherry-blossom trees at the corners
    Vector3 bp[3] = { ctr + Vector3{ -12, 0, 10 }, ctr + Vector3{ 12, 0, 7 }, ctr + Vector3{ 11, 0, -12 } };
    for (int i = 0; i < 3; i++) { Blossom b; b.base = bp[i]; b.h = 3.5f + i * 0.5f; b.r = 2.6f + i * 0.4f; s_blossoms.push_back(b); }

    s_wisps.push_back(ctr + Vector3{ 0, 3.0f, 0 });   // pavilion glow
    for (auto& w : s_wisps) {
        if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break;
        s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 6.0f });
    }
}

static void draw_shrine() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color verm{ 200, 70, 45, 255 }, wood{ 110, 64, 48, 255 }, stoneL{ 126, 124, 118, 255 },
          roof{ 66, 86, 78, 255 }, gold{ 200, 170, 90, 255 }, canopy{ 235, 165, 195, 255 };

    // torii gates + stone lanterns
    for (auto& t : s_torii) draw_torii(t);
    for (auto& L : s_lanterns) draw_lantern(L.b, stoneL);

    // cherry-blossom trees: a lit trunk + a soft pink dome canopy
    for (auto& b : s_blossoms) {
        DrawModelEx(s_cyl, b.base, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, b.h, 0.3f }, wood);
        DrawModelEx(s_dome, b.base + Vector3{ 0, b.h * 0.85f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ b.r, b.r * 0.8f, b.r }, canopy);
        DrawModelEx(s_dome, b.base + Vector3{ b.r * 0.4f, b.h * 0.7f, 0 }, Vector3{ 0,1,0 }, 90, Vector3{ b.r * 0.7f, b.r * 0.6f, b.r * 0.7f }, canopy);
    }

    // central shrine pavilion: stepped base + wood floor + 4 vermillion pillars + tiered pagoda roof
    DrawModelEx(s_column, ctr + Vector3{ 0, 0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 7.5f, 0.5f, 7.5f }, stoneL);
    DrawModelEx(s_column, ctr + Vector3{ 0, 0.7f, 0 },  Vector3{ 0,1,0 }, 0, Vector3{ 6.2f, 0.4f, 6.2f }, wood);
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            DrawModelEx(s_cyl, ctr + Vector3{ sx * 2.4f, 0.9f, sz * 2.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.32f, 3.6f, 0.32f }, verm);
    DrawModelEx(s_column, ctr + Vector3{ 0, 4.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 8.4f, 0.5f, 8.4f }, roof);   // wide eave
    DrawModelEx(s_column, ctr + Vector3{ 0, 5.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.8f, 0.5f, 5.8f }, roof);
    DrawModelEx(s_column, ctr + Vector3{ 0, 5.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f, 0.5f, 3.2f }, roof);
    DrawModelEx(s_cyl,    ctr + Vector3{ 0, 6.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.25f, 1.4f, 0.25f }, gold); // finial

    // warm paper-lantern glow (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        float fl = 0.6f + 0.4f * sinf(s_time * 3.0f + (float)i * 1.7f);
        DrawSphereEx(w, 0.18f + 0.05f * fl, 6, 6, Color{ 255, 190, 95, 150 });
        DrawSphereEx(w, 0.5f + 0.1f * fl, 8, 8, Color{ 255, 160, 70, 35 });
    }
    EndBlendMode();
}

// The Dragon Boneyard: a desolate ash basin strewn with colossal skeletons, built
// procedurally (no boss_arena.glb), per design/dragon_boneyard_design.md. Towering
// ribcages, horned skulls with dropped jaws, and loose bones -- all drawn from lit
// cylinder segments oriented between arbitrary endpoints (draw_bone_seg). Dry: this
// draw lays a pale ash floor. Lit by sickly corpse-light wisps.
static void draw_bone_seg(Vector3 a, Vector3 b, float r, Color col) {
    Vector3 d = Vector3Subtract(b, a);
    float len = Vector3Length(d);
    if (len < 0.0001f) return;
    Vector3 dir = Vector3Scale(d, 1.0f / len);
    Vector3 axis = Vector3CrossProduct(Vector3{ 0,1,0 }, dir);
    float al = Vector3Length(axis);
    float ang = acosf(Clamp(Vector3DotProduct(Vector3{ 0,1,0 }, dir), -1.0f, 1.0f)) * RAD2DEG;
    if (al < 0.0001f) axis = Vector3{ 1,0,0 }; else axis = Vector3Scale(axis, 1.0f / al);
    DrawModelEx(s_cyl, a, axis, ang, Vector3{ r, len, r }, col);   // unit cylinder spans its local +Y, scaled to len
}

static void draw_ribcage(Vector3 c, float yawDeg, float len, float ribH, float spread, int pairs, Color bone, Color dark) {
    float yr = yawDeg * DEG2RAD;
    auto W = [&](float lx, float ly, float lz) { return Vector3Add(c, rotate_y(Vector3{ lx, ly, lz }, yr)); };
    draw_bone_seg(W(0, 0.45f, -len * 0.5f), W(0, 0.45f, len * 0.5f), 0.5f, bone);   // spine
    for (int i = 0; i < pairs; i++) {
        float f = (pairs > 1) ? i / (float)(pairs - 1) : 0.5f;
        float z = -len * 0.42f + len * 0.84f * f;
        float taper = 1.0f - 0.35f * fabsf(f * 2.0f - 1.0f);     // ribs taller mid-cage
        float rh = ribH * (0.7f + 0.3f * taper), rs = spread * (0.7f + 0.3f * taper);
        DrawModelEx(s_cyl, W(0, 0.45f, z), Vector3{ 0,1,0 }, 0, Vector3{ 0.55f, 0.55f, 0.55f }, dark);   // vertebra knob
        for (int s = -1; s <= 1; s += 2) {
            int segs = 6;
            Vector3 prev = W(0, 0.45f, z);
            for (int k = 1; k <= segs; k++) {
                float t = k / (float)segs;
                float a = t * (PI * 0.82f);
                float lx = s * rs * sinf(a);
                float ly = 0.45f + rh * (1.0f - cosf(a)) / (1.0f - cosf(PI * 0.82f));
                Vector3 cur = W(lx, ly, z);
                draw_bone_seg(prev, cur, 0.24f * (1.1f - 0.55f * t), bone);   // rib tapers upward
                prev = cur;
            }
        }
    }
}

static void draw_skull(Vector3 c, float yawDeg, float s, Color bone, Color dark) {
    float yr = yawDeg * DEG2RAD;
    auto W = [&](float lx, float ly, float lz) { return Vector3Add(c, rotate_y(Vector3{ lx, ly, lz }, yr)); };
    DrawModelEx(s_dome,   W(0, s * 0.6f, 0),       Vector3{ 0,1,0 }, yawDeg, Vector3{ s * 1.1f, s * 0.95f, s * 1.3f }, bone);  // cranium
    DrawModelEx(s_column, W(0, s * 0.4f, s * 1.25f), Vector3{ 0,1,0 }, yawDeg, Vector3{ s * 0.95f, s * 0.7f, s * 1.5f }, bone); // snout
    DrawModelEx(s_column, W(0, s * 0.05f, s * 1.4f), Vector3{ 0,1,0 }, yawDeg, Vector3{ s * 0.85f, s * 0.35f, s * 1.5f }, dark);// dropped jaw
    for (int e = -1; e <= 1; e += 2)
        DrawModelEx(s_cyl, W(e * s * 0.45f, s * 0.62f, s * 0.6f), Vector3{ 0,0,1 }, 90, Vector3{ s * 0.26f, s * 0.4f, s * 0.26f }, dark);   // eye sockets
    for (int h = -1; h <= 1; h += 2) {                      // horns sweeping back
        Vector3 prev = W(h * s * 0.6f, s * 1.05f, -s * 0.2f);
        int segs = 5;
        for (int k = 1; k <= segs; k++) {
            float t = k / (float)segs;
            Vector3 cur = W(h * s * (0.6f + 0.55f * t), s * (1.05f + 0.9f * t), -s * (0.2f + 1.3f * t * t));
            draw_bone_seg(prev, cur, s * 0.20f * (1.0f - 0.6f * t), bone);
            prev = cur;
        }
    }
}

static void build_bones() {
    s_bones.clear(); s_ribcages.clear(); s_skulls.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(70077);

    s_ribcages.push_back(Ribcage{ ctr,                          12.0f, 16.0f, 5.5f, 5.0f, 7 });   // great central cage
    s_ribcages.push_back(Ribcage{ ctr + Vector3{ -15, 0, 10 },  50.0f,  8.0f, 3.0f, 3.0f, 5 });
    s_ribcages.push_back(Ribcage{ ctr + Vector3{  14, 0, -13 }, -40.0f,  9.0f, 3.4f, 3.2f, 5 });

    s_skulls.push_back(Skull{ ctr + Vector3{ 0, 0, -16 },   0.0f, 2.6f });   // great skull at the spine's head
    s_skulls.push_back(Skull{ ctr + Vector3{ 16, 0, 8 },  130.0f, 1.6f });

    // loose long bones scattered on the ash
    int placed = 0, attempts = 0, want = 12;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float ang = rand_range(0, 2 * PI), rad = rand_range(6.0f, boundary_radius - 2.0f);
        Vector3 a = ctr + Vector3{ sinf(ang) * rad, 0.3f, cosf(ang) * rad };
        if (a.z > ctr.z + 13.0f) continue;
        float dir = rand_range(0, 2 * PI), len = rand_range(2.5f, 6.0f);
        Vector3 b = a + Vector3{ sinf(dir) * len, rand_range(0.0f, 0.8f), cosf(dir) * len };
        Color col = (GetRandomValue(0, 1)) ? Color{ 210, 205, 190, 255 } : Color{ 188, 182, 168, 255 };
        s_bones.push_back(BonePiece{ a, b, rand_range(0.18f, 0.34f), col });
        placed++;
    }

    // sickly corpse-lights (marsh gas)
    static const float wAng[5] = { 40, 110, 180, 250, 320 };
    for (int i = 0; i < 5; i++) {
        float aa = wAng[i] * DEG2RAD, r = rand_range(7.0f, 18.0f);
        s_wisps.push_back(ctr + Vector3{ sinf(aa) * r, 1.0f, cosf(aa) * r });
    }
    for (auto& w : s_wisps) {
        if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break;
        s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 6.0f });
    }
}

static void draw_bones() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color ash{ 150, 146, 134, 255 }, ashD{ 120, 116, 106, 255 }, bone{ 206, 200, 185, 255 }, dark{ 92, 88, 80, 255 };

    // dry ash basin floor + low ash mounds
    DrawModelEx(s_column, Vector3{ ctr.x, -0.5f, ctr.z }, Vector3{ 0,1,0 }, 0, Vector3{ 130.0f, 1.0f, 130.0f }, ash);
    static const float duAng[5] = { 30, 100, 175, 250, 320 };
    for (int i = 0; i < 5; i++) {
        float a = duAng[i] * DEG2RAD, r = 15.0f + (i % 3) * 2.5f;
        DrawModelEx(s_dome, ctr + Vector3{ sinf(a) * r, -0.25f, cosf(a) * r }, Vector3{ 0,1,0 }, (float)(i * 50), Vector3{ 5.5f + i, 1.0f, 4.0f + i }, ashD);
    }

    // skeletons
    for (auto& rc : s_ribcages) draw_ribcage(rc.c, rc.yaw, rc.len, rc.ribH, rc.spread, rc.pairs, bone, dark);
    for (auto& sk : s_skulls)   draw_skull(sk.c, sk.yaw, sk.s, bone, dark);
    for (auto& bp : s_bones)    draw_bone_seg(bp.a, bp.b, bp.r, bp.col);

    // sickly corpse-light wisps (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        float ph = s_time * 1.3f + (float)i * 1.7f;
        Vector3 fp = w + Vector3{ 0.3f * sinf(ph), 0.3f * sinf(ph * 0.7f), 0.3f * cosf(ph * 1.1f) };
        float fl = 0.5f + 0.5f * sinf(s_time * 4.0f + i * 2.3f);
        DrawSphereEx(fp, 0.11f + 0.05f * fl, 6, 6, Color{ 170, 230, 110, 150 });
        DrawSphereEx(fp, 0.30f + 0.10f * fl, 8, 8, Color{ 120, 200, 80, 35 });
    }
    EndBlendMode();
}

// The Crystal Cavern: a dark cave of colossal glowing crystal formations, built
// procedurally (no boss_arena.glb), per design/crystal_cavern_design.md. Clusters of
// faceted shards (lit hex cones) jut from dark rocky mounds and hang from the ceiling,
// around a giant central geode, glowing amethyst/magenta/cyan and mirrored in the
// reused cool reflective water.
static void build_geode() {
    s_crys.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(40404);
    Color cols[3]  = { { 150, 90, 210, 255 }, { 210, 80, 180, 255 }, { 90, 180, 220, 255 } };
    Color glows[3] = { { 185, 120, 255, 255 }, { 255, 120, 210, 255 }, { 130, 215, 255, 255 } };

    auto add_cluster = [&](Vector3 base, float scale, bool ceiling) {
        int n = GetRandomValue(4, 7);
        for (int i = 0; i < n; i++) {
            Crys c;
            c.base = base;
            c.r = rand_range(0.22f, 0.55f) * scale;
            c.h = rand_range(1.4f, 3.8f) * scale;
            c.tilt = rand_range(0.0f, 26.0f);
            c.tiltYaw = rand_range(0.0f, 360.0f);
            c.ceiling = ceiling;
            int ci = GetRandomValue(0, 2);
            c.col = cols[ci]; c.glow = glows[ci];
            s_crys.push_back(c);
        }
        if ((int)s_crystal_lights.size() < MAX_CRYSTAL_LIGHTS) {
            s_wisps.push_back(Vector3{ base.x, ceiling ? base.y - 1.5f : base.y + 1.5f, base.z });
            s_crystal_lights.push_back(Vector4{ base.x, ceiling ? 2.0f : 1.0f, base.z, 5.5f });
        }
    };

    add_cluster(ctr, 2.6f, false);                         // giant central geode
    int placed = 0, attempts = 0, want = 10;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float ang = rand_range(0, 2 * PI), rad = rand_range(6.0f, boundary_radius - 2.0f);
        Vector3 b = ctr + Vector3{ sinf(ang) * rad, 0, cosf(ang) * rad };
        if (b.z > ctr.z + 13.0f) continue;
        add_cluster(b, rand_range(0.7f, 1.6f), false);
        placed++;
    }
    for (int i = 0; i < 6; i++) {                          // hanging ceiling crystals
        float ang = rand_range(0, 2 * PI), rad = rand_range(4.0f, boundary_radius - 4.0f);
        Vector3 b = ctr + Vector3{ sinf(ang) * rad, rand_range(9.0f, 14.0f), cosf(ang) * rad };
        add_cluster(b, rand_range(0.8f, 1.4f), true);
    }
}

static void draw_geode() {
    if (!s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color rock{ 46, 42, 58, 255 }, rockD{ 32, 30, 42, 255 };

    // dark rocky mounds the crystals grow from
    static const float mAng[6] = { 20, 80, 140, 200, 265, 325 };
    for (int i = 0; i < 6; i++) {
        float a = mAng[i] * DEG2RAD, r = 12.0f + (i % 3) * 3.0f;
        DrawModelEx(s_dome, ctr + Vector3{ sinf(a) * r, -0.6f, cosf(a) * r }, Vector3{ 0,1,0 }, (float)(i * 40), Vector3{ 5.0f + i, 2.2f, 4.0f + i }, (i % 2) ? rock : rockD);
    }
    DrawModelEx(s_dome, ctr + Vector3{ 0, -0.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.0f, 2.6f, 6.0f }, rock);   // base under the geode

    // crystal shards (lit hex cones); ceiling ones flipped to hang down
    for (auto& c : s_crys) {
        rlPushMatrix();
        rlTranslatef(c.base.x, c.base.y, c.base.z);
        rlRotatef(c.tiltYaw, 0, 1, 0);
        rlRotatef(c.ceiling ? 180.0f + c.tilt : c.tilt, 1, 0, 0);
        DrawModelEx(s_cone, Vector3{ 0, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ c.r, c.h, c.r }, c.col);
        rlPopMatrix();
    }

    // additive glow at the shard tips
    BeginBlendMode(BLEND_ADDITIVE);
    for (auto& c : s_crys) {
        Vector3 tip = c.base + Vector3{ 0, (c.ceiling ? -c.h : c.h) * 0.85f, 0 };
        float fl = 0.6f + 0.4f * sinf(s_time * 2.5f + c.base.x * 0.3f + c.base.z * 0.2f);
        DrawSphereEx(tip, 0.18f + 0.4f * c.r, 6, 6, Color{ c.glow.r, c.glow.g, c.glow.b, (unsigned char)(85 * fl) });
    }
    EndBlendMode();
}

// The Overgrown Temple: a ruined jungle temple swallowed by nature, built
// procedurally (no boss_arena.glb), per design/overgrown_temple_design.md. A broken
// mossy stepped temple with leaning pillars + a doorway, draped over by colossal
// gnarled roots (chained cylinder segments via draw_bone_seg) + hanging vines, lush
// fern/tree foliage, and drifting fireflies, over the reused jade reflecting pool.
static void draw_root(Vector3 a, Vector3 b, float archH, float r, Color col) {
    int segs = 8;
    Vector3 prev = a;
    for (int k = 1; k <= segs; k++) {
        float t = k / (float)segs;
        Vector3 p = Vector3Lerp(a, b, t);
        p.y += sinf(t * PI) * archH;                       // arch up over the middle
        p.x += 0.6f * sinf(t * PI * 2.0f + a.x);           // gnarl wiggle
        p.z += 0.6f * cosf(t * PI * 2.0f + a.z);
        draw_bone_seg(prev, p, r * (1.0f - 0.45f * t), col);   // taper into the ruin
        prev = p;
    }
}

static void build_temple() {
    s_roots.clear(); s_foliage.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(20251);

    // colossal roots arcing in from the jungle edge toward the ruin
    static const float rAng[5] = { 30, 95, 165, 235, 310 };
    for (int i = 0; i < 5; i++) {
        float a = rAng[i] * DEG2RAD;
        Vector3 edge = ctr + Vector3{ sinf(a) * (boundary_radius - 1.5f), 0, cosf(a) * (boundary_radius - 1.5f) };
        Vector3 inner = ctr + Vector3{ sinf(a) * 3.5f, 0.6f, cosf(a) * 3.5f };
        s_roots.push_back(Root{ edge, inner, rand_range(3.0f, 6.0f), rand_range(0.5f, 0.9f) });
    }

    // foliage: low fern/bush domes + a few jungle trees (entrance aisle + temple kept clear)
    int placed = 0, attempts = 0, want = 16;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float ang = rand_range(0, 2 * PI), rad = rand_range(5.0f, boundary_radius - 1.5f);
        Vector3 p = ctr + Vector3{ sinf(ang) * rad, 0, cosf(ang) * rad };
        if (p.z > ctr.z + 13.0f) continue;
        if (Vector3Distance(p, ctr) < 4.0f) continue;
        Foliage f;
        f.pos = p;
        f.tree = (GetRandomValue(0, 4) == 0);
        f.r = f.tree ? rand_range(2.5f, 3.6f) : rand_range(0.8f, 1.8f);
        f.col = (GetRandomValue(0, 1)) ? Color{ 54, 110, 56, 255 } : Color{ 72, 128, 62, 255 };
        s_foliage.push_back(f);
        placed++;
    }

    // fireflies drifting over the pool + ruin
    static const float wAng[7] = { 20, 70, 120, 175, 230, 290, 335 };
    for (int i = 0; i < 7; i++) {
        float a = wAng[i] * DEG2RAD, r = rand_range(5.0f, 16.0f);
        s_wisps.push_back(ctr + Vector3{ sinf(a) * r, 1.2f + rand_range(0.0f, 1.5f), cosf(a) * r });
    }
    for (auto& w : s_wisps) {
        if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break;
        s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 5.0f });
    }
}

static void draw_temple() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color moss{ 86, 96, 78, 255 }, stoneD{ 58, 66, 52, 255 }, bark{ 78, 56, 40, 255 }, vine{ 76, 116, 58, 255 };

    // ruined stepped temple: broken tiers (offset/rotated) + leaning pillars + a doorway
    DrawModelEx(s_column, ctr + Vector3{ 0, 0.3f, 0 },          Vector3{ 0,1,0 }, 6,  Vector3{ 7.0f, 0.6f, 7.0f }, moss);
    DrawModelEx(s_column, ctr + Vector3{ 0.4f, 0.85f, -0.3f },  Vector3{ 0,1,0 }, -4, Vector3{ 5.4f, 0.6f, 5.4f }, stoneD);
    DrawModelEx(s_column, ctr + Vector3{ -0.3f, 1.4f, 0.2f },   Vector3{ 0,1,0 }, 9,  Vector3{ 3.8f, 0.6f, 3.8f }, moss);
    struct PL { float x, z, h, lean, yaw; };
    PL pls[6] = { {-2.6f,-2.6f,3.6f,0,0},{2.6f,-2.6f,2.2f,6,30},{2.6f,2.6f,3.2f,-4,0},{-2.6f,2.6f,1.4f,10,0},{0,-3.0f,2.8f,0,0},{0,3.0f,1.0f,14,0} };
    for (auto& p : pls) {
        rlPushMatrix();
        rlTranslatef(ctr.x + p.x, 1.7f, ctr.z + p.z);
        rlRotatef(p.yaw, 0, 1, 0); rlRotatef(p.lean, 1, 0, 0);
        DrawModelEx(s_cyl, Vector3{ 0, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.45f, p.h, 0.45f }, moss);
        rlPopMatrix();
    }
    for (int s = -1; s <= 1; s += 2)
        DrawModelEx(s_column, ctr + Vector3{ s * 1.6f, 3.0f, 3.4f }, Vector3{ 0,1,0 }, 4.0f * s, Vector3{ 0.6f, 4.0f, 0.6f }, stoneD);
    DrawModelEx(s_column, ctr + Vector3{ 0, 5.0f, 3.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 4.2f, 0.6f, 0.7f }, moss);   // lintel
    draw_bone_seg(ctr + Vector3{ -6, 0.4f, 4 }, ctr + Vector3{ -10, 0.4f, 6.5f }, 0.5f, stoneD);   // toppled columns
    draw_bone_seg(ctr + Vector3{ 7, 0.4f, -3 }, ctr + Vector3{ 11, 0.5f, -5 }, 0.5f, moss);

    // colossal roots arcing over the ruin
    for (auto& r : s_roots) draw_root(r.a, r.b, r.archH, r.r, bark);

    // foliage: fern/bush domes + jungle trees
    for (auto& f : s_foliage) {
        if (f.tree) {
            DrawModelEx(s_cyl, f.pos, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 4.5f, 0.4f }, bark);
            DrawModelEx(s_dome, f.pos + Vector3{ 0, 4.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ f.r, f.r * 0.9f, f.r }, f.col);
            DrawModelEx(s_dome, f.pos + Vector3{ f.r * 0.4f, 3.3f, 0 }, Vector3{ 0,1,0 }, 120, Vector3{ f.r * 0.7f, f.r * 0.6f, f.r * 0.7f }, f.col);
        } else {
            DrawModelEx(s_dome, f.pos, Vector3{ 0,1,0 }, (float)(((int)(f.pos.x * 7)) % 360), Vector3{ f.r, f.r * 0.7f, f.r }, f.col);
        }
    }

    // hanging vines swaying from the root arches
    for (auto& r : s_roots) {
        Vector3 mid = Vector3Lerp(r.a, r.b, 0.5f); mid.y += r.archH;
        Vector3 prev = mid;
        for (int k = 1; k <= 5; k++) {
            float t = k / 5.0f;
            Vector3 cur = mid + Vector3{ 0.3f * sinf(s_time * 1.5f + r.a.x + t * 3.0f), -t * 3.0f, 0.0f };
            draw_bone_seg(prev, cur, 0.08f, vine);
            prev = cur;
        }
    }

    // fireflies (additive warm-green)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        float ph = s_time * 1.1f + (float)i * 1.7f;
        Vector3 fp = w + Vector3{ 0.6f * sinf(ph), 0.4f * sinf(ph * 0.6f), 0.6f * cosf(ph * 0.8f) };
        float fl = 0.5f + 0.5f * sinf(s_time * 5.0f + i * 2.3f);
        DrawSphereEx(fp, 0.06f + 0.04f * fl, 5, 5, Color{ 210, 235, 120, 160 });
    }
    EndBlendMode();
}

// The Abandoned Mine: a dark flooded mineshaft, built procedurally (no boss_arena.glb),
// per design/abandoned_mine_design.md. Receding timber support frames, mine-cart rails
// + derelict carts, a central pit-head winch tower, wooden scaffolds, and glowing gold
// ore veins (s_cone shards in dark rock), lit by lanterns over the reused dark water.
static void draw_frame(Vector3 c, float yaw, float w, float h, Color wood, Color dark) {
    rlPushMatrix();
    rlTranslatef(c.x, 0, c.z);
    rlRotatef(yaw, 0, 1, 0);
    for (int s = -1; s <= 1; s += 2)
        DrawModelEx(s_cyl, Vector3{ w * s, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, h, 0.3f }, wood);   // posts
    DrawModelEx(s_column, Vector3{ 0, h, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w * 2.4f, 0.4f, 0.5f }, dark);  // top beam
    for (int s = -1; s <= 1; s += 2)
        DrawModelEx(s_column, Vector3{ w * 0.65f * s, h * 0.78f, 0 }, Vector3{ 0,0,1 }, 35.0f * s, Vector3{ w * 0.9f, 0.22f, 0.32f }, wood);   // braces
    rlPopMatrix();
}

static void draw_cart(Vector3 c, float yaw, bool tipped, Color body, Color iron) {
    rlPushMatrix();
    rlTranslatef(c.x, 0, c.z);
    rlRotatef(yaw, 0, 1, 0);
    if (tipped) rlRotatef(62.0f, 1, 0, 0);
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            DrawModelEx(s_cyl, Vector3{ sx * 0.55f, 0.25f, sz * 0.7f }, Vector3{ 0,0,1 }, 90, Vector3{ 0.25f, 0.16f, 0.25f }, iron);   // wheels
    DrawModelEx(s_column, Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.3f, 0.2f, 1.8f }, body);     // floor
    for (int sz = -1; sz <= 1; sz += 2)
        DrawModelEx(s_column, Vector3{ 0, 0.9f, sz * 0.85f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.3f, 0.7f, 0.15f }, body);   // ends
    for (int sx = -1; sx <= 1; sx += 2)
        DrawModelEx(s_column, Vector3{ sx * 0.62f, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.15f, 0.7f, 1.8f }, body);   // sides
    rlPopMatrix();
}

static void build_mine() {
    s_frames.clear(); s_carts.clear(); s_veins.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(31415);

    float zoff[6] = { 12, 8, 4, -4, -8, -12 };
    for (int i = 0; i < 6; i++)
        s_frames.push_back(Frame{ ctr + Vector3{ 0, 0, zoff[i] }, 0.0f, 2.4f, 4.2f });   // shaft avenue
    s_frames.push_back(Frame{ ctr + Vector3{ -10, 0, 6 },  60.0f, 2.0f, 3.6f });
    s_frames.push_back(Frame{ ctr + Vector3{ 11, 0, -4 }, -50.0f, 2.0f, 3.8f });
    s_frames.push_back(Frame{ ctr + Vector3{ -9, 0, -10 }, 25.0f, 1.8f, 3.4f });

    s_carts.push_back(Cart{ ctr + Vector3{ 0.0f, 0, 9.5f }, 0.0f, false });
    s_carts.push_back(Cart{ ctr + Vector3{ 0.0f, 0, -6.0f }, 0.0f, false });
    s_carts.push_back(Cart{ ctr + Vector3{ -9.0f, 0, 7.5f }, 50.0f, true });    // tipped
    s_carts.push_back(Cart{ ctr + Vector3{ 10.0f, 0, 9.0f }, 20.0f, false });

    int placed = 0, attempts = 0, want = 6;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float ang = rand_range(0, 2 * PI), rad = rand_range(8.0f, boundary_radius - 2.0f);
        Vector3 b = ctr + Vector3{ sinf(ang) * rad, 0, cosf(ang) * rad };
        if (b.z > ctr.z + 13.0f) continue;
        s_veins.push_back(OreVein{ b, rand_range(0.7f, 1.3f) });
        s_wisps.push_back(b + Vector3{ 0, 1.2f, 0 });
        placed++;
    }
    for (int i = 0; i < 6; i += 2)
        s_wisps.push_back(ctr + Vector3{ 0, 3.8f, zoff[i] });   // avenue lanterns
    for (auto& w : s_wisps) {
        if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break;
        s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 6.0f });
    }
}

static void draw_mine() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color wood{ 96, 66, 42, 255 }, dark{ 70, 48, 30, 255 }, rock{ 54, 50, 48, 255 }, rockD{ 40, 37, 36, 255 },
          iron{ 116, 112, 116, 255 }, ore{ 230, 180, 70, 255 };

    // mine-cart rail tracks
    auto draw_track = [&](Vector3 a, Vector3 b) {
        a.y = 0.06f; b.y = 0.06f;
        Vector3 d = Vector3Subtract(b, a); float len = Vector3Length(d);
        if (len < 0.01f) return;
        Vector3 dir = Vector3Scale(d, 1.0f / len);
        Vector3 perp{ -dir.z, 0, dir.x };
        float yaw = atan2f(dir.x, dir.z) * RAD2DEG;
        Vector3 mid = Vector3Lerp(a, b, 0.5f);
        for (int s = -1; s <= 1; s += 2)
            DrawModelEx(s_column, Vector3Add(mid, Vector3Scale(perp, 0.5f * s)), Vector3{ 0,1,0 }, yaw, Vector3{ 0.1f, 0.12f, len }, iron);
        int ties = (int)(len / 1.2f); if (ties < 1) ties = 1;
        for (int k = 0; k <= ties; k++)
            DrawModelEx(s_column, Vector3Add(Vector3Lerp(a, b, k / (float)ties), Vector3{ 0,-0.02f,0 }), Vector3{ 0,1,0 }, yaw, Vector3{ 1.3f, 0.1f, 0.25f }, wood);
    };
    draw_track(ctr + Vector3{ 0, 0, 13 }, ctr + Vector3{ 0, 0, -13 });
    draw_track(ctr + Vector3{ 0, 0, 2 }, ctr + Vector3{ 11, 0, 9 });

    // central pit-head winch tower (leaning posts + platform + pulley + pit rim)
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            draw_bone_seg(ctr + Vector3{ sx * 2.2f, 0, sz * 2.2f }, ctr + Vector3{ sx * 0.8f, 7.0f, sz * 0.8f }, 0.28f, dark);
    DrawModelEx(s_column, ctr + Vector3{ 0, 7.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 0.4f, 2.4f }, wood);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 6.2f, 0 }, Vector3{ 0,0,1 }, 90, Vector3{ 0.6f, 0.4f, 0.6f }, iron);   // pulley
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 0.4f, 2.4f }, rockD);   // pit rim

    // timber frames + ore carts
    for (auto& f : s_frames) draw_frame(f.c, f.yaw, f.w, f.h, wood, dark);
    for (auto& c : s_carts)  draw_cart(c.c, c.yaw, c.tipped, dark, iron);

    // two scaffolding platforms on posts
    for (int s = 0; s < 2; s++) {
        Vector3 p = ctr + Vector3{ s ? 12.0f : -12.0f, 0, s ? -8.0f : 8.0f };
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
                DrawModelEx(s_cyl, p + Vector3{ sx * 1.4f, 0, sz * 1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 3.0f, 0.18f }, dark);
        DrawModelEx(s_column, p + Vector3{ 0, 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f, 0.25f, 2.4f }, wood);
    }

    // ore veins: dark rock mound + a cluster of small gold crystals
    for (auto& v : s_veins) {
        DrawModelEx(s_dome, v.base + Vector3{ 0, -0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.8f * v.scale, 1.4f * v.scale, 1.8f * v.scale }, rock);
        SetRandomSeed((int)(v.base.x * 13.7f + v.base.z * 7.1f) + 9000);
        for (int i = 0; i < 4; i++) {
            float a = rand_range(0, 2 * PI), rr = rand_range(0.2f, 0.8f) * v.scale;
            Vector3 b = v.base + Vector3{ cosf(a) * rr, 0.4f * v.scale, sinf(a) * rr };
            DrawModelEx(s_cone, b, Vector3{ 0,1,0 }, rand_range(0, 360), Vector3{ 0.22f * v.scale, rand_range(0.7f, 1.4f) * v.scale, 0.22f * v.scale }, ore);
        }
    }

    // ore + lantern glow (additive warm)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        float fl = 0.6f + 0.4f * sinf(s_time * 4.0f + (float)i * 1.9f);
        DrawSphereEx(w, 0.14f + 0.05f * fl, 6, 6, Color{ 255, 190, 90, 150 });
        DrawSphereEx(w, 0.45f + 0.10f * fl, 8, 8, Color{ 255, 150, 60, 35 });
    }
    EndBlendMode();
}

// The Astral Observatory: a moonlit stargazer's platform, built procedurally (no
// boss_arena.glb), per design/astral_observatory_design.md. A giant armillary sphere
// (concentric lit tori spinning on different axes around a glowing sun-orb with
// orbiting planets), ringed by telescopes on tripods and celestial globe pedestals,
// under a starry sky + pale moon, over the reused reflective floor.
static void build_obs() {
    s_scopes.clear(); s_globes.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(19191);

    static const float tAng[4] = { 40, 130, 220, 310 };
    for (int i = 0; i < 4; i++) {
        float a = tAng[i] * DEG2RAD, r = 9.0f + (i % 2) * 2.0f;
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        if (p.z > ctr.z + 13.0f) p.z = ctr.z + 13.0f;
        s_scopes.push_back(Scope{ p, tAng[i] + 90.0f, rand_range(40.0f, 62.0f) });
    }
    static const float gAng[5] = { 15, 80, 160, 235, 300 };
    Color gcol[3] = { { 120,150,210,255 }, { 200,150,120,255 }, { 150,200,180,255 } };
    for (int i = 0; i < 5; i++) {
        float a = gAng[i] * DEG2RAD, r = rand_range(6.0f, 14.0f);
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        if (p.z > ctr.z + 13.0f) continue;
        s_globes.push_back(Globe{ p, gcol[i % 3], (i % 2 == 0) });
        s_wisps.push_back(p + Vector3{ 0, 1.9f, 0 });
    }
    s_wisps.push_back(ctr + Vector3{ 0, 3.5f, 0 });   // sun-orb glow
    for (auto& w : s_wisps) {
        if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break;
        s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 5.0f });
    }
}

static void draw_obs() {
    if (!s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color brass{ 184, 150, 92, 255 }, brassD{ 150, 116, 64, 255 }, stone{ 70, 74, 92, 255 }, dark{ 48, 50, 66, 255 };
    Vector3 hub = ctr + Vector3{ 0, 3.5f, 0 };

    // plinth + central axle under the armillary
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 0.4f, 2.6f }, stone);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 3.0f, 0.4f }, brassD);

    // armillary rings: concentric tori on different planes, each spinning on its own axis
    struct R { float rad, tiltX, tiltZ, speed; };
    R rings[4] = { { 3.0f, 12, 0, 22 }, { 3.8f, 90, 0, -16 }, { 4.6f, 55, 35, 13 }, { 5.3f, 90, 90, -10 } };
    for (int i = 0; i < 4; i++) {
        rlPushMatrix();
        rlTranslatef(hub.x, hub.y, hub.z);
        rlRotatef(rings[i].tiltX, 1, 0, 0);
        rlRotatef(rings[i].tiltZ, 0, 0, 1);
        rlRotatef(s_time * rings[i].speed, 0, 1, 0);
        DrawModelEx(s_torus, Vector3{ 0,0,0 }, Vector3{ 0,1,0 }, 0, Vector3{ rings[i].rad, rings[i].rad, rings[i].rad }, (i % 2) ? brass : brassD);
        rlPopMatrix();
    }

    // sun-orb (additive) + orbiting planets
    BeginBlendMode(BLEND_ADDITIVE);
    DrawSphereEx(hub, 0.9f + 0.08f * sinf(s_time * 3.0f), 12, 12, Color{ 255, 210, 110, 180 });
    DrawSphereEx(hub, 1.8f, 12, 12, Color{ 255, 170, 70, 40 });
    EndBlendMode();
    Color planet[3] = { { 120,170,230,255 }, { 220,150,110,255 }, { 160,210,170,255 } };
    for (int i = 0; i < 3; i++) {
        float a = s_time * (0.5f + 0.25f * i) + i * 2.1f, r = 2.4f + i * 1.1f;
        DrawSphereEx(hub + Vector3{ cosf(a) * r, 0.3f * sinf(a * 1.3f), sinf(a) * r }, 0.22f + 0.05f * i, 8, 8, planet[i]);
    }

    // telescopes: splayed tripod legs + a tube tilted up + aperture
    for (auto& s : s_scopes) {
        rlPushMatrix();
        rlTranslatef(s.c.x, 0, s.c.z);
        rlRotatef(s.yaw, 0, 1, 0);
        for (int l = 0; l < 3; l++) {
            float la = l * 120.0f * DEG2RAD;
            DrawModelEx(s_cyl, Vector3{ sinf(la) * 0.7f, 0, cosf(la) * 0.7f }, Vector3{ 0,0,1 }, 12.0f, Vector3{ 0.1f, 1.8f, 0.1f }, brassD);
        }
        rlTranslatef(0, 1.7f, 0);
        rlRotatef(s.pitch, 1, 0, 0);
        DrawModelEx(s_cyl, Vector3{ 0, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, 2.6f, 0.22f }, brass);
        DrawModelEx(s_cyl, Vector3{ 0, 2.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.4f, 0.3f }, brassD);
        rlPopMatrix();
    }

    // celestial globes on pedestals (+ a ring for ringed ones)
    for (auto& g : s_globes) {
        DrawModelEx(s_cyl, g.c + Vector3{ 0,0.1f,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.2f, 0.5f }, dark);
        DrawModelEx(s_cyl, g.c + Vector3{ 0,0.6f,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.25f, 1.2f, 0.25f }, stone);
        DrawSphereEx(g.c + Vector3{ 0, 2.0f, 0 }, 0.55f, 10, 10, g.col);
        if (g.ringed) {
            rlPushMatrix();
            rlTranslatef(g.c.x, g.c.y + 2.0f, g.c.z);
            rlRotatef(24.0f, 1, 0, 0);
            DrawModelEx(s_torus, Vector3{ 0,0,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.85f, 0.85f, 0.85f }, brass);
            rlPopMatrix();
        }
    }

    // stars high in the sky + soft globe glow (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    SetRandomSeed(7);
    for (int i = 0; i < 80; i++) {
        float a = rand_range(0, 2 * PI), el = rand_range(0.3f, 1.2f);
        Vector3 sp = ctr + Vector3{ cosf(a) * cosf(el) * 60.0f, 18.0f + sinf(el) * 40.0f, sinf(a) * cosf(el) * 60.0f };
        float tw = 0.5f + 0.5f * sinf(s_time * 2.0f + i * 1.7f);
        DrawSphereEx(sp, 0.25f + 0.2f * tw, 4, 4, Color{ 230, 235, 255, (unsigned char)(120 * tw + 50) });
    }
    for (auto& g : s_globes)
        DrawSphereEx(g.c + Vector3{ 0, 2.0f, 0 }, 0.8f, 8, 8, Color{ g.col.r, g.col.g, g.col.b, 30 });
    EndBlendMode();
}

// The Forgotten Archive: a vast abandoned library, built procedurally (no
// boss_arena.glb), per design/forgotten_archive_design.md. Towering bookshelves of
// coloured book-spines in aisles, reading lecterns, candelabra, a central rotunda of
// stacked giant tomes, drifting spectral books, and broken high walls, on a dry wood
// floor in dusty warm light. All from the lit cube/cylinder primitives.
static const Color BOOK[6] = { { 150,50,45,255 }, { 52,72,140,255 }, { 50,112,72,255 }, { 182,150,80,255 }, { 120,82,52,255 }, { 96,60,120,255 } };
static const float CANDLE_ANG[5] = { 15, 85, 160, 235, 310 };

static void draw_shelf(Vector3 c, float yaw, float w, float h, float lean, Color wood, Color trim) {
    rlPushMatrix();
    rlTranslatef(c.x, 0, c.z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(lean, 1, 0, 0);
    float d = 0.9f;
    DrawModelEx(s_column, Vector3{ 0, h * 0.5f, -d * 0.42f }, Vector3{ 0,1,0 }, 0, Vector3{ w, h, 0.12f }, trim);   // back
    for (int s = -1; s <= 1; s += 2)
        DrawModelEx(s_column, Vector3{ w * 0.5f * s, h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.14f, h, d }, wood);   // sides
    DrawModelEx(s_column, Vector3{ 0, h - 0.08f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, 0.16f, d }, wood);   // top
    DrawModelEx(s_column, Vector3{ 0, 0.08f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, 0.16f, d }, wood);       // bottom
    int rows = 4;
    for (int r = 0; r < rows; r++) {
        float y0 = 0.16f + (h - 0.32f) * r / (float)rows;
        float y1 = 0.16f + (h - 0.32f) * (r + 1) / (float)rows;
        DrawModelEx(s_column, Vector3{ 0, y1, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, 0.07f, d }, trim);   // shelf board
        int nb = 11;
        for (int i = 0; i < nb; i++) {
            float hx = sinf(r * 13.1f + i * 4.7f), hh = sinf(r * 7.3f + i * 9.1f);
            float bw = (w - 0.36f) / nb;
            float bh = (y1 - y0) * (0.62f + 0.28f * fabsf(hh));
            float bx = -w * 0.5f + 0.18f + (i + 0.5f) * bw;
            DrawModelEx(s_column, Vector3{ bx, y0 + bh * 0.5f, 0.16f }, Vector3{ 0,1,0 }, hx * 6.0f, Vector3{ bw * 0.82f, bh, 0.46f }, BOOK[((int)(fabsf(hx) * 6.0f)) % 6]);
        }
    }
    rlPopMatrix();
}

static void draw_lectern(Vector3 c, float yaw, Color wood, Color parch) {
    rlPushMatrix();
    rlTranslatef(c.x, 0, c.z);
    rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_cyl, Vector3{ 0, 0.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.45f, 0.2f, 0.45f }, wood);   // base
    DrawModelEx(s_cyl, Vector3{ 0, 0.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 1.2f, 0.16f }, wood);   // post
    rlTranslatef(0, 1.3f, 0);
    rlRotatef(28.0f, 1, 0, 0);
    DrawModelEx(s_column, Vector3{ 0, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 0.08f, 0.7f }, wood);    // desk
    for (int s = -1; s <= 1; s += 2)
        DrawModelEx(s_column, Vector3{ 0.22f * s, 0.06f, 0 }, Vector3{ 0,1,0 }, 6.0f * s, Vector3{ 0.4f, 0.04f, 0.6f }, parch);   // open pages
    rlPopMatrix();
}

static void build_lib() {
    s_shelves.clear(); s_lecterns.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(12321);

    // bookshelves in a grid of aisles (rotunda + spawn aisle kept clear)
    for (int gx = -2; gx <= 2; gx++)
        for (int gz = -2; gz <= 1; gz++) {
            Vector3 p = ctr + Vector3{ gx * 6.0f, 0, gz * 5.5f };
            if (Vector3Distance(p, ctr) < 5.5f) continue;
            if (p.z > ctr.z + 12.0f) continue;
            if (Vector3Distance(p, ctr) > boundary_radius - 3.0f) continue;
            Shelf s; s.c = p; s.yaw = 0.0f; s.w = 4.6f; s.h = 5.0f + ((gx + gz) & 1) * 0.8f;
            s.lean = (GetRandomValue(0, 5) == 0) ? rand_range(-7.0f, 7.0f) : 0.0f;
            s_shelves.push_back(s);
        }
    // lecterns
    static const float lA[4] = { 35, 120, 215, 305 };
    for (int i = 0; i < 4; i++) { float a = lA[i] * DEG2RAD, r = rand_range(6.0f, 11.0f); s_lecterns.push_back(Lectern{ ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }, lA[i] }); }
    // candelabra glow points (ring)
    for (int i = 0; i < 5; i++) {
        float a = CANDLE_ANG[i] * DEG2RAD, r = 9.0f + (i % 2) * 2.5f;
        s_wisps.push_back(ctr + Vector3{ sinf(a) * r, 2.7f, cosf(a) * r });
    }
    for (auto& w : s_wisps) {
        if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break;
        s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 6.0f });
    }
}

static void draw_lib() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color wood{ 80, 56, 38, 255 }, trim{ 58, 40, 26, 255 }, floorC{ 64, 46, 32, 255 },
          stone{ 92, 86, 78, 255 }, stoneD{ 66, 62, 56, 255 }, parch{ 200, 185, 150, 255 };

    // dry plank floor + broken high perimeter walls (interior hall; fog hides the open top)
    DrawModelEx(s_column, Vector3{ ctr.x, -0.5f, ctr.z }, Vector3{ 0,1,0 }, 0, Vector3{ 120.0f, 1.0f, 120.0f }, floorC);
    int segs = 40; float wr = boundary_radius - 1.0f, segW = 2.0f * wr * sinf(PI / segs) * 1.2f;
    for (int i = 0; i < segs; i++) {
        if (i % 5 == 0) continue;
        float deg = i * 360.0f / segs, a = deg * DEG2RAD;
        Vector3 p = ctr + Vector3{ sinf(a) * wr, 0, cosf(a) * wr };
        float h = 6.0f + (float)((i * 3) % 5) * 0.6f;
        DrawModelEx(s_column, Vector3{ p.x, h * 0.5f, p.z }, Vector3{ 0,1,0 }, deg, Vector3{ segW, h, 0.8f }, (i % 2) ? stone : stoneD);
    }

    // bookshelves + lecterns
    for (auto& s : s_shelves) draw_shelf(s.c, s.yaw, s.w, s.h, s.lean, wood, trim);
    for (auto& l : s_lecterns) draw_lectern(l.c, l.yaw, wood, parch);

    // central reading rotunda: dais + ring pillars + a pillar of stacked giant tomes
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f, 0.4f, 3.2f }, stoneD);
    for (int i = 0; i < 6; i++) { float a = i * 60.0f * DEG2RAD; DrawModelEx(s_cyl, ctr + Vector3{ sinf(a) * 2.7f, 1.2f, cosf(a) * 2.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, 2.4f, 0.22f }, wood); }
    for (int k = 0; k < 8; k++)
        DrawModelEx(s_column, ctr + Vector3{ 0, 0.55f + k * 0.5f, 0 }, Vector3{ 0,1,0 }, (float)(k * 25), Vector3{ 1.3f - 0.06f * k, 0.45f, 1.1f - 0.05f * k }, BOOK[k % 6]);

    // candelabra: tall stand + three candle flames (additive)
    for (int i = 0; i < 5; i++) {
        float a = CANDLE_ANG[i] * DEG2RAD, r = 9.0f + (i % 2) * 2.5f;
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        DrawModelEx(s_cyl, Vector3{ p.x, 1.3f, p.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 2.6f, 0.12f }, trim);
        DrawModelEx(s_cyl, Vector3{ p.x, 0.15f, p.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.3f, 0.4f }, trim);
        DrawModelEx(s_column, Vector3{ p.x, 2.55f, p.z }, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f, 0.1f, 0.18f }, trim);   // cross-arm
    }
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 5; i++) {
        float a = CANDLE_ANG[i] * DEG2RAD, r = 9.0f + (i % 2) * 2.5f;
        Vector3 p = ctr + Vector3{ sinf(a) * r, 2.7f, cosf(a) * r };
        for (int f = -1; f <= 1; f++) {
            Vector3 fp = p + Vector3{ f * 0.5f, 0.0f, 0 };
            float fl = 0.6f + 0.4f * sinf(s_time * 7.0f + i * 2.0f + f);
            DrawSphereEx(fp + Vector3{ 0, 0.12f * fl, 0 }, 0.1f + 0.04f * fl, 6, 6, Color{ 255, 190, 95, 150 });
            DrawSphereEx(fp, 0.35f, 8, 8, Color{ 255, 150, 60, 28 });
        }
    }
    // drifting spectral tomes (additive), placed deterministically by index
    for (int i = 0; i < 6; i++) {
        float a = i * 2.39996f, r = 5.0f + (float)((i * 31) % 12);
        Vector3 base = ctr + Vector3{ sinf(a) * r, 3.6f + (i % 3) * 0.8f, cosf(a) * r };
        Vector3 fp = base + Vector3{ 0.4f * sinf(s_time * 0.9f + i), 0.4f * sinf(s_time * 0.6f + i * 1.7f), 0.4f * cosf(s_time * 0.8f + i) };
        DrawSphereEx(fp, 0.22f, 6, 6, Color{ 170, 200, 255, 70 });
    }
    EndBlendMode();
    // the tomes' physical pages (lit), under their glow
    for (int i = 0; i < 6; i++) {
        float a = i * 2.39996f, r = 5.0f + (float)((i * 31) % 12);
        Vector3 base = ctr + Vector3{ sinf(a) * r, 3.6f + (i % 3) * 0.8f, cosf(a) * r };
        Vector3 fp = base + Vector3{ 0.4f * sinf(s_time * 0.9f + i), 0.4f * sinf(s_time * 0.6f + i * 1.7f), 0.4f * cosf(s_time * 0.8f + i) };
        DrawModelEx(s_column, fp, Vector3{ 0,1,0 }, s_time * 30.0f + i * 40.0f, Vector3{ 0.5f, 0.12f, 0.36f }, parch);
    }
}

// The Siege Camp: an abandoned besieging army's encampment, built procedurally (no
// boss_arena.glb), per design/siege_camp_design.md. Conical canvas tents (s_cone),
// banner poles with swaying flags, a sharpened-log palisade with a gate, a central
// command pavilion, campfires, and weapon racks, on a dry muddy field. Lit primitives.
static void draw_tent(const Tent& t, Color wood) {
    rlPushMatrix();
    rlTranslatef(t.c.x, 0, t.c.z);
    rlRotatef(t.yaw, 0, 1, 0);
    if (t.collapsed) rlRotatef(40.0f, 1, 0, 0);
    DrawModelEx(s_cone, Vector3{ 0,0,0 }, Vector3{ 0,1,0 }, 0, Vector3{ t.r, t.h, t.r }, t.canvas);             // canvas
    DrawModelEx(s_cyl,  Vector3{ 0,0,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.08f, t.h * 1.12f, 0.08f }, wood);     // center pole
    DrawModelEx(s_column, Vector3{ 0, t.h * 0.26f, t.r * 0.8f }, Vector3{ 0,1,0 }, 0, Vector3{ t.r * 0.32f, t.h * 0.45f, 0.12f }, Color{ 28,22,16,255 });   // entrance
    rlPopMatrix();
}

static void build_camp() {
    s_tents.clear(); s_banners.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(54321);
    Color canvas[4] = { { 165,130,95,255 }, { 150,80,70,255 }, { 90,100,130,255 }, { 130,120,80,255 } };

    int placed = 0, attempts = 0, want = 11;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float ang = rand_range(0, 2 * PI), rad = rand_range(6.0f, boundary_radius - 3.0f);
        Vector3 p = ctr + Vector3{ sinf(ang) * rad, 0, cosf(ang) * rad };
        if (p.z > ctr.z + 13.0f) continue;
        Tent t; t.c = p; t.yaw = rand_range(0, 360);
        t.r = rand_range(1.6f, 2.6f); t.h = rand_range(2.6f, 4.0f);
        t.collapsed = (GetRandomValue(0, 4) == 0);
        t.canvas = canvas[GetRandomValue(0, 3)];
        s_tents.push_back(t);
        placed++;
    }
    static const float bA[5] = { 25, 95, 170, 250, 320 };
    Color flags[3] = { { 160,50,45,255 }, { 50,70,130,255 }, { 180,150,70,255 } };
    for (int i = 0; i < 5; i++) { float a = bA[i] * DEG2RAD, r = rand_range(7.0f, 15.0f); s_banners.push_back(Banner{ ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }, rand_range(4.5f, 6.5f), flags[i % 3] }); }
    s_banners.push_back(Banner{ ctr + Vector3{ -3.5f, 0, 3.0f }, 6.5f, flags[0] });
    s_banners.push_back(Banner{ ctr + Vector3{  3.5f, 0, 3.0f }, 6.5f, flags[1] });

    static const float fA[4] = { 50, 130, 215, 300 };
    for (int i = 0; i < 4; i++) { float a = fA[i] * DEG2RAD, r = rand_range(6.0f, 13.0f); s_wisps.push_back(ctr + Vector3{ sinf(a) * r, 0.6f, cosf(a) * r }); }
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 0.8f, w.z, 6.0f }); }
}

static void draw_camp() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color mud{ 78, 66, 50, 255 }, mudD{ 62, 52, 40, 255 }, wood{ 90, 64, 42, 255 }, dark{ 60, 44, 30, 255 };

    // muddy field + low mounds
    DrawModelEx(s_column, Vector3{ ctr.x, -0.5f, ctr.z }, Vector3{ 0,1,0 }, 0, Vector3{ 120.0f, 1.0f, 120.0f }, mud);
    static const float duAng[5] = { 35, 110, 180, 250, 320 };
    for (int i = 0; i < 5; i++) { float a = duAng[i] * DEG2RAD, r = 14.0f + (i % 3) * 2.0f; DrawModelEx(s_dome, ctr + Vector3{ sinf(a) * r, -0.25f, cosf(a) * r }, Vector3{ 0,1,0 }, (float)(i * 40), Vector3{ 5.0f + i, 0.8f, 4.0f + i }, mudD); }

    // palisade ring of sharpened logs (gate gap toward +Z)
    int n = 46; float pr = boundary_radius - 1.0f;
    for (int i = 0; i < n; i++) {
        float deg = i * 360.0f / n, dd = deg > 180 ? 360 - deg : deg;
        if (dd < 14.0f) continue;
        float a = deg * DEG2RAD; Vector3 p = ctr + Vector3{ sinf(a) * pr, 0, cosf(a) * pr };
        float h = 2.6f + ((i * 3) % 4) * 0.2f;
        DrawModelEx(s_cyl, Vector3{ p.x, h * 0.5f, p.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, h, 0.22f }, wood);
        DrawModelEx(s_cone, Vector3{ p.x, h, p.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, 0.5f, 0.22f }, dark);
    }
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cyl, ctr + Vector3{ s * 3.0f, 1.8f, pr }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 3.6f, 0.4f }, dark);
    DrawModelEx(s_column, ctr + Vector3{ 0, 3.6f, pr }, Vector3{ 0,1,0 }, 0, Vector3{ 6.8f, 0.5f, 0.6f }, wood);   // gate lintel

    // central command pavilion on a platform
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 0.4f, 3.4f }, mudD);
    Tent cmd; cmd.c = ctr; cmd.yaw = 0; cmd.r = 3.0f; cmd.h = 5.5f; cmd.collapsed = false; cmd.canvas = Color{ 150,80,70,255 };
    draw_tent(cmd, dark);

    // tents + banners + weapon racks
    for (auto& t : s_tents) draw_tent(t, wood);
    for (auto& b : s_banners) {
        DrawModelEx(s_cyl, b.c + Vector3{ 0, b.h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.08f, b.h, 0.08f }, dark);
        rlPushMatrix();
        rlTranslatef(b.c.x, b.h * 0.86f, b.c.z);
        rlRotatef(12.0f * sinf(s_time * 2.0f + b.c.x), 0, 1, 0);
        DrawModelEx(s_column, Vector3{ 0.75f, -0.45f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 0.9f, 0.04f }, b.flag);
        rlPopMatrix();
    }
    static const float wrA[3] = { 60, 200, 310 };
    for (int i = 0; i < 3; i++) {
        float a = wrA[i] * DEG2RAD, r = 9.0f; Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cyl, p + Vector3{ s * 0.5f, 0.7f, 0 }, Vector3{ 0,0,1 }, 22.0f * s, Vector3{ 0.07f, 1.5f, 0.07f }, wood);   // A-frame
        DrawModelEx(s_cyl, p + Vector3{ 0, 1.3f, 0 }, Vector3{ 1,0,0 }, 90, Vector3{ 0.06f, 1.2f, 0.06f }, wood);   // crossbar
        for (int sp = -2; sp <= 2; sp++) DrawModelEx(s_cyl, p + Vector3{ sp * 0.22f, 1.4f, 0.1f }, Vector3{ 1,0,0 }, 14.0f, Vector3{ 0.04f, 2.4f, 0.04f }, dark);   // leaning spears
    }

    // campfires: stone ring + crossed logs + flames (additive)
    for (auto& w : s_wisps) {
        Vector3 f{ w.x, 0, w.z };
        for (int s = 0; s < 5; s++) { float a = s * 72 * DEG2RAD; DrawModelEx(s_dome, f + Vector3{ sinf(a) * 0.6f, 0, cosf(a) * 0.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.25f, 0.3f }, mudD); }
        DrawModelEx(s_cyl, f + Vector3{ 0, 0.25f, 0 }, Vector3{ 1,0,0 }, 80, Vector3{ 0.12f, 0.9f, 0.12f }, dark);
        DrawModelEx(s_cyl, f + Vector3{ 0, 0.25f, 0 }, Vector3{ 0,0,1 }, 80, Vector3{ 0.12f, 0.9f, 0.12f }, dark);
    }
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 f{ s_wisps[i].x, 0.3f, s_wisps[i].z };
        for (int k = 0; k < 5; k++) {
            float fa = k * 1.3f + i, fl = 0.5f + 0.5f * sinf(s_time * 7.0f + fa * 2.1f);
            DrawSphereEx(f + Vector3{ 0.12f * sinf(fa + s_time), 0.3f + (0.3f + 0.5f * fl) * 0.5f, 0.12f * cosf(fa + s_time) }, 0.16f + 0.1f * fl, 6, 6, Color{ 255, (unsigned char)(150 + 80 * fl), 40, 140 });
        }
        DrawSphereEx(f + Vector3{ 0, 0.6f, 0 }, 0.8f, 8, 8, Color{ 255, 130, 40, 45 });
    }
    EndBlendMode();
}

// The Sunken Aqueduct: a vast flooded cistern, built procedurally (no boss_arena.glb),
// per design/sunken_aqueduct_design.md. Two arcades of vaulted stone arches march down
// a dark central water channel, crossed by a great central arch, with dripping pipes,
// broken brick walls, and green algae glow at the waterline. All from lit cube/cylinder.
static void draw_arch(const Arch& A, Color stone) {
    rlPushMatrix();
    rlTranslatef(A.c.x, 0, A.c.z);
    rlRotatef(A.yaw, 0, 1, 0);
    float r = A.span * 0.5f;
    for (int s = -1; s <= 1; s += 2)
        DrawModelEx(s_column, Vector3{ r * s, A.pierH * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ A.thick, A.pierH, A.thick }, stone);   // piers
    int segs = 9;
    for (int k = 0; k <= segs; k++) {                          // voussoir ring (half circle)
        float a = PI * k / (float)segs;
        rlPushMatrix();
        rlTranslatef(-r * cosf(a), A.pierH + r * sinf(a), 0);
        rlRotatef((a - PI * 0.5f) * RAD2DEG, 0, 0, 1);
        DrawModelEx(s_column, Vector3{ 0,0,0 }, Vector3{ 0,1,0 }, 0, Vector3{ A.thick * 1.1f, A.thick * 0.55f, A.thick }, stone);
        rlPopMatrix();
    }
    DrawModelEx(s_column, Vector3{ 0, A.pierH + r + A.thick * 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ A.span + A.thick * 1.5f, A.thick * 0.7f, A.thick * 1.3f }, stone);   // entablature
    rlPopMatrix();
}

static void build_aqua() {
    s_arches.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    float zoff[6] = { 12, 7, 2, -3, -8, -13 };
    for (int i = 0; i < 6; i++)
        for (int sx = -1; sx <= 1; sx += 2)
            s_arches.push_back(Arch{ ctr + Vector3{ sx * 6.0f, 0, zoff[i] }, 90.0f, 6.0f, 4.0f, 0.9f });   // two flanking arcades
    s_arches.push_back(Arch{ ctr, 0.0f, 9.0f, 5.5f, 1.2f });   // grand central arch across the channel

    for (auto& a : s_arches) {
        if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break;
        s_wisps.push_back(a.c + Vector3{ 0, 0.5f, 0 });
        s_crystal_lights.push_back(Vector4{ a.c.x, 0.6f, a.c.z, 5.0f });
    }
}

static void draw_aqua() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color stone{ 78, 84, 80, 255 }, stoneD{ 58, 64, 60, 255 }, brick{ 86, 78, 70, 255 };

    for (size_t i = 0; i < s_arches.size(); i++) draw_arch(s_arches[i], (i % 2) ? stone : stoneD);

    // dripping horizontal pipes jutting from a few piers
    for (size_t i = 0; i < s_arches.size(); i += 3)
        DrawModelEx(s_cyl, s_arches[i].c + Vector3{ 0, 2.5f, 0 }, Vector3{ 0,0,1 }, 90, Vector3{ 0.18f, 1.6f, 0.18f }, stoneD);

    // broken brick perimeter walls
    int n = 40; float wr = boundary_radius - 1.0f, segW = 2.0f * wr * sinf(PI / n) * 1.2f;
    for (int i = 0; i < n; i++) {
        if (i % 6 == 0) continue;
        float deg = i * 360.0f / n, a = deg * DEG2RAD;
        Vector3 p = ctr + Vector3{ sinf(a) * wr, 0, cosf(a) * wr };
        float h = 5.0f + ((i * 3) % 4) * 0.5f;
        DrawModelEx(s_column, Vector3{ p.x, h * 0.5f, p.z }, Vector3{ 0,1,0 }, deg, Vector3{ segW, h, 0.7f }, (i % 2) ? brick : stoneD);
    }

    // green algae glow at the waterline (additive) + slow drips
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        float fl = 0.5f + 0.5f * sinf(s_time * 2.0f + i * 1.5f);
        DrawSphereEx(Vector3{ w.x, 0.3f, w.z }, 0.28f + 0.1f * fl, 8, 8, Color{ 60, 200, 110, 45 });
    }
    EndBlendMode();
}

// The Sentinel Court: a solemn moonlit court of colossal guardian statues, built
// procedurally (no boss_arena.glb), per design/sentinel_court_design.md. Towering
// helmeted sentinels gripping greatswords point-down line an avenue (some headless),
// around a colossal central sentinel, with braziers, fallen-statue debris, and a
// broken colonnade, over the reused dark reflecting floor. All from lit primitives.
static void draw_sentinel(Vector3 c, float yaw, float s, bool headless, Color stone, Color dark) {
    rlPushMatrix();
    rlTranslatef(c.x, 0, c.z);
    rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 0.4f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f * s, 0.8f * s, 2.4f * s }, dark);    // pedestal
    DrawModelEx(s_column, Vector3{ 0, 0.9f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f * s, 0.3f * s, 2.0f * s }, stone);   // plinth
    float b = 1.05f * s;
    for (int sx = -1; sx <= 1; sx += 2)
        DrawModelEx(s_column, Vector3{ sx * 0.45f * s, b + 1.4f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f * s, 2.8f * s, 0.7f * s }, stone);   // legs
    DrawModelEx(s_column, Vector3{ 0, b + 3.4f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.8f * s, 2.2f * s, 1.1f * s }, stone);   // torso
    DrawModelEx(s_column, Vector3{ 0, b + 4.7f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.3f * s, 0.6f * s, 1.3f * s }, dark);    // pauldrons
    for (int sx = -1; sx <= 1; sx += 2)
        DrawModelEx(s_column, Vector3{ sx * 1.2f * s, b + 3.3f * s, 0.1f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f * s, 2.6f * s, 0.5f * s }, stone);   // arms
    DrawModelEx(s_column, Vector3{ 0, b + 2.2f * s, 0.95f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f * s, 5.0f * s, 0.14f * s }, dark);    // greatsword blade
    DrawModelEx(s_column, Vector3{ 0, b + 4.4f * s, 0.95f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f * s, 0.26f * s, 0.26f * s }, dark);   // crossguard
    if (!headless) {
        DrawModelEx(s_dome,   Vector3{ 0, b + 5.0f * s, 0 }, Vector3{ 0,1,0 }, 0,  Vector3{ 0.75f * s, 0.85f * s, 0.75f * s }, stone);   // helmet head
        DrawModelEx(s_column, Vector3{ 0, b + 5.7f * s, 0 }, Vector3{ 0,1,0 }, 45, Vector3{ 0.28f * s, 0.6f * s, 0.28f * s }, dark);     // crest
    } else {
        DrawModelEx(s_column, Vector3{ 0, b + 4.95f * s, 0 }, Vector3{ 0,1,0 }, 20, Vector3{ 0.6f * s, 0.25f * s, 0.6f * s }, dark);     // broken neck stump
    }
    rlPopMatrix();
}

static void build_court() {
    s_statues.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(88888);
    float zoff[4] = { 11, 5, -3, -10 };
    for (int i = 0; i < 4; i++)
        for (int sx = -1; sx <= 1; sx += 2) {
            Statue st; st.c = ctr + Vector3{ sx * 6.5f, 0, zoff[i] };
            st.yaw = (sx < 0) ? 90.0f : -90.0f;   // face inward across the avenue
            st.scale = 1.0f;
            st.headless = (GetRandomValue(0, 3) == 0);
            s_statues.push_back(st);
        }
    s_statues.push_back(Statue{ ctr, 0.0f, 1.5f, false });   // colossal central sentinel facing the entrance

    static const float fA[4] = { 45, 135, 225, 315 };
    for (int i = 0; i < 4; i++) { float a = fA[i] * DEG2RAD, r = 10.0f; s_wisps.push_back(ctr + Vector3{ sinf(a) * r, 1.6f, cosf(a) * r }); }
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 6.0f }); }
}

static void draw_court() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color stone{ 96, 100, 110, 255 }, dark{ 66, 70, 80, 255 }, brazierC{ 50, 52, 60, 255 };

    for (auto& st : s_statues) draw_sentinel(st.c, st.yaw, st.scale, st.headless, stone, dark);

    // fallen-statue debris: toppled heads + broken column drums + rubble
    SetRandomSeed(424242);
    for (int i = 0; i < 7; i++) {
        float a = rand_range(0, 2 * PI), r = rand_range(7.0f, boundary_radius - 2.0f);
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        if (p.z > ctr.z + 13.0f) continue;
        int kind = GetRandomValue(0, 2);
        if (kind == 0)      DrawModelEx(s_dome,   p + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, rand_range(0, 360), Vector3{ 0.8f, 0.7f, 0.8f }, stone);   // fallen head
        else if (kind == 1) DrawModelEx(s_cyl,    Vector3{ p.x, 0.45f, p.z }, Vector3{ 1,0,0 }, 90, Vector3{ 0.5f, rand_range(2.0f, 3.5f), 0.5f }, dark);  // toppled drum
        else                DrawModelEx(s_column, p + Vector3{ 0, 0.35f, 0 }, Vector3{ 0,1,0 }, rand_range(0, 360), Vector3{ 1.2f, 0.7f, 1.0f }, dark);    // rubble
    }

    // braziers (pedestal + flames)
    for (auto& w : s_wisps) DrawModelEx(s_cyl, Vector3{ w.x, 0.8f, w.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.6f, 0.5f }, brazierC);
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        for (int f = 0; f < 5; f++) {
            float fa = f * 1.3f + i, fl = 0.5f + 0.5f * sinf(s_time * 7.0f + fa * 2.1f);
            DrawSphereEx(Vector3{ w.x + 0.12f * sinf(fa + s_time), 1.7f + (0.3f + 0.5f * fl) * 0.5f, w.z + 0.12f * cosf(fa + s_time) }, 0.15f + 0.1f * fl, 6, 6, Color{ 255, (unsigned char)(150 + 80 * fl), 40, 140 });
        }
        DrawSphereEx(Vector3{ w.x, 2.0f, w.z }, 0.7f, 8, 8, Color{ 255, 130, 40, 42 });
    }
    EndBlendMode();

    // broken perimeter colonnade
    int n = 28; float wr = boundary_radius - 1.5f;
    for (int i = 0; i < n; i++) {
        if (i % 4 == 0) continue;
        float deg = i * 360.0f / n, a = deg * DEG2RAD;
        Vector3 p = ctr + Vector3{ sinf(a) * wr, 0, cosf(a) * wr };
        float h = 4.0f + ((i * 3) % 4) * 0.7f;
        DrawModelEx(s_cyl, Vector3{ p.x, h * 0.5f, p.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, h, 0.7f }, (i % 2) ? stone : dark);
    }
}

// The Hanging Gardens: a lush formal water-garden, built procedurally (no
// boss_arena.glb), per design/hanging_gardens_design.md. A grand tiered central
// fountain, four quadrant flower-beds of coloured blooms, a trellis rose-walk, a
// cypress topiary ring, a hedge border, and drifting petals, over the reused cool
// reflecting pool. Brings vivid colour; all from lit cube/cylinder/cone/dome.
static void build_garden() {
    s_bushes.clear(); s_cypress.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(70707);
    Color flowers[6] = { { 214,84,96,255 }, { 232,150,172,255 }, { 232,202,96,255 }, { 150,92,202,255 }, { 238,238,238,255 }, { 224,124,64,255 } };

    Vector3 bed[4] = { ctr + Vector3{ -7,0,-7 }, ctr + Vector3{ 7,0,-7 }, ctr + Vector3{ -7,0,7 }, ctr + Vector3{ 7,0,7 } };
    for (int q = 0; q < 4; q++) {
        if (bed[q].z > ctr.z + 13.0f) continue;
        for (int i = 0; i < 7; i++)
            s_bushes.push_back(Bush{ bed[q] + Vector3{ rand_range(-2.2f, 2.2f), 0, rand_range(-2.2f, 2.2f) }, rand_range(0.55f, 1.0f), flowers[GetRandomValue(0, 5)] });
    }
    int placed = 0, attempts = 0, want = 8;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float a = rand_range(0, 2 * PI), r = rand_range(11.0f, boundary_radius - 2.0f);
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        if (p.z > ctr.z + 13.0f) continue;
        s_bushes.push_back(Bush{ p, rand_range(0.8f, 1.5f), flowers[GetRandomValue(0, 5)] });
        placed++;
    }
    static const float cA[8] = { 20, 65, 110, 155, 205, 250, 295, 340 };
    for (int i = 0; i < 8; i++) { float a = cA[i] * DEG2RAD, r = boundary_radius - 3.0f; s_cypress.push_back(Cypress{ ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }, rand_range(5.0f, 7.5f), rand_range(0.9f, 1.4f) }); }

    s_wisps.push_back(ctr + Vector3{ 0, 2.6f, 0 });
    s_wisps.push_back(ctr + Vector3{ -11, 1.8f, 3 });
    s_wisps.push_back(ctr + Vector3{ 11, 1.8f, -3 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 5.0f }); }
}

static void draw_garden() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color stone{ 152,142,122,255 }, stoneD{ 122,114,96,255 }, hedge{ 56,108,58,255 }, cypressC{ 42,86,54,255 }, trunk{ 96,70,48,255 };

    // central tiered fountain on a low dais
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 0.4f, 3.0f }, stone);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 0.6f, 2.2f }, stoneD);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 1.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 1.4f, 0.4f }, stone);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 2.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f, 0.4f, 1.1f }, stoneD);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 2.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.25f, 0.8f, 0.25f }, stone);

    // four formal bed borders (thin low frames)
    Vector3 bed[4] = { ctr + Vector3{ -7,0,-7 }, ctr + Vector3{ 7,0,-7 }, ctr + Vector3{ -7,0,7 }, ctr + Vector3{ 7,0,7 } };
    for (int q = 0; q < 4; q++) {
        if (bed[q].z > ctr.z + 13.0f) continue;
        for (int s = -1; s <= 1; s += 2) {
            DrawModelEx(s_column, bed[q] + Vector3{ 0, 0.2f, s * 2.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 5.4f, 0.4f, 0.3f }, stone);
            DrawModelEx(s_column, bed[q] + Vector3{ s * 2.6f, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.4f, 5.4f }, stone);
        }
    }
    // flower bushes
    for (auto& b : s_bushes) DrawModelEx(s_dome, b.pos, Vector3{ 0,1,0 }, (float)(((int)(b.pos.x * 9)) % 360), Vector3{ b.r, b.r * 0.8f, b.r }, b.col);
    // cypress topiary (trunk + tall cone)
    for (auto& c : s_cypress) {
        DrawModelEx(s_cyl, c.pos + Vector3{ 0, 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.6f, 0.3f }, trunk);
        DrawModelEx(s_cone, c.pos + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ c.r, c.h, c.r }, cypressC);
    }
    // trellis rose-walk along +Z
    for (int i = 0; i < 4; i++) {
        float z = ctr.z + 11.0f - i * 4.5f;
        for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cyl, Vector3{ ctr.x + s * 3.5f, 1.4f, z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 2.8f, 0.12f }, trunk);
        for (int k = -2; k <= 2; k++) {
            float fx = k / 2.0f, h = 2.8f + 0.6f * (1.0f - fx * fx);
            DrawModelEx(s_column, Vector3{ ctr.x + k * 1.6f, h, z }, Vector3{ 0,1,0 }, 0, Vector3{ 1.7f, 0.14f, 0.2f }, trunk);
            DrawModelEx(s_dome, Vector3{ ctr.x + k * 1.6f, h + 0.2f, z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.42f, 0.32f, 0.42f }, Color{ 232,140,172,255 });
        }
    }
    // low hedge border ring
    int n = 40; float hr = boundary_radius - 1.2f, segW = 2.0f * hr * sinf(PI / n) * 1.25f;
    for (int i = 0; i < n; i++) {
        if (i % 7 == 0) continue;
        float deg = i * 360.0f / n, a = deg * DEG2RAD;
        Vector3 p = ctr + Vector3{ sinf(a) * hr, 0, cosf(a) * hr };
        DrawModelEx(s_column, Vector3{ p.x, 0.8f, p.z }, Vector3{ 0,1,0 }, deg, Vector3{ segW, 1.6f, 0.9f }, hedge);
    }

    // fountain jets + drifting flower petals (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        for (int j = 0; j < 6; j++) { float t = fmodf(s_time * 0.7f + j * 0.16f + i, 1.0f); DrawSphereEx(w + Vector3{ 0.1f * sinf(j + i), t * 1.3f, 0.1f * cosf(j + i) }, 0.08f * (1.0f - t) + 0.03f, 5, 5, Color{ 170,205,240,(unsigned char)(90 * (1.0f - t)) }); }
    }
    for (int i = 0; i < 16; i++) {
        float a = i * 2.39996f + s_time * 0.15f, r = 5.0f + (float)((i * 53) % 16);
        Vector3 p = ctr + Vector3{ sinf(a) * r, 2.0f + 1.5f * sinf(s_time * 0.5f + i) + (i % 4), cosf(a) * r };
        DrawSphereEx(p, 0.11f, 5, 5, Color{ 235,165,195,85 });
    }
    EndBlendMode();
}

// The Chasm Bridge: a colossal ancient bridge over a bottomless chasm, built
// procedurally (no boss_arena.glb), per design/chasm_bridge_design.md. A long railed
// stone deck (the floor; railing-wall obstacles confine it to a strip) flanked by four
// tall gate-towers, with great iron chains drooping in catenary arcs between the towers
// + hanging lanterns, a midspan ruined gate-arch, broken deck gaps, and a cloud band
// far below the void. Dry; all from lit cube/cylinder/cone + draw_bone_seg chains.
static void build_bridge() {
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    float hw = 11.0f;
    for (float z = ctr.z - 21.0f; z <= ctr.z + 21.0f; z += 3.0f)   // railing-wall obstacles
        for (int sx = -1; sx <= 1; sx += 2)
            obstacles.push_back({ Vector3{ sx * hw, 0, z }, 1.7f });
    for (int sx = -1; sx <= 1; sx += 2)                            // hanging lanterns (warm lights)
        for (int i = 0; i < 3; i++)
            s_wisps.push_back(Vector3{ sx * hw, 4.5f, ctr.z - 12.0f + i * 12.0f });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 4.0f, w.z, 7.0f }); }
}

static void draw_bridge() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color stone{ 92,92,100,255 }, stoneD{ 66,66,74,255 }, iron{ 58,56,62,255 }, dark{ 18,18,22,255 };
    float hw = 11.0f;
    auto chainY = [&](float z) { float t = (z - ctr.z) / 20.0f; return 12.0f - 6.0f * (1.0f - t * t); };

    // deck slab + plank seams + broken gaps
    DrawModelEx(s_column, Vector3{ ctr.x, -0.5f, ctr.z }, Vector3{ 0,1,0 }, 0, Vector3{ hw * 2.0f, 1.0f, 50.0f }, stone);
    for (int i = 0; i < 16; i++) { float z = ctr.z - 24.0f + i * 3.2f; DrawModelEx(s_column, Vector3{ ctr.x, 0.02f, z }, Vector3{ 0,1,0 }, 0, Vector3{ hw * 2.0f, 0.06f, 0.3f }, stoneD); }
    DrawModelEx(s_column, Vector3{ -4.0f, -0.1f, ctr.z + 14.0f }, Vector3{ 0,1,0 }, 12, Vector3{ 4.0f, 0.5f, 3.0f }, dark);
    DrawModelEx(s_column, Vector3{  5.0f, -0.1f, ctr.z - 15.0f }, Vector3{ 0,1,0 }, -8, Vector3{ 3.5f, 0.5f, 3.5f }, dark);

    // balustrade railings (both long edges)
    for (int sx = -1; sx <= 1; sx += 2) {
        DrawModelEx(s_column, Vector3{ sx * hw, 1.1f, ctr.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.3f, 46.0f }, stone);
        DrawModelEx(s_column, Vector3{ sx * hw, 0.35f, ctr.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.7f, 46.0f }, stoneD);
        for (int i = 0; i < 22; i++) { if ((i % 7) == 3) continue; float z = ctr.z - 22.0f + i * 2.0f; DrawModelEx(s_cyl, Vector3{ sx * hw, 0.7f, z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 1.0f, 0.12f }, stone); }
    }

    // gate-towers (four corners) + arrow slits + cone roofs
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
            float tx = sx * hw, tz = ctr.z + sz * 20.0f;
            DrawModelEx(s_column, Vector3{ tx, 6.0f, tz }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 12.0f, 3.0f }, (sx < 0) ? stone : stoneD);
            DrawModelEx(s_column, Vector3{ tx, 12.3f, tz }, Vector3{ 0,1,0 }, 0, Vector3{ 3.6f, 0.6f, 3.6f }, iron);
            DrawModelEx(s_cone, Vector3{ tx, 12.6f, tz }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 3.2f, 2.4f }, stoneD);
            DrawModelEx(s_column, Vector3{ tx, 7.0f, tz + (sz < 0 ? 1.6f : -1.6f) }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 2.2f, 0.2f }, dark);
        }

    // midspan ruined gate-arch (piers + crown)
    for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_column, Vector3{ sx * 5.0f, 3.0f, ctr.z }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 6.0f, 1.6f }, stone);
    for (int k = -2; k <= 2; k++) { float fx = k / 2.0f, h = 6.0f + 1.4f * (1.0f - fx * fx); DrawModelEx(s_column, Vector3{ ctr.x + k * 2.0f, h, ctr.z }, Vector3{ 0,1,0 }, 0, Vector3{ 2.1f, 0.5f, 1.6f }, (k == 0) ? iron : stoneD); }

    // great chains (catenary) along each side
    for (int sx = -1; sx <= 1; sx += 2) {
        Vector3 prev{ sx * hw, 12.0f, ctr.z - 20.0f };
        for (int k = 1; k <= 12; k++) { float z = ctr.z - 20.0f + 40.0f * k / 12.0f; Vector3 cur{ sx * hw, chainY(z), z }; draw_bone_seg(prev, cur, 0.22f, iron); prev = cur; }
    }

    // hanging lanterns (rope up to the chain + box)
    for (auto& w : s_wisps) {
        float rl = chainY(w.z) - w.y; if (rl < 0.2f) rl = 0.2f;
        DrawModelEx(s_cyl, Vector3{ w.x, w.y + 0.4f, w.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.05f, rl, 0.05f }, iron);
        DrawModelEx(s_column, Vector3{ w.x, w.y, w.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.6f, 0.4f }, iron);
    }

    // cloud band far below the void + lantern glow (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 8; i++) { float a = i * (2 * PI / 8) + s_time * 0.02f; Vector3 cp = ctr + Vector3{ sinf(a) * (20.0f + i * 4), -26.0f, cosf(a) * (20.0f + i * 4) }; DrawSphereEx(cp, 14.0f, 8, 8, Color{ 180,185,200,12 }); }
    for (size_t i = 0; i < s_wisps.size(); i++) { Vector3 w = s_wisps[i]; float fl = 0.6f + 0.4f * sinf(s_time * 4.0f + i * 1.7f); DrawSphereEx(w, 0.18f + 0.06f * fl, 6, 6, Color{ 255,185,90,150 }); DrawSphereEx(w, 0.5f + 0.1f * fl, 8, 8, Color{ 255,150,60,32 }); }
    EndBlendMode();
}

// The Beacon Coast: a storm-lashed rocky cape, built procedurally (no boss_arena.glb),
// per design/beacon_coast_design.md. A tall banded lighthouse whose lamp sweeps a
// rotating beam, jagged sea-stacks rising from the dark surf, a boulder breakwater ring,
// wrecked rowboats, and surf foam, over the reused stormy sea. Lit primitives + an
// additive sweeping beam.
static void build_beacon() {
    s_stacks.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(60504);
    int placed = 0, attempts = 0, want = 7;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float a = rand_range(0, 2 * PI), r = rand_range(8.0f, boundary_radius - 3.0f);
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        if (p.z > ctr.z + 13.0f) continue;
        s_stacks.push_back(Stack{ p, rand_range(4.0f, 9.0f), rand_range(1.2f, 2.4f) });
        placed++;
    }
    s_wisps.push_back(ctr + Vector3{ 0, 13.3f, 0 });                 // lamp (additive glow)
    s_crystal_lights.push_back(Vector4{ ctr.x, 4.0f, ctr.z, 12.0f }); // warm lamp pool on the water/rocks
}

static void draw_beacon() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color white{ 210,210,215,255 }, red{ 170,60,55,255 }, dark{ 70,72,80,255 }, glass{ 120,150,170,255 },
          rock{ 60,62,70,255 }, rockD{ 46,48,56,255 };

    // lighthouse: tapered banded tower + gallery + glass lamp room + cone roof
    float baseR = 2.6f, topR = 1.6f, segH = 2.4f;
    for (int i = 0; i < 5; i++) {
        float r0 = baseR + (topR - baseR) * i / 5.0f;
        DrawModelEx(s_cyl, ctr + Vector3{ 0, i * segH, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r0, segH, r0 }, (i % 2) ? red : white);
    }
    float topY = 5 * segH;
    DrawModelEx(s_cyl, ctr + Vector3{ 0, topY, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ topR + 0.6f, 0.4f, topR + 0.6f }, dark);          // gallery
    DrawModelEx(s_cyl, ctr + Vector3{ 0, topY + 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ topR * 0.7f, 1.8f, topR * 0.7f }, glass);  // lamp room
    DrawModelEx(s_cone, ctr + Vector3{ 0, topY + 2.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ topR + 0.5f, 1.6f, topR + 0.5f }, dark);  // roof

    // sea-stacks (jagged dark rock spires) + boulder breakwater
    for (auto& s : s_stacks) {
        DrawModelEx(s_cone, s.base, Vector3{ 0,1,0 }, (float)(((int)(s.base.x * 11)) % 360), Vector3{ s.r, s.h, s.r }, rock);
        DrawModelEx(s_cone, s.base + Vector3{ s.r * 0.3f, 0, s.r * 0.2f }, Vector3{ 0,1,0 }, 40, Vector3{ s.r * 0.6f, s.h * 0.7f, s.r * 0.6f }, rockD);
    }
    for (int i = 0; i < 26; i++) {
        float a = i * (360.0f / 26) * DEG2RAD, r = boundary_radius - 2.0f + (i % 3);
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        DrawModelEx(s_dome, p + Vector3{ 0, -0.2f, 0 }, Vector3{ 0,1,0 }, (float)(i * 30), Vector3{ 1.6f + (i % 3) * 0.5f, 1.2f, 1.6f }, (i % 2) ? rock : rockD);
    }
    // two wrecked rowboats (small tilted hulls)
    for (int b = 0; b < 2; b++) {
        Vector3 p = ctr + Vector3{ b ? 10.0f : -9.0f, 0, b ? 6.0f : -7.0f };
        rlPushMatrix();
        rlTranslatef(p.x, 0.2f, p.z);
        rlRotatef(b ? 40.0f : -60.0f, 0, 1, 0);
        rlRotatef(28.0f, 1, 0, 0);
        DrawModelEx(s_column, Vector3{ 0, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 0.5f, 3.4f }, rockD);
        DrawModelEx(s_column, Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 1.2f, 0.16f }, dark);   // broken mast stub
        rlPopMatrix();
    }

    // the lamp glow + ROTATING beam + surf foam (additive)
    Vector3 lamp = ctr + Vector3{ 0, topY + 1.3f, 0 };
    BeginBlendMode(BLEND_ADDITIVE);
    DrawSphereEx(lamp, 0.9f + 0.1f * sinf(s_time * 8.0f), 10, 10, Color{ 255, 235, 170, 190 });
    rlPushMatrix();
    rlTranslatef(lamp.x, lamp.y, lamp.z);
    rlRotatef(s_time * 45.0f, 0, 1, 0);
    for (int s = 0; s < 2; s++) {                                   // two opposite sweeping beams
        DrawCube(Vector3{ 14.0f, 0, 0 }, 28.0f, 0.6f, 2.2f, Color{ 255, 240, 190, 24 });
        DrawCube(Vector3{ 14.0f, 0, 0 }, 28.0f, 0.3f, 0.9f, Color{ 255, 245, 210, 40 });
        rlRotatef(180.0f, 0, 1, 0);
    }
    rlPopMatrix();
    // surf foam at the breakwater waterline
    for (int i = 0; i < 18; i++) {
        float a = i * (360.0f / 18) * DEG2RAD, r = boundary_radius - 2.5f;
        float fl = 0.5f + 0.5f * sinf(s_time * 3.0f + i * 1.3f);
        DrawSphereEx(ctr + Vector3{ sinf(a) * r, 0.25f, cosf(a) * r }, 0.6f + 0.3f * fl, 6, 6, Color{ 200, 215, 225, 30 });
    }
    EndBlendMode();
}

// The Windmill Fields: a windswept farmland, built procedurally (no boss_arena.glb),
// per design/windmill_fields_design.md. Windmills with great turning 4-sail crosses, a
// gable barn, split-rail fences, hay bales/stacks, scarecrows, and wheat, on a dry
// grass field. Lit primitives; sails spun by s_time.
static void draw_windmill(Vector3 c, float yaw, float s, float spin, Color wood, Color roof, Color sail) {
    rlPushMatrix();
    rlTranslatef(c.x, 0, c.z);
    rlRotatef(yaw, 0, 1, 0);
    for (int i = 0; i < 4; i++) { float r = (2.0f - i * 0.22f) * s; DrawModelEx(s_cyl, Vector3{ 0, i * 2.0f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r, 2.0f * s, r }, (i % 2) ? wood : roof); }
    float topY = 8.0f * s;
    DrawModelEx(s_cone, Vector3{ 0, topY, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f * s, 1.8f * s, 1.5f * s }, roof);   // cap
    Vector3 hub{ 0, topY - 0.8f * s, 1.45f * s };
    DrawModelEx(s_cyl, hub, Vector3{ 1,0,0 }, 90, Vector3{ 0.2f * s, 0.7f * s, 0.2f * s }, roof);   // axle (+z)
    rlPushMatrix();
    rlTranslatef(hub.x, hub.y, hub.z);
    rlRotatef(spin, 0, 0, 1);
    for (int a = 0; a < 4; a++) {
        rlPushMatrix();
        rlRotatef(a * 90.0f, 0, 0, 1);
        DrawModelEx(s_column, Vector3{ 0, 2.6f * s, 0.1f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f * s, 5.2f * s, 0.16f * s }, roof);    // arm
        DrawModelEx(s_column, Vector3{ 0.6f * s, 2.6f * s, 0.12f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f * s, 4.4f * s, 0.05f * s }, sail);   // sail panel
        for (int k = 0; k < 3; k++) DrawModelEx(s_column, Vector3{ 0.6f * s, (1.2f + k * 1.5f) * s, 0.14f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f * s, 0.08f * s, 0.06f * s }, roof);   // slats
        rlPopMatrix();
    }
    rlPopMatrix();
    rlPopMatrix();
}

static void build_mill() {
    s_mills.clear(); s_hay.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(33990);
    s_mills.push_back(Mill{ ctr, 0.0f, 1.4f, 26.0f });
    static const float mA[3] = { 70, 180, 290 };
    for (int i = 0; i < 3; i++) { float a = mA[i] * DEG2RAD, r = rand_range(10.0f, boundary_radius - 4.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 13.0f) continue; s_mills.push_back(Mill{ p, rand_range(0, 360), rand_range(0.8f, 1.1f), rand_range(18.0f, 40.0f) * (GetRandomValue(0, 1) ? 1.0f : -1.0f) }); }
    int placed = 0, attempts = 0, want = 10;
    while (placed < want && attempts < want * 30) { attempts++; float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 13.0f) continue; s_hay.push_back(Hay{ p, rand_range(0.8f, 1.4f), (GetRandomValue(0, 2) == 0) }); placed++; }
    Vector3 barn = ctr + Vector3{ -13, 0, -8 };
    s_wisps.push_back(barn + Vector3{ 1.5f, 2.0f, 3.1f });
    s_wisps.push_back(barn + Vector3{ -1.5f, 2.0f, 3.1f });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 2.0f, w.z, 5.0f }); }
}

static void draw_mill() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color grass{ 96,116,66,255 }, grassD{ 80,100,56,255 }, wood{ 110,80,52,255 }, roofC{ 86,60,40,255 }, sail{ 200,196,180,255 }, hayC{ 200,172,80,255 }, barnC{ 140,64,52,255 };

    // grass field + rolling mounds
    DrawModelEx(s_column, Vector3{ ctr.x, -0.5f, ctr.z }, Vector3{ 0,1,0 }, 0, Vector3{ 130.0f, 1.0f, 130.0f }, grass);
    static const float duAng[5] = { 25, 100, 175, 250, 320 };
    for (int i = 0; i < 5; i++) { float a = duAng[i] * DEG2RAD, r = 15.0f + (i % 3) * 2.0f; DrawModelEx(s_dome, ctr + Vector3{ sinf(a) * r, -0.4f, cosf(a) * r }, Vector3{ 0,1,0 }, (float)(i * 40), Vector3{ 7.0f + i, 1.2f, 6.0f + i }, grassD); }

    // windmills (sails spun by time)
    for (auto& m : s_mills) draw_windmill(m.c, m.yaw, m.scale, s_time * m.speed, wood, roofC, sail);

    // barn: body + gable roof + door
    Vector3 barn = ctr + Vector3{ -13, 0, -8 };
    DrawModelEx(s_column, barn + Vector3{ 0, 2.0f, 0 }, Vector3{ 0,1,0 }, 20, Vector3{ 8.0f, 4.0f, 6.0f }, barnC);
    rlPushMatrix(); rlTranslatef(barn.x, 4.0f, barn.z); rlRotatef(20, 0, 1, 0);
    for (int s = -1; s <= 1; s += 2) { rlPushMatrix(); rlTranslatef(s * 1.7f, 0.7f, 0); rlRotatef(-40.0f * s, 0, 0, 1); DrawModelEx(s_column, Vector3{ 0,0,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.4f, 0.3f, 6.4f }, roofC); rlPopMatrix(); }
    rlPopMatrix();
    DrawModelEx(s_column, barn + Vector3{ 1.0f, 1.4f, 3.05f }, Vector3{ 0,1,0 }, 20, Vector3{ 2.4f, 2.8f, 0.2f }, wood);   // door

    // split-rail fences
    auto fence = [&](Vector3 a, Vector3 b) {
        Vector3 d = Vector3Subtract(b, a); float len = Vector3Length(d); if (len < 0.5f) return;
        Vector3 dir = Vector3Scale(d, 1.0f / len); float yaw = atan2f(dir.x, dir.z) * RAD2DEG; Vector3 mid = Vector3Lerp(a, b, 0.5f);
        for (float t = 0; t <= 1.0f; t += 1.2f / len) DrawModelEx(s_cyl, Vector3Add(a, Vector3Scale(dir, t * len)) + Vector3{ 0,0.5f,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 1.0f, 0.1f }, wood);
        for (int r = 0; r < 2; r++) DrawModelEx(s_column, mid + Vector3{ 0, 0.35f + r * 0.4f, 0 }, Vector3{ 0,1,0 }, yaw, Vector3{ 0.08f, 0.1f, len }, wood);
    };
    fence(ctr + Vector3{ -18,0,8 }, ctr + Vector3{ -2,0,8 });
    fence(ctr + Vector3{ 4,0,-6 }, ctr + Vector3{ 16,0,-14 });
    fence(ctr + Vector3{ 8,0,12 }, ctr + Vector3{ 8,0,2 });

    // hay bales (cylinders on their side) + stacks (cones)
    for (auto& h : s_hay) {
        if (h.stack) DrawModelEx(s_cone, h.pos, Vector3{ 0,1,0 }, 0, Vector3{ h.r * 1.4f, h.r * 2.4f, h.r * 1.4f }, hayC);
        else { rlPushMatrix(); rlTranslatef(h.pos.x, h.r, h.pos.z); rlRotatef(90, 1, 0, 0); DrawModelEx(s_cyl, Vector3{ 0,0,0 }, Vector3{ 0,1,0 }, 0, Vector3{ h.r, h.r * 1.6f, h.r }, hayC); rlPopMatrix(); }
    }

    // scarecrows (post + cross-arm + sack head)
    static const float scA[2] = { 130, 300 };
    for (int i = 0; i < 2; i++) {
        float a = scA[i] * DEG2RAD, r = 9.0f; Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        DrawModelEx(s_cyl, p + Vector3{ 0, 1.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 2.6f, 0.12f }, wood);
        DrawModelEx(s_column, p + Vector3{ 0, 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 0.12f, 0.12f }, wood);
        DrawModelEx(s_dome, p + Vector3{ 0, 2.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.5f, 0.4f }, hayC);
    }

    // wheat tufts (deterministic small golden cones)
    SetRandomSeed(771);
    for (int i = 0; i < 20; i++) { float a = rand_range(0, 2 * PI), r = rand_range(5.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 13.0f) continue; DrawModelEx(s_cone, p, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.9f, 0.4f }, hayC); }

    // barn-window glow (additive warm)
    BeginBlendMode(BLEND_ADDITIVE);
    for (auto& w : s_wisps) { float fl = 0.6f + 0.4f * sinf(s_time * 3.0f + w.x); DrawSphereEx(w, 0.25f + 0.06f * fl, 6, 6, Color{ 255,210,120,90 }); }
    EndBlendMode();
}

// The Ossuary Catacombs: a candlelit, half-flooded crypt of stacked bones, built
// procedurally (no boss_arena.glb), per design/ossuary_catacombs_design.md. Perimeter
// walls of stacked skulls + long-bone courses, a central skull-reliquary, stone
// sarcophagi with effigies, iron candle-stands, and bone piles, reflected in dark
// water. All from lit primitives + draw_bone_seg.
static void draw_bonewall(Vector3 c, float yaw, float w, float h, Color bone, Color boneD) {
    rlPushMatrix();
    rlTranslatef(c.x, 0, c.z);
    rlRotatef(yaw, 0, 1, 0);
    int rows = (int)(h / 0.7f);
    for (int r = 0; r < rows; r++) {
        float y = 0.4f + r * 0.7f;
        if (r % 2 == 0) DrawModelEx(s_cyl, Vector3{ 0, y - 0.18f, 0.0f }, Vector3{ 0,0,1 }, 90, Vector3{ 0.12f, w, 0.12f }, boneD);   // long-bone course
        int nsk = (int)(w / 0.8f);
        for (int i = 0; i < nsk; i++) {
            float x = -w * 0.5f + 0.4f + i * 0.8f;
            DrawModelEx(s_dome, Vector3{ x, y, 0.12f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.32f, 0.3f }, (((i + r) % 5) == 0) ? boneD : bone);
        }
    }
    rlPopMatrix();
}

static void build_ossuary() {
    s_sarcos.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(66606);
    static const float sA[4] = { 40, 130, 220, 310 };
    for (int i = 0; i < 4; i++) { float a = sA[i] * DEG2RAD, r = rand_range(8.0f, 15.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 13.0f) continue; s_sarcos.push_back(Sarco{ p, sA[i] }); }
    static const float cA[5] = { 20, 90, 160, 250, 320 };
    for (int i = 0; i < 5; i++) { float a = cA[i] * DEG2RAD, r = 10.0f + (i % 2) * 3.0f; s_wisps.push_back(ctr + Vector3{ sinf(a) * r, 2.4f, cosf(a) * r }); }
    s_wisps.push_back(ctr + Vector3{ 0, 4.0f, 0 });   // reliquary top
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.5f, w.z, 6.0f }); }
}

static void draw_ossuary() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color bone{ 198,190,170,255 }, boneD{ 150,142,124,255 }, stone{ 78,76,72,255 }, dark{ 40,38,36,255 };

    // perimeter skull-walls (bone panels facing inward; gap toward +Z entrance)
    int n = 16; float wr = boundary_radius - 1.0f, segW = 2.0f * wr * sinf(PI / n) * 1.25f;
    for (int i = 0; i < n; i++) {
        float deg = i * 360.0f / n, dd = deg > 180 ? 360 - deg : deg;
        if (dd < 11.0f) continue;
        float a = deg * DEG2RAD; Vector3 p = ctr + Vector3{ sinf(a) * wr, 0, cosf(a) * wr };
        draw_bonewall(p, deg, segW, 2.8f, bone, boneD);
    }

    // central skull reliquary: a core + stacked rings of skulls + a crowning urn
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 4.0f, 0.6f }, stone);
    for (int t = 0; t < 5; t++) {
        float y = 0.5f + t * 0.8f, rr = 1.6f - t * 0.18f; int ns = 10;
        for (int i = 0; i < ns; i++) { float a = i * (2 * PI / ns) + t * 0.3f; DrawModelEx(s_dome, ctr + Vector3{ sinf(a) * rr, y, cosf(a) * rr }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.32f, 0.3f }, ((i + t) % 4 == 0) ? boneD : bone); }
    }
    DrawModelEx(s_cone, ctr + Vector3{ 0, 4.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 1.2f, 0.9f }, stone);

    // sarcophagi (stone box + lid + a simple effigy)
    for (auto& s : s_sarcos) {
        rlPushMatrix(); rlTranslatef(s.c.x, 0, s.c.z); rlRotatef(s.yaw, 0, 1, 0);
        DrawModelEx(s_column, Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 1.0f, 3.2f }, stone);
        DrawModelEx(s_column, Vector3{ 0, 1.05f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.8f, 0.2f, 3.4f }, dark);
        DrawModelEx(s_column, Vector3{ 0, 1.35f, -0.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.4f, 2.2f }, stone);   // effigy body
        DrawModelEx(s_dome, Vector3{ 0, 1.55f, -1.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.4f, 0.4f }, bone);      // effigy head
        rlPopMatrix();
    }

    // candle stands (the warm lights)
    for (auto& w : s_wisps) {
        if (w.y > 3.5f) continue;
        DrawModelEx(s_cyl, Vector3{ w.x, 1.2f, w.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 2.4f, 0.12f }, dark);
        DrawModelEx(s_cyl, Vector3{ w.x, 0.15f, w.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.35f, 0.3f, 0.35f }, dark);
    }
    // scattered bone piles
    SetRandomSeed(31313);
    for (int i = 0; i < 8; i++) {
        float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 3.0f);
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        if (p.z > ctr.z + 13.0f) continue;
        for (int b = 0; b < 3; b++) { float ba = rand_range(0, 2 * PI); draw_bone_seg(p, p + Vector3{ cosf(ba) * 1.2f, 0.2f, sinf(ba) * 1.2f }, 0.12f, bone); }
        DrawModelEx(s_dome, p + Vector3{ 0, 0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.32f, 0.32f, 0.32f }, boneD);
    }

    // candle flames (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i]; float fy = (w.y > 3.5f) ? w.y : 2.55f, fl = 0.6f + 0.4f * sinf(s_time * 7.0f + i * 2.1f);
        DrawSphereEx(Vector3{ w.x, fy + 0.1f * fl, w.z }, 0.1f + 0.05f * fl, 6, 6, Color{ 255,190,90,150 });
        DrawSphereEx(Vector3{ w.x, fy, w.z }, 0.4f + 0.1f * fl, 8, 8, Color{ 255,150,60,30 });
    }
    EndBlendMode();
}

// The Forsaken Fairground: a derelict rain-slicked carnival, built procedurally (no
// boss_arena.glb), per design/forsaken_fairground_design.md. A slowly turning carousel
// (bobbing horses under a striped canopy), brightly striped tents (reusing draw_tent),
// sagging bunting between light-poles, game stalls, and colored string-lights, reflected
// in the wet ground. Lit primitives; the carousel is spun by s_time.
static void build_fair() {
    s_tents.clear(); s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(45654);
    Color carn[5] = { { 200,60,60,255 }, { 60,90,200,255 }, { 220,190,70,255 }, { 60,170,110,255 }, { 210,210,215,255 } };
    int placed = 0, attempts = 0, want = 8;
    while (placed < want && attempts < want * 30) {
        attempts++;
        float a = rand_range(0, 2 * PI), r = rand_range(8.0f, boundary_radius - 3.0f);
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        if (p.z > ctr.z + 13.0f) continue;
        Tent t; t.c = p; t.yaw = rand_range(0, 360); t.r = rand_range(1.8f, 3.0f); t.h = rand_range(3.5f, 5.5f);
        t.collapsed = (GetRandomValue(0, 5) == 0); t.canvas = carn[GetRandomValue(0, 4)];
        s_tents.push_back(t);
        placed++;
    }
    for (int i = 0; i < 10; i++) { float a = i * 36 * DEG2RAD, r = 12.0f; s_wisps.push_back(ctr + Vector3{ sinf(a) * r, 3.5f, cosf(a) * r }); }
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 2.0f, w.z, 5.0f }); }
}

static void draw_fair() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color wood{ 110,80,52,255 }, dark{ 60,44,30,255 }, gold{ 200,170,90,255 }, red{ 200,60,60,255 }, white{ 215,215,215,255 }, blue{ 60,90,200,255 };
    float spin = s_time * 18.0f;

    // carousel: base + central pole + striped canopy (slowly turning) + bobbing horses on poles
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.2f, 0.6f, 4.2f }, wood);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 3.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 6.0f, 0.4f }, gold);
    DrawModelEx(s_cone, ctr + Vector3{ 0, 6.0f, 0 }, Vector3{ 0,1,0 }, spin * 0.3f, Vector3{ 5.0f, 2.6f, 5.0f }, red);
    rlPushMatrix(); rlTranslatef(ctr.x, 6.0f, ctr.z); rlRotatef(spin * 0.3f, 0, 1, 0);
    for (int i = 0; i < 8; i++) { rlPushMatrix(); rlRotatef(i * 45.0f, 0, 1, 0); DrawModelEx(s_column, Vector3{ 0, 1.3f, 2.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 2.8f, 0.1f }, (i % 2) ? white : red); rlPopMatrix(); }
    rlPopMatrix();
    rlPushMatrix(); rlTranslatef(ctr.x, 0, ctr.z); rlRotatef(spin, 0, 1, 0);
    for (int h = 0; h < 6; h++) {
        rlPushMatrix(); rlRotatef(h * 60.0f, 0, 1, 0); rlTranslatef(0, 0, 3.0f);
        float bob = 0.4f * sinf(s_time * 3.0f + h);
        DrawModelEx(s_cyl, Vector3{ 0, 2.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.08f, 5.0f, 0.08f }, gold);   // pole
        Color hc = (h % 2) ? white : blue;
        DrawModelEx(s_column, Vector3{ 0, 1.6f + bob, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.7f, 1.6f }, hc);     // body
        DrawModelEx(s_column, Vector3{ 0, 2.2f + bob, 0.7f }, Vector3{ 0,1,0 }, 30, Vector3{ 0.35f, 0.8f, 0.4f }, hc);// head/neck
        for (int l = -1; l <= 1; l += 2) for (int f = -1; f <= 1; f += 2) DrawModelEx(s_cyl, Vector3{ l * 0.18f, 1.0f + bob, f * 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.08f, 0.9f, 0.08f }, hc);   // legs
        rlPopMatrix();
    }
    rlPopMatrix();

    // striped carnival tents (reuse draw_tent)
    for (auto& t : s_tents) draw_tent(t, dark);

    // bunting flags between light-poles + the poles
    for (int i = 0; i < 10; i++) {
        float a0 = i * 36 * DEG2RAD, a1 = (i + 1) * 36 * DEG2RAD, r = 12.0f;
        Vector3 p0 = ctr + Vector3{ sinf(a0) * r, 3.5f, cosf(a0) * r }, p1 = ctr + Vector3{ sinf(a1) * r, 3.5f, cosf(a1) * r };
        DrawModelEx(s_cyl, Vector3{ p0.x, 1.75f, p0.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 3.5f, 0.1f }, dark);
        for (int k = 1; k < 5; k++) { float t = k / 5.0f; Vector3 p = Vector3Lerp(p0, p1, t); p.y -= 0.8f * sinf(t * PI); Color fc = ((k % 3) == 0) ? red : ((k % 3) == 1) ? white : blue; DrawModelEx(s_column, p, Vector3{ 0,1,0 }, 45, Vector3{ 0.3f, 0.3f, 0.05f }, fc); }
    }

    // game stalls (counter + striped awning + posts)
    static const float stA[3] = { 60, 180, 300 };
    for (int i = 0; i < 3; i++) {
        float a = stA[i] * DEG2RAD, r = 9.0f; Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        DrawModelEx(s_column, p + Vector3{ 0, 0.8f, 0 }, Vector3{ 0,1,0 }, stA[i], Vector3{ 3.0f, 1.6f, 1.2f }, wood);
        DrawModelEx(s_column, p + Vector3{ 0, 2.6f, -0.3f }, Vector3{ 0,1,0 }, stA[i], Vector3{ 3.2f, 0.2f, 1.6f }, (i % 2) ? red : blue);
        for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cyl, p + Vector3{ s * 1.3f, 1.8f, -0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 2.0f, 0.1f }, dark);
    }

    // colored string-light bulbs (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    Color bulbs[4] = { { 255,120,90,150 }, { 120,160,255,150 }, { 255,220,120,150 }, { 150,255,150,150 } };
    for (size_t i = 0; i < s_wisps.size(); i++) { Vector3 w = s_wisps[i]; float fl = 0.6f + 0.4f * sinf(s_time * 4.0f + i * 1.7f); DrawSphereEx(w, 0.12f + 0.05f * fl, 5, 5, bulbs[i % 4]); }
    EndBlendMode();
}

// The Ruined Throne Hall: a fallen royal hall, built procedurally (no boss_arena.glb),
// per design/ruined_throne_hall_design.md. A colonnade flanks a red carpet runner up to
// a raised dais with a great empty throne, tattered banners sway between the columns,
// chandeliers (s_torus) lie fallen or hang crooked, and stone guardians (draw_sentinel)
// flank the throne. Dry; lit primitives + reused helpers.
static void draw_column(Vector3 p, float h, Color stone, Color dark) {
    DrawModelEx(s_column, p + Vector3{ 0, 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 0.6f, 1.6f }, dark);   // base
    DrawModelEx(s_cyl, p + Vector3{ 0, h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, h, 0.7f }, stone);    // shaft
    DrawModelEx(s_column, p + Vector3{ 0, h + 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 0.5f, 1.5f }, dark);  // capital
    DrawModelEx(s_column, p + Vector3{ 0, h + 0.6f, 0 }, Vector3{ 0,1,0 }, 45, Vector3{ 1.2f, 0.3f, 1.2f }, stone); // abacus
}

static void build_throne() {
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    for (int side = -1; side <= 1; side += 2)
        for (int j = 0; j < 3; j++) s_wisps.push_back(Vector3{ side * 7.5f, 2.6f, ctr.z + 10.0f - j * 8.0f });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.5f, w.z, 6.0f }); }
}

static void draw_throne() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color floorC{ 64,60,58,255 }, stone{ 110,104,96,255 }, dark{ 74,70,64,255 }, gold{ 175,148,80,255 }, goldD{ 140,116,60,255 }, carpet{ 130,40,40,255 }, banner{ 120,44,52,255 }, iron{ 60,58,62,255 };

    // hall floor + red carpet runner
    DrawModelEx(s_column, Vector3{ ctr.x, -0.5f, ctr.z }, Vector3{ 0,1,0 }, 0, Vector3{ 120, 1, 120 }, floorC);
    DrawModelEx(s_column, ctr + Vector3{ 0, 0.07f, 8.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f, 0.14f, 17.0f }, carpet);

    // colonnade (two rows)
    float colZ[4] = { 12, 6, -2, -8 };
    for (int j = 0; j < 4; j++) for (int side = -1; side <= 1; side += 2) draw_column(ctr + Vector3{ side * 6.5f, 0, colZ[j] }, 9.0f, stone, dark);

    // throne dais (3 steps) + the great throne
    for (int s = 0; s < 3; s++) { float w = 8.0f - s * 1.6f; DrawModelEx(s_column, ctr + Vector3{ 0, 0.25f + s * 0.5f, -0.6f }, Vector3{ 0,1,0 }, 0, Vector3{ w, 0.5f, w * 0.8f }, dark); }
    float seatY = 1.9f;
    DrawModelEx(s_column, ctr + Vector3{ 0, seatY, -1.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 0.5f, 2.4f }, gold);          // seat
    DrawModelEx(s_column, ctr + Vector3{ 0, seatY + 2.6f, -2.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 5.2f, 0.5f }, gold);   // tall back
    for (int s = -1; s <= 1; s += 2) {
        DrawModelEx(s_column, ctr + Vector3{ s * 1.6f, seatY + 0.8f, -1.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 1.4f, 2.4f }, goldD);   // arms
        DrawModelEx(s_cone, ctr + Vector3{ s * 1.4f, seatY + 5.4f, -2.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.1f, 0.5f }, gold);      // back finials
    }
    DrawModelEx(s_cone, ctr + Vector3{ 0, seatY + 5.6f, -2.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 1.4f, 0.7f }, goldD);    // crown finial

    // stone guardians flanking the throne (face the entrance)
    draw_sentinel(ctr + Vector3{ -3.6f, 0, -1.0f }, 0.0f, 0.85f, false, stone, dark);
    draw_sentinel(ctr + Vector3{  3.6f, 0, -1.0f }, 0.0f, 0.85f, false, stone, dark);

    // tattered banners hanging from the columns (swaying)
    for (int j = 0; j < 4; j++) for (int side = -1; side <= 1; side += 2) {
        Vector3 cp = ctr + Vector3{ side * 6.5f, 6.5f, colZ[j] };
        rlPushMatrix(); rlTranslatef(cp.x, cp.y, cp.z + 0.7f); rlRotatef(3.0f * sinf(s_time * 1.2f + cp.x + j), 1, 0, 0);
        DrawModelEx(s_column, Vector3{ 0, -2.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 4.4f, 0.08f }, banner);
        rlPopMatrix();
    }

    // fallen chandeliers (rings + candle stubs) + one crooked hanging chandelier
    Vector3 ch[2] = { ctr + Vector3{ -3.5f, 0.2f, 9.0f }, ctr + Vector3{ 4.0f, 0.2f, 3.0f } };
    for (int c = 0; c < 2; c++) {
        DrawModelEx(s_torus, ch[c], Vector3{ 1,0,0 }, 12.0f * (c ? -1 : 1), Vector3{ 1.6f, 1.6f, 1.6f }, iron);
        for (int i = 0; i < 6; i++) { float a = i * 60 * DEG2RAD; DrawModelEx(s_cyl, ch[c] + Vector3{ cosf(a) * 1.5f, 0.25f, sinf(a) * 1.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.08f, 0.4f, 0.08f }, gold); }
    }
    Vector3 hc = ctr + Vector3{ 0.6f, 6.5f, 6.0f };
    draw_bone_seg(ctr + Vector3{ 0, 9.0f, 6.0f }, hc, 0.06f, iron);   // chain
    rlPushMatrix(); rlTranslatef(hc.x, hc.y, hc.z); rlRotatef(16.0f, 0, 0, 1);
    DrawModelEx(s_torus, Vector3{ 0,0,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 1.4f, 1.4f }, iron);
    for (int i = 0; i < 6; i++) { float a = i * 60 * DEG2RAD; DrawModelEx(s_cyl, Vector3{ cosf(a) * 1.3f, 0.3f, sinf(a) * 1.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.07f, 0.4f, 0.07f }, gold); }
    rlPopMatrix();

    // sconce stands + flames (additive)
    for (auto& w : s_wisps) DrawModelEx(s_cyl, Vector3{ w.x, 1.3f, w.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 2.6f, 0.12f }, iron);
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) { Vector3 w = s_wisps[i]; float fl = 0.6f + 0.4f * sinf(s_time * 7.0f + i * 2.1f); DrawSphereEx(Vector3{ w.x, 2.7f + 0.1f * fl, w.z }, 0.12f + 0.05f * fl, 6, 6, Color{ 255,200,110,150 }); DrawSphereEx(Vector3{ w.x, 2.7f, w.z }, 0.4f + 0.1f * fl, 8, 8, Color{ 255,160,70,30 }); }
    DrawSphereEx(hc + Vector3{ 0, 0.3f, 0 }, 0.5f, 8, 8, Color{ 255,180,90,40 });
    EndBlendMode();
}

// The Geyser Springs: a steaming geothermal basin, built procedurally (no
// boss_arena.glb), per design/geyser_springs_design.md. Concentric travertine
// terrace-pool rims (s_torus) of cream/orange mineral around turquoise spring water,
// rising steam columns, bubbling mud pots, mineral mounds, and an erupting central
// geyser. Lit primitives + additive steam over the reused turquoise water.
static void build_springs() {
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    static const float vA[6] = { 30, 90, 150, 210, 270, 330 };
    for (int i = 0; i < 6; i++) { float a = vA[i] * DEG2RAD, r = 8.0f + (i % 3) * 4.0f; s_wisps.push_back(ctr + Vector3{ sinf(a) * r, 0.5f, cosf(a) * r }); }
    s_wisps.push_back(ctr + Vector3{ 0, 0.5f, 0 });   // central geyser
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 0.6f, w.z, 5.0f }); }
}

static void draw_springs() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color cream{ 214,198,168,255 }, orange{ 198,150,96,255 }, mud{ 70,60,50,255 };

    // concentric travertine terrace rims (low flattened torus ledges, rising outward)
    float rr[5] = { 6.0f, 9.5f, 13.0f, 16.5f, 20.0f };
    for (int i = 0; i < 5; i++) DrawModelEx(s_torus, ctr + Vector3{ 0, 0.15f + i * 0.12f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rr[i], rr[i] * 0.5f, rr[i] }, (i % 2) ? cream : orange);

    // mineral mounds
    SetRandomSeed(98765);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), r = rand_range(5.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 13.0f) continue; DrawModelEx(s_dome, p + Vector3{ 0, -0.2f, 0 }, Vector3{ 0,1,0 }, (float)(i * 40), Vector3{ 1.4f + (i % 3) * 0.6f, 0.9f, 1.4f + (i % 3) * 0.6f }, (i % 2) ? cream : orange); }
    // dark mud pots
    SetRandomSeed(54321);
    for (int i = 0; i < 5; i++) { float a = rand_range(0, 2 * PI), r = rand_range(7.0f, boundary_radius - 3.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 13.0f) continue; DrawModelEx(s_cyl, p + Vector3{ 0, 0.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 0.2f, 1.4f }, mud); }

    // central geyser: mineral cone + dark vent
    DrawModelEx(s_cone, ctr, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 1.6f, 2.4f }, cream);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.4f, 0.6f }, mud);

    // erupting geyser jet + steam columns at every vent (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    float jet = 6.0f + 3.0f * fabsf(sinf(s_time * 0.8f));
    for (int k = 0; k < 10; k++) { float t = k / 10.0f; DrawSphereEx(ctr + Vector3{ 0.2f * sinf(s_time * 3 + k), 1.5f + t * jet, 0.2f * cosf(s_time * 3 + k) }, 0.5f + t * 0.6f, 8, 8, Color{ 220,235,235,(unsigned char)(60 * (1.0f - t)) }); }
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        for (int k = 0; k < 6; k++) { float ph = s_time * 0.5f + k * 0.16f + i, t = fmodf(ph, 1.0f); DrawSphereEx(w + Vector3{ 0.4f * sinf(ph * 2 + i), 0.5f + t * 5.0f, 0.4f * cosf(ph * 2 + i) }, 0.5f + t * 0.8f, 7, 7, Color{ 225,228,225,(unsigned char)(45 * (1.0f - t)) }); }
    }
    EndBlendMode();
}

// A petrified tree: a twisted stone trunk (chained draw_bone_seg with a sinuous wiggle)
// that tapers as it rises, forking into a few stubby stone branches. Wholly deterministic
// from `seed` so the forest is stable frame-to-frame.
static void draw_ptree(Vector3 base, float h, float r, int seed, Color bark) {
    SetRandomSeed(seed);
    const int segs = 7;
    float lean = rand_range(-0.5f, 0.5f), wob = rand_range(0.8f, 1.6f), twist = rand_range(0, 2 * PI);
    Vector3 prev = base;
    float seglen = h / segs;
    for (int i = 0; i < segs; i++) {
        float t = (i + 1) / (float)segs;
        // sinuous wiggle: drift sideways with height, modulated by a per-tree wobble
        float off = sinf(t * 3.0f * wob + twist) * 0.55f * h * 0.12f;
        Vector3 nxt = { base.x + lean * t * h * 0.18f + off,
                        base.y + seglen * (i + 1),
                        base.z + cosf(t * 2.4f * wob + twist) * 0.45f * h * 0.12f };
        float rad = r * (1.0f - 0.7f * t);          // taper toward the crown
        draw_bone_seg(prev, nxt, rad < 0.08f ? 0.08f : rad, bark);
        // sprout a forked branch from the upper half
        if (i >= 3 && (i % 2) == 0) {
            float ba = rand_range(0, 2 * PI), bl = rand_range(1.0f, 2.4f);
            Vector3 tip = { nxt.x + sinf(ba) * bl, nxt.y + rand_range(0.4f, 1.2f), nxt.z + cosf(ba) * bl };
            draw_bone_seg(nxt, tip, rad * 0.55f, bark);
            // a second twig off the branch
            Vector3 tip2 = { tip.x + sinf(ba + 1.1f) * bl * 0.6f, tip.y + rand_range(0.2f, 0.7f), tip.z + cosf(ba + 1.1f) * bl * 0.6f };
            draw_bone_seg(tip, tip2, rad * 0.32f, bark);
        }
        prev = nxt;
    }
}

static void build_pforest() {
    s_ptrees.clear();
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(43117);
    int tries = 0;
    while ((int)s_ptrees.size() < 15 && tries < 400) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(5.0f, boundary_radius - 2.5f);
        Vector3 p = ctr + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, ctr) < 4.0f) continue;          // keep the central fallen-log arena clear
        if (p.z > ctr.z + 13.0f) continue;                      // keep the player spawn aisle clear
        bool clash = false;
        for (auto& t : s_ptrees) if (Vector3Distance(p, t.base) < 4.0f) { clash = true; break; }
        if (clash) continue;
        PTree t; t.base = p; t.h = rand_range(7.0f, 13.0f); t.r = rand_range(0.55f, 1.0f); t.seed = 700 + (int)s_ptrees.size() * 37;
        s_ptrees.push_back(t);
        obstacles.push_back({ p, t.r + 0.5f });                 // each trunk blocks movement
    }
    // amber resin pools glow at the base of a few trees + along the fallen log
    for (size_t i = 0; i < s_ptrees.size(); i += 3) s_wisps.push_back(s_ptrees[i].base + Vector3{ 0, 0.4f, 0 });
    s_wisps.push_back(ctr + Vector3{ 0, 0.6f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 0.6f, w.z, 4.5f }); }
}

static void draw_pforest() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color ash{ 120,112,104,255 }, ashDk{ 92,86,80,255 }, bark{ 138,128,116,255 }, barkDk{ 104,96,88,255 };
    Color amber{ 224,150,70,255 };

    // ashen ground slab + scattered ash mounds
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, ash);
    SetRandomSeed(771);
    for (int i = 0; i < 14; i++) {
        float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.0f);
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        DrawModelEx(s_dome, p + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, (float)(i * 33), Vector3{ rand_range(0.9f, 2.0f), rand_range(0.3f, 0.7f), rand_range(0.9f, 2.0f) }, (i % 2) ? ash : ashDk);
    }

    // colossal fallen petrified log across the centre + its torn root-ball
    Vector3 la = ctr + Vector3{ -7.0f, 1.3f, 1.5f }, lb = ctr + Vector3{ 8.0f, 1.6f, -2.0f };
    draw_bone_seg(la, lb, 1.3f, barkDk);
    DrawModelEx(s_dome, la + Vector3{ -1.2f, -0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 2.0f, 2.4f }, bark);   // root ball
    SetRandomSeed(9112);
    for (int i = 0; i < 7; i++) { float a = rand_range(0, 2 * PI); Vector3 tip = la + Vector3{ -1.2f + sinf(a) * 2.6f, rand_range(-0.4f, 1.6f), cosf(a) * 2.6f }; draw_bone_seg(la + Vector3{ -1.2f, 0.2f, 0 }, tip, 0.22f, barkDk); }

    // broken stumps
    SetRandomSeed(3391);
    for (int i = 0; i < 6; i++) {
        float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 3.0f);
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        if (p.z > ctr.z + 12.0f) continue;
        DrawModelEx(s_cyl, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.6f, 1.1f), rand_range(0.8f, 2.0f), rand_range(0.6f, 1.1f) }, (i % 2) ? bark : barkDk);
    }

    // the standing petrified trees
    for (auto& t : s_ptrees) draw_ptree(t.base, t.h, t.r, t.seed, (t.seed % 2) ? bark : barkDk);

    // amber resin glow: pooled at marked bases + drifting motes (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        DrawSphereEx(w + Vector3{ 0, 0.1f, 0 }, 0.7f + 0.15f * sinf(s_time * 1.5f + i), 8, 8, Color{ amber.r, amber.g, amber.b, 70 });
        for (int k = 0; k < 4; k++) { float ph = s_time * 0.4f + k * 0.25f + i, tt = fmodf(ph, 1.0f); DrawSphereEx(w + Vector3{ 0.5f * sinf(ph * 2 + i), 0.4f + tt * 3.0f, 0.5f * cosf(ph * 2 + i) }, 0.18f * (1.0f - tt), 6, 6, Color{ amber.r, amber.g, amber.b, (unsigned char)(90 * (1.0f - tt)) }); }
    }
    EndBlendMode();
}

// A timber cottage: walls + corner posts + dark doorway + a pitched A-frame roof
// (two lit slabs tilted about the local ridge axis) + ridge beam + chimney. Drawn
// inside a yawed rlgl matrix frame so the whole house can face any direction and
// still take light + fog (same trick as the leaned headstones).
static void draw_cottage(Vector3 base, float w, float d, float wallH, float roofH, float yawDeg,
                         Color wall, Color roof, Color dark) {
    float thetaDeg = atan2f(roofH, d * 0.5f) * RAD2DEG;
    float slope = sqrtf((d * 0.5f) * (d * 0.5f) + roofH * roofH) + 0.06f;
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(yawDeg, 0, 1, 0);
    // walls
    DrawModelEx(s_column, Vector3{ 0, wallH * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, wallH, d }, wall);
    // exposed dark timber corner posts
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2)
        DrawModelEx(s_column, Vector3{ sx * (w * 0.5f - 0.06f), wallH * 0.5f, sz * (d * 0.5f - 0.06f) }, Vector3{ 0,1,0 }, 0, Vector3{ 0.17f, wallH + 0.02f, 0.17f }, dark);
    // dark doorway recess on the +z face
    DrawModelEx(s_column, Vector3{ 0, wallH * 0.40f, d * 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ w * 0.34f, wallH * 0.74f, 0.22f }, dark);
    // pitched roof: left slab tilts -theta about local X, right slab +theta (peak at the ridge)
    DrawModelEx(s_column, Vector3{ 0, wallH + roofH * 0.5f, -d * 0.25f }, Vector3{ 1,0,0 }, -thetaDeg, Vector3{ w + 0.5f, 0.16f, slope }, roof);
    DrawModelEx(s_column, Vector3{ 0, wallH + roofH * 0.5f,  d * 0.25f }, Vector3{ 1,0,0 },  thetaDeg, Vector3{ w + 0.5f, 0.16f, slope }, roof);
    // ridge beam + chimney
    DrawModelEx(s_column, Vector3{ 0, wallH + roofH, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w + 0.5f, 0.15f, 0.15f }, dark);
    DrawModelEx(s_column, Vector3{ w * 0.28f, wallH + roofH + 0.45f, d * 0.18f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.36f, 1.1f, 0.36f }, dark);
    rlPopMatrix();
}

static void build_hamlet() {
    s_cottages.clear();
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(52219);
    int tries = 0;
    while ((int)s_cottages.size() < 9 && tries < 500) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(7.0f, boundary_radius - 4.0f);
        Vector3 p = ctr + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, ctr) < 6.0f) continue;          // keep the central well-square clear
        if (p.z > ctr.z + 13.0f) continue;                      // keep the player spawn aisle clear
        bool clash = false;
        for (auto& c : s_cottages) if (Vector3Distance(p, c.base) < 6.5f) { clash = true; break; }
        if (clash) continue;
        Cottage c; c.base = p;
        c.w = rand_range(3.4f, 4.6f); c.d = rand_range(3.0f, 4.0f);
        c.wallH = rand_range(2.6f, 3.4f); c.roofH = rand_range(1.6f, 2.4f);
        // face roughly toward the square
        float toCtr = atan2f(ctr.x - p.x, ctr.z - p.z) * RAD2DEG;
        c.yawDeg = toCtr + rand_range(-25.0f, 25.0f);
        c.variant = (int)s_cottages.size();
        s_cottages.push_back(c);
        obstacles.push_back({ p, (c.w > c.d ? c.w : c.d) * 0.5f + 0.4f });
    }
    // chapel: a fixed landmark across the square from the spawn aisle
    Vector3 chapel = ctr + Vector3{ -2.0f, 0, -16.0f };
    obstacles.push_back({ chapel, 3.0f });
    // sickly plague-lantern glow at the well, the chapel door, and a few cottage doors
    s_wisps.push_back(ctr + Vector3{ 0, 1.2f, 0 });
    s_wisps.push_back(chapel + Vector3{ 0, 1.4f, 3.2f });
    for (size_t i = 0; i < s_cottages.size(); i += 2) s_wisps.push_back(s_cottages[i].base + Vector3{ 0, 1.4f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.2f, w.z, 4.5f }); }
}

static void draw_hamlet() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color dirt{ 96,86,72,255 }, cobble{ 120,116,108,255 }, mud{ 64,58,48,255 };
    Color wallA{ 150,134,108,255 }, wallB{ 132,120,100,255 }, roofA{ 96,70,58,255 }, roofB{ 78,72,66,255 };
    Color tim{ 58,48,40,255 }, stone{ 138,134,126,255 }, plague{ 150,176,86,255 };

    // dirt ground slab + a cobbled square + scattered mud puddles
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, dirt);
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.46f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 13.0f, 1.0f, 13.0f }, cobble);
    SetRandomSeed(331);
    for (int i = 0; i < 12; i++) { float a = rand_range(0, 2 * PI), r = rand_range(5.0f, boundary_radius - 1.5f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.8f, 1.8f), 0.12f, rand_range(0.8f, 1.8f) }, mud); }

    // central well: stone ring + dark shaft + a little gabled roof on two posts + bucket rope
    DrawModelEx(s_cyl, ctr, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 1.1f, 1.5f }, stone);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 1.12f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f, 0.1f, 1.1f }, mud);   // dark water/opening
    for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_column, ctr + Vector3{ sx * 1.2f, 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, 2.4f, 0.22f }, tim);   // posts
    DrawModelEx(s_column, ctr + Vector3{ 0, 3.2f, 0 }, Vector3{ 1,0,0 }, 35.0f, Vector3{ 3.0f, 0.14f, 1.6f }, roofA);   // simple pitched well-roof
    DrawModelEx(s_column, ctr + Vector3{ 0, 3.2f, 0 }, Vector3{ 1,0,0 }, -35.0f, Vector3{ 3.0f, 0.14f, 1.6f }, roofA);
    DrawModelEx(s_column, ctr + Vector3{ 0, 2.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 1.2f, 0.12f }, tim);   // bucket rope

    // chapel: stone nave + a tall tapered steeple topped with a cross
    Vector3 chapel = ctr + Vector3{ -2.0f, 0, -16.0f };
    DrawModelEx(s_column, chapel + Vector3{ 0, 2.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.6f, 4.4f, 6.0f }, stone);
    DrawModelEx(s_column, chapel + Vector3{ 0, 4.4f + 1.0f, -1.5f }, Vector3{ 1,0,0 }, -50.0f, Vector3{ 5.0f, 0.18f, 4.0f }, roofB);
    DrawModelEx(s_column, chapel + Vector3{ 0, 4.4f + 1.0f,  1.5f }, Vector3{ 1,0,0 },  50.0f, Vector3{ 5.0f, 0.18f, 4.0f }, roofB);
    Vector3 tower = chapel + Vector3{ 0, 0, 3.0f };
    DrawModelEx(s_column, tower + Vector3{ 0, 3.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 7.0f, 2.2f }, stone);   // bell tower
    DrawModelEx(s_cone, tower + Vector3{ 0, 7.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 3.4f, 1.5f }, roofB);     // spire
    DrawModelEx(s_column, tower + Vector3{ 0, 10.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 1.0f, 0.12f }, tim);  // cross post
    DrawModelEx(s_column, tower + Vector3{ 0, 10.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.12f, 0.12f }, tim);  // cross arm
    DrawModelEx(s_column, tower + Vector3{ 0, 5.6f, 1.12f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 1.4f, 0.12f }, mud); // dark belfry opening

    // the cottages
    for (auto& c : s_cottages) draw_cottage(c.base, c.w, c.d, c.wallH, c.roofH, c.yawDeg, (c.variant % 2) ? wallA : wallB, (c.variant % 2) ? roofA : roofB, tim);

    // broken fences + a few barrels + plague crosses daubed on doors
    SetRandomSeed(6604);
    for (int i = 0; i < 7; i++) {
        float a = rand_range(0, 2 * PI), r = rand_range(8.0f, boundary_radius - 2.0f);
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        if (p.z > ctr.z + 12.0f) continue;
        // a short fence run: 3 posts + 2 rails
        float fa = rand_range(0, PI);
        Vector3 dir{ cosf(fa), 0, sinf(fa) };
        for (int k = -1; k <= 1; k++) DrawModelEx(s_column, p + Vector3{ dir.x * k * 1.2f, 0.5f, dir.z * k * 1.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.14f, 1.0f, 0.14f }, tim);
        draw_bone_seg(p + Vector3{ -dir.x * 1.2f, 0.7f, -dir.z * 1.2f }, p + Vector3{ dir.x * 1.2f, 0.7f, dir.z * 1.2f }, 0.06f, tim);
        draw_bone_seg(p + Vector3{ -dir.x * 1.2f, 0.4f, -dir.z * 1.2f }, p + Vector3{ dir.x * 1.2f, 0.4f, dir.z * 1.2f }, 0.06f, tim);
        // a barrel beside it
        DrawModelEx(s_cyl, p + Vector3{ dir.z * 0.9f, 0.0f, -dir.x * 0.9f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.45f, 0.9f, 0.45f }, roofA);
    }

    // additive sickly plague glow + drifting motes/flies
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        DrawSphereEx(w, 0.55f + 0.12f * sinf(s_time * 1.6f + i), 8, 8, Color{ plague.r, plague.g, plague.b, 60 });
        for (int k = 0; k < 5; k++) { float ph = s_time * 0.7f + k * 0.2f + i, tt = fmodf(ph, 1.0f); DrawSphereEx(w + Vector3{ 0.7f * sinf(ph * 3 + i), 0.2f + tt * 2.2f, 0.7f * cosf(ph * 3 + i) }, 0.08f * (1.0f - tt), 5, 5, Color{ plague.r, plague.g, plague.b, (unsigned char)(120 * (1.0f - tt)) }); }
    }
    EndBlendMode();
}

// One standing megalith: a lit stone box rooted at the ground. Upright stones draw
// flat; leaning/fallen stones tilt via the rlgl matrix (so they still take light).
static void draw_megalith(Vector3 pos, float w, float h, float t, float yawDeg, float tiltDeg, Color col) {
    if (fabsf(tiltDeg) < 0.01f) {
        DrawModelEx(s_column, pos + Vector3{ 0, h * 0.5f, 0 }, Vector3{ 0,1,0 }, yawDeg, Vector3{ w, h, t }, col);
        return;
    }
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(yawDeg, 0, 1, 0);
    rlRotatef(tiltDeg, 1, 0, 0);
    DrawModelEx(s_column, Vector3{ 0, h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, h, t }, col);
    rlPopMatrix();
}

static void build_henge() {
    s_sarsens.clear();
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    // outer sarsen ring: ~12 uprights, most capped by a lintel (alternating, a few gaps)
    const int N = 12; float R = 16.0f;
    for (int i = 0; i < N; i++) {
        float a = (i / (float)N) * 2 * PI;
        Vector3 p = ctr + Vector3{ sinf(a) * R, 0, cosf(a) * R };
        if (p.z > ctr.z + 13.5f) continue;                       // leave the spawn aisle open
        Sarsen s; s.pos = p; s.w = 2.2f; s.h = 6.0f; s.t = 1.2f;
        s.yawDeg = -a * RAD2DEG;                                  // face the stone tangent to the ring
        s.tiltDeg = (i == 3 || i == 8) ? 14.0f : 0.0f;           // a couple lean
        s.lintel = (i % 2 == 0) && (i != 3 && i != 8);           // capped pairs read as the lintelled ring
        s.variant = i;
        s_sarsens.push_back(s);
        obstacles.push_back({ p, 1.4f });
    }
    // inner horseshoe: 5 great trilithons (taller), opening toward the spawn aisle (+z)
    const int M = 5; float r = 7.5f;
    for (int i = 0; i < M; i++) {
        float a = PI + (i - (M - 1) * 0.5f) * (PI / (M + 1));     // arc on the far (-z) side
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        Sarsen s; s.pos = p; s.w = 2.6f; s.h = 8.5f; s.t = 1.5f;
        s.yawDeg = -a * RAD2DEG; s.tiltDeg = 0.0f; s.lintel = true; s.variant = 100 + i;
        s_sarsens.push_back(s);
        obstacles.push_back({ p, 1.6f });
    }
    // ley-line glow: the altar + a glow at the foot of a few stones
    s_wisps.push_back(ctr + Vector3{ 0, 0.8f, 0 });
    for (size_t i = 0; i < s_sarsens.size(); i += 3) s_wisps.push_back(s_sarsens[i].pos + Vector3{ 0, 0.6f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 0.8f, w.z, 5.0f }); }
}

static void draw_henge() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color turf{ 78,92,64,255 }, turfDk{ 64,76,52,255 }, stone{ 138,136,128,255 }, stoneDk{ 110,108,100,255 };
    Color altar{ 120,118,112,255 }, ley{ 110,220,235,255 };

    // heath turf slab + low grassy barrow mounds + a worn dirt path down the aisle
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, turf);
    SetRandomSeed(414);
    for (int i = 0; i < 16; i++) { float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.2f, 3.0f), rand_range(0.3f, 0.7f), rand_range(1.2f, 3.0f) }, (i % 2) ? turf : turfDk); }

    // central altar stone (flat slab on two low plinths) — obstacle + ritual focus
    DrawModelEx(s_column, ctr + Vector3{ 0, 0.55f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 0.7f, 1.8f }, altar);
    for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_column, ctr + Vector3{ sx * 1.1f, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.5f, 1.4f }, stoneDk);

    // the standing stones (outer ring + inner trilithons); lintel caps where flagged
    for (auto& s : s_sarsens) {
        Color c = (s.variant % 2) ? stone : stoneDk;
        draw_megalith(s.pos, s.w, s.h, s.t, s.yawDeg, s.tiltDeg, c);
        if (s.lintel && s.tiltDeg == 0.0f)
            DrawModelEx(s_column, s.pos + Vector3{ 0, s.h + 0.45f, 0 }, Vector3{ 0,1,0 }, s.yawDeg, Vector3{ s.w * 1.5f, 0.9f, s.t * 1.05f }, c);
    }

    // a lone heel stone outside the ring + a couple of fallen sarsens
    draw_megalith(ctr + Vector3{ 4.0f, 0, 20.0f }, 2.0f, 5.0f, 1.3f, 18.0f, 22.0f, stoneDk);
    rlPushMatrix();
    rlTranslatef(ctr.x - 14.0f, 0.4f, ctr.z + 6.0f); rlRotatef(80.0f, 1, 0, 0);
    DrawModelEx(s_column, Vector3{ 0, 3.0f, 0 }, Vector3{ 0,1,0 }, 30.0f, Vector3{ 2.2f, 6.0f, 1.1f }, stoneDk);   // a toppled stone lying in the grass
    rlPopMatrix();

    // ley-line energy: a soft vertical altar beam + ground-rune glow + drifting motes (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    float pulse = 0.6f + 0.4f * sinf(s_time * 1.3f);
    DrawCube(ctr + Vector3{ 0, 6.0f, 0 }, 0.7f, 12.0f, 0.7f, Color{ ley.r, ley.g, ley.b, (unsigned char)(40 * pulse) });   // altar beam
    DrawCube(ctr + Vector3{ 0, 6.0f, 0 }, 0.35f, 12.0f, 0.35f, Color{ 220, 250, 255, (unsigned char)(55 * pulse) });
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        DrawSphereEx(w, 0.5f + 0.12f * sinf(s_time * 1.5f + i), 8, 8, Color{ ley.r, ley.g, ley.b, 55 });
        for (int k = 0; k < 4; k++) { float ph = s_time * 0.5f + k * 0.25f + i, tt = fmodf(ph, 1.0f); DrawSphereEx(w + Vector3{ 0.6f * sinf(ph * 2 + i), 0.3f + tt * 3.2f, 0.6f * cosf(ph * 2 + i) }, 0.14f * (1.0f - tt), 6, 6, Color{ ley.r, ley.g, ley.b, (unsigned char)(90 * (1.0f - tt)) }); }
    }
    EndBlendMode();
}

// A weeping dead willow: a gnarled tapering trunk (chained draw_bone_seg) crowned
// with drooping branches that arc up then sweep down toward the water, with a few
// hanging moss strands. Deterministic from `seed`.
static void draw_willow(Vector3 base, float h, float r, int seed, Color bark, Color moss) {
    SetRandomSeed(seed);
    const int segs = 5;
    float lean = rand_range(-0.4f, 0.4f), twist = rand_range(0, 2 * PI);
    Vector3 prev = base, crown = base;
    float seglen = h / segs;
    for (int i = 0; i < segs; i++) {
        float t = (i + 1) / (float)segs;
        Vector3 nxt = { base.x + lean * t * h * 0.16f + sinf(t * 2.2f + twist) * 0.4f,
                        base.y + seglen * (i + 1),
                        base.z + cosf(t * 1.8f + twist) * 0.4f };
        float rad = r * (1.0f - 0.55f * t);
        draw_bone_seg(prev, nxt, rad < 0.1f ? 0.1f : rad, bark);
        prev = nxt; crown = nxt;
    }
    // drooping branches sweeping down to the water
    int nb = 6 + (seed % 3);
    for (int b = 0; b < nb; b++) {
        float a = (b / (float)nb) * 2 * PI + rand_range(-0.3f, 0.3f);
        Vector3 dir{ sinf(a), 0, cosf(a) };
        Vector3 start = crown + Vector3{ 0, rand_range(-1.0f, 0.0f), 0 };
        Vector3 apex  = start + dir * rand_range(1.4f, 2.4f) + Vector3{ 0, rand_range(0.4f, 1.2f), 0 };
        float reach = rand_range(2.0f, 3.4f);
        Vector3 mid   = apex + dir * reach * 0.5f + Vector3{ 0, -reach * 0.7f, 0 };
        Vector3 tip   = apex + dir * reach + Vector3{ 0, -(crown.y - base.y) * 0.85f, 0 };
        draw_bone_seg(start, apex, 0.16f, bark);
        draw_bone_seg(apex, mid, 0.12f, moss);
        draw_bone_seg(mid, tip, 0.08f, moss);
        // a couple of hanging moss strands off the mid
        for (int m = 0; m < 2; m++) { Vector3 mm = mid + dir * (0.3f + m * 0.4f); draw_bone_seg(mm, mm + Vector3{ 0, -rand_range(0.8f, 1.8f), 0 }, 0.05f, moss); }
    }
}

static void build_bog() {
    s_willows.clear();
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    SetRandomSeed(80213);
    int tries = 0;
    while ((int)s_willows.size() < 12 && tries < 400) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(5.0f, boundary_radius - 2.5f);
        Vector3 p = ctr + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, ctr) < 4.0f) continue;          // keep the central stump clear
        if (p.z > ctr.z + 13.0f) continue;                      // keep the player spawn aisle clear
        bool clash = false;
        for (auto& w : s_willows) if (Vector3Distance(p, w.base) < 4.5f) { clash = true; break; }
        if (clash) continue;
        Willow w; w.base = p; w.h = rand_range(6.0f, 10.0f); w.r = rand_range(0.5f, 0.9f); w.seed = 800 + (int)s_willows.size() * 41;
        s_willows.push_back(w);
        obstacles.push_back({ p, w.r + 0.5f });
    }
    obstacles.push_back({ ctr + Vector3{ 12.0f, 0, -8.0f }, 2.6f });   // the witch's stilt hut
    // swamp-gas / will-o'-wisp glow: center + a few willow bases + the hut
    s_wisps.push_back(ctr + Vector3{ 0, 0.6f, 0 });
    s_wisps.push_back(ctr + Vector3{ 12.0f, 2.0f, -8.0f });
    for (size_t i = 0; i < s_willows.size(); i += 2) s_wisps.push_back(s_willows[i].base + Vector3{ rand_range(-1.5f,1.5f), 0.4f, rand_range(-1.5f,1.5f) });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 0.6f, w.z, 4.0f }); }
}

static void draw_bog() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color bark{ 60,54,44,255 }, moss{ 70,96,56,255 }, mud{ 58,52,42,255 }, mudDk{ 44,40,32,255 };
    Color lily{ 64,110,64,255 }, reed{ 92,104,66,255 }, wood{ 96,82,62,255 }, woodDk{ 64,54,42,255 };
    Color gas{ 130,210,120,255 };

    // mud banks poking above the murky water (no ground slab — the water plane is the floor)
    SetRandomSeed(2207);
    for (int i = 0; i < 14; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.1f, 0 }, Vector3{ 0,1,0 }, (float)(i * 27), Vector3{ rand_range(1.2f, 2.6f), rand_range(0.3f, 0.6f), rand_range(1.2f, 2.6f) }, (i % 2) ? mud : mudDk); }

    // central great mossy stump (obstacle landmark): wide stump + a domed moss cap + radiating roots
    DrawModelEx(s_cyl, ctr, Vector3{ 0,1,0 }, 0, Vector3{ 1.9f, 1.8f, 1.9f }, bark);
    DrawModelEx(s_dome, ctr + Vector3{ 0, 1.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 0.7f, 2.0f }, moss);
    SetRandomSeed(551);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI); Vector3 tip = ctr + Vector3{ sinf(a) * 3.2f, -0.1f, cosf(a) * 3.2f }; draw_bone_seg(ctr + Vector3{ sinf(a) * 1.6f, 0.3f, cosf(a) * 1.6f }, tip, 0.22f, bark); }

    // cypress knees: clusters of small spikes poking from the water near the willows
    SetRandomSeed(7741);
    for (int i = 0; i < 22; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_cone, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.18f, 0.4f), rand_range(0.5f, 1.4f), rand_range(0.18f, 0.4f) }, bark); }

    // lily pads floating on the surface, a few with a glowing bulb
    SetRandomSeed(3120);
    for (int i = 0; i < 26; i++) { float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0.06f, cosf(a) * r }; DrawModelEx(s_cyl, p, Vector3{ 0,1,0 }, (float)(i * 31), Vector3{ rand_range(0.5f, 1.0f), 0.05f, rand_range(0.5f, 1.0f) }, lily); }

    // reeds: thin tufts in clumps along the banks
    SetRandomSeed(990);
    for (int i = 0; i < 18; i++) { float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 1.0f); Vector3 c = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; for (int k = 0; k < 5; k++) { Vector3 p = c + Vector3{ rand_range(-0.5f,0.5f), 0, rand_range(-0.5f,0.5f) }; draw_bone_seg(p, p + Vector3{ rand_range(-0.2f,0.2f), rand_range(1.0f,1.8f), rand_range(-0.2f,0.2f) }, 0.05f, reed); } }

    // the witch's hut: 4 stilts + a deck + a raised cabin (reusing draw_cottage) + a ladder
    Vector3 hut = ctr + Vector3{ 12.0f, 0, -8.0f };
    float stilt = 2.6f;
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2)
        DrawModelEx(s_cyl, hut + Vector3{ sx * 1.6f, 0, sz * 1.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, stilt, 0.22f }, woodDk);
    DrawModelEx(s_column, hut + Vector3{ 0, stilt, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.4f, 0.25f, 4.0f }, wood);
    draw_cottage(hut + Vector3{ 0, stilt + 0.2f, 0 }, 3.6f, 3.2f, 2.6f, 1.7f, 0.0f, wood, woodDk, woodDk);
    for (int s = 0; s < 2; s++) draw_bone_seg(hut + Vector3{ -0.5f + s * 1.0f, 0, 2.0f }, hut + Vector3{ -0.5f + s * 1.0f, stilt, 1.6f }, 0.07f, woodDk);   // ladder rails
    for (int rg = 0; rg < 4; rg++) { float yy = (rg + 0.5f) * stilt / 4.0f; draw_bone_seg(hut + Vector3{ -0.5f, yy, 1.9f }, hut + Vector3{ 0.5f, yy, 1.9f }, 0.05f, woodDk); }

    // the weeping willows
    for (auto& w : s_willows) draw_willow(w.base, w.h, w.r, w.seed, bark, moss);

    // swamp gas bubbles rising + drifting fireflies (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        DrawSphereEx(w, 0.5f + 0.12f * sinf(s_time * 1.4f + i), 8, 8, Color{ gas.r, gas.g, gas.b, 55 });
        for (int k = 0; k < 5; k++) { float ph = s_time * 0.6f + k * 0.2f + i, tt = fmodf(ph, 1.0f); DrawSphereEx(w + Vector3{ 0.8f * sinf(ph * 2.5f + i), 0.1f + tt * 2.6f, 0.8f * cosf(ph * 2.5f + i) }, 0.1f * (1.0f - tt), 6, 6, Color{ gas.r, gas.g, gas.b, (unsigned char)(110 * (1.0f - tt)) }); }
    }
    EndBlendMode();
}

// A pyramidal stack of horizontal logs. Logs run along the yawed local-X axis; each
// row has one fewer log than the row below, nudged inward so the pile tapers. Every
// log is a lit draw_bone_seg cylinder, with a paler end-cap disc facing the viewer.
static void draw_logpile(Vector3 c, float yawDeg, float logLen, float logR, int rows, Color wood, Color cap) {
    float yr = yawDeg * DEG2RAD;
    Vector3 ax = rotate_y(Vector3{ 1,0,0 }, yr);     // log long-axis
    Vector3 lat = rotate_y(Vector3{ 0,0,1 }, yr);    // lateral stacking direction
    float dx = logR * 1.95f;
    for (int row = 0; row < rows; row++) {
        int cnt = rows - row;
        float y = logR + row * (logR * 1.78f);
        float start = -(cnt - 1) * 0.5f * dx;
        for (int i = 0; i < cnt; i++) {
            Vector3 mid = Vector3Add(Vector3Add(c, Vector3Scale(lat, start + i * dx)), Vector3{ 0, y, 0 });
            Vector3 a = Vector3Subtract(mid, Vector3Scale(ax, logLen * 0.5f));
            Vector3 b = Vector3Add(mid, Vector3Scale(ax, logLen * 0.5f));
            draw_bone_seg(a, b, logR, wood);
            DrawModelEx(s_cyl, b, ax, 90.0f, Vector3{ logR * 0.98f, 0.12f, logR * 0.98f }, cap);   // pale sawn end-cap
        }
    }
}

// The giant circular sawblade: a thin lit steel disc standing in a vertical plane,
// spun about its own (horizontal) axle by s_time, ringed with cutting teeth.
static void draw_sawblade(Vector3 pos, float R, Color steel, Color dark) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(90.0f, 1, 0, 0);              // tip the disc upright (axle now along world Z)
    rlRotatef(s_time * 220.0f, 0, 1, 0);    // spin about the axle
    DrawModelEx(s_cyl, Vector3{ 0, -0.04f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ R, 0.08f, R }, steel);          // blade plate
    DrawModelEx(s_cyl, Vector3{ 0, -0.06f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ R * 0.30f, 0.14f, R * 0.30f }, dark); // hub
    const int teeth = 18;
    for (int i = 0; i < teeth; i++) {
        float a = (i / (float)teeth) * 2 * PI;
        Vector3 p{ cosf(a) * R, 0, sinf(a) * R };
        DrawModelEx(s_column, p, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 0.45f, 0.1f, 0.18f }, steel);
    }
    rlPopMatrix();
}

static void build_lumber() {
    s_logpiles.clear();
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    // the central stack (matches the obstacle pushed in load)
    s_logpiles.push_back({ ctr, 25.0f, 9.0f, 0.7f, 4 });
    // scattered yard stacks
    SetRandomSeed(60413);
    int tries = 0;
    while ((int)s_logpiles.size() < 6 && tries < 400) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(8.0f, boundary_radius - 4.0f);
        Vector3 p = ctr + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, ctr) < 7.0f) continue;
        if (p.z > ctr.z + 13.0f) continue;                      // keep the spawn aisle clear
        bool clash = false;
        for (auto& l : s_logpiles) if (Vector3Distance(p, l.c) < 7.0f) { clash = true; break; }
        if (clash) continue;
        LogPile l; l.c = p; l.yawDeg = rand_range(0, 180.0f); l.logLen = rand_range(6.0f, 9.0f); l.logR = rand_range(0.5f, 0.75f); l.rows = 3 + GetRandomValue(0, 1);
        s_logpiles.push_back(l);
        obstacles.push_back({ p, l.logLen * 0.45f });
    }
    obstacles.push_back({ ctr + Vector3{ -11.0f, 0, -10.0f }, 4.0f });   // the mill shed
    // warm work-lamps at the shed + a couple of stacks
    s_wisps.push_back(ctr + Vector3{ -11.0f, 3.0f, -10.0f });
    s_wisps.push_back(ctr + Vector3{ -8.0f, 2.4f, -7.0f });
    for (size_t i = 1; i < s_logpiles.size(); i += 2) s_wisps.push_back(s_logpiles[i].c + Vector3{ 0, 2.0f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 2.2f, w.z, 4.5f }); }
}

static void draw_lumber() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color dirt{ 104,90,68,255 }, chips{ 150,128,92,255 }, wood{ 142,112,76,255 }, woodDk{ 104,82,56,255 };
    Color cap{ 198,168,120,255 }, beam{ 90,72,52,255 }, steel{ 180,184,190,255 }, steelDk{ 96,100,108,255 };

    // dirt yard slab + scattered sawdust/chip patches
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, dirt);
    SetRandomSeed(1201);
    for (int i = 0; i < 16; i++) { float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.0f, 2.2f), 0.12f, rand_range(1.0f, 2.2f) }, chips); }

    // cut stumps scattered around
    SetRandomSeed(818);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; DrawModelEx(s_cyl, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.7f, 1.1f), rand_range(0.4f, 0.9f), rand_range(0.7f, 1.1f) }, woodDk); DrawModelEx(s_cyl, p + Vector3{ 0, rand_range(0.4f, 0.9f), 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.7f, 1.1f), 0.06f, rand_range(0.7f, 1.1f) }, cap); }

    // plank stacks: low piles of flat boards
    SetRandomSeed(444);
    for (int i = 0; i < 5; i++) {
        float a = rand_range(0, 2 * PI), r = rand_range(7.0f, boundary_radius - 3.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        if (p.z > ctr.z + 12.0f) continue;
        float yaw = rand_range(0, 180.0f);
        for (int k = 0; k < 6; k++) DrawModelEx(s_column, p + Vector3{ 0, 0.12f + k * 0.16f, 0 }, Vector3{ 0,1,0 }, yaw, Vector3{ 3.2f, 0.14f, 0.9f }, (k % 2) ? wood : cap);
    }

    // the log piles
    for (auto& l : s_logpiles) draw_logpile(l.c, l.yawDeg, l.logLen, l.logR, l.rows, (l.rows % 2) ? wood : woodDk, cap);

    // the mill shed: open timber-framed shed with a pitched roof, housing the sawblade
    Vector3 shed = ctr + Vector3{ -11.0f, 0, -10.0f };
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2)
        DrawModelEx(s_column, shed + Vector3{ sx * 3.4f, 2.0f, sz * 3.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 4.0f, 0.4f }, beam);   // corner posts
    DrawModelEx(s_column, shed + Vector3{ 0, 4.0f, -2.8f }, Vector3{ 1,0,0 }, -38.0f, Vector3{ 7.6f, 0.2f, 4.4f }, woodDk);   // roof slabs
    DrawModelEx(s_column, shed + Vector3{ 0, 4.0f,  2.8f }, Vector3{ 1,0,0 },  38.0f, Vector3{ 7.6f, 0.2f, 4.4f }, woodDk);
    DrawModelEx(s_column, shed + Vector3{ 0, 2.2f, -3.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 7.0f, 4.4f, 0.3f }, wood);          // back wall
    // saw table + a log being cut + the spinning blade rising through the slot
    DrawModelEx(s_column, shed + Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.0f, 0.3f, 1.4f }, beam);             // table
    for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_column, shed + Vector3{ sx * 2.0f, 0.45f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.9f, 1.2f }, beam);  // legs
    draw_bone_seg(shed + Vector3{ -2.4f, 1.25f, 0.0f }, shed + Vector3{ 1.2f, 1.25f, 0.0f }, 0.5f, wood);                    // the log on the table
    DrawModelEx(s_cyl, shed + Vector3{ 1.2f, 1.25f, 0.0f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.5f, 0.1f, 0.5f }, cap);
    draw_sawblade(shed + Vector3{ -0.4f, 1.6f, 0.0f }, 1.7f, steel, steelDk);

    // a log-crane gantry: two leaning posts + a beam + a hanging chain & hook
    Vector3 cr = ctr + Vector3{ 9.0f, 0, 4.0f };
    draw_bone_seg(cr + Vector3{ -2.0f, 0, 0 }, cr + Vector3{ -0.6f, 5.0f, 0 }, 0.22f, beam);
    draw_bone_seg(cr + Vector3{  2.0f, 0, 0 }, cr + Vector3{  0.6f, 5.0f, 0 }, 0.22f, beam);
    DrawModelEx(s_column, cr + Vector3{ 0, 5.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.4f, 0.3f, 0.3f }, beam);
    draw_bone_seg(cr + Vector3{ 1.4f, 5.0f, 0 }, cr + Vector3{ 1.4f, 2.2f + 0.5f * sinf(s_time * 0.7f), 0 }, 0.05f, steelDk);   // swaying chain
    DrawModelEx(s_cyl, cr + Vector3{ 1.4f, 2.0f + 0.5f * sinf(s_time * 0.7f), 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 0.5f, 0.18f }, steelDk);   // hook block

    // warm work-lamp glow + drifting sawdust motes (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        DrawSphereEx(w, 0.5f + 0.1f * sinf(s_time * 2.0f + i), 8, 8, Color{ 255, 200, 120, 60 });
        for (int k = 0; k < 5; k++) { float ph = s_time * 0.5f + k * 0.2f + i, tt = fmodf(ph, 1.0f); DrawSphereEx(w + Vector3{ 1.0f * sinf(ph * 2 + i), 0.2f - tt * 1.4f, 1.0f * cosf(ph * 2 + i) }, 0.06f * (1.0f - tt), 5, 5, Color{ 220, 190, 140, (unsigned char)(90 * (1.0f - tt)) }); }
    }
    EndBlendMode();
}

// Fixed perimeter layout for the gaol, shared by build_gaol (obstacles/lights) and
// draw_gaol (geometry) so the two never drift. cells: xyz = position, w = yaw degrees
// (the barred front faces the courtyard). gibbets: xyz = post base, w = sway phase.
static void gaol_layout(std::vector<Vector4>& cells, std::vector<Vector4>& gibbets) {
    Vector3 c = boundary_center;
    cells.clear(); gibbets.clear();
    for (int i = -2; i <= 2; i++) cells.push_back({ c.x + i * 4.2f, c.y, c.z - 16.0f, 0.0f });    // back row, faces +z
    for (int i = -1; i <= 1; i++) cells.push_back({ c.x - 16.0f, c.y, c.z + i * 4.2f - 4.0f, 90.0f });   // left wall, faces +x
    for (int i = -1; i <= 1; i++) cells.push_back({ c.x + 16.0f, c.y, c.z + i * 4.2f - 4.0f, -90.0f });  // right wall, faces -x
    gibbets.push_back({ c.x - 8.0f, c.y, c.z - 7.0f, 0.0f });
    gibbets.push_back({ c.x + 8.0f, c.y, c.z - 7.0f, 1.7f });
    gibbets.push_back({ c.x - 6.0f, c.y, c.z + 5.0f, 3.1f });
    gibbets.push_back({ c.x + 6.0f, c.y, c.z + 5.0f, 4.4f });
}

// A gaol cell: stone back + side walls + roof, with an iron-barred front (top &
// bottom rails + a row of vertical bars). Drawn in a yawed rlgl frame so the barred
// face can point any direction.
static void draw_cell(Vector3 pos, float yawDeg, Color stone, Color stoneDk, Color iron) {
    float w = 3.6f, h = 3.6f, d = 3.4f;
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(yawDeg, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, h * 0.5f, -d * 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ w, h, 0.4f }, stone);          // back wall
    for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_column, Vector3{ sx * w * 0.5f, h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, h, d }, stoneDk);   // side walls
    DrawModelEx(s_column, Vector3{ 0, h + 0.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w + 0.3f, 0.3f, d + 0.3f }, stone);    // roof slab
    // barred front (at +z)
    DrawModelEx(s_column, Vector3{ 0, h - 0.2f, d * 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ w, 0.22f, 0.22f }, iron);       // top rail
    DrawModelEx(s_column, Vector3{ 0, 0.2f, d * 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ w, 0.22f, 0.22f }, iron);           // bottom rail
    int nb = 7;
    for (int i = 0; i < nb; i++) { float x = -w * 0.5f + (i + 0.5f) * (w / nb); DrawModelEx(s_cyl, Vector3{ x, 0, d * 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.07f, h, 0.07f }, iron); }
    rlPopMatrix();
}

// A gibbet: an L-frame (post + arm) from which an iron cage hangs on a chain and
// sways with s_time; a dark slumped shape inside hints at an occupant.
static void draw_gibbet(Vector3 base, float phase, Color iron, Color dark) {
    DrawModelEx(s_cyl, base, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 4.2f, 0.2f }, dark);                          // post
    DrawModelEx(s_column, base + Vector3{ 0.9f, 4.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 0.2f, 0.2f }, dark);   // arm
    Vector3 hang = base + Vector3{ 1.7f, 4.1f, 0 };
    float sway = 0.5f * sinf(s_time * 0.9f + phase);
    Vector3 cageTop = hang + Vector3{ sway, -1.0f, 0 };
    draw_bone_seg(hang, cageTop, 0.05f, dark);                                                                  // chain
    // cage: 4 corner bars + top & bottom square rails
    float cs = 0.55f, ch = 1.5f;
    Vector3 cc = cageTop + Vector3{ 0, -ch, 0 };   // cage base centre
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2)
        DrawModelEx(s_cyl, cc + Vector3{ sx * cs, 0, sz * cs }, Vector3{ 0,1,0 }, 0, Vector3{ 0.06f, ch, 0.06f }, iron);
    for (int t = 0; t <= 1; t++) { float yy = cc.y + t * ch; DrawModelEx(s_column, Vector3{ cc.x, yy, cc.z }, Vector3{ 0,1,0 }, 0, Vector3{ cs * 2 + 0.12f, 0.08f, 0.12f }, iron); DrawModelEx(s_column, Vector3{ cc.x, yy, cc.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 0.08f, cs * 2 + 0.12f }, iron); }
    DrawModelEx(s_dome, cc + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.45f, 0.5f, 0.45f }, dark);  // slumped occupant
}

static void build_gaol() {
    s_wisps.clear();
    std::vector<Vector4> cells, gibbets;
    gaol_layout(cells, gibbets);
    for (auto& c : cells) obstacles.push_back({ Vector3{ c.x, 0, c.z }, 2.1f });
    for (auto& g : gibbets) obstacles.push_back({ Vector3{ g.x, 0, g.z }, 0.5f });
    Vector3 ctr = boundary_center;
    // torch glow: braziers at the corners + the scaffold + a couple of cells
    s_wisps.push_back(ctr + Vector3{ 0, 1.4f, 0 });
    s_wisps.push_back(ctr + Vector3{ -10.0f, 1.4f, 8.0f });
    s_wisps.push_back(ctr + Vector3{ 10.0f, 1.4f, 8.0f });
    for (size_t i = 0; i < cells.size(); i += 3) s_wisps.push_back(Vector3{ cells[i].x, 2.2f, cells[i].z });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.6f, w.z, 4.5f }); }
}

static void draw_gaol() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color flag{ 96,92,86,255 }, flagDk{ 78,74,68,255 }, stone{ 120,116,108,255 }, stoneDk{ 96,92,86,255 };
    Color iron{ 54,50,48,255 }, wood{ 92,72,52,255 }, fire{ 255,150,60,255 };

    // flagstone courtyard + a darker worn central path
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, flag);
    SetRandomSeed(1717);
    for (int i = 0; i < 12; i++) { float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_column, p + Vector3{ 0, -0.46f, 0 }, Vector3{ 0,1,0 }, (float)(i * 24), Vector3{ rand_range(2.0f, 4.0f), 1.0f, rand_range(2.0f, 4.0f) }, (i % 2) ? flagDk : flag); }

    // broken outer wall arc behind the cells (back + sides)
    for (int i = -3; i <= 3; i++) DrawModelEx(s_column, ctr + Vector3{ i * 5.5f, 2.4f, -20.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 5.2f, rand_range(4.0f, 5.5f), 0.8f }, (i % 2) ? stone : stoneDk);

    // the cells + the gibbets
    std::vector<Vector4> cells, gibbets;
    gaol_layout(cells, gibbets);
    for (auto& c : cells) draw_cell(Vector3{ c.x, 0, c.z }, c.w, stone, stoneDk, iron);
    for (auto& g : gibbets) draw_gibbet(Vector3{ g.x, 0, g.z }, g.w, iron, stoneDk);

    // central gallows scaffold: raised platform on posts + steps + an upright frame with two nooses
    DrawModelEx(s_column, ctr + Vector3{ 0, 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.0f, 0.3f, 4.0f }, wood);   // platform
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2)
        DrawModelEx(s_column, ctr + Vector3{ sx * 1.7f, 0.6f, sz * 1.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.2f, 0.3f }, wood);   // legs
    for (int s = 0; s < 3; s++) DrawModelEx(s_column, ctr + Vector3{ 0, 0.2f + s * 0.3f, 2.2f + s * 0.45f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 0.25f, 0.5f }, wood);   // steps
    for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_column, ctr + Vector3{ sx * 1.6f, 3.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 4.4f, 0.3f }, wood);   // uprights
    DrawModelEx(s_column, ctr + Vector3{ 0, 5.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.8f, 0.3f, 0.3f }, wood);   // crossbeam
    for (int sx = -1; sx <= 1; sx += 2) { Vector3 top = ctr + Vector3{ sx * 0.9f, 5.35f, 0 }; draw_bone_seg(top, top + Vector3{ 0.1f * sinf(s_time + sx), -1.6f, 0 }, 0.04f, iron); }   // swaying nooses

    // torch braziers (stone bowl on a post) at the scaffold + courtyard corners
    Vector3 br[3] = { ctr + Vector3{ -10.0f, 0, 8.0f }, ctr + Vector3{ 10.0f, 0, 8.0f }, ctr + Vector3{ 0, 0, -2.5f } };
    for (int i = 0; i < 3; i++) { DrawModelEx(s_cyl, br[i], Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 1.4f, 0.18f }, iron); DrawModelEx(s_dome, br[i] + Vector3{ 0, 1.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.35f, 0.5f }, stoneDk); }

    // additive torch flames + warm glow + drifting embers
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 3; i++) { Vector3 f = br[i] + Vector3{ 0, 1.7f, 0 }; for (int k = 0; k < 4; k++) DrawSphereEx(f + Vector3{ 0.12f * sinf(s_time * 6 + k + i), 0.18f * k, 0.12f * cosf(s_time * 6 + k + i) }, 0.34f - 0.06f * k, 7, 7, Color{ fire.r, fire.g, fire.b, (unsigned char)(120 - 22 * k) }); }
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        DrawSphereEx(w, 0.4f + 0.1f * sinf(s_time * 2 + i), 8, 8, Color{ fire.r, fire.g, fire.b, 45 });
        for (int k = 0; k < 3; k++) { float ph = s_time * 0.7f + k * 0.33f + i, tt = fmodf(ph, 1.0f); DrawSphereEx(w + Vector3{ 0.5f * sinf(ph * 3 + i), tt * 2.4f, 0.5f * cosf(ph * 3 + i) }, 0.05f * (1.0f - tt), 5, 5, Color{ 255, 180, 110, (unsigned char)(110 * (1.0f - tt)) }); }
    }
    EndBlendMode();
}

// Fixed layout for the tar pits, shared by build_tar (obstacles/lights) and draw_tar
// (geometry). trees: xyz = base, w = height. pumps: xyz = skid base, w = yaw degrees.
static void tar_layout(std::vector<Vector4>& trees, std::vector<Vector4>& pumps) {
    Vector3 c = boundary_center;
    trees.clear(); pumps.clear();
    pumps.push_back({ c.x - 11.0f, c.y, c.z - 6.0f, 25.0f });
    pumps.push_back({ c.x + 12.0f, c.y, c.z - 4.0f, -50.0f });
    pumps.push_back({ c.x + 2.0f, c.y, c.z + 9.0f, 200.0f });
    SetRandomSeed(40519);
    int tries = 0;
    while ((int)trees.size() < 9 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(5.0f, boundary_radius - 2.5f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 4.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& t : trees) if (Vector3Distance(p, Vector3{ t.x,0,t.z }) < 3.5f) { clash = true; break; }
        for (auto& pm : pumps) if (Vector3Distance(p, Vector3{ pm.x,0,pm.z }) < 4.0f) { clash = true; break; }
        if (clash) continue;
        trees.push_back({ p.x, p.y, p.z, rand_range(5.0f, 9.0f) });
    }
}

// A "nodding donkey" pumpjack: a Samson A-frame carrying a walking beam that rocks
// about its pivot with s_time (a horsehead front, a counterweight crank rear), over
// a wellhead. The animated signature of the tar pits.
static void draw_pump(Vector3 base, float yawDeg, float phase, Color steel, Color dark) {
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(yawDeg, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.0f, 0.5f, 1.4f }, dark);    // base skid
    draw_bone_seg(Vector3{ -0.9f, 0.4f, 0 }, Vector3{ 0, 3.4f, 0 }, 0.13f, steel);                            // Samson post legs
    draw_bone_seg(Vector3{ 0.9f, 0.4f, 0 }, Vector3{ 0, 3.4f, 0 }, 0.13f, steel);
    DrawModelEx(s_cyl, Vector3{ 2.6f, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.3f, 0.3f }, dark);        // wellhead
    // walking beam rocks about the pivot
    float rock = 15.0f * sinf(s_time * 1.3f + phase);
    rlPushMatrix();
    rlTranslatef(0, 3.4f, 0);
    rlRotatef(rock, 0, 0, 1);
    DrawModelEx(s_column, Vector3{ 0, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.6f, 0.3f, 0.32f }, steel);      // the beam
    DrawModelEx(s_column, Vector3{ 2.8f, -0.45f, 0 }, Vector3{ 0,0,1 }, 18.0f, Vector3{ 0.55f, 1.0f, 0.45f }, steel);  // horsehead
    DrawModelEx(s_cyl, Vector3{ -2.8f, 0, 0 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.55f, 0.5f, 0.55f }, dark);          // counterweight crank
    rlPopMatrix();
    rlPopMatrix();
}

static void build_tar() {
    s_wisps.clear();
    std::vector<Vector4> trees, pumps;
    tar_layout(trees, pumps);
    for (auto& t : trees) obstacles.push_back({ Vector3{ t.x, 0, t.z }, 0.9f });
    for (auto& p : pumps) obstacles.push_back({ Vector3{ p.x, 0, p.z }, 2.2f });
    Vector3 ctr = boundary_center;
    // sulfur-seep glow at the central geyser + a few pools/pumps
    s_wisps.push_back(ctr + Vector3{ 0, 1.0f, 0 });
    for (auto& p : pumps) s_wisps.push_back(Vector3{ p.x, 1.0f, p.z });
    SetRandomSeed(7180);
    for (int i = 0; i < 4; i++) { float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 3.0f); s_wisps.push_back(ctr + Vector3{ sinf(a) * r, 0.4f, cosf(a) * r }); }
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 0.6f, w.z, 4.5f }); }
}

static void draw_tar() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color earth{ 78,66,52,255 }, earthDk{ 60,50,40,255 }, tar{ 18,16,16,255 }, tarSheen{ 44,38,32,255 };
    Color steel{ 96,92,90,255 }, dark{ 30,28,28,255 }, sulfur{ 220,200,90,255 }, pitch{ 40,28,18,255 };

    // cracked-earth ground slab + dark dried patches
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, earth);
    SetRandomSeed(2061);
    for (int i = 0; i < 12; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.2f, 2.6f), 0.12f, rand_range(1.2f, 2.6f) }, earthDk); }

    // tar pools: overlapping near-black flat discs sitting just above the ground
    SetRandomSeed(909);
    std::vector<Vector3> pools;
    for (int i = 0; i < 9; i++) {
        float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 3.0f);
        Vector3 p = ctr + Vector3{ sinf(a) * r, 0.04f, cosf(a) * r };
        pools.push_back(p);
        int blobs = 2 + GetRandomValue(0, 2);
        for (int b = 0; b < blobs; b++) { Vector3 q = p + Vector3{ rand_range(-2.0f,2.0f), 0, rand_range(-2.0f,2.0f) }; DrawModelEx(s_cyl, q, Vector3{ 0,1,0 }, (float)(b * 40), Vector3{ rand_range(2.0f, 3.8f), 0.06f, rand_range(2.0f, 3.8f) }, tar); }
    }
    // central tar geyser mound
    DrawModelEx(s_dome, ctr + Vector3{ 0, -0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 1.4f, 2.6f }, pitch);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 0.5f, 0.8f }, tar);

    // tar mounds / asphalt seeps
    SetRandomSeed(333);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), r = rand_range(5.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.8f, 1.6f), rand_range(0.4f, 0.9f), rand_range(0.8f, 1.6f) }, pitch); }

    // tar-blackened dead trees (reuse the petrified-tree silhouette in near-black) + pumps
    std::vector<Vector4> trees, pumps;
    tar_layout(trees, pumps);
    for (size_t i = 0; i < trees.size(); i++) draw_ptree(Vector3{ trees[i].x, 0, trees[i].z }, trees[i].w, 0.5f, 900 + (int)i * 31, pitch);
    for (size_t i = 0; i < pumps.size(); i++) draw_pump(Vector3{ pumps[i].x, 0, pumps[i].z }, pumps[i].w, (float)i * 1.7f, steel, dark);

    // additive: tar sheen on pools + swelling/popping bubbles + central jet + sulfur smoke
    BeginBlendMode(BLEND_ADDITIVE);
    // central erupting tar jet (dark, with a hot sulfur core)
    float jet = 4.0f + 2.0f * fabsf(sinf(s_time * 0.9f));
    for (int k = 0; k < 9; k++) { float t = k / 9.0f; DrawSphereEx(ctr + Vector3{ 0.2f * sinf(s_time * 3 + k), 1.0f + t * jet, 0.2f * cosf(s_time * 3 + k) }, 0.45f + t * 0.5f, 8, 8, Color{ pitch.r, pitch.g, pitch.b, (unsigned char)(70 * (1.0f - t)) }); }
    for (size_t i = 0; i < pools.size(); i++) {
        Vector3 p = pools[i];
        DrawSphereEx(p + Vector3{ 0, 0.05f, 0 }, 1.0f, 8, 8, Color{ tarSheen.r, tarSheen.g, tarSheen.b, 40 });   // sheen
        // a few bubbles swelling then popping
        for (int b = 0; b < 3; b++) {
            float ph = fmodf(s_time * 0.6f + b * 0.33f + i * 0.21f, 1.0f);
            float rr = 0.15f + ph * 0.45f;
            unsigned char al = (unsigned char)(120 * (1.0f - ph));
            DrawSphereEx(p + Vector3{ 0.6f * sinf(b + i) , 0.1f + ph * 0.3f, 0.6f * cosf(b + i) }, rr, 7, 7, Color{ pitch.r, pitch.g, pitch.b, al });
        }
    }
    // sulfur smoke at the seeps
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        for (int k = 0; k < 4; k++) { float ph = s_time * 0.4f + k * 0.25f + i, tt = fmodf(ph, 1.0f); DrawSphereEx(w + Vector3{ 0.5f * sinf(ph * 2 + i), 0.3f + tt * 3.0f, 0.5f * cosf(ph * 2 + i) }, 0.4f * (1.0f - tt) + 0.1f, 6, 6, Color{ sulfur.r, sulfur.g, sulfur.b, (unsigned char)(45 * (1.0f - tt)) }); }
    }
    EndBlendMode();
}

// Seeded cypress layout for the vineyard, shared by build_vine (obstacles) and
// draw_vine (geometry). xyz = base, w = height.
static void vine_layout(std::vector<Vector4>& cyp) {
    Vector3 c = boundary_center;
    cyp.clear();
    SetRandomSeed(33041);
    int tries = 0;
    while ((int)cyp.size() < 8 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(10.0f, boundary_radius - 1.5f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (p.z > c.z + 13.5f) continue;
        bool clash = false;
        for (auto& t : cyp) if (Vector3Distance(p, Vector3{ t.x,0,t.z }) < 4.0f) { clash = true; break; }
        if (clash) continue;
        cyp.push_back({ p.x, p.y, p.z, rand_range(5.0f, 8.0f) });
    }
}

// One row of grape trellis: posts joined by wires, with leafy clumps and hanging
// purple grape clusters along the span. Runs from `start` along `dir`.
static void draw_trellis(Vector3 start, Vector3 dir, int posts, float spacing, Color wood, Color leaf, Color grape) {
    Vector3 prev = start;
    for (int i = 0; i < posts; i++) {
        Vector3 p = start + dir * (i * spacing);
        DrawModelEx(s_cyl, p, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 2.0f, 0.1f }, wood);   // post
        if (i > 0) {
            for (int h = 0; h < 3; h++) { float y = 0.7f + h * 0.5f; draw_bone_seg(prev + Vector3{ 0,y,0 }, p + Vector3{ 0,y,0 }, 0.03f, wood); }
            Vector3 mid = (prev + p) * 0.5f + Vector3{ 0, 1.45f, 0 };
            DrawModelEx(s_dome, mid, Vector3{ 0,1,0 }, (float)(i * 37), Vector3{ spacing * 0.55f, 0.7f, 0.9f }, leaf);   // foliage clump
            DrawModelEx(s_cone, mid + Vector3{ 0, -0.1f, 0.35f }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 0.22f, 0.55f, 0.22f }, grape);   // hanging cluster
        }
        prev = p;
    }
}

static void build_vine() {
    s_wisps.clear();
    std::vector<Vector4> cyp;
    vine_layout(cyp);
    for (auto& t : cyp) obstacles.push_back({ Vector3{ t.x, 0, t.z }, 0.8f });
    Vector3 ctr = boundary_center;
    Vector3 villa = ctr + Vector3{ -15.0f, 0, -14.0f };
    obstacles.push_back({ villa, 4.0f });
    Vector3 barrels[3] = { ctr + Vector3{ 12.0f, 0, -12.0f }, ctr + Vector3{ 14.0f, 0, 6.0f }, ctr + Vector3{ -10.0f, 0, 8.0f } };
    for (auto& b : barrels) obstacles.push_back({ b, 1.6f });
    // warm golden lamps at the villa, press, barrels
    s_wisps.push_back(ctr + Vector3{ 0, 1.4f, 0 });
    s_wisps.push_back(villa + Vector3{ 0, 2.0f, 3.0f });
    for (auto& b : barrels) s_wisps.push_back(b + Vector3{ 0, 1.4f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.4f, w.z, 4.5f }); }
}

static void draw_vine() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color earth{ 122,98,66,255 }, earthDk{ 100,80,54,255 }, grass{ 110,120,70,255 };
    Color wood{ 110,82,54,255 }, leaf{ 86,120,58,255 }, grape{ 92,52,104,255 };
    Color oak{ 120,86,54,255 }, oakCap{ 168,132,86,255 }, stucco{ 196,176,138,255 }, roof{ 150,86,62,255 }, dark{ 70,54,40,255 };

    // terraced earth ground + low retaining banks + grassy strips
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, earth);
    for (int i = -2; i <= 2; i++) DrawModelEx(s_column, ctr + Vector3{ 0, -0.3f, i * 5.0f }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.0f, 0.4f, 0.6f }, earthDk);   // terrace banks
    SetRandomSeed(2401);
    for (int i = 0; i < 12; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.0f, 2.2f), 0.1f, rand_range(1.0f, 2.2f) }, grass); }

    // rows of grape trellis (running along +x), skipping the central press row
    float rowZ[5] = { -15.0f, -10.0f, -5.0f, 5.0f, 10.0f };
    for (int r = 0; r < 5; r++) draw_trellis(ctr + Vector3{ -18.0f, 0, rowZ[r] }, Vector3{ 1,0,0 }, 13, 3.0f, wood, leaf, grape);

    // central wine press: a big oak vat + a screw press (post + beam + wheel) on top
    DrawModelEx(s_cyl, ctr, Vector3{ 0,1,0 }, 0, Vector3{ 1.8f, 1.6f, 1.8f }, oak);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 0.15f, 1.6f }, oakCap);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 2.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 2.4f, 0.18f }, dark);   // screw
    DrawModelEx(s_column, ctr + Vector3{ 0, 4.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 0.25f, 0.25f }, wood);   // press beam
    DrawModelEx(s_torus, ctr + Vector3{ 0, 4.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 0.8f, 0.8f }, dark);   // capstan wheel

    // oak-barrel stacks (reuse draw_logpile with short fat "logs")
    Vector3 barrels[3] = { ctr + Vector3{ 12.0f, 0, -12.0f }, ctr + Vector3{ 14.0f, 0, 6.0f }, ctr + Vector3{ -10.0f, 0, 8.0f } };
    for (int i = 0; i < 3; i++) draw_logpile(barrels[i], (float)(i * 40), 1.5f, 0.6f, 3, oak, oakCap);

    // the Tuscan farmhouse (reuse draw_cottage, scaled up)
    Vector3 villa = ctr + Vector3{ -15.0f, 0, -14.0f };
    draw_cottage(villa, 6.0f, 5.0f, 4.2f, 2.6f, 35.0f, stucco, roof, dark);

    // cypress trees lining the field
    std::vector<Vector4> cyp;
    vine_layout(cyp);
    for (auto& t : cyp) { DrawModelEx(s_cyl, Vector3{ t.x, 0, t.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.0f, 0.3f }, dark); DrawModelEx(s_cone, Vector3{ t.x, 0.6f, t.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.95f, t.w, 0.95f }, leaf); }

    // a few grape crates near the press
    SetRandomSeed(660);
    for (int i = 0; i < 4; i++) { Vector3 p = ctr + Vector3{ rand_range(-4.0f,4.0f), 0.3f, 3.0f + rand_range(-1.0f,1.0f) }; DrawModelEx(s_column, p, Vector3{ 0,1,0 }, (float)(i * 25), Vector3{ 1.0f, 0.6f, 0.7f }, wood); DrawModelEx(s_dome, p + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.45f, 0.3f, 0.32f }, grape); }

    // warm golden-hour glow + drifting dust/gnats (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        DrawSphereEx(w, 0.45f + 0.1f * sinf(s_time * 1.8f + i), 8, 8, Color{ 255, 210, 140, 55 });
        for (int k = 0; k < 4; k++) { float ph = s_time * 0.4f + k * 0.25f + i, tt = fmodf(ph, 1.0f); DrawSphereEx(w + Vector3{ 1.0f * sinf(ph * 2 + i), 0.3f + tt * 1.6f, 1.0f * cosf(ph * 2 + i) }, 0.05f * (1.0f - tt), 5, 5, Color{ 255, 225, 170, (unsigned char)(80 * (1.0f - tt)) }); }
    }
    EndBlendMode();
}

// Seeded layout for the frozen floes, shared by build_floes (obstacles/lights) and
// draw_floes (geometry). floes: xyz = centre (at sea level), w = radius. bergs: xyz =
// base, w = size.
static void ice_layout(std::vector<Vector4>& floes, std::vector<Vector4>& bergs) {
    Vector3 c = boundary_center;
    floes.clear(); bergs.clear();
    SetRandomSeed(51177);
    for (int i = 0; i < 16; i++) { float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 1.5f); floes.push_back({ c.x + sinf(a) * r, 0.0f, c.z + cosf(a) * r, rand_range(2.0f, 4.5f) }); }
    int tries = 0;
    while ((int)bergs.size() < 5 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(7.0f, boundary_radius - 3.0f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 6.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& b : bergs) if (Vector3Distance(p, Vector3{ b.x,0,b.z }) < 7.0f) { clash = true; break; }
        if (clash) continue;
        bergs.push_back({ p.x, p.y, p.z, rand_range(2.2f, 3.8f) });
    }
}

// A jagged iceberg: a submerged base shelf + a main peak + leaning secondary peaks,
// faceted in pale ice tones. Deterministic from `seed`.
static void draw_iceberg(Vector3 base, float size, int seed, Color ice, Color iceDk, Color iceLt) {
    SetRandomSeed(seed);
    DrawModelEx(s_dome, base + Vector3{ 0, -0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ size * 1.5f, size * 0.5f, size * 1.5f }, iceDk);   // base shelf
    DrawModelEx(s_cone, base, Vector3{ 0,1,0 }, 0, Vector3{ size * 0.95f, size * 2.2f, size * 0.95f }, ice);                            // main peak
    int n = 3 + (seed % 2);
    for (int i = 0; i < n; i++) {
        float a = rand_range(0, 2 * PI), r = rand_range(size * 0.4f, size * 0.9f);
        Vector3 p = base + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        DrawModelEx(s_cone, p, Vector3{ sinf(a), 0, cosf(a) }, rand_range(-12.0f, 12.0f), Vector3{ size * 0.5f, rand_range(size * 1.0f, size * 1.8f), size * 0.5f }, (i % 2) ? iceLt : ice);
    }
    DrawModelEx(s_dome, base + Vector3{ 0, size * 1.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ size * 0.4f, size * 0.4f, size * 0.4f }, iceLt);   // faceted cap
}

static void build_floes() {
    s_wisps.clear();
    std::vector<Vector4> floes, bergs;
    ice_layout(floes, bergs);
    for (auto& b : bergs) obstacles.push_back({ Vector3{ b.x, 0, b.z }, b.w * 0.8f });
    Vector3 ctr = boundary_center;
    // pale ice / aurora glow at the central berg + a few bergs + scattered floes
    s_wisps.push_back(ctr + Vector3{ 0, 2.0f, 0 });
    for (auto& b : bergs) s_wisps.push_back(Vector3{ b.x, 1.5f, b.z });
    for (size_t i = 0; i < floes.size(); i += 4) s_wisps.push_back(Vector3{ floes[i].x, 0.5f, floes[i].z });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 4.5f }); }
}

static void draw_floes() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color ice{ 212,232,246,255 }, iceDk{ 150,182,208,255 }, iceLt{ 236,248,255,255 }, snow{ 240,246,252,255 };
    Color wood{ 96,80,60,255 }, woodDk{ 66,54,42,255 };

    std::vector<Vector4> floes, bergs;
    ice_layout(floes, bergs);

    // pack-ice floes: flat pale slabs riding the sea (no ground slab — water is the floor)
    SetRandomSeed(8021);
    for (auto& f : floes) {
        Vector3 c = Vector3{ f.x, 0.12f, f.z };
        int blobs = 2 + GetRandomValue(0, 2);
        for (int b = 0; b < blobs; b++) { Vector3 q = c + Vector3{ rand_range(-f.w * 0.5f, f.w * 0.5f), 0, rand_range(-f.w * 0.5f, f.w * 0.5f) }; DrawModelEx(s_cyl, q, Vector3{ 0,1,0 }, (float)(b * 47), Vector3{ f.w * rand_range(0.7f, 1.1f), 0.16f, f.w * rand_range(0.7f, 1.1f) }, (b % 2) ? ice : iceLt); }
        if (GetRandomValue(0, 1)) DrawModelEx(s_dome, c + Vector3{ 0, 0.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ f.w * 0.5f, 0.4f, f.w * 0.5f }, snow);   // snow drift
    }

    // icebergs + the central great iceberg
    for (size_t i = 0; i < bergs.size(); i++) draw_iceberg(Vector3{ bergs[i].x, 0, bergs[i].z }, bergs[i].w, 600 + (int)i * 53, ice, iceDk, iceLt);
    draw_iceberg(ctr, 3.6f, 999, ice, iceDk, iceLt);

    // serac ice-spike clusters
    SetRandomSeed(414);
    for (int i = 0; i < 7; i++) { float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 2.0f); Vector3 c = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; for (int k = 0; k < 4; k++) { Vector3 p = c + Vector3{ rand_range(-1.0f,1.0f), 0, rand_range(-1.0f,1.0f) }; DrawModelEx(s_cone, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.3f,0.6f), rand_range(1.2f,2.6f), rand_range(0.3f,0.6f) }, (k % 2) ? ice : iceLt); } }

    // a frozen jetty: plank deck on posts reaching in from the near edge
    Vector3 j = ctr + Vector3{ 4.0f, 0, 14.0f };
    for (int s = 0; s < 5; s++) { Vector3 p = j + Vector3{ 0, 0, -s * 2.2f }; for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_cyl, p + Vector3{ sx * 1.0f, -0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 1.0f, 0.18f }, woodDk); DrawModelEx(s_column, p + Vector3{ 0, 0.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 0.18f, 2.0f }, wood); }

    // additive: aurora ribbons high in the sky + glints on ice + falling snow
    BeginBlendMode(BLEND_ADDITIVE);
    for (int b = 0; b < 3; b++) {
        float yy = 22.0f + b * 3.0f;
        float sway = 4.0f * sinf(s_time * 0.4f + b);
        unsigned char g = (unsigned char)(120 + 60 * sinf(s_time * 0.6f + b));
        DrawCube(ctr + Vector3{ sway, yy, -10.0f - b * 4.0f }, 60.0f, 3.0f, 1.5f, Color{ 80, g, (unsigned char)(160 + 40 * sinf(s_time + b)), 26 });
    }
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.12f * sinf(s_time * 1.5f + i), 8, 8, Color{ ice.r, ice.g, ice.b, 40 });
    // falling snow specks
    SetRandomSeed(2024);
    for (int i = 0; i < 60; i++) {
        float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius), sp = rand_range(0.5f, 1.2f), off = rand_range(0, 1);
        float y = 12.0f - fmodf(s_time * sp + off, 1.0f) * 12.0f;
        DrawSphereEx(ctr + Vector3{ bx + sinf(s_time + i) * 0.5f, y, bz }, 0.06f, 4, 4, Color{ 245, 250, 255, 150 });
    }
    EndBlendMode();
}

// A timber shear-legs crane: a tripod of leaning legs + a boom, from which a cut
// stone block hangs on a chain and swings with s_time. The animated quarry signature.
static void draw_crane(Vector3 base, float phase, Color wood, Color iron, Color stone) {
    Vector3 top = base + Vector3{ 0, 5.5f, 0 };
    draw_bone_seg(base + Vector3{ -2.0f, 0, -1.2f }, top, 0.22f, wood);
    draw_bone_seg(base + Vector3{ 2.0f, 0, -1.2f }, top, 0.22f, wood);
    draw_bone_seg(base + Vector3{ 0, 0, 2.0f }, top, 0.22f, wood);          // tripod third leg
    Vector3 tip = base + Vector3{ 0, 4.0f, 4.0f };
    draw_bone_seg(top, tip, 0.18f, wood);                                   // boom
    draw_bone_seg(top, base + Vector3{ 0, 4.6f, 2.2f }, 0.08f, iron);       // stay
    float sw = 0.7f * sinf(s_time * 0.8f + phase);
    Vector3 blk = tip + Vector3{ sw, -2.8f, 0 };
    draw_bone_seg(tip, blk + Vector3{ 0, 1.0f, 0 }, 0.05f, iron);           // chain
    DrawModelEx(s_column, blk, Vector3{ 0,1,0 }, sw * 25.0f, Vector3{ 1.5f, 1.5f, 1.5f }, stone);   // swinging cut block
}

static void build_quarry() {
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    Vector3 crane = ctr + Vector3{ 10.0f, 0, 6.0f };
    Vector3 colossus = ctr + Vector3{ 0, 0, -19.0f };
    Vector3 stacks[3] = { ctr + Vector3{ -12.0f, 0, -6.0f }, ctr + Vector3{ 12.0f, 0, -11.0f }, ctr + Vector3{ -8.0f, 0, 8.0f } };
    obstacles.push_back({ crane, 2.2f });
    obstacles.push_back({ colossus, 3.5f });
    for (auto& s : stacks) obstacles.push_back({ s, 2.0f });
    // warm work-lamps at the crane, central block, stacks
    s_wisps.push_back(ctr + Vector3{ 0, 1.6f, 0 });
    s_wisps.push_back(crane + Vector3{ 0, 2.0f, 0 });
    for (auto& s : stacks) s_wisps.push_back(s + Vector3{ 0, 1.6f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.4f, w.z, 4.5f }); }
}

static void draw_quarry() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color rock{ 142,134,120,255 }, rockDk{ 112,105,93,255 }, rockLt{ 168,160,144,255 };
    Color wood{ 110,84,56,255 }, iron{ 70,66,62,255 }, dust{ 150,140,120,255 };

    // dusty quarry floor + scattered rubble patches
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, rockDk);
    SetRandomSeed(1330);
    for (int i = 0; i < 14; i++) { float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.0f, 2.4f), rand_range(0.2f, 0.5f), rand_range(1.0f, 2.4f) }, (i % 2) ? rock : dust); }

    // terraced quarry walls: concentric rings of cut rock benches rising outward
    // (the +z front arc left open as the spawn ramp/aisle)
    float rr[3] = { 17.0f, 20.0f, 23.0f }, hh[3] = { 1.6f, 3.4f, 5.4f };
    for (int t = 0; t < 3; t++) {
        for (int j = 0; j < 18; j++) {
            float a = (j / 18.0f) * 2 * PI;
            Vector3 p = ctr + Vector3{ sinf(a) * rr[t], hh[t] * 0.5f, cosf(a) * rr[t] };
            if (p.z > ctr.z + 14.0f) continue;                  // leave the entrance ramp open
            DrawModelEx(s_column, p, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 6.6f, hh[t], 3.2f }, (j % 2) ? rock : rockDk);
        }
    }

    // central great cut block with chisel grooves
    DrawModelEx(s_column, ctr + Vector3{ 0, 1.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.6f, 2.8f, 3.0f }, rock);
    for (int i = 0; i < 4; i++) DrawModelEx(s_column, ctr + Vector3{ -1.5f + i * 1.0f, 1.4f, 1.51f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 2.6f, 0.06f }, rockDk);   // grooves

    // cut-block stacks (small pyramids of megalith cubes)
    Vector3 stacks[3] = { ctr + Vector3{ -12.0f, 0, -6.0f }, ctr + Vector3{ 12.0f, 0, -11.0f }, ctr + Vector3{ -8.0f, 0, 8.0f } };
    for (int s = 0; s < 3; s++) {
        Vector3 b = stacks[s];
        for (int k = 0; k < 3; k++) DrawModelEx(s_column, b + Vector3{ 0, 0.8f + k * 1.5f, 0 }, Vector3{ 0,1,0 }, (float)(s * 20 + k * 8), Vector3{ 2.6f - k * 0.5f, 1.5f, 2.0f - k * 0.4f }, (k % 2) ? rock : rockLt);
    }

    // the timber shear-legs crane (swinging block)
    draw_crane(ctr + Vector3{ 10.0f, 0, 6.0f }, 0.0f, wood, iron, rock);

    // a scaffold ramp leaning against the wall
    Vector3 sc = ctr + Vector3{ -16.0f, 0, 2.0f };
    draw_bone_seg(sc, sc + Vector3{ 3.0f, 5.0f, 0 }, 0.16f, wood);
    draw_bone_seg(sc + Vector3{ 0, 0, 1.5f }, sc + Vector3{ 3.0f, 5.0f, 1.5f }, 0.16f, wood);
    for (int k = 0; k < 5; k++) DrawModelEx(s_column, sc + Vector3{ 0.6f * k, 1.0f * k, 0.75f }, Vector3{ 0,0,1 }, 59.0f, Vector3{ 0.1f, 1.4f, 1.6f }, wood);   // rungs/planks

    // a half-carved colossus emerging from the back wall (rough hewn giant head)
    Vector3 col = ctr + Vector3{ 0, 0, -19.0f };
    DrawModelEx(s_column, col + Vector3{ 0, 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.0f, 6.0f, 4.0f }, rockDk);   // rough block
    DrawModelEx(s_dome, col + Vector3{ 0, 6.0f, 0.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 2.8f, 2.2f }, rock);    // head
    for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_column, col + Vector3{ sx * 0.9f, 6.3f, 1.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.45f, 0.3f }, iron);   // eye sockets
    DrawModelEx(s_column, col + Vector3{ 0, 5.7f, 2.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.2f, 0.6f }, rock);   // nose

    // warm work-lamp glow + drifting rock dust (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        DrawSphereEx(w, 0.45f + 0.1f * sinf(s_time * 1.7f + i), 8, 8, Color{ 255, 205, 130, 55 });
        for (int k = 0; k < 4; k++) { float ph = s_time * 0.3f + k * 0.25f + i, tt = fmodf(ph, 1.0f); DrawSphereEx(w + Vector3{ 1.0f * sinf(ph * 2 + i), 0.2f + tt * 1.4f, 1.0f * cosf(ph * 2 + i) }, 0.06f * (1.0f - tt), 5, 5, Color{ 210, 196, 168, (unsigned char)(80 * (1.0f - tt)) }); }
    }
    EndBlendMode();
}

// A spinning propeller: a hub + 3 radiating blades turning about its axis with s_time.
static void draw_prop(Vector3 pos, float phase, Color blade, Color hub) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(s_time * 360.0f + phase, 0, 0, 1);
    for (int b = 0; b < 3; b++) {
        rlPushMatrix();
        rlRotatef(b * 120.0f, 0, 0, 1);
        DrawModelEx(s_column, Vector3{ 0, 0.75f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 1.5f, 0.05f }, blade);
        rlPopMatrix();
    }
    DrawModelEx(s_dome, Vector3{ 0, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.25f, 0.25f, 0.25f }, hub);
    rlPopMatrix();
}

// The moored dirigible: an elongated gas envelope (capsule + domed caps), tail fins,
// a hanging gondola with windows, mooring struts and two spinning propellers. The
// whole ship bobs gently with s_time. `c` is the envelope-centre anchor (high up).
static void draw_airship(Vector3 c) {
    Color silver{ 202,208,216,255 }, silverDk{ 166,174,184,255 }, stripe{ 178,72,60,255 };
    Color fin{ 150,152,162,255 }, cabin{ 120,98,70,255 }, cabinDk{ 90,72,52,255 }, glass{ 44,54,74,255 }, iron{ 72,72,78,255 };
    float bob = 0.6f * sinf(s_time * 0.5f);
    Vector3 mid = c + Vector3{ 0, bob, 0 };
    Vector3 L = mid + Vector3{ -8.0f, 0, 0 }, R = mid + Vector3{ 8.0f, 0, 0 };
    draw_bone_seg(L, R, 3.2f, silver);                                                   // envelope body
    DrawModelEx(s_dome, L, Vector3{ 0,0,1 }, -90.0f, Vector3{ 3.2f, 3.4f, 3.2f }, silver);    // nose cap
    DrawModelEx(s_dome, R, Vector3{ 0,0,1 }, 90.0f, Vector3{ 3.2f, 3.4f, 3.2f }, silverDk);   // tail cap
    draw_bone_seg(L + Vector3{ 0, 3.05f, 0 }, R + Vector3{ 0, 3.05f, 0 }, 0.18f, stripe);     // ridge stripe
    // tail fins at the rear
    DrawModelEx(s_column, R + Vector3{ 1.4f, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 0.2f, 5.0f }, fin);
    DrawModelEx(s_column, R + Vector3{ 1.4f, 2.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 4.2f, 0.2f }, fin);
    DrawModelEx(s_column, R + Vector3{ 1.4f, -2.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 4.2f, 0.2f }, fin);
    // gondola hung below the centre
    Vector3 g = mid + Vector3{ 0, -4.4f, 0 };
    DrawModelEx(s_column, g, Vector3{ 0,1,0 }, 0, Vector3{ 6.0f, 1.8f, 2.4f }, cabin);
    DrawModelEx(s_column, g + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.6f, 0.3f, 2.0f }, cabinDk);
    for (int i = -2; i <= 2; i++) DrawModelEx(s_column, g + Vector3{ i * 1.0f, 0.1f, 1.21f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.7f, 0.06f }, glass);   // windows
    draw_bone_seg(mid + Vector3{ -3.0f, -3.0f, 0 }, g + Vector3{ -2.5f, 0.9f, 0 }, 0.08f, iron);    // struts
    draw_bone_seg(mid + Vector3{ 3.0f, -3.0f, 0 }, g + Vector3{ 2.5f, 0.9f, 0 }, 0.08f, iron);
    draw_prop(g + Vector3{ -3.5f, -0.2f, -1.0f }, 0.0f, fin, iron);                       // propellers
    draw_prop(g + Vector3{ 3.5f, -0.2f, -1.0f }, 60.0f, fin, iron);
}

static void build_dock() {
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    // corner mooring masts (the central mast is the obstacle pushed in load)
    Vector3 masts[4] = { ctr + Vector3{ -15.0f, 0, -12.0f }, ctr + Vector3{ 15.0f, 0, -12.0f }, ctr + Vector3{ -15.0f, 0, 8.0f }, ctr + Vector3{ 15.0f, 0, 8.0f } };
    for (auto& m : masts) obstacles.push_back({ m, 0.8f });
    obstacles.push_back({ ctr + Vector3{ -18.0f, 0, -2.0f }, 2.2f });   // control tower
    // warm signal lamps at the masts + tower + central mast
    s_wisps.push_back(ctr + Vector3{ 0, 6.0f, 0 });
    for (auto& m : masts) s_wisps.push_back(m + Vector3{ 0, 7.5f, 0 });
    s_wisps.push_back(ctr + Vector3{ -18.0f, 4.0f, -2.0f });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 2.0f, w.z, 4.5f }); }
}

static void draw_dock() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color deck{ 122,112,98,255 }, deckDk{ 96,88,76,255 }, plank{ 120,96,66,255 }, iron{ 76,76,82,255 };
    Color rope{ 120,104,78,255 }, lamp{ 255,196,110,255 };

    // stone-and-timber dock platform + plank decking strips + a low parapet rim
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.0f, 1.0f, boundary_radius * 2.0f }, deck);
    for (int i = -5; i <= 5; i++) DrawModelEx(s_column, ctr + Vector3{ i * 3.6f, -0.04f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 1.0f, boundary_radius * 1.9f }, deckDk);   // plank seams
    for (int j = 0; j < 24; j++) { float a = (j / 24.0f) * 2 * PI; Vector3 p = ctr + Vector3{ sinf(a) * (boundary_radius - 1.0f), 0.4f, cosf(a) * (boundary_radius - 1.0f) }; if (p.z > ctr.z + 14.0f) continue; DrawModelEx(s_column, p, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 1.6f, 0.8f, 0.3f }, deckDk); }   // parapet posts

    // central mooring mast (tower) the airship nose tethers to
    DrawModelEx(s_cyl, ctr, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 8.0f, 0.8f }, iron);
    DrawModelEx(s_dome, ctr + Vector3{ 0, 8.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 0.8f, 1.2f }, deckDk);

    // corner mooring masts
    Vector3 masts[4] = { ctr + Vector3{ -15.0f, 0, -12.0f }, ctr + Vector3{ 15.0f, 0, -12.0f }, ctr + Vector3{ -15.0f, 0, 8.0f }, ctr + Vector3{ 15.0f, 0, 8.0f } };
    for (auto& m : masts) { DrawModelEx(s_cyl, m, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 7.5f, 0.4f }, iron); DrawModelEx(s_dome, m + Vector3{ 0, 7.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.4f, 0.6f }, lamp); }

    // a control tower with a glazed cab + a gangway ramp
    Vector3 tw = ctr + Vector3{ -18.0f, 0, -2.0f };
    DrawModelEx(s_column, tw + Vector3{ 0, 2.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 5.0f, 3.0f }, deckDk);
    DrawModelEx(s_column, tw + Vector3{ 0, 5.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 1.4f, 3.4f }, iron);
    for (int s = 0; s < 4; s++) DrawModelEx(s_column, tw + Vector3{ 0, 5.4f, 1.71f - s * 0.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 0.9f, 0.06f }, Color{ 60,80,110,255 });

    // the moored airship, floating above the central mast
    Vector3 ship = ctr + Vector3{ 0, 13.0f, 0 };
    draw_airship(ship);

    // mooring ropes: ship nose to the central mast, gondola corners to corner masts
    float bob = 0.6f * sinf(s_time * 0.5f);
    Vector3 nose = ctr + Vector3{ -8.0f, 13.0f + bob, 0 };
    draw_bone_seg(ctr + Vector3{ 0, 8.0f, 0 }, nose, 0.05f, rope);
    Vector3 gond = ctr + Vector3{ 0, 13.0f + bob - 4.4f, 0 };
    for (auto& m : masts) draw_bone_seg(m + Vector3{ 0, 7.5f, 0 }, gond + Vector3{ (m.x < ctr.x ? -2.8f : 2.8f), 0, (m.z < ctr.z ? -1.0f : 1.0f) }, 0.04f, rope);

    // dockside cargo: crates + barrels + gas cylinders
    SetRandomSeed(1390);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 4.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; DrawModelEx(s_column, p + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, (float)(i * 27), Vector3{ 1.2f, 1.2f, 1.2f }, plank); if (i % 2) DrawModelEx(s_cyl, p + Vector3{ 1.4f, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.4f, 0.5f }, iron); }

    // a cloud band hanging around the platform edge to sell the altitude (additive)
    BeginBlendMode(BLEND_ADDITIVE);
    SetRandomSeed(77);
    for (int i = 0; i < 26; i++) { float a = rand_range(0, 2 * PI), r = rand_range(boundary_radius - 2.0f, boundary_radius + 6.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, -1.5f + 1.0f * sinf(s_time * 0.2f + i), cosf(a) * r }; DrawSphereEx(p, rand_range(2.0f, 3.5f), 7, 7, Color{ 235, 240, 248, 26 }); }
    // warm signal-lamp glow + slow blink
    for (size_t i = 0; i < s_wisps.size(); i++) { float bl = 0.6f + 0.4f * sinf(s_time * 2.0f + i); DrawSphereEx(s_wisps[i], 0.4f + 0.12f * bl, 8, 8, Color{ lamp.r, lamp.g, lamp.b, (unsigned char)(70 * bl) }); }
    EndBlendMode();
}

// Seeded palm layout for the conservatory, shared by build_glass (obstacles) and
// draw_glass (geometry). xyz = base, w = height.
static void glass_layout(std::vector<Vector4>& palms) {
    Vector3 c = boundary_center;
    palms.clear();
    SetRandomSeed(60619);
    int tries = 0;
    while ((int)palms.size() < 7 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(7.0f, 17.0f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 5.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& t : palms) if (Vector3Distance(p, Vector3{ t.x,0,t.z }) < 4.5f) { clash = true; break; }
        if (clash) continue;
        palms.push_back({ p.x, p.y, p.z, rand_range(5.0f, 8.0f) });
    }
}

// A palm: a gently curving trunk (chained draw_bone_seg) crowned with a ring of
// arching fronds (segment pairs + a leaf blade), with a few hanging fruit.
static void draw_palm(Vector3 base, float h, Color trunk, Color frond, Color fruit) {
    int segs = 5; Vector3 prev = base;
    for (int i = 0; i < segs; i++) { float t = (i + 1) / (float)segs; Vector3 nxt = base + Vector3{ sinf(t * 1.4f) * 0.6f, h * t, cosf(t * 0.8f) * 0.3f }; draw_bone_seg(prev, nxt, 0.35f * (1.0f - 0.4f * t), trunk); prev = nxt; }
    Vector3 crown = prev;
    int nf = 9;
    for (int i = 0; i < nf; i++) {
        float a = (i / (float)nf) * 2 * PI;
        Vector3 dir{ sinf(a), 0, cosf(a) };
        Vector3 mid = crown + dir * 1.7f + Vector3{ 0, 0.6f, 0 };
        Vector3 tip = crown + dir * 3.4f + Vector3{ 0, -1.4f, 0 };
        draw_bone_seg(crown, mid, 0.11f, frond);
        draw_bone_seg(mid, tip, 0.07f, frond);
        DrawModelEx(s_dome, mid, dir, 70.0f, Vector3{ 0.5f, 0.18f, 1.4f }, frond);   // leaf blade
    }
    for (int i = 0; i < 4; i++) { float a = i * 1.57f; DrawModelEx(s_dome, crown + Vector3{ sinf(a) * 0.4f, -0.4f, cosf(a) * 0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.25f, 0.25f, 0.25f }, fruit); }
}

static void build_glass() {
    s_wisps.clear();
    std::vector<Vector4> palms;
    glass_layout(palms);
    for (auto& t : palms) obstacles.push_back({ Vector3{ t.x, 0, t.z }, 0.9f });
    Vector3 ctr = boundary_center;
    // soft sun-through-glass glow at the apex, central palm, and a few palms/beds
    s_wisps.push_back(ctr + Vector3{ 0, 10.0f, 0 });
    s_wisps.push_back(ctr + Vector3{ 0, 2.0f, 0 });
    for (size_t i = 0; i < palms.size(); i += 2) s_wisps.push_back(Vector3{ palms[i].x, 3.0f, palms[i].z });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.4f, w.z, 5.0f }); }
}

static void draw_glass() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color tile{ 176,170,158,255 }, tileDk{ 150,144,132,255 }, soil{ 78,62,48,255 }, stone{ 150,148,138,255 };
    Color iron{ 58,58,60,255 }, trunk{ 122,94,62,255 }, frond{ 72,142,66,255 }, fruit{ 150,110,60,255 };
    Color bloom[4] = { {206,86,96,255}, {224,184,80,255}, {150,96,196,255}, {230,150,170,255} };
    Color glass{ 150,220,205,255 };
    float Rg = 20.0f, Hh = 14.0f;

    // tiled floor + a darker tile border + a central round bed kerb
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.0f, 1.0f, boundary_radius * 2.0f }, tile);
    for (int i = -6; i <= 6; i++) { DrawModelEx(s_column, ctr + Vector3{ i * 3.4f, -0.04f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 1.0f, boundary_radius * 1.9f }, tileDk); DrawModelEx(s_column, ctr + Vector3{ 0, -0.04f, i * 3.4f }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 1.9f, 1.0f, 0.12f }, tileDk); }

    // low stone base ring (planter wall), front arc left open
    for (int j = 0; j < 24; j++) { float a = (j / 24.0f) * 2 * PI; Vector3 p = ctr + Vector3{ cosf(a) * Rg, 0.6f, sinf(a) * Rg }; if (p.z > ctr.z + 14.0f) continue; DrawModelEx(s_column, p, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 2.8f, 1.2f, 0.6f }, (j % 2) ? stone : tileDk); }

    // iron dome: meridian ribs (quarter-arcs to the apex) + 2 latitude rings + apex hub
    int ribs = 10, segs = 6;
    for (int r = 0; r < ribs; r++) {
        float A = (r / (float)ribs) * 2 * PI;
        Vector3 prev = ctr + Vector3{ cosf(A) * Rg, 0.6f, sinf(A) * Rg };
        for (int t = 1; t <= segs; t++) { float u = (t / (float)segs) * (PI * 0.5f); Vector3 p = ctr + Vector3{ cosf(A) * Rg * cosf(u), 0.6f + Hh * sinf(u), sinf(A) * Rg * cosf(u) }; draw_bone_seg(prev, p, 0.16f, iron); prev = p; }
    }
    for (int lr = 0; lr < 2; lr++) { float u = ((lr + 1) / 3.0f) * (PI * 0.5f), ry = 0.6f + Hh * sinf(u), rr = Rg * cosf(u); Vector3 pv = ctr + Vector3{ rr, ry, 0 }; for (int s = 1; s <= 24; s++) { float b = (s / 24.0f) * 2 * PI; Vector3 p = ctr + Vector3{ cosf(b) * rr, ry, sinf(b) * rr }; draw_bone_seg(pv, p, 0.07f, iron); pv = p; } }
    DrawModelEx(s_dome, ctr + Vector3{ 0, 0.6f + Hh, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 1.1f, 1.6f }, iron);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.6f + Hh + 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.15f, 1.2f, 0.15f }, iron);   // finial

    // four quadrant flower beds (low soil boxes brimming with colored blooms)
    Vector3 beds[4] = { ctr + Vector3{ 9.0f, 0, 9.0f }, ctr + Vector3{ -9.0f, 0, 9.0f }, ctr + Vector3{ 9.0f, 0, -9.0f }, ctr + Vector3{ -9.0f, 0, -9.0f } };
    for (int b = 0; b < 4; b++) {
        DrawModelEx(s_column, beds[b] + Vector3{ 0, 0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.0f, 0.5f, 5.0f }, stone);
        DrawModelEx(s_column, beds[b] + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.4f, 0.3f, 4.4f }, soil);
        SetRandomSeed(700 + b * 13);
        for (int k = 0; k < 14; k++) { Vector3 p = beds[b] + Vector3{ rand_range(-2.0f,2.0f), 0.6f, rand_range(-2.0f,2.0f) }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.3f,0.6f), rand_range(0.3f,0.6f), rand_range(0.3f,0.6f) }, bloom[k % 4]); }
    }

    // the palms + a giant central palm
    std::vector<Vector4> palms;
    glass_layout(palms);
    for (auto& t : palms) draw_palm(Vector3{ t.x, 0, t.z }, t.w, trunk, frond, fruit);
    draw_palm(ctr, 9.5f, trunk, frond, fruit);

    // hanging baskets from the lower latitude ring
    for (int i = 0; i < 6; i++) { float a = (i / 6.0f) * 2 * PI; Vector3 hp = ctr + Vector3{ cosf(a) * (Rg * 0.7f), 7.5f, sinf(a) * (Rg * 0.7f) }; draw_bone_seg(hp, hp + Vector3{ 0, -1.4f, 0 }, 0.04f, iron); DrawModelEx(s_dome, hp + Vector3{ 0, -1.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.5f, 0.5f }, bloom[i % 4]); }

    // additive: glazed wall/roof panes shimmer + warm sun-through-glass glow + pollen motes
    BeginBlendMode(BLEND_ADDITIVE);
    for (int r = 0; r < ribs; r++) {
        float A = ((r + 0.5f) / (float)ribs) * 2 * PI;
        for (int t = 0; t < segs; t++) { float u = ((t + 0.5f) / segs) * (PI * 0.5f); Vector3 p = ctr + Vector3{ cosf(A) * Rg * cosf(u), 0.6f + Hh * sinf(u), sinf(A) * Rg * cosf(u) }; if (p.z > ctr.z + 14.0f && p.y < 4.0f) continue; DrawSphereEx(p, 1.4f, 6, 6, Color{ glass.r, glass.g, glass.b, 14 }); }   // pane shimmer
    }
    for (size_t i = 0; i < s_wisps.size(); i++) {
        Vector3 w = s_wisps[i];
        DrawSphereEx(w, 0.5f + 0.12f * sinf(s_time * 1.6f + i), 8, 8, Color{ 255, 240, 180, 45 });
        for (int k = 0; k < 4; k++) { float ph = s_time * 0.3f + k * 0.25f + i, tt = fmodf(ph, 1.0f); DrawSphereEx(w + Vector3{ 1.2f * sinf(ph * 2 + i), 0.4f + tt * 2.0f, 1.2f * cosf(ph * 2 + i) }, 0.05f * (1.0f - tt), 5, 5, Color{ 255, 248, 200, (unsigned char)(70 * (1.0f - tt)) }); }
    }
    EndBlendMode();
}

// Seeded box-hive layout for the apiary, shared by build_apiary (obstacles) and
// draw_apiary (geometry). xyz = base, w = yaw degrees.
static void apiary_layout(std::vector<Vector4>& hives) {
    Vector3 c = boundary_center;
    hives.clear();
    SetRandomSeed(71023);
    int tries = 0;
    while ((int)hives.size() < 9 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(6.0f, boundary_radius - 3.0f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 5.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& h : hives) if (Vector3Distance(p, Vector3{ h.x,0,h.z }) < 3.5f) { clash = true; break; }
        if (clash) continue;
        hives.push_back({ p.x, p.y, p.z, rand_range(0, 360.0f) });
    }
}

// A stacked wooden box-hive (stand + supers + telescoping lid + an entrance slot),
// drawn in a yawed rlgl frame.
static void draw_hive(Vector3 base, int boxes, float yawDeg, Color wood, Color lid, Color dark) {
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(yawDeg, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 0.35f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.7f, 0.7f, 1.3f }, dark);   // stand
    float y = 0.7f;
    for (int b = 0; b < boxes; b++) { DrawModelEx(s_column, Vector3{ 0, y + 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 1.0f, 1.0f }, (b % 2) ? wood : lid); y += 1.0f; }
    DrawModelEx(s_column, Vector3{ 0, y + 0.15f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 0.3f, 1.2f }, dark);   // telescoping lid
    DrawModelEx(s_column, Vector3{ 0, 0.95f, 0.51f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 0.12f, 0.05f }, dark); // entrance slot
    rlPopMatrix();
}

// A traditional conical straw skep: stacked shrinking coiled discs capped by a knob.
static void draw_skep(Vector3 base, Color straw, Color strawDk, Color dark) {
    float r = 1.0f, y = 0.0f;
    for (int i = 0; i < 5; i++) { DrawModelEx(s_cyl, base + Vector3{ 0, y, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r, 0.4f, r }, (i % 2) ? straw : strawDk); y += 0.36f; r *= 0.82f; }
    DrawModelEx(s_dome, base + Vector3{ 0, y, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.45f, 0.45f, 0.45f }, straw);
    DrawModelEx(s_column, base + Vector3{ 0, 0.2f, 0.9f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.2f, 0.1f }, dark);   // entrance
}

static void build_apiary() {
    s_wisps.clear();
    std::vector<Vector4> hives;
    apiary_layout(hives);
    for (auto& h : hives) obstacles.push_back({ Vector3{ h.x, 0, h.z }, 1.1f });
    Vector3 ctr = boundary_center;
    Vector3 skeps[3] = { ctr + Vector3{ -7.0f, 0, 5.0f }, ctr + Vector3{ 7.0f, 0, 6.0f }, ctr + Vector3{ 5.0f, 0, -8.0f } };
    for (auto& s : skeps) obstacles.push_back({ s, 1.1f });
    // honey-glow lights at the bee-tree, a few hives and skeps
    s_wisps.push_back(ctr + Vector3{ 0, 2.5f, 0 });
    for (size_t i = 0; i < hives.size(); i += 2) s_wisps.push_back(Vector3{ hives[i].x, 2.0f, hives[i].z });
    for (auto& s : skeps) s_wisps.push_back(s + Vector3{ 0, 1.4f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.4f, w.z, 4.5f }); }
}

static void draw_apiary() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color grass{ 98,130,64,255 }, grassDk{ 82,112,54,255 }, wood{ 198,170,116,255 }, lid{ 168,138,90,255 }, dark{ 74,58,42,255 };
    Color straw{ 206,176,104,255 }, strawDk{ 176,146,84,255 }, trunk{ 112,86,58,255 }, comb{ 238,198,92,255 };
    Color bloom[4] = { {224,196,86,255}, {220,120,150,255}, {150,120,210,255}, {236,150,90,255} };

    // meadow grass + wildflower patches
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, grass);
    SetRandomSeed(1450);
    for (int i = 0; i < 16; i++) { float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.4f, 2.8f), 0.12f, rand_range(1.4f, 2.8f) }, grassDk); for (int k = 0; k < 5; k++) DrawModelEx(s_dome, p + Vector3{ rand_range(-1.2f,1.2f), 0.2f, rand_range(-1.2f,1.2f) }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.3f, 0.2f }, bloom[(i + k) % 4]); }

    // the box hives
    std::vector<Vector4> hives;
    apiary_layout(hives);
    for (size_t i = 0; i < hives.size(); i++) draw_hive(Vector3{ hives[i].x, 0, hives[i].z }, 2 + (int)(i % 3), hives[i].w, wood, lid, dark);

    // straw skeps
    Vector3 skeps[3] = { ctr + Vector3{ -7.0f, 0, 5.0f }, ctr + Vector3{ 7.0f, 0, 6.0f }, ctr + Vector3{ 5.0f, 0, -8.0f } };
    for (auto& s : skeps) draw_skep(s, straw, strawDk, dark);

    // the great hollow bee-tree (reuse the petrified-tree trunk, in brown) with comb + a dark hollow
    draw_ptree(ctr, 9.0f, 0.95f, 1234, trunk);
    DrawModelEx(s_column, ctr + Vector3{ 0, 3.0f, 0.85f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 1.6f, 0.3f }, dark);   // hollow
    SetRandomSeed(99);
    for (int i = 0; i < 7; i++) { float a = rand_range(0, 2 * PI), y = rand_range(2.0f, 6.0f); Vector3 p = ctr + Vector3{ sinf(a) * 1.1f, y, cosf(a) * 1.1f }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.5f, 0.4f }, comb); }   // comb lobes

    // a smoker prop by one hive
    Vector3 sm = ctr + Vector3{ -9.0f, 0, -3.0f };
    DrawModelEx(s_cyl, sm + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.35f, 1.0f, 0.35f }, dark);
    DrawModelEx(s_cone, sm + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.35f, 0.5f, 0.35f }, lid);

    // additive: dense bee swarm + honey glow + smoker smoke
    BeginBlendMode(BLEND_ADDITIVE);
    Color bee{ 250, 224, 90, 255 };
    auto swarm = [&](Vector3 o, int n, float rad) { for (int k = 0; k < n; k++) { float ph = s_time * 2.0f + k * 1.7f, ph2 = s_time * 1.3f + k * 0.9f; Vector3 p = o + Vector3{ sinf(ph) * rad * (0.4f + 0.6f * sinf(ph2)), 1.4f + 1.0f * sinf(ph2 * 1.3f + k), cosf(ph) * rad * (0.4f + 0.6f * cosf(ph2)) }; DrawSphereEx(p, 0.06f, 4, 4, Color{ bee.r, bee.g, bee.b, 200 }); } };
    swarm(ctr, 24, 2.6f);                                          // dense swarm at the bee-tree
    for (size_t i = 0; i < hives.size(); i++) swarm(Vector3{ hives[i].x, 0, hives[i].z }, 6, 1.4f);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.1f * sinf(s_time * 1.7f + i), 8, 8, Color{ 255, 210, 120, 50 });
    for (int k = 0; k < 5; k++) { float tt = fmodf(s_time * 0.5f + k * 0.2f, 1.0f); DrawSphereEx(sm + Vector3{ 0.2f * sinf(s_time + k), 1.3f + tt * 2.5f, 0 }, 0.3f * (1.0f - tt) + 0.1f, 6, 6, Color{ 220, 220, 210, (unsigned char)(60 * (1.0f - tt)) }); }
    EndBlendMode();
}

// Seeded bottle-kiln layout for the kiln yard, shared by build_kiln (obstacles) and
// draw_kiln (geometry). xyz = base, w = scale.
static void kiln_layout(std::vector<Vector4>& kilns) {
    Vector3 c = boundary_center;
    kilns.clear();
    SetRandomSeed(81133);
    int tries = 0;
    while ((int)kilns.size() < 4 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(9.0f, boundary_radius - 4.0f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 8.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& k : kilns) if (Vector3Distance(p, Vector3{ k.x,0,k.z }) < 8.0f) { clash = true; break; }
        if (clash) continue;
        kilns.push_back({ p.x, p.y, p.z, rand_range(0.85f, 1.1f) });
    }
}

// A bulbous brick bottle-kiln: a bottle-profile body of stacked brick rings narrowing
// to a chimney neck, with iron banding hoops and a dark firing door.
static void draw_bottle_oven(Vector3 base, float sc, Color brick, Color brickDk, Color dark) {
    float prof[7] = { 1.7f, 2.2f, 2.4f, 2.2f, 1.7f, 1.2f, 0.85f };
    float y = 0;
    for (int i = 0; i < 7; i++) { float rr = prof[i] * sc; DrawModelEx(s_cyl, base + Vector3{ 0, y, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rr, 0.68f * sc, rr }, (i % 2) ? brick : brickDk); y += 0.64f * sc; }
    DrawModelEx(s_cyl, base + Vector3{ 0, y, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.75f * sc, 1.6f * sc, 0.75f * sc }, brick);   // chimney neck
    DrawModelEx(s_cyl, base + Vector3{ 0, 0.64f * sc * 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.55f * sc, 0.12f * sc, 2.55f * sc }, dark);   // iron hoops
    DrawModelEx(s_cyl, base + Vector3{ 0, 0.64f * sc * 4.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.85f * sc, 0.12f * sc, 1.85f * sc }, dark);
    DrawModelEx(s_column, base + Vector3{ 0, 1.0f * sc, 2.0f * sc }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f * sc, 1.6f * sc, 0.4f * sc }, dark);   // firing door
}

static void build_kiln() {
    s_wisps.clear();
    std::vector<Vector4> kilns;
    kiln_layout(kilns);
    for (auto& k : kilns) obstacles.push_back({ Vector3{ k.x, 0, k.z }, 2.4f * k.w });
    Vector3 ctr = boundary_center;
    // kiln-fire glow at every kiln door + the central great kiln
    s_wisps.push_back(ctr + Vector3{ 0, 1.4f, 3.0f });
    for (auto& k : kilns) s_wisps.push_back(Vector3{ k.x, 1.2f, k.z + 2.6f * k.w });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 5.0f }); }
}

static void draw_kiln() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color ground{ 142,120,96,255 }, groundDk{ 118,98,78,255 }, brick{ 184,104,72,255 }, brickDk{ 154,86,58,255 }, dark{ 54,40,32,255 };
    Color pot{ 198,126,84,255 }, potDk{ 168,102,66,255 }, clay{ 92,78,60,255 }, wood{ 120,94,62,255 }, fire{ 255,142,46,255 };

    // packed-earth brickyard floor + scattered ash/clay patches
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, ground);
    SetRandomSeed(1500);
    for (int i = 0; i < 12; i++) { float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.2f, 2.4f), 0.12f, rand_range(1.2f, 2.4f) }, groundDk); }

    // the bottle-kilns + the central great kiln
    std::vector<Vector4> kilns;
    kiln_layout(kilns);
    for (auto& k : kilns) draw_bottle_oven(Vector3{ k.x, 0, k.z }, k.w, brick, brickDk, dark);
    draw_bottle_oven(ctr, 1.4f, brick, brickDk, dark);

    // clay pits (dark wet slabs) + clay mounds near the kilns
    SetRandomSeed(220);
    for (int i = 0; i < 5; i++) { float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 3.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0.04f, cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; DrawModelEx(s_cyl, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.5f,2.5f), 0.06f, rand_range(1.5f,2.5f) }, clay); DrawModelEx(s_dome, p + Vector3{ 1.8f, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 0.6f, 0.8f }, clay); }

    // drying racks: posts + 2 shelves laden with rows of pots
    Vector3 racks[2] = { ctr + Vector3{ -10.0f, 0, 6.0f }, ctr + Vector3{ 11.0f, 0, -4.0f } };
    for (int r = 0; r < 2; r++) {
        Vector3 b = racks[r];
        for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) DrawModelEx(s_cyl, b + Vector3{ sx * 1.8f, 1.0f, sz * 0.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 2.0f, 0.12f }, wood);
        for (int sh = 0; sh < 2; sh++) { float yy = 0.9f + sh * 0.9f; DrawModelEx(s_column, b + Vector3{ 0, yy, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.0f, 0.12f, 1.5f }, wood); for (int p = 0; p < 5; p++) DrawModelEx(s_cyl, b + Vector3{ -1.6f + p * 0.8f, yy + 0.35f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.28f, 0.55f, 0.28f }, (p % 2) ? pot : potDk); }
    }

    // stacked fired-pot pyramids
    SetRandomSeed(660);
    for (int s = 0; s < 3; s++) { float a = rand_range(0, 2 * PI), r = rand_range(7.0f, boundary_radius - 3.0f); Vector3 b = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (b.z > ctr.z + 12.0f) continue; for (int row = 0; row < 3; row++) { int cnt = 3 - row; for (int c = 0; c < cnt; c++) DrawModelEx(s_cyl, b + Vector3{ (-(cnt - 1) * 0.5f + c) * 0.7f, 0.4f + row * 0.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.6f, 0.3f }, (c % 2) ? pot : potDk); } }

    // a spinning potter's wheel forming a pot
    Vector3 pw = ctr + Vector3{ 7.0f, 0, 7.0f };
    DrawModelEx(s_cyl, pw + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.0f, 0.6f }, wood);
    rlPushMatrix();
    rlTranslatef(pw.x, pw.y + 1.0f, pw.z);
    rlRotatef(s_time * 180.0f, 0, 1, 0);
    DrawModelEx(s_cyl, Vector3{ 0, 0.05f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.95f, 0.12f, 0.95f }, dark);
    DrawModelEx(s_cyl, Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.32f, 0.6f, 0.32f }, pot);
    rlPopMatrix();

    // additive: kiln-fire door glow (flickering) + chimney smoke + ember motes
    BeginBlendMode(BLEND_ADDITIVE);
    auto kilnfire = [&](Vector3 base, float sc) {
        float fl = 0.7f + 0.3f * sinf(s_time * 8.0f + base.x);
        Vector3 door = base + Vector3{ 0, 1.0f * sc, 2.0f * sc };
        for (int k = 0; k < 4; k++) DrawSphereEx(door + Vector3{ 0.1f * sinf(s_time * 6 + k), 0.2f * k * sc, 0 }, (0.5f - 0.08f * k) * sc * fl, 7, 7, Color{ fire.r, fire.g, fire.b, (unsigned char)(130 - 26 * k) });
        // chimney smoke
        float chy = 0.64f * sc * 7.0f + 1.6f * sc;
        for (int k = 0; k < 5; k++) { float tt = fmodf(s_time * 0.4f + k * 0.2f, 1.0f); DrawSphereEx(base + Vector3{ 0.3f * sinf(s_time + k), chy + tt * 4.0f, 0.3f * cosf(s_time + k) }, (0.5f + tt * 0.5f) * sc, 6, 6, Color{ 120, 116, 108, (unsigned char)(50 * (1.0f - tt)) }); }
    };
    for (auto& k : kilns) kilnfire(Vector3{ k.x, 0, k.z }, k.w);
    kilnfire(ctr, 1.4f);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.1f * sinf(s_time * 3 + i), 8, 8, Color{ fire.r, fire.g, fire.b, 45 });
    EndBlendMode();
}

// Seeded coral layout for the reef, shared by build_reef (obstacles) and draw_reef
// (geometry). xyz = base, w = scale. The coral kind is index % 4.
static void reef_layout(std::vector<Vector4>& corals) {
    Vector3 c = boundary_center;
    corals.clear();
    SetRandomSeed(90427);
    int tries = 0;
    while ((int)corals.size() < 15 && tries < 400) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(5.0f, boundary_radius - 2.0f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 4.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& k : corals) if (Vector3Distance(p, Vector3{ k.x,0,k.z }) < 3.2f) { clash = true; break; }
        if (clash) continue;
        corals.push_back({ p.x, p.y, p.z, rand_range(0.8f, 1.5f) });
    }
}

static void build_reef() {
    s_wisps.clear();
    std::vector<Vector4> corals;
    reef_layout(corals);
    for (auto& k : corals) obstacles.push_back({ Vector3{ k.x, 0, k.z }, 0.8f * k.w + 0.3f });
    Vector3 ctr = boundary_center;
    // bioluminescent glow at the central coral + a few coral heads
    s_wisps.push_back(ctr + Vector3{ 0, 1.5f, 0 });
    for (size_t i = 0; i < corals.size(); i += 2) s_wisps.push_back(Vector3{ corals[i].x, 1.2f, corals[i].z });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 4.5f }); }
}

static void draw_reef() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color sand{ 198,186,150,255 }, sandDk{ 172,160,126,255 };
    Color coA{ 236,128,96,255 }, coB{ 220,110,170,255 }, coC{ 150,110,210,255 }, coD{ 110,200,180,255 }, brain{ 184,196,118,255 };
    Color kelp{ 80,140,90,255 }, anem{ 230,140,160,255 }, jar{ 150,120,92,255 }, biolum{ 130,220,240,255 }, fish{ 190,224,236,255 };

    // sandy seabed slab + ripple mounds (the deep-blue fog reads as "underwater")
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, sand);
    SetRandomSeed(1600);
    for (int i = 0; i < 14; i++) { float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.4f, 2.8f), 0.14f, rand_range(1.4f, 2.8f) }, sandDk); }

    // coral formations (4 kinds by index)
    std::vector<Vector4> corals;
    reef_layout(corals);
    for (size_t i = 0; i < corals.size(); i++) {
        Vector3 b = Vector3{ corals[i].x, 0, corals[i].z }; float sc = corals[i].w; int kind = (int)(i % 4);
        if (kind == 0) {                                          // staghorn (branching) — reuse draw_ptree
            draw_ptree(b, 2.4f * sc, 0.28f * sc, 400 + (int)i * 17, coA);
        } else if (kind == 1) {                                   // brain coral
            DrawModelEx(s_dome, b + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f * sc, 1.1f * sc, 1.4f * sc }, brain);
            DrawModelEx(s_dome, b + Vector3{ 0, 0.9f * sc, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f * sc, 0.5f * sc, 0.8f * sc }, coB);
        } else if (kind == 2) {                                   // fan coral (sways)
            float sw = 8.0f * sinf(s_time * 0.8f + i);
            DrawModelEx(s_cyl, b + Vector3{ 0, 0.4f * sc, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.15f * sc, 0.8f * sc, 0.15f * sc }, coC);   // stalk
            DrawModelEx(s_dome, b + Vector3{ 0, 1.4f * sc, 0 }, Vector3{ 1,0,0 }, 90.0f + sw, Vector3{ 1.8f * sc, 0.12f * sc, 1.4f * sc }, coC);   // fan blade
        } else {                                                  // tube coral cluster
            SetRandomSeed(50 + (int)i);
            for (int t = 0; t < 5; t++) { Vector3 q = b + Vector3{ rand_range(-0.6f,0.6f) * sc, 0, rand_range(-0.6f,0.6f) * sc }; DrawModelEx(s_cyl, q, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f * sc, rand_range(0.8f,1.8f) * sc, 0.22f * sc }, coD); }
        }
    }
    // central great coral head: a big brain coral crowned with staghorn branches
    DrawModelEx(s_dome, ctr + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 2.0f, 2.6f }, brain);
    draw_ptree(ctr + Vector3{ 0, 1.6f, 0 }, 3.0f, 0.35f, 1234, coA);

    // kelp clumps: tall swaying strands
    SetRandomSeed(311);
    for (int i = 0; i < 9; i++) {
        float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 1.0f); Vector3 cc = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        for (int s = 0; s < 4; s++) {
            Vector3 base = cc + Vector3{ rand_range(-0.8f,0.8f), 0, rand_range(-0.8f,0.8f) }; Vector3 prev = base; float h = rand_range(3.0f, 5.5f);
            for (int seg = 1; seg <= 5; seg++) { float t = seg / 5.0f; Vector3 nx = base + Vector3{ sinf(s_time * 0.9f + t * 3.0f + i) * t * 1.2f, h * t, cosf(s_time * 0.7f + t * 2.0f) * t * 0.6f }; draw_bone_seg(prev, nx, 0.09f, kelp); prev = nx; }
        }
    }

    // sea anemones: a base bulb + radiating wavy tentacles
    SetRandomSeed(733);
    for (int i = 0; i < 6; i++) {
        float a = rand_range(0, 2 * PI), r = rand_range(5.0f, boundary_radius - 2.0f); Vector3 b = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r };
        DrawModelEx(s_dome, b + Vector3{ 0, 0.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.5f, 0.7f }, anem);
        for (int t = 0; t < 8; t++) { float ta = (t / 8.0f) * 2 * PI; Vector3 dir{ sinf(ta), 0, cosf(ta) }; Vector3 tip = b + dir * 0.7f + Vector3{ sinf(s_time * 2 + t) * 0.2f, 1.0f + 0.2f * sinf(s_time * 3 + t), 0 }; draw_bone_seg(b + Vector3{ 0, 0.4f, 0 }, tip, 0.05f, anem); }
    }

    // a couple of sunken amphorae + a toppled column (a hint of ruin)
    DrawModelEx(s_cyl, ctr + Vector3{ 6.0f, 0.4f, 8.0f }, Vector3{ 1,0,0 }, 70.0f, Vector3{ 0.5f, 1.2f, 0.5f }, jar);
    DrawModelEx(s_cyl, ctr + Vector3{ -7.0f, 0.5f, 6.0f }, Vector3{ 0,0,1 }, 80.0f, Vector3{ 0.7f, 3.0f, 0.7f }, sandDk);

    // additive: drifting light shafts + rising bubbles + a darting fish school + coral biolum
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 6; i++) { float x = -18.0f + i * 7.0f + 2.0f * sinf(s_time * 0.3f + i); DrawCube(ctr + Vector3{ x, 9.0f, -4.0f + i * 2.0f }, 2.2f, 18.0f, 2.2f, Color{ 120, 200, 230, 12 }); }   // light shafts
    SetRandomSeed(404);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 2.0f); Vector3 v = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; for (int k = 0; k < 5; k++) { float tt = fmodf(s_time * 0.5f + k * 0.2f + i, 1.0f); DrawSphereEx(v + Vector3{ 0.3f * sinf(s_time * 2 + k), 0.2f + tt * 7.0f, 0.3f * cosf(s_time * 2 + k) }, 0.1f + 0.05f * tt, 6, 6, Color{ 200, 230, 240, (unsigned char)(70 * (1.0f - tt)) }); } }   // bubbles
    Color fc = fish; for (int k = 0; k < 20; k++) { float ph = s_time * 1.6f + k * 0.5f; Vector3 p = ctr + Vector3{ sinf(ph) * 9.0f, 3.0f + 1.5f * sinf(ph * 0.7f + k), cosf(ph) * 9.0f }; DrawSphereEx(p, 0.12f, 4, 4, Color{ fc.r, fc.g, fc.b, 180 }); }   // fish school
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.12f * sinf(s_time * 1.8f + i), 8, 8, Color{ biolum.r, biolum.g, biolum.b, 50 });
    EndBlendMode();
}

// A blast-furnace cupola: a stack of slightly tapering brick/iron rings + a charging
// chimney + iron hoops + a dark tap-hole at the base.
static void draw_furnace(Vector3 base, float sc, Color brick, Color iron, Color ironDk) {
    float prof[6] = { 2.0f, 1.95f, 1.85f, 1.7f, 1.5f, 1.3f };
    float y = 0;
    for (int i = 0; i < 6; i++) { DrawModelEx(s_cyl, base + Vector3{ 0, y, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ prof[i] * sc, 0.9f * sc, prof[i] * sc }, (i % 2) ? brick : iron); y += 0.86f * sc; }
    DrawModelEx(s_cyl, base + Vector3{ 0, y, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.95f * sc, 2.0f * sc, 0.95f * sc }, ironDk);   // charging chimney
    DrawModelEx(s_cyl, base + Vector3{ 0, 0.86f * sc * 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.1f * sc, 0.14f * sc, 2.1f * sc }, ironDk);   // iron hoop
    DrawModelEx(s_column, base + Vector3{ 0, 0.7f * sc, 1.9f * sc }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f * sc, 1.0f * sc, 0.4f * sc }, ironDk);   // tap-hole
    for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_cyl, base + Vector3{ sx * 1.7f * sc, 1.0f * sc, 0 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.18f * sc, 1.2f * sc, 0.18f * sc }, ironDk);   // tuyere pipes
}

// A water-/cam-driven trip-hammer: a pivoting timber helve with a heavy iron head
// that rises slowly then SLAMS the anvil, on a cycle driven by s_time.
static void draw_triphammer(Vector3 base, float phase, Color wood, Color iron, Color ironDk) {
    for (int sx = 0; sx <= 1; sx++) for (int sz = -1; sz <= 1; sz += 2)
        draw_bone_seg(base + Vector3{ -0.4f + sx * 1.6f, 0, sz * 1.0f }, base + Vector3{ 0, 2.6f, 0 }, 0.18f, wood);   // A-frame supports
    Vector3 anv = base + Vector3{ -3.0f, 0, 0 };
    DrawModelEx(s_column, anv + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 1.0f, 0.9f }, ironDk);   // anvil base
    DrawModelEx(s_column, anv + Vector3{ 0, 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.9f, 0.3f, 0.8f }, iron);     // anvil face
    float t = fmodf(s_time * 1.2f + phase, 1.0f);
    float a = (t < 0.75f) ? (t / 0.75f) : (1.0f - (t - 0.75f) / 0.25f);   // rise then quick slam
    float ang = (1.0f - a) * 22.0f;
    rlPushMatrix();
    rlTranslatef(base.x, base.y + 2.6f, base.z);
    rlRotatef(ang, 0, 0, 1);
    DrawModelEx(s_column, Vector3{ -0.8f, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.8f, 0.3f, 0.4f }, wood);          // helve
    DrawModelEx(s_column, Vector3{ -3.2f, -0.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 0.9f, 0.7f }, iron);     // hammer head
    rlPopMatrix();
}

static void build_foundry() {
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    Vector3 hammer = ctr + Vector3{ 9.0f, 0, 4.0f };
    Vector3 anvil2 = ctr + Vector3{ -9.0f, 0, -5.0f };
    Vector3 molds[2] = { ctr + Vector3{ -6.0f, 0, 8.0f }, ctr + Vector3{ 8.0f, 0, -9.0f } };
    obstacles.push_back({ hammer, 2.6f });
    obstacles.push_back({ anvil2, 1.4f });
    for (auto& m : molds) obstacles.push_back({ m, 1.8f });
    // molten/furnace glow at the furnace tap, hammer anvil, molds
    s_wisps.push_back(ctr + Vector3{ 0, 1.2f, 3.2f });
    s_wisps.push_back(hammer + Vector3{ -3.0f, 1.4f, 0 });
    for (auto& m : molds) s_wisps.push_back(m + Vector3{ 0, 0.6f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 0.8f, w.z, 5.0f }); }
}

static void draw_foundry() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color floor{ 86,82,84,255 }, floorDk{ 64,62,66,255 }, brick{ 150,90,70,255 }, iron{ 80,80,86,255 }, ironDk{ 50,50,56,255 };
    Color wood{ 110,84,56,255 }, ingot{ 122,126,134,255 }, sand{ 150,134,104,255 }, molten{ 255,140,40,255 };

    // iron-plate floor + plate seams + scattered slag mounds
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, floor);
    for (int i = -6; i <= 6; i++) { DrawModelEx(s_column, ctr + Vector3{ i * 3.6f, -0.04f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 1.0f, boundary_radius * 1.9f }, floorDk); DrawModelEx(s_column, ctr + Vector3{ 0, -0.04f, i * 3.6f }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 1.9f, 1.0f, 0.16f }, floorDk); }
    SetRandomSeed(1700);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), r = rand_range(5.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.8f,1.6f), rand_range(0.3f,0.6f), rand_range(0.8f,1.6f) }, ironDk); }

    // central blast furnace
    draw_furnace(ctr, 1.4f, brick, iron, ironDk);

    // casting molds (sand frames with a molten pool) + ingot stacks
    Vector3 molds[2] = { ctr + Vector3{ -6.0f, 0, 8.0f }, ctr + Vector3{ 8.0f, 0, -9.0f } };
    for (auto& m : molds) { DrawModelEx(s_column, m + Vector3{ 0, 0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 0.5f, 2.2f }, sand); DrawModelEx(s_column, m + Vector3{ 0, 0.45f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 0.25f, 1.6f }, ironDk); }
    SetRandomSeed(551);
    for (int s = 0; s < 4; s++) { float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 3.0f); Vector3 b = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (b.z > ctr.z + 12.0f) continue; for (int row = 0; row < 3; row++) { int cnt = 3 - row; for (int c = 0; c < cnt; c++) DrawModelEx(s_column, b + Vector3{ (-(cnt - 1) * 0.5f + c) * 0.7f, 0.2f + row * 0.32f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.28f, 1.4f }, ingot); } }

    // the trip-hammer + a second standalone anvil + a quench trough
    draw_triphammer(ctr + Vector3{ 9.0f, 0, 4.0f }, 0.0f, wood, iron, ironDk);
    Vector3 anvil2 = ctr + Vector3{ -9.0f, 0, -5.0f };
    DrawModelEx(s_column, anvil2 + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.3f, 1.0f, 0.8f }, ironDk);
    DrawModelEx(s_column, anvil2 + Vector3{ 0, 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.8f, 0.3f, 0.7f }, iron);
    DrawModelEx(s_column, ctr + Vector3{ -3.0f, 0.4f, -10.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 0.8f, 1.2f }, ironDk);   // quench trough

    // additive: furnace tap-pour + molten runners/molds + chimney smoke + hammer sparks
    BeginBlendMode(BLEND_ADDITIVE);
    // furnace tap glow + molten runner toward the nearer mold
    Vector3 tap = ctr + Vector3{ 0, 1.0f, 3.4f };
    for (int k = 0; k < 4; k++) DrawSphereEx(tap + Vector3{ 0.1f * sinf(s_time * 6 + k), -0.1f * k, 0.3f * k }, 0.6f - 0.1f * k, 7, 7, Color{ molten.r, molten.g, molten.b, (unsigned char)(140 - 28 * k) });
    DrawCube(ctr + Vector3{ -3.0f, 0.06f, 5.6f }, 0.7f, 0.1f, 5.5f, Color{ molten.r, molten.g, molten.b, 70 });   // a glowing runner channel
    for (auto& m : molds) DrawCube(m + Vector3{ 0, 0.55f, 0 }, 2.2f, 0.1f, 1.4f, Color{ molten.r, molten.g, molten.b, (unsigned char)(90 + 30 * sinf(s_time * 2 + m.x)) });
    // chimney smoke
    for (int k = 0; k < 5; k++) { float tt = fmodf(s_time * 0.35f + k * 0.2f, 1.0f); DrawSphereEx(ctr + Vector3{ 0.3f * sinf(s_time + k), 6.5f + tt * 4.5f, 0.3f * cosf(s_time + k) }, 0.7f + tt * 0.7f, 6, 6, Color{ 90, 86, 84, (unsigned char)(55 * (1.0f - tt)) }); }
    // hammer sparks at the anvil on impact
    float t = fmodf(s_time * 1.2f, 1.0f), a = (t < 0.75f) ? (t / 0.75f) : (1.0f - (t - 0.75f) / 0.25f);
    if (a < 0.14f) { Vector3 anv = ctr + Vector3{ 9.0f - 3.0f, 1.4f, 4.0f }; for (int k = 0; k < 12; k++) { Vector3 d{ cosf(k * 1.7f), fabsf(sinf(k * 2.3f)), sinf(k * 1.1f) }; DrawSphereEx(anv + d * (0.4f + 0.5f * (1.0f - a / 0.14f)), 0.08f, 4, 4, Color{ 255, 215, 140, 220 }); } }
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.1f * sinf(s_time * 3 + i), 8, 8, Color{ molten.r, molten.g, molten.b, 45 });
    EndBlendMode();
}

// Fixed pier grid for the cathedral nave, shared by build_nave (obstacles) and
// draw_nave (geometry): two rows of 5 clustered piers flanking the central aisle.
static void nave_piers(std::vector<Vector3>& out) {
    Vector3 c = boundary_center;
    out.clear();
    float zz[5] = { -15.0f, -9.0f, -3.0f, 3.0f, 9.0f };
    for (int i = 0; i < 5; i++) { out.push_back(c + Vector3{ -6.0f, 0, zz[i] }); out.push_back(c + Vector3{ 6.0f, 0, zz[i] }); }
}

// A clustered Gothic pier: a square base, a fat core shaft ringed by 4 slender
// shafts, and a capital block.
static void draw_pier(Vector3 b, float h, Color stone, Color stoneDk) {
    DrawModelEx(s_column, b + Vector3{ 0, 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 0.6f, 1.5f }, stoneDk);   // base
    DrawModelEx(s_cyl, b + Vector3{ 0, 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, h, 0.6f }, stone);          // core
    for (int i = 0; i < 4; i++) { float a = i * 1.5708f; DrawModelEx(s_cyl, b + Vector3{ cosf(a) * 0.5f, 0.3f, sinf(a) * 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.24f, h, 0.24f }, stone); }   // shafts
    DrawModelEx(s_column, b + Vector3{ 0, h + 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 0.5f, 1.4f }, stoneDk);   // capital
}

// A pointed (Gothic) arch: two arcs springing from L and R that meet at a peak `rise`
// above them, each drawn as a chain of lit cylinder segments.
static void draw_gothic_arch(Vector3 L, Vector3 R, float rise, float rad, Color col) {
    float topY = (L.y > R.y ? L.y : R.y);
    Vector3 peak{ (L.x + R.x) * 0.5f, topY + rise, (L.z + R.z) * 0.5f };
    int seg = 5;
    Vector3 pv = L; for (int i = 1; i <= seg; i++) { float t = i / (float)seg; Vector3 p{ L.x + (peak.x - L.x) * t, L.y + rise * powf(t, 0.7f), L.z + (peak.z - L.z) * t }; draw_bone_seg(pv, p, rad, col); pv = p; }
    pv = R; for (int i = 1; i <= seg; i++) { float t = i / (float)seg; Vector3 p{ R.x + (peak.x - R.x) * t, R.y + rise * powf(t, 0.7f), R.z + (peak.z - R.z) * t }; draw_bone_seg(pv, p, rad, col); pv = p; }
}

static void build_nave() {
    s_wisps.clear();
    std::vector<Vector3> piers;
    nave_piers(piers);
    for (auto& p : piers) obstacles.push_back({ p, 1.2f });
    Vector3 ctr = boundary_center;
    obstacles.push_back({ ctr + Vector3{ 0, 0, -19.0f }, 2.2f });   // altar
    // candle / stained-glass glow at the altar, font, rose window, chandeliers
    s_wisps.push_back(ctr + Vector3{ 0, 1.4f, 0 });                 // font
    s_wisps.push_back(ctr + Vector3{ 0, 1.6f, -19.0f });            // altar
    s_wisps.push_back(ctr + Vector3{ 0, 9.0f, -21.0f });            // rose window
    for (int i = 0; i < 3; i++) s_wisps.push_back(ctr + Vector3{ 0, 6.0f, -9.0f + i * 6.0f });   // chandeliers
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y * 0.4f + 1.0f, w.z, 5.0f }); }
}

static void draw_nave() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color stone{ 152,148,138,255 }, stoneDk{ 120,116,108,255 }, carpet{ 132,42,42,255 }, gold{ 202,170,98,255 }, wood{ 92,70,50,255 };
    Color glass[4] = { {220,80,80,255}, {80,120,220,255}, {90,200,120,255}, {232,202,92,255} };

    // flagstone floor + central carpet aisle
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.0f, 1.0f, boundary_radius * 2.0f }, stone);
    for (int i = -7; i <= 7; i++) DrawModelEx(s_column, ctr + Vector3{ i * 3.0f, -0.04f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 1.0f, boundary_radius * 1.9f }, stoneDk);   // flag seams
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.02f, -3.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 1.0f, 34.0f }, carpet);   // aisle runner

    // side walls with tall lancet window bays
    for (int sx = -1; sx <= 1; sx += 2) {
        DrawModelEx(s_column, ctr + Vector3{ sx * 10.0f, 5.0f, -3.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 10.0f, 34.0f }, stoneDk);
        for (int b = 0; b < 5; b++) DrawModelEx(s_column, ctr + Vector3{ sx * 9.6f, 5.5f, -15.0f + b * 6.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 5.0f, 1.6f }, glass[b % 4]);   // lancet glass
    }

    // the pier arcades + pointed arches (transverse across the nave + longitudinal)
    std::vector<Vector3> piers;
    nave_piers(piers);
    float H = 7.3f;
    for (auto& p : piers) draw_pier(p, 7.0f, stone, stoneDk);
    for (int i = 0; i < 5; i++) {
        Vector3 L = piers[i * 2] + Vector3{ 0, H, 0 }, R = piers[i * 2 + 1] + Vector3{ 0, H, 0 };
        draw_gothic_arch(L, R, 4.0f, 0.26f, stone);                 // transverse arch (across the aisle)
        if (i < 4) { draw_gothic_arch(L, piers[(i + 1) * 2] + Vector3{ 0, H, 0 }, 2.0f, 0.2f, stone); draw_gothic_arch(R, piers[(i + 1) * 2 + 1] + Vector3{ 0, H, 0 }, 2.0f, 0.2f, stone); }   // longitudinal
    }

    // the great rose window at the far (-z) end + back wall
    DrawModelEx(s_column, ctr + Vector3{ 0, 7.0f, -21.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 20.0f, 14.0f, 0.6f }, stoneDk);   // chancel back wall
    Vector3 rose = ctr + Vector3{ 0, 9.0f, -21.1f };
    DrawModelEx(s_torus, rose, Vector3{ 1,0,0 }, 90.0f, Vector3{ 4.6f, 4.6f, 4.6f }, stone);   // outer tracery ring
    for (int i = 0; i < 12; i++) { float a = i * (PI / 6.0f); draw_bone_seg(rose, rose + Vector3{ cosf(a) * 4.4f, sinf(a) * 4.4f, 0 }, 0.08f, stone); }   // radial tracery

    // altar on steps + a baptismal font at centre + pews along the aisle
    Vector3 altar = ctr + Vector3{ 0, 0, -19.0f };
    for (int s = 0; s < 3; s++) DrawModelEx(s_column, altar + Vector3{ 0, 0.2f + s * 0.25f, (float)s * 0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 5.0f - s * 1.0f, 0.3f, 3.0f - s * 0.6f }, stoneDk);
    DrawModelEx(s_column, altar + Vector3{ 0, 1.2f, -0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 1.2f, 1.4f }, gold);   // altar table
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 1.2f, 0.8f }, stone);          // font pedestal
    DrawModelEx(s_dome, ctr + Vector3{ 0, 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f, 0.6f, 1.1f }, stoneDk);       // font basin
    for (int sx = -1; sx <= 1; sx += 2) for (int r = 0; r < 5; r++) DrawModelEx(s_column, ctr + Vector3{ sx * 2.4f, 0.5f, -13.0f + r * 4.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 0.9f, 1.0f }, wood);   // pews

    // hanging chandeliers (rings) swaying gently
    for (int i = 0; i < 3; i++) { float sw = 0.2f * sinf(s_time * 0.6f + i); Vector3 ch = ctr + Vector3{ sw, 6.0f, -9.0f + i * 6.0f }; draw_bone_seg(ctr + Vector3{ 0, 9.5f, -9.0f + i * 6.0f }, ch, 0.04f, stoneDk); DrawModelEx(s_torus, ch, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 1.6f, 1.6f }, gold); }

    // additive: rose-window stained glass + lancet glow + candle flames + dust shafts
    BeginBlendMode(BLEND_ADDITIVE);
    for (int ring = 1; ring <= 2; ring++) { float rr = ring * 1.7f; int n = ring * 8; for (int i = 0; i < n; i++) { float a = (i / (float)n) * 2 * PI; DrawSphereEx(rose + Vector3{ cosf(a) * rr, sinf(a) * rr, 0.2f }, 0.6f, 6, 6, Color{ glass[i % 4].r, glass[i % 4].g, glass[i % 4].b, 60 }); } }
    DrawSphereEx(rose + Vector3{ 0, 0, 0.2f }, 0.9f, 7, 7, Color{ 240, 230, 200, 70 });
    for (int sx = -1; sx <= 1; sx += 2) for (int b = 0; b < 5; b++) DrawSphereEx(ctr + Vector3{ sx * 9.0f, 5.5f, -15.0f + b * 6.0f }, 1.2f, 6, 6, Color{ glass[b % 4].r, glass[b % 4].g, glass[b % 4].b, 26 });   // lancet glow
    for (size_t i = 0; i < s_wisps.size(); i++) { Vector3 w = s_wisps[i]; for (int k = 0; k < 3; k++) DrawSphereEx(w + Vector3{ 0.05f * sinf(s_time * 7 + k + i), 0.12f * k, 0.05f * cosf(s_time * 6 + k) }, 0.22f - 0.05f * k, 6, 6, Color{ 255, 200, 120, (unsigned char)(110 - 26 * k) }); }   // candle flames
    EndBlendMode();
}

// A great paddled water-wheel: two lit `s_torus` rims joined by spokes and a ring of
// paddle boards, spun about its (horizontal) axle by s_time. Its lower paddles dip
// into the millrace.
static void draw_waterwheel(Vector3 c, float R, float phase, Color wood, Color woodDk) {
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(s_time * 45.0f + phase, 0, 0, 1);
    for (int side = -1; side <= 1; side += 2) DrawModelEx(s_torus, Vector3{ 0, 0, side * 0.8f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ R, R, R }, wood);   // rims
    DrawModelEx(s_cyl, Vector3{ 0, 0, -0.9f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.4f, 1.8f, 0.4f }, woodDk);   // hub/axle
    for (int i = 0; i < 8; i++) { float a = i * 0.7854f; draw_bone_seg(Vector3{ 0,0,0 }, Vector3{ cosf(a) * R, sinf(a) * R, 0 }, 0.1f, woodDk); }   // spokes
    for (int i = 0; i < 12; i++) { float a = i * 0.5236f; Vector3 p{ cosf(a) * R, sinf(a) * R, 0 }; DrawModelEx(s_column, p, Vector3{ 0,0,1 }, a * RAD2DEG, Vector3{ 0.25f, 1.0f, 1.7f }, wood); }   // paddles
    rlPopMatrix();
}

static void build_watermill() {
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    Vector3 mill = ctr + Vector3{ -11.0f, 0, -6.0f };
    obstacles.push_back({ mill, 3.6f });                          // mill house
    obstacles.push_back({ ctr + Vector3{ -6.0f, 0, -6.0f }, 1.2f });   // water-wheel hub base
    obstacles.push_back({ ctr + Vector3{ 4.0f, 0, -6.0f }, 0.6f });    // footbridge pier
    // warm lantern glow at the mill, wheel, footbridge, central millstones
    s_wisps.push_back(ctr + Vector3{ 0, 1.4f, 0 });
    s_wisps.push_back(mill + Vector3{ 0, 2.4f, 3.2f });
    s_wisps.push_back(ctr + Vector3{ -6.0f, 3.0f, -4.0f });
    s_wisps.push_back(ctr + Vector3{ 4.0f, 1.8f, -6.0f });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 4.5f }); }
}

static void draw_watermill() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color yard{ 120,112,88,255 }, grass{ 96,128,64,255 }, stone{ 150,146,136,255 }, wood{ 120,92,60,255 }, woodDk{ 88,66,46,255 };
    Color roof{ 132,86,62,255 }, water{ 64,118,114,255 }, sack{ 184,162,112,255 }, millstone{ 140,136,128,255 };

    // mill-yard ground + grassy banks + the millrace channel (a recessed teal water strip)
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, yard);
    SetRandomSeed(1800);
    for (int i = 0; i < 12; i++) { float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (fabsf(p.z - (ctr.z - 6.0f)) < 2.2f) continue; DrawModelEx(s_dome, p + Vector3{ 0, -0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.4f,2.8f), 0.12f, rand_range(1.4f,2.8f) }, grass); }
    DrawModelEx(s_column, ctr + Vector3{ 2.0f, -0.06f, -6.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 30.0f, 1.0f, 3.6f }, water);   // millrace water
    for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_column, ctr + Vector3{ 2.0f, 0.05f, -6.0f + sx * 2.1f }, Vector3{ 0,1,0 }, 0, Vector3{ 30.0f, 0.4f, 0.5f }, stone);   // race kerbs

    // the mill house (reuse draw_cottage, scaled up) + a tail-race wheel housing wall
    Vector3 mill = ctr + Vector3{ -11.0f, 0, -6.0f };
    draw_cottage(mill, 6.0f, 5.0f, 4.4f, 2.6f, 90.0f, stone, roof, woodDk);

    // the great water-wheel (axle at y=3, R=3 so the lower paddles dip into the race)
    draw_waterwheel(ctr + Vector3{ -6.5f, 3.0f, -6.0f }, 3.0f, 0.0f, wood, woodDk);
    // wheel mounting posts
    for (int sz = -1; sz <= 1; sz += 2) draw_bone_seg(ctr + Vector3{ -6.5f, 0, -6.0f + sz * 1.1f }, ctr + Vector3{ -6.5f, 3.0f, -6.0f + sz * 1.1f }, 0.18f, woodDk);

    // a weir / sluice gate upstream + a plank footbridge across the race
    DrawModelEx(s_column, ctr + Vector3{ 14.0f, 0.5f, -6.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 1.6f, 3.6f }, stone);   // weir
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) DrawModelEx(s_cyl, ctr + Vector3{ 4.0f + sx * 1.4f, 0.5f, -6.0f + sz * 1.8f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 1.2f, 0.18f }, woodDk);   // bridge piers
    DrawModelEx(s_column, ctr + Vector3{ 4.0f, 1.4f, -6.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 0.25f, 4.4f }, wood);   // bridge deck
    for (int sz = -1; sz <= 1; sz += 2) DrawModelEx(s_column, ctr + Vector3{ 4.0f, 1.9f, -6.0f + sz * 2.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 0.6f, 0.12f }, woodDk);   // handrails

    // central millstone display (stacked round stones on a stand)
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 0.4f, 1.4f }, millstone);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 0.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 0.3f, 1.2f }, stone);
    DrawModelEx(s_cyl, ctr + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.5f, 0.2f }, woodDk);   // spindle

    // grain sacks, barrels and reed clumps at the banks
    SetRandomSeed(551);
    for (int i = 0; i < 7; i++) { float a = rand_range(0, 2 * PI), r = rand_range(7.0f, boundary_radius - 3.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 12.0f || fabsf(p.z - (ctr.z - 6.0f)) < 2.5f) continue; DrawModelEx(s_dome, p + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.8f, 0.6f }, sack); if (i % 2) DrawModelEx(s_cyl, p + Vector3{ 1.2f, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.0f, 0.5f }, wood); }
    SetRandomSeed(990);
    for (int i = 0; i < 10; i++) { float x = ctr.x - 14.0f + i * 3.0f; Vector3 c = Vector3{ x, 0, ctr.z - 6.0f + (i % 2 ? 2.2f : -2.2f) }; for (int k = 0; k < 4; k++) { Vector3 p = c + Vector3{ rand_range(-0.4f,0.4f), 0, rand_range(-0.4f,0.4f) }; draw_bone_seg(p, p + Vector3{ rand_range(-0.2f,0.2f), rand_range(1.0f,1.8f), rand_range(-0.2f,0.2f) }, 0.05f, grass); } }

    // additive: flowing race shimmer + falling water at the weir + wheel splash + lantern glow
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 16; i++) { float x = ctr.x - 14.0f + fmodf(s_time * 4.0f + i * 2.0f, 30.0f); DrawSphereEx(Vector3{ x, 0.15f, ctr.z - 6.0f + 0.8f * sinf(s_time * 2 + i) }, 0.25f, 6, 6, Color{ 200, 235, 232, 40 }); }   // race flow
    for (int k = 0; k < 6; k++) { float tt = fmodf(s_time * 1.2f + k * 0.16f, 1.0f); DrawSphereEx(ctr + Vector3{ 13.0f, 1.4f - tt * 1.3f, -6.0f + rand_range(-1.5f,1.5f) }, 0.3f, 6, 6, Color{ 220, 240, 238, (unsigned char)(80 * (1.0f - tt)) }); }   // weir fall
    for (int k = 0; k < 8; k++) DrawSphereEx(ctr + Vector3{ -6.5f + rand_range(-1.0f,1.0f), 0.2f + 0.3f * fabsf(sinf(s_time * 5 + k)), -6.0f + rand_range(-1.2f,1.2f) }, 0.18f, 5, 5, Color{ 220, 240, 238, 90 });   // wheel splash
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.1f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 205, 130, 50 });
    EndBlendMode();
}

// Seeded salt-pile layout for the salt pans, shared by build_salt (obstacles) and
// draw_salt (geometry). xyz = base, w = scale.
static void salt_layout(std::vector<Vector4>& piles) {
    Vector3 c = boundary_center;
    piles.clear();
    SetRandomSeed(91157);
    int tries = 0;
    while ((int)piles.size() < 9 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(6.0f, boundary_radius - 2.0f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 5.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& k : piles) if (Vector3Distance(p, Vector3{ k.x,0,k.z }) < 4.0f) { clash = true; break; }
        if (clash) continue;
        piles.push_back({ p.x, p.y, p.z, rand_range(0.8f, 1.5f) });
    }
}

static void build_salt() {
    s_wisps.clear();
    std::vector<Vector4> piles;
    salt_layout(piles);
    for (auto& k : piles) if (k.w > 1.1f) obstacles.push_back({ Vector3{ k.x, 0, k.z }, 1.4f * k.w });
    Vector3 ctr = boundary_center;
    obstacles.push_back({ ctr + Vector3{ -14.0f, 0, -8.0f }, 2.4f });   // salt-works hut
    // pale brine-sheen glow at the central pile, a few piles, the hut
    s_wisps.push_back(ctr + Vector3{ 0, 1.4f, 0 });
    for (size_t i = 0; i < piles.size(); i += 2) s_wisps.push_back(Vector3{ piles[i].x, 1.0f, piles[i].z });
    s_wisps.push_back(ctr + Vector3{ -14.0f, 1.6f, -8.0f });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.0f, w.z, 4.5f }); }
}

static void draw_salt() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color salt{ 236,232,224,255 }, saltDk{ 210,204,194,255 }, brineA{ 176,216,220,255 }, brineB{ 222,202,212,255 };
    Color crystal{ 246,246,250,255 }, wood{ 120,94,62,255 }, hut{ 206,200,188,255 }, roof{ 150,120,90,255 };

    // blinding salt-crust ground slab
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, salt);

    // grid of shallow brine evaporation ponds (raised white berms = the gaps between)
    for (int ix = -2; ix <= 2; ix++) for (int iz = -2; iz <= 2; iz++) {
        Vector3 cell = ctr + Vector3{ ix * 8.0f, 0, iz * 8.0f };
        if (cell.z > ctr.z + 13.0f) continue;
        if (ix == 0 && iz == 0) continue;                       // central pile sits here
        DrawModelEx(s_column, cell + Vector3{ 0, 0.05f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 7.0f, 0.12f, 7.0f }, ((ix + iz) & 1) ? brineA : brineB);
    }
    // raised berm crests along the grid lines (low salt walls)
    for (int i = -2; i <= 2; i++) { DrawModelEx(s_column, ctr + Vector3{ i * 8.0f - 4.0f, 0.18f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.5f, 40.0f }, saltDk); DrawModelEx(s_column, ctr + Vector3{ 0, 0.18f, i * 8.0f - 4.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 40.0f, 0.5f, 0.6f }, saltDk); }

    // conical harvested-salt piles + the central great pile
    std::vector<Vector4> piles;
    salt_layout(piles);
    for (auto& k : piles) { DrawModelEx(s_cone, Vector3{ k.x, 0, k.z }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f * k.w, 2.2f * k.w, 1.6f * k.w }, salt); DrawModelEx(s_cone, Vector3{ k.x, 0, k.z }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f * k.w, 1.3f * k.w, 1.0f * k.w }, crystal); }
    DrawModelEx(s_cone, ctr, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 3.4f, 2.4f }, salt);
    DrawModelEx(s_cone, ctr, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 2.0f, 1.5f }, crystal);

    // jagged salt-crust shard clusters at berm crossings
    SetRandomSeed(414);
    for (int i = 0; i < 9; i++) { float a = rand_range(0, 2 * PI), r = rand_range(5.0f, boundary_radius - 2.0f); Vector3 c = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (c.z > ctr.z + 12.0f) continue; for (int k = 0; k < 4; k++) { Vector3 p = c + Vector3{ rand_range(-0.8f,0.8f), 0, rand_range(-0.8f,0.8f) }; DrawModelEx(s_cone, p, Vector3{ rand_range(-0.2f,0.2f),1,rand_range(-0.2f,0.2f) }, 0, Vector3{ rand_range(0.25f,0.5f), rand_range(0.5f,1.2f), rand_range(0.25f,0.5f) }, crystal); } }

    // salt-works hut + leaning rakes + a handcart of salt
    Vector3 h = ctr + Vector3{ -14.0f, 0, -8.0f };
    draw_cottage(h, 4.0f, 3.4f, 3.0f, 1.8f, 40.0f, hut, roof, wood);
    for (int i = 0; i < 3; i++) draw_bone_seg(h + Vector3{ 2.6f + i * 0.4f, 0, 2.0f }, h + Vector3{ 2.0f + i * 0.4f, 2.4f, 2.4f }, 0.07f, wood);   // leaning rakes
    Vector3 cart = ctr + Vector3{ 6.0f, 0, 9.0f };
    DrawModelEx(s_column, cart + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 0.8f, 1.2f }, wood); DrawModelEx(s_dome, cart + Vector3{ 0, 1.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 0.6f, 0.7f }, crystal);
    for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_cyl, cart + Vector3{ sx * 1.0f, 0.4f, 0.7f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.4f, 0.2f, 0.4f }, wood);

    // additive: brine-pond glints + heat shimmer
    BeginBlendMode(BLEND_ADDITIVE);
    SetRandomSeed(77);
    for (int i = 0; i < 24; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0.16f, cosf(a) * r }; DrawSphereEx(p, 0.25f + 0.1f * sinf(s_time * 3 + i), 5, 5, Color{ 230, 245, 248, (unsigned char)(40 + 30 * sinf(s_time * 2 + i)) }); }
    for (int i = 0; i < 14; i++) { float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 1.0f); float tt = fmodf(s_time * 0.3f + i * 0.2f, 1.0f); DrawSphereEx(ctr + Vector3{ sinf(a) * r + sinf(s_time + i), 0.3f + tt * 2.0f, cosf(a) * r }, 0.5f * (1.0f - tt), 5, 5, Color{ 240, 240, 235, (unsigned char)(20 * (1.0f - tt)) }); }
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.1f * sinf(s_time * 2 + i), 8, 8, Color{ 240, 235, 230, 40 });
    EndBlendMode();
}

// Seeded stilt-hut layout for the fishing village, shared by build_stilt (obstacles)
// and draw_stilt (geometry). xyz = base, w = yaw degrees.
static void stilt_layout(std::vector<Vector4>& huts) {
    Vector3 c = boundary_center;
    huts.clear();
    SetRandomSeed(60413);
    int tries = 0;
    while ((int)huts.size() < 6 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(8.0f, boundary_radius - 3.0f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 7.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& h : huts) if (Vector3Distance(p, Vector3{ h.x,0,h.z }) < 7.0f) { clash = true; break; }
        if (clash) continue;
        huts.push_back({ p.x, p.y, p.z, rand_range(0, 360.0f) });
    }
}

// A hut raised on 4 stilts over the water: posts + a deck + a raised cabin (reusing
// draw_cottage) + a ladder down to the waterline.
static void draw_stilthut(Vector3 base, float yawDeg, float w, float d, float wallH, Color wood, Color woodDk, Color roof, Color deck) {
    float yr = yawDeg * DEG2RAD, st = 2.7f;
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) { Vector3 o = rotate_y(Vector3{ sx * (w * 0.5f), 0, sz * (d * 0.5f) }, yr); DrawModelEx(s_cyl, base + o, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, st, 0.22f }, woodDk); }
    DrawModelEx(s_column, base + Vector3{ 0, st, 0 }, Vector3{ 0,1,0 }, yawDeg, Vector3{ w + 0.6f, 0.25f, d + 0.6f }, deck);
    draw_cottage(base + Vector3{ 0, st + 0.25f, 0 }, w, d, wallH, wallH * 0.6f, yawDeg, wood, roof, woodDk);
    Vector3 lz = rotate_y(Vector3{ 0, 0, d * 0.5f + 0.5f }, yr);
    for (int s = 0; s < 2; s++) { Vector3 r = rotate_y(Vector3{ -0.4f + s * 0.8f, 0, 0 }, yr); draw_bone_seg(base + lz + r, base + lz + r + Vector3{ 0, st, 0 }, 0.06f, woodDk); }
}

// A net-drying frame: two posts + a top beam hung with a draped rope net (a mesh of
// thin vertical and horizontal strands).
static void draw_net(Vector3 c, float yawDeg, Color woodDk, Color rope) {
    float yr = yawDeg * DEG2RAD;
    Vector3 lp = rotate_y(Vector3{ -1.6f, 0, 0 }, yr), rp = rotate_y(Vector3{ 1.6f, 0, 0 }, yr);
    DrawModelEx(s_cyl, c + lp, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 3.0f, 0.12f }, woodDk);
    DrawModelEx(s_cyl, c + rp, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 3.0f, 0.12f }, woodDk);
    Vector3 lt = c + lp + Vector3{ 0, 3.0f, 0 }, rt = c + rp + Vector3{ 0, 3.0f, 0 };
    draw_bone_seg(lt, rt, 0.07f, woodDk);
    for (int i = 0; i <= 6; i++) { float t = i / 6.0f; Vector3 top = Vector3Lerp(lt, rt, t); float drop = 1.4f + 0.5f * sinf(t * PI); draw_bone_seg(top, top + Vector3{ 0, -drop, 0 }, 0.03f, rope); }   // vertical strands (sag in the middle)
    for (int h = 0; h < 3; h++) { float y = 3.0f - 0.4f - h * 0.45f; draw_bone_seg(c + lp + Vector3{ 0, y, 0 }, c + rp + Vector3{ 0, y, 0 }, 0.025f, rope); }   // horizontal strands
}

static void build_stilt() {
    s_wisps.clear();
    std::vector<Vector4> huts;
    stilt_layout(huts);
    for (auto& h : huts) obstacles.push_back({ Vector3{ h.x, 0, h.z }, 2.2f });
    Vector3 ctr = boundary_center;
    // warm fisher-lantern glow at the longhouse + each hut
    s_wisps.push_back(ctr + Vector3{ 0, 3.4f, 0 });
    for (auto& h : huts) s_wisps.push_back(Vector3{ h.x, 3.2f, h.z });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 3.0f, w.z, 5.0f }); }
}

static void draw_stilt() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color wood{ 120,92,60,255 }, woodDk{ 88,66,46,255 }, roof{ 112,86,62,255 }, deck{ 134,104,68,255 };
    Color rope{ 200,188,156,255 }, fish{ 184,172,150,255 }, buoy{ 186,120,86,255 }, sand{ 178,164,132,255 };

    // a few muddy sandbars breaking the lagoon surface (no ground slab — water is the floor)
    SetRandomSeed(2207);
    for (int i = 0; i < 10; i++) { float a = rand_range(0, 2 * PI), r = rand_range(5.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.4f,2.8f), 0.25f, rand_range(1.4f,2.8f) }, sand); }

    // central stilt longhouse + the surrounding huts
    draw_stilthut(ctr, 20.0f, 7.0f, 4.5f, 3.4f, wood, woodDk, roof, deck);
    std::vector<Vector4> huts;
    stilt_layout(huts);
    for (auto& h : huts) draw_stilthut(Vector3{ h.x, 0, h.z }, h.w, 4.0f, 3.4f, 2.8f, wood, woodDk, roof, deck);

    // plank walkways linking the longhouse deck to each hut deck
    auto walk = [&](Vector3 a, Vector3 b) { Vector3 d = Vector3Subtract(b, a); float len = sqrtf(d.x * d.x + d.z * d.z); float yaw = atan2f(d.x, d.z) * RAD2DEG; Vector3 mid = Vector3Scale(Vector3Add(a, b), 0.5f); mid.y = 2.9f; DrawModelEx(s_column, mid, Vector3{ 0,1,0 }, yaw, Vector3{ 1.4f, 0.16f, len }, deck); for (int k = 0; k < (int)(len / 3.0f); k++) { Vector3 pp = Vector3Lerp(a, b, (k + 0.5f) / (len / 3.0f)); DrawModelEx(s_cyl, Vector3{ pp.x, 0, pp.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 2.9f, 0.18f }, woodDk); } };
    for (auto& h : huts) walk(ctr, Vector3{ h.x, 0, h.z });

    // net-drying frames + fish-drying racks + moored coracles + buoys + reeds
    SetRandomSeed(771);
    for (int i = 0; i < 5; i++) { float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; draw_net(p, rand_range(0, 180.0f), woodDk, rope); }
    SetRandomSeed(330);
    for (int i = 0; i < 4; i++) { float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0.2f, cosf(a) * r }; for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_cyl, p + Vector3{ sx * 1.2f, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 1.2f, 0.1f }, woodDk); draw_bone_seg(p + Vector3{ -1.2f, 1.2f, 0 }, p + Vector3{ 1.2f, 1.2f, 0 }, 0.06f, woodDk); for (int f = 0; f < 4; f++) DrawModelEx(s_dome, p + Vector3{ -0.9f + f * 0.6f, 0.95f, 0 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.16f, 0.4f, 0.16f }, fish); }
    SetRandomSeed(99);
    for (int i = 0; i < 6; i++) { float a = rand_range(0, 2 * PI), r = rand_range(5.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0.08f, cosf(a) * r }; DrawModelEx(s_dome, p, Vector3{ 1,0,0 }, 180.0f, Vector3{ 1.0f, 0.5f, 0.6f }, wood); }   // coracles (upturned bowls)
    SetRandomSeed(512);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0.15f + 0.1f * sinf(s_time + i), cosf(a) * r }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ 0.35f, 0.35f, 0.35f }, buoy); }   // buoys

    // additive: warm lanterns + lagoon ripple sparkle
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 200, 120, 70 });
    SetRandomSeed(404);
    for (int i = 0; i < 20; i++) { float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 1.0f); DrawSphereEx(ctr + Vector3{ sinf(a) * r, 0.1f, cosf(a) * r }, 0.2f + 0.08f * sinf(s_time * 3 + i), 5, 5, Color{ 200, 220, 230, (unsigned char)(30 + 25 * sinf(s_time * 2 + i)) }); }
    EndBlendMode();
}

// One honeycomb cell: six lit wall panels arranged tangentially into a hexagon tube,
// with a dark recessed base (so it reads as an open hex cell).
static void draw_hexcell(Vector3 c, float R, float h, Color wall, Color dark) {
    float ap = R * 0.866f;
    for (int i = 0; i < 6; i++) {
        float am = i * 1.0472f + 0.5236f;                          // edge-midpoint angle
        Vector3 p = c + Vector3{ cosf(am) * ap, h * 0.5f, sinf(am) * ap };
        DrawModelEx(s_column, p, Vector3{ 0,1,0 }, -am * RAD2DEG, Vector3{ 0.22f, h, R * 1.05f }, wall);
    }
    DrawModelEx(s_cone, c + Vector3{ 0, 0.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ R * 0.82f, 0.4f, R * 0.82f }, dark);   // dark cell floor
}

// A honeycomb cluster: a centre cell ringed by six cells (the classic comb pattern).
static void draw_comb_cluster(Vector3 c, float R, float h, Color wall, Color dark) {
    draw_hexcell(c, R, h, wall, dark);
    for (int i = 0; i < 6; i++) { float a = i * 1.0472f; draw_hexcell(c + Vector3{ cosf(a) * R * 1.732f, 0, sinf(a) * R * 1.732f }, R, h, wall, dark); }
}

// The great papery nest: a bulbous layered teardrop of staggered paper bands with a
// dark entrance hole low at the front.
static void draw_papernest(Vector3 base, float R, float H, Color paper, Color paperDk, Color dark) {
    int n = 8; float bandH = H / n;
    for (int i = 0; i < n; i++) { float t = i / (float)(n - 1); float r = R * (0.32f + 0.68f * sinf(t * PI)); DrawModelEx(s_cyl, base + Vector3{ 0, i * bandH, 0 }, Vector3{ 0,1,0 }, (float)(i * 15), Vector3{ r, bandH * 1.12f, r }, (i % 2) ? paper : paperDk); }
    DrawModelEx(s_column, base + Vector3{ 0, H * 0.22f, R * 0.72f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f, 1.5f, 0.4f }, dark);   // entrance hole
}

// Seeded honeycomb-cluster layout, shared by build_hive (obstacles) and draw_hive.
// xyz = base, w = scale.
static void hive_layout(std::vector<Vector4>& combs) {
    Vector3 c = boundary_center;
    combs.clear();
    SetRandomSeed(48817);
    int tries = 0;
    while ((int)combs.size() < 5 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(8.0f, boundary_radius - 4.0f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 7.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& k : combs) if (Vector3Distance(p, Vector3{ k.x,0,k.z }) < 7.0f) { clash = true; break; }
        if (clash) continue;
        combs.push_back({ p.x, p.y, p.z, rand_range(0.9f, 1.3f) });
    }
}

static void build_hive() {
    s_wisps.clear();
    std::vector<Vector4> combs;
    hive_layout(combs);
    for (auto& k : combs) obstacles.push_back({ Vector3{ k.x, 0, k.z }, 2.8f * k.w });
    Vector3 ctr = boundary_center;
    // amber-wax glow at the nest + each comb cluster
    s_wisps.push_back(ctr + Vector3{ 0, 2.0f, 0 });
    for (auto& k : combs) s_wisps.push_back(Vector3{ k.x, 1.4f, k.z });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.2f, w.z, 5.0f }); }
}

static void draw_hive() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color floor{ 72,56,40,255 }, floorDk{ 56,42,30,255 }, comb{ 224,176,80,255 }, combDk{ 182,140,62,255 }, cellDk{ 38,28,18,255 };
    Color paper{ 178,172,160,255 }, paperDk{ 142,136,124,255 }, larva{ 238,226,184,255 }, mound{ 86,66,46,255 };

    // waxy chitinous floor + scattered comb-wax patches
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, floor);
    SetRandomSeed(1900);
    for (int i = 0; i < 12; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.2f,2.4f), 0.12f, rand_range(1.2f,2.4f) }, floorDk); }

    // central great paper nest
    draw_papernest(ctr, 3.0f, 9.0f, paper, paperDk, cellDk);

    // honeycomb-cell clusters (true hexagonal cells)
    std::vector<Vector4> combs;
    hive_layout(combs);
    for (auto& k : combs) draw_comb_cluster(Vector3{ k.x, 0, k.z }, 1.3f * k.w, 2.6f * k.w, (k.w > 1.1f ? comb : combDk), cellDk);

    // hanging larval combs (golden) drooping from above + chitinous drone mounds
    SetRandomSeed(311);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 3.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; float hy = rand_range(6.0f, 9.0f); draw_bone_seg(p + Vector3{ 0, hy, 0 }, p + Vector3{ 0, hy - 1.5f, 0 }, 0.1f, paperDk); for (int k = 0; k < 3; k++) DrawModelEx(s_dome, p + Vector3{ 0, hy - 2.0f - k * 0.7f, 0 }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 0.7f - k * 0.12f, 0.6f, 0.7f - k * 0.12f }, (k % 2) ? comb : larva); }
    SetRandomSeed(733);
    for (int i = 0; i < 7; i++) { float a = rand_range(0, 2 * PI), r = rand_range(5.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; DrawModelEx(s_dome, p + Vector3{ 0, -0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.9f,1.6f), rand_range(0.6f,1.2f), rand_range(0.9f,1.6f) }, mound); }

    // additive: amber wax glow + dense wasp swarm
    BeginBlendMode(BLEND_ADDITIVE);
    Color wasp{ 245, 205, 95, 255 };
    auto swarm = [&](Vector3 o, int n, float rad) { for (int k = 0; k < n; k++) { float ph = s_time * 2.2f + k * 1.7f, ph2 = s_time * 1.5f + k * 0.9f; Vector3 p = o + Vector3{ sinf(ph) * rad * (0.4f + 0.6f * sinf(ph2)), 1.6f + 1.4f * sinf(ph2 * 1.3f + k), cosf(ph) * rad * (0.4f + 0.6f * cosf(ph2)) }; DrawSphereEx(p, 0.07f, 4, 4, Color{ wasp.r, wasp.g, wasp.b, 210 }); } };
    swarm(ctr, 30, 3.2f);
    for (auto& k : combs) swarm(Vector3{ k.x, 0, k.z }, 8, 1.8f);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.14f * sinf(s_time * 1.6f + i), 8, 8, Color{ 255, 170, 70, 55 });
    EndBlendMode();
}

// A copper pot still: a brick furnace base, a bulbous onion pot, a tapering swan-neck
// rising to an arcing lyne pipe that drops into a wooden condenser (worm tub).
static void draw_still(Vector3 base, Color copper, Color copperLt, Color brick, Color wood) {
    DrawModelEx(s_cyl, base + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.7f, 1.2f, 1.7f }, brick);      // furnace base
    DrawModelEx(s_cyl, base + Vector3{ 0, 1.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.8f, 1.6f, 1.8f }, copper);     // pot body
    DrawModelEx(s_dome, base + Vector3{ 0, 3.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.9f, 1.4f, 1.9f }, copperLt);  // onion bulb
    DrawModelEx(s_cyl, base + Vector3{ 0, 4.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.4f, 0.5f }, copper);     // swan neck
    Vector3 top = base + Vector3{ 0, 6.0f, 0 };
    draw_bone_seg(top, top + Vector3{ 1.6f, 0.2f, 0 }, 0.22f, copper);                                              // lyne arm
    draw_bone_seg(top + Vector3{ 1.6f, 0.2f, 0 }, base + Vector3{ 3.0f, 3.0f, 0 }, 0.22f, copper);                  // down-pipe
    DrawModelEx(s_cyl, base + Vector3{ 3.0f, 1.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 2.8f, 1.0f }, wood);    // condenser worm tub
    DrawModelEx(s_dome, base + Vector3{ 3.0f, 2.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.4f, 1.0f }, copper);
}

// A hoop-bound fermentation vat (upright cask): body + iron hoops + a domed lid.
static void draw_vat(Vector3 base, float sc, bool open, Color wood, Color woodDk) {
    DrawModelEx(s_cyl, base + Vector3{ 0, 1.4f * sc, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f * sc, 2.8f * sc, 1.4f * sc }, wood);
    for (int h = 0; h < 3; h++) DrawModelEx(s_cyl, base + Vector3{ 0, 0.6f * sc + h * 1.0f * sc, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f * sc, 0.12f * sc, 1.5f * sc }, woodDk);
    if (!open) DrawModelEx(s_dome, base + Vector3{ 0, 2.8f * sc, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.45f * sc, 0.5f * sc, 1.45f * sc }, woodDk);
}

// Seeded vat layout, shared by build_brew (obstacles) and draw_brew. xyz = base,
// w = scale.
static void brew_layout(std::vector<Vector4>& vats) {
    Vector3 c = boundary_center;
    vats.clear();
    SetRandomSeed(50519);
    int tries = 0;
    while ((int)vats.size() < 8 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(6.0f, boundary_radius - 3.0f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 6.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& k : vats) if (Vector3Distance(p, Vector3{ k.x,0,k.z }) < 4.0f) { clash = true; break; }
        if (clash) continue;
        vats.push_back({ p.x, p.y, p.z, rand_range(0.9f, 1.3f) });
    }
}

static void build_brew() {
    s_wisps.clear();
    std::vector<Vector4> vats;
    brew_layout(vats);
    for (auto& k : vats) obstacles.push_back({ Vector3{ k.x, 0, k.z }, 1.6f * k.w });
    Vector3 ctr = boundary_center;
    // copper/furnace glow at the still furnace + open vats
    s_wisps.push_back(ctr + Vector3{ 0, 0.8f, 1.7f });
    for (size_t i = 0; i < vats.size(); i += 2) s_wisps.push_back(Vector3{ vats[i].x, 2.8f * vats[i].w, vats[i].z });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 4.5f }); }
}

static void draw_brew() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color floor{ 96,84,68,255 }, floorDk{ 76,66,52,255 }, stone{ 140,134,124,255 }, wood{ 120,92,60,255 }, woodDk{ 88,66,46,255 };
    Color copper{ 200,118,70,255 }, copperLt{ 224,150,96,255 }, brick{ 150,90,70,255 }, sack{ 184,162,112,255 }, hop{ 120,150,80,255 }, fire{ 255,150,60,255 };

    // flagged brewhouse floor + plank seams + spill stains
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, floor);
    for (int i = -6; i <= 6; i++) DrawModelEx(s_column, ctr + Vector3{ i * 3.4f, -0.04f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.14f, 1.0f, boundary_radius * 1.9f }, floorDk);
    SetRandomSeed(1900);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.44f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.0f,2.0f), 0.1f, rand_range(1.0f,2.0f) }, floorDk); }

    // the central copper still
    draw_still(ctr, copper, copperLt, brick, wood);

    // fermentation vats (every 3rd left open, brimming with froth)
    std::vector<Vector4> vats;
    brew_layout(vats);
    for (size_t i = 0; i < vats.size(); i++) draw_vat(Vector3{ vats[i].x, 0, vats[i].z }, vats[i].w, (i % 3 == 0), wood, woodDk);

    // copper pipes linking the still to a couple of vats
    for (size_t i = 0; i < vats.size() && i < 3; i++) { Vector3 v = Vector3{ vats[i].x, 2.4f, vats[i].z }; draw_bone_seg(ctr + Vector3{ 0, 2.4f, 0 }, v, 0.12f, copper); }

    // wide shallow mash tuns + cask-stack pyramids + grain sacks + hop garlands
    SetRandomSeed(330);
    for (int i = 0; i < 2; i++) { float a = rand_range(0, 2 * PI), r = rand_range(7.0f, boundary_radius - 3.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; DrawModelEx(s_cyl, p + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 1.0f, 2.4f }, wood); for (int h = 0; h < 2; h++) DrawModelEx(s_cyl, p + Vector3{ 0, 0.3f + h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.5f, 0.12f, 2.5f }, woodDk); }
    Vector3 casks[2] = { ctr + Vector3{ -12.0f, 0, 6.0f }, ctr + Vector3{ 11.0f, 0, -9.0f } };
    for (int i = 0; i < 2; i++) draw_logpile(casks[i], (float)(i * 40), 1.6f, 0.65f, 3, wood, copperLt);
    SetRandomSeed(551);
    for (int i = 0; i < 6; i++) { float a = rand_range(0, 2 * PI), r = rand_range(8.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; DrawModelEx(s_dome, p + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.9f, 0.6f }, sack); }
    for (int i = 0; i < 5; i++) { Vector3 a = ctr + Vector3{ -10.0f + i * 5.0f, 5.0f, -13.0f }, b = ctr + Vector3{ -7.5f + i * 5.0f, 4.4f, -13.0f }; draw_bone_seg(a, b, 0.05f, hop); for (int k = 0; k < 4; k++) DrawModelEx(s_dome, Vector3Lerp(a, b, k / 4.0f) + Vector3{ 0, -0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.4f, 0.2f }, hop); }

    // additive: furnace glow + still steam + vat froth + open-vat froth + copper glints
    BeginBlendMode(BLEND_ADDITIVE);
    Vector3 furn = ctr + Vector3{ 0, 0.8f, 1.7f };
    for (int k = 0; k < 4; k++) DrawSphereEx(furn + Vector3{ 0.1f * sinf(s_time * 6 + k), 0, 0.2f * k }, 0.5f - 0.09f * k, 7, 7, Color{ fire.r, fire.g, fire.b, (unsigned char)(130 - 26 * k) });
    for (int k = 0; k < 5; k++) { float tt = fmodf(s_time * 0.4f + k * 0.2f, 1.0f); DrawSphereEx(ctr + Vector3{ 0, 6.5f + tt * 3.5f, 0 }, 0.5f + tt * 0.6f, 6, 6, Color{ 210, 210, 205, (unsigned char)(45 * (1.0f - tt)) }); }
    for (size_t i = 0; i < vats.size(); i++) if (i % 3 == 0) { Vector3 v = Vector3{ vats[i].x, 2.8f * vats[i].w, vats[i].z }; DrawSphereEx(v, 1.2f * vats[i].w, 7, 7, Color{ 236, 228, 200, 60 }); for (int k = 0; k < 3; k++) { float tt = fmodf(s_time * 0.5f + k * 0.3f + i, 1.0f); DrawSphereEx(v + Vector3{ 0.3f * sinf(s_time * 2 + k), tt * 1.2f, 0 }, 0.3f * (1.0f - tt), 5, 5, Color{ 230, 222, 196, (unsigned char)(70 * (1.0f - tt)) }); } }
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.1f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 170, 90, 45 });
    EndBlendMode();
}

// Seeded bamboo-clump layout, shared by build_bamboo (obstacles) and draw_bamboo.
// xyz = clump centre, w = seed.
static void bamboo_layout(std::vector<Vector4>& clumps) {
    Vector3 c = boundary_center;
    clumps.clear();
    SetRandomSeed(31771);
    int tries = 0;
    while ((int)clumps.size() < 16 && tries < 400) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(4.0f, boundary_radius - 1.5f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 4.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& k : clumps) if (Vector3Distance(p, Vector3{ k.x,0,k.z }) < 3.2f) { clash = true; break; }
        if (clash) continue;
        clumps.push_back({ p.x, p.y, p.z, (float)(300 + (int)clumps.size() * 23) });
    }
}

// A bamboo culm: a tall segmented stalk (chained draw_bone_seg, slightly curved and
// tapering) with darker node rings between joints and a few leaf sprays at the top.
static void draw_bamboo_stalk(Vector3 base, float h, int seed, Color green, Color greenDk, Color node, Color leaf) {
    SetRandomSeed(seed);
    int segs = 6; float lean = rand_range(-0.35f, 0.35f), r0 = rand_range(0.13f, 0.2f), tw = rand_range(0, 6.28f);
    Vector3 prev = base; float seglen = h / segs;
    for (int i = 0; i < segs; i++) {
        float t = (i + 1) / (float)segs;
        Vector3 nxt = base + Vector3{ lean * t * h * 0.12f + sinf(t * 3 + tw) * h * 0.02f, seglen * (i + 1), cosf(t * 2 + tw) * h * 0.015f };
        float r = r0 * (1.0f - 0.35f * t); if (r < 0.06f) r = 0.06f;
        draw_bone_seg(prev, nxt, r, (i % 2) ? green : greenDk);
        DrawModelEx(s_cyl, nxt, Vector3{ 0,1,0 }, 0, Vector3{ r * 1.45f, 0.1f, r * 1.45f }, node);
        prev = nxt;
    }
    for (int k = 0; k < 4; k++) { float a = k * 1.5708f + tw; draw_bone_seg(prev, prev + Vector3{ cosf(a) * 0.9f, 0.5f, sinf(a) * 0.9f }, 0.05f, leaf); }
}

static void build_bamboo() {
    s_wisps.clear();
    std::vector<Vector4> clumps;
    bamboo_layout(clumps);
    for (auto& k : clumps) obstacles.push_back({ Vector3{ k.x, 0, k.z }, 1.0f });
    Vector3 ctr = boundary_center;
    Vector3 lanterns[4] = { ctr + Vector3{ -6.0f, 0, 5.0f }, ctr + Vector3{ 6.0f, 0, 5.0f }, ctr + Vector3{ -7.0f, 0, -7.0f }, ctr + Vector3{ 7.0f, 0, -7.0f } };
    for (auto& l : lanterns) obstacles.push_back({ l, 0.6f });
    // soft dappled + warm lantern glow at the zen garden + lanterns
    s_wisps.push_back(ctr + Vector3{ 0, 1.4f, 0 });
    for (auto& l : lanterns) s_wisps.push_back(l + Vector3{ 0, 1.5f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 1.2f, w.z, 4.5f }); }
}

static void draw_bamboo() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color ground{ 84,96,64,255 }, groundDk{ 70,82,52,255 }, gravel{ 186,184,170,255 }, green{ 120,168,86,255 }, greenDk{ 94,138,68,255 };
    Color node{ 68,94,50,255 }, leaf{ 122,178,84,255 }, boulder{ 112,110,102,255 }, lstone{ 172,166,154,255 }, wood{ 120,92,60,255 };

    // mossy ground + scattered moss mounds
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, ground);
    SetRandomSeed(1900);
    for (int i = 0; i < 12; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.2f,2.4f), 0.12f, rand_range(1.2f,2.4f) }, groundDk); }

    // raked-gravel zen circle at centre + a winding gravel "dry stream"
    DrawModelEx(s_cyl, ctr + Vector3{ 0, -0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.0f, 0.18f, 5.0f }, gravel);
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.42f, 8.0f }, Vector3{ 0,1,0 }, 18.0f, Vector3{ 3.0f, 0.18f, 14.0f }, gravel);   // stream toward the entrance

    // the central zen rock arrangement (a few mossy boulders set in the gravel)
    DrawModelEx(s_dome, ctr, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 1.4f, 1.6f }, boulder);
    DrawModelEx(s_dome, ctr + Vector3{ 1.6f, 0, 0.8f }, Vector3{ 0,1,0 }, 30.0f, Vector3{ 0.9f, 0.7f, 0.9f }, boulder);
    DrawModelEx(s_dome, ctr + Vector3{ -1.4f, 0, -1.0f }, Vector3{ 0,1,0 }, 60.0f, Vector3{ 0.7f, 0.5f, 0.7f }, boulder);
    DrawModelEx(s_dome, ctr + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 0.4f, 1.4f }, greenDk);   // moss cap

    // the bamboo clumps (4 culms each)
    std::vector<Vector4> clumps;
    bamboo_layout(clumps);
    for (auto& k : clumps) { Vector3 b = Vector3{ k.x, 0, k.z }; SetRandomSeed((int)k.w); for (int s = 0; s < 4; s++) { Vector3 o = b + Vector3{ rand_range(-0.7f,0.7f), 0, rand_range(-0.7f,0.7f) }; draw_bamboo_stalk(o, rand_range(7.0f, 12.0f), (int)k.w + s * 7, green, greenDk, node, leaf); } }

    // stone lanterns (pedestal + fire box + cap)
    Vector3 lanterns[4] = { ctr + Vector3{ -6.0f, 0, 5.0f }, ctr + Vector3{ 6.0f, 0, 5.0f }, ctr + Vector3{ -7.0f, 0, -7.0f }, ctr + Vector3{ 7.0f, 0, -7.0f } };
    for (auto& l : lanterns) { DrawModelEx(s_cyl, l + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.0f, 0.3f }, lstone); DrawModelEx(s_column, l + Vector3{ 0, 1.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.6f, 0.6f }, lstone); DrawModelEx(s_cone, l + Vector3{ 0, 1.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.5f, 0.7f }, lstone); }

    // a small arched moon-bridge over the gravel stream
    Vector3 br = ctr + Vector3{ 0, 0, 12.0f };
    for (int i = -2; i <= 2; i++) { float y = 0.5f + (1.0f - (i * i) / 4.0f) * 0.8f; DrawModelEx(s_column, br + Vector3{ (float)i * 0.9f, y, 0 }, Vector3{ 0,0,1 }, (float)(-i * 12), Vector3{ 1.0f, 0.16f, 2.6f }, wood); }
    for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_cyl, br + Vector3{ sx * 2.0f, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.15f, 1.0f, 0.15f }, wood);

    // additive: green dappled light shafts + drifting leaves + warm lantern glow
    BeginBlendMode(BLEND_ADDITIVE);
    SetRandomSeed(404);
    for (int i = 0; i < 7; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r + 2.0f * sinf(s_time * 0.2f + i), 7.0f, cosf(a) * r }; DrawCube(p, 1.6f, 14.0f, 1.6f, Color{ 150, 210, 120, 14 }); }
    SetRandomSeed(77);
    for (int i = 0; i < 40; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius), sp = rand_range(0.3f, 0.7f), off = rand_range(0, 1); float y = 11.0f - fmodf(s_time * sp + off, 1.0f) * 11.0f; DrawSphereEx(ctr + Vector3{ bx + sinf(s_time * 0.5f + i) * 0.8f, y, bz }, 0.08f, 4, 4, Color{ 180, 200, 110, 130 }); }
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.1f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 215, 150, 45 });
    EndBlendMode();
}

// A charcoal-burner's mound (meiler): a stacked-cordwood drum buried under a turf
// dome, vented by a central chimney. Smoulders from within.
static void draw_meiler(Vector3 base, float sc, Color turf, Color turfDk, Color log, Color dark) {
    DrawModelEx(s_cyl, base + Vector3{ 0, 0.5f * sc, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f * sc, 1.0f * sc, 2.0f * sc }, log);       // cordwood drum
    DrawModelEx(s_dome, base + Vector3{ 0, 1.0f * sc, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f * sc, 1.9f * sc, 2.2f * sc }, turf);     // turf dome
    DrawModelEx(s_dome, base + Vector3{ 0.7f * sc, 1.4f * sc, 0.4f * sc }, Vector3{ 0,1,0 }, 40.0f, Vector3{ 0.9f * sc, 0.5f * sc, 0.9f * sc }, turfDk);   // turf patch
    DrawModelEx(s_cyl, base + Vector3{ 0, 2.7f * sc, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f * sc, 0.7f * sc, 0.3f * sc }, dark);      // chimney vent
}

// Seeded meiler layout, shared by build_collier (obstacles) and draw_collier.
// xyz = base, w = scale.
static void collier_layout(std::vector<Vector4>& mounds) {
    Vector3 c = boundary_center;
    mounds.clear();
    SetRandomSeed(52119);
    int tries = 0;
    while ((int)mounds.size() < 5 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(8.0f, boundary_radius - 4.0f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 8.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& k : mounds) if (Vector3Distance(p, Vector3{ k.x,0,k.z }) < 7.0f) { clash = true; break; }
        if (clash) continue;
        mounds.push_back({ p.x, p.y, p.z, rand_range(0.85f, 1.15f) });
    }
}

static void build_collier() {
    s_wisps.clear();
    std::vector<Vector4> mounds;
    collier_layout(mounds);
    for (auto& k : mounds) obstacles.push_back({ Vector3{ k.x, 0, k.z }, 2.3f * k.w });
    Vector3 ctr = boundary_center;
    obstacles.push_back({ ctr + Vector3{ -13.0f, 0, 7.0f }, 2.2f });   // collier's hut
    // ember glow at the central mound + each meiler vent
    s_wisps.push_back(ctr + Vector3{ 0, 1.2f, 0 });
    for (auto& k : mounds) s_wisps.push_back(Vector3{ k.x, 1.0f * k.w, k.z });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 4.5f }); }
}

static void draw_collier() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color ground{ 96,86,72,255 }, groundDk{ 66,58,48,255 }, turf{ 108,106,72,255 }, turfDk{ 84,90,56,255 }, log{ 112,84,56,255 };
    Color dark{ 46,40,32,255 }, charcoal{ 38,36,36,255 }, hut{ 120,110,82,255 }, roof{ 96,84,60,255 }, ember{ 255,120,50,255 };

    // ashy clearing floor + scorched scorch-rings
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, ground);
    SetRandomSeed(1900);
    for (int i = 0; i < 12; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_cyl, p + Vector3{ 0, -0.44f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.4f,2.8f), 0.12f, rand_range(1.4f,2.8f) }, groundDk); }

    // central great meiler + the smaller mounds
    draw_meiler(ctr, 1.4f, turf, turfDk, log, dark);
    std::vector<Vector4> mounds;
    collier_layout(mounds);
    for (auto& k : mounds) draw_meiler(Vector3{ k.x, 0, k.z }, k.w, turf, turfDk, log, dark);

    // cordwood stacks (reuse draw_logpile) + black charcoal piles
    Vector3 stacks[2] = { ctr + Vector3{ 12.0f, 0, 5.0f }, ctr + Vector3{ -8.0f, 0, -10.0f } };
    for (int i = 0; i < 2; i++) draw_logpile(stacks[i], (float)(i * 50), 4.5f, 0.45f, 4, log, turfDk);
    SetRandomSeed(551);
    for (int i = 0; i < 6; i++) { float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; DrawModelEx(s_dome, p + Vector3{ 0, -0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.8f,1.4f), rand_range(0.4f,0.8f), rand_range(0.8f,1.4f) }, charcoal); }

    // collier's turf hut + axes-in-stumps + a charcoal barrow
    Vector3 h = ctr + Vector3{ -13.0f, 0, 7.0f };
    draw_cottage(h, 3.6f, 3.0f, 2.6f, 1.6f, 30.0f, hut, roof, dark);
    DrawModelEx(s_dome, h + Vector3{ 0, 2.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 0.8f, 2.0f }, turf);   // turf roof over the cabin
    SetRandomSeed(330);
    for (int i = 0; i < 4; i++) { float a = rand_range(0, 2 * PI), r = rand_range(7.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; DrawModelEx(s_cyl, p, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.7f, 0.6f }, log); draw_bone_seg(p + Vector3{ 0, 0.7f, 0 }, p + Vector3{ 0.5f, 1.6f, 0 }, 0.07f, dark); DrawModelEx(s_column, p + Vector3{ 0.6f, 1.7f, 0 }, Vector3{ 0,0,1 }, 30.0f, Vector3{ 0.5f, 0.3f, 0.12f }, groundDk); }   // axe in stump

    // additive: ember glow at vents + rising smoke columns
    BeginBlendMode(BLEND_ADDITIVE);
    auto smoulder = [&](Vector3 b, float sc) {
        DrawSphereEx(b + Vector3{ 0, 2.9f * sc, 0 }, 0.4f * sc * (0.7f + 0.3f * sinf(s_time * 4 + b.x)), 7, 7, Color{ ember.r, ember.g, ember.b, 90 });   // vent ember
        for (int k = 0; k < 6; k++) { float tt = fmodf(s_time * 0.3f + k * 0.16f, 1.0f); DrawSphereEx(b + Vector3{ 0.3f * sinf(s_time + k), 3.0f * sc + tt * 5.0f, 0.3f * cosf(s_time + k) }, (0.5f + tt * 0.7f) * sc, 6, 6, Color{ 120, 116, 110, (unsigned char)(55 * (1.0f - tt)) }); }
    };
    smoulder(ctr, 1.4f);
    for (auto& k : mounds) smoulder(Vector3{ k.x, 0, k.z }, k.w);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.1f * sinf(s_time * 3 + i), 8, 8, Color{ ember.r, ember.g, ember.b, 40 });
    EndBlendMode();
}

// A grand multi-tiered civic fountain: three stacked stone basins of decreasing size
// on stems, each holding a teal water disc, crowned with a gilded finial.
static void draw_grandfount(Vector3 b, Color stone, Color water, Color gold) {
    DrawModelEx(s_cyl, b + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 1.0f, 3.4f }, stone);    // lower basin
    DrawModelEx(s_cyl, b + Vector3{ 0, 0.95f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 0.2f, 3.0f }, water);
    DrawModelEx(s_cyl, b + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.4f, 0.6f }, stone);    // stem
    DrawModelEx(s_cyl, b + Vector3{ 0, 2.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.9f, 0.5f, 1.9f }, stone);    // mid basin
    DrawModelEx(s_cyl, b + Vector3{ 0, 2.72f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 0.16f, 1.6f }, water);
    DrawModelEx(s_cyl, b + Vector3{ 0, 3.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.45f, 1.0f, 0.45f }, stone);  // stem 2
    DrawModelEx(s_cyl, b + Vector3{ 0, 4.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f, 0.4f, 1.1f }, stone);    // top basin
    DrawModelEx(s_cyl, b + Vector3{ 0, 4.22f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 0.14f, 0.9f }, water);
    DrawModelEx(s_dome, b + Vector3{ 0, 4.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.9f, 0.5f }, gold);    // finial
}

static void build_plaza() {
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    // arcade ring (14 arches) — obstacles at the perimeter
    const int N = 14; float R = 20.0f;
    for (int i = 0; i < N; i++) { float a = (i / (float)N) * 2 * PI; Vector3 p = ctr + Vector3{ sinf(a) * R, 0, cosf(a) * R }; if (p.z > ctr.z + 14.0f) continue; obstacles.push_back({ p, 1.6f }); }
    Vector3 obelisks[4] = { ctr + Vector3{ -12.0f, 0, -12.0f }, ctr + Vector3{ 12.0f, 0, -12.0f }, ctr + Vector3{ -12.0f, 0, 10.0f }, ctr + Vector3{ 12.0f, 0, 10.0f } };
    for (auto& o : obelisks) obstacles.push_back({ o, 1.0f });
    // civic-lamp glow at the fountain + corner lamps
    s_wisps.push_back(ctr + Vector3{ 0, 3.0f, 0 });
    for (auto& o : obelisks) s_wisps.push_back(o + Vector3{ 0, 3.5f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_plaza() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color cobble{ 138,134,126,255 }, cobbleDk{ 110,106,100,255 }, stone{ 164,160,150,255 }, water{ 120,180,200,255 }, gold{ 204,172,100,255 };
    Color awnA{ 182,82,72,255 }, awnB{ 210,205,195,255 }, wood{ 120,92,60,255 }, iron{ 70,70,76,255 };

    // cobbled paving + a darker compass-rose inlay (concentric rings + radial spokes)
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, cobble);
    for (int r = 0; r < 3; r++) DrawModelEx(s_cyl, ctr + Vector3{ 0, -0.46f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 7.0f + r * 5.0f, 1.0f, 7.0f + r * 5.0f }, (r % 2) ? cobbleDk : cobble);
    for (int i = 0; i < 8; i++) { float a = i * (PI / 4.0f); DrawModelEx(s_column, ctr + Vector3{ sinf(a) * 9.0f, -0.45f, cosf(a) * 9.0f }, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 0.5f, 1.0f, 14.0f }, cobbleDk); }

    // the grand fountain
    draw_grandfount(ctr, stone, water, gold);

    // arcade colonnade ring (reuse draw_arch, Romanesque) facing inward
    const int N = 14; float R = 20.0f;
    for (int i = 0; i < N; i++) { float a = (i / (float)N) * 2 * PI; Vector3 p = ctr + Vector3{ sinf(a) * R, 0, cosf(a) * R }; if (p.z > ctr.z + 14.0f) continue; Arch A{ p, -a * RAD2DEG, 5.0f, 5.5f, 1.1f }; draw_arch(A, (i % 2) ? stone : cobbleDk); }

    // corner obelisks (tapered stone) + statue plinths with urns
    Vector3 obelisks[4] = { ctr + Vector3{ -12.0f, 0, -12.0f }, ctr + Vector3{ 12.0f, 0, -12.0f }, ctr + Vector3{ -12.0f, 0, 10.0f }, ctr + Vector3{ 12.0f, 0, 10.0f } };
    for (auto& o : obelisks) { DrawModelEx(s_column, o + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 1.0f, 1.4f }, stone); DrawModelEx(s_cone, o + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 5.0f, 0.7f }, stone); DrawModelEx(s_dome, o + Vector3{ 0, 6.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.25f, 0.4f, 0.25f }, gold); }

    // market stalls (post frame + striped awning + counter)
    SetRandomSeed(2300);
    for (int i = 0; i < 4; i++) { float a = rand_range(0, 2 * PI), r = rand_range(8.0f, 14.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_cyl, p + Vector3{ sx * 1.6f, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.14f, 2.0f, 0.14f }, wood); DrawModelEx(s_column, p + Vector3{ 0, 2.0f, -0.6f }, Vector3{ 1,0,0 }, 25.0f, Vector3{ 3.6f, 0.12f, 2.0f }, (i % 2) ? awnA : awnB); DrawModelEx(s_column, p + Vector3{ 0, 0.8f, 0.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 0.9f, 0.8f }, wood); }

    // iron lamp posts with glowing lamps
    SetRandomSeed(660);
    for (int i = 0; i < 6; i++) { float a = (i / 6.0f) * 2 * PI; Vector3 p = ctr + Vector3{ sinf(a) * 11.0f, 0, cosf(a) * 11.0f }; if (p.z > ctr.z + 13.0f) continue; DrawModelEx(s_cyl, p + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 3.2f, 0.16f }, iron); DrawModelEx(s_dome, p + Vector3{ 0, 3.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.5f, 0.4f }, gold); }

    // additive: fountain cascades + central jet + lamp glow + pigeons
    BeginBlendMode(BLEND_ADDITIVE);
    float jet = 2.0f + 0.6f * fabsf(sinf(s_time * 1.2f));
    for (int k = 0; k < 6; k++) { float t = k / 6.0f; DrawSphereEx(ctr + Vector3{ 0.1f * sinf(s_time * 4 + k), 4.6f + t * jet, 0.1f * cosf(s_time * 4 + k) }, 0.3f * (1.0f - t * 0.5f), 6, 6, Color{ 200, 235, 245, (unsigned char)(90 * (1.0f - t)) }); }
    for (int tier = 0; tier < 2; tier++) { float ry = tier == 0 ? 4.0f : 2.4f, rr = tier == 0 ? 0.9f : 1.6f; for (int k = 0; k < 10; k++) { float a = (k / 10.0f) * 2 * PI, tt = fmodf(s_time * 1.5f + k * 0.1f, 1.0f); DrawSphereEx(ctr + Vector3{ cosf(a) * rr, ry - tt * 1.2f, sinf(a) * rr }, 0.14f, 5, 5, Color{ 200, 235, 245, (unsigned char)(80 * (1.0f - tt)) }); } }   // cascade rings
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.1f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 210, 150, 50 });
    for (int k = 0; k < 10; k++) { float ph = s_time * 0.8f + k * 0.62f; DrawSphereEx(ctr + Vector3{ sinf(ph) * 13.0f, 5.0f + 1.0f * sinf(ph * 0.7f + k), cosf(ph) * 13.0f }, 0.12f, 4, 4, Color{ 220, 220, 225, 150 }); }   // pigeons
    EndBlendMode();
}

// A ribbed pumpkin: a squashed sphere (a top dome + a flipped bottom dome) banded by
// a few darker ribs, with a green stem.
static void draw_pumpkin_gourd(Vector3 b, float r, Color col, Color colDk, Color stem) {
    DrawModelEx(s_dome, b + Vector3{ 0, r * 0.55f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r, r * 0.7f, r }, col);          // top half
    DrawModelEx(s_dome, b + Vector3{ 0, r * 0.55f, 0 }, Vector3{ 1,0,0 }, 180.0f, Vector3{ r, r * 0.7f, r }, col);     // bottom half
    for (int i = 0; i < 5; i++) { float a = i * 1.2566f; draw_bone_seg(b + Vector3{ cosf(a) * r * 0.92f, 0.05f, sinf(a) * r * 0.92f }, b + Vector3{ cosf(a) * 0.15f, r * 1.05f, sinf(a) * 0.15f }, 0.035f * r, colDk); }   // ribs
    DrawModelEx(s_cyl, b + Vector3{ 0, r * 0.95f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f * r, 0.5f * r, 0.12f * r }, stem);   // stem
}

// A corn shock: a teepee bundle of dried cornstalks tied with a band.
static void draw_cornshock(Vector3 base, float sc, Color corn, Color cornDk) {
    Vector3 top = base + Vector3{ 0, 2.4f * sc, 0 };
    for (int i = 0; i < 9; i++) { float a = i * 0.698f; draw_bone_seg(base + Vector3{ cosf(a) * 0.7f * sc, 0, sinf(a) * 0.7f * sc }, top + Vector3{ cosf(a) * 0.15f * sc, 0, sinf(a) * 0.15f * sc }, 0.06f * sc, (i % 2) ? corn : cornDk); }
    DrawModelEx(s_cyl, base + Vector3{ 0, 1.4f * sc, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f * sc, 0.12f * sc, 0.7f * sc }, cornDk);   // tie band
}

// Seeded pumpkin layout, shared by build_pumpkin (obstacles) and draw_pumpkin.
// xyz = base, w = radius.
static void pumpkin_layout(std::vector<Vector4>& gourds) {
    Vector3 c = boundary_center;
    gourds.clear();
    SetRandomSeed(53117);
    int tries = 0;
    while ((int)gourds.size() < 13 && tries < 400) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(4.0f, boundary_radius - 2.0f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 4.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& k : gourds) if (Vector3Distance(p, Vector3{ k.x,0,k.z }) < 3.0f) { clash = true; break; }
        if (clash) continue;
        gourds.push_back({ p.x, p.y, p.z, rand_range(0.7f, 1.5f) });
    }
}

static void build_pumpkin() {
    s_wisps.clear();
    std::vector<Vector4> gourds;
    pumpkin_layout(gourds);
    for (auto& k : gourds) if (k.w > 1.1f) obstacles.push_back({ Vector3{ k.x, 0, k.z }, k.w + 0.2f });
    Vector3 ctr = boundary_center;
    // jack-o'-lantern glow at the central great pumpkin + every 3rd gourd (the carved ones)
    s_wisps.push_back(ctr + Vector3{ 0, 1.4f, 0 });
    for (size_t i = 0; i < gourds.size(); i += 3) s_wisps.push_back(Vector3{ gourds[i].x, gourds[i].w * 0.6f, gourds[i].z });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, 0.8f, w.z, 4.0f }); }
}

static void draw_pumpkin() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color earth{ 96,80,58,255 }, earthDk{ 72,60,44,255 }, vine{ 90,128,64,255 }, pump{ 224,118,40,255 }, pumpDk{ 176,86,30,255 };
    Color stem{ 110,140,70,255 }, corn{ 196,176,120,255 }, cornDk{ 158,138,92,255 }, hay{ 200,180,124,255 }, straw{ 206,184,134,255 }, dark{ 38,28,22,255 };

    // earthy field + furrow rows + sprawling vine tendrils + fallen leaves
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, earth);
    for (int i = -7; i <= 7; i++) DrawModelEx(s_column, ctr + Vector3{ i * 3.0f, -0.46f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 1.0f, boundary_radius * 1.9f }, earthDk);   // furrows
    SetRandomSeed(1900);
    for (int i = 0; i < 14; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; for (int k = 0; k < 3; k++) { Vector3 q = p + Vector3{ rand_range(-1.5f,1.5f), 0, rand_range(-1.5f,1.5f) }; draw_bone_seg(q, q + Vector3{ rand_range(-1.0f,1.0f), 0.1f, rand_range(-1.0f,1.0f) }, 0.05f, vine); } }

    // the pumpkins (every 3rd carved into a glowing jack-o'-lantern) + central great pumpkin
    std::vector<Vector4> gourds;
    pumpkin_layout(gourds);
    for (size_t i = 0; i < gourds.size(); i++) {
        Vector3 b = Vector3{ gourds[i].x, 0, gourds[i].z }; float r = gourds[i].w;
        draw_pumpkin_gourd(b, r, pump, pumpDk, stem);
        if (i % 3 == 0) {   // carve a face (dark insets on the +z front)
            for (int sx = -1; sx <= 1; sx += 2) DrawModelEx(s_column, b + Vector3{ sx * 0.32f * r, r * 0.62f, r * 0.82f }, Vector3{ 0,0,1 }, 45.0f, Vector3{ 0.22f * r, 0.22f * r, 0.12f }, dark);   // eyes
            DrawModelEx(s_column, b + Vector3{ 0, r * 0.36f, r * 0.88f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f * r, 0.18f * r, 0.12f }, dark);   // mouth
        }
    }
    draw_pumpkin_gourd(ctr, 1.8f, pump, pumpDk, stem);

    // corn shocks in rows + round hay bales + a scarecrow
    SetRandomSeed(330);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), r = rand_range(6.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; draw_cornshock(p, rand_range(0.9f, 1.3f), corn, cornDk); }
    SetRandomSeed(551);
    for (int i = 0; i < 6; i++) { float a = rand_range(0, 2 * PI), r = rand_range(8.0f, boundary_radius - 2.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; DrawModelEx(s_cyl, p + Vector3{ 0, 0.9f, 0 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.9f, 1.6f, 0.9f }, hay); }   // round bales
    Vector3 sc = ctr + Vector3{ -10.0f, 0, -6.0f };
    DrawModelEx(s_cyl, sc + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.14f, 3.2f, 0.14f }, cornDk);   // post
    DrawModelEx(s_column, sc + Vector3{ 0, 2.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 0.14f, 0.14f }, cornDk);   // arms
    DrawModelEx(s_dome, sc + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 1.0f, 0.5f }, straw);   // straw body
    draw_pumpkin_gourd(sc + Vector3{ 0, 3.0f, 0 }, 0.6f, pump, pumpDk, stem);   // pumpkin head

    // additive: jack-o'-lantern glow + drifting leaves + warm dusk haze sparkle
    BeginBlendMode(BLEND_ADDITIVE);
    Color jack{ 255, 150, 50, 255 };
    for (size_t i = 0; i < gourds.size(); i++) if (i % 3 == 0) { Vector3 b = Vector3{ gourds[i].x, gourds[i].w * 0.55f, gourds[i].z + gourds[i].w * 0.7f }; DrawSphereEx(b, gourds[i].w * 0.5f * (0.8f + 0.2f * sinf(s_time * 5 + i)), 7, 7, Color{ jack.r, jack.g, jack.b, 110 }); }
    DrawSphereEx(ctr + Vector3{ 0, 1.0f, 1.4f }, 1.0f * (0.8f + 0.2f * sinf(s_time * 4)), 8, 8, Color{ jack.r, jack.g, jack.b, 120 });   // central pumpkin inner glow (suggest carved)
    SetRandomSeed(77);
    for (int i = 0; i < 30; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius), sp = rand_range(0.3f, 0.6f), off = rand_range(0, 1); float y = 8.0f - fmodf(s_time * sp + off, 1.0f) * 8.0f; DrawSphereEx(ctr + Vector3{ bx + sinf(s_time + i) * 0.8f, y, bz }, 0.08f, 4, 4, Color{ 210, 130, 60, 130 }); }   // leaves
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.1f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 150, 60, 45 });
    EndBlendMode();
}

// A bronze bell: a flared skirt (cone, wide rim at the bottom) + a domed crown + a
// canon loop + a clapper, hung from `top` and swinging gently with s_time.
static void draw_bell(Vector3 top, float size, float phase, Color bronze, Color bronzeLt, Color bronzeDk) {
    float sway = 8.0f * sinf(s_time * 1.3f + phase);
    rlPushMatrix();
    rlTranslatef(top.x, top.y, top.z);
    rlRotatef(sway, 0, 0, 1);
    DrawModelEx(s_cone, Vector3{ 0, -size, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ size * 0.72f, size, size * 0.72f }, bronze);          // flared skirt
    DrawModelEx(s_dome, Vector3{ 0, -0.1f * size, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ size * 0.55f, size * 0.45f, size * 0.55f }, bronzeLt);   // crown
    DrawModelEx(s_cyl, Vector3{ 0, 0.2f * size, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f * size, 0.4f * size, 0.1f * size }, bronzeDk);        // canon loop
    draw_bone_seg(Vector3{ 0, -0.1f * size, 0 }, Vector3{ 0, -0.85f * size, 0 }, 0.06f * size, bronzeDk);                            // clapper rod
    DrawModelEx(s_dome, Vector3{ 0, -0.85f * size, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f * size, 0.18f * size, 0.18f * size }, bronzeDk); // clapper ball
    rlPopMatrix();
}

// A timber bell-frame hung with three bells of varying size, each with a rope tail.
static void draw_bellframe(Vector3 base, float yaw, Color wood, Color woodDk, Color bronze, Color bronzeLt, Color bronzeDk) {
    float yr = yaw * DEG2RAD;
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) { Vector3 o = rotate_y(Vector3{ sx * 2.6f, 0, sz * 0.9f }, yr); DrawModelEx(s_cyl, base + o + Vector3{ 0, 2.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, 5.0f, 0.22f }, woodDk); }
    for (int sz = -1; sz <= 1; sz += 2) draw_bone_seg(base + rotate_y(Vector3{ -2.6f, 5.0f, sz * 0.9f }, yr), base + rotate_y(Vector3{ 2.6f, 5.0f, sz * 0.9f }, yr), 0.18f, wood);
    float xs[3] = { -1.7f, 0.0f, 1.7f }, ss[3] = { 1.0f, 1.4f, 1.0f };
    for (int i = 0; i < 3; i++) { Vector3 hp = base + rotate_y(Vector3{ xs[i], 4.8f, 0 }, yr); draw_bell(hp, ss[i], xs[i] * 2.0f + yaw, bronze, bronzeLt, bronzeDk); draw_bone_seg(hp + Vector3{ 0, -ss[i] * 1.2f, 0 }, base + rotate_y(Vector3{ xs[i], 0.5f, 0 }, yr), 0.03f, woodDk); }
}

// Fixed bell-frame layout, shared by build_bell (obstacles) and draw_bellyard.
// xyz = base, w = yaw degrees.
static void bell_layout(std::vector<Vector4>& frames) {
    Vector3 c = boundary_center;
    frames.clear();
    frames.push_back({ c.x - 11.0f, c.y, c.z - 5.0f, 0.0f });
    frames.push_back({ c.x + 11.0f, c.y, c.z - 5.0f, 0.0f });
    frames.push_back({ c.x - 9.0f, c.y, c.z + 7.0f, 90.0f });
    frames.push_back({ c.x + 9.0f, c.y, c.z + 7.0f, 90.0f });
}

static void build_bell() {
    s_wisps.clear();
    std::vector<Vector4> frames;
    bell_layout(frames);
    for (auto& f : frames) obstacles.push_back({ Vector3{ f.x, 0, f.z }, 2.8f });
    Vector3 ctr = boundary_center;
    // warm bronze glow at the central bell + each frame
    s_wisps.push_back(ctr + Vector3{ 0, 3.0f, 0 });
    for (auto& f : frames) s_wisps.push_back(Vector3{ f.x, 4.0f, f.z });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 4.5f }); }
}

static void draw_bellyard() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color floor{ 110,100,84,255 }, floorDk{ 86,78,64,255 }, stone{ 150,144,132,255 }, wood{ 116,88,58,255 }, woodDk{ 86,64,44,255 };
    Color bronze{ 178,128,70,255 }, bronzeLt{ 206,156,96,255 }, bronzeDk{ 130,92,52,255 };

    // stone yard + scuffed patches
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, floor);
    SetRandomSeed(1900);
    for (int i = 0; i < 12; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_column, p + Vector3{ 0, -0.46f, 0 }, Vector3{ 0,1,0 }, (float)(i * 24), Vector3{ rand_range(1.6f,3.0f), 1.0f, rand_range(1.6f,3.0f) }, floorDk); }

    // central great bell on a heavy frame
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) DrawModelEx(s_cyl, ctr + Vector3{ sx * 2.0f, 3.5f, sz * 2.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 7.0f, 0.4f }, woodDk);
    for (int sz = -1; sz <= 1; sz += 2) draw_bone_seg(ctr + Vector3{ -2.0f, 7.0f, sz * 2.0f }, ctr + Vector3{ 2.0f, 7.0f, sz * 2.0f }, 0.3f, wood);
    draw_bell(ctr + Vector3{ 0, 6.6f, 0 }, 3.0f, 0.0f, bronze, bronzeLt, bronzeDk);

    // the bell-frames hung with bells
    std::vector<Vector4> frames;
    bell_layout(frames);
    for (auto& f : frames) draw_bellframe(Vector3{ f.x, 0, f.z }, f.w, wood, woodDk, bronze, bronzeLt, bronzeDk);

    // a couple of cracked, fallen bells lying in the yard
    SetRandomSeed(771);
    for (int i = 0; i < 3; i++) { float a = rand_range(0, 2 * PI), r = rand_range(7.0f, boundary_radius - 3.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; DrawModelEx(s_cone, p + Vector3{ 0, 0.8f, 0 }, Vector3{ 1,0,0 }, 80.0f, Vector3{ 1.0f, 1.4f, 1.0f }, bronzeDk); DrawModelEx(s_dome, p + Vector3{ 0, 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 0.4f, 0.8f }, bronze); }

    // additive: warm bronze glow + expanding sound-ring shimmers off the great bell
    BeginBlendMode(BLEND_ADDITIVE);
    for (int r = 0; r < 3; r++) { float tt = fmodf(s_time * 0.5f + r * 0.33f, 1.0f); float rr = 1.0f + tt * 10.0f; int n = 20; for (int k = 0; k < n; k++) { float a = (k / (float)n) * 2 * PI; DrawSphereEx(ctr + Vector3{ cosf(a) * rr, 4.0f, sinf(a) * rr }, 0.12f, 4, 4, Color{ 220, 180, 120, (unsigned char)(50 * (1.0f - tt)) }); } }   // sound rings
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.12f * sinf(s_time * 1.6f + i), 8, 8, Color{ 255, 180, 90, 50 });
    EndBlendMode();
}

// Seeded aviary contents, shared by build_aviary (obstacles) and draw_aviary.
// xyz = base; w < 0 → a perch-tree of height -w; w > 0 → a nest of scale w.
static void aviary_layout(std::vector<Vector4>& nodes) {
    Vector3 c = boundary_center;
    nodes.clear();
    SetRandomSeed(47119);
    int tries = 0;
    while ((int)nodes.size() < 11 && tries < 400) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(5.0f, 17.0f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 4.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& k : nodes) if (Vector3Distance(p, Vector3{ k.x,0,k.z }) < 4.0f) { clash = true; break; }
        if (clash) continue;
        bool tree = ((int)nodes.size() % 2 == 0);
        nodes.push_back({ p.x, p.y, p.z, tree ? -rand_range(6.0f, 10.0f) : rand_range(1.0f, 1.5f) });
    }
}

// A giant woven nest on a post: a twig bowl + a dark hollow + eggs + a few twig pokes.
static void draw_nest(Vector3 base, float sc, Color wood, Color nest, Color nestDk, Color egg) {
    DrawModelEx(s_cyl, base + Vector3{ 0, 0.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.35f, 1.6f, 0.35f }, wood);                 // support post
    DrawModelEx(s_dome, base + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f * sc, 0.9f * sc, 1.5f * sc }, nest);  // bowl
    DrawModelEx(s_dome, base + Vector3{ 0, 2.05f, 0 }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 1.05f * sc, 0.5f * sc, 1.05f * sc }, nestDk);   // hollow
    SetRandomSeed((int)(base.x * 13 + base.z * 7) + 99);
    for (int k = 0; k < 3; k++) { float a = k * 2.094f; DrawModelEx(s_dome, base + Vector3{ cosf(a) * 0.4f * sc, 1.95f, sinf(a) * 0.4f * sc }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f * sc, 0.4f * sc, 0.3f * sc }, egg); }
    for (int i = 0; i < 6; i++) { float a = i * 1.047f; draw_bone_seg(base + Vector3{ cosf(a) * 1.4f * sc, 1.6f, sinf(a) * 1.4f * sc }, base + Vector3{ cosf(a) * 1.9f * sc, 1.9f, sinf(a) * 1.9f * sc }, 0.05f, nestDk); }
}

static void build_aviary() {
    s_wisps.clear();
    std::vector<Vector4> nodes;
    aviary_layout(nodes);
    for (auto& k : nodes) obstacles.push_back({ Vector3{ k.x, 0, k.z }, k.w < 0 ? 0.8f : 1.4f * k.w });
    Vector3 ctr = boundary_center;
    // warm dusty cage light at the central nest + a few perches
    s_wisps.push_back(ctr + Vector3{ 0, 2.0f, 0 });
    for (size_t i = 0; i < nodes.size(); i += 2) s_wisps.push_back(Vector3{ nodes[i].x, 2.5f, nodes[i].z });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_aviary() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color floor{ 120,114,100,255 }, floorDk{ 96,90,78,255 }, iron{ 66,66,72,255 }, twig{ 110,86,58,255 }, nest{ 96,76,52,255 };
    Color nestDk{ 64,50,34,255 }, egg{ 222,212,192,255 }, wood{ 116,88,58,255 }, bare{ 112,102,88,255 }, bird{ 70,64,58,255 };

    // guano-stained cage floor + straw + feathers
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, floor);
    SetRandomSeed(1900);
    for (int i = 0; i < 14; i++) { float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.44f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.8f,1.8f), 0.08f, rand_range(0.8f,1.8f) }, floorDk); }

    // the great cylindrical iron cage: vertical bars + 3 hoops + a converging dome cap
    const int BARS = 34; float R = 22.0f, H = 16.0f;
    for (int i = 0; i < BARS; i++) { float a = (i / (float)BARS) * 2 * PI; Vector3 p = ctr + Vector3{ cosf(a) * R, 0, sinf(a) * R }; DrawModelEx(s_cyl, p + Vector3{ 0, H * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, H, 0.16f }, iron); }
    for (int h = 0; h < 3; h++) { float y = 3.0f + h * 5.0f; Vector3 pv = ctr + Vector3{ R, y, 0 }; for (int s = 1; s <= 36; s++) { float b = (s / 36.0f) * 2 * PI; Vector3 q = ctr + Vector3{ cosf(b) * R, y, sinf(b) * R }; draw_bone_seg(pv, q, 0.1f, iron); pv = q; } }
    for (int m = 0; m < 17; m++) { float a = (m / 17.0f) * 2 * PI; Vector3 prev = ctr + Vector3{ cosf(a) * R, H, sinf(a) * R }; for (int t = 1; t <= 4; t++) { float u = t / 4.0f; Vector3 p = ctr + Vector3{ cosf(a) * R * (1.0f - u), H + u * 7.0f - u * u * 2.0f, sinf(a) * R * (1.0f - u) }; draw_bone_seg(prev, p, 0.12f, iron); prev = p; } }
    DrawModelEx(s_cyl, ctr + Vector3{ 0, H + 5.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.4f, 0.3f }, iron);   // apex finial
    DrawModelEx(s_dome, ctr + Vector3{ 0, H + 6.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.5f, 0.5f }, iron);

    // central great nest + the perch-trees and nests
    draw_nest(ctr, 1.8f, wood, nest, nestDk, egg);
    std::vector<Vector4> nodes;
    aviary_layout(nodes);
    for (size_t i = 0; i < nodes.size(); i++) { Vector3 b = Vector3{ nodes[i].x, 0, nodes[i].z }; if (nodes[i].w < 0) draw_ptree(b, -nodes[i].w, 0.45f, 500 + (int)i * 19, bare); else draw_nest(b, nodes[i].w, wood, nest, nestDk, egg); }

    // hanging seed-feeders on chains from the lower hoop
    for (int i = 0; i < 6; i++) { float a = (i / 6.0f) * 2 * PI; Vector3 hp = ctr + Vector3{ cosf(a) * (R * 0.6f), 8.0f, sinf(a) * (R * 0.6f) }; draw_bone_seg(hp, hp + Vector3{ 0, -1.6f, 0 }, 0.04f, iron); DrawModelEx(s_cyl, hp + Vector3{ 0, -2.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.8f, 0.4f }, wood); }

    // perched birds (small dark forms) on the middle hoop
    SetRandomSeed(330);
    for (int i = 0; i < 10; i++) { float a = rand_range(0, 2 * PI); Vector3 p = ctr + Vector3{ cosf(a) * R, 8.0f + 0.1f, sinf(a) * R }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.4f, 0.5f }, bird); }

    // additive: wheeling pale birds + dust shafts + warm cage glow
    BeginBlendMode(BLEND_ADDITIVE);
    for (int k = 0; k < 14; k++) { float ph = s_time * 0.9f + k * 0.45f; DrawSphereEx(ctr + Vector3{ sinf(ph) * 12.0f, 9.0f + 3.0f * sinf(ph * 0.6f + k), cosf(ph) * 12.0f }, 0.16f, 4, 4, Color{ 220, 216, 206, 150 }); }
    for (int i = 0; i < 5; i++) { float a = i * 1.2566f; DrawCube(ctr + Vector3{ sinf(a) * 8.0f, 9.0f, cosf(a) * 8.0f }, 1.4f, 16.0f, 1.4f, Color{ 200, 190, 160, 12 }); }
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.1f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 220, 160, 40 });
    EndBlendMode();
}

// An orb web in a vertical plane (yawed about Y): radial strands from the hub + four
// concentric rings, drawn as thin lit silk cylinders.
static void draw_web(Vector3 center, float R, float yawDeg, Color silk, Color silkDk) {
    float yr = yawDeg * DEG2RAD;
    auto W = [&](float lx, float ly) { return center + rotate_y(Vector3{ lx, 0, 0 }, yr) + Vector3{ 0, ly, 0 }; };
    const int sp = 12;
    for (int i = 0; i < sp; i++) { float a = i * (2 * PI / sp); draw_bone_seg(center, W(cosf(a) * R, sinf(a) * R), 0.03f, silkDk); }
    for (int ring = 1; ring <= 4; ring++) { float rr = R * ring / 4.0f; Vector3 pv = W(rr, 0); for (int s = 1; s <= sp; s++) { float a = s * (2 * PI / sp); Vector3 p = W(cosf(a) * rr, sinf(a) * rr); draw_bone_seg(pv, p, 0.02f, silk); pv = p; } }
}

// A silk-wrapped cocoon hanging on a thread.
static void draw_cocoon(Vector3 top, float len, Color silk, Color silkDk) {
    draw_bone_seg(top, top + Vector3{ 0, -0.6f, 0 }, 0.03f, silkDk);
    DrawModelEx(s_cyl, top + Vector3{ 0, -0.6f - len * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.36f, len, 0.36f }, silk);
    DrawModelEx(s_dome, top + Vector3{ 0, -0.6f - len, 0 }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 0.36f, 0.3f, 0.36f }, silk);
    for (int k = 0; k < 3; k++) DrawModelEx(s_cyl, top + Vector3{ 0, -0.85f - k * len * 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.42f, 0.06f, 0.42f }, silkDk);
}

// Seeded rock-spire layout (web anchors), shared by build_web (obstacles) and
// draw_web_lair.
static void web_layout(std::vector<Vector3>& spires) {
    Vector3 c = boundary_center;
    spires.clear();
    SetRandomSeed(56119);
    int tries = 0;
    while ((int)spires.size() < 8 && tries < 400) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(7.0f, boundary_radius - 2.0f);
        Vector3 p = c + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (Vector3Distance(p, c) < 6.0f) continue;
        if (p.z > c.z + 13.0f) continue;
        bool clash = false;
        for (auto& k : spires) if (Vector3Distance(p, k) < 6.0f) { clash = true; break; }
        if (clash) continue;
        spires.push_back(p);
    }
}

static void build_web() {
    s_wisps.clear();
    std::vector<Vector3> spires;
    web_layout(spires);
    for (auto& s : spires) obstacles.push_back({ s, 1.4f });
    Vector3 ctr = boundary_center;
    // pale silk glow at the central web + a few web hubs
    s_wisps.push_back(ctr + Vector3{ 0, 4.0f, 0 });
    for (size_t i = 0; i < spires.size(); i += 2) s_wisps.push_back(spires[i] + Vector3{ 0, 4.0f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 4.5f }); }
}

static void draw_web_lair() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color floor{ 52,50,52,255 }, floorDk{ 40,38,40,255 }, rock{ 76,72,72,255 }, rockDk{ 56,52,52,255 };
    Color silk{ 206,210,202,255 }, silkDk{ 150,156,148,255 }, sac{ 200,206,178,255 }, cocoon{ 184,188,170,255 };

    // dark cave floor + silk-strewn patches + dark rubble
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.1f, 1.0f, boundary_radius * 2.1f }, floor);
    SetRandomSeed(1900);
    for (int i = 0; i < 12; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.0f,2.2f), rand_range(0.3f,0.7f), rand_range(1.0f,2.2f) }, (i % 2) ? rock : floorDk); }

    // rock spires (web anchors) + a central hub spire
    std::vector<Vector3> spires;
    web_layout(spires);
    DrawModelEx(s_cone, ctr, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 9.0f, 1.4f }, rock);   // central hub spire
    for (auto& s : spires) { DrawModelEx(s_cone, s, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, rand_range(6.0f,9.0f), 1.2f }, (Vector3Distance(s, ctr) > 12 ? rockDk : rock)); }

    // orb-webs strung between consecutive spires + from the centre to a couple spires
    for (size_t i = 0; i < spires.size(); i++) {
        Vector3 a = spires[i], b = spires[(i + 1) % spires.size()];
        Vector3 mid = Vector3Scale(Vector3Add(a, b), 0.5f); mid.y = 5.5f;
        float dist = Vector3Distance(Vector3{ a.x,0,a.z }, Vector3{ b.x,0,b.z }); float R = dist * 0.5f; if (R > 5.5f) R = 5.5f; if (R < 2.5f) R = 2.5f;
        float yaw = atan2f(b.x - a.x, b.z - a.z) * RAD2DEG;
        draw_web(mid, R, yaw, silk, silkDk);
        if (i < 3) { Vector3 d = spires[i]; Vector3 m2 = Vector3Scale(Vector3Add(ctr, d), 0.5f); m2.y = 6.0f; float yaw2 = atan2f(d.x - ctr.x, d.z - ctr.z) * RAD2DEG; draw_web(m2, 5.0f, yaw2, silk, silkDk); }
    }
    // the central great web facing the entrance
    draw_web(ctr + Vector3{ 0, 6.0f, 0 }, 6.0f, 0.0f, silk, silkDk);

    // egg sacs clustered at some web hubs + hanging cocoons
    SetRandomSeed(330);
    for (size_t i = 0; i < spires.size(); i += 2) { Vector3 b = spires[i] + Vector3{ 0, 5.0f, 0 }; for (int k = 0; k < 4; k++) DrawModelEx(s_dome, b + Vector3{ rand_range(-0.6f,0.6f), rand_range(-0.4f,0.4f), rand_range(-0.6f,0.6f) }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.3f,0.6f), rand_range(0.3f,0.6f), rand_range(0.3f,0.6f) }, sac); }
    SetRandomSeed(771);
    for (int i = 0; i < 6; i++) { float a = rand_range(0, 2 * PI), r = rand_range(5.0f, boundary_radius - 3.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, rand_range(5.0f,8.0f), cosf(a) * r }; if (p.z > ctr.z + 12.0f) continue; draw_cocoon(p, rand_range(1.4f, 2.6f), cocoon, silkDk); }

    // additive: pale silk sheen at web hubs + drifting motes
    BeginBlendMode(BLEND_ADDITIVE);
    DrawSphereEx(ctr + Vector3{ 0, 6.0f, 0 }, 1.2f, 8, 8, Color{ silk.r, silk.g, silk.b, 30 });
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.12f * sinf(s_time * 1.4f + i), 8, 8, Color{ 180, 210, 180, 45 });
    SetRandomSeed(404);
    for (int i = 0; i < 24; i++) { float a = rand_range(0, 2 * PI), r = rand_range(3.0f, boundary_radius - 2.0f); DrawSphereEx(ctr + Vector3{ sinf(a) * r + sinf(s_time * 0.5f + i) * 0.5f, 1.0f + 4.0f * fabsf(sinf(s_time * 0.3f + i)), cosf(a) * r }, 0.06f, 4, 4, Color{ 200, 215, 195, 80 }); }
    EndBlendMode();
}

// A lily pad floating on the pond, optionally bearing a pink lotus bloom (a bud +
// a ring of flared petals).
static void draw_lotus(Vector3 base, float r, bool flower, Color pad, Color padDk, Color lotus, Color lotusLt) {
    DrawModelEx(s_cyl, base + Vector3{ 0, 0.07f, 0 }, Vector3{ 0,1,0 }, (float)((int)(base.x * 7) % 90), Vector3{ r, 0.05f, r }, ((int)(base.z * 5) & 1) ? pad : padDk);
    if (flower) {
        DrawModelEx(s_dome, base + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.28f, 0.3f, 0.28f }, lotusLt);
        for (int k = 0; k < 6; k++) { float a = k * 1.047f; DrawModelEx(s_cone, base + Vector3{ cosf(a) * 0.22f, 0.18f, sinf(a) * 0.22f }, Vector3{ sinf(a), 0, -cosf(a) }, 40.0f, Vector3{ 0.12f, 0.45f, 0.12f }, (k % 2) ? lotus : lotusLt); }
    }
}

// An open water-pavilion on stilts with a two-tier flared pagoda roof and eave
// lanterns.
static void draw_pavilion(Vector3 base, Color wood, Color woodDk, Color roof, Color gold, Color lamp) {
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) DrawModelEx(s_cyl, base + Vector3{ sx * 2.5f, 0.5f, sz * 2.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.24f, 1.0f, 0.24f }, woodDk);   // stilts
    DrawModelEx(s_column, base + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.0f, 0.3f, 6.0f }, wood);   // deck
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) DrawModelEx(s_cyl, base + Vector3{ sx * 2.4f, 2.6f, sz * 2.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 3.2f, 0.2f }, wood);          // roof posts
    DrawModelEx(s_cone, base + Vector3{ 0, 4.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.2f, 1.3f, 5.2f }, roof);     // roof tier 1
    DrawModelEx(s_cone, base + Vector3{ 0, 5.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 1.2f, 3.4f }, roof);     // roof tier 2
    DrawModelEx(s_cyl, base + Vector3{ 0, 6.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.8f, 0.2f }, gold);      // finial
    DrawModelEx(s_dome, base + Vector3{ 0, 6.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.3f, 0.3f }, gold);
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) DrawModelEx(s_dome, base + Vector3{ sx * 2.4f, 3.6f, sz * 2.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.4f, 0.3f }, lamp);          // eave lanterns
}

// Seeded lily-pad layout for the pond, shared by build_lotus and draw_lotus_pond.
// xyz = centre (at water level), w = radius.
static void lotus_layout(std::vector<Vector4>& pads) {
    Vector3 c = boundary_center;
    pads.clear();
    SetRandomSeed(57219);
    for (int i = 0; i < 26; i++) { float a = rand_range(0, 2 * PI), r = rand_range(4.0f, boundary_radius - 1.5f); pads.push_back({ c.x + sinf(a) * r, 0.0f, c.z + cosf(a) * r, rand_range(0.8f, 1.8f) }); }
}

static void build_lotus() {
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    Vector3 stones[4] = { ctr + Vector3{ -8.0f, 0, 6.0f }, ctr + Vector3{ 8.0f, 0, 6.0f }, ctr + Vector3{ -7.0f, 0, -8.0f }, ctr + Vector3{ 7.0f, 0, -8.0f } };
    for (auto& s : stones) obstacles.push_back({ s, 1.2f });
    // warm lantern glow at the pavilion + stones
    s_wisps.push_back(ctr + Vector3{ 0, 3.6f, 0 });
    for (auto& s : stones) s_wisps.push_back(s + Vector3{ 0, 1.0f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 4.5f }); }
}

static void draw_lotus_pond() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color pad{ 86,140,76,255 }, padDk{ 66,116,60,255 }, lotus{ 228,150,178,255 }, lotusLt{ 246,196,210,255 };
    Color wood{ 120,92,60,255 }, woodDk{ 88,66,46,255 }, roof{ 150,86,72,255 }, gold{ 206,170,100,255 }, stone{ 140,134,124,255 }, reed{ 96,128,70,255 }, lamp{ 255,205,140,255 };

    // mud banks at the rim (no ground slab — the water plane is the pond)
    SetRandomSeed(2207);
    for (int i = 0; i < 10; i++) { float a = rand_range(0, 2 * PI), r = rand_range(boundary_radius - 4.0f, boundary_radius - 0.5f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p + Vector3{ 0, -0.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.4f,2.6f), 0.3f, rand_range(1.4f,2.6f) }, padDk); }

    // lily pads + blooms
    std::vector<Vector4> pads;
    lotus_layout(pads);
    for (size_t i = 0; i < pads.size(); i++) draw_lotus(Vector3{ pads[i].x, 0, pads[i].z }, pads[i].w, (i % 4 == 0), pad, padDk, lotus, lotusLt);

    // central water-pavilion
    draw_pavilion(ctr, wood, woodDk, roof, gold, lamp);

    // stepping stones + zigzag plank bridges to the pavilion
    Vector3 stones[4] = { ctr + Vector3{ -8.0f, 0, 6.0f }, ctr + Vector3{ 8.0f, 0, 6.0f }, ctr + Vector3{ -7.0f, 0, -8.0f }, ctr + Vector3{ 7.0f, 0, -8.0f } };
    for (auto& s : stones) { DrawModelEx(s_cyl, s + Vector3{ 0, 0.15f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 0.3f, 1.2f }, stone); }
    auto walk = [&](Vector3 a, Vector3 b) { Vector3 d = Vector3Subtract(b, a); float len = sqrtf(d.x * d.x + d.z * d.z); float yaw = atan2f(d.x, d.z) * RAD2DEG; Vector3 mid = Vector3Scale(Vector3Add(a, b), 0.5f); mid.y = 0.5f; DrawModelEx(s_column, mid, Vector3{ 0,1,0 }, yaw, Vector3{ 1.1f, 0.14f, len }, wood); for (int k = 0; k < (int)(len / 2.5f); k++) { Vector3 pp = Vector3Lerp(a, b, (k + 0.5f) / (len / 2.5f)); DrawModelEx(s_cyl, Vector3{ pp.x, 0, pp.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.14f, 0.5f, 0.14f }, woodDk); } };
    Vector3 deckEdge = ctr + Vector3{ 0, 0, 3.0f };
    for (auto& s : stones) walk(s, deckEdge);

    // reed clumps along the banks
    SetRandomSeed(990);
    for (int i = 0; i < 10; i++) { float a = rand_range(0, 2 * PI), r = rand_range(boundary_radius - 4.0f, boundary_radius - 1.0f); Vector3 c = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; for (int k = 0; k < 5; k++) { Vector3 p = c + Vector3{ rand_range(-0.6f,0.6f), 0, rand_range(-0.6f,0.6f) }; draw_bone_seg(p, p + Vector3{ rand_range(-0.2f,0.2f), rand_range(1.2f,2.0f), rand_range(-0.2f,0.2f) }, 0.05f, reed); } }

    // additive: warm lanterns + darting koi + drifting petals + water sparkle
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.1f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 205, 150, 60 });
    for (int k = 0; k < 10; k++) { float ph = s_time * 1.0f + k * 0.62f; DrawSphereEx(ctr + Vector3{ sinf(ph) * 10.0f, 0.15f, cosf(ph) * 10.0f }, 0.18f, 5, 5, Color{ 245, 150, 70, 120 }); }   // koi
    SetRandomSeed(77);
    for (int i = 0; i < 20; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius), sp = rand_range(0.3f, 0.6f), off = rand_range(0, 1); float y = 6.0f - fmodf(s_time * sp + off, 1.0f) * 6.0f; DrawSphereEx(ctr + Vector3{ bx + sinf(s_time + i) * 0.6f, y, bz }, 0.07f, 4, 4, Color{ 240, 180, 200, 110 }); }   // petals
    EndBlendMode();
}

// ---- The Archtree: a single colossal sacred tree dominating the arena ----
// satellite saplings: xyz + w = height. Shared by build (obstacles) + draw (geometry)
// so the two never drift (no new global struct/vector needed).
static void archtree_layout(std::vector<Vector4>& trees) {
    Vector3 ctr = boundary_center;
    SetRandomSeed(5813);
    int tries = 0;
    while ((int)trees.size() < 7 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rad = rand_range(9.0f, boundary_radius - 2.5f);
        Vector3 p = ctr + Vector3{ sinf(a) * rad, 0, cosf(a) * rad };
        if (p.z > ctr.z + 12.0f) continue;                      // keep the player spawn aisle clear
        bool ok = true;
        for (auto& t : trees) if (Vector3Distance(p, Vector3{ t.x, 0, t.z }) < 4.6f) { ok = false; break; }
        if (!ok) continue;
        trees.push_back(Vector4{ p.x, p.y, p.z, rand_range(5.0f, 8.0f) });
    }
}

static void build_archtree() {
    s_wisps.clear();
    Vector3 ctr = boundary_center;
    // six splayed root buttresses become obstacles the player weaves between
    for (int i = 0; i < 6; i++) {
        float a = (float)i / 6 * 2 * PI + 0.4f;
        obstacles.push_back({ ctr + Vector3{ sinf(a) * 5.0f, 0, cosf(a) * 5.0f }, 1.0f });
    }
    // satellite saplings are obstacles too
    std::vector<Vector4> trees; archtree_layout(trees);
    for (auto& t : trees) obstacles.push_back({ Vector3{ t.x, 0, t.z }, 0.9f });
    // glowing sap-light: high in the canopy + scattered around the trunk hollows
    s_wisps.push_back(ctr + Vector3{ 0, 16.0f, 0 });
    SetRandomSeed(771);
    for (int i = 0; i < 7; i++) { float a = rand_range(0, 2 * PI), r = rand_range(2.0f, 6.0f); s_wisps.push_back(ctr + Vector3{ sinf(a) * r, rand_range(1.0f, 4.0f), cosf(a) * r }); }
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 6.0f }); }
}

static void draw_archtree() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 ctr = boundary_center;
    Color bark{ 96,72,52,255 }, barkDk{ 70,52,38,255 }, leaf{ 86,138,72,255 }, leafLt{ 120,170,96,255 }, leafDk{ 60,104,56,255 };
    Color moss{ 70,96,58,255 }, mossDk{ 54,76,46,255 }, shroom{ 206,170,120,255 };

    // mossy forest floor (dry — the water plane is suppressed for this level)
    DrawModelEx(s_column, ctr + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, moss);
    SetRandomSeed(3322);
    for (int i = 0; i < 26; i++) { float a = rand_range(0, 2 * PI), r = rand_range(2.0f, boundary_radius - 1.0f); Vector3 p = ctr + Vector3{ sinf(a) * r, 0, cosf(a) * r }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.8f,2.0f), rand_range(0.2f,0.5f), rand_range(0.8f,2.0f) }, (i & 1) ? mossDk : moss); }

    // --- the colossal central trunk: stacked tapering bone-segments, leaning slightly ---
    const int TS = 9; float trunkH = 20.0f;
    for (int i = 0; i < TS; i++) {
        float t0 = i / (float)TS, t1 = (i + 1) / (float)TS;
        float r0 = 3.4f * (1 - 0.62f * t0) + 0.4f, r1 = 3.4f * (1 - 0.62f * t1) + 0.4f;
        Vector3 a = ctr + Vector3{ sinf(t0 * 2.2f) * 0.6f, trunkH * t0, cosf(t0 * 1.4f) * 0.3f };
        Vector3 b = ctr + Vector3{ sinf(t1 * 2.2f) * 0.6f, trunkH * t1, cosf(t1 * 1.4f) * 0.3f };
        draw_bone_seg(a, b, (r0 + r1) * 0.5f, (i & 1) ? barkDk : bark);
    }
    Vector3 crown = ctr + Vector3{ sinf(2.2f) * 0.6f, trunkH, cosf(1.4f) * 0.3f };

    // --- root buttresses sweeping out and down into the ground ---
    for (int i = 0; i < 6; i++) {
        float a = (float)i / 6 * 2 * PI + 0.4f;
        Vector3 hip = ctr + Vector3{ sinf(a) * 1.6f, 3.0f, cosf(a) * 1.6f };
        Vector3 mid = ctr + Vector3{ sinf(a) * 3.6f, 1.0f, cosf(a) * 3.6f };
        Vector3 tip = ctr + Vector3{ sinf(a) * 5.4f, 0.0f, cosf(a) * 5.4f };
        draw_bone_seg(hip, mid, 0.9f, bark);
        draw_bone_seg(mid, tip, 0.55f, barkDk);
    }

    // --- canopy limbs radiating from the crown, each tipped with foliage masses + a hanging vine ---
    SetRandomSeed(606);
    for (int i = 0; i < 8; i++) {
        float a = (float)i / 8 * 2 * PI + 0.2f;
        Vector3 base = crown + Vector3{ 0, rand_range(-2.0f, 0.0f), 0 };
        Vector3 elb = crown + Vector3{ sinf(a) * 4.0f, rand_range(1.5f, 3.0f), cosf(a) * 4.0f };
        Vector3 end = crown + Vector3{ sinf(a) * 7.5f, rand_range(2.5f, 4.5f), cosf(a) * 7.5f };
        draw_bone_seg(base, elb, 0.8f, bark);
        draw_bone_seg(elb, end, 0.4f, barkDk);
        for (int k = 0; k < 3; k++) { Vector3 fp = end + Vector3{ rand_range(-1.6f,1.6f), rand_range(-0.5f,1.6f), rand_range(-1.6f,1.6f) }; float fs = rand_range(2.0f, 3.4f); DrawModelEx(s_dome, fp, Vector3{ 0,1,0 }, 0, Vector3{ fs, fs * 0.7f, fs }, (k & 1) ? leafDk : leaf); }
        draw_bone_seg(end, end + Vector3{ rand_range(-0.5f,0.5f), -rand_range(3.0f,6.0f), rand_range(-0.5f,0.5f) }, 0.06f, leafDk);
    }
    // a broad crown cap of overlapping foliage domes
    for (int k = 0; k < 5; k++) { Vector3 cp = crown + Vector3{ rand_range(-3.0f,3.0f), rand_range(2.0f,5.0f), rand_range(-3.0f,3.0f) }; float cs = rand_range(3.0f, 4.6f); DrawModelEx(s_dome, cp, Vector3{ 0,1,0 }, 0, Vector3{ cs, cs * 0.7f, cs }, (k & 1) ? leaf : leafLt); }

    // --- bracket mushrooms climbing the trunk ---
    SetRandomSeed(414);
    for (int i = 0; i < 7; i++) { float a = rand_range(0, 2 * PI), hh = rand_range(1.5f, 10.0f); float rr = 3.0f * (1 - 0.5f * (hh / trunkH)); Vector3 p = ctr + Vector3{ sinf(a) * rr, hh, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, a * RAD2DEG, Vector3{ rand_range(0.8f,1.4f), 0.3f, rand_range(0.8f,1.4f) }, shroom); }

    // --- satellite saplings (reuse draw_ptree + foliage domes) ---
    std::vector<Vector4> trees; archtree_layout(trees);
    for (size_t i = 0; i < trees.size(); i++) {
        Vector3 b{ trees[i].x, 0, trees[i].z };
        draw_ptree(b, trees[i].w, 0.5f, 920 + (int)i * 37, bark);
        SetRandomSeed(640 + (int)i * 11);
        for (int k = 0; k < 3; k++) { Vector3 fp = b + Vector3{ rand_range(-1.4f,1.4f), trees[i].w * rand_range(0.7f,1.0f), rand_range(-1.4f,1.4f) }; DrawModelEx(s_dome, fp, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.4f,2.2f), rand_range(1.0f,1.6f), rand_range(1.4f,2.2f) }, (k & 1) ? leafDk : leaf); }
    }

    // additive: glowing sap motes + fireflies + falling leaves
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.35f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 230, 220, 140, 70 });
    SetRandomSeed(150);
    for (int i = 0; i < 26; i++) { float a = rand_range(0, 2 * PI), r = rand_range(2.0f, boundary_radius), yb = rand_range(1.0f, 8.0f); DrawSphereEx(ctr + Vector3{ sinf(a) * r + sinf(s_time * 1.2f + i) * 0.5f, yb + sinf(s_time * 0.8f + i) * 0.6f, cosf(a) * r }, 0.05f, 4, 4, Color{ 200, 240, 150, 130 }); }   // fireflies
    SetRandomSeed(88);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius), sp = rand_range(0.2f, 0.45f), off = rand_range(0, 1); float y = 12.0f - fmodf(s_time * sp + off, 1.0f) * 12.0f; DrawSphereEx(ctr + Vector3{ bx + sinf(s_time * 0.6f + i) * 1.0f, y, bz }, 0.08f, 4, 4, Color{ 150, 200, 110, 110 }); }   // falling leaves
    EndBlendMode();
}

// ---- The Gambit: a colossal marble chessboard with towering stone chess pieces ----
// one stylised piece (cyl bodies + distinct tops) at base, scaled by s, kind 0..5 =
// pawn / rook / bishop / knight / queen / king. Reuses the lit s_cyl/s_dome/s_cone/s_column.
static void draw_chesspiece(Vector3 base, float s, int kind, Color st, Color sd) {
    // stacked foot
    DrawModelEx(s_cyl, base, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f * s, 0.35f * s, 1.1f * s }, sd);
    DrawModelEx(s_cyl, base + Vector3{ 0, 0.35f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.85f * s, 0.25f * s, 0.85f * s }, st);
    Vector3 b = base + Vector3{ 0, 0.6f * s, 0 };
    auto ball = [&](Vector3 cen, float rr, Color col) { DrawModelEx(s_dome, cen, Vector3{ 0,1,0 }, 0, Vector3{ rr,rr,rr }, col); DrawModelEx(s_dome, cen, Vector3{ 1,0,0 }, 180, Vector3{ rr,rr,rr }, col); };
    if (kind == 0) {            // pawn: short body + ball head
        DrawModelEx(s_cyl, b, Vector3{ 0,1,0 }, 0, Vector3{ 0.55f * s, 0.9f * s, 0.55f * s }, st);
        ball(b + Vector3{ 0, 1.05f * s, 0 }, 0.42f * s, st);
    } else if (kind == 1) {     // rook: tower + crenellations
        DrawModelEx(s_cyl, b, Vector3{ 0,1,0 }, 0, Vector3{ 0.78f * s, 1.9f * s, 0.78f * s }, st);
        DrawModelEx(s_cyl, b + Vector3{ 0, 1.9f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.95f * s, 0.3f * s, 0.95f * s }, st);
        for (int k = 0; k < 6; k++) { float a = k / 6.0f * 2 * PI; DrawModelEx(s_column, b + Vector3{ sinf(a) * 0.7f * s, 2.3f * s, cosf(a) * 0.7f * s }, Vector3{ 0,1,0 }, a * RAD2DEG, Vector3{ 0.28f * s, 0.5f * s, 0.28f * s }, st); }
    } else if (kind == 2) {     // bishop: slim body + mitre head + finial
        DrawModelEx(s_cyl, b, Vector3{ 0,1,0 }, 0, Vector3{ 0.66f * s, 1.9f * s, 0.66f * s }, st);
        DrawModelEx(s_dome, b + Vector3{ 0, 1.9f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f * s, 0.7f * s, 0.5f * s }, st);
        ball(b + Vector3{ 0, 2.75f * s, 0 }, 0.18f * s, st);
    } else if (kind == 3) {     // knight: neck + tilted blocky horse-head
        DrawModelEx(s_cyl, b, Vector3{ 0,1,0 }, 0, Vector3{ 0.72f * s, 1.2f * s, 0.72f * s }, st);
        rlPushMatrix(); rlTranslatef(b.x, b.y + 1.2f * s, b.z); rlRotatef(-22.0f, 1, 0, 0);
        DrawModelEx(s_column, Vector3{ 0, 0.55f * s, 0.15f * s }, Vector3{ 1,0,0 }, 0, Vector3{ 0.7f * s, 1.3f * s, 0.85f * s }, st);
        DrawModelEx(s_column, Vector3{ 0, 0.85f * s, 0.85f * s }, Vector3{ 1,0,0 }, 0, Vector3{ 0.55f * s, 0.55f * s, 0.7f * s }, st);
        rlPopMatrix();
    } else if (kind == 4) {     // queen: tall body + collar + crown of points + orb
        DrawModelEx(s_cyl, b, Vector3{ 0,1,0 }, 0, Vector3{ 0.82f * s, 2.4f * s, 0.82f * s }, st);
        DrawModelEx(s_dome, b + Vector3{ 0, 2.4f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.62f * s, 0.4f * s, 0.62f * s }, st);
        for (int k = 0; k < 8; k++) { float a = k / 8.0f * 2 * PI; DrawModelEx(s_cone, b + Vector3{ sinf(a) * 0.42f * s, 2.7f * s, cosf(a) * 0.42f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.15f * s, 0.45f * s, 0.15f * s }, st); }
        ball(b + Vector3{ 0, 2.95f * s, 0 }, 0.2f * s, st);
    } else {                    // king: tallest body + crown + cross
        DrawModelEx(s_cyl, b, Vector3{ 0,1,0 }, 0, Vector3{ 0.88f * s, 2.7f * s, 0.88f * s }, st);
        DrawModelEx(s_dome, b + Vector3{ 0, 2.7f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f * s, 0.45f * s, 0.6f * s }, st);
        DrawModelEx(s_column, b + Vector3{ 0, 3.4f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f * s, 0.95f * s, 0.16f * s }, st);
        DrawModelEx(s_column, b + Vector3{ 0, 3.6f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.55f * s, 0.16f * s, 0.16f * s }, st);
    }
}

// piece roster: x, y=scale, z, w=kind(0..5). Shared by build (obstacles) + draw (geometry).
static void chess_layout(std::vector<Vector4>& pcs) {
    Vector3 c = boundary_center;
    int rank[8] = { 1, 3, 2, 4, 4, 2, 3, 1 };               // R N B Q Q B N R back rank
    for (int i = 0; i < 8; i++) { float x = (i - 3.5f) * 3.0f; pcs.push_back(Vector4{ c.x + x, 1.4f, c.z - 10.0f, (float)rank[i] }); }
    pcs.push_back(Vector4{ c.x - 12.0f, 2.0f, c.z - 2.0f, 1.0f });   // flanking colossal rooks
    pcs.push_back(Vector4{ c.x + 12.0f, 2.0f, c.z - 2.0f, 1.0f });
    pcs.push_back(Vector4{ c.x - 11.0f, 1.6f, c.z + 4.0f, 2.0f });   // side bishops (off the spawn aisle)
    pcs.push_back(Vector4{ c.x + 11.0f, 1.6f, c.z + 4.0f, 2.0f });
    for (int i = 0; i < 6; i++) { float x = (i - 2.5f) * 3.2f; pcs.push_back(Vector4{ c.x + x, 1.1f, c.z - 15.0f, 0.0f }); }   // pawn line
}

static void build_chess() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    std::vector<Vector4> pcs; chess_layout(pcs);
    for (auto& p : pcs) obstacles.push_back({ Vector3{ p.x, 0, p.z }, 0.8f * p.y });   // standing pieces block
    // royal glow: high over the central king + atop the major pieces
    s_wisps.push_back(c + Vector3{ 0, 8.0f, 0 });
    for (auto& p : pcs) if (p.w >= 4.0f) s_wisps.push_back(Vector3{ p.x, 5.0f * p.y, p.z });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.5f }); }
}

static void draw_chess() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color white{ 228,224,212,255 }, whiteDk{ 196,190,176,255 }, black{ 56,54,62,255 }, blackDk{ 38,36,44,255 };

    // dark base slab, then a checkered marble board on top
    DrawModelEx(s_column, c + Vector3{ 0, -0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.6f, boundary_radius * 2 + 6 }, blackDk);
    const int N = 9; const float T = 3.4f;
    for (int gx = 0; gx < N; gx++) for (int gz = 0; gz < N; gz++) {
        Vector3 t = c + Vector3{ (gx - (N - 1) / 2.0f) * T, 0, (gz - (N - 1) / 2.0f) * T };
        DrawModelEx(s_column, t, Vector3{ 0,1,0 }, 0, Vector3{ T * 0.98f, 0.2f, T * 0.98f }, ((gx + gz) & 1) ? white : black);
    }

    // standing pieces — far army marble, near army basalt
    std::vector<Vector4> pcs; chess_layout(pcs);
    for (auto& p : pcs) { bool far = (p.z < c.z); draw_chesspiece(Vector3{ p.x, 0, p.z }, p.y, (int)p.w, far ? white : black, far ? whiteDk : blackDk); }
    // the colossal central king
    draw_chesspiece(c, 2.0f, 5, white, whiteDk);

    // two toppled pieces lying on the board (captured) — rlgl frame lays them flat
    rlPushMatrix(); rlTranslatef(c.x + 9.0f, 0.6f, c.z - 6.0f); rlRotatef(90.0f, 0, 0, 1); draw_chesspiece(Vector3{ 0,0,0 }, 1.4f, 1, black, blackDk); rlPopMatrix();
    rlPushMatrix(); rlTranslatef(c.x - 8.0f, 0.6f, c.z - 12.0f); rlRotatef(90.0f, 1, 0, 0); draw_chesspiece(Vector3{ 0,0,0 }, 1.4f, 2, white, whiteDk); rlPopMatrix();

    // additive: cool royal glow + drifting dust motes
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 180, 200, 255, 55 });
    SetRandomSeed(303);
    for (int i = 0; i < 28; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius), sp = rand_range(0.1f, 0.3f), off = rand_range(0, 1); float y = fmodf(s_time * sp + off, 1.0f) * 5.0f; DrawSphereEx(c + Vector3{ bx, y, bz }, 0.05f, 4, 4, Color{ 200, 215, 255, 90 }); }
    EndBlendMode();
}

// ---- The Hedge Maze: tall clipped-hedge corridors around an open central clearing ----
// one hedge wall segment centred at c, oriented by yaw(deg), with bushy dome clumps on top.
static void draw_hedge(Vector3 c, float yaw, float length, float height, float thick, Color leaf, Color leafDk) {
    DrawModelEx(s_column, c + Vector3{ 0, height * 0.5f, 0 }, Vector3{ 0,1,0 }, yaw, Vector3{ thick, height, length }, leafDk);
    float yr = yaw * DEG2RAD; Vector3 dir{ sinf(yr), 0, cosf(yr) };
    int clumps = (int)(length / 1.4f); if (clumps < 1) clumps = 1;
    for (int i = 0; i < clumps; i++) { float t = (i + 0.5f) / clumps - 0.5f; Vector3 p = c + Vector3Scale(dir, t * length) + Vector3{ 0, height, 0 }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, yaw, Vector3{ thick * 0.7f + 0.4f, 0.6f, 1.0f }, leaf); }
}

// a clipped topiary: kind 0 = stacked ball-tree, kind 1 = cone spire.
static void draw_topiary(Vector3 base, float s, int kind, Color leaf, Color leafDk) {
    DrawModelEx(s_cyl, base, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f * s, 0.5f * s, 0.5f * s }, leafDk);   // planter
    Vector3 b = base + Vector3{ 0, 0.5f * s, 0 };
    if (kind == 0) {
        DrawModelEx(s_cyl, b, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f * s, 0.6f * s, 0.18f * s }, leafDk);   // trunk
        float yy = 0.6f * s, r = 0.9f * s;
        for (int i = 0; i < 3; i++) { Vector3 cen = b + Vector3{ 0, yy + r, 0 }; DrawModelEx(s_dome, cen, Vector3{ 0,1,0 }, 0, Vector3{ r,r,r }, leaf); DrawModelEx(s_dome, cen, Vector3{ 1,0,0 }, 180, Vector3{ r,r,r }, leaf); yy += 2 * r * 0.82f; r *= 0.72f; }
    } else {
        DrawModelEx(s_cone, b, Vector3{ 0,1,0 }, 0, Vector3{ 0.95f * s, 2.6f * s, 0.95f * s }, leaf);
    }
}

// maze wall roster relative to boundary_center: dx, dz, yaw(deg), length. Cardinal entrance
// lane (front, x~0) and the central clearing are left open so the level stays navigable.
static void maze_layout(std::vector<Vector4>& w) {
    w.push_back(Vector4{ 16, 0, 0, 18 });    w.push_back(Vector4{ -16, 0, 0, 18 });   // long side walls
    w.push_back(Vector4{ 0, -16, 90, 22 });                                           // far back wall
    w.push_back(Vector4{ 8, 9, 90, 8 });     w.push_back(Vector4{ -8, 9, 90, 8 });    // front, flanking the entrance gap
    w.push_back(Vector4{ 9, -7, 0, 8 });     w.push_back(Vector4{ -9, -7, 0, 8 });    // inner corridor walls
    w.push_back(Vector4{ 7, -8, 90, 5 });    w.push_back(Vector4{ -7, -8, 90, 5 });   // back spurs
    w.push_back(Vector4{ 6, 2, 0, 4 });      w.push_back(Vector4{ -6, 2, 0, 4 });     // clearing-edge cover
    w.push_back(Vector4{ 12, -12, 90, 6 });  w.push_back(Vector4{ -12, -12, 90, 6 }); // corner accents
}

static void build_maze() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    std::vector<Vector4> walls; maze_layout(walls);
    for (auto& w : walls) {
        float yr = w.z * DEG2RAD; Vector3 dir{ sinf(yr), 0, cosf(yr) };
        int n = (int)(w.w / 2.5f) + 1;
        for (int i = 0; i < n; i++) { float t = (i + 0.5f) / n - 0.5f; Vector3 p = c + Vector3{ w.x, 0, w.y } + Vector3Scale(dir, t * w.w); obstacles.push_back({ p, 1.3f }); }
    }
    // garden-lamp glow at the 4 junctions + the central topiary
    Vector3 lamps[4] = { c + Vector3{ -7, 0, 7 }, c + Vector3{ 7, 0, 7 }, c + Vector3{ -7, 0, -7 }, c + Vector3{ 7, 0, -7 } };
    for (auto& l : lamps) s_wisps.push_back(l + Vector3{ 1.6f, 2.6f, 0 });
    s_wisps.push_back(c + Vector3{ 0, 3.2f, 0 });
    for (auto& wv : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ wv.x, wv.y, wv.z, 5.0f }); }
}

static void draw_maze() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color leaf{ 72,128,66,255 }, leafDk{ 50,98,50,255 }, grass{ 86,132,74,255 }, gravel{ 156,150,134,255 }, stone{ 150,148,140,255 }, lamp{ 255,212,150,255 };

    // grass floor + a gravel central clearing disc
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, grass);
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.03f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 8.0f, 0.06f, 8.0f }, gravel);

    // hedge walls
    std::vector<Vector4> walls; maze_layout(walls);
    for (auto& w : walls) draw_hedge(c + Vector3{ w.x, 0, w.y }, w.z, w.w, 3.2f, 1.2f, leaf, leafDk);

    // central grand topiary on a stone pedestal
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 0.4f, 2.0f }, stone);
    draw_topiary(c + Vector3{ 0, 0.4f, 0 }, 1.9f, 0, leaf, leafDk);

    // junction topiary + lamp posts
    Vector3 tps[4] = { c + Vector3{ -7, 0, 7 }, c + Vector3{ 7, 0, 7 }, c + Vector3{ -7, 0, -7 }, c + Vector3{ 7, 0, -7 } };
    for (int i = 0; i < 4; i++) {
        draw_topiary(tps[i], 1.2f, 1, leaf, leafDk);
        Vector3 l = tps[i] + Vector3{ 1.6f, 0, 0 };
        DrawModelEx(s_cyl, l, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 2.6f, 0.12f }, stone);
        DrawModelEx(s_dome, l + Vector3{ 0, 2.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.35f, 0.35f, 0.35f }, lamp);
    }

    // additive: warm lamp glow + drifting pollen / butterflies
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 212, 150, 55 });
    SetRandomSeed(515);
    for (int i = 0; i < 24; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); float y = 1.4f + 1.2f * sinf(s_time * 0.8f + i); DrawSphereEx(c + Vector3{ bx + sinf(s_time + i) * 0.8f, y, bz }, 0.06f, 4, 4, Color{ 240, 235, 160, 110 }); }
    EndBlendMode();
}

// ---- The Cascade: a great waterfall plunging off a rugged cliff into a turquoise pool ----
static void build_falls() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    Vector3 rocks[4] = { c + Vector3{ -7,0,-2 }, c + Vector3{ 8,0,-4 }, c + Vector3{ -9,0,4 }, c + Vector3{ 6,0,6 } };
    for (auto& r : rocks) obstacles.push_back({ r, 1.4f });   // midstream boulders (cover)
    float cliffZ = c.z - 16.0f, fallsX[3] = { -9.0f, 0.0f, 9.0f };
    for (int f = 0; f < 3; f++) s_wisps.push_back(c + Vector3{ fallsX[f], 2.0f, cliffZ + 2.4f });   // spray glow at each falls base
    s_wisps.push_back(c + Vector3{ 0, 2.4f, 0 });                                                   // central rock
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_falls() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color rock{ 96,92,86,255 }, rockDk{ 70,68,64,255 }, moss{ 74,104,60,255 }, mossDk{ 58,84,50,255 };
    float cliffZ = c.z - 16.0f;

    // --- rugged cliff backdrop: stacked rock blocks across the back, gently bowed ---
    SetRandomSeed(7000);
    const int cols = 14;
    for (int i = 0; i < cols; i++) {
        float u = i / (float)(cols - 1) - 0.5f;                 // -0.5..0.5
        float fx = u * 34.0f;
        float bow = -3.0f * cosf(u * PI);                       // shallow curve, ends toward the pool
        int stack = 6 + (i % 3);
        for (int k = 0; k < stack; k++) {
            float w = rand_range(2.2f, 3.0f), h = rand_range(2.6f, 3.4f), d = rand_range(2.2f, 3.2f);
            Vector3 p = c + Vector3{ fx + rand_range(-0.5f,0.5f), k * 2.8f + h * 0.5f, cliffZ + bow + rand_range(-0.5f,0.5f) };
            DrawModelEx(s_column, p, Vector3{ 0,1,0 }, rand_range(-8.0f, 8.0f), Vector3{ w, h, d }, ((k + i) & 1) ? rock : rockDk);
            if (k <= 1) DrawModelEx(s_dome, p + Vector3{ 0, h * 0.5f, d * 0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ w * 0.5f, 0.4f, 0.6f }, (k & 1) ? moss : mossDk);
        }
    }

    // --- midstream mossy boulders (cover) + the central great rock ---
    Vector3 rocks[4] = { c + Vector3{ -7,0,-2 }, c + Vector3{ 8,0,-4 }, c + Vector3{ -9,0,4 }, c + Vector3{ 6,0,6 } };
    for (auto& r : rocks) { DrawModelEx(s_dome, r, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f,1.2f,1.6f }, rock); DrawModelEx(s_dome, r + Vector3{ 0,0.4f,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f,0.5f,1.0f }, moss); }
    DrawModelEx(s_dome, c, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 2.0f, 2.4f }, rock);
    DrawModelEx(s_dome, c + Vector3{ 0,0.9f,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 0.7f, 1.6f }, moss);

    // --- waterfall curtains pouring from the cliff top into the pool ---
    float topY = 19.0f, fallsX[3] = { -9.0f, 0.0f, 9.0f };
    BeginBlendMode(BLEND_ALPHA);
    for (int f = 0; f < 3; f++) {
        float fx = fallsX[f];
        for (int s = 0; s < 11; s++) {
            float t = s / 11.0f, y = topY * (1.0f - t);
            float wob = sinf(s_time * 3.0f + s * 0.55f + f) * (0.1f + t * 0.25f);
            DrawCube(c + Vector3{ fx + wob, y, cliffZ + 2.2f }, 3.2f, topY / 11.0f + 0.25f, 0.5f, Color{ 150, 210, 230, 110 });
        }
    }
    EndBlendMode();
    // --- additive: scrolling droplets + churning foam + rising mist + spray glow ---
    BeginBlendMode(BLEND_ADDITIVE);
    SetRandomSeed(31);
    for (int f = 0; f < 3; f++) {
        float fx = fallsX[f];
        for (int s = 0; s < 14; s++) { float off = rand_range(0, 1); float y = topY - fmodf(s_time * (6.0f + 2 * f) + off * topY, topY); DrawSphereEx(c + Vector3{ fx + rand_range(-1.4f,1.4f), y, cliffZ + 2.4f }, 0.12f, 4, 4, Color{ 210, 240, 250, 150 }); }
        for (int s = 0; s < 12; s++) { float a = rand_range(0, 2 * PI), rr = rand_range(0.5f, 2.6f); DrawSphereEx(c + Vector3{ fx + sinf(a) * rr, 0.4f + 0.3f * sinf(s_time * 5 + s), cliffZ + 2.4f + cosf(a) * 1.2f }, 0.3f, 6, 6, Color{ 230, 245, 250, 90 }); }
    }
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.6f + 0.2f * sinf(s_time * 2 + i), 8, 8, Color{ 200, 235, 245, 50 });
    SetRandomSeed(64);
    for (int i = 0; i < 22; i++) { float bx = rand_range(-12, 12), off = rand_range(0, 1); float y = fmodf(s_time * 0.4f + off, 1.0f) * 8.0f; DrawSphereEx(c + Vector3{ bx, y, cliffZ + 3.0f + rand_range(-1.0f,3.0f) }, 0.5f, 6, 6, Color{ 220, 235, 240, 40 }); }   // rising mist
    EndBlendMode();
}

// ---- The Crater: a scorched impact basin with a half-buried glowing fallen star ----
static void build_crater() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    // larger ejecta boulders become obstacles (same seeded sequence as draw_crater)
    SetRandomSeed(8200);
    for (int i = 0; i < 12; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(6.0f, boundary_radius - 5.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; float s = rand_range(0.8f, 1.8f); if (s > 1.3f) obstacles.push_back({ p, s * 0.9f }); }
    // violet star-glow at the centre + fumarole vent glows
    s_wisps.push_back(c + Vector3{ 0, 2.0f, 0 });
    Vector3 vents[4] = { c + Vector3{ -10,0,-6 }, c + Vector3{ 11,0,-4 }, c + Vector3{ -8,0,8 }, c + Vector3{ 9,0,7 } };
    for (auto& v : vents) s_wisps.push_back(v + Vector3{ 0, 1.0f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 6.0f }); }
}

static void draw_crater() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color ground{ 60,54,50,255 }, groundDk{ 44,40,38,255 }, rock{ 86,80,76,255 }, rockDk{ 62,58,54,255 }, meteor{ 42,38,48,255 }, meteorDk{ 28,24,34,255 };

    // scorched crater floor + a darker blast scorch disc
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, ground);
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.02f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 14.0f, 0.05f, 14.0f }, groundDk);

    // raised crater rim: a ring of rock blocks tilted outward
    SetRandomSeed(8100);
    const int rimN = 28;
    for (int i = 0; i < rimN; i++) {
        float a = i / (float)rimN * 2 * PI, rr = boundary_radius - 3.0f, h = rand_range(2.5f, 4.5f);
        Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr };
        rlPushMatrix(); rlTranslatef(p.x, 0, p.z); rlRotatef(a * RAD2DEG, 0, 1, 0); rlRotatef(rand_range(8.0f, 20.0f), 1, 0, 0);
        DrawModelEx(s_column, Vector3{ 0, h * 0.5f, 0 }, Vector3{ 0,1,0 }, rand_range(-10.0f, 10.0f), Vector3{ rand_range(2.5f,4.0f), h, rand_range(2.0f,3.0f) }, (i & 1) ? rock : rockDk);
        rlPopMatrix();
    }

    // radial scorch streaks from the centre
    for (int i = 0; i < 10; i++) { float a = i / 10.0f * 2 * PI; rlPushMatrix(); rlTranslatef(c.x, 0.03f, c.z); rlRotatef(a * RAD2DEG, 0, 1, 0); DrawModelEx(s_column, Vector3{ 0, 0, 6.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 0.04f, 11.0f }, groundDk); rlPopMatrix(); }

    // scattered ejecta boulders
    SetRandomSeed(8200);
    for (int i = 0; i < 12; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(6.0f, boundary_radius - 5.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; float s = rand_range(0.8f, 1.8f); DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ s, s * 0.7f, s }, (i & 1) ? rock : rockDk); }

    // smoking fumarole vents
    Vector3 vents[4] = { c + Vector3{ -10,0,-6 }, c + Vector3{ 11,0,-4 }, c + Vector3{ -8,0,8 }, c + Vector3{ 9,0,7 } };
    for (auto& v : vents) { DrawModelEx(s_dome, v, Vector3{ 0,1,0 }, 0, Vector3{ 1.8f, 0.8f, 1.8f }, rockDk); DrawModelEx(s_cyl, v + Vector3{ 0,0.4f,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.5f, 0.5f }, meteorDk); }

    // --- the colossal fallen star: a half-buried cracked sphere + jagged shards ---
    Vector3 m = c + Vector3{ 0, 0.5f, 0 };
    DrawModelEx(s_dome, m, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 3.0f, 3.4f }, meteor);
    DrawModelEx(s_dome, m, Vector3{ 1,0,0 }, 180, Vector3{ 3.4f, 1.2f, 3.4f }, meteorDk);
    SetRandomSeed(8300);
    for (int i = 0; i < 7; i++) { float a = rand_range(0, 2 * PI), el = rand_range(0.2f, 1.0f); Vector3 dir{ sinf(a) * cosf(el), sinf(el), cosf(a) * cosf(el) }; draw_bone_seg(m + Vector3Scale(dir, 2.6f), m + Vector3Scale(dir, rand_range(3.6f, 4.8f)), rand_range(0.2f, 0.4f), meteorDk); }

    // --- additive: glowing crack-veins + core + fumarole smoke/glow + drifting embers ---
    BeginBlendMode(BLEND_ADDITIVE);
    SetRandomSeed(8300);
    for (int i = 0; i < 10; i++) { float a = rand_range(0, 2 * PI), el = rand_range(0, 1.2f); Vector3 dir{ sinf(a) * cosf(el), sinf(el) + 0.2f, cosf(a) * cosf(el) }; Vector3 s0 = m + Vector3Scale(dir, 1.2f), s1 = m + Vector3Scale(dir, 3.2f); for (int k = 0; k < 4; k++) { Vector3 pp = Vector3Lerp(s0, s1, k / 3.0f); DrawSphereEx(pp, 0.12f + 0.05f * sinf(s_time * 4 + i + k), 5, 5, Color{ 180, 120, 255, 150 }); } }
    DrawSphereEx(m, 1.4f + 0.3f * sinf(s_time * 3), 10, 10, Color{ 150, 90, 230, 70 });
    for (auto& v : vents) { for (int s = 0; s < 6; s++) { float off = (float)s / 6; float y = fmodf(s_time * 0.3f + off, 1.0f) * 6.0f; DrawSphereEx(v + Vector3{ sinf(s_time + s) * 0.4f, 0.6f + y, 0 }, 0.4f + y * 0.1f, 6, 6, Color{ 120, 90, 150, 40 }); } DrawSphereEx(v + Vector3{ 0,0.5f,0 }, 0.4f, 6, 6, Color{ 180, 120, 255, 60 }); }
    SetRandomSeed(515);
    for (int i = 0; i < 26; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius), sp = rand_range(0.2f, 0.5f), off = rand_range(0, 1); float y = fmodf(s_time * sp + off, 1.0f) * 7.0f; DrawSphereEx(c + Vector3{ bx, 0.5f + y, bz }, 0.06f, 4, 4, Color{ 200, 150, 255, 110 }); }
    EndBlendMode();
}

// ---- The Fallen Titan: a colossal shattered statue strewn across a desolate dusk plain ----
// the iconic reaching hand: a half-buried forearm + palm slab + four standing curled fingers + a thumb.
static void draw_giant_hand(Vector3 base, float yaw, float s, Color st, Color sd) {
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(-14.0f, 1, 0, 0);
    DrawModelEx(s_cyl, Vector3{ 0, 0.2f * s, -3.4f * s }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 1.2f * s, 3.4f * s, 1.2f * s }, sd);   // forearm sinking into the earth
    DrawModelEx(s_column, Vector3{ 0, 0.5f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.8f * s, 1.0f * s, 2.4f * s }, st);            // palm
    for (int f = 0; f < 4; f++) {
        float fx = (f - 1.5f) * 0.7f * s;
        rlPushMatrix();
        rlTranslatef(fx, 0.9f * s, 1.2f * s);
        rlRotatef((f - 1.5f) * 8.0f, 0, 0, 1);   // splay
        rlRotatef(-55.0f, 1, 0, 0);              // stand the finger up
        DrawModelEx(s_column, Vector3{ 0, 1.0f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f * s, 2.0f * s, 0.55f * s }, st);
        DrawModelEx(s_column, Vector3{ 0, 2.2f * s, 0.25f * s }, Vector3{ 1,0,0 }, 28.0f, Vector3{ 0.42f * s, 1.1f * s, 0.48f * s }, sd);   // curled tip
        rlPopMatrix();
    }
    rlPushMatrix();   // thumb
    rlTranslatef(1.5f * s, 0.8f * s, -0.2f * s);
    rlRotatef(-40.0f, 0, 0, 1); rlRotatef(-28.0f, 1, 0, 0);
    DrawModelEx(s_column, Vector3{ 0, 0.9f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.55f * s, 1.8f * s, 0.6f * s }, st);
    rlPopMatrix();
    rlPopMatrix();
}

// the toppled head: a cracked cranium + face block, brow, dark eye sockets, nose, a broken laurel.
static void draw_giant_head(Vector3 base, float yaw, float s, Color st, Color sd) {
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(-25.0f, 1, 0, 0);   // face tilts up toward the sky
    Vector3 ctr = Vector3{ 0, 1.6f * s, 0 };
    DrawModelEx(s_dome, ctr, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f * s, 3.0f * s, 3.0f * s }, st);
    DrawModelEx(s_dome, ctr, Vector3{ 1,0,0 }, 180, Vector3{ 3.0f * s, 2.2f * s, 3.0f * s }, sd);
    DrawModelEx(s_column, Vector3{ 0, 0.9f * s, 2.0f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f * s, 2.4f * s, 1.2f * s }, st);   // face block
    DrawModelEx(s_column, Vector3{ 0, 1.9f * s, 2.5f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f * s, 0.5f * s, 0.6f * s }, sd);   // brow ridge
    DrawModelEx(s_dome, Vector3{ -0.9f * s, 1.4f * s, 2.7f * s }, Vector3{ 1,0,0 }, -90, Vector3{ 0.5f * s,0.4f * s,0.5f * s }, Color{ 20,18,16,255 });
    DrawModelEx(s_dome, Vector3{ 0.9f * s, 1.4f * s, 2.7f * s }, Vector3{ 1,0,0 }, -90, Vector3{ 0.5f * s,0.4f * s,0.5f * s }, Color{ 20,18,16,255 });
    DrawModelEx(s_cone, Vector3{ 0, 1.0f * s, 2.9f * s }, Vector3{ 1,0,0 }, 90, Vector3{ 0.5f * s, 1.0f * s, 0.6f * s }, st);    // nose
    for (int i = 0; i < 5; i++) { float a = (-0.5f + i / 4.0f) * 2.4f; DrawModelEx(s_cone, ctr + Vector3{ sinf(a) * 2.6f * s, 1.8f * s, cosf(a) * 2.6f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.35f * s,0.9f * s,0.35f * s }, sd); }   // broken laurel
    rlPopMatrix();
}

// the shattered chest: a main block + broken neck stump + two toga-fold slabs.
static void draw_titan_torso(Vector3 base, float yaw, float s, Color st, Color sd) {
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(yaw, 0, 1, 0); rlRotatef(-8.0f, 1, 0, 0);
    DrawModelEx(s_column, Vector3{ 0, 1.4f * s, 0 }, Vector3{ 0,1,0 }, 6, Vector3{ 4.0f * s, 2.8f * s, 2.6f * s }, st);
    DrawModelEx(s_column, Vector3{ 0, 2.6f * s, -1.6f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f * s, 1.2f * s, 1.4f * s }, sd);   // broken neck stump
    DrawModelEx(s_column, Vector3{ -1.8f * s, 0.8f * s, 1.0f * s }, Vector3{ 0,0,1 }, 20, Vector3{ 1.2f * s, 2.2f * s, 2.0f * s }, sd);
    DrawModelEx(s_column, Vector3{ 1.8f * s, 0.8f * s, 1.0f * s }, Vector3{ 0,0,1 }, -20, Vector3{ 1.2f * s, 2.2f * s, 2.0f * s }, sd);
    rlPopMatrix();
}

static void build_titan() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c + Vector3{ -9,0,-10 }, 3.0f });   // head
    obstacles.push_back({ c + Vector3{ 10,0,-4 }, 3.2f });    // torso
    obstacles.push_back({ c + Vector3{ -11,0,5 }, 1.8f });    // fallen limb columns
    obstacles.push_back({ c + Vector3{ 8,0,9 }, 1.8f });
    SetRandomSeed(9100);
    for (int i = 0; i < 6; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(7.0f, boundary_radius - 5.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; obstacles.push_back({ p, 1.2f }); }   // rubble drums
    s_wisps.push_back(c + Vector3{ 0, 3.0f, 0 });
    s_wisps.push_back(c + Vector3{ -9, 2.5f, -10 });
    s_wisps.push_back(c + Vector3{ 10, 2.5f, -4 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.5f }); }
}

static void draw_titan() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color stone{ 150,138,120,255 }, stoneDk{ 118,108,94,255 }, sand{ 142,128,104,255 }, sandDk{ 120,108,88,255 };

    // barren cracked plain
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, sand);
    SetRandomSeed(9000);
    for (int i = 0; i < 14; i++) { float a = rand_range(0, 2 * PI); rlPushMatrix(); rlTranslatef(c.x, 0.02f, c.z); rlRotatef(a * RAD2DEG, 0, 1, 0); DrawModelEx(s_column, Vector3{ 0, 0, rand_range(5, 14) }, Vector3{ 0,1,0 }, rand_range(-12, 12), Vector3{ rand_range(0.3f,0.7f), 0.03f, rand_range(4, 9) }, sandDk); rlPopMatrix(); }

    // the shattered titan
    draw_giant_hand(c, 12.0f, 1.6f, stone, stoneDk);
    draw_giant_head(c + Vector3{ -9, 0, -10 }, 40.0f, 1.5f, stone, stoneDk);
    draw_titan_torso(c + Vector3{ 10, 0, -4 }, -50.0f, 1.4f, stone, stoneDk);
    draw_bone_seg(c + Vector3{ -13, 1.0f, 3 }, c + Vector3{ -9, 1.2f, 7 }, 1.4f, stone);      // fallen limb columns
    draw_bone_seg(c + Vector3{ 6, 1.0f, 7 }, c + Vector3{ 10, 1.1f, 11 }, 1.3f, stoneDk);

    // rubble drums + boulders
    SetRandomSeed(9100);
    for (int i = 0; i < 6; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(7.0f, boundary_radius - 5.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_cyl, p, Vector3{ 0,1,0 }, rand_range(0, 360), Vector3{ 1.0f, rand_range(0.8f, 1.6f), 1.0f }, (i & 1) ? stone : stoneDk); }
    SetRandomSeed(9200);
    for (int i = 0; i < 10; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(4.0f, boundary_radius - 2.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; float s = rand_range(0.5f, 1.2f); DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ s, s * 0.7f, s }, (i & 1) ? sandDk : stoneDk); }

    // additive: dusty warm motes + drifting sand
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 210, 180, 130, 45 });
    SetRandomSeed(303);
    for (int i = 0; i < 22; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius), sp = rand_range(0.1f, 0.3f), off = rand_range(0, 1); float y = fmodf(s_time * sp + off, 1.0f) * 6.0f; DrawSphereEx(c + Vector3{ bx, 0.5f + y, bz }, 0.05f, 4, 4, Color{ 220, 200, 160, 80 }); }
    EndBlendMode();
}

// ---- The Hoodoos: a red-rock badlands canyon of banded sandstone spires ----
// one hoodoo: a stack of s_cyl discs with wavy pinch/bulge bands, overall tapering, leaning,
// topped with a wider balanced caprock.
static void draw_hoodoo(Vector3 base, float h, float baseR, int seed, Color c1, Color c2, Color cap) {
    SetRandomSeed(seed);
    int segs = (int)(h / 1.1f); if (segs < 3) segs = 3;
    float lean = rand_range(-0.16f, 0.16f), seg = h / segs;
    for (int i = 0; i < segs; i++) {
        float t = i / (float)segs;
        float r = baseR * (1.0f - 0.55f * t) * (1.0f + 0.22f * sinf(t * PI * 2.5f + seed));   // taper * wavy bands
        DrawModelEx(s_cyl, base + Vector3{ lean * h * t, seg * i, lean * 0.5f * h * t }, Vector3{ 0,1,0 }, 0, Vector3{ r, seg * 1.02f, r }, (i & 1) ? c1 : c2);
    }
    Vector3 top = base + Vector3{ lean * h, h, lean * 0.5f * h };
    float capR = baseR * 0.78f;
    DrawModelEx(s_cyl, top, Vector3{ 0,1,0 }, 0, Vector3{ capR, seg * 0.8f, capR }, cap);
    DrawModelEx(s_dome, top + Vector3{ 0, seg * 0.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ capR * 0.9f, capR * 0.5f, capR * 0.9f }, cap);
}

// spire roster: x, z(=.y), height(=.z), baseR(=.w). Shared by build (obstacles) + draw.
static void hoodoo_layout(std::vector<Vector4>& hs) {
    Vector3 c = boundary_center;
    SetRandomSeed(12000);
    int tries = 0;
    while ((int)hs.size() < 14 && tries < 400) {
        tries++;
        float a = rand_range(0, 2 * PI), rr = rand_range(6.0f, boundary_radius - 3.0f);
        Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr };
        if (p.z > c.z + 12.0f) continue;                       // keep the player spawn aisle clear
        bool ok = true; for (auto& h : hs) if (Vector3Distance(p, Vector3{ h.x, 0, h.y }) < 4.0f) { ok = false; break; }
        if (!ok) continue;
        hs.push_back(Vector4{ p.x, p.z, rand_range(5.0f, 11.0f), rand_range(0.9f, 1.6f) });
    }
}

static void build_hoodoo() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    std::vector<Vector4> hs; hoodoo_layout(hs);
    for (auto& h : hs) obstacles.push_back({ Vector3{ h.x, 0, h.y }, h.w * 0.9f + 0.3f });
    s_wisps.push_back(c + Vector3{ 0, 4.0f, 0 });
    SetRandomSeed(12100);
    for (int i = 0; i < 5; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(8.0f, boundary_radius - 4.0f); s_wisps.push_back(c + Vector3{ sinf(a) * rr, 3.0f, cosf(a) * rr }); }
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_hoodoos() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color s1{ 198,120,72,255 }, s2{ 176,98,58,255 }, cap{ 150,80,50,255 }, sand{ 210,170,120,255 }, sandDk{ 190,150,104,255 }, mesa{ 170,96,60,255 }, mesaDk{ 140,78,50,255 }, scrub{ 120,128,72,255 };

    // sandy canyon floor + ripple dunes
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, sand);
    SetRandomSeed(12300);
    for (int i = 0; i < 18; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(3.0f, boundary_radius - 1.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, rand_range(0, 180), Vector3{ rand_range(1.2f,2.6f), 0.25f, rand_range(0.8f,1.4f) }, (i & 1) ? sandDk : sand); }

    // mesa walls along the back rim (flat-topped buttes)
    SetRandomSeed(12400);
    for (int i = 0; i < 10; i++) {
        float a = PI + (i / 9.0f - 0.5f) * 2.2f, rr = boundary_radius - 1.5f, h = rand_range(5.0f, 9.0f);
        Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr };
        DrawModelEx(s_column, p + Vector3{ 0, h * 0.5f, 0 }, Vector3{ 0,1,0 }, rand_range(-12.0f, 12.0f), Vector3{ rand_range(4.0f,6.0f), h, rand_range(3.0f,5.0f) }, (i & 1) ? mesa : mesaDk);
        DrawModelEx(s_column, p + Vector3{ 0, h, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(4.5f,6.5f), 0.6f, rand_range(3.5f,5.5f) }, cap);
    }

    // the central great hoodoo + the field
    draw_hoodoo(c, 13.0f, 1.8f, 999, s1, s2, cap);
    std::vector<Vector4> hs; hoodoo_layout(hs);
    for (size_t i = 0; i < hs.size(); i++) draw_hoodoo(Vector3{ hs[i].x, 0, hs[i].y }, hs[i].z, hs[i].w, 400 + (int)i * 53, s1, s2, cap);

    // a natural rock arch landmark
    Vector3 ab = c + Vector3{ -13, 0, -6 };
    DrawModelEx(s_cyl, ab + Vector3{ -2.2f, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 5.0f, 1.0f }, mesa);
    DrawModelEx(s_cyl, ab + Vector3{ 2.2f, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 5.0f, 1.0f }, mesa);
    draw_bone_seg(ab + Vector3{ -2.2f, 5.0f, 0 }, ab + Vector3{ 2.2f, 5.0f, 0 }, 1.0f, mesaDk);

    // sparse desert scrub
    SetRandomSeed(12500);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(5.0f, boundary_radius - 2.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p + Vector3{ 0,0.3f,0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.5f,1.0f), 0.6f, rand_range(0.5f,1.0f) }, scrub); }

    // additive: warm fill glow + dust devils + heat-shimmer motes
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 200, 140, 40 });
    Vector3 devils[2] = { c + Vector3{ 6,0,-8 }, c + Vector3{ -7,0,4 } };
    for (int d = 0; d < 2; d++) for (int k = 0; k < 14; k++) { float t = k / 14.0f, ang = s_time * 3 + t * 9 + d * 2, rad = 0.4f + t * 1.6f; DrawSphereEx(devils[d] + Vector3{ sinf(ang) * rad, t * 6.0f, cosf(ang) * rad }, 0.18f, 5, 5, Color{ 220, 190, 140, 70 }); }
    SetRandomSeed(303);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); float y = 0.5f + 1.0f * fabsf(sinf(s_time * 0.6f + i)); DrawSphereEx(c + Vector3{ bx, y, bz }, 0.05f, 4, 4, Color{ 240, 220, 170, 60 }); }
    EndBlendMode();
}

// ---- The Moai: a windswept coastal headland lined with colossal carved stone heads ----
// one moai bust: a buried torso + an elongated head with the iconic heavy brow, long nose,
// deep eye sockets, jutting chin and long ears; optional red topknot (pukao). Leans back.
static void draw_moai(Vector3 base, float yaw, float s, bool pukao, Color st, Color sd, Color red) {
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(-6.0f, 1, 0, 0);
    DrawModelEx(s_column, Vector3{ 0, 2.2f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f * s, 4.6f * s, 1.4f * s }, st);   // torso (lower part sunk)
    Vector3 h = Vector3{ 0, 5.4f * s, 0 };
    DrawModelEx(s_column, h, Vector3{ 0,1,0 }, 0, Vector3{ 1.8f * s, 2.6f * s, 1.6f * s }, st);                          // head
    DrawModelEx(s_column, h + Vector3{ 0, 0.7f * s, 0.7f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 1.7f * s, 0.5f * s, 0.4f * s }, sd);   // brow
    DrawModelEx(s_column, h + Vector3{ -0.5f * s, 0.3f * s, 0.75f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f * s,0.4f * s,0.2f * s }, Color{ 30,28,24,255 });   // eye sockets
    DrawModelEx(s_column, h + Vector3{ 0.5f * s, 0.3f * s, 0.75f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f * s,0.4f * s,0.2f * s }, Color{ 30,28,24,255 });
    DrawModelEx(s_column, h + Vector3{ 0, -0.1f * s, 0.85f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f * s, 1.4f * s, 0.5f * s }, sd);   // long nose
    DrawModelEx(s_column, h + Vector3{ 0, -1.1f * s, 0.7f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f * s, 0.6f * s, 0.5f * s }, st);   // jutting chin
    DrawModelEx(s_column, h + Vector3{ -0.95f * s, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.25f * s, 1.8f * s, 0.5f * s }, st);        // long ears
    DrawModelEx(s_column, h + Vector3{ 0.95f * s, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.25f * s, 1.8f * s, 0.5f * s }, st);
    if (pukao) DrawModelEx(s_cyl, h + Vector3{ 0, 1.5f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f * s, 0.9f * s, 1.0f * s }, red);   // red topknot
    rlPopMatrix();
}

static void build_moai() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    float rowx[6] = { -10, -6, -2, 2, 6, 10 };
    for (int i = 0; i < 6; i++) obstacles.push_back({ c + Vector3{ rowx[i], 0, -6.0f }, 1.4f });
    SetRandomSeed(13100);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(6.0f, boundary_radius - 3.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; float s = rand_range(0.7f, 1.4f); if (s > 1.0f) obstacles.push_back({ p, s }); }   // boulders
    s_wisps.push_back(c + Vector3{ 0, 5.0f, 0 });
    for (int i = 0; i < 6; i += 2) s_wisps.push_back(c + Vector3{ rowx[i], 4.0f, -6.0f });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.5f }); }
}

static void draw_moai_isle() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color stone{ 150,142,128,255 }, stoneDk{ 116,110,98,255 }, red{ 150,70,52,255 }, grass{ 96,118,72,255 }, grassDk{ 78,100,60,255 };

    // grassy headland + tussock mounds
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, grass);
    SetRandomSeed(13000);
    for (int i = 0; i < 22; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(3.0f, boundary_radius - 1.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, rand_range(0, 180), Vector3{ rand_range(0.8f,1.8f), 0.3f, rand_range(0.8f,1.8f) }, (i & 1) ? grassDk : grass); }

    // ahu platform under the row
    DrawModelEx(s_column, c + Vector3{ 0, 0.4f, -6.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 26.0f, 0.9f, 3.2f }, stoneDk);

    // the row of moai facing the player (+z), plus the central great moai
    float rowx[6] = { -10, -6, -2, 2, 6, 10 };
    for (int i = 0; i < 6; i++) draw_moai(c + Vector3{ rowx[i], 0.8f, -6.0f }, (i - 2.5f) * 3.0f, 1.0f, (i % 2 == 0), stone, stoneDk, red);
    draw_moai(c, 0.0f, 1.5f, true, stone, stoneDk, red);

    // two toppled moai lying forward
    rlPushMatrix(); rlTranslatef(c.x - 12, 0.6f, c.z + 5); rlRotatef(20.0f, 0, 1, 0); rlRotatef(82.0f, 1, 0, 0); draw_moai(Vector3{ 0,0,0 }, 0, 1.0f, false, stoneDk, stoneDk, red); rlPopMatrix();
    rlPushMatrix(); rlTranslatef(c.x + 11, 0.6f, c.z + 7); rlRotatef(-30.0f, 0, 1, 0); rlRotatef(82.0f, 1, 0, 0); draw_moai(Vector3{ 0,0,0 }, 0, 0.9f, false, stoneDk, stoneDk, red); rlPopMatrix();

    // scattered boulders
    SetRandomSeed(13100);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(6.0f, boundary_radius - 3.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; float s = rand_range(0.7f, 1.4f); DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ s, s * 0.7f, s }, (i & 1) ? stoneDk : stone); }

    // additive: cool fill glow + a sea-mist band at the back + drifting spray
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 200, 210, 220, 40 });
    SetRandomSeed(303);
    for (int i = 0; i < 16; i++) { float bx = rand_range(-boundary_radius, boundary_radius); DrawSphereEx(c + Vector3{ bx + sinf(s_time * 0.5f + i) * 1.0f, rand_range(1.0f, 5.0f), -boundary_radius + rand_range(0.0f, 6.0f) }, rand_range(1.0f, 2.0f), 6, 6, Color{ 210, 218, 225, 26 }); }
    EndBlendMode();
}

// ---- Dripstone Hollow: a limestone cavern of stalactites/stalagmites over a still pool ----
// one dripstone cone: apex-down (stalactite, base at ceiling) or apex-up (stalagmite, base on floor).
static void draw_dripcone(Vector3 base, float len, float r, bool down, Color c) {
    if (down) DrawModelEx(s_cone, base, Vector3{ 1,0,0 }, 180.0f, Vector3{ r, len, r }, c);   // base wide at ceiling, tip hangs down
    else      DrawModelEx(s_cone, base, Vector3{ 0,1,0 }, 0, Vector3{ r, len, r }, c);        // base wide on floor, tip points up
}

static void build_cavern() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    Vector3 cols[4] = { c + Vector3{ -8,0,-5 }, c + Vector3{ 9,0,-3 }, c + Vector3{ -6,0,7 }, c + Vector3{ 7,0,8 } };
    for (auto& cc : cols) obstacles.push_back({ cc, 1.4f });   // great columns
    SetRandomSeed(14200);
    for (int i = 0; i < 18; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(3.0f, boundary_radius - 2.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; float r = rand_range(0.5f, 1.2f); (void)rand_range(1.5f, 4.5f); if (r > 0.95f) obstacles.push_back({ p, r + 0.3f }); }   // big stalagmites
    s_wisps.push_back(c + Vector3{ 0, 2.0f, 0 });
    for (auto& cc : cols) s_wisps.push_back(cc + Vector3{ 0, 2.0f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.5f }); }
}

static void draw_cavern() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color rock{ 120,110,98,255 }, rockDk{ 92,84,74,255 }, drip{ 150,140,122,255 }, dripDk{ 120,112,96,255 };
    float H = 15.0f;

    // dark cave ceiling slab + stone rim banks around the pool (the pool itself is the water plane)
    DrawModelEx(s_column, c + Vector3{ 0, H, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 8, 1.4f, boundary_radius * 2 + 8 }, rockDk);
    SetRandomSeed(14000);
    for (int i = 0; i < 16; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(boundary_radius - 5.0f, boundary_radius - 0.5f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.5f,3.0f), rand_range(0.6f,1.4f), rand_range(1.5f,3.0f) }, (i & 1) ? rock : rockDk); }

    // hanging stalactites from the ceiling
    SetRandomSeed(14100);
    for (int i = 0; i < 26; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(2.0f, boundary_radius - 2.0f); Vector3 p = c + Vector3{ sinf(a) * rr, H - 0.6f, cosf(a) * rr }; draw_dripcone(p, rand_range(2.0f, 6.0f), rand_range(0.4f, 1.0f), true, (i & 1) ? drip : dripDk); }
    // rising stalagmites from the floor
    SetRandomSeed(14200);
    for (int i = 0; i < 18; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(3.0f, boundary_radius - 2.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; float r = rand_range(0.5f, 1.2f), len = rand_range(1.5f, 4.5f); draw_dripcone(p, len, r, false, (i & 1) ? drip : dripDk); }

    // great floor-to-ceiling columns (a few + the central one)
    Vector3 cols[4] = { c + Vector3{ -8,0,-5 }, c + Vector3{ 9,0,-3 }, c + Vector3{ -6,0,7 }, c + Vector3{ 7,0,8 } };
    for (auto& cc : cols) { draw_dripcone(cc, H * 0.6f, 1.3f, false, drip); draw_dripcone(cc + Vector3{ 0,H,0 }, H * 0.55f, 1.3f, true, drip); DrawModelEx(s_cyl, cc + Vector3{ 0, H * 0.45f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, H * 0.15f, 0.7f }, dripDk); }
    draw_dripcone(c, H * 0.65f, 2.0f, false, drip); draw_dripcone(c + Vector3{ 0,H,0 }, H * 0.5f, 2.0f, true, drip); DrawModelEx(s_cyl, c + Vector3{ 0, H * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f, H * 0.2f, 1.1f }, dripDk);

    // additive: falling water drips (from the stalactite tips) + mineral glow + mist over the pool
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 150, 200, 210, 45 });
    SetRandomSeed(14100);
    for (int i = 0; i < 26; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(2.0f, boundary_radius - 2.0f); float off = rand_range(0, 1); float y = (H - 2.0f) - fmodf(s_time * 2.0f + off * H, H); DrawSphereEx(c + Vector3{ sinf(a) * rr, y, cosf(a) * rr }, 0.08f, 4, 4, Color{ 170, 210, 220, 120 }); }
    SetRandomSeed(303);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); float y = 0.5f + 1.5f * fabsf(sinf(s_time * 0.5f + i)); DrawSphereEx(c + Vector3{ bx, y, bz }, 0.5f, 6, 6, Color{ 160, 180, 190, 22 }); }
    EndBlendMode();
}

// ---- Snowbound Pines: a snow-laden conifer forest with a frozen pond + log cabin ----
// one conifer: a short trunk + 4-5 stacked shrinking green cone tiers, each frosted with snow.
static void draw_conifer(Vector3 base, float h, int seed, Color green, Color greenDk, Color snow, Color bark) {
    DrawModelEx(s_cyl, base, Vector3{ 0,1,0 }, 0, Vector3{ 0.28f, h * 0.3f, 0.28f }, bark);
    int tiers = 4 + (seed % 2);
    float y = h * 0.22f, tierH = (h - y) / tiers, r = h * 0.30f;
    for (int i = 0; i < tiers; i++) {
        float t = i / (float)tiers, rr = r * (1.0f - 0.68f * t);
        DrawModelEx(s_cone, base + Vector3{ 0, y, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rr, tierH * 1.5f, rr }, (i & 1) ? greenDk : green);
        DrawModelEx(s_cone, base + Vector3{ 0, y + tierH * 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rr * 0.82f, tierH * 0.5f, rr * 0.82f }, snow);   // snow frosting on top of the tier
        y += tierH * 0.95f;
    }
}

// tree roster: x, z(=.y), height(=.z). Shared by build (obstacles) + draw.
static void pines_layout(std::vector<Vector4>& ts) {
    Vector3 c = boundary_center;
    SetRandomSeed(15000);
    int tries = 0;
    while ((int)ts.size() < 16 && tries < 400) {
        tries++;
        float a = rand_range(0, 2 * PI), rr = rand_range(6.0f, boundary_radius - 2.5f);
        Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr };
        if (p.z > c.z + 12.0f) continue;                       // keep the spawn aisle clear
        if (Vector3Distance(p, c + Vector3{ 12,0,-6 }) < 5.0f) continue;   // keep the cabin clear
        bool ok = true; for (auto& t : ts) if (Vector3Distance(p, Vector3{ t.x, 0, t.y }) < 3.5f) { ok = false; break; }
        if (!ok) continue;
        ts.push_back(Vector4{ p.x, p.z, rand_range(5.0f, 9.0f), 0 });
    }
}

static void build_pines() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    std::vector<Vector4> ts; pines_layout(ts);
    for (auto& t : ts) obstacles.push_back({ Vector3{ t.x, 0, t.y }, 0.8f });
    obstacles.push_back({ c + Vector3{ 12,0,-6 }, 2.4f });   // log cabin
    s_wisps.push_back(c + Vector3{ 12, 1.8f, -6 });          // warm cabin window
    s_wisps.push_back(c + Vector3{ 0, 4.0f, 0 });
    SetRandomSeed(15100);
    for (int i = 0; i < 4; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(8.0f, boundary_radius - 4.0f); s_wisps.push_back(c + Vector3{ sinf(a) * rr, 2.5f, cosf(a) * rr }); }
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_pines() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color snow{ 236,240,248,255 }, snowDk{ 205,212,228,255 }, green{ 46,86,62,255 }, greenDk{ 34,68,50,255 }, bark{ 86,66,48,255 }, ice{ 180,210,225,255 }, wood{ 120,92,60,255 }, woodDk{ 90,68,46,255 }, roof{ 150,86,72,255 };

    // snowy ground + drifts + a frozen pond
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, snow);
    SetRandomSeed(15300);
    for (int i = 0; i < 20; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(3.0f, boundary_radius - 1.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.2f,2.8f), rand_range(0.3f,0.7f), rand_range(1.2f,2.8f) }, (i & 1) ? snowDk : snow); }
    DrawModelEx(s_cyl, c + Vector3{ -9, 0.04f, 8 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.0f, 0.06f, 5.0f }, ice);

    // the conifers (central great pine + the field)
    draw_conifer(c, 11.0f, 999, green, greenDk, snow, bark);
    std::vector<Vector4> ts; pines_layout(ts);
    for (size_t i = 0; i < ts.size(); i++) draw_conifer(Vector3{ ts[i].x, 0, ts[i].y }, ts[i].z, 300 + (int)i * 41, green, greenDk, snow, bark);

    // log cabin with a snow roof + smoking chimney
    draw_cottage(c + Vector3{ 12, 0, -6 }, 5.0f, 5.0f, 3.0f, 2.0f, 0.0f, wood, roof, woodDk);
    DrawModelEx(s_column, c + Vector3{ 12, 5.0f, -6 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.2f, 0.4f, 5.4f }, snow);   // snow on the roof ridge
    DrawModelEx(s_column, c + Vector3{ 13.6f, 2.5f, -6 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 5.0f, 0.7f }, woodDk);   // chimney

    // a couple of snow-capped stumps + a fallen log
    DrawModelEx(s_cyl, c + Vector3{ -13, 0.4f, -3 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.8f, 0.7f }, bark);
    DrawModelEx(s_dome, c + Vector3{ -13, 1.0f, -3 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.3f, 0.7f }, snow);
    draw_bone_seg(c + Vector3{ 9, 0.5f, 10 }, c + Vector3{ 13, 0.6f, 8 }, 0.5f, bark);

    // additive: warm window + soft cold glow + chimney smoke + falling snow
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) { Color g = (i == 0) ? Color{ 255, 200, 130, 70 } : Color{ 200, 225, 245, 38 }; DrawSphereEx(s_wisps[i], 0.45f + 0.12f * sinf(s_time * 2 + i), 8, 8, g); }
    for (int s = 0; s < 6; s++) { float off = (float)s / 6; float y = fmodf(s_time * 0.4f + off, 1.0f) * 6.0f; DrawSphereEx(c + Vector3{ 13.6f + sinf(s_time + s) * 0.3f, 5.0f + y, -6 }, 0.3f + y * 0.08f, 6, 6, Color{ 200, 205, 210, 40 }); }
    SetRandomSeed(303);
    for (int i = 0; i < 40; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius), sp = rand_range(0.2f, 0.5f), off = rand_range(0, 1); float y = 10.0f - fmodf(s_time * sp + off, 1.0f) * 10.0f; DrawSphereEx(c + Vector3{ bx + sinf(s_time * 0.5f + i) * 0.5f, y, bz }, 0.07f, 4, 4, Color{ 240, 245, 255, 150 }); }
    EndBlendMode();
}

// ---- The Galleon: the storm-tossed main deck of a great sailing ship ----
static void build_galleon() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    for (float z = -17.0f; z <= 17.0f; z += 2.5f) { obstacles.push_back({ c + Vector3{ -12.5f, 0, z }, 1.4f }); obstacles.push_back({ c + Vector3{ 12.5f, 0, z }, 1.4f }); }   // side bulwarks confine play to the deck
    obstacles.push_back({ c + Vector3{ 0,0,10 }, 0.9f });    // fore mast
    obstacles.push_back({ c + Vector3{ 0,0,-10 }, 0.9f });   // mizzen mast
    obstacles.push_back({ c + Vector3{ 0,0,-15 }, 1.4f });   // wheel post / binnacle
    s_wisps.push_back(c + Vector3{ 0, 5.0f, 0 });
    s_wisps.push_back(c + Vector3{ 0, 3.0f, -15 });
    s_wisps.push_back(c + Vector3{ -10, 3.0f, 4 });
    s_wisps.push_back(c + Vector3{ 10, 3.0f, 4 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.5f }); }
}

static void draw_galleon() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color wood{ 120,92,58,255 }, woodDk{ 88,66,42,255 }, plank{ 134,104,66,255 }, sail{ 200,194,176,255 }, sailDk{ 170,164,148,255 }, iron{ 60,58,62,255 }, rope{ 150,132,96,255 }, sea{ 38,52,64,255 };

    // surrounding sea + the ship's hull
    DrawModelEx(s_column, c + Vector3{ 0, -2.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 30, 0.5f, boundary_radius * 2 + 30 }, sea);
    rlPushMatrix(); rlTranslatef(c.x - 13.5f, -1.4f, c.z); rlRotatef(12.0f, 0, 0, 1); DrawModelEx(s_column, Vector3{ 0,0,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 5.0f, 40.0f }, woodDk); rlPopMatrix();   // hull sides
    rlPushMatrix(); rlTranslatef(c.x + 13.5f, -1.4f, c.z); rlRotatef(-12.0f, 0, 0, 1); DrawModelEx(s_column, Vector3{ 0,0,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 5.0f, 40.0f }, woodDk); rlPopMatrix();
    rlPushMatrix(); rlTranslatef(c.x, -1.2f, c.z + 20.0f); rlRotatef(45.0f, 0, 1, 0); DrawModelEx(s_column, Vector3{ 0,0,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 9.0f, 4.6f, 9.0f }, woodDk); rlPopMatrix();   // diamond bow

    // planked deck (the floor) + plank lines
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 26.0f, 0.5f, 38.0f }, plank);
    for (int i = -6; i <= 6; i++) DrawModelEx(s_column, c + Vector3{ i * 2.0f, 0.02f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 0.04f, 38.0f }, woodDk);
    // bulwarks (side rails) + a bowsprit
    DrawModelEx(s_column, c + Vector3{ -12.6f, 1.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.8f, 36.0f }, wood);
    DrawModelEx(s_column, c + Vector3{ 12.6f, 1.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.8f, 36.0f }, wood);
    DrawModelEx(s_cyl, c + Vector3{ 0, 2.5f, 23.0f }, Vector3{ 1,0,0 }, 70.0f, Vector3{ 0.4f, 9.0f, 0.4f }, wood);   // bowsprit

    // raised quarterdeck at the stern + the ship's wheel
    DrawModelEx(s_column, c + Vector3{ 0, 0.7f, -15.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 20.0f, 1.4f, 8.0f }, plank);
    Vector3 wheel = c + Vector3{ 0, 3.0f, -15.0f };
    DrawModelEx(s_cyl, c + Vector3{ 0, 2.2f, -15.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.8f, 0.3f }, woodDk);
    DrawModelEx(s_torus, wheel, Vector3{ 1,0,0 }, 90.0f, Vector3{ 1.2f, 1.2f, 1.2f }, wood);
    for (int k = 0; k < 4; k++) { DrawModelEx(s_cyl, wheel, Vector3{ 0,0,1 }, k * 45.0f, Vector3{ 0.08f, 2.4f, 0.08f }, woodDk); }

    // three masts with yards, furled sails and rigging
    float mz[3] = { 10.0f, 0.0f, -10.0f }, mh[3] = { 16.0f, 21.0f, 14.0f };
    for (int m = 0; m < 3; m++) {
        Vector3 mb = c + Vector3{ 0, 0.4f, mz[m] };
        DrawModelEx(s_cyl, mb, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, mh[m], 0.5f }, wood);
        for (int yI = 0; yI < 2; yI++) {
            float yy = mh[m] * (0.52f + yI * 0.30f), yardLen = 9.0f - yI * 2.6f;
            DrawModelEx(s_cyl, mb + Vector3{ 0, yy, 0 }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 0.22f, yardLen, 0.22f }, woodDk);
            DrawModelEx(s_column, mb + Vector3{ 0, yy - 1.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ yardLen * 0.92f, 2.8f, 0.2f }, (yI & 1) ? sailDk : sail);
        }
        draw_bone_seg(mb + Vector3{ 0, mh[m], 0 }, c + Vector3{ -11, 1.6f, mz[m] + 2 }, 0.05f, rope);
        draw_bone_seg(mb + Vector3{ 0, mh[m], 0 }, c + Vector3{ 11, 1.6f, mz[m] + 2 }, 0.05f, rope);
        draw_bone_seg(mb + Vector3{ 0, mh[m], 0 }, c + Vector3{ 0, 1.6f, mz[m] - 6 }, 0.05f, rope);
    }

    // cannons along both rails + a few barrels and a hatch
    for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 3; k++) { Vector3 cp = c + Vector3{ s * 11.3f, 0.7f, -6.0f + k * 6.0f }; DrawModelEx(s_column, cp, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.7f, 1.3f }, woodDk); DrawModelEx(s_cyl, cp + Vector3{ s * 0.7f, 0.5f, 0 }, Vector3{ 0,0,1 }, s * 70.0f, Vector3{ 0.22f, 1.5f, 0.22f }, iron); }
    DrawModelEx(s_column, c + Vector3{ 0, 0.4f, 4.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 0.6f, 3.0f }, woodDk);   // deck hatch
    SetRandomSeed(16000);
    for (int i = 0; i < 7; i++) { Vector3 p = c + Vector3{ rand_range(-9.0f,9.0f), 0.6f, rand_range(-9.0f,15.0f) }; DrawModelEx(s_cyl, p, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.2f, 0.6f }, wood); DrawModelEx(s_cyl, p, Vector3{ 0,1,0 }, 0, Vector3{ 0.66f, 0.2f, 0.66f }, iron); }   // barrels

    // additive: warm lanterns + storm spray over the sea + driving rain
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.14f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 195, 120, 60 });
    SetRandomSeed(16100);
    for (int i = 0; i < 24; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(15.0f, boundary_radius + 6.0f); DrawSphereEx(c + Vector3{ sinf(a) * rr, -1.8f + 1.2f * fabsf(sinf(s_time * 1.5f + i)), cosf(a) * rr }, rand_range(0.6f, 1.4f), 6, 6, Color{ 180, 200, 215, 40 }); }   // whitecap spray
    SetRandomSeed(303);
    for (int i = 0; i < 36; i++) { float bx = rand_range(-14, 14), bz = rand_range(-19, 19), sp = rand_range(0.5f, 0.9f), off = rand_range(0, 1); float y = 14.0f - fmodf(s_time * sp + off, 1.0f) * 16.0f; DrawSphereEx(c + Vector3{ bx + 2.0f, y, bz }, 0.05f, 4, 4, Color{ 170, 190, 210, 90 }); }   // driving rain
    EndBlendMode();
}

// ---- The Sunflower Fields: a sunny field of towering sunflowers turned to the sun ----
// one sunflower: a leaning stalk + two broad leaves + a big head (green sepal, ring of yellow
// petals, brown seed disc) tilted on an rlgl frame to face the sun (+z).
static void draw_sunflower(Vector3 base, float h, float yaw, int seed, Color stalk, Color leaf, Color petal, Color center) {
    SetRandomSeed(seed);
    float lean = rand_range(-0.06f, 0.06f);
    Vector3 top = base + Vector3{ lean * h, h, 0.0f };
    draw_bone_seg(base, top, 0.16f, stalk);
    for (int i = 0; i < 2; i++) { float t = 0.4f + i * 0.25f; Vector3 lp = base + Vector3{ lean * h * t, h * t, 0 }; float la = (i & 1) ? 1.0f : -1.0f; DrawModelEx(s_dome, lp + Vector3{ la * 0.6f, 0, 0 }, Vector3{ 0,0,1 }, la * 55.0f, Vector3{ 1.5f, 0.1f, 0.7f }, leaf); }
    rlPushMatrix();
    rlTranslatef(top.x, top.y, top.z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(-72.0f, 1, 0, 0);   // tilt the flat head up to face the sun
    DrawModelEx(s_cyl, Vector3{ 0, -0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.7f, 0.2f, 1.7f }, leaf);   // green sepal back
    for (int k = 0; k < 12; k++) { float a = k / 12.0f * 2 * PI; DrawModelEx(s_column, Vector3{ sinf(a) * 1.1f, 0.0f, cosf(a) * 1.1f }, Vector3{ 0,1,0 }, a * RAD2DEG, Vector3{ 0.4f, 0.12f, 1.25f }, petal); }   // petals
    DrawModelEx(s_cyl, Vector3{ 0, 0.12f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.05f, 0.28f, 1.05f }, center);   // brown seed disc
    rlPopMatrix();
}

// field roster: x, z(=.y), height(=.z), yaw(=.w). Shared by build (obstacles) + draw.
static void sunflower_layout(std::vector<Vector4>& fs) {
    Vector3 c = boundary_center;
    SetRandomSeed(17000);
    for (float gz = -15.0f; gz <= 9.0f; gz += 5.0f) for (float gx = -15.0f; gx <= 15.0f; gx += 5.0f) {
        Vector3 p = c + Vector3{ gx + rand_range(-1.2f,1.2f), 0, gz + rand_range(-1.2f,1.2f) };
        if (Vector3Distance(p, c) < 3.5f) continue;        // keep the central great-sunflower clearing
        fs.push_back(Vector4{ p.x, p.z, rand_range(5.0f, 8.0f), rand_range(-12.0f, 12.0f) });
    }
}

static void build_sunflower() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    std::vector<Vector4> fs; sunflower_layout(fs);
    for (auto& f : fs) obstacles.push_back({ Vector3{ f.x, 0, f.y }, 0.5f });
    s_wisps.push_back(c + Vector3{ 0, 6.0f, 0 });
    SetRandomSeed(17100);
    for (int i = 0; i < 5; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(8.0f, boundary_radius - 4.0f); s_wisps.push_back(c + Vector3{ sinf(a) * rr, 3.0f, cosf(a) * rr }); }
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 4.5f }); }
}

static void draw_sunflower_field() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color soil{ 96,74,52,255 }, soilDk{ 78,60,42,255 }, stalk{ 86,120,58,255 }, leaf{ 74,108,52,255 }, petal{ 246,196,48,255 }, center{ 96,60,36,255 };

    // tilled field ground + furrow rows
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, soil);
    for (int i = -9; i <= 9; i++) DrawModelEx(s_column, c + Vector3{ i * 2.4f, 0.02f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.04f, boundary_radius * 2 }, soilDk);

    // the great central sunflower + the field
    draw_sunflower(c, 10.0f, 0.0f, 999, stalk, leaf, petal, center);
    std::vector<Vector4> fs; sunflower_layout(fs);
    for (size_t i = 0; i < fs.size(); i++) draw_sunflower(Vector3{ fs[i].x, 0, fs[i].y }, fs[i].z, fs[i].w, 200 + (int)i * 37, stalk, leaf, petal, center);

    // a scarecrow
    Vector3 sc = c + Vector3{ -12, 0, -4 };
    DrawModelEx(s_cyl, sc + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 3.2f, 0.2f }, soilDk);
    DrawModelEx(s_cyl, sc + Vector3{ 0, 2.6f, 0 }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 0.15f, 2.4f, 0.15f }, soilDk);
    DrawModelEx(s_dome, sc + Vector3{ 0, 3.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.6f, 0.5f }, petal);
    DrawModelEx(s_cone, sc + Vector3{ 0, 3.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.6f, 0.7f }, soilDk);

    // additive: warm sun glow + drifting pollen / bees
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 220, 120, 50 });
    SetRandomSeed(303);
    for (int i = 0; i < 26; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); float y = 2.0f + 1.5f * sinf(s_time * 0.8f + i); DrawSphereEx(c + Vector3{ bx + sinf(s_time + i) * 0.6f, y, bz }, 0.06f, 4, 4, Color{ 250, 230, 150, 90 }); }
    EndBlendMode();
}

// ---- The Sphinx: a colossal reclining sphinx + pyramids on a dawn-lit sand plain ----
// a reclining lion body facing +z, with forelegs/paws, a rising chest, and a pharaoh head
// wearing the nemes headdress (flaring headcloth + lappets + false beard).
static void draw_sphinx(Vector3 base, float yaw, float s, Color st, Color sd, Color band) {
    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 1.6f * s, -2.0f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f * s, 3.2f * s, 9.0f * s }, st);    // body
    DrawModelEx(s_column, Vector3{ 0, 1.8f * s, -6.0f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 3.8f * s, 3.6f * s, 3.0f * s }, st);    // haunches
    for (int sgn = -1; sgn <= 1; sgn += 2) {
        DrawModelEx(s_column, Vector3{ sgn * 1.2f * s, 0.8f * s, 4.5f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f * s, 1.6f * s, 7.0f * s }, st);   // foreleg
        DrawModelEx(s_column, Vector3{ sgn * 1.2f * s, 0.5f * s, 8.4f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f * s, 1.0f * s, 1.8f * s }, sd);   // paw
    }
    DrawModelEx(s_column, Vector3{ 0, 3.2f * s, 2.0f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f * s, 3.2f * s, 2.4f * s }, st);     // rising chest
    DrawModelEx(s_column, Vector3{ 0, 4.8f * s, 2.6f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f * s, 1.8f * s, 1.6f * s }, st);     // neck
    Vector3 h = Vector3{ 0, 6.2f * s, 2.9f * s };
    DrawModelEx(s_column, h + Vector3{ 0, 0.4f * s, -0.3f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f * s, 2.8f * s, 2.2f * s }, band);   // nemes headcloth
    for (int sgn = -1; sgn <= 1; sgn += 2) DrawModelEx(s_column, h + Vector3{ sgn * 1.5f * s, -1.3f * s, 0.7f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f * s, 2.6f * s, 1.0f * s }, band);   // lappets
    DrawModelEx(s_column, h + Vector3{ 0, 0.0f, 1.0f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f * s, 2.2f * s, 0.9f * s }, st);     // face
    DrawModelEx(s_column, h + Vector3{ 0, 0.7f * s, 1.45f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f * s, 0.4f * s, 0.4f * s }, sd); // brow
    DrawModelEx(s_column, h + Vector3{ -0.55f * s, 0.3f * s, 1.5f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f * s,0.35f * s,0.2f * s }, Color{ 30,26,20,255 });
    DrawModelEx(s_column, h + Vector3{ 0.55f * s, 0.3f * s, 1.5f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f * s,0.35f * s,0.2f * s }, Color{ 30,26,20,255 });
    DrawModelEx(s_column, h + Vector3{ 0, -0.1f * s, 1.6f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f * s,0.9f * s,0.4f * s }, sd);   // nose
    DrawModelEx(s_column, h + Vector3{ 0, -1.6f * s, 1.2f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f * s,1.4f * s,0.5f * s }, band); // false beard
    rlPopMatrix();
}

static void build_sphinx() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c + Vector3{ 0,0,8 }, 2.4f });     // front paws
    obstacles.push_back({ c + Vector3{ 0,0,-6 }, 2.6f });    // haunches
    obstacles.push_back({ c + Vector3{ -16,0,-13 }, 6.0f }); // pyramids
    obstacles.push_back({ c + Vector3{ 16,0,-13 }, 6.0f });
    obstacles.push_back({ c + Vector3{ -7,0,13 }, 1.0f });   // avenue obelisks
    obstacles.push_back({ c + Vector3{ 7,0,13 }, 1.0f });
    s_wisps.push_back(c + Vector3{ 0, 8.0f, 3 });
    s_wisps.push_back(c + Vector3{ -16, 6.0f, -13 });
    s_wisps.push_back(c + Vector3{ 16, 6.0f, -13 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 6.0f }); }
}

static void draw_sphinx_necropolis() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color sand{ 206,178,128,255 }, sandDk{ 184,156,108,255 }, stone{ 198,170,120,255 }, stoneDk{ 168,142,98,255 }, band{ 170,150,110,255 };

    // sand plain + ripple dunes
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, sand);
    SetRandomSeed(18000);
    for (int i = 0; i < 20; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(8.0f, boundary_radius - 1.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, rand_range(0, 180), Vector3{ rand_range(1.5f,3.2f), 0.3f, rand_range(1.0f,1.8f) }, (i & 1) ? sandDk : sand); }

    // smooth pyramids (hex cones) behind
    DrawModelEx(s_cone, c + Vector3{ -16, 0, -13 }, Vector3{ 0,1,0 }, 0, Vector3{ 12.0f, 15.0f, 12.0f }, stone);
    DrawModelEx(s_cone, c + Vector3{ -16, 0, -13 }, Vector3{ 0,1,0 }, 0, Vector3{ 12.4f, 15.3f, 12.4f }, Color{ stoneDk.r, stoneDk.g, stoneDk.b, 90 });
    DrawModelEx(s_cone, c + Vector3{ 16, 0, -13 }, Vector3{ 0,1,0 }, 0, Vector3{ 13.0f, 16.0f, 13.0f }, stone);
    DrawModelEx(s_cone, c + Vector3{ 0, 0, -20 }, Vector3{ 0,1,0 }, 0, Vector3{ 9.0f, 11.0f, 9.0f }, stoneDk);

    // the colossal sphinx (faces the player, +z)
    draw_sphinx(c, 0.0f, 1.25f, stone, stoneDk, band);

    // a processional avenue of small ram-sphinx plinths + flanking obelisks
    for (int sgn = -1; sgn <= 1; sgn += 2) {
        for (int k = 0; k < 3; k++) { Vector3 p = c + Vector3{ sgn * 6.0f, 0, 7.0f + k * 4.0f }; DrawModelEx(s_column, p + Vector3{ 0,0.6f,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 1.2f, 2.4f }, stoneDk); DrawModelEx(s_dome, p + Vector3{ 0,1.5f,0.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f,0.7f,0.9f }, stone); }
        DrawModelEx(s_cone, c + Vector3{ sgn * 7.0f, 0, 13.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 7.0f, 1.0f }, stone);   // obelisk
        DrawModelEx(s_cone, c + Vector3{ sgn * 7.0f, 7.0f, 13.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.0f, 0.6f }, Color{ 220,190,120,255 });   // gilt cap
    }
    // scattered ruined blocks
    SetRandomSeed(18100);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(9.0f, boundary_radius - 3.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_column, p + Vector3{ 0,0.5f,0 }, Vector3{ 0,1,0 }, rand_range(0, 90), Vector3{ rand_range(1.0f,2.0f), rand_range(0.8f,1.6f), rand_range(1.0f,2.0f) }, (i & 1) ? stone : stoneDk); }

    // additive: warm dawn glow + drifting sand + heat shimmer
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 210, 130, 40 });
    SetRandomSeed(303);
    for (int i = 0; i < 22; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius), sp = rand_range(0.1f, 0.3f), off = rand_range(0, 1); float y = fmodf(s_time * sp + off, 1.0f) * 5.0f; DrawSphereEx(c + Vector3{ bx, 0.5f + y, bz }, 0.05f, 4, 4, Color{ 245, 220, 160, 70 }); }
    EndBlendMode();
}

// ---- The Great Dam: a curved buttressed dam wall + spillways over a stilling basin ----
static void build_dam() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    float backZ = c.z - 16.0f;
    const int N = 18;
    for (int i = 0; i < N; i++) { float u = (i / (float)(N - 1)) - 0.5f; float x = u * 42.0f; float z = backZ + 7.0f * (u * u * 4.0f); obstacles.push_back({ c + Vector3{ x, 0, z + 1.5f }, 2.0f }); }   // dam face is a solid barrier
    s_wisps.push_back(c + Vector3{ 0, 18.0f, backZ });
    s_wisps.push_back(c + Vector3{ -9, 2.0f, backZ + 2.5f });
    s_wisps.push_back(c + Vector3{ 9, 2.0f, backZ + 2.5f });
    s_wisps.push_back(c + Vector3{ 0, 2.0f, backZ + 2.5f });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.5f }); }
}

static void draw_dam() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color crete{ 168,170,166,255 }, creteDk{ 138,140,136,255 }, creteLt{ 192,194,190,255 }, wet{ 96,108,112,255 }, water{ 130,190,205,255 }, wood{ 120,92,60,255 }, roof{ 110,116,120,255 };
    float H = 18.0f, backZ = c.z - 16.0f;

    // stilling-basin apron (the floor) + downstream tailwater channel strips
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, creteDk);
    for (int k = -1; k <= 1; k++) DrawModelEx(s_column, c + Vector3{ k * 9.0f, 0.04f, backZ + 9.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 4.0f, 0.08f, 14.0f }, wet);   // outflow channels
    // reservoir held behind the crest (a dark high water band)
    DrawModelEx(s_column, c + Vector3{ 0, H - 2.0f, backZ - 4.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 46.0f, 4.0f, 8.0f }, Color{ 70,96,112,255 });

    // the curved dam wall: panels along a parabola, with horizontal courses + buttress ribs
    const int N = 18;
    for (int i = 0; i < N; i++) {
        float u = (i / (float)(N - 1)) - 0.5f, x = u * 42.0f, z = backZ + 7.0f * (u * u * 4.0f);
        DrawModelEx(s_column, c + Vector3{ x, H * 0.5f, z }, Vector3{ 0,1,0 }, u * 28.0f, Vector3{ 2.7f, H, 4.0f }, (i & 1) ? crete : creteDk);
        if (i % 3 == 0) DrawModelEx(s_column, c + Vector3{ x, H * 0.42f, z + 2.0f }, Vector3{ 0,1,0 }, u * 28.0f, Vector3{ 1.1f, H * 0.84f, 1.4f }, creteLt);   // buttress rib
    }
    for (int k = 1; k <= 4; k++) { float y = H * k / 5.0f; DrawModelEx(s_column, c + Vector3{ 0, y, backZ + 1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 44.0f, 0.4f, 0.4f }, creteDk); }   // horizontal courses (front)
    // crest parapet + a gatehouse on top
    DrawModelEx(s_column, c + Vector3{ 0, H + 0.6f, backZ + 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 44.0f, 1.2f, 0.8f }, creteLt);
    draw_cottage(c + Vector3{ 0, H, backZ - 1.0f }, 5.0f, 3.5f, 3.0f, 1.6f, 0.0f, crete, roof, creteDk);

    // valve house in the basin (central obstacle) + sluice outlet arches at the base
    DrawModelEx(s_column, c + Vector3{ 0, 1.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f, 2.8f, 3.2f }, crete);
    DrawModelEx(s_column, c + Vector3{ 0, 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.6f, 0.4f, 3.6f }, creteLt);
    for (int s = -1; s <= 1; s++) DrawModelEx(s_column, c + Vector3{ s * 9.0f, 1.0f, backZ + 2.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 2.0f, 1.0f }, Color{ 40,46,50,255 });   // dark sluice mouths

    // --- water: spillway curtains down the face + sluice jets + tailwater shimmer ---
    float spillX[3] = { -9.0f, 0.0f, 9.0f };
    BeginBlendMode(BLEND_ALPHA);
    for (int f = 0; f < 3; f++) { float fx = spillX[f]; for (int s = 0; s < 10; s++) { float t = s / 10.0f, y = H * (1.0f - t); float wob = sinf(s_time * 3.0f + s * 0.5f + f) * (0.1f + t * 0.2f); DrawCube(c + Vector3{ fx + wob, y, backZ + 2.4f }, 3.0f, H / 10.0f + 0.2f, 0.5f, Color{ 150, 200, 215, 95 }); } }
    EndBlendMode();
    BeginBlendMode(BLEND_ADDITIVE);
    SetRandomSeed(19000);
    for (int f = 0; f < 3; f++) { float fx = spillX[f]; for (int s = 0; s < 12; s++) { float off = rand_range(0, 1); float y = H - fmodf(s_time * 7.0f + off * H, H); DrawSphereEx(c + Vector3{ fx + rand_range(-1.3f,1.3f), y, backZ + 2.6f }, 0.11f, 4, 4, Color{ 210, 235, 245, 140 }); } }
    for (int s = -1; s <= 1; s++) for (int k = 0; k < 8; k++) { float t = k / 8.0f; DrawSphereEx(c + Vector3{ s * 9.0f, 1.0f - 0.2f * t, backZ + 3.0f + t * 6.0f }, 0.3f + t * 0.2f, 6, 6, Color{ 200, 225, 235, 70 }); }   // sluice jets forward
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.6f + 0.2f * sinf(s_time * 2 + i), 8, 8, Color{ 190, 220, 235, 45 });
    SetRandomSeed(303);
    for (int i = 0; i < 22; i++) { float bx = rand_range(-18, 18), bz = rand_range(backZ + 2.0f, backZ + 12.0f); DrawSphereEx(c + Vector3{ bx, 0.4f + 1.0f * fabsf(sinf(s_time * 1.2f + i)), bz }, rand_range(0.5f, 1.2f), 6, 6, Color{ 180, 205, 215, 36 }); }   // basin spray mist
    EndBlendMode();
}

// ---- The Amphitheatre: a tiered Greek theatre — cavea + orchestra + scaenae frons ----
static void build_theatre() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    for (float th = -130.0f; th <= 130.0f; th += 10.0f) { if (fabsf(th) < 7.0f) continue; float a = th * DEG2RAD; obstacles.push_back({ c + Vector3{ sinf(a) * 10.0f, 0, cosf(a) * 10.0f }, 1.4f }); }   // inner seating wall (keep play in the orchestra; aisle at front)
    for (int i = -3; i <= 3; i++) obstacles.push_back({ c + Vector3{ i * 2.8f, 0, -13.5f }, 1.0f });   // scaenae-frons columns
    obstacles.push_back({ c + Vector3{ -7,0,-9 }, 0.8f });
    obstacles.push_back({ c + Vector3{ 7,0,-9 }, 0.8f });
    s_wisps.push_back(c + Vector3{ 0, 3.0f, -13 });
    s_wisps.push_back(c + Vector3{ -7, 1.8f, -9 });
    s_wisps.push_back(c + Vector3{ 7, 1.8f, -9 });
    s_wisps.push_back(c + Vector3{ 0, 5.0f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_theatre() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color marble{ 206,198,180,255 }, marbleDk{ 176,168,150,255 }, stone{ 190,182,164,255 }, stoneDk{ 160,152,134,255 }, seatLine{ 150,142,124,255 }, gold{ 220,184,110,255 };

    // orchestra floor + a central thymele altar
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, stone);
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 0.8f, 1.4f }, marbleDk);
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.4f, 1.0f }, marble);

    // tiered cavea: a stepped seating bank wrapping the orchestra (arc, with a front aisle)
    const int tiers = 6;
    for (int t = 0; t < tiers; t++) {
        float R = 11.0f + t * 1.9f, Y = 0.7f + t * 1.3f, dth = 3.0f / R * RAD2DEG;
        for (float th = -130.0f; th <= 130.0f; th += dth) {
            if (fabsf(th) < 7.0f) continue;
            float a = th * DEG2RAD; Vector3 p = c + Vector3{ sinf(a) * R, Y * 0.5f, cosf(a) * R };
            DrawModelEx(s_column, p, Vector3{ 0,1,0 }, th, Vector3{ 2.8f, Y, 2.0f }, (t & 1) ? marble : marbleDk);
            DrawModelEx(s_column, p + Vector3{ 0, Y * 0.5f, 0 }, Vector3{ 0,1,0 }, th, Vector3{ 2.8f, 0.12f, 2.0f }, seatLine);
        }
    }

    // --- scaenae frons (the ornate stage building) at the back, facing the orchestra ---
    Vector3 sb = c + Vector3{ 0, 0, -14.0f };
    DrawModelEx(s_column, sb + Vector3{ 0, 0.5f, 1.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 20.0f, 1.0f, 4.0f }, stoneDk);    // raised stage (proscenium)
    DrawModelEx(s_column, sb + Vector3{ 0, 1.4f, -1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 22.0f, 2.8f, 3.0f }, marbleDk);  // back wall base
    for (int i = -3; i <= 3; i++) { Vector3 cp = sb + Vector3{ i * 2.8f, 0, 0.5f }; DrawModelEx(s_cyl, cp + Vector3{ 0, 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 3.0f, 0.7f }, marble); DrawModelEx(s_column, cp + Vector3{ 0, 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.6f, 1.0f }, marbleDk); DrawModelEx(s_column, cp + Vector3{ 0, 6.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.5f, 1.0f }, marbleDk); }   // ground-floor columns + base/capital
    DrawModelEx(s_column, sb + Vector3{ 0, 6.6f, 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 21.0f, 1.0f, 1.6f }, marble);    // entablature
    for (int i = -2; i <= 2; i++) { DrawModelEx(s_cyl, sb + Vector3{ i * 4.0f, 8.7f, 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.55f, 1.6f, 0.55f }, marble); }   // upper-storey columns
    DrawModelEx(s_column, sb + Vector3{ 0, 9.4f, 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 18.0f, 0.8f, 1.4f }, marble);   // upper entablature
    DrawModelEx(s_cone, sb + Vector3{ 0, 9.8f, 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 7.0f, 2.6f, 2.0f }, marbleDk);    // pediment
    DrawModelEx(s_column, sb + Vector3{ 0, 2.2f, 1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 4.0f, 0.7f }, Color{ 30,26,22,255 });   // central grand doorway
    for (int i = -1; i <= 1; i += 2) { Vector3 st = sb + Vector3{ i * 5.6f, 0, 1.0f }; DrawModelEx(s_column, st + Vector3{ 0, 1.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 3.2f, 0.6f }, stone); DrawModelEx(s_dome, st + Vector3{ 0, 3.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.5f, 0.4f }, stone); }   // niche statues

    // braziers + a couple of fallen column drums in the orchestra
    for (int s = -1; s <= 1; s += 2) { Vector3 bp = c + Vector3{ s * 7.0f, 0, -9.0f }; DrawModelEx(s_cyl, bp + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 2.0f, 0.5f }, stoneDk); DrawModelEx(s_cyl, bp + Vector3{ 0, 2.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 0.4f, 0.8f }, gold); }
    DrawModelEx(s_cyl, c + Vector3{ -9, 0.5f, 4 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.7f, 3.0f, 0.7f }, marbleDk);

    // additive: warm sun glow + brazier fire + dust motes
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) { Color g = (i == 1 || i == 2) ? Color{ 255, 150, 60, 95 } : Color{ 255, 225, 160, 45 }; DrawSphereEx(s_wisps[i], 0.5f + 0.2f * sinf(s_time * 4 + i), 8, 8, g); }
    SetRandomSeed(303);
    for (int i = 0; i < 20; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); float y = 2.0f + 1.5f * sinf(s_time * 0.6f + i); DrawSphereEx(c + Vector3{ bx, y, bz }, 0.05f, 4, 4, Color{ 250, 235, 180, 60 }); }
    EndBlendMode();
}

// ---- The Great Wheel: a colossal slowly-turning observation wheel at dusk ----
static void build_wheel() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    float bz = -8.0f;
    obstacles.push_back({ c + Vector3{ -6, 0, bz + 2.6f }, 1.2f });   // support-leg feet (wheel is a rear backdrop)
    obstacles.push_back({ c + Vector3{ 6, 0, bz + 2.6f }, 1.2f });
    obstacles.push_back({ c + Vector3{ -6, 0, bz - 2.6f }, 1.2f });
    obstacles.push_back({ c + Vector3{ 6, 0, bz - 2.6f }, 1.2f });
    obstacles.push_back({ c + Vector3{ 0, 0, bz + 6.0f }, 1.8f });    // boarding platform
    obstacles.push_back({ c + Vector3{ -11, 0, 6 }, 1.6f });          // ticket booth
    s_wisps.push_back(c + Vector3{ 0, 12.0f, bz });                   // hub
    s_wisps.push_back(c + Vector3{ 0, 2.0f, bz + 6.0f });             // platform
    s_wisps.push_back(c + Vector3{ -11, 1.8f, 6 });
    s_wisps.push_back(c + Vector3{ 11, 1.8f, 4 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 6.0f }); }
}

static void draw_wheel() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color steel{ 120,128,140,255 }, steelDk{ 92,100,112,255 }, deck{ 120,92,60,255 }, deckDk{ 90,68,46,255 }, roof{ 120,70,80,255 };
    Color cars[4] = { { 196,70,72,255 }, { 70,120,180,255 }, { 210,170,60,255 }, { 90,160,110,255 } };
    float hubY = 12.0f, bz = -8.0f, R = 11.0f;
    Vector3 hub = c + Vector3{ 0, hubY, bz };

    // plaza floor + a central kiosk
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, Color{ 92,90,98,255 });
    DrawModelEx(s_cyl, c + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.8f, 2.0f, 1.8f }, steelDk);
    DrawModelEx(s_cone, c + Vector3{ 0, 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 1.6f, 2.4f }, roof);

    // two A-frame supports flanking the wheel plane, meeting at the axle
    for (int sgn = -1; sgn <= 1; sgn += 2) {
        float z = bz + sgn * 2.6f;
        draw_bone_seg(c + Vector3{ -6, 0, z }, hub + Vector3{ 0, 0, sgn * 2.6f }, 0.6f, steelDk);
        draw_bone_seg(c + Vector3{ 6, 0, z }, hub + Vector3{ 0, 0, sgn * 2.6f }, 0.6f, steelDk);
        draw_bone_seg(c + Vector3{ -6, 5.0f, z }, c + Vector3{ 6, 5.0f, z }, 0.3f, steelDk);   // cross brace
    }
    draw_bone_seg(hub + Vector3{ 0,0,-2.6f }, hub + Vector3{ 0,0,2.6f }, 0.55f, steel);   // axle
    draw_bone_seg(hub + Vector3{ 0,0,-0.8f }, hub + Vector3{ 0,0,0.8f }, 1.1f, steel);    // hub drum

    // rotating wheel: rim arcs + spokes + upright gondolas
    const int N = 14; float rot = s_time * 0.4f;
    Vector3 prev{ 0,0,0 };
    for (int k = 0; k <= N; k++) {
        float ang = rot + k * (2.0f * PI / N);
        Vector3 rim = hub + Vector3{ cosf(ang) * R, sinf(ang) * R, 0 };
        if (k > 0) draw_bone_seg(prev, rim, 0.22f, steel);
        if (k < N) {
            draw_bone_seg(hub, rim, 0.1f, steelDk);
            Vector3 gc = rim + Vector3{ 0, -1.5f, 0 };   // car hangs upright below the rim point
            DrawModelEx(s_cyl, rim + Vector3{ 0,-0.7f,0 }, Vector3{ 0,0,1 }, 0, Vector3{ 0.1f, 0.7f, 0.1f }, steelDk);   // hanger
            DrawModelEx(s_column, gc, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 1.2f, 1.6f }, cars[k % 4]);
            DrawModelEx(s_column, gc + Vector3{ 0, 0.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.7f, 0.2f, 1.8f }, steelDk);
        }
        prev = rim;
    }

    // boarding platform (at the wheel's foot), ticket booth, lamp posts
    DrawModelEx(s_column, c + Vector3{ 0, 1.0f, bz + 6.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 8.0f, 2.0f, 4.0f }, deck);
    DrawModelEx(s_column, c + Vector3{ 0, 2.2f, bz + 6.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 8.4f, 0.3f, 4.4f }, deckDk);
    draw_cottage(c + Vector3{ -11, 0, 6.0f }, 3.0f, 3.0f, 2.4f, 1.4f, 0.0f, deck, roof, deckDk);
    for (int sgn = -1; sgn <= 1; sgn += 2) { Vector3 lp = c + Vector3{ sgn * 9.0f, 0, 6.0f }; DrawModelEx(s_cyl, lp + Vector3{ 0,1.8f,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 3.6f, 0.16f }, steelDk); DrawModelEx(s_dome, lp + Vector3{ 0,3.6f,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f,0.4f,0.4f }, Color{ 255,210,150,255 }); }

    // additive: rotating rim string-lights + spoke beads + lamp glow + dusk haze
    BeginBlendMode(BLEND_ADDITIVE);
    for (int k = 0; k < N; k++) { float ang = rot + k * (2.0f * PI / N); Vector3 rim = hub + Vector3{ cosf(ang) * R, sinf(ang) * R, 0 }; Color lc = ((k & 1) ? Color{ 255,180,90,170 } : Color{ 120,180,255,160 }); DrawSphereEx(rim, 0.32f + 0.08f * sinf(s_time * 5 + k), 6, 6, lc); }
    for (int k = 0; k < N; k++) { float ang = rot + k * (2.0f * PI / N); for (int j = 1; j < 4; j++) { float t = j / 4.0f; DrawSphereEx(hub + Vector3{ cosf(ang) * R * t, sinf(ang) * R * t, 0 }, 0.12f, 5, 5, Color{ 255, 215, 150, 110 }); } }   // spoke fairy-lights
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 195, 120, 55 });
    SetRandomSeed(303);
    for (int i = 0; i < 16; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); float y = 1.0f + 1.5f * sinf(s_time * 0.5f + i); DrawSphereEx(c + Vector3{ bx, y, bz }, 0.06f, 4, 4, Color{ 220, 200, 230, 40 }); }
    EndBlendMode();
}

// ---- The Redwood Grove: a cathedral of colossal ancient sequoia trunks ----
// one redwood: a flared buttressed base + a tall slightly-tapering trunk of stacked segments.
static void draw_redwood(Vector3 base, float h, float r, int seed, Color bark, Color barkDk, Color barkLt) {
    SetRandomSeed(seed);
    float lean = rand_range(-0.04f, 0.04f);
    DrawModelEx(s_cyl, base, Vector3{ 0,1,0 }, 0, Vector3{ r * 1.5f, r * 0.8f, r * 1.5f }, barkDk);   // root flare
    for (int k = 0; k < 5; k++) { float a = k / 5.0f * 2 * PI; rlPushMatrix(); rlTranslatef(base.x + sinf(a) * r * 1.1f, 0, base.z + cosf(a) * r * 1.1f); rlRotatef(a * RAD2DEG, 0, 1, 0); rlRotatef(20.0f, 1, 0, 0); DrawModelEx(s_column, Vector3{ 0, r * 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 0.5f, r * 2.0f, r * 0.4f }, barkDk); rlPopMatrix(); }   // buttress wedges
    int segs = 8; float seg = h / segs; Vector3 prev = base + Vector3{ 0, r * 0.6f, 0 };
    for (int i = 0; i < segs; i++) { float t = (i + 1) / (float)segs, rr = r * (1.0f - 0.4f * t); Vector3 nxt = base + Vector3{ lean * h * t, r * 0.6f + seg * (i + 1), 0 }; draw_bone_seg(prev, nxt, rr, ((i + seed) & 1) ? bark : barkLt); prev = nxt; }
}

// trunk roster: x, z(=.y), height(=.z), radius(=.w). Shared by build (obstacles) + draw.
static void redwood_layout(std::vector<Vector4>& ts) {
    Vector3 c = boundary_center;
    SetRandomSeed(20000);
    int tries = 0;
    while ((int)ts.size() < 6 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rr = rand_range(8.0f, boundary_radius - 3.0f);
        Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr };
        if (p.z > c.z + 12.0f) continue;                       // keep the spawn aisle clear
        bool ok = true; for (auto& t : ts) if (Vector3Distance(p, Vector3{ t.x, 0, t.y }) < 7.0f) { ok = false; break; }
        if (!ok) continue;
        ts.push_back(Vector4{ p.x, p.z, rand_range(18.0f, 26.0f), rand_range(2.0f, 3.2f) });
    }
}

static void build_redwood() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    std::vector<Vector4> ts; redwood_layout(ts);
    for (auto& t : ts) obstacles.push_back({ Vector3{ t.x, 0, t.y }, t.w * 1.2f });
    obstacles.push_back({ c + Vector3{ -10, 0, 6 }, 2.0f });   // giant fallen log
    s_wisps.push_back(c + Vector3{ 0, 3.0f, 0 });
    SetRandomSeed(20100);
    for (int i = 0; i < 5; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(6.0f, boundary_radius - 4.0f); s_wisps.push_back(c + Vector3{ sinf(a) * rr, 4.0f, cosf(a) * rr }); }
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_redwood_grove() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color bark{ 124,74,52,255 }, barkDk{ 96,56,40,255 }, barkLt{ 150,92,64,255 }, moss{ 70,96,54,255 }, mossDk{ 54,78,46,255 }, fern{ 86,124,64,255 };

    // mossy forest floor + leaf-litter mounds
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, moss);
    SetRandomSeed(20300);
    for (int i = 0; i < 24; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(2.0f, boundary_radius - 1.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.0f,2.4f), rand_range(0.2f,0.5f), rand_range(1.0f,2.4f) }, (i & 1) ? mossDk : moss); }

    // the central colossal redwood (+ a dark fire-hollow at its base) and the grove
    draw_redwood(c, 24.0f, 2.6f, 999, bark, barkDk, barkLt);
    DrawModelEx(s_column, c + Vector3{ 0, 1.6f, 2.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 2.8f, 0.6f }, Color{ 18,12,10,255 });
    std::vector<Vector4> ts; redwood_layout(ts);
    for (size_t i = 0; i < ts.size(); i++) draw_redwood(Vector3{ ts[i].x, 0, ts[i].y }, ts[i].z, ts[i].w, 300 + (int)i * 53, bark, barkDk, barkLt);

    // a giant fallen log with a torn root-ball
    draw_bone_seg(c + Vector3{ -14, 1.6f, 4 }, c + Vector3{ -6, 1.4f, 8 }, 1.8f, barkDk);
    DrawModelEx(s_dome, c + Vector3{ -15, 1.6f, 3.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 3.0f, 3.0f }, barkDk);

    // ferns / undergrowth clusters
    SetRandomSeed(20400);
    for (int i = 0; i < 16; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(4.0f, boundary_radius - 2.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; for (int k = 0; k < 3; k++) DrawModelEx(s_dome, p + Vector3{ rand_range(-0.6f,0.6f), 0.3f, rand_range(-0.6f,0.6f) }, Vector3{ 0,1,0 }, rand_range(0, 180), Vector3{ rand_range(0.6f,1.1f), 0.5f, rand_range(0.4f,0.8f) }, fern); }

    // additive: dappled god-ray light shafts + drifting pollen motes + soft glow
    BeginBlendMode(BLEND_ADDITIVE);
    Vector3 shafts[4] = { c + Vector3{ 4,0,-3 }, c + Vector3{ -7,0,-6 }, c + Vector3{ 9,0,4 }, c + Vector3{ -3,0,9 } };
    for (int f = 0; f < 4; f++) { for (int s = 0; s < 9; s++) { float t = s / 9.0f; float y = 22.0f * (1.0f - t); DrawCube(shafts[f] + Vector3{ t * 3.0f, y, t * 1.5f }, 2.4f, 22.0f / 9.0f + 0.3f, 2.4f, Color{ 240, 235, 170, 18 }); } }
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 200, 230, 150, 40 });
    SetRandomSeed(303);
    for (int i = 0; i < 26; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); float y = 1.0f + 4.0f * fabsf(sinf(s_time * 0.4f + i)); DrawSphereEx(c + Vector3{ bx + sinf(s_time + i) * 0.5f, y, bz }, 0.05f, 4, 4, Color{ 235, 240, 180, 75 }); }
    EndBlendMode();
}

// ---- The Balloon Festival: a dawn launch-field of tethered hot-air balloons ----
// one balloon: a striped envelope (two domes + a stripe band) bobbing over a wicker basket,
// with riser ropes and a pair of ground tethers.
static void draw_balloon(Vector3 base, float s, Color env, Color accent, Color basket, float phase) {
    float R = 3.2f * s, bob = sinf(s_time * 1.2f + phase) * 0.35f;
    Vector3 ec = base + Vector3{ 0, 4.6f * s + bob, 0 };
    DrawModelEx(s_dome, ec, Vector3{ 0,1,0 }, 0, Vector3{ R, R * 1.15f, R }, env);                  // upper envelope
    DrawModelEx(s_dome, ec, Vector3{ 1,0,0 }, 180, Vector3{ R, R * 0.85f, R }, accent);             // lower envelope
    DrawModelEx(s_cyl, ec, Vector3{ 0,1,0 }, 0, Vector3{ R * 1.02f, 0.3f * s, R * 1.02f }, accent); // stripe band
    DrawModelEx(s_column, base + Vector3{ 0, 0.6f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f * s, 1.2f * s, 1.2f * s }, basket);   // wicker basket
    DrawModelEx(s_column, base + Vector3{ 0, 1.25f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.3f * s, 0.16f * s, 1.3f * s }, Color{ 90,70,46,255 });
    for (int k = 0; k < 4; k++) { float a = k / 4.0f * 2 * PI + 0.4f; draw_bone_seg(base + Vector3{ 0, 1.3f * s, 0 }, ec + Vector3{ cosf(a) * R * 0.55f, -R * 0.8f, sinf(a) * R * 0.55f }, 0.05f, Color{ 130,118,96,255 }); }   // riser ropes
    for (int k = 0; k < 2; k++) { float a = k * PI + 0.6f; draw_bone_seg(base + Vector3{ 0, 1.0f * s, 0 }, base + Vector3{ cosf(a) * 3.5f * s, 0, sinf(a) * 3.5f * s }, 0.04f, Color{ 120,110,90,255 }); }   // ground tethers
}

// balloon roster: x, z(=.y), scale(=.z), colorIndex(=.w). Shared by build (obstacles) + draw.
static void balloon_layout(std::vector<Vector4>& bs) {
    Vector3 c = boundary_center;
    SetRandomSeed(21000);
    int tries = 0;
    while ((int)bs.size() < 6 && tries < 300) {
        tries++;
        float a = rand_range(0, 2 * PI), rr = rand_range(8.0f, boundary_radius - 4.0f);
        Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr };
        if (p.z > c.z + 12.0f) continue;                       // keep the spawn aisle clear
        bool ok = true; for (auto& b : bs) if (Vector3Distance(p, Vector3{ b.x, 0, b.y }) < 8.0f) { ok = false; break; }
        if (!ok) continue;
        bs.push_back(Vector4{ p.x, p.z, rand_range(0.9f, 1.3f), (float)((int)bs.size() % 5) });
    }
}

static void build_balloon() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    std::vector<Vector4> bs; balloon_layout(bs);
    for (auto& b : bs) obstacles.push_back({ Vector3{ b.x, 0, b.y }, 1.4f * b.z });
    s_wisps.push_back(c + Vector3{ 0, 2.0f, 0 });
    for (auto& b : bs) s_wisps.push_back(Vector3{ b.x, 2.0f, b.y });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 4.5f }); }
}

static void draw_balloon_field() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color grass{ 104,134,78,255 }, grassDk{ 84,114,66,255 }, basket{ 150,116,72,255 };
    Color env[5] = { { 210,70,70,255 }, { 70,120,200,255 }, { 230,180,60,255 }, { 90,170,110,255 }, { 180,90,180,255 } };
    Color acc[5] = { { 240,240,240,255 }, { 240,230,120,255 }, { 200,60,60,255 }, { 240,240,240,255 }, { 240,220,120,255 } };

    // grassy launch field + tufts
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, grass);
    SetRandomSeed(21300);
    for (int i = 0; i < 22; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(2.0f, boundary_radius - 1.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.0f,2.2f), rand_range(0.2f,0.45f), rand_range(1.0f,2.2f) }, (i & 1) ? grassDk : grass); }

    // central balloon + the field
    draw_balloon(c, 1.3f, env[0], acc[0], basket, 0.0f);
    std::vector<Vector4> bs; balloon_layout(bs);
    for (size_t i = 0; i < bs.size(); i++) { int ci = (int)bs[i].w; draw_balloon(Vector3{ bs[i].x, 0, bs[i].y }, bs[i].z, env[ci], acc[ci], basket, bs[i].x + bs[i].y); }

    // a half-inflated envelope lying on the grass + a couple of fuel cylinders + sandbags
    DrawModelEx(s_dome, c + Vector3{ 13, 0, 5 }, Vector3{ 0,1,0 }, 30.0f, Vector3{ 4.2f, 1.4f, 5.4f }, env[2]);
    for (int sgn = -1; sgn <= 1; sgn += 2) { DrawModelEx(s_cyl, c + Vector3{ sgn * 3.0f, 0.5f, 3.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 1.0f, 0.4f }, Color{ 150,60,60,255 }); }

    // additive: burner flames into each envelope + morning glow + distant drifting balloons + mist
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < bs.size() + 1; i++) {
        Vector3 b = (i == 0) ? c : Vector3{ bs[i - 1].x, 0, bs[i - 1].y }; float s = (i == 0) ? 1.3f : bs[i - 1].z;
        float fl = 0.6f + 0.4f * sinf(s_time * 12.0f + b.x);
        for (int k = 0; k < 4; k++) DrawSphereEx(b + Vector3{ 0, 1.5f * s + k * 0.45f * s, 0 }, (0.36f - k * 0.06f) * s * fl, 6, 6, Color{ 255, 160, 60, (unsigned char)(150 - k * 30) });
    }
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f, 8, 8, Color{ 255, 180, 90, 40 });
    SetRandomSeed(77);
    for (int i = 0; i < 6; i++) { float bx = rand_range(-boundary_radius, boundary_radius); DrawSphereEx(c + Vector3{ bx, 15.0f + rand_range(0, 5), -boundary_radius - rand_range(2, 12) }, rand_range(1.6f, 3.0f), 8, 8, Color{ env[i % 5].r, env[i % 5].g, env[i % 5].b, 34 }); }
    SetRandomSeed(303);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); DrawSphereEx(c + Vector3{ bx, 0.6f + 0.6f * sinf(s_time * 0.5f + i), bz }, rand_range(0.8f, 1.6f), 6, 6, Color{ 220, 225, 230, 22 }); }
    EndBlendMode();
}

// ---- The Canals: a Venetian waterway of houses, humped bridges, gondolas + a campanile ----
static void draw_arched_bridge(Vector3 ctr, float span, Color stone, Color stoneDk) {
    float half = span * 0.5f, apex = 3.2f; int N = 7;
    for (int i = 0; i < N; i++) {
        float x = -half + (i + 0.5f) / N * span, y = apex * (1.0f - (x / half) * (x / half)) + 1.4f;
        DrawModelEx(s_column, ctr + Vector3{ x, y, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ span / N * 1.2f, 0.5f, 3.0f }, (i & 1) ? stone : stoneDk);
        DrawModelEx(s_column, ctr + Vector3{ x, y + 0.7f, 1.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.9f, 0.2f }, stoneDk);
        DrawModelEx(s_column, ctr + Vector3{ x, y + 0.7f, -1.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.9f, 0.2f }, stoneDk);
    }
    DrawModelEx(s_column, ctr + Vector3{ -half, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 2.0f, 3.4f }, stoneDk);
    DrawModelEx(s_column, ctr + Vector3{ half, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 2.0f, 3.4f }, stoneDk);
}

static void draw_gondola(Vector3 pos, float yaw, Color col) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y + 0.2f, pos.z);
    rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 0.5f, 5.0f }, col);                  // hull
    DrawModelEx(s_column, Vector3{ 0, 0.6f, 2.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.4f, 0.3f }, col);              // prow rise
    DrawModelEx(s_column, Vector3{ 0, 1.3f, 2.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.4f, 0.2f }, Color{ 200,180,140,255 });   // ferro comb
    DrawModelEx(s_column, Vector3{ 0, 0.6f, -2.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.0f, 0.3f }, col);            // stern rise
    rlPopMatrix();
}

static void build_canal() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    for (float z = -16.0f; z <= 14.0f; z += 2.5f) { obstacles.push_back({ c + Vector3{ -9.0f, 0, z }, 1.6f }); obstacles.push_back({ c + Vector3{ 9.0f, 0, z }, 1.6f }); }   // bank walls confine play to the channel
    obstacles.push_back({ c + Vector3{ 0, 0, -16 }, 2.4f });   // campanile base
    s_wisps.push_back(c + Vector3{ 0, 2.0f, 0 });
    s_wisps.push_back(c + Vector3{ 0, 9.0f, -16 });
    s_wisps.push_back(c + Vector3{ -9, 2.5f, 4 });
    s_wisps.push_back(c + Vector3{ 9, 2.5f, -6 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_canal() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color stone{ 182,170,150,255 }, stoneDk{ 150,138,120,255 }, brick{ 170,96,70,255 }, brickDk{ 140,76,56,255 };
    Color houses[5] = { { 206,160,96,255 }, { 190,110,90,255 }, { 210,198,160,255 }, { 170,120,110,255 }, { 200,170,120,255 } };
    Color roofc{ 150,86,72,255 }, gond{ 30,28,32,255 }, poleA{ 200,60,60,255 }, poleB{ 240,240,240,255 };

    // stone bank walkways (fondamenta) along each side (the canal between is the water plane)
    DrawModelEx(s_column, c + Vector3{ -9.5f, 0.6f, -1 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 1.4f, 40.0f }, stone);
    DrawModelEx(s_column, c + Vector3{ 9.5f, 0.6f, -1 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 1.4f, 40.0f }, stone);

    // tall canal houses on the banks, with arched-window insets facing the water
    for (int side = -1; side <= 1; side += 2) for (int k = 0; k < 5; k++) {
        float z = c.z - 15.0f + k * 6.0f; Color hc = houses[(k + (side > 0 ? 2 : 0)) % 5];
        draw_cottage(Vector3{ c.x + side * 12.0f, 1.3f, z }, 4.0f, 4.5f, 6.0f, 1.6f, side * 8.0f, hc, roofc, brickDk);
        for (int w = 0; w < 3; w++) DrawModelEx(s_column, Vector3{ c.x + side * 9.9f, 2.2f + w * 1.8f, z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 1.0f, 0.8f }, Color{ 40,46,50,255 });
    }

    // humped stone bridges crossing the canal
    float bz[3] = { 6.0f, -2.0f, -10.0f };
    for (int b = 0; b < 3; b++) draw_arched_bridge(c + Vector3{ 0, 0, bz[b] }, 17.0f, stone, stoneDk);

    // the campanile at the back
    Vector3 camp = c + Vector3{ 0, 0, -16.0f };
    for (int i = 0; i < 5; i++) DrawModelEx(s_column, camp + Vector3{ 0, 2.0f + i * 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 3.0f, 3.0f }, (i & 1) ? brick : brickDk);
    DrawModelEx(s_column, camp + Vector3{ 0, 17.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.6f, 2.0f, 3.6f }, stone);   // belfry
    DrawModelEx(s_cone, camp + Vector3{ 0, 19.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 3.0f, 2.6f }, brick);     // pyramidal cap

    // striped mooring poles + a central cluster + moored gondolas
    SetRandomSeed(22000);
    for (int i = 0; i < 10; i++) { float side = (i & 1) ? 1.0f : -1.0f, z = c.z - 14.0f + i * 3.0f; Vector3 pp = Vector3{ c.x + side * 7.0f, 0, z }; for (int s = 0; s < 4; s++) DrawModelEx(s_cyl, pp + Vector3{ 0, s * 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 0.6f, 0.18f }, (s & 1) ? poleA : poleB); }
    for (int k = 0; k < 3; k++) { float a = k / 3.0f * 2 * PI; DrawModelEx(s_cyl, c + Vector3{ cosf(a) * 1.2f, 0.8f, sinf(a) * 1.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 1.6f, 0.2f }, poleA); }
    draw_gondola(c + Vector3{ 2.5f, 0, 2 }, 20.0f, gond);
    draw_gondola(c + Vector3{ -3.0f, 0, -5 }, -30.0f, gond);
    draw_gondola(c + Vector3{ 4.0f, 0, -9 }, 200.0f, gond);

    // additive: warm lamp glow + canal water sparkle + wheeling pigeons
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 205, 140, 45 });
    SetRandomSeed(303);
    for (int i = 0; i < 22; i++) { float bx = rand_range(-7, 7), bz2 = rand_range(-16, 14); DrawSphereEx(c + Vector3{ bx, 0.12f, bz2 }, 0.22f + 0.1f * sinf(s_time * 3 + i), 5, 5, Color{ 200, 220, 210, 60 }); }
    for (int k = 0; k < 8; k++) { float ph = s_time * 0.8f + k * 0.78f; DrawSphereEx(c + Vector3{ sinf(ph) * 8.0f, 9.0f + sinf(ph * 2) * 1.5f, cosf(ph) * 8.0f - 4.0f }, 0.12f, 4, 4, Color{ 230, 225, 215, 80 }); }
    EndBlendMode();
}

// ---- The Grain Elevator: a dusty prairie grain-works of tall concrete silos ----
// one silo: a tall cylinder with hoop bands and a conical roof + chute vent.
static void draw_silo(Vector3 base, float r, float h, Color body, Color bodyDk, Color roof) {
    DrawModelEx(s_cyl, base, Vector3{ 0,1,0 }, 0, Vector3{ r, h, r }, body);
    for (int k = 1; k < 4; k++) DrawModelEx(s_cyl, base + Vector3{ 0, h * k / 4.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 1.03f, 0.25f, r * 1.03f }, bodyDk);
    DrawModelEx(s_cone, base + Vector3{ 0, h, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 1.05f, r * 0.9f, r * 1.05f }, roof);
    DrawModelEx(s_column, base + Vector3{ 0, h + r * 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.6f, 0.4f }, bodyDk);
}

static void build_silo() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    Vector3 silos[4] = { c + Vector3{ -5,0,-2 }, c + Vector3{ 5,0,-2 }, c + Vector3{ -9,0,-6 }, c + Vector3{ 9,0,-6 } };
    for (auto& s : silos) obstacles.push_back({ s, 2.6f });
    obstacles.push_back({ c + Vector3{ 0,0,-9 }, 3.0f });   // headhouse
    obstacles.push_back({ c + Vector3{ 0,0,5 }, 1.8f });    // wagon
    obstacles.push_back({ c + Vector3{ 13,0,3 }, 2.4f });   // barn
    s_wisps.push_back(c + Vector3{ 0, 3.0f, 5 });
    s_wisps.push_back(c + Vector3{ 0, 12.0f, -9 });
    s_wisps.push_back(c + Vector3{ 13, 2.0f, 3 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_silo_yard() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color body{ 198,190,172,255 }, bodyDk{ 168,160,142,255 }, roof{ 150,90,70,255 }, metal{ 120,124,130,255 }, grain{ 220,190,110,255 }, grainDk{ 196,164,92,255 }, woodDk{ 116,88,56,255 }, dirt{ 150,130,100,255 };

    // dusty yard
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, dirt);

    // the silo cluster (central + four)
    float sh = 16.0f, sr = 2.6f;
    Vector3 silos[5] = { c, c + Vector3{ -5,0,-2 }, c + Vector3{ 5,0,-2 }, c + Vector3{ -9,0,-6 }, c + Vector3{ 9,0,-6 } };
    for (int i = 0; i < 5; i++) draw_silo(silos[i], sr, sh, body, bodyDk, roof);

    // headhouse / work tower at the back + an elevated conveyor gallery on the silo tops
    Vector3 hh = c + Vector3{ 0, 0, -9 };
    DrawModelEx(s_column, hh + Vector3{ 0, 11.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.2f, 22.0f, 4.2f }, bodyDk);
    DrawModelEx(s_column, hh + Vector3{ 0, 22.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.8f, 1.6f, 4.8f }, metal);
    for (int w = 0; w < 5; w++) DrawModelEx(s_column, hh + Vector3{ 0, 4.0f + w * 3.5f, 2.15f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.0f, 0.2f }, Color{ 40,46,50,255 });
    DrawModelEx(s_column, c + Vector3{ 0, sh + 1.6f, -4 }, Vector3{ 0,1,0 }, 0, Vector3{ 20.0f, 1.6f, 2.0f }, metal);   // gallery
    DrawModelEx(s_column, c + Vector3{ 0, 9.0f, 3.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 12.0f, 0.9f }, metal);      // loading chute

    // a grain wagon below the chute (box + four wheels)
    DrawModelEx(s_column, c + Vector3{ 0, 1.1f, 5.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 1.7f, 2.2f }, Color{ 150,116,72,255 });
    for (int sgn = -1; sgn <= 1; sgn += 2) for (int z2 = -1; z2 <= 1; z2 += 2) DrawModelEx(s_cyl, c + Vector3{ sgn * 1.5f, 0.5f, 5.0f + z2 * 0.8f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.5f, 0.4f, 0.5f }, woodDk);

    // golden grain piles + a weathered barn
    SetRandomSeed(23000);
    for (int i = 0; i < 8; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(4.0f, boundary_radius - 3.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.0f,2.2f), rand_range(0.5f,1.0f), rand_range(1.0f,2.2f) }, (i & 1) ? grainDk : grain); }
    draw_cottage(c + Vector3{ 13, 0, 3 }, 5.0f, 6.0f, 4.0f, 2.4f, -20.0f, Color{ 150,70,60,255 }, Color{ 120,60,50,255 }, woodDk);

    // additive: drifting grain-dust motes + warm glow
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 210, 130, 45 });
    SetRandomSeed(303);
    for (int i = 0; i < 26; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); float y = 1.0f + 3.0f * fabsf(sinf(s_time * 0.5f + i)); DrawSphereEx(c + Vector3{ bx + sinf(s_time + i) * 0.8f, y, bz }, 0.06f, 4, 4, Color{ 235, 210, 150, 70 }); }
    EndBlendMode();
}

// ---- The Great Organ: a dim hall dominated by a colossal pipe organ ----
static void build_organ() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    float backZ = c.z - 15.0f;
    for (float x = -12.0f; x <= 12.0f; x += 3.0f) obstacles.push_back({ c + Vector3{ x, 0, backZ + 1.5f }, 1.6f });   // organ case is a back barrier
    obstacles.push_back({ c + Vector3{ -8, 0, 2 }, 0.8f });
    obstacles.push_back({ c + Vector3{ 8, 0, 2 }, 0.8f });
    s_wisps.push_back(c + Vector3{ 0, 5.0f, backZ });
    s_wisps.push_back(c + Vector3{ 0, 2.0f, 0 });
    s_wisps.push_back(c + Vector3{ -8, 3.5f, 2 });
    s_wisps.push_back(c + Vector3{ 8, 3.5f, 2 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_organ() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color floor{ 120,108,96,255 }, floorDk{ 96,86,76,255 }, wood{ 120,80,52,255 }, woodDk{ 92,60,40,255 }, gold{ 208,176,104,255 }, metal{ 184,188,200,255 }, metalDk{ 142,148,162,255 }, carpet{ 120,40,44,255 };
    float backZ = c.z - 15.0f, baseY = 3.0f, towers[3] = { -7.0f, 0.0f, 7.0f };

    // hall floor + side walls + a red carpet aisle
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, floor);
    DrawModelEx(s_column, c + Vector3{ 0, 0.02f, -2 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.0f, 0.05f, 30.0f }, carpet);
    DrawModelEx(s_column, c + Vector3{ -15, 7.0f, -2 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 14.0f, 34.0f }, floorDk);
    DrawModelEx(s_column, c + Vector3{ 15, 7.0f, -2 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 14.0f, 34.0f }, floorDk);

    // the organ case: plinth, side pilasters, cornice + gilt
    DrawModelEx(s_column, c + Vector3{ 0, 1.4f, backZ }, Vector3{ 0,1,0 }, 0, Vector3{ 26.0f, 3.0f, 3.0f }, wood);
    DrawModelEx(s_column, c + Vector3{ -12.5f, 8.0f, backZ }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 16.0f, 2.4f }, woodDk);
    DrawModelEx(s_column, c + Vector3{ 12.5f, 8.0f, backZ }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 16.0f, 2.4f }, woodDk);
    DrawModelEx(s_column, c + Vector3{ 0, 15.2f, backZ }, Vector3{ 0,1,0 }, 0, Vector3{ 26.0f, 1.6f, 2.6f }, wood);
    DrawModelEx(s_column, c + Vector3{ 0, 16.2f, backZ }, Vector3{ 0,1,0 }, 0, Vector3{ 26.0f, 0.5f, 2.8f }, gold);

    // pipe ranks (two, with a tower/flat envelope)
    for (int rank = 0; rank < 2; rank++) {
        float rz = backZ + 0.5f - rank * 0.5f; Color m = (rank == 0) ? metal : metalDk;
        for (int i = 0; i < 24; i++) {
            float x = (i - 11.5f) * 1.0f, env = 1.5f;
            for (int t = 0; t < 3; t++) { float d = fabsf(x - towers[t]); env = fmaxf(env, 7.5f - d * 1.5f); }
            float h = 3.5f + env;
            DrawModelEx(s_cyl, c + Vector3{ x, baseY, rz }, Vector3{ 0,1,0 }, 0, Vector3{ 0.34f, h, 0.34f }, m);
            DrawModelEx(s_column, c + Vector3{ x, baseY + 1.0f, rz + 0.36f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, 0.4f, 0.08f }, Color{ 28,28,32,255 });
        }
    }
    for (int t = 0; t < 3; t++) DrawModelEx(s_cone, c + Vector3{ towers[t], baseY + 11.5f, backZ + 0.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 2.0f, 1.6f }, gold);   // pinnacle caps

    // horizontal trumpet pipes (en chamade) fanning forward
    for (int k = -2; k <= 2; k++) DrawModelEx(s_cyl, c + Vector3{ k * 2.0f, 9.0f, backZ + 2.6f }, Vector3{ 1,0,0 }, 70.0f + k * 4.0f, Vector3{ 0.18f, 4.0f, 0.18f }, gold);

    // the console at centre (obstacle): case + stepped manuals + stop knobs + bench
    DrawModelEx(s_column, c + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.0f, 2.0f, 2.4f }, woodDk);
    for (int m = 0; m < 3; m++) DrawModelEx(s_column, c + Vector3{ 0, 1.7f + m * 0.3f, 0.7f - m * 0.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f, 0.12f, 0.5f }, Color{ 235,235,228,255 });
    for (int s = -3; s <= 3; s++) DrawModelEx(s_cyl, c + Vector3{ s * 0.5f, 2.1f, -0.9f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.1f, 0.3f, 0.1f }, gold);
    DrawModelEx(s_column, c + Vector3{ 0, 0.6f, 1.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 0.5f, 0.5f }, wood);

    // candelabra flanking the console
    for (int sgn = -1; sgn <= 1; sgn += 2) { Vector3 cl = c + Vector3{ sgn * 8.0f, 0, 2.0f }; DrawModelEx(s_cyl, cl + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 3.2f, 0.2f }, gold); DrawModelEx(s_dome, cl + Vector3{ 0, 3.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f,0.3f,0.4f }, gold); }

    // additive: warm gilt glow + candle flames + drifting dust motes
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 4 + i), 8, 8, Color{ 255, 190, 110, 55 });
    for (int sgn = -1; sgn <= 1; sgn += 2) DrawSphereEx(c + Vector3{ sgn * 8.0f, 3.5f, 2.0f }, 0.26f + 0.1f * sinf(s_time * 8 + sgn), 6, 6, Color{ 255, 180, 90, 160 });
    SetRandomSeed(303);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-13, 13), bz = rand_range(-13, 13); DrawSphereEx(c + Vector3{ bx, 2.0f + 1.5f * sinf(s_time * 0.5f + i), bz }, 0.05f, 4, 4, Color{ 235, 215, 170, 55 }); }
    EndBlendMode();
}

// ---- The Hypostyle Hall: a Karnak forest of colossal carved columns ----
// one column: a base + a banded shaft (hieroglyph rings) + a flared papyrus-bell capital + abacus.
static void draw_hypocolumn(Vector3 base, float r, float h, Color stone, Color stoneDk, Color band) {
    DrawModelEx(s_cyl, base, Vector3{ 0,1,0 }, 0, Vector3{ r * 1.25f, 0.6f, r * 1.25f }, stoneDk);
    DrawModelEx(s_cyl, base + Vector3{ 0, 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r, h, r }, stone);
    for (int k = 1; k < 5; k++) DrawModelEx(s_cyl, base + Vector3{ 0, h * k / 5.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 1.05f, 0.5f, r * 1.05f }, band);
    DrawModelEx(s_cone, base + Vector3{ 0, h + 2.6f, 0 }, Vector3{ 1,0,0 }, 180, Vector3{ r * 1.7f, 2.5f, r * 1.7f }, stone);   // papyrus bell (wide at top)
    DrawModelEx(s_column, base + Vector3{ 0, h + 2.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 2.2f, 0.8f, r * 2.2f }, stoneDk); // abacus
}

static void hypo_layout(std::vector<Vector3>& cols) {
    Vector3 c = boundary_center;
    float xs[4] = { -11, -5, 5, 11 }, zs[3] = { -12, -4, 4 };
    for (int i = 0; i < 4; i++) for (int j = 0; j < 3; j++) cols.push_back(c + Vector3{ xs[i], 0, zs[j] });
}

static void build_hypo() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    std::vector<Vector3> cols; hypo_layout(cols);
    for (auto& p : cols) obstacles.push_back({ p, 2.4f });
    s_wisps.push_back(c + Vector3{ 0, 2.0f, 0 });
    s_wisps.push_back(c + Vector3{ -5, 3.0f, -4 });
    s_wisps.push_back(c + Vector3{ 5, 3.0f, -4 });
    s_wisps.push_back(c + Vector3{ 0, 3.0f, -12 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_hypo() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color stone{ 190,160,112,255 }, stoneDk{ 156,128,88,255 }, band{ 120,92,60,255 }, floor{ 170,148,112,255 }, floorDk{ 146,124,92,255 }, gold{ 210,176,104,255 };
    float colH = 15.0f, colR = 2.2f;

    // floor with a hieroglyph-slab grid
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, floor);
    for (int gx = -3; gx <= 3; gx++) for (int gz = -3; gz <= 3; gz++) DrawModelEx(s_column, c + Vector3{ gx * 4.0f, 0.02f, gz * 4.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.7f, 0.05f, 3.7f }, ((gx + gz) & 1) ? floorDk : floor);

    // the colossal columns
    std::vector<Vector3> cols; hypo_layout(cols);
    for (size_t i = 0; i < cols.size(); i++) draw_hypocolumn(cols[i], colR, colH, stone, stoneDk, band);

    // central altar
    DrawModelEx(s_column, c + Vector3{ 0, 0.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 1.6f, 3.0f }, stoneDk);
    DrawModelEx(s_column, c + Vector3{ 0, 1.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 0.4f, 3.4f }, gold);

    // architrave beams on the column rows + side roof slabs (central nave left open to the clerestory)
    float zs[3] = { -12, -4, 4 };
    for (int j = 0; j < 3; j++) { DrawModelEx(s_column, c + Vector3{ -8, colH + 2.6f, zs[j] }, Vector3{ 0,1,0 }, 0, Vector3{ 7.0f, 1.4f, 2.2f }, stoneDk); DrawModelEx(s_column, c + Vector3{ 8, colH + 2.6f, zs[j] }, Vector3{ 0,1,0 }, 0, Vector3{ 7.0f, 1.4f, 2.2f }, stoneDk); }
    DrawModelEx(s_column, c + Vector3{ -11, colH + 4.2f, -4 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.0f, 1.0f, 22.0f }, stoneDk);
    DrawModelEx(s_column, c + Vector3{ 11, colH + 4.2f, -4 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.0f, 1.0f, 22.0f }, stoneDk);

    // fallen column drums
    SetRandomSeed(24100);
    for (int i = 0; i < 4; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(8.0f, boundary_radius - 4.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_cyl, p + Vector3{ 0, 1.0f, 0 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 1.6f, 2.2f, 1.6f }, stoneDk); }

    // additive: god-ray light shafts streaming down the nave + dust motes
    BeginBlendMode(BLEND_ADDITIVE);
    Vector3 shafts[3] = { c + Vector3{ 0,0,-8 }, c + Vector3{ -3,0,0 }, c + Vector3{ 3,0,8 } };
    for (int f = 0; f < 3; f++) for (int s = 0; s < 8; s++) { float t = s / 8.0f, top = colH + 5.0f; DrawCube(shafts[f] + Vector3{ t * 2.0f, top * (1.0f - t), 0 }, 2.6f, top / 8.0f + 0.3f, 2.6f, Color{ 255, 225, 150, 20 }); }
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 210, 130, 40 });
    SetRandomSeed(303);
    for (int i = 0; i < 24; i++) { float bx = rand_range(-12, 12), bz = rand_range(-14, 8); float y = 1.0f + 5.0f * fabsf(sinf(s_time * 0.4f + i)); DrawSphereEx(c + Vector3{ bx, y, bz }, 0.05f, 4, 4, Color{ 240, 220, 160, 70 }); }
    EndBlendMode();
}

// ---- The Star Fort: a hexagonal bastion fortress enclosing a parade ground ----
static void build_fort() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    float R = 20.0f;
    Vector3 verts[6];
    for (int i = 0; i < 6; i++) { float a = i * PI / 3.0f; verts[i] = c + Vector3{ sinf(a) * R, 0, cosf(a) * R }; }
    for (int i = 0; i < 6; i++) { Vector3 A = verts[i], B = verts[(i + 1) % 6]; for (int k = 0; k <= 8; k++) { Vector3 p = Vector3Lerp(A, B, k / 8.0f); obstacles.push_back({ Vector3{ p.x, 0, p.z }, 2.2f }); } }   // rampart walls
    s_wisps.push_back(c + Vector3{ 0, 4.0f, 0 });
    for (int i = 0; i < 6; i++) { Vector3 mid = Vector3Scale(Vector3Add(verts[i], verts[(i + 1) % 6]), 0.5f); s_wisps.push_back(Vector3{ mid.x, 6.0f, mid.z }); }
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.5f }); }
}

static void draw_fort() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color stone{ 150,146,134,255 }, stoneDk{ 120,116,106,255 }, earth{ 120,104,82,255 }, iron{ 60,58,62,255 }, wood{ 120,92,60,255 }, flag{ 170,60,56,255 };
    float R = 20.0f, thick = 2.0f;
    Vector3 verts[6];
    for (int i = 0; i < 6; i++) { float a = i * PI / 3.0f; verts[i] = c + Vector3{ sinf(a) * R, 0, cosf(a) * R }; }

    // parade-ground floor
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, Color{ 130,124,108,255 });

    // ramparts: earthwork base + wall + crenellations + cannons per segment; bastions at vertices
    for (int i = 0; i < 6; i++) {
        Vector3 A = verts[i], B = verts[(i + 1) % 6], mid = Vector3Scale(Vector3Add(A, B), 0.5f), d = Vector3Subtract(B, A);
        float len = sqrtf(d.x * d.x + d.z * d.z), yaw = atan2f(d.x, d.z) * RAD2DEG;
        Vector3 outw = Vector3Normalize(Vector3Subtract(mid, c));
        DrawModelEx(s_column, Vector3{ mid.x, 1.5f, mid.z }, Vector3{ 0,1,0 }, yaw, Vector3{ thick * 2.4f, 3.0f, len }, earth);   // earthwork
        DrawModelEx(s_column, Vector3{ mid.x, 4.5f, mid.z }, Vector3{ 0,1,0 }, yaw, Vector3{ thick, 5.0f, len }, stone);          // wall
        int merl = (int)(len / 2.5f);
        for (int m = 0; m < merl; m++) { Vector3 mp = Vector3Lerp(A, B, (m + 0.5f) / merl); DrawModelEx(s_column, Vector3{ mp.x, 7.4f, mp.z }, Vector3{ 0,1,0 }, yaw, Vector3{ thick * 1.1f, 1.2f, 1.0f }, stone); }
        for (int k = 0; k < 2; k++) { Vector3 cp = Vector3Lerp(A, B, 0.33f + k * 0.34f); Vector3 base = Vector3{ cp.x, 5.2f, cp.z }; DrawModelEx(s_column, base, Vector3{ 0,1,0 }, yaw, Vector3{ 1.4f, 0.8f, 1.2f }, wood); draw_bone_seg(base + Vector3{ 0,0.5f,0 }, Vector3Add(base, Vector3{ outw.x * 1.8f, 0.5f, outw.z * 1.8f }), 0.28f, iron); }   // cannons on the fire step
        // bastion: a projecting platform at the vertex
        Vector3 outV = Vector3Normalize(Vector3Subtract(A, c));
        DrawModelEx(s_column, Vector3{ A.x + outV.x * 2.5f, 2.5f, A.z + outV.z * 2.5f }, Vector3{ 0,1,0 }, atan2f(outV.x, outV.z) * RAD2DEG, Vector3{ 6.0f, 5.0f, 5.0f }, stoneDk);
    }

    // central flag mast + flag, a well, cannonball pyramids, a gatehouse arch
    DrawModelEx(s_cyl, c + Vector3{ 0, 4.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.25f, 9.0f, 0.25f }, wood);
    DrawModelEx(s_column, c + Vector3{ 1.4f, 8.0f, 0 }, Vector3{ 0,1,0 }, 8.0f * sinf(s_time * 2.0f), Vector3{ 2.4f, 1.4f, 0.1f }, flag);
    DrawModelEx(s_cyl, c + Vector3{ -4, 0.5f, 3 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 1.0f, 1.2f }, stoneDk);   // well
    for (int sgn = -1; sgn <= 1; sgn += 2) { Vector3 bp = c + Vector3{ sgn * 6.0f, 0, -3.0f }; for (int r = 0; r < 3; r++) for (int q = 0; q <= r; q++) DrawModelEx(s_dome, bp + Vector3{ (q - r * 0.5f) * 0.7f, (2 - r) * 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.34f,0.34f,0.34f }, iron); }   // cannonball pyramids
    DrawModelEx(s_column, Vector3{ verts[0].x, 4.0f, verts[0].z }, Vector3{ 0,1,0 }, atan2f(verts[0].x - c.x, verts[0].z - c.z) * RAD2DEG, Vector3{ 5.0f, 8.0f, 3.0f }, stoneDk);   // gatehouse block
    DrawModelEx(s_column, Vector3{ verts[0].x, 2.0f, verts[0].z }, Vector3{ 0,1,0 }, atan2f(verts[0].x - c.x, verts[0].z - c.z) * RAD2DEG, Vector3{ 2.0f, 3.4f, 3.4f }, Color{ 24,22,20,255 });   // gate archway (dark)

    // additive: cannon muzzle smoke + flag-side glow + drifting smoke
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 160, 80, 40 });
    for (int i = 0; i < 6; i++) { Vector3 mid = Vector3Scale(Vector3Add(verts[i], verts[(i + 1) % 6]), 0.5f); Vector3 outw = Vector3Normalize(Vector3Subtract(mid, c)); float ph = fmodf(s_time * 0.5f + i * 0.4f, 3.0f); if (ph < 0.4f) DrawSphereEx(Vector3{ mid.x + outw.x * 2.5f, 5.7f, mid.z + outw.z * 2.5f }, 0.8f + ph * 3.0f, 6, 6, Color{ 255, 180, 90, (unsigned char)(120 * (1 - ph / 0.4f)) }); }   // periodic muzzle flash/smoke
    SetRandomSeed(303);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-16, 16), bz = rand_range(-16, 16); float y = 2.0f + 4.0f * fabsf(sinf(s_time * 0.3f + i)); DrawSphereEx(c + Vector3{ bx, y, bz }, rand_range(0.8f, 1.8f), 6, 6, Color{ 150, 150, 150, 22 }); }   // drifting smoke
    EndBlendMode();
}

// ---- The Triumphal Arch: a colossal Roman arch crowned by a bronze quadriga ----
static void build_triumph() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    Vector3 ab = c + Vector3{ 0, 0, -15.0f };
    obstacles.push_back({ ab + Vector3{ -7, 0, 0 }, 2.8f });   // left pier
    obstacles.push_back({ ab + Vector3{ 7, 0, 0 }, 2.8f });    // right pier
    for (int sgn = -1; sgn <= 1; sgn += 2) { obstacles.push_back({ c + Vector3{ sgn * 10.0f, 0, 4 }, 1.0f }); obstacles.push_back({ c + Vector3{ sgn * 10.0f, 0, -4 }, 1.0f }); }   // avenue columns
    s_wisps.push_back(c + Vector3{ 0, 3.0f, 0 });
    s_wisps.push_back(ab + Vector3{ 0, 16.0f, 0 });
    for (int sgn = -1; sgn <= 1; sgn += 2) s_wisps.push_back(c + Vector3{ sgn * 6.0f, 1.6f, 6.0f });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.5f }); }
}

static void draw_triumph() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color stone{ 196,180,150,255 }, stoneDk{ 164,148,120,255 }, bronze{ 150,112,72,255 }, gold{ 212,178,108,255 }, lamp{ 255,200,130,255 };
    Vector3 ab = c + Vector3{ 0, 1.0f, -15.0f };
    float pierH = 13.0f, gap = 4.6f, pierW = 4.6f, off = gap + pierW * 0.5f;

    // paved plaza + a processional avenue (lighter paving down the centre)
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, Color{ 150,142,124,255 });
    DrawModelEx(s_column, c + Vector3{ 0, 0.02f, -4 }, Vector3{ 0,1,0 }, 0, Vector3{ 7.0f, 0.05f, 28.0f }, stoneDk);

    // --- the arch: piers, engaged columns, dark passage, voussoir ring, entablature, attic, reliefs ---
    DrawModelEx(s_column, ab + Vector3{ -off, pierH * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ pierW, pierH, 5.0f }, stone);
    DrawModelEx(s_column, ab + Vector3{ off, pierH * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ pierW, pierH, 5.0f }, stone);
    for (int sgn = -1; sgn <= 1; sgn += 2) for (int j = -1; j <= 1; j += 2) DrawModelEx(s_cyl, ab + Vector3{ sgn * off + j * 1.3f, pierH * 0.5f, 2.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, pierH * 0.92f, 0.7f }, stoneDk);
    DrawModelEx(s_column, ab + Vector3{ 0, pierH * 0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ gap * 2.0f, pierH * 0.85f, 4.0f }, Color{ 28,26,24,255 });   // dark passage
    for (int k = 0; k <= 12; k++) { float a = PI * k / 12.0f, r = gap + 0.7f; DrawModelEx(s_column, ab + Vector3{ cosf(a) * r, pierH - 0.7f + sinf(a) * r, 0 }, Vector3{ 0,0,1 }, a * RAD2DEG, Vector3{ 1.5f, 1.1f, 5.0f }, (k & 1) ? stone : stoneDk); }   // voussoirs
    DrawModelEx(s_column, ab + Vector3{ 0, pierH + 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ off * 2 + pierW, 2.2f, 5.4f }, stone);   // entablature
    DrawModelEx(s_column, ab + Vector3{ 0, pierH + 2.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ off * 2 + pierW + 0.4f, 0.5f, 5.6f }, gold);   // gilt cornice
    DrawModelEx(s_column, ab + Vector3{ 0, pierH + 4.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ off * 2 + pierW - 2, 3.2f, 4.2f }, stoneDk);   // attic
    DrawModelEx(s_column, ab + Vector3{ 0, pierH + 4.4f, 2.15f }, Vector3{ 0,1,0 }, 0, Vector3{ off * 2 + pierW - 4, 1.9f, 0.2f }, gold);   // inscription panel
    for (int sgn = -1; sgn <= 1; sgn += 2) DrawModelEx(s_column, ab + Vector3{ sgn * off, pierH * 0.55f, 2.55f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 5.4f, 0.2f }, stoneDk);   // relief panels

    // the bronze quadriga on top: chariot car + four horses + charioteer
    Vector3 qb = ab + Vector3{ 0, pierH + 6.4f, 0 };
    DrawModelEx(s_column, qb, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 1.3f, 1.6f }, bronze);
    for (int h = 0; h < 4; h++) { float hx = (h - 1.5f) * 0.95f; DrawModelEx(s_column, qb + Vector3{ hx, 0.4f, 1.9f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.7f, 2.3f }, bronze); DrawModelEx(s_column, qb + Vector3{ hx, 1.4f, 2.8f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.6f, 0.5f }, bronze); }
    DrawModelEx(s_column, qb + Vector3{ 0, 1.4f, -0.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.8f, 0.6f }, bronze);
    DrawModelEx(s_dome, qb + Vector3{ 0, 2.5f, -0.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.38f, 0.42f, 0.38f }, bronze);

    // flanking avenue columns (with eagle-cap finials) + braziers
    for (int sgn = -1; sgn <= 1; sgn += 2) for (int j = -1; j <= 1; j += 2) { Vector3 cp = c + Vector3{ sgn * 10.0f, 0, j * 4.0f }; DrawModelEx(s_cyl, cp + Vector3{ 0, 4.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 8.0f, 0.9f }, stone); DrawModelEx(s_column, cp + Vector3{ 0, 8.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 0.6f, 1.4f }, stoneDk); DrawModelEx(s_dome, cp + Vector3{ 0, 8.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.6f, 0.5f }, gold); }
    for (int sgn = -1; sgn <= 1; sgn += 2) { Vector3 bp = c + Vector3{ sgn * 6.0f, 0, 6.0f }; DrawModelEx(s_cyl, bp + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 2.0f, 0.5f }, stoneDk); DrawModelEx(s_cyl, bp + Vector3{ 0, 2.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 0.4f, 0.8f }, gold); }

    // central tropaeum (a captured-armour trophy on a post)
    DrawModelEx(s_cyl, c + Vector3{ 0, 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 4.0f, 0.3f }, stoneDk);
    DrawModelEx(s_cyl, c + Vector3{ 0, 3.4f, 0 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.16f, 2.4f, 0.16f }, stoneDk);   // crossbar
    DrawModelEx(s_column, c + Vector3{ 0, 2.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 1.6f, 0.6f }, bronze);       // cuirass
    DrawModelEx(s_dome, c + Vector3{ 0, 4.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.7f, 0.6f }, bronze);          // helmet
    DrawModelEx(s_cone, c + Vector3{ 0, 4.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.8f, 0.3f }, Color{ 170,60,56,255 });   // crest

    // additive: warm imperial glow + brazier flames + gilt sparkle + dust
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 3 + i), 8, 8, Color{ 255, 200, 120, 50 });
    for (int sgn = -1; sgn <= 1; sgn += 2) DrawSphereEx(c + Vector3{ sgn * 6.0f, 2.5f, 6.0f }, 0.3f + 0.12f * sinf(s_time * 9 + sgn), 6, 6, Color{ 255, 170, 80, 150 });
    SetRandomSeed(303);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-14, 14), bz = rand_range(-14, 8); DrawSphereEx(c + Vector3{ bx, 2.0f + 1.5f * sinf(s_time * 0.5f + i), bz }, 0.05f, 4, 4, Color{ 245, 225, 170, 55 }); }
    EndBlendMode();
}

// ---- The Orchard: rows of fruit trees in full spring blossom under a storm of petals ----
// one fruit tree: a leaning trunk + a few branches + a rounded canopy of blossom/leaf puffs.
static void draw_fruittree(Vector3 base, float h, int seed, Color trunk, Color leaf, Color blossom) {
    SetRandomSeed(seed);
    float lean = rand_range(-0.1f, 0.1f);
    Vector3 top = base + Vector3{ lean * h, h * 0.6f, 0 };
    draw_bone_seg(base, top, 0.22f, trunk);
    for (int b = 0; b < 3; b++) { float a = rand_range(0, 2 * PI); draw_bone_seg(top, top + Vector3{ cosf(a) * 1.2f, rand_range(0.5f, 1.2f), sinf(a) * 1.2f }, 0.12f, trunk); }
    Vector3 cc = top + Vector3{ 0, h * 0.4f, 0 };
    for (int k = 0; k < 8; k++) { Vector3 p = cc + Vector3{ rand_range(-1.7f,1.7f), rand_range(-0.6f,1.3f), rand_range(-1.7f,1.7f) }; float s = rand_range(1.4f, 2.4f); DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ s, s * 0.8f, s }, (k % 3 == 0) ? leaf : blossom); }
}

// tree roster: x, z(=.y), height(=.z). Shared by build (obstacles) + draw.
static void orchard_layout(std::vector<Vector4>& ts) {
    Vector3 c = boundary_center;
    SetRandomSeed(25000);
    for (float gz = -14.0f; gz <= 10.0f; gz += 6.0f) for (float gx = -14.0f; gx <= 14.0f; gx += 6.0f) {
        Vector3 p = c + Vector3{ gx + rand_range(-1.0f,1.0f), 0, gz + rand_range(-1.0f,1.0f) };
        if (Vector3Distance(p, c) < 4.0f) continue;        // keep the central great-tree clearing
        ts.push_back(Vector4{ p.x, p.z, rand_range(3.4f, 4.6f), 0 });
    }
}

static void build_orchard() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    std::vector<Vector4> ts; orchard_layout(ts);
    for (auto& t : ts) obstacles.push_back({ Vector3{ t.x, 0, t.y }, 0.7f });
    s_wisps.push_back(c + Vector3{ 0, 3.0f, 0 });
    SetRandomSeed(25100);
    for (int i = 0; i < 5; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(8.0f, boundary_radius - 4.0f); s_wisps.push_back(c + Vector3{ sinf(a) * rr, 3.0f, cosf(a) * rr }); }
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 4.5f }); }
}

static void draw_orchard() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color grass{ 116,142,80,255 }, grassDk{ 96,122,68,255 }, trunk{ 96,70,52,255 }, leaf{ 96,140,80,255 }, blossom{ 244,206,214,255 }, blossomLt{ 250,232,236,255 }, wood{ 140,108,68,255 }, woodDk{ 108,82,52,255 }, lamp{ 255,205,150,255 }, red{ 198,72,66,255 };

    // grassy floor + tufts
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, grass);
    SetRandomSeed(25300);
    for (int i = 0; i < 22; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(2.0f, boundary_radius - 1.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.8f,1.8f), 0.25f, rand_range(0.8f,1.8f) }, (i & 1) ? grassDk : grass); }

    // the central great blossom tree + the orchard rows
    draw_fruittree(c, 5.2f, 999, trunk, leaf, blossomLt);
    std::vector<Vector4> ts; orchard_layout(ts);
    for (size_t i = 0; i < ts.size(); i++) draw_fruittree(Vector3{ ts[i].x, 0, ts[i].y }, ts[i].z, 300 + (int)i * 31, trunk, leaf, (i & 1) ? blossom : blossomLt);

    // a leaning ladder, harvest baskets with apples, a handcart, a well
    DrawModelEx(s_column, c + Vector3{ 6, 2.0f, 4 }, Vector3{ 1,0,0 }, 24.0f, Vector3{ 0.2f, 4.5f, 0.2f }, woodDk);
    for (int sgn = -1; sgn <= 1; sgn += 2) { Vector3 bp = c + Vector3{ sgn * 4.0f, 0, 3.0f }; DrawModelEx(s_cyl, bp + Vector3{ 0,0.5f,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.6f, 0.7f }, wood); for (int k = 0; k < 4; k++) DrawModelEx(s_dome, bp + Vector3{ rand_range(-0.3f,0.3f), 0.9f, rand_range(-0.3f,0.3f) }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f,0.22f,0.22f }, red); }
    DrawModelEx(s_column, c + Vector3{ -8, 0.9f, 5 }, Vector3{ 0,1,0 }, 15.0f, Vector3{ 2.4f, 1.0f, 1.6f }, wood); for (int z2 = -1; z2 <= 1; z2 += 2) DrawModelEx(s_cyl, c + Vector3{ -8, 0.5f, 5 + z2 * 0.7f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.5f, 0.3f, 0.5f }, woodDk);
    DrawModelEx(s_cyl, c + Vector3{ 9, 0.6f, -3 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 1.2f, 1.2f }, woodDk);   // well

    // strung paper lanterns between a couple of trees
    for (int k = 0; k < 5; k++) { float t = (k + 0.5f) / 5.0f; Vector3 p = Vector3Lerp(c + Vector3{ -10, 4.0f, -6 }, c + Vector3{ 10, 4.0f, -6 }, t); p.y -= sinf(t * PI) * 0.8f; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.5f, 0.4f }, lamp); }

    // additive: heavy blossom-petal storm + lantern glow + soft pollen
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 210, 200, 45 });
    for (int k = 0; k < 5; k++) { float t = (k + 0.5f) / 5.0f; Vector3 p = Vector3Lerp(c + Vector3{ -10, 4.0f, -6 }, c + Vector3{ 10, 4.0f, -6 }, t); p.y -= sinf(t * PI) * 0.8f; DrawSphereEx(p, 0.3f, 6, 6, Color{ 255, 190, 110, 130 }); }
    SetRandomSeed(303);
    for (int i = 0; i < 48; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius), sp = rand_range(0.15f, 0.4f), off = rand_range(0, 1); float y = 8.0f - fmodf(s_time * sp + off, 1.0f) * 8.0f; DrawSphereEx(c + Vector3{ bx + sinf(s_time * 0.8f + i) * 1.2f, y, bz }, 0.08f, 4, 4, Color{ 250, 200, 212, 120 }); }   // petal storm
    EndBlendMode();
}

// ---- The Great Loom: a weaver's hall dominated by a colossal upright loom ----
static void build_loom() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    Vector3 lb = c + Vector3{ 0, 0, -15.0f };
    obstacles.push_back({ lb + Vector3{ -9, 0, 0 }, 1.6f });   // loom uprights
    obstacles.push_back({ lb + Vector3{ 9, 0, 0 }, 1.6f });
    for (int sgn = -1; sgn <= 1; sgn += 2) obstacles.push_back({ c + Vector3{ sgn * 12.0f, 0, 2.0f }, 1.4f });   // spinning wheels
    s_wisps.push_back(c + Vector3{ 0, 3.0f, 0 });
    s_wisps.push_back(lb + Vector3{ 0, 6.0f, 0 });
    for (int sgn = -1; sgn <= 1; sgn += 2) s_wisps.push_back(c + Vector3{ sgn * 8.0f, 2.5f, 4.0f });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_loom() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color floor{ 116,98,76,255 }, floorDk{ 94,78,60,255 }, wood{ 140,100,60,255 }, woodDk{ 104,74,46,255 }, thread{ 224,214,190,255 }, lamp{ 255,205,150,255 };
    Color tap[5] = { { 180,70,68,255 }, { 70,110,170,255 }, { 210,176,80,255 }, { 90,150,110,255 }, { 150,90,150,255 } };
    Color yarns[5] = { { 200,80,80,255 }, { 80,130,200,255 }, { 220,190,90,255 }, { 100,170,120,255 }, { 180,100,180,255 } };
    Vector3 lb = c + Vector3{ 0, 0, -15.0f };
    float fw = 20.0f, fh = 16.0f, clothY = 2.0f, shedY = 8.0f;

    // plank workshop floor
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, floor);
    for (int i = -8; i <= 8; i++) DrawModelEx(s_column, c + Vector3{ 0, 0.02f, i * 2.5f }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2, 0.04f, 0.12f }, floorDk);

    // --- the loom: uprights, warp beam, cloth beam, warp-thread curtain, woven cloth, shuttle ---
    DrawModelEx(s_column, lb + Vector3{ -fw * 0.5f, fh * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, fh, 1.6f }, woodDk);
    DrawModelEx(s_column, lb + Vector3{ fw * 0.5f, fh * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, fh, 1.6f }, woodDk);
    draw_bone_seg(lb + Vector3{ -fw * 0.5f, fh, 0 }, lb + Vector3{ fw * 0.5f, fh, 0 }, 0.8f, wood);
    draw_bone_seg(lb + Vector3{ -fw * 0.5f, clothY, 0 }, lb + Vector3{ fw * 0.5f, clothY, 0 }, 0.8f, wood);
    int nWarp = 44;
    for (int i = 0; i < nWarp; i++) { float x = (i / (float)(nWarp - 1) - 0.5f) * (fw - 3.0f); draw_bone_seg(lb + Vector3{ x, fh - 0.4f, 0 }, lb + Vector3{ x, shedY, 0 }, 0.03f, thread); }
    draw_bone_seg(lb + Vector3{ -fw * 0.5f, shedY, 0.35f }, lb + Vector3{ fw * 0.5f, shedY, 0.35f }, 0.28f, woodDk);   // shed/heddle bar
    for (int b = 0; b < 5; b++) DrawModelEx(s_column, lb + Vector3{ 0, clothY + 0.6f + b * 1.0f, 0.15f }, Vector3{ 0,1,0 }, 0, Vector3{ fw - 3.0f, 1.0f, 0.15f }, tap[b]);   // woven tapestry bands
    DrawModelEx(s_column, lb + Vector3{ 2.5f, clothY + 1.0f, 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f, 0.5f, 0.7f }, wood);   // shuttle

    // --- two spinning wheels (animated), each a stand + a vertical wheel that turns ---
    for (int sgn = -1; sgn <= 1; sgn += 2) {
        Vector3 wp = c + Vector3{ sgn * 12.0f, 0, 2.0f };
        DrawModelEx(s_column, wp + Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.8f, 3.0f }, woodDk);   // base
        DrawModelEx(s_cyl, wp + Vector3{ 0, 2.2f, 1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 1.6f, 0.2f }, wood);     // spindle post
        rlPushMatrix();
        rlTranslatef(wp.x, wp.y + 2.4f, wp.z - 1.0f);
        rlRotatef(90.0f, 0, 1, 0);                 // turn the wheel to face along x
        rlRotatef(s_time * 80.0f * sgn, 0, 0, 1);  // spin
        DrawModelEx(s_torus, Vector3{ 0,0,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 2.0f, 2.0f }, wood);
        for (int k = 0; k < 6; k++) { float a = k / 6.0f * 2 * PI; DrawModelEx(s_cyl, Vector3{ 0,0,0 }, Vector3{ 0,0,1 }, a * RAD2DEG, Vector3{ 0.07f, 2.0f, 0.07f }, woodDk); }
        rlPopMatrix();
    }

    // baskets of coloured yarn + hanging skeins on a peg rack
    SetRandomSeed(26000);
    for (int i = 0; i < 5; i++) { Vector3 bp = c + Vector3{ -8.0f + i * 4.0f, 0, 6.0f }; DrawModelEx(s_cyl, bp + Vector3{ 0,0.5f,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.6f, 0.7f }, woodDk); for (int k = 0; k < 4; k++) DrawModelEx(s_dome, bp + Vector3{ rand_range(-0.3f,0.3f), 0.9f, rand_range(-0.3f,0.3f) }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f,0.3f,0.3f }, yarns[i]); }
    DrawModelEx(s_column, c + Vector3{ 12, 5.0f, -4 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.4f, 8.0f }, woodDk); for (int k = 0; k < 4; k++) draw_bone_seg(c + Vector3{ 12, 5.0f, -6.0f + k * 1.6f }, c + Vector3{ 12, 2.6f, -6.0f + k * 1.6f }, 0.18f, yarns[k]);   // hanging skeins

    // central yarn-winding swift (a post + an X-frame holding a hank)
    DrawModelEx(s_cyl, c + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.25f, 3.2f, 0.25f }, woodDk);
    rlPushMatrix(); rlTranslatef(c.x, c.y + 2.6f, c.z); rlRotatef(s_time * 30.0f, 0, 1, 0);
    for (int k = 0; k < 4; k++) DrawModelEx(s_cyl, Vector3{ 0,0,0 }, Vector3{ 0,0,1 }, 45.0f + k * 90.0f, Vector3{ 0.08f, 2.4f, 0.08f }, wood);
    DrawModelEx(s_torus, Vector3{ 0,0,0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 1.2f, 1.2f }, yarns[2]);
    rlPopMatrix();

    // additive: warm lamp glow + drifting lint motes
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 200, 120, 50 });
    SetRandomSeed(303);
    for (int i = 0; i < 22; i++) { float bx = rand_range(-14, 14), bz = rand_range(-14, 8); float y = 1.0f + 3.0f * fabsf(sinf(s_time * 0.4f + i)); DrawSphereEx(c + Vector3{ bx, y, bz }, 0.05f, 4, 4, Color{ 235, 225, 200, 55 }); }
    EndBlendMode();
}

// ---- The Savanna: a golden grassland of baobab, acacias, termite mounds + a watering hole ----
// the iconic baobab: a swollen barrel trunk + a sparse crown of stubby "sky-root" branches.
static void draw_baobab(Vector3 base, float s, Color bark, Color barkDk, Color leaf) {
    DrawModelEx(s_cyl, base, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f * s, 8.0f * s, 2.6f * s }, bark);
    DrawModelEx(s_dome, base + Vector3{ 0, 8.0f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f * s, 1.6f * s, 2.4f * s }, bark);
    Vector3 top = base + Vector3{ 0, 9.0f * s, 0 };
    SetRandomSeed(551);
    for (int b = 0; b < 7; b++) { float a = b / 7.0f * 2 * PI; Vector3 e = top + Vector3{ cosf(a) * 3.2f * s, rand_range(1.0f, 2.6f) * s, sinf(a) * 3.2f * s }; draw_bone_seg(top, e, 0.4f * s, barkDk); DrawModelEx(s_dome, e, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f * s, 0.7f * s, 1.1f * s }, leaf); }
}

// a flat-topped acacia: a slender trunk + a wide umbrella canopy.
static void draw_acacia(Vector3 base, float h, int seed, Color bark, Color leaf) {
    SetRandomSeed(seed);
    Vector3 top = base + Vector3{ rand_range(-0.4f,0.4f), h, 0 };
    draw_bone_seg(base, top, 0.2f, bark);
    for (int b = 0; b < 4; b++) { float a = b / 4.0f * 2 * PI; draw_bone_seg(top, top + Vector3{ cosf(a) * 2.2f, 0.6f, sinf(a) * 2.2f }, 0.12f, bark); }
    DrawModelEx(s_cyl, top + Vector3{ 0, 0.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 0.5f, 3.4f }, leaf);
    DrawModelEx(s_dome, top + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.1f, 0.5f, 3.1f }, leaf);
}

// roster: x, z(=.y), kind(=.z: 0 acacia / 1 termite mound), param(=.w: height/scale).
static void savanna_layout(std::vector<Vector4>& fs) {
    Vector3 c = boundary_center;
    SetRandomSeed(27000);
    int tries = 0;
    while ((int)fs.size() < 11 && tries < 400) {
        tries++;
        float a = rand_range(0, 2 * PI), rr = rand_range(7.0f, boundary_radius - 3.0f);
        Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr };
        if (p.z > c.z + 12.0f) continue;
        bool ok = true; for (auto& f : fs) if (Vector3Distance(p, Vector3{ f.x, 0, f.y }) < 4.0f) { ok = false; break; }
        if (!ok) continue;
        int kind = ((int)fs.size() % 5 == 0) ? 1 : 0;
        fs.push_back(Vector4{ p.x, p.z, (float)kind, kind ? rand_range(3.0f, 5.5f) : rand_range(5.0f, 7.5f) });
    }
}

static void build_savanna() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    std::vector<Vector4> fs; savanna_layout(fs);
    for (auto& f : fs) obstacles.push_back({ Vector3{ f.x, 0, f.y }, ((int)f.z == 1) ? 1.4f : 0.7f });
    obstacles.push_back({ c + Vector3{ -12, 0, 5 }, 1.6f });   // kopje boulders
    s_wisps.push_back(c + Vector3{ 0, 5.0f, 0 });
    SetRandomSeed(27100);
    for (int i = 0; i < 5; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(8.0f, boundary_radius - 4.0f); s_wisps.push_back(c + Vector3{ sinf(a) * rr, 2.5f, cosf(a) * rr }); }
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 4.5f }); }
}

static void draw_savanna() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color grass{ 196,176,108,255 }, grassDk{ 172,150,92,255 }, bark{ 132,104,80,255 }, barkDk{ 104,80,60,255 }, leaf{ 110,140,82,255 }, mound{ 168,98,66,255 }, moundDk{ 140,80,54,255 }, rock{ 150,134,112,255 }, water{ 120,150,150,255 };

    // dry grassland + tufts
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, grass);
    SetRandomSeed(27300);
    for (int i = 0; i < 34; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(2.0f, boundary_radius - 1.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.6f,1.4f), rand_range(0.3f,0.7f), rand_range(0.6f,1.4f) }, (i & 1) ? grassDk : grass); }

    // the central colossal baobab + the scattered acacias and termite mounds
    draw_baobab(c, 1.0f, bark, barkDk, leaf);
    std::vector<Vector4> fs; savanna_layout(fs);
    for (size_t i = 0; i < fs.size(); i++) {
        Vector3 p = Vector3{ fs[i].x, 0, fs[i].y };
        if ((int)fs[i].z == 1) { float mh = fs[i].w; DrawModelEx(s_cone, p, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, mh, 1.4f }, mound); DrawModelEx(s_cone, p + Vector3{ 0.8f, 0, 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, mh * 0.6f, 0.8f }, moundDk); }
        else draw_acacia(p, fs[i].w, 200 + (int)i * 37, bark, leaf);
    }

    // a watering hole (a flat muddy pool) with reeds
    Vector3 wh = c + Vector3{ 10, 0, 6 };
    DrawModelEx(s_cyl, wh + Vector3{ 0, 0.04f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.0f, 0.06f, 3.2f }, water);
    DrawModelEx(s_cyl, wh + Vector3{ 0, 0.02f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.6f, 0.04f, 3.8f }, Color{ 120,96,66,255 });   // mud rim
    SetRandomSeed(27400);
    for (int i = 0; i < 6; i++) { Vector3 rp = wh + Vector3{ rand_range(-3.5f,3.5f), 0, rand_range(-2.8f,2.8f) }; draw_bone_seg(rp, rp + Vector3{ 0, rand_range(1.0f,1.8f), 0 }, 0.05f, leaf); }

    // a kopje (pile of boulders) + sun-bleached bones
    SetRandomSeed(27500);
    for (int i = 0; i < 6; i++) { Vector3 bp = c + Vector3{ -12 + rand_range(-1.5f,1.5f), rand_range(0.0f,1.2f), 5 + rand_range(-1.5f,1.5f) }; float s = rand_range(0.9f, 1.8f); DrawModelEx(s_dome, bp, Vector3{ 0,1,0 }, 0, Vector3{ s, s * 0.8f, s }, (i & 1) ? rock : Color{ 168,152,128,255 }); }
    for (int r = 0; r < 5; r++) { float t = r / 4.0f; draw_bone_seg(c + Vector3{ 5, 0.4f, 9 } + Vector3{ -1.4f + t * 2.8f, sinf(t * PI) * 0.8f, 0 }, c + Vector3{ 5, 0.1f, 9 } + Vector3{ -1.4f + t * 2.8f, 0, 1.2f }, 0.08f, Color{ 220,212,196,255 }); }   // bleached ribcage

    // additive: warm dust + heat shimmer + circling vultures (dark motes high up)
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 205, 120, 40 });
    SetRandomSeed(303);
    for (int i = 0; i < 22; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius), sp = rand_range(0.1f, 0.3f), off = rand_range(0, 1); float y = fmodf(s_time * sp + off, 1.0f) * 4.0f; DrawSphereEx(c + Vector3{ bx, 0.5f + y, bz }, 0.05f, 4, 4, Color{ 235, 210, 150, 70 }); }
    EndBlendMode();
    for (int k = 0; k < 4; k++) { float ph = s_time * 0.5f + k * 1.57f; DrawSphereEx(c + Vector3{ sinf(ph) * 12.0f, 13.0f + sinf(ph * 2) * 1.0f, cosf(ph) * 12.0f }, 0.25f, 5, 5, Color{ 30, 26, 24, 255 }); }   // vultures
}

// ---- The Great Mosque: a domed prayer hall + minarets around a fountain courtyard ----
// a slender minaret: a banded shaft + a balcony + an upper shaft + a domed cap + a crescent finial.
static void draw_minaret(Vector3 base, float h, Color stone, Color stoneDk, Color gold) {
    DrawModelEx(s_cyl, base, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, h, 1.0f }, stone);
    for (int k = 1; k < 4; k++) DrawModelEx(s_cyl, base + Vector3{ 0, h * k / 4.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.06f, 0.3f, 1.06f }, stoneDk);
    DrawModelEx(s_cyl, base + Vector3{ 0, h * 0.78f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 0.4f, 1.5f }, stone);   // balcony
    DrawModelEx(s_cyl, base + Vector3{ 0, h, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, h * 0.18f, 0.7f }, stone);      // upper shaft
    DrawModelEx(s_dome, base + Vector3{ 0, h + h * 0.18f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 1.8f, 1.0f }, gold);   // domed cap
    DrawModelEx(s_cyl, base + Vector3{ 0, h + h * 0.18f + 1.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 0.9f, 0.12f }, gold);   // finial
    DrawModelEx(s_dome, base + Vector3{ 0, h + h * 0.18f + 2.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.35f, 0.3f }, gold);
}

static void build_mosque() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    Vector3 hh = c + Vector3{ 0, 0, -14.0f };
    for (float x = -7.0f; x <= 7.0f; x += 3.5f) obstacles.push_back({ hh + Vector3{ x, 0, 0 }, 2.4f });   // prayer-hall front
    Vector3 mins[4] = { c + Vector3{ -14,0,-12 }, c + Vector3{ 14,0,-12 }, c + Vector3{ -14,0,6 }, c + Vector3{ 14,0,6 } };
    for (auto& m : mins) obstacles.push_back({ m, 1.4f });
    s_wisps.push_back(c + Vector3{ 0, 2.0f, 0 });
    s_wisps.push_back(hh + Vector3{ 0, 12.0f, 0 });
    for (auto& m : mins) s_wisps.push_back(m + Vector3{ 0, 2.0f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_mosque() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color stone{ 210,200,180,255 }, stoneDk{ 176,166,146,255 }, dome{ 110,150,170,255 }, domeDk{ 86,124,144,255 }, gold{ 214,180,108,255 }, tileA{ 196,186,166,255 }, tileB{ 120,140,150,255 };
    Vector3 hh = c + Vector3{ 0, 0, -14.0f };

    // geometric tile floor (a checkered field + a star-rosette centre)
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, stoneDk);
    for (int gx = -4; gx <= 4; gx++) for (int gz = -4; gz <= 4; gz++) DrawModelEx(s_column, c + Vector3{ gx * 3.0f, 0.02f, gz * 3.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.8f, 0.05f, 2.8f }, ((gx + gz) & 1) ? tileA : tileB);
    for (int k = 0; k < 8; k++) { float a = k / 8.0f * 2 * PI; DrawModelEx(s_column, c + Vector3{ sinf(a) * 1.8f, 0.04f, cosf(a) * 1.8f }, Vector3{ 0,1,0 }, a * RAD2DEG, Vector3{ 1.4f, 0.05f, 0.4f }, gold); }   // star rosette

    // --- prayer hall: a cubic base + a drum + a great dome + a gold base-band + a crescent finial + a grand iwan portal ---
    DrawModelEx(s_column, hh + Vector3{ 0, 4.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 16.0f, 8.0f, 8.0f }, stone);
    DrawModelEx(s_column, hh + Vector3{ 0, 8.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 16.4f, 0.8f, 8.4f }, gold);
    DrawModelEx(s_cyl, hh + Vector3{ 0, 9.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.2f, 2.2f, 5.2f }, stone);   // drum
    DrawModelEx(s_dome, hh + Vector3{ 0, 11.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.2f, 6.4f, 6.2f }, dome);  // great dome
    DrawModelEx(s_dome, hh + Vector3{ 0, 11.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.4f, 6.6f, 6.4f }, Color{ domeDk.r, domeDk.g, domeDk.b, 80 });
    DrawModelEx(s_cyl, hh + Vector3{ 0, 17.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.4f, 0.3f }, gold);   // finial spike
    DrawModelEx(s_dome, hh + Vector3{ 0, 18.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.5f, 0.5f }, gold);
    DrawModelEx(s_column, hh + Vector3{ 0, 4.5f, 4.1f }, Vector3{ 0,1,0 }, 0, Vector3{ 4.4f, 8.0f, 0.6f }, stoneDk);   // iwan frame
    DrawModelEx(s_column, hh + Vector3{ 0, 4.0f, 4.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f, 6.4f, 0.4f }, Color{ 28,26,30,255 });   // dark portal recess
    DrawModelEx(s_cone, hh + Vector3{ 0, 7.4f, 4.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.8f, 1.6f, 0.5f }, Color{ 28,26,30,255 });     // pointed-arch head

    // four slender minarets at the courtyard corners
    Vector3 mins[4] = { c + Vector3{ -14,0,-12 }, c + Vector3{ 14,0,-12 }, c + Vector3{ -14,0,6 }, c + Vector3{ 14,0,6 } };
    for (auto& m : mins) draw_minaret(m + Vector3{ 0, 9.0f, 0 }, 18.0f, stone, stoneDk, gold);

    // a pointed-arch arcade down each side of the courtyard
    for (int sgn = -1; sgn <= 1; sgn += 2) for (int j = 0; j < 5; j++) {
        float z = c.z - 11.0f + j * 4.0f; Vector3 cp = c + Vector3{ sgn * 12.0f, 0, z };
        DrawModelEx(s_cyl, cp + Vector3{ 0, 2.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 5.0f, 0.7f }, stone);
        DrawModelEx(s_column, cp + Vector3{ 0, 5.2f, 2.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 0.8f, 4.0f }, stoneDk);   // architrave to next column
        DrawModelEx(s_cone, cp + Vector3{ 0, 5.6f, 2.0f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 1.0f, 2.0f, 1.0f }, stoneDk); // pointed-arch fill
    }

    // central tiered ablution fountain
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 0.8f, 2.2f }, stone);
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.9f, 0.2f, 1.9f }, tileB);
    DrawModelEx(s_cyl, c + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 1.4f, 0.4f }, stone);
    DrawModelEx(s_cyl, c + Vector3{ 0, 2.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.3f, 1.0f }, tileB);

    // additive: warm glow + hanging-lamp lights + a central water jet + dust motes
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 205, 130, 50 });
    for (int j = 0; j < 6; j++) { float fy = fmodf(s_time * 2.0f + j * 0.18f, 1.0f); DrawSphereEx(c + Vector3{ sinf(j * 1.7f) * 0.4f, 2.6f + fy * 1.5f, cosf(j * 1.7f) * 0.4f }, 0.18f, 5, 5, Color{ 150, 200, 220, 110 }); }   // fountain jet
    SetRandomSeed(303);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-14, 14), bz = rand_range(-14, 8); DrawSphereEx(c + Vector3{ bx, 2.0f + 1.5f * sinf(s_time * 0.5f + i), bz }, 0.05f, 4, 4, Color{ 245, 230, 185, 50 }); }
    EndBlendMode();
}

// ---- The Totem Village: a Pacific-Northwest settlement of carved totem poles ----
// one totem: a pole carved into three stacked figures (bear / winged figure / thunderbird),
// painted in red/black/teal with outspread wing-boards and a beaked top.
static void draw_totem(Vector3 base, float h, Color wood, Color red, Color black, Color teal) {
    DrawModelEx(s_cyl, base, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, h, 1.0f }, wood);
    float secH = h / 3.0f;
    Vector3 b0 = base + Vector3{ 0, secH * 0.5f, 0 };   // bottom: bear
    DrawModelEx(s_column, b0, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, secH, 1.8f }, red);
    DrawModelEx(s_column, b0 + Vector3{ 0, -0.1f * secH, 1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, secH * 0.4f, 0.8f }, black);
    DrawModelEx(s_column, b0 + Vector3{ -0.6f, secH * 0.2f, 1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f,0.5f,0.3f }, black);
    DrawModelEx(s_column, b0 + Vector3{ 0.6f, secH * 0.2f, 1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f,0.5f,0.3f }, black);
    Vector3 b1 = base + Vector3{ 0, secH * 1.5f, 0 };   // middle: winged figure
    DrawModelEx(s_column, b1, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, secH, 1.6f }, teal);
    DrawModelEx(s_column, b1 + Vector3{ 0, 0, 0.9f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 0.7f, 0.7f }, black);
    DrawModelEx(s_column, b1 + Vector3{ -2.5f, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 1.2f, 0.4f }, red);
    DrawModelEx(s_column, b1 + Vector3{ 2.5f, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 1.2f, 0.4f }, red);
    Vector3 b2 = base + Vector3{ 0, secH * 2.5f, 0 };   // top: thunderbird
    DrawModelEx(s_column, b2, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, secH, 1.5f }, black);
    DrawModelEx(s_cone, b2 + Vector3{ 0, 0, 1.4f }, Vector3{ 1,0,0 }, 90, Vector3{ 0.5f, 1.4f, 0.5f }, red);   // beak
    DrawModelEx(s_column, b2 + Vector3{ -2.8f, secH * 0.3f, 0 }, Vector3{ 0,0,1 }, -15.0f, Vector3{ 3.4f, 0.5f, 0.4f }, teal);   // wings
    DrawModelEx(s_column, b2 + Vector3{ 2.8f, secH * 0.3f, 0 }, Vector3{ 0,0,1 }, 15.0f, Vector3{ 3.4f, 0.5f, 0.4f }, teal);
}

static void build_totem() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    Vector3 poles[5] = { c + Vector3{ -8,0,-2 }, c + Vector3{ 8,0,-2 }, c + Vector3{ -11,0,5 }, c + Vector3{ 11,0,5 }, c + Vector3{ 0,0,-8 } };
    for (auto& p : poles) obstacles.push_back({ p, 1.6f });
    obstacles.push_back({ c + Vector3{ 0,0,-15 }, 4.0f });   // longhouse
    obstacles.push_back({ c + Vector3{ -6,0,6 }, 1.2f });    // fire pit
    s_wisps.push_back(c + Vector3{ -6, 1.5f, 6 });           // fire pit (warm)
    s_wisps.push_back(c + Vector3{ 0, 3.0f, 0 });
    s_wisps.push_back(c + Vector3{ 0, 4.0f, -15 });
    for (auto& p : poles) s_wisps.push_back(p + Vector3{ 0, 4.0f, 0 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_totem_village() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color ground{ 96,104,84,255 }, groundDk{ 80,90,72,255 }, wood{ 118,90,64,255 }, woodDk{ 92,70,50,255 }, red{ 168,62,52,255 }, black{ 40,38,42,255 }, teal{ 70,128,124,255 }, cedar{ 56,86,60,255 }, fish{ 196,150,120,255 };

    // mossy clearing + scrub
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, ground);
    SetRandomSeed(28300);
    for (int i = 0; i < 20; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(2.0f, boundary_radius - 1.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.8f,1.6f), 0.25f, rand_range(0.8f,1.6f) }, (i & 1) ? groundDk : ground); }

    // the totem poles (central great pole + the village poles)
    draw_totem(c, 12.0f, woodDk, red, black, teal);
    Vector3 poles[5] = { c + Vector3{ -8,0,-2 }, c + Vector3{ 8,0,-2 }, c + Vector3{ -11,0,5 }, c + Vector3{ 11,0,5 }, c + Vector3{ 0,0,-8 } };
    float ph[5] = { 9.0f, 10.0f, 8.0f, 8.5f, 11.0f };
    for (int i = 0; i < 5; i++) draw_totem(poles[i], ph[i], woodDk, red, black, teal);

    // cedar-plank longhouse with a painted facade + a round entrance + a thunderbird design
    Vector3 lh = c + Vector3{ 0, 0, -15.0f };
    DrawModelEx(s_column, lh + Vector3{ 0, 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 14.0f, 6.0f, 7.0f }, wood);
    DrawModelEx(s_column, lh + Vector3{ 0, 6.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 14.6f, 1.0f, 7.6f }, woodDk);   // low gable cap
    DrawModelEx(s_cyl, lh + Vector3{ 0, 2.6f, 3.6f }, Vector3{ 0,0,1 }, 0, Vector3{ 1.4f, 0.3f, 1.4f }, black);     // round entrance
    DrawModelEx(s_column, lh + Vector3{ 0, 4.6f, 3.55f }, Vector3{ 0,1,0 }, 0, Vector3{ 6.0f, 1.2f, 0.2f }, red);   // painted band (thunderbird body)
    for (int sgn = -1; sgn <= 1; sgn += 2) DrawModelEx(s_column, lh + Vector3{ sgn * 4.0f, 4.6f, 3.55f }, Vector3{ 0,0,1 }, sgn * 12.0f, Vector3{ 4.0f, 0.6f, 0.2f }, teal);   // wings

    // a fire pit (stone ring + crossed logs) and two dugout canoes
    DrawModelEx(s_cyl, c + Vector3{ -6, 0.3f, 6 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.3f, 0.4f, 1.3f }, Color{ 110,108,104,255 });
    for (int k = 0; k < 3; k++) draw_bone_seg(c + Vector3{ -6.8f, 0.5f, 5.2f + k * 0.4f }, c + Vector3{ -5.2f, 0.5f, 6.8f - k * 0.4f }, 0.18f, woodDk);
    for (int sgn = -1; sgn <= 1; sgn += 2) { Vector3 cp = c + Vector3{ sgn * 13.0f, 0.4f, -8.0f }; DrawModelEx(s_column, cp, Vector3{ 0,1,0 }, sgn * 10.0f, Vector3{ 1.4f, 0.7f, 6.0f }, black); DrawModelEx(s_column, cp + Vector3{ 0, 0.6f, sgn * 0 + 3.2f }, Vector3{ 0,1,0 }, sgn * 10.0f, Vector3{ 0.6f, 1.4f, 0.6f }, red); }   // canoes w/ high prow

    // salmon-drying rack + cedar trees around the rim
    Vector3 rk = c + Vector3{ 7, 0, 8 };
    for (int sgn = -1; sgn <= 1; sgn += 2) DrawModelEx(s_cyl, rk + Vector3{ sgn * 2.0f, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 2.0f, 0.2f }, woodDk);
    draw_bone_seg(rk + Vector3{ -2, 2.0f, 0 }, rk + Vector3{ 2, 2.0f, 0 }, 0.1f, woodDk);
    for (int k = 0; k < 4; k++) DrawModelEx(s_column, rk + Vector3{ -1.5f + k * 1.0f, 1.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.7f, 0.15f }, fish);
    SetRandomSeed(28400);
    for (int i = 0; i < 7; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(boundary_radius - 5.0f, boundary_radius - 1.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; float th = rand_range(8.0f, 12.0f); draw_bone_seg(p, p + Vector3{ 0, th * 0.3f, 0 }, 0.4f, woodDk); for (int t = 0; t < 4; t++) { float y = th * 0.25f + t * th * 0.18f; DrawModelEx(s_cone, p + Vector3{ 0, y, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f - t * 0.4f, th * 0.28f, 2.2f - t * 0.4f }, cedar); } }

    // additive: fire glow + smoke from the longhouse + coastal mist
    BeginBlendMode(BLEND_ADDITIVE);
    DrawSphereEx(c + Vector3{ -6, 0.8f, 6 }, 0.7f + 0.3f * sinf(s_time * 9), 8, 8, Color{ 255, 140, 50, 150 });
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 180, 100, 38 });
    for (int s = 0; s < 6; s++) { float off = (float)s / 6; float y = fmodf(s_time * 0.4f + off, 1.0f) * 7.0f; DrawSphereEx(c + Vector3{ sinf(s_time + s) * 0.5f, 6.5f + y, -15 }, 0.4f + y * 0.1f, 6, 6, Color{ 180, 180, 185, 35 }); }   // longhouse smoke
    SetRandomSeed(303);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); DrawSphereEx(c + Vector3{ bx, 1.0f + 1.0f * sinf(s_time * 0.4f + i), bz }, rand_range(0.8f, 1.8f), 6, 6, Color{ 200, 215, 210, 22 }); }
    EndBlendMode();
}

// ---- The Oasis: a desert spring — a turquoise pool ringed by date palms + dunes ----
// WET level: the water plane IS the oasis pool (the fighting floor); this lays the banks,
// the central wellspring rock, the palms, reeds, a Bedouin tent and a resting camel around it.
static void build_oasis() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    Vector3 palms[8] = { c + Vector3{ -15,0,-12 }, c + Vector3{ 15,0,-12 }, c + Vector3{ -19,0,-2 }, c + Vector3{ 19,0,-2 }, c + Vector3{ -16,0,8 }, c + Vector3{ 16,0,8 }, c + Vector3{ -7,0,15 }, c + Vector3{ 7,0,15 } };
    for (auto& p : palms) obstacles.push_back({ p, 0.9f });
    obstacles.push_back({ c + Vector3{ 13,0,10 }, 2.0f });    // bedouin tent
    s_wisps.push_back(c + Vector3{ 0, 1.4f, 0 });             // cool sparkle off the wellspring
    s_wisps.push_back(c + Vector3{ 13, 2.0f, 10 });           // tent
    s_wisps.push_back(c + Vector3{ -15, 5.0f, -12 });         // palm crowns catching the sun
    s_wisps.push_back(c + Vector3{ 16, 5.0f, 8 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_oasis() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color sand{ 214,190,142,255 }, sandDk{ 188,162,116,255 }, dune{ 222,202,150,255 }, rock{ 156,144,124,255 }, rockDk{ 128,116,98,255 };
    Color trunk{ 120,86,52,255 }, frond{ 90,142,72,255 }, date{ 150,98,52,255 }, reed{ 120,154,84,255 }, cattail{ 96,72,52,255 };
    Color tent{ 66,60,56,255 }, tentDk{ 44,40,38,255 }, rope{ 150,130,96,255 }, rug{ 150,70,60,255 }, jar{ 120,92,60,255 };
    Color camel{ 178,146,104,255 }, camelDk{ 150,120,82,255 };

    // golden dunes ringing the oasis (outer, tall) + lower sandy banks (the immediate shore)
    SetRandomSeed(8801);
    for (int i = 0; i < 26; i++) { float a = (i / 26.0f) * 2 * PI; float rr = boundary_radius + rand_range(1.0f, 5.0f); Vector3 p = c + Vector3{ sinf(a) * rr, -0.5f, cosf(a) * rr }; float s = rand_range(4.0f, 8.0f); DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ s, rand_range(2.5f,5.0f), s }, (i & 1) ? dune : sand); }
    for (int i = 0; i < 22; i++) { float a = (i / 22.0f) * 2 * PI + 0.2f; float rr = boundary_radius - rand_range(1.0f, 3.0f); Vector3 p = c + Vector3{ sinf(a) * rr, -0.2f, cosf(a) * rr }; float s = rand_range(2.5f, 4.5f); DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ s, rand_range(0.8f,1.6f), s }, (i & 1) ? sand : sandDk); }

    // central wellspring: a tumble of wet rocks the pool bubbles up from, with a reed tuft
    Vector3 rk[5] = { { 0,0,0 }, { 1.5f,0,0.6f }, { -1.4f,0,0.8f }, { 0.7f,0,-1.5f }, { -0.9f,0,-1.3f } };
    float rks[5] = { 2.0f, 1.0f, 1.1f, 0.9f, 1.0f };
    for (int k = 0; k < 5; k++) DrawModelEx(s_dome, c + rk[k] + Vector3{ 0, 0.1f, 0 }, Vector3{ 0,1,0 }, k * 40.0f, Vector3{ rks[k], rks[k] * 0.7f, rks[k] }, (k & 1) ? rockDk : rock);
    for (int k = 0; k < 6; k++) { float a = k * 1.05f; Vector3 b = c + Vector3{ cosf(a) * 0.6f, 1.0f, sinf(a) * 0.6f }; draw_bone_seg(b, b + Vector3{ cosf(a) * 0.4f, 1.6f, sinf(a) * 0.4f }, 0.05f, reed); }

    // date palms ringing the pool (reused draw_palm: curving trunk + arching fronds + dates)
    Vector3 palms[8] = { c + Vector3{ -15,0,-12 }, c + Vector3{ 15,0,-12 }, c + Vector3{ -19,0,-2 }, c + Vector3{ 19,0,-2 }, c + Vector3{ -16,0,8 }, c + Vector3{ 16,0,8 }, c + Vector3{ -7,0,15 }, c + Vector3{ 7,0,15 } };
    float ph[8] = { 8.0f, 9.0f, 7.5f, 8.5f, 9.5f, 8.0f, 7.0f, 7.5f };
    for (int i = 0; i < 8; i++) draw_palm(palms[i], ph[i], trunk, frond, date);

    // reed + cattail clusters at the water's edge
    SetRandomSeed(8802);
    Vector3 reeds[5] = { c + Vector3{ -10,0,13 }, c + Vector3{ 11,0,13 }, c + Vector3{ -18,0,5 }, c + Vector3{ 17,0,-6 }, c + Vector3{ -6,0,-15 } };
    for (auto& rp : reeds) for (int k = 0; k < 9; k++) { float a = rand_range(0, 2 * PI), rr = rand_range(0.1f, 1.2f); Vector3 b = rp + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; float h = rand_range(1.2f, 2.4f); draw_bone_seg(b, b + Vector3{ rand_range(-0.2f,0.2f), h, rand_range(-0.2f,0.2f) }, 0.06f, reed); DrawModelEx(s_column, b + Vector3{ 0, h, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 0.4f, 0.12f }, cattail); }

    // a low Bedouin goat-hair tent on the bank: 4 poles, sloped cloth roof, open front
    Vector3 tp = c + Vector3{ 13, 0, 10 };
    Vector3 corn[4] = { { -3,0,-2.5f }, { 3,0,-2.5f }, { -3,0,2.5f }, { 3,0,2.5f } };
    for (auto& q : corn) DrawModelEx(s_cyl, tp + q, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 2.4f, 0.12f }, rope);
    DrawModelEx(s_column, tp + Vector3{ 0, 2.6f, -1.2f }, Vector3{ 1,0,0 }, 18.0f, Vector3{ 6.4f, 0.2f, 3.4f }, tent);    // back-sloped roof panel
    DrawModelEx(s_column, tp + Vector3{ 0, 2.5f, 1.6f }, Vector3{ 1,0,0 }, -10.0f, Vector3{ 6.4f, 0.2f, 3.0f }, tentDk);  // front-sloped roof panel
    DrawModelEx(s_column, tp + Vector3{ 0, 1.3f, -2.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 6.4f, 2.6f, 0.2f }, tentDk);      // back wall
    for (int sgn = -1; sgn <= 1; sgn += 2) DrawModelEx(s_column, tp + Vector3{ sgn * 3.1f, 1.3f, -0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 2.6f, 4.2f }, tent);   // side walls
    DrawModelEx(s_column, tp + Vector3{ 0, 0.06f, 3.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 4.0f, 0.12f, 2.6f }, rug);        // rug at the open front
    for (int j = 0; j < 3; j++) DrawModelEx(s_cyl, tp + Vector3{ -1.4f + j * 1.4f, 0.0f, 3.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.9f, 0.4f }, jar);   // water jars

    // a resting dromedary camel beside the tent
    Vector3 cm = c + Vector3{ 8, 0, 13 };
    float bodyY = 1.7f;
    for (int lx = -1; lx <= 1; lx += 2) for (int lz = -1; lz <= 1; lz += 2) DrawModelEx(s_cyl, cm + Vector3{ lx * 0.45f, 0, lz * 1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, bodyY - 0.2f, 0.18f }, camelDk);   // 4 legs
    DrawModelEx(s_column, cm + Vector3{ 0, bodyY, 0 }, Vector3{ 0,1,0 }, 18.0f, Vector3{ 1.2f, 1.1f, 3.0f }, camel);       // body
    DrawModelEx(s_dome, cm + Vector3{ 0, bodyY + 0.5f, 0 }, Vector3{ 0,1,0 }, 18.0f, Vector3{ 0.8f, 0.9f, 1.1f }, camelDk);// hump
    Vector3 shoulder = cm + Vector3{ 0.3f, bodyY + 0.2f, 1.4f };
    Vector3 head = cm + Vector3{ 0.5f, bodyY + 1.6f, 2.2f };
    draw_bone_seg(shoulder, head, 0.28f, camel);                                                                          // neck
    DrawModelEx(s_column, head + Vector3{ 0, 0.1f, 0.3f }, Vector3{ 0,1,0 }, 18.0f, Vector3{ 0.45f, 0.5f, 0.9f }, camel);  // head

    // additive: sun sparkle, the wellspring bubbling up, and heat shimmer over the dunes
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 225, 150, 40 });
    for (int j = 0; j < 8; j++) { float fy = fmodf(s_time * 1.2f + j * 0.13f, 1.0f); DrawSphereEx(c + Vector3{ sinf(j * 1.9f) * 0.6f, 0.2f + fy * 1.0f, cosf(j * 1.9f) * 0.6f }, 0.10f, 5, 5, Color{ 150, 210, 220, 90 }); }   // spring bubbles
    SetRandomSeed(8803);
    for (int i = 0; i < 16; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); DrawSphereEx(c + Vector3{ bx, 2.5f + 1.5f * sinf(s_time * 0.6f + i), bz }, rand_range(0.5f, 1.2f), 6, 6, Color{ 255, 240, 200, 16 }); }   // heat shimmer
    EndBlendMode();
}

// ---- The Pagoda: a five-tier Japanese temple in a raked-gravel zen garden ----
// DRY level: draw_pagoda lays its own gravel floor; the pagoda + torii + lanterns + sakura
// are the silhouette. A distinct East-Asian temple — not the mosque dome / Gothic nave.
static void build_pagoda() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    Vector3 lant[5] = { c + Vector3{ -10,0,4 }, c + Vector3{ 10,0,4 }, c + Vector3{ -8,0,-9 }, c + Vector3{ 8,0,-9 }, c + Vector3{ -7,0,12 } };
    for (auto& p : lant) obstacles.push_back({ p, 0.6f });
    for (auto& p : lant) s_wisps.push_back(p + Vector3{ 0, 1.8f, 0 });   // lantern fire boxes (warm)
    s_wisps.push_back(c + Vector3{ 0, 6.0f, 0 });                        // pagoda body glow
    s_wisps.push_back(c + Vector3{ 0, 14.0f, 0 });                       // upper-tier glow
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_pagoda() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color gravel{ 198,196,186,255 }, gravelDk{ 172,170,160,255 }, stone{ 150,148,140,255 }, moss{ 96,120,80,255 };
    Color wall{ 224,216,202,255 }, wood{ 122,80,58,255 }, roof{ 64,76,96,255 }, roofDk{ 48,58,76,255 };
    Color vermilion{ 178,58,46,255 }, gold{ 206,170,90,255 }, blossom{ 240,180,200,255 }, blossomDk{ 226,158,184,255 }, lanternStone{ 158,156,148,255 };

    // raked-gravel zen-garden floor + concentric raked rings around two feature rocks + moss
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, gravel);
    Vector3 feat[2] = { c + Vector3{ -13, 0, -4 }, c + Vector3{ 13, 0, 7 } };
    for (int f = 0; f < 2; f++) {
        DrawModelEx(s_dome, feat[f], Vector3{ 0,1,0 }, f * 50.0f, Vector3{ 1.6f, 1.0f, 1.2f }, stone);
        for (int r = 1; r <= 4; r++) DrawModelEx(s_torus, feat[f] + Vector3{ 0, 0.04f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f + r * 0.9f, 1.0f, 1.6f + r * 0.9f }, gravelDk);
    }
    SetRandomSeed(9100);
    for (int i = 0; i < 14; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(6.0f, boundary_radius - 2.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.8f,1.8f), 0.18f, rand_range(0.8f,1.8f) }, moss); }

    // the five-tier pagoda: each tier a plastered body + corner posts + a wide overhanging tiled roof
    float tierH = 3.0f;
    for (int t = 0; t < 5; t++) {
        float w = 6.0f - t * 0.9f;
        float by = 0.5f + t * tierH;
        DrawModelEx(s_column, c + Vector3{ 0, by + tierH * 0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, tierH * 0.84f, w }, wall);
        for (int k = 0; k < 4; k++) { float a = k * (PI * 0.5f) + PI * 0.25f; DrawModelEx(s_cyl, c + Vector3{ cosf(a) * w * 0.5f, by, sinf(a) * w * 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, tierH * 0.84f, 0.22f }, wood); }
        DrawModelEx(s_column, c + Vector3{ 0, by + 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w + 0.6f, 0.3f, w + 0.6f }, vermilion);   // balustrade band
        float rw = w + 2.6f, ry = by + tierH * 0.9f;
        DrawModelEx(s_column, c + Vector3{ 0, ry + 0.15f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rw, 0.4f, rw }, roof);                  // wide tiled roof slab
        DrawModelEx(s_cone, c + Vector3{ 0, ry + 0.35f, 0 }, Vector3{ 0,1,0 }, 45.0f, Vector3{ rw * 0.6f, 1.2f, rw * 0.6f }, roofDk); // hip cap
        for (int k = 0; k < 4; k++) { float a = k * (PI * 0.5f) + PI * 0.25f; Vector3 tip = c + Vector3{ cosf(a) * rw * 0.52f, ry + 0.5f, sinf(a) * rw * 0.52f }; DrawModelEx(s_cone, tip, Vector3{ 0,0,1 }, 0, Vector3{ 0.35f, 0.9f, 0.35f }, roofDk); }   // upturned corner eave tips
    }
    // sorin finial: central mast + stacked rings + a jewel
    Vector3 top = c + Vector3{ 0, 0.5f + 5 * tierH, 0 };
    DrawModelEx(s_cyl, top, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 3.2f, 0.18f }, gold);
    for (int r = 0; r < 6; r++) DrawModelEx(s_torus, top + Vector3{ 0, 0.5f + r * 0.42f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.55f - r * 0.05f, 1.0f, 0.55f - r * 0.05f }, gold);
    DrawModelEx(s_dome, top + Vector3{ 0, 3.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.55f, 0.4f }, gold);

    // a vermilion torii gate at the garden entrance (toward the player spawn)
    Vector3 tg = c + Vector3{ 0, 0, 16.0f };
    float tgW = 8.0f, tgH = 7.0f;
    for (int sgn = -1; sgn <= 1; sgn += 2) DrawModelEx(s_cyl, tg + Vector3{ sgn * tgW * 0.5f, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, tgH, 0.5f }, vermilion);
    DrawModelEx(s_column, tg + Vector3{ 0, tgH + 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ tgW + 3.0f, 0.7f, 1.3f }, vermilion);   // kasagi (top beam)
    for (int sgn = -1; sgn <= 1; sgn += 2) DrawModelEx(s_column, tg + Vector3{ sgn * (tgW * 0.5f + 1.6f), tgH + 0.8f, 0 }, Vector3{ 0,0,1 }, sgn * -16.0f, Vector3{ 2.0f, 0.45f, 1.3f }, vermilion);   // upturned ends
    DrawModelEx(s_column, tg + Vector3{ 0, tgH - 1.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ tgW + 0.4f, 0.5f, 0.9f }, vermilion);   // nuki (lower beam)
    DrawModelEx(s_column, tg + Vector3{ 0, tgH - 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.9f, 0.35f }, gold);             // central plaque

    // stone lanterns (ishidoro): base + post + glowing fire box + roof + finial
    Vector3 lant[5] = { c + Vector3{ -10,0,4 }, c + Vector3{ 10,0,4 }, c + Vector3{ -8,0,-9 }, c + Vector3{ 8,0,-9 }, c + Vector3{ -7,0,12 } };
    for (auto& p : lant) {
        DrawModelEx(s_cyl, p + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.4f, 0.7f }, lanternStone);
        DrawModelEx(s_cyl, p + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.28f, 1.1f, 0.28f }, lanternStone);
        DrawModelEx(s_column, p + Vector3{ 0, 1.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 0.7f, 0.8f }, lanternStone);              // fire box
        DrawModelEx(s_column, p + Vector3{ 0, 1.8f, 0.41f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.4f, 0.05f }, Color{ 40,38,40,255 });// dark window
        DrawModelEx(s_cone, p + Vector3{ 0, 2.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f, 0.7f, 1.1f }, roofDk);                     // roof
        DrawModelEx(s_dome, p + Vector3{ 0, 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.3f, 0.2f }, gold);                       // finial
    }

    // cherry-blossom trees around the rim (dark trunk + puffy pink canopy)
    SetRandomSeed(9101);
    for (int i = 0; i < 7; i++) {
        float a = (i / 7.0f) * 2 * PI + 0.3f, rr = rand_range(boundary_radius - 5.0f, boundary_radius - 1.5f);
        Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr };
        float th = rand_range(4.0f, 6.0f);
        draw_bone_seg(p, p + Vector3{ 0, th, 0 }, 0.35f, wood);
        for (int b = 0; b < 6; b++) { float ba = b * 1.05f; Vector3 cap = p + Vector3{ cosf(ba) * 1.6f, th + 0.4f + sinf(ba) * 0.5f, sinf(ba) * 1.6f }; DrawModelEx(s_dome, cap, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(1.4f,2.2f), rand_range(1.2f,1.8f), rand_range(1.4f,2.2f) }, (b & 1) ? blossom : blossomDk); }
        DrawModelEx(s_dome, p + Vector3{ 0, th + 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 1.8f, 2.2f }, blossom);
    }

    // additive: lantern glow + drifting cherry petals + a thread of incense smoke
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 200, 120, 45 });
    SetRandomSeed(9102);
    for (int i = 0; i < 30; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); float fall = fmodf(s_time * 0.25f + i * 0.11f, 1.0f); DrawSphereEx(c + Vector3{ bx + sinf(s_time + i) * 1.2f, 8.0f - fall * 8.0f, bz }, 0.10f, 5, 5, Color{ 245, 180, 205, 70 }); }   // petals
    for (int s = 0; s < 6; s++) { float y = fmodf(s_time * 0.4f + s * 0.16f, 1.0f) * 6.0f; DrawSphereEx(c + Vector3{ -7.0f + sinf(s_time + s) * 0.3f, 1.0f + y, 12.0f }, 0.18f, 6, 6, Color{ 200, 200, 195, 30 }); }   // incense by the front lantern
    EndBlendMode();
}

// ---- The Stepwell: a flooded Indian baori — a square funnel of crisscross stairs ----
// WET level: the water plane IS the pool at the bottom of the well; this lays the terraced
// stair walls (front + both sides) and a multi-story pillared pavilion on the fourth side.
static void draw_chhatri(Vector3 base, float pr, float ph, Color stone, Color domeC) {
    for (int k = 0; k < 4; k++) { float a = k * (PI * 0.5f) + PI * 0.25f; DrawModelEx(s_cyl, base + Vector3{ cosf(a) * pr, 0, sinf(a) * pr }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, ph, 0.18f }, stone); }
    DrawModelEx(s_column, base + Vector3{ 0, ph + 0.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ pr * 2.4f, 0.25f, pr * 2.4f }, stone);    // roof slab
    DrawModelEx(s_dome, base + Vector3{ 0, ph + 0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ pr * 1.1f, pr * 1.0f, pr * 1.1f }, domeC);  // dome
    DrawModelEx(s_cyl, base + Vector3{ 0, ph + 0.25f + pr, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.08f, 0.4f, 0.08f }, domeC);           // finial
}

static void build_stepwell() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    for (int i = 0; i < 16; i++) { float a = i / 16.0f * 2 * PI; obstacles.push_back({ c + Vector3{ cosf(a) * 17.0f, 0, sinf(a) * 17.0f }, 1.8f }); }   // confine the player to the flooded bottom
    obstacles.push_back({ c + Vector3{ 0, 0, 20.0f }, 4.0f });   // the pavilion (behind the spawn)
    s_wisps.push_back(c + Vector3{ 0, 12.0f, 20.0f });           // pavilion torch
    Vector3 tc[4] = { c + Vector3{ -17,3.0f,-17 }, c + Vector3{ 17,3.0f,-17 }, c + Vector3{ -17,3.0f,8 }, c + Vector3{ 17,3.0f,8 } };
    for (auto& t : tc) s_wisps.push_back(t);                     // torches on the stair walls
    s_wisps.push_back(c + Vector3{ 0, 1.0f, 0 });                // glow off the pool
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_stepwell() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color sand{ 206,170,120,255 }, sandDk{ 178,142,96,255 }, sandHi{ 224,192,142,255 }, shadow{ 146,112,74,255 }, dome{ 196,160,112,255 }, veg{ 96,128,72,255 };

    // terraced step walls on three sides (back -z + both sides) — a crisscross stair funnel
    int NT = 9; float e0 = 18.0f, estep = 1.7f, riseH = 1.4f;
    for (int t = 0; t < NT; t++) {
        float e = e0 + t * estep, y = t * riseH;
        Color sc = (t & 1) ? sand : sandDk;
        DrawModelEx(s_column, c + Vector3{ 0, y + 0.5f, -e }, Vector3{ 0,1,0 }, 0, Vector3{ 2 * e + estep, 1.0f, estep }, sc);   // back tread (-z, the forward hero wall)
        for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, c + Vector3{ s * e, y + 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ estep, 1.0f, 2 * e + estep }, sc);   // side treads
        int nb = (int)(e / 1.4f); float off = (t & 1) ? 0.7f : 0.0f;
        for (int b = -nb; b <= nb; b++) {
            float p = b * 1.4f + off;
            if (fabsf(p) > e - 0.4f) continue;
            DrawModelEx(s_column, c + Vector3{ p, y + 1.1f, -e }, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f, 0.5f, 1.1f }, sandHi);                      // back crisscross blocks
            for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, c + Vector3{ s * e, y + 1.1f, p }, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f, 0.5f, 1.1f }, sandHi);   // side crisscross blocks
        }
    }

    // the multi-story pillared pavilion on the fourth side (+z, behind the spawn)
    Vector3 pv = c + Vector3{ 0, 0, 20.0f };
    int stories = 4; float storyH = 3.6f, pw = 18.0f;
    for (int st = 0; st < stories; st++) {
        float yb = st * storyH;
        DrawModelEx(s_column, pv + Vector3{ 0, yb, 2.0f }, Vector3{ 0,1,0 }, 0, Vector3{ pw, 0.5f, 7.0f }, sandDk);                              // floor slab (extends back)
        int np = 6;
        for (int i = 0; i <= np; i++) { float x = -pw * 0.5f + i * (pw / np); DrawModelEx(s_cyl, pv + Vector3{ x, yb + storyH * 0.5f, -1.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, storyH, 0.5f }, sand); }   // front pillars (facing -z, toward the player)
        DrawModelEx(s_column, pv + Vector3{ 0, yb + storyH - 0.3f, -1.6f }, Vector3{ 0,1,0 }, 0, Vector3{ pw, 0.7f, 0.7f }, sandHi);             // lintel band
        for (int i = 0; i < np; i++) { float x = -pw * 0.5f + (i + 0.5f) * (pw / np); DrawModelEx(s_cone, pv + Vector3{ x, yb + storyH - 0.4f, -1.6f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 1.2f, 1.4f, 1.0f }, shadow); }   // pointed-arch shade between pillars
        DrawModelEx(s_column, pv + Vector3{ 0, yb + storyH * 0.5f, 3.4f }, Vector3{ 0,1,0 }, 0, Vector3{ pw, storyH, 0.5f }, shadow);            // shaded back wall
    }
    // chhatri kiosks crowning the pavilion
    draw_chhatri(pv + Vector3{ -pw * 0.4f, stories * storyH, -1.0f }, 1.2f, 2.2f, sand, dome);
    draw_chhatri(pv + Vector3{ pw * 0.4f, stories * storyH, -1.0f }, 1.2f, 2.2f, sand, dome);
    draw_chhatri(pv + Vector3{ 0, stories * storyH + 1.2f, -1.0f }, 1.7f, 2.8f, sandHi, dome);

    // corner chhatris where the stair walls meet
    float te = e0 + (NT - 1) * estep, ty = (NT - 1) * riseH + 1.4f;
    Vector3 corn[2] = { c + Vector3{ -te, ty, -te }, c + Vector3{ te, ty, -te } };
    for (auto& q : corn) draw_chhatri(q, 1.1f, 2.0f, sand, dome);

    // greenery sprouting from the cracks + a central stone wellhead kerb in the pool
    SetRandomSeed(9300);
    for (int i = 0; i < 14; i++) { int t = GetRandomValue(1, NT - 1); float e = e0 + t * estep, yv = t * riseH + 1.0f; float p = rand_range(-e + 1, e - 1); int side = GetRandomValue(0, 2); Vector3 pos = (side == 0) ? c + Vector3{ p, yv, -e } : (side == 1) ? c + Vector3{ -e, yv, p } : c + Vector3{ e, yv, p }; DrawModelEx(s_dome, pos, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.4f,0.9f), rand_range(0.5f,1.2f), rand_range(0.4f,0.9f) }, veg); }
    for (int k = 0; k < 12; k++) { float a = k / 12.0f * 2 * PI, a2 = (k + 1) / 12.0f * 2 * PI; draw_bone_seg(c + Vector3{ cosf(a) * 2.4f, 0.2f, sinf(a) * 2.4f }, c + Vector3{ cosf(a2) * 2.4f, 0.2f, sinf(a2) * 2.4f }, 0.28f, sandDk); }   // wellhead kerb ring

    // additive: torch glow on the tiers + warm dust motes + heat off the pool
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.15f * sinf(s_time * 3 + i), 8, 8, Color{ 255, 170, 80, 60 });
    SetRandomSeed(9301);
    for (int i = 0; i < 22; i++) { float bx = rand_range(-16, 16), bz = rand_range(-16, 16); DrawSphereEx(c + Vector3{ bx, 1.0f + 4.0f * fabsf(sinf(s_time * 0.3f + i)), bz }, rand_range(0.05f,0.12f), 4, 4, Color{ 240, 215, 160, 40 }); }   // dust motes
    EndBlendMode();
}

// ---- The Jantar Mantar: giant open-air masonry astronomical instruments ----
// Deliberately NOT the brass armillary rings of the Astral Observatory — these are the big
// stone instruments: a colossal triangular sundial gnomon, cylindrical Rama Yantras, a Jai
// Prakash bowl and small angled dials, on a cream stone plaza. DRY level.
static void draw_rama(Vector3 ctr, float R, Color stone, Color dk, Color ochre) {
    for (int k = 0; k < 24; k++) { float a = k / 24.0f * 2 * PI; DrawModelEx(s_column, ctr + Vector3{ cosf(a) * R, 0.7f, sinf(a) * R }, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ R * 0.27f, 1.4f, 0.5f }, (k & 1) ? stone : dk); }   // low circular wall
    for (int k = 0; k < 12; k++) { float a = k / 12.0f * 2 * PI; DrawModelEx(s_cyl, ctr + Vector3{ cosf(a) * R, 0, sinf(a) * R }, Vector3{ 0,1,0 }, 0, Vector3{ 0.28f, 2.6f, 0.28f }, stone); }   // ring of pillars
    for (int k = 0; k < 12; k++) { float a = k / 12.0f * 2 * PI; draw_bone_seg(ctr + Vector3{ 0, 0.06f, 0 }, ctr + Vector3{ cosf(a) * R, 0.06f, sinf(a) * R }, 0.05f, dk); }   // radial floor lines
    DrawModelEx(s_cyl, ctr, Vector3{ 0,1,0 }, 0, Vector3{ 0.45f, 3.2f, 0.45f }, stone);                 // central gnomon
    DrawModelEx(s_dome, ctr + Vector3{ 0, 3.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.32f, 0.34f, 0.32f }, ochre);
}

static void build_jantar() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c, 3.0f });                          // central Rama Yantra
    obstacles.push_back({ c + Vector3{ -14,0,-2 }, 3.0f });    // side Rama Yantra
    obstacles.push_back({ c + Vector3{ 14,0,-2 }, 3.0f });     // side Rama Yantra
    obstacles.push_back({ c + Vector3{ 0,0,-18 }, 5.0f });     // Samrat gnomon body
    obstacles.push_back({ c + Vector3{ -13,0,9 }, 3.0f });     // Jai Prakash bowl
    s_wisps.push_back(c + Vector3{ 0, 3.4f, 0 });              // central gnomon tip
    s_wisps.push_back(c + Vector3{ 0, 12.0f, -21 });           // Samrat apex
    s_wisps.push_back(c + Vector3{ -14, 3.0f, -2 });
    s_wisps.push_back(c + Vector3{ 14, 3.0f, -2 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_jantar() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color stone{ 214,196,160,255 }, stoneDk{ 186,166,128,255 }, stoneHi{ 230,214,182,255 }, ochre{ 172,96,72,255 }, line{ 110,86,64,255 }, dk{ 150,120,86,255 };

    // cream stone plaza + a meridian line (N-S) + radial measurement lines
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, stone);
    DrawModelEx(s_column, c + Vector3{ 0, 0.02f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.0f, boundary_radius * 2.0f }, line);
    for (int k = 0; k < 12; k++) { float a = k / 12.0f * 2 * PI; draw_bone_seg(c + Vector3{ 0, 0.06f, 0 }, c + Vector3{ cosf(a) * boundary_radius, 0.06f, sinf(a) * boundary_radius }, 0.05f, dk); }

    // --- Samrat Yantra: a colossal stepped triangular sundial gnomon at the back ---
    float H = 12.0f, Wx = 3.2f, fz = -14.0f, bz = -23.0f;   // height, width(x), front + back (z rel to c)
    int N = 14;
    for (int i = 0; i < N; i++) {
        float y = i * (H / N) + (H / N) * 0.5f;
        float frac = 1.0f - i / (float)N;
        float zfront = bz + (fz - bz) * frac;
        float zc = (zfront + bz) * 0.5f, len = (zfront - bz);
        DrawModelEx(s_column, c + Vector3{ 0, y, zc }, Vector3{ 0,1,0 }, 0, Vector3{ Wx, H / N + 0.06f, len }, (i & 1) ? stone : stoneDk);   // stacked slabs → right triangle
    }
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, c + Vector3{ s * (Wx * 0.5f + 0.06f), H * 0.5f, (fz + bz) * 0.5f }, Vector3{ 1,0,0 }, 45.0f, Vector3{ 0.22f, 0.4f, 14.0f }, ochre);   // ochre hypotenuse edge strips
    for (int i = 0; i <= 12; i++) { float t = i / 12.0f; float zz = fz + (bz - fz) * t, yy = t * H; DrawModelEx(s_column, c + Vector3{ 0, yy + 0.25f, zz }, Vector3{ 0,1,0 }, 0, Vector3{ Wx * 0.8f, 0.4f, 0.7f }, stoneHi); }   // stair treads up the gnomon
    // two curved quadrant scale walls flanking the gnomon base
    for (int side = -1; side <= 1; side += 2) {
        Vector3 cen = c + Vector3{ 0, 0, fz };
        for (int s = 0; s <= 16; s++) {
            float a = (s / 16.0f) * (PI * 0.5f);
            Vector3 p = cen + Vector3{ side * sinf(a) * 11.0f, 0.6f, -cosf(a) * 11.0f + 11.0f };
            DrawModelEx(s_column, p, Vector3{ 0,1,0 }, side * a * RAD2DEG, Vector3{ 0.5f, 1.2f, 1.1f }, (s & 1) ? stone : stoneDk);
            if (s % 2 == 0) DrawModelEx(s_column, p + Vector3{ 0, 0.7f, 0 }, Vector3{ 0,1,0 }, side * a * RAD2DEG, Vector3{ 0.18f, 0.6f, 0.18f }, ochre);   // hour ticks
        }
    }

    // --- three Rama Yantra (cylindrical) instruments: centre + two flanking ---
    draw_rama(c, 3.4f, stone, stoneDk, ochre);
    draw_rama(c + Vector3{ -14, 0, -2 }, 3.2f, stone, stoneDk, ochre);
    draw_rama(c + Vector3{ 14, 0, -2 }, 3.2f, stone, stoneDk, ochre);

    // --- Jai Prakash: a drum bowl with a crosshair wire ---
    Vector3 jp = c + Vector3{ -13, 0, 9 };
    DrawModelEx(s_cyl, jp + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.4f, 0.8f, 4.4f }, stone);
    DrawModelEx(s_cyl, jp + Vector3{ 0, 0.85f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.7f, 0.2f, 3.7f }, dk);   // dark bowl interior
    draw_bone_seg(jp + Vector3{ -3.7f, 1.5f, 0 }, jp + Vector3{ 3.7f, 1.5f, 0 }, 0.06f, ochre);
    draw_bone_seg(jp + Vector3{ 0, 1.5f, -3.7f }, jp + Vector3{ 0, 1.5f, 3.7f }, 0.06f, ochre);
    DrawModelEx(s_dome, jp + Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.3f, 0.3f }, ochre);

    // --- small angled sundials scattered on the plaza ---
    Vector3 sd[3] = { c + Vector3{ 12, 0, 9 }, c + Vector3{ 8, 0, 14 }, c + Vector3{ -6, 0, 14 } };
    for (auto& p : sd) {
        DrawModelEx(s_cyl, p + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 0.4f, 2.0f }, stoneDk);     // round base
        DrawModelEx(s_column, p + Vector3{ 0, 1.0f, 0 }, Vector3{ 1,0,0 }, 40.0f, Vector3{ 0.2f, 2.2f, 1.6f }, stoneHi); // tilted gnomon
        for (int k = 0; k < 9; k++) { float a = k / 8.0f * PI - PI * 0.5f; DrawModelEx(s_column, p + Vector3{ sinf(a) * 1.7f, 0.45f, cosf(a) * 1.7f }, Vector3{ 0,1,0 }, a * RAD2DEG, Vector3{ 0.12f, 0.4f, 0.3f }, ochre); }   // hour ticks
    }

    // additive: warm glow at the gnomon tips + dust motes in the bright air
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 210, 140, 40 });
    SetRandomSeed(9400);
    for (int i = 0; i < 20; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); DrawSphereEx(c + Vector3{ bx, 1.5f + 3.0f * fabsf(sinf(s_time * 0.3f + i)), bz }, rand_range(0.05f,0.11f), 4, 4, Color{ 240, 220, 170, 34 }); }
    EndBlendMode();
}

// ---- The Terracotta Army: an excavation pit of ranked clay warrior statues ----
// DRY level: draw_terracotta lays the dirt pit; ranks of standing warriors recede in rows
// between rammed-earth baulks, with a bronze chariot, toppled fragments and dusty light shafts.
static void draw_warrior(Vector3 b, float yaw, Color clay, Color dk) {
    rlPushMatrix();
    rlTranslatef(b.x, b.y, b.z);
    rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_cyl, Vector3{ 0, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.55f, 1.3f, 0.45f }, clay);          // flared lower robe
    DrawModelEx(s_column, Vector3{ 0, 1.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.85f, 1.1f, 0.55f }, dk);       // armoured torso
    DrawModelEx(s_column, Vector3{ -0.55f, 1.6f, 0.10f }, Vector3{ 0,0,1 }, 12.0f, Vector3{ 0.22f, 1.1f, 0.3f }, clay);   // arms
    DrawModelEx(s_column, Vector3{ 0.55f, 1.6f, 0.10f }, Vector3{ 0,0,1 }, -12.0f, Vector3{ 0.22f, 1.1f, 0.3f }, clay);
    DrawModelEx(s_dome, Vector3{ 0, 2.35f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.38f, 0.3f }, clay);        // head
    DrawModelEx(s_cyl, Vector3{ 0, 2.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 0.22f, 0.1f }, dk);            // topknot
    rlPopMatrix();
}

static void build_terracotta() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c + Vector3{ 0,0,-21 }, 5.0f });   // bronze chariot + back earthen wall
    s_wisps.push_back(c + Vector3{ -8, 6.0f, -10 });          // dusty light shafts
    s_wisps.push_back(c + Vector3{ 8, 6.0f, -14 });
    s_wisps.push_back(c + Vector3{ 0, 6.0f, -18 });
    s_wisps.push_back(c + Vector3{ 0, 2.0f, 4 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_terracotta() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color clay{ 186,118,86,255 }, clayDk{ 158,96,68,255 }, earth{ 142,112,80,255 }, earthDk{ 116,90,64,255 }, dirt{ 150,124,92,255 }, timber{ 110,84,58,255 }, bronze{ 150,120,70,255 };

    // dirt pit floor
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, dirt);

    // the ranks: rows of warriors receding into the pit, a low rammed-earth baulk before each row
    int rows = 5, perRow = 9; float z0 = -9.0f, dz = -3.0f, x0 = -16.0f, dx = 4.0f;
    for (int r = 0; r < rows; r++) {
        float z = z0 + r * dz;
        DrawModelEx(s_column, c + Vector3{ 0, 0.7f, z + 1.4f }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 - 2, 1.4f, 0.8f }, (r & 1) ? earth : earthDk);   // dividing baulk
        for (int i = 0; i < perRow; i++) {
            float x = x0 + i * dx + ((r & 1) ? 2.0f : 0.0f);
            if (fabsf(x) > boundary_radius - 3) continue;
            if ((r * 7 + i) % 11 == 0) continue;   // an occasional gap in the ranks
            draw_warrior(c + Vector3{ x, 0, z }, 0.0f, ((i % 3) ? clay : clayDk), clayDk);
        }
    }
    // the tall back earthen wall behind the last rank
    DrawModelEx(s_column, c + Vector3{ 0, 4.0f, z0 + rows * dz - 1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2, 8.0f, 1.5f }, earthDk);

    // a bronze war-chariot drawn by two clay horses at the centre-back
    Vector3 ch = c + Vector3{ 0, 0, -20.0f };
    DrawModelEx(s_column, ch + Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 0.6f, 3.0f }, bronze);                       // chariot box
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_torus, ch + Vector3{ s * 1.3f, 0.7f, 0 }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 0.9f, 0.9f, 0.9f }, earthDk);   // wheels
    for (int h = -1; h <= 1; h += 2) { Vector3 hp = ch + Vector3{ h * 0.7f, 0, 2.8f }; DrawModelEx(s_column, hp + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.3f, 1.8f }, clayDk); DrawModelEx(s_column, hp + Vector3{ 0, 1.7f, 1.0f }, Vector3{ 1,0,0 }, 32.0f, Vector3{ 0.4f, 0.5f, 1.1f }, clayDk); DrawModelEx(s_dome, hp + Vector3{ 0, 2.2f, 1.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.3f, 0.45f }, clayDk); }   // 2 horses

    // toppled statues + head fragments in the front excavation area
    SetRandomSeed(9500);
    for (int i = 0; i < 10; i++) {
        float x = rand_range(-16, 16), z = rand_range(-6, 6);
        Vector3 p = c + Vector3{ x, 0.3f, z };
        DrawModelEx(s_cyl, p, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.5f, 1.2f, 0.4f }, (i & 1) ? clay : clayDk);                          // a fallen torso lying down
        DrawModelEx(s_dome, p + Vector3{ rand_range(-1.2f,1.2f), -0.2f, rand_range(-1.2f,1.2f) }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.3f, 0.3f }, clay);   // a head fragment
    }

    // partial broken timber roof beams over the pit (the dig grid)
    for (int b = 0; b < 4; b++) { float z = -2.0f - b * 5.0f; DrawModelEx(s_column, c + Vector3{ 0, 5.8f, z }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 - 4, 0.4f, 0.5f }, timber); }   // cross beams
    for (int b = -2; b <= 2; b++) DrawModelEx(s_column, c + Vector3{ b * 8.0f, 5.8f, -12.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.4f, 28.0f }, timber);   // long beams
    DrawModelEx(s_column, c + Vector3{ -10, 3.0f, 3 }, Vector3{ 1,0,1 }, 35.0f, Vector3{ 0.5f, 0.4f, 8.0f }, timber);   // a collapsed beam

    // an excavation ramp + railing on the +x side (entry to the dig)
    Vector3 rp = c + Vector3{ 18, 0, 9 };
    DrawModelEx(s_column, rp, Vector3{ 0,0,1 }, -18.0f, Vector3{ 6.0f, 0.4f, 5.0f }, earth);
    for (int k = 0; k < 4; k++) DrawModelEx(s_cyl, rp + Vector3{ -2.5f + k * 1.6f, 1.0f, 2.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 1.2f, 0.1f }, timber);

    // additive: dusty light shafts down from the roof gaps + drifting motes
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f, 8, 8, Color{ 255, 220, 150, 30 });
    Vector3 shafts[3] = { c + Vector3{ -8, 0, -10 }, c + Vector3{ 8, 0, -14 }, c + Vector3{ 0, 0, -18 } };
    for (auto& s : shafts) DrawCylinderEx(s + Vector3{ 0, 6.5f, 0 }, s + Vector3{ 0, 0.2f, 0 }, 0.4f, 2.4f, 10, Color{ 255, 225, 160, 16 });   // a cone of light hitting the floor
    SetRandomSeed(9501);
    for (int i = 0; i < 24; i++) { float bx = rand_range(-18, 18), bz = rand_range(-20, 8); DrawSphereEx(c + Vector3{ bx, 1.0f + 3.0f * fabsf(sinf(s_time * 0.3f + i)), bz }, rand_range(0.04f,0.1f), 4, 4, Color{ 240, 220, 170, 42 }); }
    EndBlendMode();
}

// ---- The Ball Court: a Mesoamerican juego de pelota — an alley between high ring walls ----
// DRY level: draw_ballcourt lays the limestone alley; two sloped ring walls + a back temple +
// stelae + serpent heads + jungle vines form the silhouette. Not the round colosseum/theatre.
static void draw_serpenthead(Vector3 b, float yaw, Color stone, Color dk, Color jade) {
    rlPushMatrix();
    rlTranslatef(b.x, b.y, b.z);
    rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.7f, 1.7f, 2.6f }, stone);      // head block
    DrawModelEx(s_column, Vector3{ 0, 0.6f, 1.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 1.0f, 1.2f }, stone);   // upper snout
    DrawModelEx(s_column, Vector3{ 0, 0.2f, 1.8f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.3f, 0.4f, 1.0f }, dk);      // lower jaw
    for (int e = -1; e <= 1; e += 2) DrawModelEx(s_dome, Vector3{ e * 0.55f, 1.5f, 0.9f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.32f, 0.32f, 0.32f }, jade);   // eyes
    for (int f = -1; f <= 1; f += 2) DrawModelEx(s_column, Vector3{ f * 0.45f, 0.55f, 2.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 0.5f, 0.18f }, stone);   // fangs
    rlPopMatrix();
}

static void build_ballcourt() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    for (int s = -1; s <= 1; s += 2) for (float z = 12.0f; z >= -14.0f; z -= 4.5f) obstacles.push_back({ c + Vector3{ s * 10.5f, 0, z }, 1.8f });   // the two ring walls confine the alley
    obstacles.push_back({ c + Vector3{ 0,0,-19 }, 6.0f });    // back pyramid-temple
    obstacles.push_back({ c + Vector3{ 0,0,18 }, 4.0f });     // front altar platform
    for (int s = -1; s <= 1; s += 2) s_wisps.push_back(c + Vector3{ s * 9.0f, 6.0f, -1.0f });   // glow at the two goal rings
    s_wisps.push_back(c + Vector3{ 0, 11.0f, -19 });          // temple shrine
    s_wisps.push_back(c + Vector3{ 0, 1.5f, 4 });             // alley
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_ballcourt() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color stone{ 214,206,188,255 }, stoneDk{ 184,176,158,255 }, stoneHi{ 228,222,206,255 }, ochre{ 176,86,66,255 }, jade{ 86,150,120,255 }, vine{ 90,128,72,255 }, leaf{ 104,142,80,255 }, dark{ 36,32,30,255 };

    // limestone alley floor + a painted centre line + the two end-zone lines
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, stone);
    DrawModelEx(s_column, c + Vector3{ 0, 0.02f, -1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.0f, 30.0f }, ochre);
    for (int e = -1; e <= 1; e += 2) DrawModelEx(s_column, c + Vector3{ 0, 0.02f, e * 13.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 20.0f, 1.0f, 0.5f }, ochre);

    // the two ring walls: a sloped talud bench + a vertical upper wall + a red cornice + reliefs + a goal ring
    for (int s = -1; s <= 1; s += 2) {
        float wx = s * 10.0f;
        DrawModelEx(s_column, c + Vector3{ wx - s * 1.6f, 1.2f, -1.0f }, Vector3{ 0,0,1 }, s * 30.0f, Vector3{ 2.6f, 0.6f, 30.0f }, stoneDk);   // sloped talud bench
        DrawModelEx(s_column, c + Vector3{ wx, 4.6f, -1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 7.0f, 30.0f }, stone);                        // vertical upper wall
        DrawModelEx(s_column, c + Vector3{ wx - s * 0.3f, 7.9f, -1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 0.7f, 30.0f }, ochre);             // red cornice band
        for (int j = -4; j <= 4; j++) DrawModelEx(s_column, c + Vector3{ wx - s * 0.7f, 2.8f, j * 3.0f - 1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.4f, 1.6f }, (j & 1) ? stoneHi : stoneDk);   // relief panels
        DrawModelEx(s_column, c + Vector3{ wx, 6.0f, -1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 2.0f, 2.0f }, stoneHi);                       // ring mounting block
        DrawModelEx(s_torus, c + Vector3{ wx - s * 0.9f, 6.0f, -1.0f }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 1.5f, 1.5f, 1.5f }, ochre);           // VERTICAL stone goal ring
        DrawModelEx(s_torus, c + Vector3{ wx - s * 0.95f, 6.0f, -1.0f }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 1.2f, 1.2f, 1.2f }, jade);           // inner jade band
    }

    // the back pyramid-temple (the tall landmark) with a central stair, a shrine + roof-comb
    Vector3 bt = c + Vector3{ 0, 0, -19.0f };
    for (int t = 0; t < 4; t++) { float w = 14.0f - t * 2.8f; DrawModelEx(s_column, bt + Vector3{ 0, t * 2.0f + 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, 2.0f, w * 0.6f }, (t & 1) ? stone : stoneDk); }
    for (int s = 0; s < 6; s++) DrawModelEx(s_column, bt + Vector3{ 0, s * 1.3f + 0.6f, 4.0f - s * 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 0.6f, 1.0f }, stoneHi);   // stair
    DrawModelEx(s_column, bt + Vector3{ 0, 9.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.0f, 3.0f, 3.0f }, stone);            // shrine
    DrawModelEx(s_column, bt + Vector3{ 0, 9.0f, 1.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 2.4f, 0.4f }, dark);          // dark doorway
    DrawModelEx(s_column, bt + Vector3{ 0, 11.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.6f, 1.0f, 3.6f }, ochre);           // roof base
    DrawModelEx(s_column, bt + Vector3{ 0, 13.2f, -0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 4.2f, 2.4f, 0.4f }, stoneHi);     // roof comb crest
    draw_serpenthead(c + Vector3{ -3.0f, 0, -14.0f }, 200.0f, stone, stoneDk, jade);
    draw_serpenthead(c + Vector3{ 3.0f, 0, -14.0f }, 160.0f, stone, stoneDk, jade);

    // a low front altar platform behind the spawn
    Vector3 ft = c + Vector3{ 0, 0, 18.0f };
    DrawModelEx(s_column, ft + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 12.0f, 1.2f, 5.0f }, stoneDk);
    DrawModelEx(s_column, ft + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.0f, 1.0f, 3.0f }, stone);

    // carved stelae at the four corners
    Vector3 st[4] = { c + Vector3{ -12,0,12 }, c + Vector3{ 12,0,12 }, c + Vector3{ -12,0,-14 }, c + Vector3{ 12,0,-14 } };
    for (auto& p : st) { DrawModelEx(s_column, p + Vector3{ 0, 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 4.0f, 0.8f }, stone); DrawModelEx(s_column, p + Vector3{ 0, 2.2f, 0.45f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 3.4f, 0.1f }, ochre); DrawModelEx(s_dome, p + Vector3{ 0, 4.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 0.5f, 0.7f }, stoneDk); }

    // jungle vines creeping over the wall tops
    SetRandomSeed(9600);
    for (int i = 0; i < 14; i++) { int s = (i & 1) ? 1 : -1; float z = rand_range(-14, 12); Vector3 top = c + Vector3{ s * 9.4f, 7.6f, z }; Vector3 bot = top + Vector3{ -s * 0.6f, -rand_range(2.0f, 5.0f), 0 }; draw_bone_seg(top, bot, 0.08f, vine); DrawModelEx(s_dome, bot, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.4f, 0.5f }, leaf); }

    // additive: warm glow + flitting macaws + temple incense + dust motes
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.14f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 200, 120, 45 });
    for (int i = 0; i < 5; i++) { float t = s_time * 0.4f + i * 1.3f; DrawSphereEx(c + Vector3{ sinf(t) * 9.0f, 7.0f + cosf(t * 0.7f) * 2.0f, -2.0f + cosf(t) * 8.0f }, 0.14f, 5, 5, Color{ 220, 60, 40, 120 }); }   // macaws
    for (int s = 0; s < 5; s++) { float y = fmodf(s_time * 0.5f + s * 0.2f, 1.0f) * 6.0f; DrawSphereEx(c + Vector3{ 0, 13.5f + y, -19.0f }, 0.3f, 6, 6, Color{ 200, 200, 180, 28 }); }   // temple incense
    SetRandomSeed(9601);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-10, 10), bz = rand_range(-16, 14); DrawSphereEx(c + Vector3{ bx, 1.0f + 2.5f * fabsf(sinf(s_time * 0.3f + i)), bz }, rand_range(0.04f,0.1f), 4, 4, Color{ 230, 225, 180, 34 }); }
    EndBlendMode();
}

// ---- The Treasury (Petra): a rose-red rock-cut facade at the end of a canyon ----
// DRY level: draw_petra lays the sandy forecourt; the Al-Khazneh facade is CARVED into a giant
// back cliff (two stories, a broken pediment + tholos + urn), flanked by tall Siq canyon walls.
// Distinct from the free-standing classical levels (nave / court / hypostyle / triumph).
static void draw_petra_column(Vector3 base, float h, float r, Color stone, Color cap) {
    DrawModelEx(s_column, base + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 2.7f, 0.4f, r * 2.7f }, cap);   // base
    DrawModelEx(s_cyl, base + Vector3{ 0, 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r, h - 0.6f, r }, stone);              // shaft
    DrawModelEx(s_column, base + Vector3{ 0, h - 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 2.7f, 0.5f, r * 2.7f }, cap); // capital
}

static void build_petra() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    float fz = -17.0f;
    for (float x = -7.5f; x <= 7.5f; x += 3.0f) obstacles.push_back({ c + Vector3{ x, 0, fz }, 0.9f });   // facade lower columns
    obstacles.push_back({ c + Vector3{ 0, 0, fz - 4.0f }, 9.0f });   // the back cliff mass
    for (int s = -1; s <= 1; s += 2) for (float z = 12.0f; z >= -12.0f; z -= 5.0f) obstacles.push_back({ c + Vector3{ s * 16.0f, 0, z }, 2.5f });   // canyon walls
    s_wisps.push_back(c + Vector3{ 0, 3.0f, fz + 1.0f });    // central tomb doorway glow
    s_wisps.push_back(c + Vector3{ 0, 13.0f, fz });          // tholos
    s_wisps.push_back(c + Vector3{ -7.5f, 2.0f, fz + 1.0f });
    s_wisps.push_back(c + Vector3{ 7.5f, 2.0f, fz + 1.0f });
    s_wisps.push_back(c + Vector3{ 0, 1.5f, 6.0f });         // forecourt
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_petra() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color rock{ 196,118,96,255 }, rockDk{ 168,96,78,255 }, rockHi{ 216,150,120,255 }, sand{ 208,176,134,255 }, stoneHi{ 224,188,152,255 }, shadow{ 140,84,68,255 }, dark{ 30,22,20,255 };
    float fz = -17.0f;

    // sandy forecourt floor
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, sand);

    // the great back cliff the facade is carved into (rough stacked rose-red rock)
    DrawModelEx(s_column, c + Vector3{ 0, 14.0f, fz - 5.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 52.0f, 36.0f, 8.0f }, rockDk);
    SetRandomSeed(9700);
    for (int i = 0; i < 22; i++) { float x = rand_range(-26, 26), y = rand_range(2, 30); DrawModelEx(s_column, c + Vector3{ x, y, fz - 2.0f + rand_range(-0.5f,0.8f) }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(3.0f,7.0f), rand_range(3.0f,7.0f), 1.6f }, (i & 1) ? rock : rockDk); }   // rugged rock face

    // ===== the carved facade =====
    // a flat facade backing slab (so the carving reads against the cliff)
    DrawModelEx(s_column, c + Vector3{ 0, 8.0f, fz - 0.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 20.0f, 16.0f, 1.2f }, rock);

    // --- lower story: six columns + entablature + a low pediment + three doorways ---
    for (float x = -7.5f; x <= 7.5f; x += 3.0f) draw_petra_column(c + Vector3{ x, 0, fz }, 8.0f, 0.55f, rockHi, stoneHi);
    DrawModelEx(s_column, c + Vector3{ 0, 8.4f, fz }, Vector3{ 0,1,0 }, 0, Vector3{ 18.0f, 0.9f, 1.4f }, stoneHi);   // entablature
    DrawModelEx(s_column, c + Vector3{ 0, 9.0f, fz - 0.1f }, Vector3{ 0,1,0 }, 0, Vector3{ 17.0f, 0.5f, 1.3f }, rockHi);   // cornice
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, c + Vector3{ s * 4.2f, 9.9f, fz - 0.1f }, Vector3{ 0,0,1 }, s * -20.0f, Vector3{ 9.5f, 0.5f, 1.3f }, stoneHi);   // pediment rakes
    DrawModelEx(s_column, c + Vector3{ 0, 3.0f, fz + 0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 6.0f, 0.6f }, dark);   // central tomb doorway
    DrawModelEx(s_column, c + Vector3{ 0, 6.4f, fz + 0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 0.6f, 0.7f }, stoneHi);   // doorway lintel
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, c + Vector3{ s * 6.0f, 2.4f, fz + 0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 4.4f, 0.6f }, dark);   // side doorways

    // --- upper story: two flanking column-pavilions + a central tholos + broken half-pediments ---
    for (int s = -1; s <= 1; s += 2) { draw_petra_column(c + Vector3{ s * 6.5f, 9.6f, fz }, 6.0f, 0.45f, rockHi, stoneHi); draw_petra_column(c + Vector3{ s * 4.2f, 9.6f, fz }, 6.0f, 0.45f, rockHi, stoneHi); }
    DrawModelEx(s_column, c + Vector3{ 0, 15.7f, fz }, Vector3{ 0,1,0 }, 0, Vector3{ 14.0f, 0.7f, 1.3f }, stoneHi);   // upper entablature
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, c + Vector3{ s * 5.2f, 16.6f, fz - 0.1f }, Vector3{ 0,0,1 }, s * -22.0f, Vector3{ 5.0f, 0.5f, 1.2f }, stoneHi);   // broken (split) pediment halves

    // central tholos (round kiosk): a ring of columns + a conical roof + the famous urn
    Vector3 th = c + Vector3{ 0, 9.6f, fz + 0.6f };
    DrawModelEx(s_cyl, th + Vector3{ 0, -0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 0.4f, 2.6f }, stoneHi);   // tholos platform
    for (int k = 0; k < 6; k++) { float a = k / 6.0f * 2 * PI; draw_petra_column(th + Vector3{ cosf(a) * 2.0f, 0, sinf(a) * 1.2f }, 4.6f, 0.32f, rockHi, stoneHi); }
    DrawModelEx(s_cone, th + Vector3{ 0, 5.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.8f, 2.2f, 2.0f }, rock);       // conical roof
    DrawModelEx(s_cyl, th + Vector3{ 0, 7.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 1.0f, 0.4f }, stoneHi);     // urn neck
    DrawModelEx(s_dome, th + Vector3{ 0, 8.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 1.1f, 0.9f }, stoneHi);    // urn body
    DrawModelEx(s_dome, th + Vector3{ 0, 9.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.35f, 0.4f, 0.35f }, rockHi);   // urn finial

    // the flanking Siq canyon walls leading the eye to the facade (rough rose-red rock)
    SetRandomSeed(9701);
    for (int s = -1; s <= 1; s += 2) for (int t = 0; t < 10; t++) { float z = 13.0f - t * 3.0f; float h = rand_range(14.0f, 24.0f); DrawModelEx(s_column, c + Vector3{ s * (16.0f + rand_range(-0.6f,1.0f)), h * 0.5f, z }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(4.0f,7.0f), h, 3.0f }, (t & 1) ? rock : rockDk); }

    // forecourt rubble: a toppled column + scattered blocks + a standing obelisk
    DrawModelEx(s_cyl, c + Vector3{ -9.0f, 0.5f, 4.0f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.55f, 5.0f, 0.55f }, rockHi);   // fallen column
    SetRandomSeed(9702);
    for (int i = 0; i < 8; i++) DrawModelEx(s_column, c + Vector3{ rand_range(-12,12), 0.4f, rand_range(-8,10) }, Vector3{ 0,1,0 }, rand_range(0,90), Vector3{ rand_range(0.8f,1.6f), 0.8f, rand_range(0.8f,1.6f) }, (i & 1) ? rock : rockDk);   // blocks
    DrawModelEx(s_column, c + Vector3{ 11.0f, 3.0f, 5.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 6.0f, 1.0f }, rockHi);       // obelisk
    DrawModelEx(s_cone, c + Vector3{ 11.0f, 6.2f, 5.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 1.2f, 0.9f }, rockHi);

    // additive: warm torch glow in the doorways + dusty light + motes
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.14f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 180, 100, 45 });
    SetRandomSeed(9703);
    for (int i = 0; i < 20; i++) { float bx = rand_range(-14, 14), bz = rand_range(-14, 12); DrawSphereEx(c + Vector3{ bx, 1.0f + 4.0f * fabsf(sinf(s_time * 0.3f + i)), bz }, rand_range(0.04f,0.11f), 4, 4, Color{ 240, 200, 150, 40 }); }
    EndBlendMode();
}

// ---- The Grand Bazaar: a covered Middle-Eastern souk of stalls, awnings + a domed crossing ----
// DRY level: draw_bazaar lays the market floor; rows of laden stalls under striped awnings line
// the lanes, hung with lanterns + carpets, around a central domed crossing. A commerce scene,
// not a temple — distinct from the open plaza / fairground / siege camp.
static void draw_stall(Vector3 b, float yaw, Color wood, Color woodDk, Color cloth, Color w1, Color w2) {
    rlPushMatrix();
    rlTranslatef(b.x, b.y, b.z);
    rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 0.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 1.4f, 1.4f }, wood);        // counter
    DrawModelEx(s_column, Vector3{ 0, 1.6f, -0.8f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 3.0f, 0.3f }, woodDk);  // back shelf
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cyl, Vector3{ s * 1.4f, 0, 0.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 3.7f, 0.12f }, woodDk);   // posts
    DrawModelEx(s_column, Vector3{ 0, 3.4f, 1.1f }, Vector3{ 1,0,0 }, 20.0f, Vector3{ 3.4f, 0.12f, 2.2f }, cloth);   // awning
    for (int i = -1; i <= 1; i++) DrawModelEx(s_dome, Vector3{ i * 0.9f, 1.6f, 0.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.42f, 0.42f, 0.42f }, (i & 1) ? w1 : w2);   // piled wares
    DrawModelEx(s_cyl, Vector3{ 1.0f, 1.75f, 0.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.55f, 0.3f }, w1);    // a jar
    DrawModelEx(s_column, Vector3{ -1.0f, 2.3f, -0.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 1.3f, 0.15f }, w2);// a hung carpet on the shelf
    DrawModelEx(s_dome, Vector3{ -1.3f, 2.9f, 0.8f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.32f, 0.2f }, Color{ 200,140,70,255 });   // hanging lantern
    rlPopMatrix();
}

static void build_bazaar() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c, 3.0f });   // central domed crossing
    for (int s = -1; s <= 1; s += 2) for (float z = 12.0f; z >= -13.0f; z -= 5.0f) obstacles.push_back({ c + Vector3{ s * 9.0f, 0, z }, 1.8f });   // the two stall rows
    s_wisps.push_back(c + Vector3{ 0, 6.5f, 0 });   // dome
    for (int s = -1; s <= 1; s += 2) { s_wisps.push_back(c + Vector3{ s * 9.0f, 3.0f, 6.0f }); s_wisps.push_back(c + Vector3{ s * 9.0f, 3.0f, -6.0f }); }   // stall lanterns
    s_wisps.push_back(c + Vector3{ 0, 1.5f, 8.0f });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_bazaar() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color flag{ 170,150,124,255 }, flagDk{ 146,128,104,255 }, wood{ 120,86,58,255 }, woodDk{ 96,68,46,255 }, stone{ 200,180,150,255 }, stoneHi{ 216,198,168,255 }, brass{ 186,142,72,255 };
    Color clothA{ 184,72,60,255 }, clothB{ 212,190,150,255 }, carpetA{ 150,60,56,255 }, carpetB{ 60,82,128,255 };
    Color spice1{ 202,120,40,255 }, spice2{ 172,58,40,255 }, spice3{ 204,182,64,255 }, spice4{ 96,124,64,255 };

    // worn market floor + scattered carpet patches
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, flag);
    SetRandomSeed(9800);
    for (int i = 0; i < 8; i++) { Vector3 p = c + Vector3{ rand_range(-7,7), 0.03f, rand_range(-12,12) }; DrawModelEx(s_column, p, Vector3{ 0,1,0 }, rand_range(0,90), Vector3{ rand_range(2.0f,3.5f), 1.0f, rand_range(3.0f,5.0f) }, (i & 1) ? carpetA : carpetB); }

    // the two rows of laden stalls flanking the central lane
    for (int s = -1; s <= 1; s += 2) for (int j = 0; j < 6; j++) { float z = 12.0f - j * 5.0f; draw_stall(c + Vector3{ s * 9.0f, 0, z }, (s > 0) ? 180.0f : 0.0f, wood, woodDk, (j & 1) ? clothA : clothB, (j % 3) ? spice1 : spice3, (j & 1) ? carpetA : spice2); }

    // striped awnings stretched over the side lanes (behind each stall row)
    for (int s = -1; s <= 1; s += 2) for (int j = 0; j < 7; j++) { float z = 12.0f - j * 4.0f; DrawModelEx(s_column, c + Vector3{ s * 13.0f, 4.0f, z }, Vector3{ 0,1,0 }, 0, Vector3{ 7.0f, 0.12f, 1.8f }, (j & 1) ? clothA : clothB); }

    // a couple of stone archways spanning the central lane
    for (int a = 0; a < 2; a++) {
        float z = 9.0f - a * 13.0f;
        for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cyl, c + Vector3{ s * 6.5f, 2.6f, z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 5.2f, 0.6f }, stone);
        DrawModelEx(s_column, c + Vector3{ 0, 5.4f, z }, Vector3{ 0,1,0 }, 0, Vector3{ 14.0f, 0.9f, 1.0f }, stoneHi);
        DrawModelEx(s_cone, c + Vector3{ 0, 5.0f, z }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 3.2f, 3.2f, 1.0f }, stone);   // arch keystone fill
    }

    // the central domed crossing (qaysariyya) + a small fountain beneath it
    for (int k = 0; k < 8; k++) { float a = k / 8.0f * 2 * PI; DrawModelEx(s_cyl, c + Vector3{ cosf(a) * 3.0f, 0, sinf(a) * 3.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 5.2f, 0.4f }, stone); }
    DrawModelEx(s_torus, c + Vector3{ 0, 5.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.3f, 3.3f, 3.3f }, stoneHi);   // cornice ring
    DrawModelEx(s_cyl, c + Vector3{ 0, 5.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 1.2f, 2.6f }, stone);       // drum
    DrawModelEx(s_dome, c + Vector3{ 0, 6.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.8f, 2.4f, 2.8f }, stoneHi);    // dome
    DrawModelEx(s_cyl, c + Vector3{ 0, 9.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.15f, 0.9f, 0.15f }, brass);     // finial
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.7f, 0.8f, 1.7f }, stone);       // fountain basin
    DrawModelEx(s_cyl, c + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 1.0f, 0.4f }, stoneHi);     // fountain spout

    // scattered goods: spice pyramids + pottery stacks + rolled carpets + baskets
    SetRandomSeed(9801);
    Color spices[4] = { spice1, spice2, spice3, spice4 };
    for (int i = 0; i < 7; i++) { Vector3 p = c + Vector3{ rand_range(-6.5f,6.5f), 0.0f, rand_range(-11,11) }; DrawModelEx(s_cyl, p + Vector3{ 0, 0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 0.5f, 0.9f }, woodDk); DrawModelEx(s_cone, p + Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.85f, 1.0f, 0.85f }, spices[i % 4]); }   // spice cones in baskets
    for (int i = 0; i < 5; i++) { Vector3 p = c + Vector3{ rand_range(-7,7), 0.4f, rand_range(-11,11) }; DrawModelEx(s_cyl, p, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.4f, 1.6f, 0.4f }, (i & 1) ? carpetA : carpetB); }   // rolled carpets

    // additive: warm lantern glow + light shafts through the awning gaps + spice motes
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.14f * sinf(s_time * 3 + i), 8, 8, Color{ 255, 180, 90, 50 });
    Vector3 shafts[3] = { c + Vector3{ -4, 0, 4 }, c + Vector3{ 5, 0, -3 }, c + Vector3{ 0, 0, -9 } };
    for (auto& s : shafts) DrawCylinderEx(s + Vector3{ 0, 6.0f, 0 }, s + Vector3{ 0, 0.2f, 0 }, 0.3f, 1.8f, 10, Color{ 255, 220, 150, 14 });
    SetRandomSeed(9802);
    for (int i = 0; i < 20; i++) { float bx = rand_range(-12, 12), bz = rand_range(-13, 13); DrawSphereEx(c + Vector3{ bx, 1.0f + 3.0f * fabsf(sinf(s_time * 0.3f + i)), bz }, rand_range(0.04f,0.1f), 4, 4, Color{ 235, 200, 150, 40 }); }
    EndBlendMode();
}

// ---- The Siege Works: a field of war machines before a battered curtain wall ----
// DRY level: draw_siege lays the churned mud; trebuchets, catapults, a ballista, a siege tower
// and a battering ram are arrayed against a breached stone wall. Engines, not tents (siege camp).
static void draw_trebuchet(Vector3 b, float yaw, Color wood, Color woodDk, Color iron) {
    rlPushMatrix(); rlTranslatef(b.x, b.y, b.z); rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f, 0.8f, 5.5f }, woodDk);   // base sledge
    for (int s = -1; s <= 1; s += 2) { draw_bone_seg(Vector3{ s * 1.3f, 0.8f, 1.6f }, Vector3{ 0, 6.0f, 0 }, 0.26f, wood); draw_bone_seg(Vector3{ s * 1.3f, 0.8f, -1.6f }, Vector3{ 0, 6.0f, 0 }, 0.26f, wood); }   // A-frame
    draw_bone_seg(Vector3{ 0, 2.6f, 2.9f }, Vector3{ 0, 11.0f, -5.2f }, 0.24f, woodDk);   // the long throwing beam (raised)
    DrawModelEx(s_column, Vector3{ 0, 1.8f, 2.9f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 1.7f, 1.5f }, iron);   // counterweight box
    draw_bone_seg(Vector3{ 0, 11.0f, -5.2f }, Vector3{ 0, 8.8f, -4.2f }, 0.05f, wood);    // sling rope
    DrawModelEx(s_dome, Vector3{ 0, 8.5f, -4.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.5f, 0.5f }, iron);    // boulder in the sling
    rlPopMatrix();
}
static void draw_catapult(Vector3 b, float yaw, Color wood, Color woodDk, Color iron) {
    rlPushMatrix(); rlTranslatef(b.x, b.y, b.z); rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 0.6f, 4.2f }, woodDk);   // frame base
    for (int wx = -1; wx <= 1; wx += 2) for (int wz = -1; wz <= 1; wz += 2) DrawModelEx(s_torus, Vector3{ wx * 1.3f, 0.5f, wz * 1.7f }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 0.6f, 0.6f, 0.6f }, iron);   // wheels
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cyl, Vector3{ s * 0.9f, 0.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 2.8f, 0.2f }, wood);   // uprights
    DrawModelEx(s_column, Vector3{ 0, 3.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 0.3f, 0.4f }, wood);     // crossbar stop
    draw_bone_seg(Vector3{ 0, 1.1f, 1.6f }, Vector3{ 0, 3.4f, -2.6f }, 0.2f, woodDk);    // throwing arm (cocked back)
    DrawModelEx(s_dome, Vector3{ 0, 3.6f, -2.9f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.45f, 0.5f }, iron);  // bucket + boulder
    rlPopMatrix();
}
static void draw_ballista(Vector3 b, float yaw, Color wood, Color woodDk, Color iron) {
    rlPushMatrix(); rlTranslatef(b.x, b.y, b.z); rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_cyl, Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.8f, 0.5f }, woodDk);     // pedestal
    DrawModelEx(s_column, Vector3{ 0, 1.9f, -0.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 0.5f, 2.6f }, wood); // stock
    DrawModelEx(s_cyl, Vector3{ 0, 2.0f, 0.6f }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 0.12f, 3.2f, 0.12f }, wood);// the bow (horizontal)
    for (int s = -1; s <= 1; s += 2) draw_bone_seg(Vector3{ s * 1.6f, 2.0f, 0.6f }, Vector3{ 0, 2.0f, -0.9f }, 0.04f, iron);   // bowstring
    DrawModelEx(s_cyl, Vector3{ 0, 2.0f, -1.4f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.08f, 2.6f, 0.08f }, iron);   // the bolt
    rlPopMatrix();
}

static void build_siege() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c + Vector3{ 0,0,-16 }, 4.0f });    // siege tower
    obstacles.push_back({ c + Vector3{ -13,0,-12 }, 3.0f });  // trebuchet L
    obstacles.push_back({ c + Vector3{ 13,0,-12 }, 3.0f });   // trebuchet R
    obstacles.push_back({ c + Vector3{ -9,0,-2 }, 2.5f });    // catapult L
    obstacles.push_back({ c + Vector3{ 9,0,-2 }, 2.5f });     // catapult R
    obstacles.push_back({ c + Vector3{ 0,0,-8 }, 1.6f });     // ballista
    obstacles.push_back({ c + Vector3{ 13,0,7 }, 2.5f });     // battering ram
    for (float x = -18.0f; x <= 18.0f; x += 6.0f) obstacles.push_back({ c + Vector3{ x, 0, -22 }, 3.0f });   // the besieged wall
    s_wisps.push_back(c + Vector3{ -6, 1.5f, 9 });            // braziers
    s_wisps.push_back(c + Vector3{ 7, 1.5f, 11 });
    s_wisps.push_back(c + Vector3{ 0, 11.0f, -16 });          // tower-top torch
    s_wisps.push_back(c + Vector3{ 0, 1.0f, -4 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_siege() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color mud{ 96,78,58,255 }, mudDk{ 78,62,46,255 }, wood{ 120,86,58,255 }, woodDk{ 92,66,44,255 }, iron{ 84,84,90,255 }, stone{ 150,144,132,255 }, stoneDk{ 120,114,104,255 }, banner{ 152,60,56,255 };

    // churned mud floor + ruts + scattered boulders
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, mud);
    SetRandomSeed(9900);
    for (int i = 0; i < 16; i++) { Vector3 p = c + Vector3{ rand_range(-18,18), 0.02f, rand_range(-18,16) }; DrawModelEx(s_column, p, Vector3{ 0,1,0 }, rand_range(0,90), Vector3{ rand_range(1.5f,4.0f), 0.9f, rand_range(0.6f,1.2f) }, mudDk); }   // ruts/churn
    for (int i = 0; i < 10; i++) { Vector3 p = c + Vector3{ rand_range(-16,16), 0.3f, rand_range(-14,14) }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.4f,0.8f), rand_range(0.4f,0.7f), rand_range(0.4f,0.8f) }, (i & 1) ? stoneDk : iron); }   // spent shot

    // the besieged stone curtain wall (battered, with a central breach) + rubble in the gap
    int wh[7] = { 11, 12, 9, 0, 9, 12, 11 };
    for (int i = -3; i <= 3; i++) { int h = wh[i + 3]; if (h == 0) continue; float x = i * 6.0f; DrawModelEx(s_column, c + Vector3{ x, h * 0.5f, -22 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.2f, (float)h, 2.6f }, (i & 1) ? stone : stoneDk); for (int m = -2; m <= 2; m++) DrawModelEx(s_column, c + Vector3{ x + m * 1.2f, h + 0.4f, -22 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.9f, 2.6f }, stone); }
    SetRandomSeed(9901);
    for (int i = 0; i < 12; i++) DrawModelEx(s_dome, c + Vector3{ rand_range(-4,4), 0.4f, -20.0f + rand_range(-1,2) }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.6f,1.4f), rand_range(0.6f,1.2f), rand_range(0.6f,1.4f) }, (i & 1) ? stone : stoneDk);   // breach rubble

    // a wheeled siege tower (the tall landmark) with a lowered drawbridge + a banner
    Vector3 st = c + Vector3{ 0, 0, -16 };
    DrawModelEx(s_column, st + Vector3{ 0, 7.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.0f, 14.0f, 5.0f }, woodDk);
    for (int s = -1; s <= 1; s += 2) for (int z = -1; z <= 1; z += 2) DrawModelEx(s_cyl, st + Vector3{ s * 2.4f, 7.0f, z * 2.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.32f, 14.0f, 0.32f }, wood);   // corner posts
    for (int f = 1; f <= 4; f++) DrawModelEx(s_column, st + Vector3{ 0, f * 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.3f, 0.3f, 5.3f }, wood);   // floors
    DrawModelEx(s_column, st + Vector3{ 0, 11.6f, 4.6f }, Vector3{ 1,0,0 }, 42.0f, Vector3{ 4.2f, 0.3f, 5.0f }, wood);   // drawbridge (lowered toward the wall)
    for (int s = -1; s <= 1; s += 2) for (int z = -1; z <= 1; z += 2) DrawModelEx(s_torus, st + Vector3{ s * 2.0f, 0.7f, z * 2.0f }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 0.8f, 0.8f, 0.8f }, iron);   // wheels
    DrawModelEx(s_cyl, st + Vector3{ 0, 15.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 3.0f, 0.1f }, wood); DrawModelEx(s_column, st + Vector3{ 0.9f, 16.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 1.0f, 0.1f }, banner);   // banner

    // two trebuchets + two catapults + a ballista, all facing the wall (-z)
    draw_trebuchet(c + Vector3{ -13, 0, -12 }, 0.0f, wood, woodDk, iron);
    draw_trebuchet(c + Vector3{ 13, 0, -12 }, 0.0f, wood, woodDk, iron);
    draw_catapult(c + Vector3{ -9, 0, -2 }, 0.0f, wood, woodDk, iron);
    draw_catapult(c + Vector3{ 9, 0, -2 }, 0.0f, wood, woodDk, iron);
    draw_ballista(c + Vector3{ 0, 0, -8 }, 0.0f, wood, woodDk, iron);

    // a covered battering ram (penthouse + suspended iron-headed log) on the flank
    Vector3 rm = c + Vector3{ 13, 0, 7 };
    DrawModelEx(s_column, rm + Vector3{ 0, 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f, 0.5f, 6.5f }, woodDk);   // penthouse roof
    for (int s = -1; s <= 1; s += 2) for (int z = -1; z <= 1; z += 2) DrawModelEx(s_cyl, rm + Vector3{ s * 1.3f, 1.5f, z * 2.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 3.0f, 0.2f }, wood);   // posts
    draw_bone_seg(rm + Vector3{ 0, 2.2f, -3.0f }, rm + Vector3{ 0, 2.2f, 3.2f }, 0.4f, wood);   // the ram log
    DrawModelEx(s_cone, rm + Vector3{ 0, 2.2f, -3.4f }, Vector3{ 1,0,0 }, -90.0f, Vector3{ 0.5f, 1.0f, 0.5f }, iron);   // iron ram head
    for (int z = -1; z <= 1; z += 2) draw_bone_seg(rm + Vector3{ 0, 2.9f, z * 1.6f }, rm + Vector3{ 0, 2.2f, z * 1.6f }, 0.04f, iron);   // suspending chains

    // ammunition: boulder stacks + barrels + a couple of pointed palisade stakes
    DrawModelEx(s_dome, c + Vector3{ -16, 0.5f, 2 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 1.0f, 1.6f }, stoneDk); DrawModelEx(s_dome, c + Vector3{ -16, 1.3f, 2 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 0.7f, 0.9f }, stone);   // boulder pile
    for (int i = 0; i < 3; i++) DrawModelEx(s_cyl, c + Vector3{ 15.0f + (i % 2), 0.7f, -3.0f + i * 1.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.4f, 0.5f }, woodDk);   // barrels
    SetRandomSeed(9902);
    for (int i = 0; i < 10; i++) { int s = (i & 1) ? 1 : -1; float z = rand_range(-18, 14); DrawModelEx(s_cone, c + Vector3{ s * 19.0f, 1.2f, z }, Vector3{ 0,0,1 }, s * 10.0f, Vector3{ 0.35f, 2.6f, 0.35f }, wood); }   // palisade stakes

    // additive: brazier fire + drifting battle smoke + embers + crows
    BeginBlendMode(BLEND_ADDITIVE);
    DrawSphereEx(c + Vector3{ -6, 0.9f, 9 }, 0.7f + 0.3f * sinf(s_time * 9), 8, 8, Color{ 255, 130, 40, 150 });
    DrawSphereEx(c + Vector3{ 7, 0.9f, 11 }, 0.6f + 0.3f * sinf(s_time * 8 + 1), 8, 8, Color{ 255, 130, 40, 150 });
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 150, 70, 36 });
    for (int s = 0; s < 8; s++) { float y = fmodf(s_time * 0.4f + s * 0.13f, 1.0f); DrawSphereEx(c + Vector3{ -6.0f + sinf(s) * 0.6f, 1.5f + y * 7.0f, 9.0f }, 0.4f + y * 0.5f, 6, 6, Color{ 90, 86, 82, 30 }); }   // smoke column
    SetRandomSeed(9903);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-18, 18), bz = rand_range(-18, 16); DrawSphereEx(c + Vector3{ bx, 1.0f + 2.0f * fabsf(sinf(s_time * 0.5f + i)), bz }, rand_range(0.04f,0.1f), 4, 4, Color{ 255, 150, 70, 36 }); }   // embers
    for (int i = 0; i < 4; i++) { float t = s_time * 0.5f + i * 1.6f; DrawSphereEx(c + Vector3{ sinf(t) * 14.0f, 12.0f + cosf(t * 0.6f) * 3.0f, -6.0f + cosf(t) * 10.0f }, 0.18f, 5, 5, Color{ 20, 20, 22, 160 }); }   // crows
    EndBlendMode();
}

// ---- The Whaling Station: a dead try-works on a cold foggy harbour ----
// WET level: the harbour water plane IS the sea; draw_whaling lays a stone+timber wharf (the
// fighting floor) with try-pots, a whale ribcage, a slipway + a moored whaleboat around it.
static void draw_trypot(Vector3 b, Color brick, Color brickDk, Color iron, Color ironDk) {
    DrawModelEx(s_column, b + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 2.0f, 2.6f }, brick);          // brick furnace block
    DrawModelEx(s_column, b + Vector3{ 0, 0.55f, 1.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.8f, 0.4f }, Color{ 28,24,22,255 });   // dark firebox mouth
    DrawModelEx(s_cyl, b + Vector3{ 0, 2.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 1.0f, 1.2f }, iron);              // iron try-pot
    DrawModelEx(s_cyl, b + Vector3{ 0, 2.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.25f, 0.2f, 1.25f }, ironDk);          // rim
    DrawModelEx(s_cyl, b + Vector3{ 0, 2.75f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.1f, 1.0f }, Color{ 30,26,22,255 });   // oily surface
}

static void build_whaling() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    Vector3 pots[3] = { c + Vector3{ -3,0,-13 }, c + Vector3{ 0,0,-14 }, c + Vector3{ 3,0,-13 } };
    for (auto& p : pots) obstacles.push_back({ p, 1.6f });    // the try-pots
    obstacles.push_back({ c + Vector3{ 0,0,-15 }, 3.0f });    // try-works shed
    obstacles.push_back({ c + Vector3{ -11,0,-2 }, 2.0f });   // whale carcass / ribcage
    obstacles.push_back({ c + Vector3{ 12,0,4 }, 1.5f });     // barrel stacks
    for (auto& p : pots) s_wisps.push_back(p + Vector3{ 0, 1.0f, 0 });   // try-pot fires (warm)
    s_wisps.push_back(c + Vector3{ 0, 5.0f, -16 });           // chimney glow
    s_wisps.push_back(c + Vector3{ 0, 1.0f, 2 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_whaling() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    float wl = water_level;
    Color stone{ 140,140,146,255 }, stoneDk{ 112,112,120,255 }, wood{ 104,90,72,255 }, woodDk{ 82,70,56,255 }, brick{ 150,96,80,255 }, brickDk{ 120,76,62,255 }, iron{ 70,70,78,255 }, ironDk{ 50,50,58,255 }, bone{ 216,210,196,255 }, boneDk{ 188,182,166,255 }, rope{ 150,134,100,255 };

    // the stone-and-timber wharf (the fighting floor) — water shows at the rim + slipway
    DrawModelEx(s_column, c + Vector3{ 0, -0.2f, -2.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 30.0f, 0.6f, 28.0f }, stone);
    for (int i = -4; i <= 4; i++) DrawModelEx(s_column, c + Vector3{ i * 3.4f, 0.06f, -2.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.5f, 26.0f }, woodDk);   // plank seams

    // a slipway ramping down into the sea at the front (where whales were hauled up)
    DrawModelEx(s_column, c + Vector3{ 0, -0.6f, 15.0f }, Vector3{ 1,0,0 }, -16.0f, Vector3{ 9.0f, 0.5f, 9.0f }, stoneDk);
    for (int i = -1; i <= 1; i += 2) for (int k = 0; k < 4; k++) DrawModelEx(s_cyl, c + Vector3{ i * 5.0f, wl + 0.6f, 12.0f + k * 2.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.35f, 2.4f, 0.35f }, woodDk);   // slipway pilings

    // the try-works: a row of try-pots in their brick furnaces under a timber shed + chimney
    Vector3 pots[3] = { c + Vector3{ -3,0,-13 }, c + Vector3{ 0,0,-14 }, c + Vector3{ 3,0,-13 } };
    for (auto& p : pots) draw_trypot(p, brick, brickDk, iron, ironDk);
    DrawModelEx(s_column, c + Vector3{ 0, 4.6f, -15.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 12.0f, 0.5f, 5.0f }, woodDk);   // shed roof
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cyl, c + Vector3{ s * 5.0f, 2.3f, -15.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 4.6f, 0.3f }, wood);   // shed posts
    DrawModelEx(s_column, c + Vector3{ 4.5f, 6.0f, -16.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 3.0f, 1.4f }, brickDk);   // chimney

    // the whale carcass: a great ribcage arching over the flensing ground + a tapering skull
    Vector3 rb = c + Vector3{ -11, 0, -1 };
    for (int i = 0; i < 7; i++) {
        float z = -6.0f + i * 2.0f;
        for (int s = -1; s <= 1; s += 2) { Vector3 base = rb + Vector3{ s * 0.6f, 0.2f, z }; Vector3 mid = rb + Vector3{ s * 3.6f, 4.0f, z }; Vector3 top = rb + Vector3{ s * 1.0f, 6.6f, z }; draw_bone_seg(base, mid, 0.22f, (i & 1) ? bone : boneDk); draw_bone_seg(mid, top, 0.18f, bone); }
    }
    draw_bone_seg(rb + Vector3{ 0, 6.6f, -6 }, rb + Vector3{ 0, 6.9f, 8 }, 0.3f, boneDk);   // backbone
    DrawModelEx(s_cone, rb + Vector3{ 0, 1.4f, -9.0f }, Vector3{ 1,0,0 }, -90.0f, Vector3{ 1.6f, 3.4f, 1.2f }, bone);   // skull
    DrawModelEx(s_column, rb + Vector3{ 0, 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 0.5f, 11.0f }, stoneDk);      // flensing platform under it

    // a moored whaleboat on the harbour water + a jetty of pilings
    Vector3 wb = c + Vector3{ -9, wl, 13 };
    DrawModelEx(s_column, wb + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 12.0f, Vector3{ 2.0f, 0.7f, 6.0f }, wood);
    DrawModelEx(s_cone, wb + Vector3{ 0.6f, 0.5f, 3.2f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.9f, 1.6f, 0.5f }, woodDk);   // prow
    DrawModelEx(s_cyl, wb + Vector3{ 0, 1.4f, 2.2f }, Vector3{ 1,0,0 }, 60.0f, Vector3{ 0.06f, 2.4f, 0.06f }, iron);       // a harpoon stowed
    for (int k = 0; k < 5; k++) DrawModelEx(s_cyl, c + Vector3{ 13.0f, wl + 0.8f, 10.0f - k * 3.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 2.6f, 0.4f }, woodDk);   // jetty pilings

    // oil barrels stacked + a harpoon/flensing-tool rack
    SetRandomSeed(10000);
    for (int i = 0; i < 7; i++) { Vector3 p = c + Vector3{ 11.0f + (i % 3) * 1.2f, 0.7f, 2.0f + (i / 3) * 1.4f }; DrawModelEx(s_cyl, p, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.4f, 0.6f }, woodDk); DrawModelEx(s_cyl, p + Vector3{ 0, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.62f, 0.2f, 0.62f }, iron); }   // barrels w/ iron hoops
    DrawModelEx(s_column, c + Vector3{ 8, 1.2f, -6 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 0.2f, 0.2f }, wood);   // tool rack rail
    for (int k = 0; k < 4; k++) DrawModelEx(s_cyl, c + Vector3{ 6.6f + k * 0.8f, 1.0f, -6.0f }, Vector3{ 0,0,1 }, 8.0f, Vector3{ 0.05f, 3.0f, 0.05f }, iron);   // leaning harpoons

    // additive: try-pot fires + greasy smoke + cold drifting fog + wheeling gulls
    BeginBlendMode(BLEND_ADDITIVE);
    for (auto& p : pots) DrawSphereEx(p + Vector3{ 0, 0.6f, 1.3f }, 0.5f + 0.2f * sinf(s_time * 9 + p.x), 8, 8, Color{ 255, 120, 40, 140 });
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 140, 60, 34 });
    for (int s = 0; s < 7; s++) { float y = fmodf(s_time * 0.4f + s * 0.14f, 1.0f); DrawSphereEx(c + Vector3{ 4.5f + sinf(s) * 0.5f, 7.0f + y * 6.0f, -16.0f }, 0.4f + y * 0.6f, 6, 6, Color{ 60, 58, 56, 36 }); }   // chimney smoke
    SetRandomSeed(10001);
    for (int i = 0; i < 16; i++) { float bx = rand_range(-18, 18), bz = rand_range(-16, 16); DrawSphereEx(c + Vector3{ bx, 1.0f + 1.5f * sinf(s_time * 0.3f + i), bz }, rand_range(0.9f, 2.0f), 6, 6, Color{ 180, 190, 200, 20 }); }   // sea fog
    for (int i = 0; i < 4; i++) { float t = s_time * 0.5f + i * 1.6f; DrawSphereEx(c + Vector3{ sinf(t) * 13.0f, 9.0f + cosf(t * 0.7f) * 2.0f, 2.0f + cosf(t) * 9.0f }, 0.16f, 5, 5, Color{ 230, 230, 235, 120 }); }   // gulls
    EndBlendMode();
}

// ---- The Cactus Forest: a Sonoran desert stand of saguaro + companion succulents ----
// DRY level: draw_cactus lays the desert hardpan; saguaros with raised arms, prickly pear,
// ocotillo, agave + barrel cacti, a red mesa and bleached bones make a new desert biome.
static void draw_saguaro(Vector3 b, float h, int arms, Color green, Color greenDk, Color flower) {
    DrawModelEx(s_cyl, b + Vector3{ 0, h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.75f, h, 0.75f }, green);   // trunk
    DrawModelEx(s_dome, b + Vector3{ 0, h, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.75f, 0.5f, 0.75f }, green);       // rounded top
    for (int r = 0; r < 6; r++) { float a = r / 6.0f * 2 * PI; DrawModelEx(s_column, b + Vector3{ cosf(a) * 0.72f, h * 0.5f, sinf(a) * 0.72f }, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 0.07f, h * 0.95f, 0.16f }, greenDk); }   // ribs
    for (int f = 0; f < 5; f++) { float a = f / 5.0f * 2 * PI; DrawModelEx(s_dome, b + Vector3{ cosf(a) * 0.4f, h + 0.35f, sinf(a) * 0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 0.2f, 0.18f }, flower); }   // crown flowers
    for (int k = 0; k < arms; k++) {
        float a = (k / (float)arms) * 2 * PI + 0.6f, af = 0.42f + 0.13f * (k % 3);
        Vector3 a0 = b + Vector3{ cosf(a) * 0.6f, h * af, sinf(a) * 0.6f };
        Vector3 a1 = a0 + Vector3{ cosf(a) * 1.7f, 0.3f, sinf(a) * 1.7f };
        Vector3 a2 = a1 + Vector3{ 0, h * 0.4f, 0 };
        draw_bone_seg(a0, a1, 0.42f, green); draw_bone_seg(a1, a2, 0.36f, green);
        DrawModelEx(s_dome, a2, Vector3{ 0,1,0 }, 0, Vector3{ 0.36f, 0.42f, 0.36f }, green);
        DrawModelEx(s_dome, a2 + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.14f, 0.16f, 0.14f }, flower);
    }
}

static void build_cactus() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    Vector3 sag[9] = { c + Vector3{ -7,0,-3 }, c + Vector3{ 8,0,-5 }, c + Vector3{ -11,0,4 }, c + Vector3{ 12,0,6 }, c + Vector3{ -4,0,-10 }, c + Vector3{ 5,0,-11 }, c + Vector3{ -13,0,-8 }, c + Vector3{ 13,0,-2 }, c + Vector3{ 0,0,9 } };
    for (auto& p : sag) obstacles.push_back({ p, 0.9f });
    obstacles.push_back({ c + Vector3{ -2,0,0 }, 1.0f });     // the great central saguaro
    obstacles.push_back({ c + Vector3{ 0,0,-19 }, 7.0f });    // the back mesa
    s_wisps.push_back(c + Vector3{ 0, 8.0f, 0 });             // warm sun glow
    s_wisps.push_back(c + Vector3{ -2, 9.0f, 0 });
    s_wisps.push_back(c + Vector3{ 0, 6.0f, -19 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_cactus() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color sand{ 206,178,138,255 }, sandDk{ 182,154,116,255 }, rock{ 176,108,84,255 }, rockDk{ 150,88,68,255 }, green{ 96,134,80,255 }, greenDk{ 76,112,64,255 }, greenHi{ 120,156,96,255 }, flower{ 210,90,110,255 }, fruit{ 184,60,70,255 }, bone{ 212,204,188,255 }, brush{ 150,142,96,255 };

    // cracked desert hardpan + cracks + scattered rocks + dry brush
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, sand);
    SetRandomSeed(10100);
    for (int i = 0; i < 18; i++) { Vector3 p = c + Vector3{ rand_range(-20,20), 0.04f, rand_range(-18,16) }; DrawModelEx(s_column, p, Vector3{ 0,1,0 }, rand_range(0,180), Vector3{ rand_range(2.0f,5.0f), 1.0f, 0.18f }, sandDk); }   // cracks
    for (int i = 0; i < 12; i++) { Vector3 p = c + Vector3{ rand_range(-18,18), 0.2f, rand_range(-16,14) }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.5f,1.4f), rand_range(0.4f,1.0f), rand_range(0.5f,1.4f) }, (i & 1) ? rock : rockDk); }   // rocks
    for (int i = 0; i < 14; i++) { Vector3 p = c + Vector3{ rand_range(-19,19), 0.3f, rand_range(-17,15) }; for (int s = 0; s < 5; s++) { float a = s * 1.2f; draw_bone_seg(p, p + Vector3{ cosf(a) * 0.5f, 0.7f, sinf(a) * 0.5f }, 0.04f, brush); } }   // dry brush tufts

    // the flat-topped red mesa at the back (landmark)
    Vector3 mesa = c + Vector3{ 0, 0, -19 };
    for (int t = 0; t < 3; t++) { float w = 17.0f - t * 2.5f; DrawModelEx(s_column, mesa + Vector3{ 0, t * 3.0f + 1.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, 3.0f, w * 0.6f }, (t & 1) ? rock : rockDk); }
    DrawModelEx(s_column, mesa + Vector3{ -7, 5.0f, 1.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 4.0f, 2.0f }, rockDk);   // an eroded spur

    // the saguaro stand (fixed positions; varied height/arms) + the great central one
    Vector3 sag[9] = { c + Vector3{ -7,0,-3 }, c + Vector3{ 8,0,-5 }, c + Vector3{ -11,0,4 }, c + Vector3{ 12,0,6 }, c + Vector3{ -4,0,-10 }, c + Vector3{ 5,0,-11 }, c + Vector3{ -13,0,-8 }, c + Vector3{ 13,0,-2 }, c + Vector3{ 0,0,9 } };
    float sh[9] = { 7.0f, 8.0f, 6.0f, 7.5f, 9.0f, 6.5f, 7.0f, 8.0f, 5.5f };
    int sarm[9] = { 2, 3, 1, 2, 4, 2, 2, 3, 1 };
    for (int i = 0; i < 9; i++) draw_saguaro(sag[i], sh[i], sarm[i], green, greenDk, flower);
    draw_saguaro(c + Vector3{ -2, 0, 0 }, 11.0f, 4, green, greenDk, flower);

    // companion succulents (seeded): prickly pear + ocotillo + agave + barrel cacti
    SetRandomSeed(10101);
    for (int i = 0; i < 9; i++) { Vector3 p = c + Vector3{ rand_range(-16,16), 0, rand_range(-13,13) }; for (int q = 0; q < 4; q++) { float a = q * 1.5f, yy = 0.6f + q * 0.5f; DrawModelEx(s_column, p + Vector3{ cosf(a) * 0.5f * q, yy, sinf(a) * 0.5f * q }, Vector3{ 0,1,0 }, a * RAD2DEG, Vector3{ 0.8f, 1.0f, 0.18f }, greenHi); DrawModelEx(s_dome, p + Vector3{ cosf(a) * 0.5f * q, yy + 0.5f, sinf(a) * 0.5f * q }, Vector3{ 0,1,0 }, 0, Vector3{ 0.14f, 0.14f, 0.14f }, fruit); } }   // prickly pears
    for (int i = 0; i < 5; i++) { Vector3 p = c + Vector3{ rand_range(-17,17), 0, rand_range(-14,14) }; for (int w = 0; w < 9; w++) { float a = w / 9.0f * 2 * PI; Vector3 tip = p + Vector3{ cosf(a) * 2.4f, 4.2f, sinf(a) * 2.4f }; draw_bone_seg(p + Vector3{ 0, 0.2f, 0 }, tip, 0.07f, greenDk); DrawModelEx(s_dome, tip, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 0.3f, 0.18f }, flower); } }   // ocotillo
    for (int i = 0; i < 6; i++) { Vector3 p = c + Vector3{ rand_range(-17,17), 0, rand_range(-13,13) }; for (int l = 0; l < 9; l++) { float a = l / 9.0f * 2 * PI; DrawModelEx(s_cone, p + Vector3{ cosf(a) * 0.7f, 0.9f, sinf(a) * 0.7f }, Vector3{ cosf(a), 0.3f, sinf(a) }, 40.0f, Vector3{ 0.22f, 1.8f, 0.22f }, greenHi); } }   // agave rosettes
    for (int i = 0; i < 6; i++) { Vector3 p = c + Vector3{ rand_range(-18,18), 0, rand_range(-15,15) }; DrawModelEx(s_cyl, p + Vector3{ 0, 0.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 1.4f, 1.0f }, green); DrawModelEx(s_dome, p + Vector3{ 0, 1.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.4f, 1.0f }, green); for (int f = 0; f < 4; f++) { float a = f * 1.57f; DrawModelEx(s_dome, p + Vector3{ cosf(a) * 0.5f, 1.7f, sinf(a) * 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 0.16f, 0.16f }, flower); } }   // barrel cacti

    // a bleached steer skull, a dead saguaro's ribs, and a couple of tumbleweeds
    DrawModelEx(s_dome, c + Vector3{ 9, 0.5f, 10 }, Vector3{ 0,1,0 }, 30.0f, Vector3{ 0.9f, 0.7f, 1.1f }, bone);   // steer skull
    for (int s = -1; s <= 1; s += 2) draw_bone_seg(c + Vector3{ 9, 0.6f, 10 }, c + Vector3{ 9 + s * 1.2f, 1.6f, 9.4f }, 0.12f, bone);   // horns
    for (int r = 0; r < 5; r++) { float a = r / 5.0f * PI - PI * 0.5f; draw_bone_seg(c + Vector3{ -15, 0.2f, 11 }, c + Vector3{ -15 + sinf(a) * 1.4f, 3.0f, 11 + r * 0.3f }, 0.1f, bone); }   // dead saguaro ribs
    for (int i = 0; i < 2; i++) { Vector3 p = c + Vector3{ -6.0f + i * 12.0f, 0.8f, 13.0f }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, s_time * 40.0f, Vector3{ 0.8f, 0.8f, 0.8f }, brush); }   // tumbleweeds

    // additive: warm sun glow + heat shimmer + dust + circling vultures
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 220, 150, 30 });
    SetRandomSeed(10102);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); DrawSphereEx(c + Vector3{ bx, 1.5f + 2.5f * fabsf(sinf(s_time * 0.3f + i)), bz }, rand_range(0.5f,1.2f), 6, 6, Color{ 255, 240, 200, 12 }); }   // heat shimmer
    for (int i = 0; i < 3; i++) { float t = s_time * 0.3f + i * 2.1f; DrawSphereEx(c + Vector3{ sinf(t) * 16.0f, 16.0f + cosf(t * 0.6f) * 2.0f, -2.0f + cosf(t) * 12.0f }, 0.2f, 5, 5, Color{ 20, 18, 18, 150 }); }   // vultures
    EndBlendMode();
}

// ---- The Great Enclosure (Great Zimbabwe): a dry-stone royal court + the Conical Tower ----
// DRY level: draw_zimbabwe lays the dry ground; a tall curved mortarless granite wall (chevron
// course) rings the solid Conical Tower, with daga huts + soapstone birds. A new African
// architecture — distinct from the savanna biome, the desert ziggurat and the grain silo.
static void draw_conical_tower(Vector3 b, float h, float baseR, float topR, Color stone, Color stoneDk) {
    int courses = 16;
    for (int i = 0; i < courses; i++) { float t = i / (float)courses; float r = baseR + (topR - baseR) * t; DrawModelEx(s_cyl, b + Vector3{ 0, t * h + h / courses * 0.5f, 0 }, Vector3{ 0,1,0 }, i * 7.0f, Vector3{ r, h / courses * 1.06f, r }, (i & 1) ? stone : stoneDk); }
    DrawModelEx(s_dome, b + Vector3{ 0, h, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ topR, topR * 1.1f, topR }, stone);
}
static void draw_daga_hut(Vector3 b, float r, Color mud, Color mudDk, Color thatch) {
    DrawModelEx(s_cyl, b + Vector3{ 0, 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r, 2.4f, r }, mud);
    DrawModelEx(s_column, b + Vector3{ 0, 0.9f, r * 0.95f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 1.7f, 0.3f }, Color{ 30,24,20,255 });   // dark doorway
    DrawModelEx(s_cone, b + Vector3{ 0, 2.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 1.5f, 2.2f, r * 1.5f }, thatch);                    // conical thatch roof
}
static void draw_zim_bird(Vector3 b, Color stone, Color soap) {
    DrawModelEx(s_column, b + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 3.2f, 0.5f }, stone);   // monolith
    DrawModelEx(s_dome, b + Vector3{ 0, 3.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.8f, 0.32f }, soap);     // upright bird body
    DrawModelEx(s_dome, b + Vector3{ 0, 4.4f, 0.1f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, 0.3f, 0.25f }, soap); // head
    DrawModelEx(s_cone, b + Vector3{ 0, 4.4f, 0.35f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.1f, 0.4f, 0.1f }, soap);// beak
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, b + Vector3{ s * 0.28f, 3.5f, -0.1f }, Vector3{ 0,0,1 }, s * 18.0f, Vector3{ 0.12f, 0.9f, 0.3f }, soap);   // folded wings
}

static void build_zimbabwe() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    for (int i = 0; i < 44; i++) { float a = (i / 44.0f) * 2 * PI; if (a < 0.55f || a > 2 * PI - 0.55f) continue; obstacles.push_back({ c + Vector3{ sinf(a) * 17.0f, 0, cosf(a) * 17.0f }, 1.8f }); }   // the curved enclosure wall (gap at front)
    obstacles.push_back({ c + Vector3{ 0,0,-13 }, 3.5f });    // the great Conical Tower
    obstacles.push_back({ c + Vector3{ 5,0,-13 }, 2.0f });    // the lesser tower
    obstacles.push_back({ c + Vector3{ -12,0,3 }, 2.4f });    // daga huts
    obstacles.push_back({ c + Vector3{ 11,0,4 }, 2.4f });
    obstacles.push_back({ c + Vector3{ -8,0,-3 }, 2.0f });
    s_wisps.push_back(c + Vector3{ 0, 9.0f, -13 });           // tower top
    s_wisps.push_back(c + Vector3{ -12, 2.5f, 3 });           // hut fires
    s_wisps.push_back(c + Vector3{ 11, 2.5f, 4 });
    s_wisps.push_back(c + Vector3{ 0, 1.5f, 2 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_zimbabwe() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color granite{ 160,150,134,255 }, graniteDk{ 134,124,110,255 }, graniteHi{ 184,174,156,255 }, ground{ 196,176,130,255 }, groundDk{ 172,152,110,255 }, daga{ 182,128,86,255 }, dagaDk{ 152,104,70,255 }, thatch{ 150,124,78,255 }, soap{ 120,128,110,255 }, grass{ 184,170,98,255 };

    // dry tan ground + patches of golden grass
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, ground);
    SetRandomSeed(10200);
    for (int i = 0; i < 16; i++) { Vector3 p = c + Vector3{ rand_range(-15,15), 0.03f, rand_range(-13,13) }; DrawModelEx(s_column, p, Vector3{ 0,1,0 }, rand_range(0,90), Vector3{ rand_range(2.0f,4.0f), 1.0f, rand_range(2.0f,4.0f) }, (i & 1) ? groundDk : ground); }

    // the great curved dry-stone enclosure wall (coursed granite) with a chevron top course
    for (int i = 0; i < 44; i++) {
        float a = (i / 44.0f) * 2 * PI;
        if (a < 0.55f || a > 2 * PI - 0.55f) continue;   // entrance gap at the front
        Vector3 p = c + Vector3{ sinf(a) * 17.0f, 0, cosf(a) * 17.0f };
        float wallH = 8.0f + sinf(a * 3.0f) * 0.6f;
        DrawModelEx(s_column, Vector3{ p.x, wallH * 0.5f, p.z }, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 2.9f, wallH, 2.0f }, (i & 1) ? granite : graniteDk);
        if (i % 2 == 0) DrawModelEx(s_column, Vector3{ p.x, wallH + 0.3f, p.z }, Vector3{ 0,1,0 }, -a * RAD2DEG + 45.0f, Vector3{ 0.7f, 0.7f, 0.7f }, graniteHi);   // chevron course
        DrawModelEx(s_column, Vector3{ p.x, wallH + 0.1f, p.z }, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 2.9f, 0.4f, 2.1f }, graniteHi);   // capstone
    }

    // a short inner wall forming the famous narrow parallel passage to the towers
    for (int i = 0; i < 20; i++) { float a = PI - 0.9f + (i / 20.0f) * 1.8f; Vector3 p = c + Vector3{ sinf(a) * 11.0f, 0, cosf(a) * 11.0f }; DrawModelEx(s_column, Vector3{ p.x, 3.0f, p.z }, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 2.4f, 6.0f, 1.6f }, (i & 1) ? granite : graniteDk); }

    // the Conical Tower (solid coursed-stone cone) + a lesser tower beside it
    draw_conical_tower(c + Vector3{ 0, 0, -13 }, 11.0f, 3.2f, 1.5f, granite, graniteDk);
    draw_conical_tower(c + Vector3{ 5, 0, -13 }, 6.0f, 1.8f, 1.0f, granite, graniteDk);

    // round daga huts with conical thatch roofs
    draw_daga_hut(c + Vector3{ -12, 0, 3 }, 2.2f, daga, dagaDk, thatch);
    draw_daga_hut(c + Vector3{ 11, 0, 4 }, 2.2f, daga, dagaDk, thatch);
    draw_daga_hut(c + Vector3{ -8, 0, -3 }, 1.9f, daga, dagaDk, thatch);

    // soapstone Zimbabwe-bird monoliths flanking the way to the towers
    draw_zim_bird(c + Vector3{ -3.5f, 0, -8 }, granite, soap);
    draw_zim_bird(c + Vector3{ 3.5f, 0, -8 }, granite, soap);

    // granite boulders (a natural kopje) inside + rising beyond the wall
    SetRandomSeed(10201);
    for (int i = 0; i < 7; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(20.0f, 26.0f); Vector3 p = c + Vector3{ sinf(a) * rr, 0, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, rand_range(0,90), Vector3{ rand_range(3.0f,6.0f), rand_range(3.0f,7.0f), rand_range(3.0f,6.0f) }, (i & 1) ? graniteDk : granite); }
    for (int i = 0; i < 5; i++) { Vector3 p = c + Vector3{ rand_range(-12,12), 0.4f, rand_range(-9,9) }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.8f,1.6f), rand_range(0.6f,1.2f), rand_range(0.8f,1.6f) }, granite); }

    // additive: warm hut/tower glow + dry dust + circling birds
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.14f * sinf(s_time * 3 + i), 8, 8, Color{ 255, 190, 110, 38 });
    DrawSphereEx(c + Vector3{ -12, 1.0f, 3 }, 0.6f + 0.2f * sinf(s_time * 8), 8, 8, Color{ 255, 140, 50, 90 });   // a hut hearth
    SetRandomSeed(10202);
    for (int i = 0; i < 16; i++) { float bx = rand_range(-15, 15), bz = rand_range(-13, 13); DrawSphereEx(c + Vector3{ bx, 1.0f + 2.0f * fabsf(sinf(s_time * 0.3f + i)), bz }, rand_range(0.05f,0.12f), 4, 4, Color{ 235, 215, 170, 30 }); }   // dust
    for (int i = 0; i < 3; i++) { float t = s_time * 0.4f + i * 2.0f; DrawSphereEx(c + Vector3{ sinf(t) * 14.0f, 13.0f + cosf(t * 0.6f) * 2.0f, -4.0f + cosf(t) * 11.0f }, 0.16f, 5, 5, Color{ 30, 26, 24, 140 }); }   // birds
    EndBlendMode();
}

// ---- Saint Basil's: the Russian onion-dome cathedral on the cold square (level 100) ----
// DRY level: draw_basil lays the square; brick chapel-towers each crowned by a bulbous, ribbed,
// colourful onion dome ring a tall tent-spire. The onion-dome shape is new — not the smooth
// single dome of the mosque or the Gothic nave.
static void draw_onion_dome(Vector3 base, float drumH, float drumR, Color brick, Color dome, Color rib, Color trim, Color gold) {
    DrawModelEx(s_cyl, base + Vector3{ 0, drumH * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ drumR, drumH, drumR }, brick);   // drum
    for (int w = 0; w < 6; w++) { float a = w / 6.0f * 2 * PI; DrawModelEx(s_column, base + Vector3{ cosf(a) * drumR, drumH * 0.62f, sinf(a) * drumR }, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 0.3f, drumH * 0.4f, 0.16f }, Color{ 44,38,42,255 }); }   // arched windows
    DrawModelEx(s_cyl, base + Vector3{ 0, drumH + 0.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ drumR * 1.06f, 0.4f, drumR * 1.06f }, trim);   // cornice
    for (int k = 0; k < 8; k++) { float a = k / 8.0f * 2 * PI; DrawModelEx(s_cone, base + Vector3{ cosf(a) * drumR * 0.92f, drumH + 0.5f, sinf(a) * drumR * 0.92f }, Vector3{ 1,0,0 }, -70.0f, Vector3{ 0.4f, 0.8f, 0.4f }, trim); }   // kokoshnik gables
    // the bulbous onion: two flipped hemispheres make the belly (wider than the drum), then a cone to the point
    Vector3 belly = base + Vector3{ 0, drumH + drumR * 0.7f, 0 };
    DrawModelEx(s_dome, belly, Vector3{ 0,1,0 }, 0, Vector3{ drumR * 1.32f, drumR * 0.95f, drumR * 1.32f }, dome);          // upper belly
    DrawModelEx(s_dome, belly, Vector3{ 1,0,0 }, 180.0f, Vector3{ drumR * 1.32f, drumR * 0.7f, drumR * 1.32f }, dome);     // lower belly (flipped → bulge)
    DrawModelEx(s_cone, belly + Vector3{ 0, drumR * 0.85f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ drumR * 1.06f, drumR * 1.5f, drumR * 1.06f }, dome);   // taper to the point
    for (int r = 0; r < 8; r++) { float a = r / 8.0f * 2 * PI; Vector3 p0 = base + Vector3{ cosf(a) * drumR, drumH, sinf(a) * drumR }; Vector3 p1 = belly + Vector3{ cosf(a) * drumR * 1.3f, 0, sinf(a) * drumR * 1.3f }; Vector3 p2 = belly + Vector3{ 0, drumR * 2.1f, 0 }; draw_bone_seg(p0, p1, 0.06f, rib); draw_bone_seg(p1, p2, 0.05f, rib); }   // ribs hugging the onion
    float topY = belly.y + drumR * 2.1f;
    DrawModelEx(s_cyl, Vector3{ belly.x, topY, belly.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 0.5f, 0.12f }, gold);        // neck
    DrawModelEx(s_dome, Vector3{ belly.x, topY + 0.4f, belly.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.3f, 0.3f }, gold);  // ball
    DrawModelEx(s_column, Vector3{ belly.x, topY + 1.2f, belly.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.06f, 1.2f, 0.06f }, gold);   // cross upright
    DrawModelEx(s_column, Vector3{ belly.x, topY + 1.4f, belly.z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.06f, 0.06f }, gold);   // cross bar
}

static void build_basil() {
    s_wisps.clear();
    Vector3 c = boundary_center, cc = c + Vector3{ 0, 0, -11 };
    obstacles.push_back({ cc, 4.0f });   // central tent tower
    for (int k = 0; k < 8; k++) { float a = k / 8.0f * 2 * PI; obstacles.push_back({ cc + Vector3{ sinf(a) * 6.5f, 0, cosf(a) * 6.5f }, 2.5f }); }   // chapels
    s_wisps.push_back(cc + Vector3{ 0, 16.0f, 0 });          // tent spire top
    s_wisps.push_back(cc + Vector3{ 0, 6.0f, 7.0f });        // front portal
    for (int k = 0; k < 8; k += 2) { float a = k / 8.0f * 2 * PI; s_wisps.push_back(cc + Vector3{ sinf(a) * 6.5f, 5.0f, cosf(a) * 6.5f }); }   // chapel windows
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_basil() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center, cc = c + Vector3{ 0, 0, -11 };
    Color cobble{ 150,140,138,255 }, cobbleDk{ 126,116,114,255 }, snow{ 224,226,230,255 }, brick{ 172,98,82,255 }, brickDk{ 144,80,68,255 }, trim{ 226,224,218,255 }, gold{ 212,176,90,255 }, tent{ 150,82,72,255 };
    Color domeCol[8] = { { 88,152,112,255 }, { 80,112,180,255 }, { 184,72,68,255 }, { 212,178,92,255 }, { 92,162,166,255 }, { 224,222,214,255 }, { 132,98,172,255 }, { 210,142,72,255 } };
    Color ribCol[8] = { trim, gold, trim, brickDk, trim, { 120,120,128,255 }, gold, trim };

    // the cold square (cobbles) + a darker pattern + scattered snow patches
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, cobble);
    for (int i = -6; i <= 6; i++) { DrawModelEx(s_column, c + Vector3{ i * 3.2f, -0.04f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 1.0f, boundary_radius * 1.8f }, cobbleDk); DrawModelEx(s_column, c + Vector3{ 0, -0.04f, i * 3.2f }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 1.8f, 1.0f, 0.12f }, cobbleDk); }
    SetRandomSeed(10300);
    for (int i = 0; i < 10; i++) { Vector3 p = c + Vector3{ rand_range(-15,15), 0.02f, rand_range(-2,15) }; DrawModelEx(s_column, p, Vector3{ 0,1,0 }, rand_range(0,90), Vector3{ rand_range(1.5f,3.5f), 1.0f, rand_range(1.5f,3.5f) }, snow); }

    // the raised stone platform the cathedral stands on, with a grand front staircase
    DrawModelEx(s_cyl, cc + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 11.0f, 0.8f, 11.0f }, trim);
    DrawModelEx(s_cyl, cc + Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 10.0f, 0.4f, 10.0f }, brickDk);
    for (int s = 0; s < 4; s++) DrawModelEx(s_column, cc + Vector3{ 0, 0.2f + s * 0.22f, 9.5f + s * 0.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 6.0f, 0.4f, 0.8f }, trim);   // staircase

    // the eight chapel-towers, each a brick drum crowned by a distinct onion dome
    float ch[8] = { 11.0f, 8.5f, 11.0f, 8.5f, 11.0f, 8.5f, 11.0f, 8.5f };
    float cr[8] = { 1.9f, 1.5f, 1.9f, 1.5f, 1.9f, 1.5f, 1.9f, 1.5f };
    for (int k = 0; k < 8; k++) { float a = k / 8.0f * 2 * PI; Vector3 p = cc + Vector3{ sinf(a) * 6.5f, 1.1f, cosf(a) * 6.5f }; draw_onion_dome(p, ch[k], cr[k], (k & 1) ? brickDk : brick, domeCol[k], ribCol[k], trim, gold); }

    // the central tent-roofed tower: a tall drum + an octagonal tent spire + a small gold onion
    DrawModelEx(s_cyl, cc + Vector3{ 0, 7.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 13.0f, 3.0f }, brick);
    for (int w = 0; w < 8; w++) { float a = w / 8.0f * 2 * PI; DrawModelEx(s_column, cc + Vector3{ cosf(a) * 3.0f, 6.0f, sinf(a) * 3.0f }, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 0.4f, 3.5f, 0.18f }, Color{ 44,38,42,255 }); }   // windows
    DrawModelEx(s_cyl, cc + Vector3{ 0, 14.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f, 0.5f, 3.2f }, trim);   // cornice
    DrawModelEx(s_cone, cc + Vector3{ 0, 18.0f, 0 }, Vector3{ 0,1,0 }, 22.5f, Vector3{ 3.6f, 6.5f, 3.6f }, tent);   // tent spire
    for (int r = 0; r < 8; r++) { float a = r / 8.0f * 2 * PI; draw_bone_seg(cc + Vector3{ cosf(a) * 3.4f, 14.6f, sinf(a) * 3.4f }, cc + Vector3{ 0, 21.0f, 0 }, 0.06f, trim); }   // tent ribs
    draw_onion_dome(cc + Vector3{ 0, 21.0f, 0 }, 0.6f, 0.7f, brick, gold, trim, trim, gold);   // crowning gold onion

    // additive: warm window glow + drifting snow + a few pigeons
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 200, 120, 36 });
    SetRandomSeed(10301);
    for (int i = 0; i < 40; i++) { float bx = rand_range(-18, 18), bz = rand_range(-18, 16); float fall = fmodf(s_time * 0.2f + i * 0.07f, 1.0f); DrawSphereEx(c + Vector3{ bx + sinf(s_time + i) * 1.0f, 14.0f - fall * 14.0f, bz }, 0.09f, 4, 4, Color{ 230, 234, 240, 60 }); }   // snow
    for (int i = 0; i < 4; i++) { float t = s_time * 0.5f + i * 1.6f; DrawSphereEx(c + Vector3{ sinf(t) * 12.0f, 8.0f + cosf(t * 0.6f) * 2.0f, 2.0f + cosf(t) * 9.0f }, 0.14f, 5, 5, Color{ 60, 60, 66, 120 }); }   // pigeons
    EndBlendMode();
}

// ---- The Printing House: a candlelit print workshop of presses + drying sheets ----
// DRY level: draw_print lays the workshop floor; timber platen presses stand in rows under a
// canopy of hung printed sheets, with type cases, paper reams and ink barrels. A print-craft
// scene — distinct from the Forgotten Archive (book shelves) and the Great Loom (textiles).
static void draw_press(Vector3 b, float yaw, Color wood, Color woodDk, Color brass, Color iron) {
    rlPushMatrix(); rlTranslatef(b.x, b.y, b.z); rlRotatef(yaw, 0, 1, 0);
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, Vector3{ s * 1.1f, 2.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.45f, 4.4f, 0.45f }, wood);   // uprights
    DrawModelEx(s_column, Vector3{ 0, 4.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 0.6f, 0.7f }, woodDk);     // head beam
    DrawModelEx(s_column, Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 0.8f, 1.8f }, woodDk);     // base
    DrawModelEx(s_column, Vector3{ 0, 1.0f, 0.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.9f, 0.3f, 1.3f }, iron);    // the type bed / coffin
    DrawModelEx(s_cyl, Vector3{ 0, 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.25f, 2.0f, 0.25f }, brass);       // the great screw
    DrawModelEx(s_cyl, Vector3{ 0, 3.6f, 0 }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 0.08f, 2.6f, 0.08f }, iron);    // bar lever
    DrawModelEx(s_dome, Vector3{ 1.3f, 3.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.2f, 0.2f }, wood);      // lever knob
    DrawModelEx(s_column, Vector3{ 0, 2.2f, 0.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 0.3f, 1.0f }, woodDk);  // the platen
    DrawModelEx(s_column, Vector3{ 0, 1.6f, -1.0f }, Vector3{ 1,0,0 }, -60.0f, Vector3{ 1.6f, 0.1f, 1.4f }, wood); // tympan/frisket flap (raised)
    rlPopMatrix();
}
static void draw_typecase(Vector3 b, float yaw, Color wood, Color woodDk, Color ink) {
    rlPushMatrix(); rlTranslatef(b.x, b.y, b.z); rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 2.0f, 1.0f }, woodDk);     // cabinet body
    DrawModelEx(s_column, Vector3{ 0, 2.2f, 0.45f }, Vector3{ 1,0,0 }, 28.0f, Vector3{ 2.6f, 0.12f, 1.3f }, wood);   // slanted tray
    for (int i = -2; i <= 2; i++) for (int j = 0; j < 2; j++) DrawModelEx(s_column, b + Vector3{ 0,0,0 } + Vector3{ i * 0.45f, 2.35f - j * 0.25f, 0.5f }, Vector3{ 1,0,0 }, 28.0f, Vector3{ 0.34f, 0.05f, 0.34f }, ink);   // type compartments
    rlPopMatrix();
}

static void build_print() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c, 1.8f });                        // the great central press
    Vector3 pr[4] = { c + Vector3{ -7,0,-2 }, c + Vector3{ 7,0,-2 }, c + Vector3{ -7,0,-9 }, c + Vector3{ 7,0,-9 } };
    for (auto& p : pr) obstacles.push_back({ p, 1.6f });     // the row presses
    for (int i = 0; i < 5; i++) obstacles.push_back({ c + Vector3{ -16.0f + i * 8.0f, 0, -14 }, 1.6f });   // back type cases
    s_wisps.push_back(c + Vector3{ 0, 3.0f, 1 });            // lamps
    s_wisps.push_back(c + Vector3{ -8, 3.0f, -6 });
    s_wisps.push_back(c + Vector3{ 8, 3.0f, -6 });
    s_wisps.push_back(c + Vector3{ 0, 1.5f, 8 });            // proofing candle
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_print() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color floor{ 122,106,86,255 }, floorDk{ 100,86,68,255 }, wood{ 130,94,64,255 }, woodDk{ 102,74,50,255 }, iron{ 74,74,80,255 }, brass{ 172,140,78,255 }, paper{ 226,222,210,255 }, paperDk{ 200,196,184,255 }, rope{ 150,134,100,255 }, ink{ 40,38,42,255 }, book{ 132,84,68,255 };

    // plank workshop floor + ink stains
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, floor);
    for (int i = -7; i <= 7; i++) DrawModelEx(s_column, c + Vector3{ i * 3.0f, -0.04f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 1.0f, boundary_radius * 1.9f }, floorDk);   // plank seams
    SetRandomSeed(10400);
    for (int i = 0; i < 8; i++) DrawModelEx(s_dome, c + Vector3{ rand_range(-14,14), 0.02f, rand_range(-12,12) }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.5f,1.4f), 0.06f, rand_range(0.5f,1.4f) }, ink);   // ink stains

    // the overhead canopy of printed sheets drying on strung lines
    for (int row = 0; row < 6; row++) {
        float z = -11.0f + row * 3.4f;
        draw_bone_seg(c + Vector3{ -16, 5.4f, z }, c + Vector3{ 16, 5.4f, z }, 0.03f, rope);
        SetRandomSeed(10410 + row);
        for (int s = 0; s < 9; s++) { float x = -14.0f + s * 3.4f; float sway = sinf(s_time * 0.8f + s + row) * 0.1f; DrawModelEx(s_column, c + Vector3{ x, 4.7f, z }, Vector3{ 0,0,1 }, sway * RAD2DEG * 4.0f, Vector3{ 1.1f, 1.5f, 0.04f }, (s & 1) ? paper : paperDk); }
    }

    // the great central press + four row presses, all facing the front
    draw_press(c, 0.0f, wood, woodDk, brass, iron);
    Vector3 pr[4] = { c + Vector3{ -7,0,-2 }, c + Vector3{ 7,0,-2 }, c + Vector3{ -7,0,-9 }, c + Vector3{ 7,0,-9 } };
    for (auto& p : pr) draw_press(p, 0.0f, wood, woodDk, brass, iron);

    // type cases along the back wall + a couple of composing tables
    for (int i = 0; i < 5; i++) draw_typecase(c + Vector3{ -16.0f + i * 8.0f, 0, -14 }, 0.0f, wood, woodDk, ink);
    for (int t = -1; t <= 1; t += 2) { Vector3 tb = c + Vector3{ t * 12.0f, 0, 2 }; DrawModelEx(s_column, tb + Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 0.3f, 1.6f }, woodDk); for (int l = -1; l <= 1; l += 2) for (int m = -1; m <= 1; m += 2) DrawModelEx(s_cyl, tb + Vector3{ l * 1.3f, 0.45f, m * 0.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 0.9f, 0.12f }, woodDk); DrawModelEx(s_column, tb + Vector3{ 0, 1.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 0.12f, 1.0f }, iron); }   // imposing stones

    // paper reams stacked + ink barrels + leather ink balls on a stand
    SetRandomSeed(10401);
    for (int i = 0; i < 6; i++) { Vector3 p = c + Vector3{ rand_range(-13,13), 0.0f, rand_range(-10,10) }; for (int k = 0; k < 3; k++) DrawModelEx(s_column, p + Vector3{ 0, 0.25f + k * 0.4f, 0 }, Vector3{ 0,1,0 }, rand_range(-8,8), Vector3{ 1.2f, 0.35f, 1.6f }, (k & 1) ? paper : paperDk); }   // paper reams
    for (int i = 0; i < 4; i++) { Vector3 p = c + Vector3{ rand_range(-14,14), 0.7f, rand_range(-11,11) }; DrawModelEx(s_cyl, p, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.4f, 0.6f }, woodDk); DrawModelEx(s_cyl, p + Vector3{ 0, 0, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.62f, 0.2f, 0.62f }, iron); }   // ink barrels
    DrawModelEx(s_cyl, c + Vector3{ 4, 1.0f, 6 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 2.0f, 0.12f }, woodDk); for (int s = -1; s <= 1; s += 2) { DrawModelEx(s_cyl, c + Vector3{ 4 + s * 0.3f, 2.0f, 6 }, Vector3{ 0,0,1 }, s * 30.0f, Vector3{ 0.08f, 0.7f, 0.08f }, woodDk); DrawModelEx(s_dome, c + Vector3{ 4 + s * 0.6f, 2.4f, 6 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.32f, 0.4f, 0.32f }, ink); }   // ink balls

    // a back shelf of finished books + a proofing desk with a candle
    DrawModelEx(s_column, c + Vector3{ 13, 2.0f, -10 }, Vector3{ 0,1,0 }, 90.0f, Vector3{ 6.0f, 4.0f, 0.6f }, woodDk);
    SetRandomSeed(10402);
    for (int i = 0; i < 18; i++) DrawModelEx(s_column, c + Vector3{ 13.0f + rand_range(-0.2f,0.2f), 0.8f + (i % 3) * 1.2f, -12.5f + (i / 3) * 0.9f }, Vector3{ 0,1,0 }, 90.0f, Vector3{ 0.9f, 0.6f, 0.18f }, (i & 1) ? book : woodDk);   // books
    DrawModelEx(s_column, c + Vector3{ 0, 0.9f, 8 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 0.3f, 1.2f }, woodDk); DrawModelEx(s_column, c + Vector3{ -0.5f, 1.2f, 8 }, Vector3{ 1,0,0 }, 20.0f, Vector3{ 0.9f, 0.05f, 1.2f }, paper);   // proofing desk + a sheet

    // additive: warm lamp glow + a candle flame + drifting paper dust
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.14f * sinf(s_time * 3 + i), 8, 8, Color{ 255, 190, 100, 46 });
    DrawSphereEx(c + Vector3{ 0.6f, 1.6f, 8 }, 0.18f + 0.05f * sinf(s_time * 11), 6, 6, Color{ 255, 170, 80, 150 });   // proofing candle
    SetRandomSeed(10403);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-15, 15), bz = rand_range(-12, 12); DrawSphereEx(c + Vector3{ bx, 1.0f + 3.0f * fabsf(sinf(s_time * 0.3f + i)), bz }, rand_range(0.04f,0.09f), 4, 4, Color{ 220, 215, 200, 36 }); }   // paper dust
    EndBlendMode();
}

// ---- The Ger Camp: a Mongolian steppe encampment of round felt yurts ----
// DRY level: draw_ger lays the grassland; round felt gers ring a sacred ovoo cairn, with a
// campfire, a horse line, spirit-banners + an ox-cart. Round domed dwellings — not the pointed
// military tents of the siege camp nor the cubic huts of the village levels.
static void draw_yurt(Vector3 b, float r, Color felt, Color feltDk, Color door, Color wood) {
    DrawModelEx(s_cyl, b + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r, 2.0f, r }, felt);                       // felt wall
    DrawModelEx(s_cone, b + Vector3{ 0, 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 1.12f, r * 0.85f, r * 1.12f }, feltDk);// low roof
    DrawModelEx(s_cyl, b + Vector3{ 0, 1.85f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 1.02f, 0.16f, r * 1.02f }, door);     // painted top band
    DrawModelEx(s_cyl, b + Vector3{ 0, 2.0f + r * 0.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r * 0.24f, 0.35f, r * 0.24f }, wood);   // crown / smoke ring
    for (int k = 0; k < 8; k++) { float a = k / 8.0f * 2 * PI; draw_bone_seg(b + Vector3{ 0, 2.0f + r * 0.7f, 0 }, b + Vector3{ cosf(a) * r, 2.0f, sinf(a) * r }, 0.04f, wood); }   // roof poles
    DrawModelEx(s_column, b + Vector3{ 0, 0.9f, r * 0.96f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 1.7f, 0.2f }, door);      // painted door
    DrawModelEx(s_column, b + Vector3{ 0, 0.9f, r * 1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.4f, 0.1f }, Color{ 42,32,26,255 });   // dark doorway
}

static void build_ger() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    Vector3 gers[6] = { c + Vector3{ -8,0,-2 }, c + Vector3{ 8,0,-3 }, c + Vector3{ -11,0,5 }, c + Vector3{ 10,0,6 }, c + Vector3{ -6,0,-10 }, c + Vector3{ 6,0,-10 } };
    for (auto& p : gers) obstacles.push_back({ p, 3.0f });
    obstacles.push_back({ c + Vector3{ 0,0,-15 }, 2.6f });    // the ovoo cairn
    obstacles.push_back({ c + Vector3{ -13,0,9 }, 2.0f });    // ox-cart
    s_wisps.push_back(c + Vector3{ 0, 1.2f, 4 });             // campfire
    s_wisps.push_back(c + Vector3{ 0, 5.0f, -15 });           // ovoo
    s_wisps.push_back(c + Vector3{ -8, 2.0f, -2 });           // a ger door
    s_wisps.push_back(c + Vector3{ 8, 2.0f, -3 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_ger() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color grass{ 150,160,92,255 }, grassDk{ 126,138,78,255 }, dirt{ 156,140,108,255 }, felt{ 224,220,206,255 }, feltDk{ 200,196,182,255 }, door{ 196,108,52,255 }, wood{ 120,92,60,255 }, stone{ 150,144,132,255 }, stoneDk{ 120,114,104,255 }, blue{ 90,130,190,255 }, horse{ 142,100,72,255 }, horseDk{ 110,78,56,255 }, banner{ 170,60,56,255 };

    // green-gold grassland + grass tufts + dirt patches + low distant hills
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, grass);
    SetRandomSeed(10500);
    for (int i = 0; i < 16; i++) { Vector3 p = c + Vector3{ rand_range(-16,16), 0.03f, rand_range(-14,14) }; DrawModelEx(s_column, p, Vector3{ 0,1,0 }, rand_range(0,90), Vector3{ rand_range(2.0f,4.0f), 1.0f, rand_range(2.0f,4.0f) }, grassDk); }
    for (int i = 0; i < 26; i++) { Vector3 p = c + Vector3{ rand_range(-18,18), 0.2f, rand_range(-16,15) }; for (int s = 0; s < 4; s++) { float a = s * 1.5f; draw_bone_seg(p, p + Vector3{ cosf(a) * 0.3f, 0.6f, sinf(a) * 0.3f }, 0.03f, grassDk); } }   // grass tufts
    for (int i = 0; i < 14; i++) { float a = i / 14.0f * 2 * PI; DrawModelEx(s_dome, c + Vector3{ sinf(a) * 27.0f, -1.0f, cosf(a) * 27.0f }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(5.0f,9.0f), rand_range(2.0f,4.0f), rand_range(5.0f,9.0f) }, (i & 1) ? grassDk : grass); }   // distant hills

    // the gers (round felt yurts)
    Vector3 gers[6] = { c + Vector3{ -8,0,-2 }, c + Vector3{ 8,0,-3 }, c + Vector3{ -11,0,5 }, c + Vector3{ 10,0,6 }, c + Vector3{ -6,0,-10 }, c + Vector3{ 6,0,-10 } };
    float gr[6] = { 3.0f, 2.6f, 2.8f, 2.6f, 3.0f, 2.7f };
    for (int i = 0; i < 6; i++) draw_yurt(gers[i], gr[i], felt, feltDk, door, wood);

    // the sacred ovoo: a stone cairn + a cone of poles draped with blue prayer scarves + flags
    Vector3 ov = c + Vector3{ 0, 0, -15 };
    for (int t = 0; t < 5; t++) { float rr = 2.2f - t * 0.36f; DrawModelEx(s_dome, ov + Vector3{ 0, t * 0.6f, 0 }, Vector3{ 0,1,0 }, t * 30.0f, Vector3{ rr, 0.7f, rr }, (t & 1) ? stone : stoneDk); }
    Vector3 ovTop = ov + Vector3{ 0, 5.2f, 0 };
    for (int k = 0; k < 8; k++) { float a = k / 8.0f * 2 * PI; draw_bone_seg(ov + Vector3{ cosf(a) * 1.9f, 1.5f, sinf(a) * 1.9f }, ovTop, 0.05f, wood); }   // poles
    DrawModelEx(s_cyl, ov + Vector3{ 0, 2.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 0.14f, 1.4f }, blue); DrawModelEx(s_cyl, ov + Vector3{ 0, 3.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 0.14f, 0.9f }, blue);   // scarf bands
    for (int k = 0; k < 6; k++) { float a = k / 6.0f * 2 * PI; DrawModelEx(s_column, ovTop + Vector3{ cosf(a) * 0.4f, -0.3f, sinf(a) * 0.4f }, Vector3{ 0,1,0 }, a * RAD2DEG, Vector3{ 0.5f, 0.4f, 0.05f }, (k & 1) ? blue : banner); }   // flags

    // the central campfire with a tripod cauldron
    Vector3 fr = c + Vector3{ 0, 0, 4 };
    for (int k = 0; k < 6; k++) { float a = k / 6.0f * 2 * PI; DrawModelEx(s_dome, fr + Vector3{ cosf(a) * 0.9f, 0.15f, sinf(a) * 0.9f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.25f, 0.3f }, stoneDk); }   // fire-ring stones
    for (int k = 0; k < 3; k++) { float a = k / 3.0f * 2 * PI; draw_bone_seg(fr + Vector3{ cosf(a) * 1.0f, 0, sinf(a) * 1.0f }, fr + Vector3{ 0, 2.4f, 0 }, 0.06f, wood); }   // tripod
    DrawModelEx(s_dome, fr + Vector3{ 0, 1.6f, 0 }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 0.7f, 0.7f, 0.7f }, stoneDk);   // hanging cauldron

    // the horse-hitching line + a few horses
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cyl, c + Vector3{ 13.0f, 1.0f, -2.0f + s * 3.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 2.0f, 0.16f }, wood);
    draw_bone_seg(c + Vector3{ 13, 1.8f, -5 }, c + Vector3{ 13, 1.8f, 1 }, 0.03f, horseDk);   // rope
    for (int h = 0; h < 3; h++) { Vector3 hp = c + Vector3{ 11.5f, 0, -4.0f + h * 2.4f }; DrawModelEx(s_column, hp + Vector3{ 0, 1.3f, 0 }, Vector3{ 0,1,0 }, 90.0f, Vector3{ 0.7f, 0.9f, 2.0f }, (h & 1) ? horse : horseDk); for (int lz = -1; lz <= 1; lz += 2) for (int lx = -1; lx <= 1; lx += 2) DrawModelEx(s_cyl, hp + Vector3{ lx * 0.8f, 0, lz * 0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 1.3f, 0.12f }, horseDk); draw_bone_seg(hp + Vector3{ 0.9f, 1.6f, 0 }, hp + Vector3{ 1.7f, 2.4f, 0 }, 0.18f, horse); DrawModelEx(s_column, hp + Vector3{ 1.8f, 2.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.4f, 0.7f }, horse); }   // body + legs + neck + head

    // tall horsehair spirit-banners (tug) flanking the ovoo + an ox-cart
    for (int s = -1; s <= 1; s += 2) { Vector3 tp = ov + Vector3{ s * 4.0f, 0, 2 }; DrawModelEx(s_cyl, tp + Vector3{ 0, 2.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 5.2f, 0.1f }, wood); DrawModelEx(s_cone, tp + Vector3{ 0, 5.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.7f, 0.3f }, stoneDk); for (int k = 0; k < 12; k++) { float a = k / 12.0f * 2 * PI; draw_bone_seg(tp + Vector3{ 0, 4.8f, 0 }, tp + Vector3{ cosf(a) * 0.8f, 3.7f, sinf(a) * 0.8f }, 0.03f, horseDk); } }   // tug
    Vector3 ct = c + Vector3{ -13, 0, 9 };
    DrawModelEx(s_column, ct + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 0.4f, 3.2f }, wood);
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_torus, ct + Vector3{ s * 1.4f, 0.9f, 0 }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 0.9f, 0.9f, 0.9f }, horseDk);
    DrawModelEx(s_column, ct + Vector3{ 0, 1.0f, 2.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.2f, 2.0f }, wood);   // shaft

    // additive: warm campfire + drifting dust + circling eagles
    BeginBlendMode(BLEND_ADDITIVE);
    DrawSphereEx(fr + Vector3{ 0, 0.5f, 0 }, 0.7f + 0.3f * sinf(s_time * 9), 8, 8, Color{ 255, 140, 50, 150 });
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 170, 90, 34 });
    SetRandomSeed(10501);
    for (int i = 0; i < 14; i++) { float bx = rand_range(-18, 18), bz = rand_range(-16, 14); DrawSphereEx(c + Vector3{ bx, 1.0f + 2.0f * fabsf(sinf(s_time * 0.3f + i)), bz }, rand_range(0.05f,0.12f), 4, 4, Color{ 220, 215, 180, 28 }); }   // dust
    for (int i = 0; i < 3; i++) { float t = s_time * 0.3f + i * 2.1f; DrawSphereEx(c + Vector3{ sinf(t) * 15.0f, 14.0f + cosf(t * 0.6f) * 2.0f, -2.0f + cosf(t) * 12.0f }, 0.18f, 5, 5, Color{ 30, 26, 22, 150 }); }   // eagles
    EndBlendMode();
}

// ---- The Alchemist's Laboratory: a cluttered arcane chamber of glowing apparatus ----
// DRY level: draw_alchemy lays the flagstones + a chalk pentagram; an athanor furnace, bubbling
// alembics, jar shelves + a hanging crocodile fill the room, lit by eerie green/purple glows.
static void draw_alembic(Vector3 b, Color glass, Color liquid, Color brass) {
    DrawModelEx(s_cyl, b + Vector3{ 0, 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.6f, 0.5f }, brass);              // brazier
    DrawModelEx(s_dome, b + Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.7f, 0.6f }, glass);             // upper bulb
    DrawModelEx(s_dome, b + Vector3{ 0, 0.9f, 0 }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 0.6f, 0.5f, 0.6f }, glass);        // lower bulb
    DrawModelEx(s_dome, b + Vector3{ 0, 0.72f, 0 }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 0.5f, 0.34f, 0.5f }, liquid);     // the elixir inside
    DrawModelEx(s_dome, b + Vector3{ 0, 1.55f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.5f, 0.4f }, glass);            // alembic head
    draw_bone_seg(b + Vector3{ 0.3f, 1.6f, 0 }, b + Vector3{ 0.9f, 1.2f, 0 }, 0.06f, glass);                            // beak (neck)
    draw_bone_seg(b + Vector3{ 0.9f, 1.2f, 0 }, b + Vector3{ 1.2f, 0.7f, 0 }, 0.06f, glass);
    DrawModelEx(s_dome, b + Vector3{ 1.2f, 0.55f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.35f, 0.45f, 0.35f }, glass);      // receiving flask
    DrawModelEx(s_dome, b + Vector3{ 1.2f, 0.42f, 0 }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 0.28f, 0.22f, 0.28f }, liquid);
}
static void draw_jarshelf(Vector3 b, float yaw, Color wood, Color woodDk, const Color* jars, int nj) {
    rlPushMatrix(); rlTranslatef(b.x, b.y, b.z); rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 2.0f, -0.1f }, Vector3{ 0,1,0 }, 0, Vector3{ 4.0f, 4.0f, 0.5f }, woodDk);          // back
    for (int sh = 0; sh < 3; sh++) { float y = 0.9f + sh * 1.2f; DrawModelEx(s_column, Vector3{ 0, y, 0.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 4.0f, 0.12f, 0.7f }, wood); for (int j = 0; j < 6; j++) { float x = -1.7f + j * 0.68f; DrawModelEx(s_cyl, Vector3{ x, y + 0.42f, 0.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 0.7f, 0.18f }, jars[(sh * 6 + j) % nj]); DrawModelEx(s_dome, Vector3{ x, y + 0.78f, 0.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 0.12f, 0.12f }, Color{ 60,50,40,255 }); } }
    rlPopMatrix();
}

static void build_alchemy() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c + Vector3{ 0,0,-13 }, 3.5f });   // the athanor
    obstacles.push_back({ c, 1.0f });                        // central pedestal + orb
    obstacles.push_back({ c + Vector3{ -8,0,-2 }, 2.0f });   // alembic benches
    obstacles.push_back({ c + Vector3{ 8,0,-4 }, 2.0f });
    obstacles.push_back({ c + Vector3{ 11,0,7 }, 2.0f });    // workbench
    obstacles.push_back({ c + Vector3{ -12,0,5 }, 1.6f });   // cauldron
    s_wisps.push_back(c + Vector3{ 0, 1.4f, 0 });            // the orb (green)
    s_wisps.push_back(c + Vector3{ 0, 2.0f, -12 });          // athanor fire
    s_wisps.push_back(c + Vector3{ -8, 1.4f, -2 });          // alembics
    s_wisps.push_back(c + Vector3{ 8, 1.4f, -4 });
    s_wisps.push_back(c + Vector3{ -12, 1.2f, 5 });          // cauldron
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_alchemy() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color floor{ 60,60,66,255 }, floorDk{ 48,48,54,255 }, stone{ 120,116,110,255 }, stoneDk{ 96,92,88,255 }, wood{ 110,80,54,255 }, woodDk{ 86,62,42,255 }, brass{ 168,134,72,255 }, glass{ 168,196,206,255 }, chalk{ 206,206,222,255 }, wax{ 222,212,184,255 }, parch{ 210,196,160,255 }, croc{ 96,108,72,255 }, bone{ 210,204,188,255 }, dark{ 30,26,24,255 };
    Color pgreen{ 90,200,110,255 }, ppurple{ 170,90,200,255 }, pamber{ 222,160,72,255 }, pblue{ 90,150,210,255 }, pred{ 210,90,90,255 }, pcyan{ 90,210,200,255 };
    Color jars[6] = { pgreen, ppurple, pamber, pblue, pred, pcyan };

    // dark flagstone floor + a chalk pentagram (outer rings + a 5-pointed star) ringed with candles
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, floor);
    for (int i = -6; i <= 6; i++) { DrawModelEx(s_column, c + Vector3{ i * 3.2f, -0.04f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 1.0f, boundary_radius * 1.7f }, floorDk); DrawModelEx(s_column, c + Vector3{ 0, -0.04f, i * 3.2f }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 1.7f, 1.0f, 0.1f }, floorDk); }
    DrawModelEx(s_torus, c + Vector3{ 0, 0.07f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.0f, 1.0f, 6.0f }, chalk); DrawModelEx(s_torus, c + Vector3{ 0, 0.07f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.4f, 1.0f, 5.4f }, chalk);
    for (int k = 0; k < 5; k++) { float a0 = k * 2 * PI / 5 - PI * 0.5f, a1 = (k + 2) * 2 * PI / 5 - PI * 0.5f; draw_bone_seg(c + Vector3{ cosf(a0) * 5.4f, 0.08f, sinf(a0) * 5.4f }, c + Vector3{ cosf(a1) * 5.4f, 0.08f, sinf(a1) * 5.4f }, 0.06f, chalk); DrawModelEx(s_cyl, c + Vector3{ cosf(a0) * 5.7f, 0.4f, sinf(a0) * 5.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 0.7f, 0.12f }, wax); }   // star + candles

    // the great athanor furnace (domed, with a chimney + a glowing fire-door + retorts on top)
    Vector3 at = c + Vector3{ 0, 0, -13 };
    DrawModelEx(s_column, at + Vector3{ 0, 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.0f, 4.0f, 3.0f }, stone);
    DrawModelEx(s_dome, at + Vector3{ 0, 4.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 1.8f, 2.0f }, stoneDk);
    DrawModelEx(s_cyl, at + Vector3{ 1.3f, 5.6f, -0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 2.4f, 0.5f }, stoneDk);   // chimney
    DrawModelEx(s_column, at + Vector3{ 0, 1.2f, 1.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 1.6f, 0.3f }, dark);       // fire door
    draw_alembic(at + Vector3{ -1.0f, 4.4f, 0.4f }, glass, ppurple, brass);

    // the central pedestal cradling a glowing orb (the philosopher's experiment)
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 1.0f, 0.8f }, stoneDk);
    DrawModelEx(s_dome, c + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.3f, 0.6f }, brass);

    // alembic benches (bubbling glassware), jar shelves + a cauldron + a workbench
    for (int i = 0; i < 3; i++) draw_alembic(c + Vector3{ -9.0f + i * 1.4f, 0.4f, -2.0f }, glass, (i & 1) ? pgreen : pcyan, brass);
    for (int i = 0; i < 3; i++) draw_alembic(c + Vector3{ 7.0f + i * 1.4f, 0.4f, -4.0f }, glass, (i & 1) ? pamber : pred, brass);
    draw_jarshelf(c + Vector3{ -12, 0, -10 }, 60.0f, wood, woodDk, jars, 6);
    draw_jarshelf(c + Vector3{ 12, 0, -10 }, -60.0f, wood, woodDk, jars, 6);
    draw_jarshelf(c + Vector3{ -6, 0, -16 }, 0.0f, wood, woodDk, jars, 6);
    Vector3 cl = c + Vector3{ -12, 0, 5 };   // cauldron on a tripod
    for (int k = 0; k < 3; k++) { float a = k / 3.0f * 2 * PI; draw_bone_seg(cl + Vector3{ cosf(a) * 1.0f, 0, sinf(a) * 1.0f }, cl + Vector3{ 0, 2.2f, 0 }, 0.06f, woodDk); }
    DrawModelEx(s_dome, cl + Vector3{ 0, 1.5f, 0 }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 0.9f, 0.9f, 0.9f }, stoneDk);
    Vector3 wb = c + Vector3{ 11, 0, 7 };
    DrawModelEx(s_column, wb + Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 0.3f, 1.6f }, woodDk);
    for (int l = -1; l <= 1; l += 2) for (int m = -1; m <= 1; m += 2) DrawModelEx(s_cyl, wb + Vector3{ l * 1.5f, 0.45f, m * 0.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 0.9f, 0.12f }, woodDk);
    DrawModelEx(s_cyl, wb + Vector3{ -1.2f, 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.4f, 0.3f }, brass);        // mortar
    DrawModelEx(s_dome, wb + Vector3{ 1.2f, 1.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.4f, 0.4f }, glass);       // crystal ball
    DrawModelEx(s_column, wb + Vector3{ 0, 1.1f, 0.2f }, Vector3{ 1,0,0 }, 12.0f, Vector3{ 1.2f, 0.08f, 0.9f }, parch); // open grimoire

    // a hanging stuffed crocodile + a hanging skeleton from the rafters + stacks of grimoires
    DrawModelEx(s_column, c + Vector3{ -4, 5.0f, -1 }, Vector3{ 0,1,0 }, 16.0f, Vector3{ 0.7f, 0.5f, 3.6f }, croc); DrawModelEx(s_cone, c + Vector3{ -4, 5.0f, 1.6f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.4f, 1.4f, 0.4f }, croc); for (int s = -1; s <= 1; s += 2) draw_bone_seg(c + Vector3{ -4, 6.0f, -1.0f + s * 0.8f }, c + Vector3{ -4, 5.0f, -1.0f + s * 0.8f }, 0.03f, woodDk);   // croc + cords
    DrawModelEx(s_cyl, c + Vector3{ 5, 4.4f, -3 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 1.8f, 0.18f }, bone); for (int r = 0; r < 4; r++) for (int s = -1; s <= 1; s += 2) draw_bone_seg(c + Vector3{ 5, 4.8f - r * 0.4f, -3 }, c + Vector3{ 5 + s * 0.5f, 4.4f - r * 0.4f, -3 }, 0.04f, bone);   // hanging skeleton ribs
    SetRandomSeed(10600);
    for (int i = 0; i < 7; i++) { Vector3 p = c + Vector3{ rand_range(-12,12), 0.0f, rand_range(-9,9) }; for (int k = 0; k < 3; k++) DrawModelEx(s_column, p + Vector3{ 0, 0.25f + k * 0.28f, 0 }, Vector3{ 0,1,0 }, rand_range(0,40), Vector3{ 0.9f, 0.26f, 0.7f }, (k & 1) ? wood : parch); }   // grimoire stacks

    // additive: the orb + elixir glows (green/purple/amber) + the athanor fire + drifting fumes
    BeginBlendMode(BLEND_ADDITIVE);
    DrawSphereEx(c + Vector3{ 0, 1.4f, 0 }, 0.5f + 0.18f * sinf(s_time * 3), 10, 10, Color{ 110, 240, 140, 150 });   // the orb
    DrawSphereEx(at + Vector3{ 0, 1.2f, 1.7f }, 0.6f + 0.25f * sinf(s_time * 8), 8, 8, Color{ 255, 130, 50, 130 });  // athanor fire
    for (int i = 0; i < 3; i++) { DrawSphereEx(c + Vector3{ -9.0f + i * 1.4f, 1.0f, -2.0f }, 0.3f + 0.1f * sinf(s_time * 5 + i), 6, 6, Color{ 90, 230, 120, 110 }); DrawSphereEx(c + Vector3{ 7.0f + i * 1.4f, 1.0f, -4.0f }, 0.3f + 0.1f * sinf(s_time * 5 + i + 2), 6, 6, Color{ 210, 130, 230, 110 }); }
    DrawSphereEx(c + Vector3{ -12, 1.6f, 5 }, 0.5f + 0.2f * sinf(s_time * 7), 8, 8, Color{ 120, 220, 170, 120 });    // cauldron
    SetRandomSeed(10601);
    for (int i = 0; i < 22; i++) { float bx = rand_range(-14, 14), bz = rand_range(-14, 12); float h = fmodf(s_time * 0.3f + i * 0.1f, 1.0f); DrawSphereEx(c + Vector3{ bx, 1.0f + h * 5.0f, bz }, rand_range(0.1f,0.3f), 5, 5, Color{ 100, 160, 120, 18 }); }   // rising fumes
    EndBlendMode();
}

// ---- The Bath House: a Roman thermae — a steaming pool ringed by a marble colonnade ----
// WET level: the turquoise water plane IS the bath (the fighting floor); draw_baths lays the
// coping, colonnade, statues, a central fountain + a domed caldarium around it. Reuses
// draw_petra_column. A bathing complex — not the aqueduct's channels nor the geyser terraces.
static void draw_statue(Vector3 b, Color stone, Color plinth) {
    DrawModelEx(s_column, b + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 1.0f, 1.4f }, plinth);      // plinth
    DrawModelEx(s_cyl, b + Vector3{ 0, 1.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.42f, 1.6f, 0.42f }, stone);        // robe/legs
    DrawModelEx(s_column, b + Vector3{ 0, 2.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 1.2f, 0.5f }, stone);       // torso
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cyl, b + Vector3{ s * 0.5f, 2.9f, 0.1f }, Vector3{ 0,0,1 }, s * 22.0f, Vector3{ 0.16f, 1.0f, 0.16f }, stone);   // arms
    DrawModelEx(s_dome, b + Vector3{ 0, 3.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.4f, 0.3f }, stone);         // head
}

static void build_baths() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    for (int i = 0; i < 22; i++) { float a = (i / 22.0f) * 2 * PI; if (a < 0.6f || a > 2 * PI - 0.6f) continue; obstacles.push_back({ c + Vector3{ sinf(a) * 16.0f, 0, cosf(a) * 16.0f }, 1.7f }); }   // the coping/colonnade ring (gap at the front steps)
    obstacles.push_back({ c, 1.8f });                        // central fountain
    s_wisps.push_back(c + Vector3{ 0, 2.5f, 0 });            // fountain
    s_wisps.push_back(c + Vector3{ 0, 8.0f, -20 });          // caldarium
    for (int k = 2; k < 16; k += 5) { float a = k / 16.0f * 2 * PI; s_wisps.push_back(c + Vector3{ sinf(a) * 14.5f, 2.0f, cosf(a) * 14.5f }); }   // braziers
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_baths() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center; float wl = water_level;
    Color marble{ 224,220,210,255 }, marbleDk{ 198,194,182,255 }, marbleHi{ 236,232,224,255 }, gold{ 206,176,96,255 }, statue{ 212,208,198,255 }, bronze{ 150,120,80,255 }, terra{ 182,112,84,255 }, dark{ 40,38,42,255 };

    // the marble coping ring around the bath (the pool edge) + a tiled mosaic band on the bottom
    for (int k = 0; k < 36; k++) { float a = k / 36.0f * 2 * PI; Vector3 p = c + Vector3{ sinf(a) * 16.5f, 0, cosf(a) * 16.5f }; DrawModelEx(s_column, Vector3{ p.x, 0.45f, p.z }, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 3.2f, 1.1f, 1.6f }, (k & 1) ? marble : marbleDk); }
    for (int k = 0; k < 24; k++) { float a = k / 24.0f * 2 * PI; DrawModelEx(s_column, c + Vector3{ sinf(a) * 13.0f, wl + 0.05f, cosf(a) * 13.0f }, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 1.6f, 0.5f, 0.3f }, (k & 1) ? terra : gold); }   // mosaic rim under the water

    // marble steps descending into the bath at the front
    for (int s = 0; s < 4; s++) DrawModelEx(s_column, c + Vector3{ 0, 0.3f - s * 0.25f, 16.0f - s * 1.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 8.0f, 0.5f, 1.4f }, (s & 1) ? marble : marbleDk);

    // the colonnade: a ring of marble columns + an architrave ring + statues between them
    for (int k = 0; k < 16; k++) { float a = k / 16.0f * 2 * PI; draw_petra_column(c + Vector3{ sinf(a) * 16.5f, 0.8f, cosf(a) * 16.5f }, 7.0f, 0.55f, marble, marbleHi); }
    DrawModelEx(s_torus, c + Vector3{ 0, 8.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 16.8f, 16.8f, 16.8f }, marbleHi);   // architrave ring
    for (int k = 0; k < 16; k += 4) { float a = (k + 0.5f) / 16.0f * 2 * PI; draw_statue(c + Vector3{ sinf(a) * 15.0f, 0.9f, cosf(a) * 15.0f }, statue, marbleDk); }

    // bronze braziers on a few column bases
    for (int k = 2; k < 16; k += 5) { float a = k / 16.0f * 2 * PI; Vector3 p = c + Vector3{ sinf(a) * 14.5f, 0.8f, cosf(a) * 14.5f }; DrawModelEx(s_cyl, p + Vector3{ 0, 0.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 1.6f, 0.4f }, bronze); DrawModelEx(s_cyl, p + Vector3{ 0, 1.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.4f, 0.7f }, bronze); }

    // the central tiered marble fountain
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 1.0f, 2.2f }, marble);
    DrawModelEx(s_cyl, c + Vector3{ 0, 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.4f, 0.5f }, marbleDk);
    DrawModelEx(s_cyl, c + Vector3{ 0, 2.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 0.3f, 1.2f }, marbleHi);
    DrawModelEx(s_cyl, c + Vector3{ 0, 2.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.8f, 0.3f }, marble);
    DrawModelEx(s_dome, c + Vector3{ 0, 3.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.4f, 0.4f }, gold);

    // the great domed caldarium rising behind the back colonnade
    Vector3 cal = c + Vector3{ 0, 0, -20 };
    DrawModelEx(s_cyl, cal + Vector3{ 0, 4.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 7.0f, 8.0f, 7.0f }, marble);
    DrawModelEx(s_dome, cal + Vector3{ 0, 8.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 7.2f, 5.0f, 7.2f }, marbleDk);
    DrawModelEx(s_cyl, cal + Vector3{ 0, 13.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 1.0f, 0.9f }, gold);   // oculus finial
    DrawModelEx(s_column, cal + Vector3{ 0, 2.6f, 6.9f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 5.0f, 0.6f }, dark);   // arched doorway

    // a lion-head wall spout pouring into the bath on one side + a couple of urns
    Vector3 sp = c + Vector3{ 14.0f, 0, 4.0f };
    DrawModelEx(s_column, sp + Vector3{ 0, 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 1.2f, 0.6f }, marbleDk); DrawModelEx(s_dome, sp + Vector3{ -0.6f, 2.0f, 0 }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 0.4f, 0.5f, 0.4f }, gold);
    for (int u = -1; u <= 1; u += 2) { Vector3 up = c + Vector3{ u * 8.0f, 0.6f, 14.0f }; DrawModelEx(s_cyl, up + Vector3{ 0, 0.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 1.6f, 0.7f }, marble); DrawModelEx(s_dome, up + Vector3{ 0, 1.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.4f, 0.7f }, marbleDk); }

    // additive: brazier fire + fountain + spout jets + steam rising off the whole bath
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.14f * sinf(s_time * 4 + i), 8, 8, Color{ 255, 170, 80, 50 });
    for (int j = 0; j < 6; j++) { float fy = fmodf(s_time * 1.6f + j * 0.16f, 1.0f); DrawSphereEx(c + Vector3{ sinf(j * 1.7f) * 0.4f, 3.4f + fy * 1.4f, cosf(j * 1.7f) * 0.4f }, 0.16f, 5, 5, Color{ 150, 200, 220, 110 }); }   // fountain jet
    for (int j = 0; j < 5; j++) { float fy = fmodf(s_time * 1.4f + j * 0.2f, 1.0f); DrawSphereEx(sp + Vector3{ -1.0f, 2.0f - fy * 1.6f, 0 }, 0.12f, 5, 5, Color{ 150, 200, 220, 120 }); }   // lion-head spout
    SetRandomSeed(10700);
    for (int i = 0; i < 26; i++) { float bx = rand_range(-14, 14), bz = rand_range(-14, 12); DrawSphereEx(c + Vector3{ bx, wl + 0.5f + 2.0f * fabsf(sinf(s_time * 0.4f + i)), bz }, rand_range(0.8f, 1.8f), 6, 6, Color{ 230, 232, 235, 18 }); }   // steam off the water
    EndBlendMode();
}

// ---- The Floating Market: a tropical river of laden vendor boats around a market pier ----
// WET level: the turquoise river plane is the water; draw_float lays a wooden market pier (the
// fighting floor) ringed by colourful sampans + riverside stilt-shops + palms. Reuses draw_palm.
static void draw_boat(Vector3 b, float yaw, Color wood, Color woodDk, Color p1, Color p2, Color hat) {
    rlPushMatrix(); rlTranslatef(b.x, b.y, b.z); rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 0.35f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 0.6f, 6.0f }, wood);              // hull
    DrawModelEx(s_cone, Vector3{ 0, 0.5f, 3.3f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.8f, 1.4f, 0.7f }, woodDk);        // prow
    DrawModelEx(s_cone, Vector3{ 0, 0.5f, -3.3f }, Vector3{ 1,0,0 }, -90.0f, Vector3{ 0.8f, 1.2f, 0.7f }, woodDk);      // stern
    for (int i = 0; i < 3; i++) { float z = -1.4f + i * 1.3f; DrawModelEx(s_cone, Vector3{ 0, 0.9f, z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.9f, 0.7f }, (i & 1) ? p1 : p2); for (int k = 0; k < 4; k++) { float a = k * 1.57f; DrawModelEx(s_dome, Vector3{ cosf(a) * 0.5f, 0.95f, z + sinf(a) * 0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 0.18f, 0.18f }, (k & 1) ? p2 : p1); } }   // produce pyramids + fruit
    DrawModelEx(s_cyl, Vector3{ 0.6f, 0.85f, 1.8f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.5f, 0.4f }, Color{ 180,150,90,255 });   // basket
    DrawModelEx(s_cyl, Vector3{ 0, 1.1f, -2.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 1.0f, 0.4f }, Color{ 120,90,70,255 });       // vendor body
    DrawModelEx(s_cone, Vector3{ 0, 1.8f, -2.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.5f, 0.7f }, hat);              // conical hat
    draw_bone_seg(Vector3{ 0.5f, 1.3f, -2.4f }, Vector3{ 1.5f, 0.0f, -3.4f }, 0.05f, woodDk);                          // paddle
    rlPopMatrix();
}
static void draw_stiltshop(Vector3 b, float yaw, Color wood, Color woodDk, Color awning, Color lantern) {
    rlPushMatrix(); rlTranslatef(b.x, b.y, b.z); rlRotatef(yaw, 0, 1, 0);
    for (int x = -1; x <= 1; x += 2) for (int z = -1; z <= 1; z += 2) DrawModelEx(s_cyl, Vector3{ x * 1.5f, 1.0f, z * 1.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 2.2f, 0.18f }, woodDk);   // stilts
    DrawModelEx(s_column, Vector3{ 0, 2.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.6f, 0.3f, 3.6f }, wood);              // platform
    DrawModelEx(s_column, Vector3{ 0, 3.3f, -1.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.6f, 2.4f, 0.3f }, woodDk);        // back wall
    DrawModelEx(s_cone, Vector3{ 0, 4.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 1.6f, 3.0f }, woodDk);              // peaked roof
    DrawModelEx(s_column, Vector3{ 0, 3.7f, 2.0f }, Vector3{ 1,0,0 }, 22.0f, Vector3{ 3.8f, 0.12f, 2.0f }, awning);    // awning
    DrawModelEx(s_dome, Vector3{ -1.2f, 3.4f, 1.8f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.3f, 0.2f }, lantern);      // lantern
    rlPopMatrix();
}

static void build_float() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    for (int k = 0; k < 8; k++) { float a = k / 8.0f * 2 * PI; obstacles.push_back({ c + Vector3{ sinf(a) * 16.5f, 0, cosf(a) * 16.5f }, 2.2f }); }   // the vendor boats ring the pier
    Vector3 shops[3] = { c + Vector3{ -16,0,-12 }, c + Vector3{ 16,0,-12 }, c + Vector3{ 0,0,-20 } };
    for (auto& s : shops) obstacles.push_back({ s, 2.4f });   // riverside stilt-shops
    s_wisps.push_back(c + Vector3{ 0, 2.5f, 0 });             // pier lanterns
    for (int k = 0; k < 8; k += 2) { float a = k / 8.0f * 2 * PI; s_wisps.push_back(c + Vector3{ sinf(a) * 16.5f, 1.5f, cosf(a) * 16.5f }); }   // boat lanterns
    s_wisps.push_back(c + Vector3{ 0, 3.0f, -20 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_float() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center; float wl = water_level;
    Color deck{ 152,118,82,255 }, deckDk{ 124,94,64,255 }, wood{ 134,100,68,255 }, woodDk{ 104,76,50,255 }, palmTrunk{ 122,90,58,255 }, palmFrond{ 86,142,72,255 }, palmFruit{ 150,110,60,255 }, lantern{ 230,150,70,255 }, lotus{ 120,170,90,255 };
    Color awningA{ 192,72,62,255 }, awningB{ 218,196,150,255 };
    Color pr[6] = { { 200,80,70,255 }, { 220,150,60,255 }, { 120,170,80,255 }, { 220,200,90,255 }, { 220,140,170,255 }, { 90,170,150,255 } };
    Color hats[3] = { { 214,194,144,255 }, { 196,176,128,255 }, { 224,206,160,255 } };

    // the central wooden market pier (the fighting floor) raised on the river + plank seams + posts
    DrawModelEx(s_column, c + Vector3{ 0, 0.05f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 28.0f, 0.5f, 26.0f }, deck);
    for (int i = -6; i <= 6; i++) DrawModelEx(s_column, c + Vector3{ i * 2.2f, 0.32f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 0.1f, 25.0f }, deckDk);   // plank seams
    Vector3 post[4] = { c + Vector3{ -12,0,12 }, c + Vector3{ 12,0,12 }, c + Vector3{ -12,0,-12 }, c + Vector3{ 12,0,-12 } };
    for (auto& p : post) { DrawModelEx(s_cyl, p + Vector3{ 0, 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 4.0f, 0.2f }, woodDk); DrawModelEx(s_dome, p + Vector3{ 0, 3.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.4f, 0.3f }, lantern); }
    for (int s = 0; s < 2; s++) draw_bone_seg(post[s * 2] + Vector3{ 0, 3.8f, 0 }, post[s * 2 + 1] + Vector3{ 0, 3.8f, 0 }, 0.03f, woodDk);   // lantern strings
    for (int j = 0; j < 7; j++) { float x = -10.0f + j * 3.3f; DrawModelEx(s_dome, c + Vector3{ x, 3.6f, 12.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 0.26f, 0.18f }, lantern); }   // hung lanterns

    // the colourful vendor boats (sampans) ringing the pier on the river
    for (int k = 0; k < 8; k++) { float a = k / 8.0f * 2 * PI; Vector3 p = c + Vector3{ sinf(a) * 16.5f, wl, cosf(a) * 16.5f }; draw_boat(p, a * RAD2DEG + 90.0f, wood, woodDk, pr[k % 6], pr[(k + 2) % 6], hats[k % 3]); }

    // riverside stilt-shops + coconut palms on the banks
    draw_stiltshop(c + Vector3{ -16, 0, -12 }, 50.0f, wood, woodDk, awningA, lantern);
    draw_stiltshop(c + Vector3{ 16, 0, -12 }, -50.0f, wood, woodDk, awningB, lantern);
    draw_stiltshop(c + Vector3{ 0, 0, -20 }, 0.0f, wood, woodDk, awningA, lantern);
    Vector3 palms[4] = { c + Vector3{ -19, 0, 4 }, c + Vector3{ 19, 0, 5 }, c + Vector3{ -14, 0, -18 }, c + Vector3{ 14, 0, -18 } };
    float ph[4] = { 8.0f, 7.5f, 8.5f, 7.0f };
    for (int i = 0; i < 4; i++) draw_palm(palms[i], ph[i], palmTrunk, palmFrond, palmFruit);

    // floating lily pads + lotus on the river
    SetRandomSeed(10800);
    for (int i = 0; i < 16; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(18.0f, 24.0f); Vector3 p = c + Vector3{ sinf(a) * rr, wl + 0.05f, cosf(a) * rr }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.7f,1.4f), 0.12f, rand_range(0.7f,1.4f) }, lotus); }

    // additive: warm lantern glow + river sparkle + drifting dragonflies/birds
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.12f * sinf(s_time * 3 + i), 8, 8, Color{ 255, 190, 100, 44 });
    SetRandomSeed(10801);
    for (int i = 0; i < 24; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(15.0f, 24.0f); DrawSphereEx(c + Vector3{ sinf(a) * rr, wl + 0.2f + 0.3f * fabsf(sinf(s_time * 2 + i)), cosf(a) * rr }, 0.18f, 5, 5, Color{ 180, 210, 220, 30 }); }   // river sparkle
    for (int i = 0; i < 4; i++) { float t = s_time * 0.6f + i * 1.5f; DrawSphereEx(c + Vector3{ sinf(t) * 10.0f, 4.0f + cosf(t * 0.7f) * 1.5f, -2.0f + cosf(t) * 8.0f }, 0.12f, 5, 5, Color{ 90, 120, 80, 90 }); }   // dragonflies
    EndBlendMode();
}

// ---- The Ishtar Gate: a Babylonian processional gate of blue glazed brick ----
// DRY level: draw_ishtar lays the processional way; twin crenellated towers + an arched passage,
// banded with golden reliefs, are approached down lion-walled walls with lamassu guardians.
static void draw_gate_tower(Vector3 b, float w, float h, Color lapis, Color lapisDk, Color gold) {
    DrawModelEx(s_column, b + Vector3{ 0, h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, h, w }, lapis);              // blue brick body
    DrawModelEx(s_column, b + Vector3{ 0, h * 0.5f, w * 0.52f }, Vector3{ 0,1,0 }, 0, Vector3{ w * 0.86f, h * 0.88f, 0.12f }, lapisDk);   // recessed front panel
    for (int band = 0; band < 4; band++) { float y = 2.2f + band * 2.8f; for (int a = 0; a < 4; a++) DrawModelEx(s_column, b + Vector3{ -w * 0.36f + a * w * 0.24f, y, w * 0.55f }, Vector3{ 0,1,0 }, 0, Vector3{ w * 0.18f, 1.1f, 0.16f }, gold); }   // golden relief-animal bands
    for (int m = 0; m < 5; m++) DrawModelEx(s_column, b + Vector3{ -w * 0.4f + m * w * 0.2f, h + 0.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w * 0.13f, 1.4f, w }, lapisDk);   // stepped crenellations
}
static void draw_lamassu(Vector3 b, float yaw, Color stone, Color stoneDk, Color gold) {
    rlPushMatrix(); rlTranslatef(b.x, b.y, b.z); rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 1.7f, -0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 2.6f, 4.0f }, stone);          // bull body
    for (int z = -1; z <= 1; z += 2) for (int x = -1; x <= 1; x += 2) DrawModelEx(s_cyl, Vector3{ x * 0.6f, 0, z * 1.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.6f, 0.3f }, stoneDk);   // 4 legs
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, Vector3{ s * 0.95f, 2.9f, -0.4f }, Vector3{ 0,0,1 }, s * 18.0f, Vector3{ 0.3f, 2.2f, 2.6f }, stoneDk);   // raised wings
    DrawModelEx(s_column, Vector3{ 0, 3.5f, 2.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 1.7f, 1.0f }, stone);           // human head
    DrawModelEx(s_column, Vector3{ 0, 2.6f, 2.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 1.1f, 0.5f }, stoneDk);         // square beard
    DrawModelEx(s_column, Vector3{ 0, 4.4f, 2.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 0.6f, 1.0f }, gold);            // crown
    rlPopMatrix();
}

static void build_ishtar() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c + Vector3{ -6,0,-15 }, 4.0f }); obstacles.push_back({ c + Vector3{ 6,0,-15 }, 4.0f });   // gate towers
    for (int s = -1; s <= 1; s += 2) for (float z = 10.0f; z >= -10.0f; z -= 4.0f) obstacles.push_back({ c + Vector3{ s * 14.0f, 0, z }, 1.8f });   // processional walls
    for (int s = -1; s <= 1; s += 2) obstacles.push_back({ c + Vector3{ s * 4.0f, 0, -11 }, 1.8f });   // lamassu
    s_wisps.push_back(c + Vector3{ 0, 10.0f, -15 });         // gate header
    s_wisps.push_back(c + Vector3{ -6, 5.0f, -13 }); s_wisps.push_back(c + Vector3{ 6, 5.0f, -13 });   // towers
    for (int s = -1; s <= 1; s += 2) s_wisps.push_back(c + Vector3{ s * 11.0f, 1.5f, 6 });   // braziers
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_ishtar() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color lapis{ 50,92,172,255 }, lapisDk{ 38,72,140,255 }, gold{ 212,178,92,255 }, brick{ 196,176,140,255 }, brickDk{ 172,152,116,255 }, stone{ 200,182,148,255 }, stoneDk{ 172,154,122,255 }, bronze{ 150,120,80,255 }, banner{ 168,60,56,255 }, dark{ 22,26,44,255 };

    // the paved processional way + a central path band
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, brick);
    DrawModelEx(s_column, c + Vector3{ 0, 0.02f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 8.0f, 1.0f, boundary_radius * 2.0f }, brickDk);
    for (int i = -8; i <= 8; i++) DrawModelEx(s_column, c + Vector3{ 0, 0.04f, i * 2.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 8.2f, 1.0f, 0.18f }, brick);   // paving joints

    // the processional walls (blue) with golden striding-lion reliefs + crenellations
    for (int s = -1; s <= 1; s += 2) {
        float wx = s * 14.0f;
        DrawModelEx(s_column, c + Vector3{ wx, 3.0f, -1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 6.0f, 26.0f }, lapis);
        DrawModelEx(s_column, c + Vector3{ wx - s * 0.7f, 3.4f, -1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 0.6f, 25.0f }, gold);   // top register line
        for (int j = 0; j < 7; j++) { float z = 10.0f - j * 3.4f; DrawModelEx(s_column, c + Vector3{ wx - s * 0.78f, 2.6f, z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.15f, 1.8f, 2.4f }, gold); }   // lion relief panels
        for (int j = 0; j < 9; j++) { float z = 11.5f - j * 2.7f; DrawModelEx(s_column, c + Vector3{ wx, 6.6f, z }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 1.3f, 1.2f }, lapisDk); }   // crenellations
    }

    // the gate: two great towers + the arched passage + a banded header across the top
    draw_gate_tower(c + Vector3{ -6, 0, -15 }, 7.0f, 14.0f, lapis, lapisDk, gold);
    draw_gate_tower(c + Vector3{ 6, 0, -15 }, 7.0f, 14.0f, lapis, lapisDk, gold);
    DrawModelEx(s_column, c + Vector3{ 0, 12.5f, -15 }, Vector3{ 0,1,0 }, 0, Vector3{ 19.0f, 5.0f, 5.0f }, lapis);          // header lintel
    for (int a = 0; a < 9; a++) DrawModelEx(s_column, c + Vector3{ -8.0f + a * 2.0f, 12.5f, -12.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 1.3f, 0.16f }, gold);   // header relief band
    for (int m = 0; m < 9; m++) DrawModelEx(s_column, c + Vector3{ -8.0f + m * 2.0f, 15.4f, -15 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 1.5f, 5.0f }, lapisDk);   // header crenellations
    DrawModelEx(s_column, c + Vector3{ 0, 5.0f, -14.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 4.6f, 9.0f, 0.6f }, dark);          // dark passage opening
    DrawModelEx(s_cone, c + Vector3{ 0, 9.6f, -14.0f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 2.5f, 2.6f, 1.0f }, dark);        // arched head of the opening
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, c + Vector3{ s * 2.6f, 5.0f, -13.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 9.0f, 0.3f }, gold);   // gold door jambs

    // winged lamassu guardians flanking the opening
    draw_lamassu(c + Vector3{ -4.0f, 0, -11 }, 0.0f, stone, stoneDk, gold);
    draw_lamassu(c + Vector3{ 4.0f, 0, -11 }, 0.0f, stone, stoneDk, gold);

    // bronze braziers + banner poles flanking the way
    for (int s = -1; s <= 1; s += 2) { Vector3 bp = c + Vector3{ s * 11.0f, 0, 6.0f }; DrawModelEx(s_cyl, bp + Vector3{ 0, 0.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 1.6f, 0.4f }, bronze); DrawModelEx(s_cyl, bp + Vector3{ 0, 1.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.4f, 0.7f }, bronze); Vector3 fp = c + Vector3{ s * 12.0f, 0, -4.0f }; DrawModelEx(s_cyl, fp + Vector3{ 0, 4.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 8.0f, 0.12f }, stoneDk); DrawModelEx(s_column, fp + Vector3{ 0, 6.5f, s * -0.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 3.0f, 1.6f }, banner); }

    // additive: warm brazier fire + tower-top glow + dust motes
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.14f * sinf(s_time * 3 + i), 8, 8, Color{ 255, 180, 90, 42 });
    for (int s = -1; s <= 1; s += 2) DrawSphereEx(c + Vector3{ s * 11.0f, 2.1f, 6.0f }, 0.6f + 0.25f * sinf(s_time * 9 + s), 8, 8, Color{ 255, 140, 50, 140 });   // brazier fire
    SetRandomSeed(10900);
    for (int i = 0; i < 16; i++) { float bx = rand_range(-13, 13), bz = rand_range(-12, 14); DrawSphereEx(c + Vector3{ bx, 1.5f + 3.0f * fabsf(sinf(s_time * 0.3f + i)), bz }, rand_range(0.05f,0.11f), 4, 4, Color{ 235, 220, 180, 26 }); }
    EndBlendMode();
}

// ---- The Tannery: a Moroccan dye-works of round colour pits + drying hides ----
// DRY level: draw_tannery lays the earthen floor; a honeycomb of vivid stone dye pits fills the
// back with hides drying on racks + mud-brick rooftops. Round colour pits — not the salt pans.
static void draw_dyepit(Vector3 b, float r, Color dye, Color rim) {
    DrawModelEx(s_cyl, b + Vector3{ 0, 0.35f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r + 0.35f, 0.7f, r + 0.35f }, rim);   // raised stone rim
    DrawModelEx(s_cyl, b + Vector3{ 0, 0.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r + 0.06f, 0.6f, r + 0.06f }, dye);   // brimming dye surface (proud of the rim)
    DrawModelEx(s_dome, b + Vector3{ 0, 0.95f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r, 0.12f, r }, dye);                // a slight meniscus so the colour reads from above
}
static void draw_hiderack(Vector3 b, float yaw, Color wood, Color h1, Color h2, Color h3) {
    rlPushMatrix(); rlTranslatef(b.x, b.y, b.z); rlRotatef(yaw, 0, 1, 0);
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cyl, Vector3{ s * 1.8f, 1.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 2.4f, 0.12f }, wood);   // posts
    DrawModelEx(s_cyl, Vector3{ 0, 2.2f, 0 }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 0.08f, 3.8f, 0.08f }, wood);          // rail
    Color hc[3] = { h1, h2, h3 };
    for (int i = -1; i <= 1; i++) DrawModelEx(s_column, Vector3{ i * 1.1f, 1.45f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 1.5f, 0.05f }, hc[(i + 1) % 3]);   // draped hides
    rlPopMatrix();
}

static void build_tannery() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    for (int gx = -2; gx <= 2; gx++) for (int gz = 0; gz < 3; gz++) obstacles.push_back({ c + Vector3{ gx * 3.4f, 0, -9.0f - gz * 3.0f }, 1.6f });   // the dye-pit grid
    obstacles.push_back({ c + Vector3{ -11,0,2 }, 1.6f }); obstacles.push_back({ c + Vector3{ 11,0,2 }, 1.6f });   // two front pits
    s_wisps.push_back(c + Vector3{ 0, 2.0f, -12 });          // over the pit field
    s_wisps.push_back(c + Vector3{ -10, 2.0f, 4 }); s_wisps.push_back(c + Vector3{ 10, 2.0f, 4 });
    s_wisps.push_back(c + Vector3{ 0, 4.0f, -17 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_tannery() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color earth{ 178,152,114,255 }, earthDk{ 152,126,94,255 }, mud{ 188,158,122,255 }, mudDk{ 158,130,100,255 }, rim{ 172,162,142,255 }, wood{ 128,96,64,255 }, basket{ 180,150,90,255 };
    Color hideA{ 202,182,142,255 }, hideB{ 172,142,102,255 }, hideC{ 150,112,82,255 }, hideW{ 220,212,196,255 };
    Color dyes[7] = { { 222,192,62,255 }, { 192,72,60,255 }, { 70,112,172,255 }, { 142,92,60,255 }, { 112,162,82,255 }, { 212,206,196,255 }, { 222,142,52,255 } };

    // the dusty earthen tannery floor + stone walkway joints
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, earth);
    SetRandomSeed(11000);
    for (int i = 0; i < 12; i++) DrawModelEx(s_column, c + Vector3{ rand_range(-14,14), -0.02f, rand_range(-14,14) }, Vector3{ 0,1,0 }, rand_range(0,90), Vector3{ rand_range(2.0f,5.0f), 1.0f, 0.6f }, earthDk);   // walkway/stains

    // the honeycomb of vivid dye pits (a 5x3 grid) + two front pits
    int idx = 0;
    for (int gx = -2; gx <= 2; gx++) for (int gz = 0; gz < 3; gz++) { draw_dyepit(c + Vector3{ gx * 3.4f, 0, -9.0f - gz * 3.0f }, 1.5f, dyes[idx % 7], rim); idx++; }
    draw_dyepit(c + Vector3{ -11, 0, 2 }, 1.5f, dyes[2], rim);
    draw_dyepit(c + Vector3{ 11, 0, 2 }, 1.5f, dyes[0], rim);

    // tall terraced mud-brick walls (sides + back) with hides drying on the rooftops
    for (int s = -1; s <= 1; s += 2) {
        float wx = s * 15.0f;
        DrawModelEx(s_column, c + Vector3{ wx, 3.5f, -3 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 7.0f, 22.0f }, mud);
        DrawModelEx(s_column, c + Vector3{ wx, 7.2f, -3 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 0.5f, 22.0f }, mudDk);   // parapet
        for (int j = 0; j < 6; j++) DrawModelEx(s_column, c + Vector3{ wx, 7.6f, 5.0f - j * 3.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 0.08f, 1.6f }, (j & 1) ? hideA : hideC);   // hides on the roof
    }
    DrawModelEx(s_column, c + Vector3{ 0, 4.5f, -18 }, Vector3{ 0,1,0 }, 0, Vector3{ 32.0f, 9.0f, 2.4f }, mud);   // back wall
    for (int w = 0; w < 6; w++) DrawModelEx(s_column, c + Vector3{ -12.0f + w * 4.8f, 6.0f, -16.9f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 2.4f, 0.3f }, Color{ 40,32,26,255 });   // dark windows
    for (int w = 0; w < 8; w++) DrawModelEx(s_column, c + Vector3{ -14.0f + w * 4.0f, 9.4f, -18 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 1.6f, 2.4f }, mudDk);   // roofline hides drying / merlons

    // drying-hide racks + baskets of raw pigment + a stack of cured hides in the working area
    draw_hiderack(c + Vector3{ -10, 0, -3 }, 80.0f, wood, hideA, hideB, hideW);
    draw_hiderack(c + Vector3{ 10, 0, -3 }, -80.0f, wood, hideB, hideC, hideA);
    draw_hiderack(c + Vector3{ -7, 0, 8 }, 20.0f, wood, hideW, hideA, hideB);
    draw_hiderack(c + Vector3{ 7, 0, 9 }, -20.0f, wood, hideC, hideW, hideA);
    SetRandomSeed(11001);
    for (int i = 0; i < 8; i++) { Vector3 p = c + Vector3{ rand_range(-12,12), 0.0f, rand_range(-6,10) }; DrawModelEx(s_cyl, p + Vector3{ 0, 0.35f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.7f, 0.6f }, basket); DrawModelEx(s_dome, p + Vector3{ 0, 0.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.55f, 0.3f, 0.55f }, dyes[i % 7]); }   // pigment baskets
    for (int i = 0; i < 4; i++) DrawModelEx(s_column, c + Vector3{ -13.0f, 0.3f + i * 0.2f, 9.0f }, Vector3{ 0,1,0 }, (float)(i * 12), Vector3{ 1.8f, 0.22f, 1.4f }, (i & 1) ? hideB : hideA);   // cured-hide stack

    // additive: warm sun glow + drifting dust + flies/birds over the pits
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 210, 140, 28 });
    SetRandomSeed(11002);
    for (int i = 0; i < 16; i++) { float bx = rand_range(-14, 14), bz = rand_range(-16, 12); DrawSphereEx(c + Vector3{ bx, 1.5f + 2.5f * fabsf(sinf(s_time * 0.3f + i)), bz }, rand_range(0.05f,0.11f), 4, 4, Color{ 235, 220, 180, 22 }); }   // dust
    for (int i = 0; i < 5; i++) { float t = s_time * 0.8f + i * 1.2f; DrawSphereEx(c + Vector3{ sinf(t) * 8.0f, 2.0f + cosf(t * 0.7f) * 1.0f, -10.0f + cosf(t) * 6.0f }, 0.08f, 4, 4, Color{ 40, 36, 30, 110 }); }   // flies
    EndBlendMode();
}

// ---- The Natural History Hall: a museum with a mounted T-rex skeleton ----
// DRY level: draw_museum lays the gallery floor; a mounted, articulated dinosaur skeleton rears
// at the centre, ringed by glass display cases + a colonnade. A museum — not the bone graveyard.
static void draw_trex(Vector3 base, Color bone, Color boneDk, Color steel, Color plinth) {
    DrawModelEx(s_column, base + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 8.0f, 0.8f, 5.5f }, plinth);     // plinth
    Vector3 hips = base + Vector3{ 0, 4.2f, -1.5f };
    for (int s = -1; s <= 1; s += 2) { Vector3 hip = hips + Vector3{ s * 0.8f, 0, 0 }; Vector3 knee = hip + Vector3{ s * 0.3f, -1.8f, 0.6f }; Vector3 foot = knee + Vector3{ 0, -2.0f, 1.0f }; draw_bone_seg(hip, knee, 0.3f, bone); draw_bone_seg(knee, foot, 0.24f, bone); draw_bone_seg(foot, foot + Vector3{ 0, 0, 0.8f }, 0.16f, boneDk); }   // two legs + feet
    Vector3 shoulder = hips + Vector3{ 0, 0.6f, 4.2f };
    draw_bone_seg(hips, shoulder, 0.36f, bone);                                          // back
    Vector3 neck = shoulder + Vector3{ 0, 1.2f, 1.6f }, head = neck + Vector3{ 0, 0.6f, 1.6f };
    draw_bone_seg(shoulder, neck, 0.3f, bone); draw_bone_seg(neck, head, 0.26f, bone);   // neck
    DrawModelEx(s_column, head + Vector3{ 0, 0, 0.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 1.0f, 2.2f }, bone);       // skull
    DrawModelEx(s_column, head + Vector3{ 0, -0.5f, 0.9f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 0.35f, 1.9f }, boneDk);// lower jaw
    for (int t = 0; t < 6; t++) { float x = -0.3f + (t % 3) * 0.3f, zz = 1.0f + (t / 3) * 0.8f; DrawModelEx(s_cone, head + Vector3{ x, -0.15f, zz }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 0.06f, 0.32f, 0.06f }, bone); }   // upper teeth
    Vector3 t1 = hips + Vector3{ 0, -0.2f, -3.2f }, t2 = hips + Vector3{ 0, -1.4f, -6.4f }, t3 = hips + Vector3{ 0, -1.8f, -9.0f };
    draw_bone_seg(hips, t1, 0.3f, bone); draw_bone_seg(t1, t2, 0.2f, bone); draw_bone_seg(t2, t3, 0.12f, boneDk);      // tail
    for (int i = 0; i < 5; i++) { float u = i / 5.0f; Vector3 sp = Vector3Lerp(hips, shoulder, u); for (int s = -1; s <= 1; s += 2) draw_bone_seg(sp, sp + Vector3{ s * 1.3f, -1.6f, 0 }, 0.06f, bone); }   // ribs
    for (int s = -1; s <= 1; s += 2) draw_bone_seg(shoulder + Vector3{ s * 0.6f, 0, 0.3f }, shoulder + Vector3{ s * 0.9f, -0.8f, 0.8f }, 0.08f, boneDk);   // tiny arms
    draw_bone_seg(base + Vector3{ 0, 0.8f, -1.5f }, hips, 0.1f, steel);                  // armature support
    draw_bone_seg(base + Vector3{ 1.6f, 0.8f, 5.5f }, head + Vector3{ 0, -0.5f, 0 }, 0.08f, steel);   // head support
}
static void draw_case(Vector3 b, float yaw, Color wood, Color woodDk, Color glass, Color specimen) {
    rlPushMatrix(); rlTranslatef(b.x, b.y, b.z); rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 0.55f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 1.1f, 1.6f }, woodDk);   // base cabinet
    DrawModelEx(s_column, Vector3{ 0, 2.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.9f, 2.6f, 1.3f }, glass);     // glass case
    DrawModelEx(s_dome, Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.6f, 0.5f }, specimen);    // specimen
    DrawModelEx(s_column, Vector3{ 0, 3.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 0.3f, 1.6f }, wood);      // top
    rlPopMatrix();
}

static void build_museum() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c, 4.5f });   // the mounted skeleton's plinth
    for (int k = 0; k < 8; k++) { float a = k / 8.0f * 2 * PI + 0.4f; obstacles.push_back({ c + Vector3{ cosf(a) * 15.0f, 0, sinf(a) * 15.0f }, 1.6f }); }   // display cases
    for (int k = 0; k < 12; k++) { float a = k / 12.0f * 2 * PI; obstacles.push_back({ c + Vector3{ cosf(a) * 18.5f, 0, sinf(a) * 18.5f }, 1.4f }); }   // colonnade
    s_wisps.push_back(c + Vector3{ 0, 9.0f, 2 });           // skylight on the skeleton
    s_wisps.push_back(c + Vector3{ 0, 1.5f, 6 });
    for (int k = 0; k < 8; k += 2) { float a = k / 8.0f * 2 * PI + 0.4f; s_wisps.push_back(c + Vector3{ cosf(a) * 15.0f, 2.5f, sinf(a) * 15.0f }); }   // case lights
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_museum() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color floorW{ 168,138,98,255 }, floorDk{ 144,116,80,255 }, marble{ 212,208,198,255 }, bone{ 184,162,128,255 }, boneDk{ 154,132,102,255 }, steel{ 110,114,124,255 }, wood{ 122,90,60,255 }, woodDk{ 96,70,48,255 }, glass{ 182,206,212,255 }, brass{ 172,140,78,255 }, plinth{ 150,142,130,255 }, globe{ 110,150,184,255 }, fur{ 120,90,64,255 };
    Color spec[4] = { { 110,170,120,255 }, { 190,110,110,255 }, { 120,140,200,255 }, { 200,180,110,255 } };

    // the parquet gallery floor + a central marble rotunda + radial inlay
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, floorW);
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 12.0f, 0.5f, 12.0f }, marble);
    for (int k = 0; k < 16; k++) { float a = k / 16.0f * 2 * PI; draw_bone_seg(c + Vector3{ 0, 0.04f, 0 }, c + Vector3{ cosf(a) * boundary_radius, 0.04f, sinf(a) * boundary_radius }, 0.06f, floorDk); }

    // the mounted T-rex skeleton centrepiece
    draw_trex(c, bone, boneDk, steel, plinth);

    // a ring of glass display cases + the colonnade + a gallery balcony rail
    for (int k = 0; k < 8; k++) { float a = k / 8.0f * 2 * PI + 0.4f; draw_case(c + Vector3{ cosf(a) * 15.0f, 0, sinf(a) * 15.0f }, a * RAD2DEG, wood, woodDk, glass, spec[k % 4]); }
    for (int k = 0; k < 12; k++) { float a = k / 12.0f * 2 * PI; draw_petra_column(c + Vector3{ cosf(a) * 18.5f, 0, sinf(a) * 18.5f }, 8.5f, 0.5f, marble, marble); }
    DrawModelEx(s_torus, c + Vector3{ 0, 8.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 18.8f, 18.8f, 18.8f }, woodDk);   // gallery balcony rail

    // taxidermy mounts (a standing bear + a deer) + a great globe on a stand
    Vector3 bear = c + Vector3{ -10, 0, 11 };
    DrawModelEx(s_column, bear + Vector3{ 0, 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 3.0f, 1.2f }, fur); DrawModelEx(s_dome, bear + Vector3{ 0, 3.7f, 0.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.8f, 0.7f }, fur); for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cyl, bear + Vector3{ s * 0.7f, 3.0f, 0.3f }, Vector3{ 0,0,1 }, s * 25.0f, Vector3{ 0.3f, 1.4f, 0.3f }, fur);   // bear
    Vector3 deer = c + Vector3{ 10, 0, 11 };
    DrawModelEx(s_column, deer + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 2.0f, 2.4f }, fur); draw_bone_seg(deer + Vector3{ 0, 2.4f, 1.0f }, deer + Vector3{ 0, 3.6f, 1.8f }, 0.16f, fur); DrawModelEx(s_column, deer + Vector3{ 0, 3.7f, 2.1f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.6f, 0.9f }, fur); for (int s = -1; s <= 1; s += 2) { draw_bone_seg(deer + Vector3{ s * 0.2f, 4.0f, 2.0f }, deer + Vector3{ s * 0.9f, 4.8f, 1.6f }, 0.05f, boneDk); }   // deer + antlers
    Vector3 gl = c + Vector3{ 0, 0, 13 };
    DrawModelEx(s_cyl, gl + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 2.0f, 0.3f }, brass); DrawModelEx(s_dome, gl + Vector3{ 0, 2.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 1.2f, 1.2f }, globe); DrawModelEx(s_dome, gl + Vector3{ 0, 2.4f, 0 }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 1.2f, 1.2f, 1.2f }, globe); DrawModelEx(s_torus, gl + Vector3{ 0, 2.4f, 0 }, Vector3{ 1,0,0 }, 70.0f, Vector3{ 1.5f, 1.5f, 1.5f }, brass);   // globe + meridian ring

    // fossil panels mounted on the colonnade piers
    for (int k = 0; k < 12; k += 3) { float a = k / 12.0f * 2 * PI; Vector3 p = c + Vector3{ cosf(a) * 18.0f, 3.5f, sinf(a) * 18.0f }; DrawModelEx(s_column, p, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ 2.2f, 2.6f, 0.2f }, woodDk); for (int q = 0; q < 6; q++) { float qa = q / 6.0f * 2 * PI; DrawModelEx(s_dome, p + Vector3{ cosf(qa) * 0.6f, sinf(qa) * 0.6f, 0.1f }, Vector3{ 0,0,1 }, 0, Vector3{ 0.16f - q * 0.02f, 0.16f - q * 0.02f, 0.05f }, bone); } }   // spiral ammonite fossils

    // additive: a skylight beam down on the skeleton + soft case glow + dust motes in the light
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCylinderEx(c + Vector3{ 0, 14.0f, 1 }, c + Vector3{ 0, 0.5f, 1 }, 1.0f, 6.0f, 12, Color{ 255, 245, 220, 12 });   // skylight shaft
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.1f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 225, 170, 28 });
    SetRandomSeed(11100);
    for (int i = 0; i < 22; i++) { float bx = rand_range(-8, 8), bz = rand_range(-7, 9); DrawSphereEx(c + Vector3{ bx, 1.0f + 7.0f * fmodf(s_time * 0.1f + i * 0.1f, 1.0f), bz }, rand_range(0.04f,0.09f), 4, 4, Color{ 245, 235, 210, 36 }); }   // motes in the beam
    EndBlendMode();
}

// ---- The Mountain Monastery: a Tibetan gompa of white walls, stupas + prayer flags ----
// DRY level: draw_gompa lays the courtyard; whitewashed battered buildings + chortens ring a
// canopy of colourful prayer-flag strings. A Himalayan-Buddhist scene — not pagoda/mosque/basil.
static void draw_chorten(Vector3 b, float s, Color white, Color gold, Color red) {
    for (int t = 0; t < 3; t++) { float w = s * (2.2f - t * 0.45f); DrawModelEx(s_column, b + Vector3{ 0, 0.3f * s + t * 0.6f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, 0.6f * s, w }, (t & 1) ? white : red); }   // stepped base
    DrawModelEx(s_dome, b + Vector3{ 0, 2.0f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ s * 1.3f, s * 1.4f, s * 1.3f }, white);   // bumpa (dome)
    DrawModelEx(s_column, b + Vector3{ 0, 3.2f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ s * 0.85f, s * 0.5f, s * 0.85f }, gold);// harmika
    DrawModelEx(s_cone, b + Vector3{ 0, 4.4f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ s * 0.7f, s * 2.2f, s * 0.7f }, gold);   // 13-ring spire
    DrawModelEx(s_dome, b + Vector3{ 0, 6.0f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ s * 0.35f, s * 0.22f, s * 0.35f }, gold);// crescent moon
    DrawModelEx(s_dome, b + Vector3{ 0, 6.4f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ s * 0.2f, s * 0.2f, s * 0.2f }, gold);   // sun
}
static void draw_flagstring(Vector3 a, Vector3 b, int n) {
    Color fc[5] = { { 70,120,190,255 }, { 226,224,216,255 }, { 200,80,70,255 }, { 100,162,90,255 }, { 222,200,80,255 } };
    draw_bone_seg(a, b, 0.02f, Color{ 60,56,52,255 });
    for (int i = 0; i < n; i++) { float t = (i + 0.5f) / n; Vector3 p = Vector3Lerp(a, b, t); p.y -= sinf(t * PI) * 1.2f; DrawModelEx(s_column, p, Vector3{ 0,1,0 }, 0, Vector3{ 0.45f, 0.55f, 0.04f }, fc[i % 5]); }   // colour flags, drooping
}
static void draw_tibhouse(Vector3 b, float w, float h, float d, Color white, Color whiteDk, Color red, Color gold, Color black) {
    DrawModelEx(s_column, b + Vector3{ 0, h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, h, d }, white);                 // battered white wall
    DrawModelEx(s_column, b + Vector3{ 0, h - 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w + 0.2f, 1.0f, d + 0.2f }, red);   // maroon kemar band
    for (int wx = -1; wx <= 1; wx++) for (int wy = 0; wy < 2; wy++) { Vector3 wp = b + Vector3{ wx * w * 0.3f, 1.6f + wy * 2.4f, d * 0.51f }; DrawModelEx(s_column, wp, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 1.3f, 0.12f }, black); DrawModelEx(s_column, wp + Vector3{ 0, 0, 0.04f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.8f, 0.1f }, whiteDk); }   // trapezoidal windows (black surround)
    DrawModelEx(s_column, b + Vector3{ 0, h + 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w + 0.4f, 0.4f, d + 0.4f }, whiteDk);   // flat-roof parapet
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cyl, b + Vector3{ s * (w * 0.4f), h + 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.2f, 0.3f }, gold);   // gold roof ornaments (gyaltsen)
}

static void build_gompa() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c + Vector3{ 0,0,-15 }, 5.0f });    // temple hall
    obstacles.push_back({ c + Vector3{ -10,0,-12 }, 2.2f }); obstacles.push_back({ c + Vector3{ 10,0,-12 }, 2.2f });   // chortens
    obstacles.push_back({ c + Vector3{ -15,0,2 }, 2.4f }); obstacles.push_back({ c + Vector3{ 15,0,2 }, 2.4f });       // side houses
    obstacles.push_back({ c, 0.8f });                        // central darchen pole
    s_wisps.push_back(c + Vector3{ 0, 4.0f, -13 });          // temple
    s_wisps.push_back(c + Vector3{ -10, 4.0f, -12 }); s_wisps.push_back(c + Vector3{ 10, 4.0f, -12 });   // stupas
    s_wisps.push_back(c + Vector3{ -13, 1.5f, 6 }); s_wisps.push_back(c + Vector3{ 13, 1.5f, 6 });       // butter lamps
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_gompa() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color stone{ 170,166,158,255 }, stoneDk{ 146,142,134,255 }, white{ 228,226,218,255 }, whiteDk{ 202,200,192,255 }, red{ 150,60,56,255 }, gold{ 208,178,92,255 }, black{ 42,40,44,255 }, snow{ 234,236,240,255 }, rock{ 138,136,140,255 };

    // the flagstone courtyard + snowy mountain peaks rising beyond the boundary
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, stone);
    for (int i = -6; i <= 6; i++) { DrawModelEx(s_column, c + Vector3{ i * 3.2f, -0.04f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 1.0f, boundary_radius * 1.7f }, stoneDk); DrawModelEx(s_column, c + Vector3{ 0, -0.04f, i * 3.2f }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 1.7f, 1.0f, 0.1f }, stoneDk); }
    SetRandomSeed(11200);
    for (int i = 0; i < 16; i++) { float a = i / 16.0f * 2 * PI; float h = rand_range(10.0f, 20.0f); Vector3 p = c + Vector3{ sinf(a) * 30.0f, -1.0f, cosf(a) * 30.0f }; DrawModelEx(s_cone, p, Vector3{ 0,1,0 }, rand_range(0,60), Vector3{ rand_range(8.0f,14.0f), h, rand_range(8.0f,14.0f) }, rock); DrawModelEx(s_cone, p + Vector3{ 0, h * 0.55f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(4.0f,7.0f), h * 0.5f, rand_range(4.0f,7.0f) }, snow); }   // peaks w/ snowcaps

    // the great temple hall (white, gold roof) + two chortens + two side houses
    Vector3 th = c + Vector3{ 0, 0, -15 };
    draw_tibhouse(th, 14.0f, 9.0f, 7.0f, white, whiteDk, red, gold, black);
    DrawModelEx(s_column, th + Vector3{ 0, 9.8f, 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 12.0f, 1.4f, 6.0f }, gold);   // golden roof
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cone, th + Vector3{ s * 3.0f, 11.0f, 0.5f }, Vector3{ 0,0,1 }, s * 8.0f, Vector3{ 1.2f, 2.0f, 1.2f }, gold);   // roof finials (makara)
    DrawModelEx(s_column, th + Vector3{ 0, 2.4f, 3.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 4.4f, 0.4f }, Color{ 90,40,38,255 });   // dark maroon doorway
    DrawModelEx(s_column, th + Vector3{ 0, 4.8f, 3.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 4.0f, 0.6f, 0.4f }, gold);   // door lintel
    draw_chorten(c + Vector3{ -10, 0, -12 }, 1.4f, white, gold, red);
    draw_chorten(c + Vector3{ 10, 0, -12 }, 1.4f, white, gold, red);
    draw_tibhouse(c + Vector3{ -16, 0, 2 }, 6.0f, 7.0f, 7.0f, white, whiteDk, red, gold, black);
    draw_tibhouse(c + Vector3{ 16, 0, 2 }, 6.0f, 7.0f, 7.0f, white, whiteDk, red, gold, black);

    // the central darchen pole + a radiating canopy of prayer-flag strings
    DrawModelEx(s_cyl, c + Vector3{ 0, 5.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 10.0f, 0.18f }, red);
    DrawModelEx(s_cone, c + Vector3{ 0, 10.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.8f, 0.4f }, gold);
    Vector3 top = c + Vector3{ 0, 9.5f, 0 };
    Vector3 anchors[8] = { c + Vector3{ -16,7,2 }, c + Vector3{ 16,7,2 }, c + Vector3{ -10,5,-12 }, c + Vector3{ 10,5,-12 }, th + Vector3{ -7,9,0 }, th + Vector3{ 7,9,0 }, c + Vector3{ -14,4,14 }, c + Vector3{ 14,4,14 } };
    for (auto& an : anchors) draw_flagstring(top, an, 9);
    draw_flagstring(c + Vector3{ -16,7,2 }, c + Vector3{ 16,7,2 }, 14);   // a long cross-string

    // a row of prayer wheels along the +x wall + a couple of butter-lamp braziers
    for (int i = 0; i < 6; i++) { Vector3 p = c + Vector3{ 13.0f, 1.4f, 7.0f - i * 2.4f }; DrawModelEx(s_cyl, p, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.7f, 0.9f, 0.7f }, gold); DrawModelEx(s_cyl, p + Vector3{ 0, 0, 0 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.72f, 0.2f, 0.72f }, red); }   // prayer wheels
    for (int s = -1; s <= 1; s += 2) { Vector3 lp = c + Vector3{ s * 6.0f, 0, 8.0f }; DrawModelEx(s_cyl, lp + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.0f, 0.6f }, gold); }   // butter-lamp stands

    // additive: warm butter-lamp glow + temple glow + incense smoke + mountain mist
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.14f * sinf(s_time * 4 + i), 8, 8, Color{ 255, 180, 90, 40 });
    for (int s = -1; s <= 1; s += 2) DrawSphereEx(c + Vector3{ s * 6.0f, 1.1f, 8.0f }, 0.4f + 0.15f * sinf(s_time * 9 + s), 8, 8, Color{ 255, 150, 60, 120 });   // butter lamps
    for (int j = 0; j < 6; j++) { float y = fmodf(s_time * 0.4f + j * 0.16f, 1.0f) * 7.0f; DrawSphereEx(th + Vector3{ 0, 5.0f + y, 4.0f }, 0.3f + y * 0.08f, 6, 6, Color{ 200, 200, 205, 30 }); }   // incense
    SetRandomSeed(11201);
    for (int i = 0; i < 14; i++) { float bx = rand_range(-boundary_radius, boundary_radius), bz = rand_range(-boundary_radius, boundary_radius); DrawSphereEx(c + Vector3{ bx, 2.0f + 1.5f * sinf(s_time * 0.3f + i), bz }, rand_range(0.9f,1.8f), 6, 6, Color{ 210, 220, 230, 16 }); }   // mountain mist
    EndBlendMode();
}

// ---- The Great Buddha: a colossal seated Buddha in a cliff grotto ----
// DRY level: draw_buddha lays the courtyard; a gilded seated, meditating Buddha sits in an arched
// rock niche, flanked by bodhisattvas, with an incense burner + lanterns. A SEATED colossus.
static void draw_seated_buddha(Vector3 b, float s, Color gold, Color goldDk, Color halo, Color lotus, Color white) {
    DrawModelEx(s_cyl, b + Vector3{ 0, 0.5f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.2f * s, 1.0f * s, 4.2f * s }, lotus);     // lotus pedestal
    for (int k = 0; k < 12; k++) { float a = k / 12.0f * 2 * PI; DrawModelEx(s_cone, b + Vector3{ cosf(a) * 3.8f * s, 1.3f * s, sinf(a) * 3.8f * s }, Vector3{ cosf(a), 0.5f, sinf(a) }, 50.0f, Vector3{ 0.5f * s, 1.0f * s, 0.5f * s }, lotus); }   // lotus petals
    float legY = b.y + 2.6f * s;
    DrawModelEx(s_column, b + Vector3{ 0, legY - b.y, 1.0f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 6.4f * s, 1.8f * s, 3.6f * s }, gold);   // crossed legs / lap
    for (int s2 = -1; s2 <= 1; s2 += 2) DrawModelEx(s_dome, b + Vector3{ s2 * 2.7f * s, legY - b.y, 1.8f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f * s, 1.1f * s, 1.9f * s }, gold);   // knees
    DrawModelEx(s_dome, b + Vector3{ 0, legY - b.y + 0.7f * s, 2.4f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 1.3f * s, 0.5f * s, 0.9f * s }, goldDk);   // cupped hands (dhyana mudra)
    DrawModelEx(s_column, b + Vector3{ 0, legY - b.y + 3.0f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.2f * s, 4.2f * s, 2.9f * s }, gold);   // torso
    for (int i = 0; i < 4; i++) DrawModelEx(s_column, b + Vector3{ 0, legY - b.y + 1.4f * s + i * 0.9f * s, 1.5f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 3.6f * s, 0.2f * s, 0.3f * s }, goldDk);   // robe folds
    DrawModelEx(s_dome, b + Vector3{ 0, legY - b.y + 5.0f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.1f * s, 1.1f * s, 1.7f * s }, gold);   // shoulders
    Vector3 head = b + Vector3{ 0, legY - b.y + 6.6f * s, 0 };
    DrawModelEx(s_dome, head, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f * s, 1.6f * s, 1.4f * s }, gold);                              // head (upper)
    DrawModelEx(s_dome, head, Vector3{ 1,0,0 }, 180.0f, Vector3{ 1.4f * s, 1.0f * s, 1.4f * s }, gold);                        // head (lower)
    DrawModelEx(s_column, head + Vector3{ 0, 0.05f * s, 1.2f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.28f * s, 0.5f * s, 0.2f * s }, goldDk);   // nose
    for (int e = -1; e <= 1; e += 2) DrawModelEx(s_column, head + Vector3{ e * 0.5f * s, 0.4f * s, 1.1f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f * s, 0.07f * s, 0.1f * s }, goldDk);   // closed eyes
    DrawModelEx(s_column, head + Vector3{ 0, -0.5f * s, 1.1f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f * s, 0.07f * s, 0.1f * s }, goldDk);   // mouth
    DrawModelEx(s_dome, head + Vector3{ 0, 0.7f * s, 1.25f * s }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f * s, 0.12f * s, 0.12f * s }, white);    // urna
    DrawModelEx(s_dome, head + Vector3{ 0, 1.4f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.65f * s, 0.9f * s, 0.65f * s }, gold);              // ushnisha
    DrawModelEx(s_dome, head + Vector3{ 0, 2.3f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.25f * s, 0.35f * s, 0.25f * s }, gold);            // ushnisha tip
    for (int e = -1; e <= 1; e += 2) DrawModelEx(s_column, head + Vector3{ e * 1.35f * s, -0.3f * s, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f * s, 1.5f * s, 0.4f * s }, gold);   // long ears
    DrawModelEx(s_torus, head + Vector3{ 0, 0.2f * s, -0.7f * s }, Vector3{ 0,0,1 }, 0, Vector3{ 2.7f * s, 2.7f * s, 2.7f * s }, halo);       // halo/nimbus
}

static void build_buddha() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c + Vector3{ 0,0,-13 }, 6.0f });    // the great Buddha
    obstacles.push_back({ c + Vector3{ -9,0,-11 }, 2.0f }); obstacles.push_back({ c + Vector3{ 9,0,-11 }, 2.0f });   // bodhisattvas
    obstacles.push_back({ c + Vector3{ 0,0,4 }, 1.6f });      // incense burner
    s_wisps.push_back(c + Vector3{ 0, 10.0f, -13 });          // Buddha head/halo
    s_wisps.push_back(c + Vector3{ 0, 1.5f, 4 });             // incense burner
    s_wisps.push_back(c + Vector3{ -9, 3.0f, -11 }); s_wisps.push_back(c + Vector3{ 9, 3.0f, -11 });
    for (int s = -1; s <= 1; s += 2) s_wisps.push_back(c + Vector3{ s * 8.0f, 1.5f, 8 });   // braziers
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_buddha() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color stone{ 160,156,148,255 }, stoneDk{ 134,130,122,255 }, rock{ 178,150,114,255 }, rockDk{ 120,100,76,255 }, gold{ 116,160,132,255 }, goldDk{ 92,132,108,255 }, halo{ 220,190,116,255 }, lotus{ 96,120,100,255 }, white{ 228,226,218,255 }, bronze{ 110,140,116,255 }, red{ 168,72,60,255 }, lantern{ 232,162,82,255 };

    // the flagstone courtyard + a central path leading to the Buddha
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, stone);
    DrawModelEx(s_column, c + Vector3{ 0, 0.02f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.0f, 1.0f, boundary_radius * 2.0f }, stoneDk);
    for (int i = -8; i <= 8; i++) DrawModelEx(s_column, c + Vector3{ 0, 0.04f, i * 2.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 6.2f, 1.0f, 0.16f }, stone);

    // the rock cliff with a great arched grotto-niche framing the Buddha
    DrawModelEx(s_column, c + Vector3{ 0, 15.0f, -21 }, Vector3{ 0,1,0 }, 0, Vector3{ 46.0f, 38.0f, 8.0f }, rock);
    DrawModelEx(s_column, c + Vector3{ 0, 9.0f, -17 }, Vector3{ 0,1,0 }, 0, Vector3{ 15.0f, 18.0f, 2.0f }, rockDk);   // niche recess
    DrawModelEx(s_cone, c + Vector3{ 0, 18.0f, -17 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 7.5f, 4.5f, 2.0f }, rockDk);  // arched top of niche
    SetRandomSeed(11300);
    for (int i = 0; i < 16; i++) { float x = rand_range(-22, 22), y = rand_range(4, 30); DrawModelEx(s_column, c + Vector3{ x, y, -19.5f + rand_range(-0.4f,0.8f) }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(3.0f,6.0f), rand_range(3.0f,6.0f), 1.4f }, (i & 1) ? rock : rockDk); }   // rugged rock face

    // the colossal seated Buddha + two attendant bodhisattvas
    draw_seated_buddha(c + Vector3{ 0, 0, -13 }, 1.3f, gold, goldDk, halo, lotus, white);
    for (int s = -1; s <= 1; s += 2) { Vector3 bp = c + Vector3{ s * 9.0f, 0, -11 }; DrawModelEx(s_cyl, bp + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 2.0f, 0.7f }, gold); DrawModelEx(s_column, bp + Vector3{ 0, 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 2.4f, 0.9f }, gold); DrawModelEx(s_dome, bp + Vector3{ 0, 4.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.8f, 0.6f }, gold); DrawModelEx(s_dome, bp + Vector3{ 0, 5.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.5f, 0.3f }, gold); DrawModelEx(s_torus, bp + Vector3{ 0, 4.6f, -0.5f }, Vector3{ 0,0,1 }, 0, Vector3{ 1.1f, 1.1f, 1.1f }, halo); }   // bodhisattvas

    // a great bronze incense burner + offering table + prayer-stone cairns
    Vector3 ib = c + Vector3{ 0, 0, 4 };
    DrawModelEx(s_cyl, ib + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f, 1.2f, 1.4f }, bronze); DrawModelEx(s_dome, ib + Vector3{ 0, 1.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 0.8f, 1.5f }, bronze); for (int l = 0; l < 3; l++) DrawModelEx(s_cyl, ib + Vector3{ 0, 1.0f, 0 }, Vector3{ (l == 0) ? 1.0f : 0.0f, 1, (l == 2) ? 1.0f : 0.0f }, 90.0f, Vector3{ 0.1f, 3.6f, 0.1f }, bronze);   // burner + legs? (legs)
    DrawModelEx(s_column, c + Vector3{ 0, 0.7f, 8 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.0f, 0.4f, 1.6f }, rockDk);   // offering table
    SetRandomSeed(11301);
    for (int i = 0; i < 6; i++) { Vector3 p = c + Vector3{ rand_range(-11,11), 0, rand_range(-6,12) }; for (int t = 0; t < 4; t++) { float rr = 0.6f - t * 0.12f; DrawModelEx(s_dome, p + Vector3{ 0, 0.2f + t * 0.32f, 0 }, Vector3{ 0,1,0 }, t * 30.0f, Vector3{ rr, rr * 0.6f, rr }, (t & 1) ? stone : stoneDk); } }   // prayer-stone cairns

    // stone lanterns lining the approach
    for (int s = -1; s <= 1; s += 2) for (int j = 0; j < 3; j++) { Vector3 lp = c + Vector3{ s * 8.0f, 0, 9.0f - j * 6.0f }; DrawModelEx(s_cyl, lp + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.0f, 0.5f }, stone); DrawModelEx(s_column, lp + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.8f, 0.8f, 0.8f }, stone); DrawModelEx(s_cone, lp + Vector3{ 0, 2.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.7f, 1.0f }, stoneDk); }

    // additive: warm gilded glow on the Buddha + incense smoke + braziers + falling petals
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.14f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 200, 110, 38 });
    DrawSphereEx(c + Vector3{ 0, 1.6f, 4 }, 0.5f + 0.2f * sinf(s_time * 7), 8, 8, Color{ 255, 150, 60, 120 });   // incense embers
    for (int j = 0; j < 7; j++) { float y = fmodf(s_time * 0.4f + j * 0.14f, 1.0f) * 8.0f; DrawSphereEx(c + Vector3{ sinf(j + s_time) * 0.5f, 1.8f + y, 4 }, 0.3f + y * 0.08f, 6, 6, Color{ 200, 195, 200, 30 }); }   // incense smoke
    for (int s = -1; s <= 1; s += 2) DrawSphereEx(c + Vector3{ s * 8.0f, 1.1f, 8 }, 0.4f + 0.15f * sinf(s_time * 9 + s), 8, 8, Color{ 255, 150, 60, 110 });   // braziers
    SetRandomSeed(11302);
    for (int i = 0; i < 20; i++) { float bx = rand_range(-13, 13), bz = rand_range(-12, 12); float fall = fmodf(s_time * 0.18f + i * 0.1f, 1.0f); DrawSphereEx(c + Vector3{ bx + sinf(s_time + i) * 1.0f, 12.0f - fall * 12.0f, bz }, 0.1f, 4, 4, Color{ 240, 200, 150, 50 }); }   // falling petals
    EndBlendMode();
}

// ---- Angkor Wat: a Khmer temple-mountain of lotus-bud towers in a moat ----
// WET level: the reflecting moat is the water plane; draw_angkor lays a raised galleried temple
// platform (the fighting floor) carrying the five prasat towers + a naga causeway.
static void draw_prasat(Vector3 b, float h, float bw, Color stone, Color stoneDk, Color stoneHi) {
    DrawModelEx(s_column, b + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ bw, 2.0f, bw }, stoneDk);   // base
    int tiers = 8;
    for (int t = 0; t < tiers; t++) {
        float frac = t / (float)tiers, w = bw * 0.74f * (1.0f - frac * 0.62f), y = 2.5f + frac * h + (h / tiers) * 0.5f;
        DrawModelEx(s_column, b + Vector3{ 0, y, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, h / tiers * 1.04f, w }, (t & 1) ? stone : stoneHi);   // tier body
        DrawModelEx(s_column, b + Vector3{ 0, y + (h / tiers) * 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w * 1.18f, h / tiers * 0.22f, w * 1.18f }, stoneDk);   // tier cornice lip
        for (int k = 0; k < 4; k++) { float a = k * (PI * 0.5f) + PI * 0.25f; DrawModelEx(s_column, b + Vector3{ cosf(a) * w * 0.5f, y, sinf(a) * w * 0.5f }, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ w * 0.14f, h / tiers * 1.1f, w * 0.2f }, stoneDk); }   // corner redentations (ribs)
    }
    float topY = 2.5f + h;
    DrawModelEx(s_dome, b + Vector3{ 0, topY, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ bw * 0.3f, bw * 0.42f, bw * 0.3f }, stoneHi);   // lotus-bud
    DrawModelEx(s_cone, b + Vector3{ 0, topY + bw * 0.28f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ bw * 0.2f, bw * 0.5f, bw * 0.2f }, stoneHi);   // finial
}

static void build_angkor() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c + Vector3{ 0,0,-8 }, 4.0f });    // central prasat
    Vector3 corners[4] = { c + Vector3{ -9,0,-2 }, c + Vector3{ 9,0,-2 }, c + Vector3{ -9,0,-13 }, c + Vector3{ 9,0,-13 } };
    for (auto& p : corners) obstacles.push_back({ p, 2.6f });
    s_wisps.push_back(c + Vector3{ 0, 12.0f, -8 });          // central tower
    for (auto& p : corners) s_wisps.push_back(p + Vector3{ 0, 6.0f, 0 });
    s_wisps.push_back(c + Vector3{ 0, 1.5f, 10 });           // causeway
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_angkor() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center; float wl = water_level;
    Color stone{ 172,164,148,255 }, stoneDk{ 146,138,122,255 }, stoneHi{ 190,182,166,255 }, moss{ 108,138,90,255 }, naga{ 156,150,134,255 }, green{ 90,130,80,255 }, lotusG{ 120,170,90,255 };

    // the raised galleried temple platform (the fighting floor) on the moat + stepped edges
    DrawModelEx(s_column, c + Vector3{ 0, 0.05f, -4 }, Vector3{ 0,1,0 }, 0, Vector3{ 30.0f, 0.6f, 26.0f }, stone);
    for (int t = 1; t <= 2; t++) DrawModelEx(s_column, c + Vector3{ 0, -0.1f - t * 0.3f, -4 }, Vector3{ 0,1,0 }, 0, Vector3{ 30.0f + t * 3.0f, 0.4f, 26.0f + t * 3.0f }, stoneDk);   // terrace steps
    for (int s = -1; s <= 1; s += 2) for (int j = 0; j < 8; j++) { float z = 6.0f - j * 2.6f; DrawModelEx(s_cyl, c + Vector3{ s * 14.0f, 1.4f, z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 3.0f, 0.5f }, stoneDk); }   // gallery colonnade
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, c + Vector3{ s * 14.0f, 3.2f, -4 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 0.6f, 22.0f }, stone);   // gallery roof beam

    // the five lotus-bud prasat towers (a tall central one + four corner towers)
    draw_prasat(c + Vector3{ 0, 0, -8 }, 15.0f, 6.0f, stone, stoneDk, stoneHi);
    draw_prasat(c + Vector3{ -9, 0, -2 }, 8.0f, 4.0f, stone, stoneDk, stoneHi);
    draw_prasat(c + Vector3{ 9, 0, -2 }, 8.0f, 4.0f, stone, stoneDk, stoneHi);
    draw_prasat(c + Vector3{ -9, 0, -13 }, 8.0f, 4.0f, stone, stoneDk, stoneHi);
    draw_prasat(c + Vector3{ 9, 0, -13 }, 8.0f, 4.0f, stone, stoneDk, stoneHi);

    // the stone causeway across the moat (toward the spawn) with multi-headed naga balustrades
    DrawModelEx(s_column, c + Vector3{ 0, 0.2f, 13 }, Vector3{ 0,1,0 }, 0, Vector3{ 8.0f, 0.5f, 16.0f }, stone);
    for (int s = -1; s <= 1; s += 2) {
        for (int j = 0; j < 6; j++) { float z = 19.0f - j * 3.0f; DrawModelEx(s_column, c + Vector3{ s * 4.0f, 0.9f, z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.3f, 2.7f }, naga); }   // serpent-body railing
        Vector3 hd = c + Vector3{ s * 4.0f, 0, 20.0f };
        DrawModelEx(s_column, hd + Vector3{ 0, 1.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 2.4f, 1.0f }, naga);   // raised hood
        for (int k = 0; k < 5; k++) DrawModelEx(s_cone, hd + Vector3{ -0.8f + k * 0.4f, 3.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 0.9f, 0.18f }, naga);   // seven-headed cobra hood
    }

    // two library buildings + apsara reliefs + creeping jungle on the towers
    for (int s = -1; s <= 1; s += 2) { Vector3 lib = c + Vector3{ s * 11.0f, 0, 8.0f }; DrawModelEx(s_column, lib + Vector3{ 0, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.0f, 3.2f, 3.0f }, stone); DrawModelEx(s_column, lib + Vector3{ 0, 3.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.4f, 1.0f, 3.4f }, stoneDk); DrawModelEx(s_column, lib + Vector3{ 0, 1.4f, 1.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 2.2f, 0.3f }, Color{ 40,38,36,255 }); }   // libraries
    SetRandomSeed(11400);
    for (int i = 0; i < 14; i++) { float x = rand_range(-12, 12), z = rand_range(-15, 2); float h = rand_range(2.0f, 6.0f); Vector3 p = c + Vector3{ x, rand_range(3.0f, 10.0f), z }; draw_bone_seg(p, p + Vector3{ rand_range(-1.0f,1.0f), -h, rand_range(-1.0f,1.0f) }, 0.06f, green); DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.4f, 0.6f }, moss); }   // creeping vines on the stone
    SetRandomSeed(11401);
    for (int i = 0; i < 14; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(18.0f, 24.0f); DrawModelEx(s_dome, c + Vector3{ sinf(a) * rr, wl + 0.05f, cosf(a) * rr }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.7f,1.4f), 0.12f, rand_range(0.7f,1.4f) }, lotusG); }   // lotus pads on the moat

    // additive: warm tower glow + moat sparkle + drifting haze + birds
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 200, 120, 30 });
    SetRandomSeed(11402);
    for (int i = 0; i < 20; i++) { float a = rand_range(0, 2 * PI), rr = rand_range(16.0f, 24.0f); DrawSphereEx(c + Vector3{ sinf(a) * rr, wl + 0.2f + 0.3f * fabsf(sinf(s_time * 2 + i)), cosf(a) * rr }, 0.16f, 5, 5, Color{ 180, 210, 200, 26 }); }   // moat sparkle
    for (int i = 0; i < 4; i++) { float t = s_time * 0.4f + i * 1.6f; DrawSphereEx(c + Vector3{ sinf(t) * 13.0f, 13.0f + cosf(t * 0.6f) * 2.0f, -6.0f + cosf(t) * 10.0f }, 0.14f, 5, 5, Color{ 30, 28, 26, 120 }); }   // birds
    EndBlendMode();
}

// ---- The Wind Towers: a Persian desert town of badgirs + a yakhchal ice-house ----
// DRY level: draw_badgir lays the adobe townscape; tall slatted wind-towers rise over flat-roofed
// mud-brick houses around a great beehive yakhchal cone. Square slatted towers — not any minaret.
static void draw_windtower(Vector3 b, float h, float w, Color adobe, Color adobeDk, Color adobeHi, Color dark) {
    DrawModelEx(s_column, b + Vector3{ 0, h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, h, w }, adobe);   // tall square shaft
    for (int f = 0; f < 4; f++) { float a = f * (PI * 0.5f); Vector3 dir{ sinf(a), 0, cosf(a) }, perp{ cosf(a), 0, -sinf(a) };
        DrawModelEx(s_column, b + dir * (w * 0.5f) + Vector3{ 0, h * 0.82f, 0 }, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ w * 0.86f, h * 0.28f, 0.1f }, dark);   // dark vent recess
        for (int s = -2; s <= 2; s++) DrawModelEx(s_column, b + dir * (w * 0.53f) + perp * (s * w * 0.17f) + Vector3{ 0, h * 0.82f, 0 }, Vector3{ 0,1,0 }, -a * RAD2DEG, Vector3{ w * 0.07f, h * 0.3f, 0.12f }, adobeHi);   // vertical mullions
    }
    DrawModelEx(s_column, b + Vector3{ 0, h + 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w + 0.6f, 0.6f, w + 0.6f }, adobeDk);   // overhanging cap
}
static void draw_adobehouse(Vector3 b, float w, float h, float d, Color adobe, Color adobeDk, Color dark) {
    DrawModelEx(s_column, b + Vector3{ 0, h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w, h, d }, adobe);
    DrawModelEx(s_column, b + Vector3{ 0, h + 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w + 0.3f, 0.4f, d + 0.3f }, adobeDk);   // flat-roof parapet
    DrawModelEx(s_column, b + Vector3{ 0, h * 0.4f, d * 0.51f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, h * 0.55f, 0.2f }, dark); // arched doorway
    DrawModelEx(s_cone, b + Vector3{ 0, h * 0.68f, d * 0.51f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.55f, 0.5f, 0.2f }, dark);  // arch top
}

static void build_badgir() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c + Vector3{ 0,0,-15 }, 6.0f });    // the yakhchal
    Vector3 tw[5] = { c + Vector3{ -11,0,-6 }, c + Vector3{ 11,0,-6 }, c + Vector3{ -9,0,4 }, c + Vector3{ 9,0,4 }, c + Vector3{ 0,0,-9 } };
    for (auto& p : tw) obstacles.push_back({ p, 2.2f });      // badgir towers / houses
    obstacles.push_back({ c + Vector3{ -15,0,8 }, 2.4f }); obstacles.push_back({ c + Vector3{ 15,0,8 }, 2.4f });   // side houses
    s_wisps.push_back(c + Vector3{ 0, 8.0f, -15 });           // yakhchal
    for (int i = 0; i < 4; i++) s_wisps.push_back(tw[i] + Vector3{ 0, 6.0f, 0 });
    s_wisps.push_back(c + Vector3{ 0, 1.5f, 6 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_badgir() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color adobe{ 200,170,130,255 }, adobeDk{ 172,144,106,255 }, adobeHi{ 216,188,148,255 }, ground{ 188,162,126,255 }, groundDk{ 164,140,106,255 }, dark{ 44,36,30,255 }, palmTrunk{ 122,90,58,255 }, palmFrond{ 90,142,72,255 }, palmFruit{ 150,110,60,255 };

    // the packed-earth town floor + worn alley joints
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, ground);
    SetRandomSeed(11500);
    for (int i = 0; i < 12; i++) DrawModelEx(s_column, c + Vector3{ rand_range(-14,14), -0.02f, rand_range(-14,14) }, Vector3{ 0,1,0 }, rand_range(0,90), Vector3{ rand_range(2.0f,5.0f), 1.0f, 0.6f }, groundDk);

    // the great yakhchal (beehive ice-house): a cylindrical base + a tall ring-coursed adobe cone
    Vector3 yk = c + Vector3{ 0, 0, -15 };
    DrawModelEx(s_cyl, yk + Vector3{ 0, 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.0f, 6.0f, 6.0f }, adobe);
    DrawModelEx(s_cone, yk + Vector3{ 0, 6.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.0f, 11.0f, 6.0f }, adobe);
    for (int r = 0; r < 7; r++) { float y = 6.5f + r * 1.5f; float rr = 6.0f * (1.0f - r / 8.0f); DrawModelEx(s_cyl, yk + Vector3{ 0, y, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rr + 0.2f, 0.18f, rr + 0.2f }, adobeDk); }   // ring courses
    DrawModelEx(s_cyl, yk + Vector3{ 0, 17.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.35f, 0.7f, 0.35f }, adobeDk);   // top vent
    DrawModelEx(s_column, yk + Vector3{ 0, 2.0f, 6.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 3.0f, 0.4f }, dark);   // entrance

    // flat-roofed mud-brick houses (clustered + terraced) with badgirs rising over them
    draw_adobehouse(c + Vector3{ -15, 0, 8 }, 6.0f, 5.0f, 6.0f, adobe, adobeDk, dark);
    draw_adobehouse(c + Vector3{ 15, 0, 8 }, 6.0f, 5.5f, 6.0f, adobe, adobeDk, dark);
    draw_adobehouse(c + Vector3{ -13, 0, -2 }, 5.0f, 4.5f, 5.0f, adobeDk, adobe, dark);
    draw_adobehouse(c + Vector3{ 13, 0, -2 }, 5.0f, 4.5f, 5.0f, adobeDk, adobe, dark);
    draw_adobehouse(c + Vector3{ 0, 0, -9 }, 7.0f, 5.0f, 5.0f, adobe, adobeDk, dark);
    // a couple of small adobe domes on rooftops
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_dome, c + Vector3{ s * 15.0f, 5.5f, 8 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 1.6f, 2.0f }, adobeDk);

    // the badgir wind-towers (varying heights), some standalone, some atop the houses
    draw_windtower(c + Vector3{ -11, 0, -6 }, 12.0f, 2.4f, adobe, adobeDk, adobeHi, dark);
    draw_windtower(c + Vector3{ 11, 0, -6 }, 14.0f, 2.6f, adobe, adobeDk, adobeHi, dark);
    draw_windtower(c + Vector3{ -9, 0, 4 }, 10.0f, 2.2f, adobe, adobeDk, adobeHi, dark);
    draw_windtower(c + Vector3{ 9, 0, 4 }, 11.0f, 2.2f, adobe, adobeDk, adobeHi, dark);
    draw_windtower(c + Vector3{ 0, 5.0f, -9 }, 8.0f, 2.0f, adobe, adobeDk, adobeHi, dark);   // atop the central house
    draw_windtower(c + Vector3{ 15, 5.5f, 8 }, 7.0f, 1.8f, adobe, adobeDk, adobeHi, dark);   // atop a side house

    // a low domed cistern (ab anbar) + date palms + a few clay water jars
    Vector3 ci = c + Vector3{ -16, 0, -8 };
    DrawModelEx(s_cyl, ci + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 2.0f, 3.0f }, adobeDk); DrawModelEx(s_dome, ci + Vector3{ 0, 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f, 2.0f, 3.2f }, adobe); draw_windtower(ci + Vector3{ 0, 3.0f, 0 }, 6.0f, 1.6f, adobe, adobeDk, adobeHi, dark);
    draw_palm(c + Vector3{ -6, 0, 12 }, 8.0f, palmTrunk, palmFrond, palmFruit);
    draw_palm(c + Vector3{ 6, 0, 13 }, 7.0f, palmTrunk, palmFrond, palmFruit);
    SetRandomSeed(11501);
    for (int i = 0; i < 6; i++) DrawModelEx(s_cyl, c + Vector3{ rand_range(-10,10), 0.5f, rand_range(-4,10) }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.0f, 0.5f }, adobeDk);   // clay jars

    // additive: warm sun glow + drifting dust + heat shimmer + pigeons
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 210, 140, 26 });
    SetRandomSeed(11502);
    for (int i = 0; i < 18; i++) { float bx = rand_range(-14, 14), bz = rand_range(-14, 12); DrawSphereEx(c + Vector3{ bx, 1.5f + 3.0f * fabsf(sinf(s_time * 0.3f + i)), bz }, rand_range(0.5f,1.1f), 6, 6, Color{ 255, 240, 200, 12 }); }   // heat shimmer
    for (int i = 0; i < 5; i++) { float t = s_time * 0.7f + i * 1.3f; DrawSphereEx(c + Vector3{ sinf(t) * 11.0f, 9.0f + cosf(t * 0.7f) * 2.0f, -4.0f + cosf(t) * 9.0f }, 0.1f, 4, 4, Color{ 200, 195, 195, 90 }); }   // pigeons
    EndBlendMode();
}

// ---- The Topiary Garden: a formal garden of clipped-hedge sculptures ----
// DRY level: draw_topiary lays the lawn + gravel parterre; shaped topiaries (a peacock, an
// elephant, spirals, cones, spheres + an arch) ring a fountain. Shaped hedges, not maze walls.
static void draw_conetree(Vector3 b, float h, float r, Color hedge, Color hedgeDk, Color trunk) {
    DrawModelEx(s_cyl, b + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.8f, 0.2f }, trunk);
    for (int t = 0; t < 3; t++) { float rr = r * (1.0f - t * 0.28f); DrawModelEx(s_cone, b + Vector3{ 0, 0.8f + t * h * 0.3f + h * 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ rr, h * 0.42f, rr }, (t & 1) ? hedge : hedgeDk); }   // tiered cone
}
static void draw_spiraltopiary(Vector3 b, float h, float r, Color hedge, Color hedgeDk, Color trunk) {
    DrawModelEx(s_cyl, b + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.8f, 0.2f }, trunk);
    DrawModelEx(s_cone, b + Vector3{ 0, 0.8f + h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ r, h, r }, hedge);
    for (int i = 0; i < 22; i++) { float t = i / 22.0f, a = t * PI * 4.0f, rr = r * (1.0f - t) * 0.52f; DrawModelEx(s_column, b + Vector3{ cosf(a) * rr, 0.9f + t * h * 0.92f, sinf(a) * rr }, Vector3{ 0,1,0 }, a * RAD2DEG, Vector3{ 0.18f, 0.16f, 0.5f }, hedgeDk); }   // spiral groove
}

static void build_topiary() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c, 2.6f });   // central fountain
    obstacles.push_back({ c + Vector3{ -9,0,-6 }, 2.2f }); obstacles.push_back({ c + Vector3{ 9,0,-6 }, 2.2f });   // peacock + elephant
    obstacles.push_back({ c + Vector3{ -9,0,6 }, 1.8f }); obstacles.push_back({ c + Vector3{ 9,0,6 }, 1.8f });     // spirals
    for (int s = -1; s <= 1; s += 2) obstacles.push_back({ c + Vector3{ s * 4.0f, 0, 14 }, 1.0f });   // arch piers
    s_wisps.push_back(c + Vector3{ 0, 2.5f, 0 });           // fountain
    s_wisps.push_back(c + Vector3{ -9, 2.0f, -6 }); s_wisps.push_back(c + Vector3{ 9, 2.0f, -6 });
    for (int s = -1; s <= 1; s += 2) s_wisps.push_back(c + Vector3{ s * 12.0f, 1.5f, 2 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 5.0f }); }
}

static void draw_topiary() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color lawn{ 122,162,92,255 }, lawnDk{ 104,144,80,255 }, hedge{ 96,142,80,255 }, hedgeDk{ 78,124,66,255 }, hedgeHi{ 122,162,98,255 }, gravel{ 198,182,152,255 }, marble{ 224,220,210,255 }, marbleDk{ 198,194,184,255 }, gold{ 206,176,96,255 }, trunk{ 110,84,58,255 };
    Color flw[4] = { { 204,92,102,255 }, { 222,202,92,255 }, { 172,112,184,255 }, { 228,224,216,255 } };

    // the manicured lawn + a gravel cross-axis path + a central round gravel court
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, lawn);
    DrawModelEx(s_column, c + Vector3{ 0, 0.02f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.0f, 1.0f, boundary_radius * 2.0f }, gravel);
    DrawModelEx(s_column, c + Vector3{ 0, 0.02f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.0f, 1.0f, 5.0f }, gravel);
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.03f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 5.0f, 1.0f, 5.0f }, gravel);

    // a box parterre: low clipped-hedge borders in the four quadrants + flower beds
    Vector3 quad[4] = { c + Vector3{ -10,0,-10 }, c + Vector3{ 10,0,-10 }, c + Vector3{ -10,0,10 }, c + Vector3{ 10,0,10 } };
    for (int q = 0; q < 4; q++) { Vector3 bd = quad[q]; for (int s = -1; s <= 1; s += 2) { DrawModelEx(s_column, bd + Vector3{ 0, 0.4f, s * 4.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 9.5f, 0.8f, 0.5f }, hedgeDk); DrawModelEx(s_column, bd + Vector3{ s * 4.5f, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.8f, 9.5f }, hedgeDk); } SetRandomSeed(11600 + q); for (int f = 0; f < 10; f++) { Vector3 p = bd + Vector3{ rand_range(-3.5f,3.5f), 0.4f, rand_range(-3.5f,3.5f) }; DrawModelEx(s_dome, p, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(0.3f,0.6f), rand_range(0.3f,0.6f), rand_range(0.3f,0.6f) }, flw[(q + f) % 4]); } }

    // the central tiered marble fountain
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 1.0f, 2.4f }, marble);
    DrawModelEx(s_cyl, c + Vector3{ 0, 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.4f, 0.5f }, marbleDk);
    DrawModelEx(s_cyl, c + Vector3{ 0, 2.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 0.3f, 1.2f }, marble);
    DrawModelEx(s_dome, c + Vector3{ 0, 2.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.4f, 0.4f }, gold);

    // ===== the topiary sculptures =====
    // a peacock (body + curved neck + head + a vertical fan of feathers)
    Vector3 pk = c + Vector3{ -9, 0, -6 };
    DrawModelEx(s_dome, pk + Vector3{ 0, 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 1.4f, 1.7f }, hedge);
    DrawModelEx(s_cyl, pk + Vector3{ 0, 2.2f, 0.9f }, Vector3{ 1,0,0 }, -40.0f, Vector3{ 0.3f, 1.2f, 0.3f }, hedge);
    DrawModelEx(s_dome, pk + Vector3{ 0, 3.0f, 1.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.5f, 0.4f }, hedge);
    for (int f = -3; f <= 3; f++) { float a = f * 0.28f; DrawModelEx(s_cone, pk + Vector3{ sinf(a) * 2.4f, 1.6f + (3 - abs(f)) * 0.4f, -1.6f }, Vector3{ 0,0,1 }, -a * 30.0f, Vector3{ 0.3f, 2.6f, 0.3f }, (f & 1) ? hedge : hedgeHi); DrawModelEx(s_dome, pk + Vector3{ sinf(a) * 3.4f, 1.6f + (3 - abs(f)) * 0.4f + 2.4f, -1.6f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.3f, 0.3f }, flw[(f + 3) % 4]); }   // tail "eyes"

    // an elephant (body + four legs + head + curling trunk + ears)
    Vector3 el = c + Vector3{ 9, 0, -6 };
    DrawModelEx(s_column, el + Vector3{ 0, 1.8f, -0.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 2.0f, 3.4f }, hedge);
    for (int x = -1; x <= 1; x += 2) for (int z = -1; z <= 1; z += 2) DrawModelEx(s_cyl, el + Vector3{ x * 0.7f, 0.7f, z * 1.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 1.4f, 0.4f }, hedgeDk);
    DrawModelEx(s_dome, el + Vector3{ 0, 2.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.1f, 1.0f, 1.4f }, hedge);   // hump
    DrawModelEx(s_dome, el + Vector3{ 0, 2.0f, 1.9f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 1.2f, 1.0f }, hedge);   // head
    draw_bone_seg(el + Vector3{ 0, 1.8f, 2.5f }, el + Vector3{ 0, 0.4f, 3.6f }, 0.32f, hedge);   // trunk
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_dome, el + Vector3{ s * 0.9f, 2.0f, 1.7f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 1.0f, 0.25f }, hedgeHi);   // ears

    // two spiral topiaries + cone-trees lining the path + sphere-on-stem topiaries
    draw_spiraltopiary(c + Vector3{ -9, 0, 6 }, 4.0f, 1.4f, hedge, hedgeDk, trunk);
    draw_spiraltopiary(c + Vector3{ 9, 0, 6 }, 4.0f, 1.4f, hedge, hedgeDk, trunk);
    for (int s = -1; s <= 1; s += 2) for (int j = 0; j < 3; j++) draw_conetree(c + Vector3{ s * 3.3f, 0, 11.0f - j * 7.0f }, 3.5f, 1.0f, hedge, hedgeDk, trunk);
    for (int s = -1; s <= 1; s += 2) { Vector3 sp = c + Vector3{ s * 13.0f, 0, 0 }; DrawModelEx(s_cyl, sp + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.25f, 2.0f, 0.25f }, trunk); DrawModelEx(s_dome, sp + Vector3{ 0, 2.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.3f, 1.3f, 1.3f }, hedge); DrawModelEx(s_dome, sp + Vector3{ 0, 2.4f, 0 }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 1.3f, 1.3f, 1.3f }, hedge); }   // sphere topiaries

    // a clipped-hedge entrance archway at the front + marble urns at the path corners
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, c + Vector3{ s * 4.0f, 2.0f, 14 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 4.0f, 1.4f }, hedge);
    DrawModelEx(s_column, c + Vector3{ 0, 4.4f, 14 }, Vector3{ 0,1,0 }, 0, Vector3{ 9.0f, 1.4f, 1.4f }, hedge);   // arch top
    DrawModelEx(s_cone, c + Vector3{ 0, 4.0f, 14 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 2.6f, 2.0f, 1.0f }, hedgeDk);   // arched underside cut
    for (int qx = -1; qx <= 1; qx += 2) for (int qz = -1; qz <= 1; qz += 2) { Vector3 up = c + Vector3{ qx * 3.0f, 0, qz * 3.0f }; DrawModelEx(s_cyl, up + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 1.0f, 0.4f }, marbleDk); DrawModelEx(s_dome, up + Vector3{ 0, 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.8f, 0.7f }, marble); }   // urns

    // additive: warm sun glow + fountain jet + butterflies + birds
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.4f + 0.12f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 220, 150, 26 });
    for (int j = 0; j < 6; j++) { float fy = fmodf(s_time * 1.6f + j * 0.16f, 1.0f); DrawSphereEx(c + Vector3{ sinf(j * 1.7f) * 0.3f, 2.9f + fy * 1.3f, cosf(j * 1.7f) * 0.3f }, 0.14f, 5, 5, Color{ 150, 200, 220, 110 }); }   // fountain jet
    for (int i = 0; i < 6; i++) { float t = s_time * 0.9f + i * 1.0f; DrawSphereEx(c + Vector3{ sinf(t) * 10.0f, 2.0f + cosf(t * 1.3f) * 1.0f, -3.0f + cosf(t) * 9.0f }, 0.12f, 4, 4, Color{ 230, 180, 90, 80 }); }   // butterflies
    EndBlendMode();
}

// ---- The Glassworks: a glassblower's hot shop, dim and lit by furnace fire ----
// DRY level: a great brick melting furnace (arched glowing mouth + chimney) dominates the back;
// cylindrical glory-hole reheating furnaces, blowpipe benches carrying molten gathers, a steel
// marver, tall racks of colourful blown vessels, an annealing oven + sand barrels ring the shop.
// Distinct silhouette: industrial brick furnaces + glowing apertures, not hedges/kilns/foundry.
static void draw_glassrack(Vector3 b, Color wood, Color woodDk, const Color* glass, int seed) {
    // a tall timber shelving rack of blown vessels (vases / bottles / goblets)
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, b + Vector3{ s * 1.5f, 1.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 3.2f, 0.6f }, woodDk);   // posts
    for (int sh = 0; sh < 4; sh++) { float y = 0.6f + sh * 0.85f; DrawModelEx(s_column, b + Vector3{ 0, y, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.2f, 0.12f, 0.6f }, wood);   // shelf
        SetRandomSeed(seed + sh * 31);
        for (int v = 0; v < 5; v++) { float vx = -1.2f + v * 0.6f; Color gc = glass[(sh * 5 + v) % 6]; int kind = (seed + sh + v) % 3;
            if (kind == 0) { DrawModelEx(s_cyl, b + Vector3{ vx, y + 0.08f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 0.5f, 0.16f }, gc); DrawModelEx(s_dome, b + Vector3{ vx, y + 0.58f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 0.14f, 0.16f }, gc); }   // bottle
            else if (kind == 1) { DrawModelEx(s_dome, b + Vector3{ vx, y + 0.26f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, 0.34f, 0.22f }, gc); DrawModelEx(s_cyl, b + Vector3{ vx, y + 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 0.16f, 0.1f }, gc); }   // vase
            else { DrawModelEx(s_cyl, b + Vector3{ vx, y + 0.1f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.06f, 0.28f, 0.06f }, gc); DrawModelEx(s_dome, b + Vector3{ vx, y + 0.46f, 0 }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 0.2f, 0.24f, 0.2f }, gc); } }   // goblet
    }
}
static void draw_gloryhole(Vector3 b, Color brick, Color brickDk, Color iron) {
    // a squat cylindrical reheating furnace on legs with a glowing round port facing front
    for (int s = -1; s <= 1; s += 2) for (int z = -1; z <= 1; z += 2) DrawModelEx(s_column, b + Vector3{ s * 0.55f, 0.4f, z * 0.55f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 0.8f, 0.16f }, iron);   // legs
    DrawModelEx(s_cyl, b + Vector3{ 0, 1.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.95f, 1.0f, 0.95f }, brick);   // drum
    DrawModelEx(s_cyl, b + Vector3{ 0, 1.3f, 0 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.42f, 1.0f, 0.42f }, brickDk);   // glory-hole bore through it
    DrawModelEx(s_dome, b + Vector3{ 0, 1.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.95f, 0.5f, 0.95f }, brickDk);   // crown
    DrawModelEx(s_cyl, b + Vector3{ 0, 2.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, 0.4f, 0.22f }, iron);   // short flue
}
static void draw_blowbench(Vector3 b, float yaw, Color wood, Color iron, Color glow) {
    // a glassblower's bench: a long timber rail + two steel arm-rails + a blowpipe with a gather
    rlPushMatrix();
    rlTranslatef(b.x, b.y, b.z); rlRotatef(yaw, 0, 1, 0);
    DrawModelEx(s_column, Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 0.18f, 0.5f }, wood);
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, Vector3{ s * 1.0f, 0.45f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 0.9f, 0.4f }, wood);   // legs
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, Vector3{ 0, 1.05f, s * 0.32f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 0.06f, 0.06f }, iron);   // arm-rails
    // a blowpipe lying across the rails with a glowing molten gather on the end
    DrawModelEx(s_cyl, Vector3{ 0.2f, 1.12f, 0 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.04f, 2.0f, 0.04f }, iron);
    DrawModelEx(s_dome, Vector3{ 0.2f, 1.12f, 1.05f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 0.2f, 0.18f }, glow);
    DrawModelEx(s_dome, Vector3{ 0.2f, 1.12f, 1.05f }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 0.18f, 0.2f, 0.18f }, glow);
    rlPopMatrix();
}

static void build_glassworks() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    obstacles.push_back({ c + Vector3{ 0,0,-13 }, 4.2f });   // the great melting furnace at the back
    for (int s = -1; s <= 1; s += 2) obstacles.push_back({ c + Vector3{ s * 8.5f, 0, -4 }, 1.2f });   // glory holes
    for (int s = -1; s <= 1; s += 2) obstacles.push_back({ c + Vector3{ s * 4.5f, 0, 3 }, 1.4f });     // blowing benches
    for (int s = -1; s <= 1; s += 2) obstacles.push_back({ c + Vector3{ s * 12.0f, 0, 6 }, 1.0f });    // vessel racks
    obstacles.push_back({ c + Vector3{ 9, 0, -11 }, 1.6f });   // annealing oven
    // furnace mouth + chimney crown + the two glory-hole ports glow
    s_wisps.push_back(c + Vector3{ 0, 1.6f, -11 });            // furnace mouth
    s_wisps.push_back(c + Vector3{ 0, 8.5f, -13 });            // chimney top
    for (int s = -1; s <= 1; s += 2) s_wisps.push_back(c + Vector3{ s * 8.5f, 1.3f, -4 });   // glory holes
    for (int s = -1; s <= 1; s += 2) s_wisps.push_back(c + Vector3{ s * 4.5f, 1.2f, 4.0f }); // molten gathers on benches
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 6.0f }); }
}

static void draw_glassworks() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color floorC{ 60,52,48,255 }, floorDk{ 48,42,38,255 }, brick{ 120,72,54,255 }, brickDk{ 86,52,40,255 }, brickHi{ 150,96,72,255 }, iron{ 54,50,52,255 }, ironDk{ 40,37,39,255 }, wood{ 116,84,54,255 }, woodDk{ 88,62,40,255 }, sand{ 156,138,104,255 };
    Color molten{ 255,150,40,255 }, ember{ 255,96,24,255 };
    Color glass[6] = { { 70,130,170,235 }, { 90,170,120,235 }, { 190,70,72,235 }, { 214,170,70,235 }, { 150,100,180,235 }, { 210,214,220,225 } };

    // the swept brick shop floor + a soot-darkened working apron in front of the furnaces
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, floorC);
    DrawModelEx(s_column, c + Vector3{ 0, 0.01f, -6 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.0f, 0.5f, 14.0f }, floorDk);

    // ===== the great melting furnace at the back: brick mass + glowing arched mouth + chimney =====
    Vector3 fz = c + Vector3{ 0, 0, -13 };
    DrawModelEx(s_column, fz + Vector3{ 0, 2.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 8.0f, 5.2f, 4.0f }, brick);   // furnace body
    DrawModelEx(s_column, fz + Vector3{ 0, 5.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 8.4f, 0.5f, 4.4f }, brickDk);   // cap course
    // brick course banding
    for (int b = 0; b < 5; b++) DrawModelEx(s_column, fz + Vector3{ 0, 0.7f + b * 1.0f, 2.01f }, Vector3{ 0,1,0 }, 0, Vector3{ 8.0f, 0.12f, 0.1f }, brickDk);
    // the glowing arched melting mouth (recessed dark bore + a bright molten interior + arch ring)
    DrawModelEx(s_column, fz + Vector3{ 0, 1.5f, 2.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.0f, 2.6f, 0.6f }, brickHi);   // surround
    DrawModelEx(s_dome, fz + Vector3{ 0, 2.6f, 2.1f }, Vector3{ 1,0,0 }, -90.0f, Vector3{ 1.5f, 0.5f, 1.4f }, brickHi);   // arch top
    DrawModelEx(s_column, fz + Vector3{ 0, 1.4f, 2.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 2.0f, 0.4f }, ember);       // glowing maw
    DrawModelEx(s_dome, fz + Vector3{ 0, 2.4f, 2.25f }, Vector3{ 1,0,0 }, -90.0f, Vector3{ 1.1f, 0.4f, 1.0f }, ember);   // glowing arch
    // the tapering brick chimney rising from the crown
    DrawModelEx(s_column, fz + Vector3{ 0, 6.6f, -0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 2.6f, 2.4f }, brick);
    DrawModelEx(s_column, fz + Vector3{ 0, 8.6f, -0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.8f, 1.6f, 1.8f }, brickDk);
    DrawModelEx(s_column, fz + Vector3{ 0, 9.6f, -0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 0.4f, 2.2f }, brickHi);   // chimney lip

    // ===== glory-hole reheating furnaces flanking the working apron =====
    for (int s = -1; s <= 1; s += 2) draw_gloryhole(c + Vector3{ s * 8.5f, 0, -4 }, brick, brickDk, iron);

    // ===== glassblowers' benches with blowpipes + glowing gathers, facing the furnace =====
    draw_blowbench(c + Vector3{ -4.5f, 0, 3 }, 90.0f, wood, iron, molten);
    draw_blowbench(c + Vector3{ 4.5f, 0, 3 }, 90.0f, wood, iron, molten);
    // a heavy steel marver table between the benches for rolling the gather
    DrawModelEx(s_column, c + Vector3{ 0, 0.95f, 4 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 0.16f, 1.1f }, iron);
    for (int x = -1; x <= 1; x += 2) for (int z = -1; z <= 1; z += 2) DrawModelEx(s_column, c + Vector3{ x * 0.9f, 0.45f, 4 + z * 0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 0.9f, 0.16f }, ironDk);

    // ===== tall racks of finished, colourful blown vessels along the side walls =====
    draw_glassrack(c + Vector3{ -12.0f, 0, 6 }, wood, woodDk, glass, 4100);
    draw_glassrack(c + Vector3{ 12.0f, 0, 6 }, wood, woodDk, glass, 4200);

    // ===== an annealing oven (brick box + small glowing door) in the back corner =====
    Vector3 an = c + Vector3{ 9, 0, -11 };
    DrawModelEx(s_column, an + Vector3{ 0, 1.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.4f, 2.8f, 2.4f }, brick);
    DrawModelEx(s_column, an + Vector3{ 0, 2.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.6f, 0.3f, 2.6f }, brickDk);
    DrawModelEx(s_column, an + Vector3{ 0, 1.0f, 1.21f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 1.2f, 0.1f }, ember);   // glowing door
    DrawModelEx(s_cyl, an + Vector3{ 0, 3.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.5f, 0.2f }, iron);

    // ===== sand barrels + crates of cullet + a water trough scattered around the apron =====
    Vector3 props[5] = { c + Vector3{ -10, 0, -2 }, c + Vector3{ -7.5f, 0, 8 }, c + Vector3{ 7.0f, 0, 8 }, c + Vector3{ 11, 0, -2 }, c + Vector3{ -2.5f, 0, 9 } };
    for (int i = 0; i < 5; i++) { Vector3 p = props[i];
        if (i % 2 == 0) { DrawModelEx(s_cyl, p + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.55f, 1.2f, 0.55f }, woodDk); DrawModelEx(s_cyl, p + Vector3{ 0, 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.1f, 0.5f }, sand); for (int h = 0; h < 3; h++) DrawModelEx(s_cyl, p + Vector3{ 0, 0.3f + h * 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.58f, 0.05f, 0.58f }, iron); }   // sand barrel + hoops
        else { DrawModelEx(s_column, p + Vector3{ 0, 0.45f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.9f, 1.0f }, wood); DrawModelEx(s_column, p + Vector3{ 0, 0.9f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.9f, 0.1f, 0.9f }, woodDk); }   // crate
    }
    // a low water trough for quenching tools
    DrawModelEx(s_column, c + Vector3{ -11, 0.4f, 1 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.8f, 0.8f, 0.9f }, ironDk);
    DrawModelEx(s_column, c + Vector3{ -11, 0.55f, 1 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.5f, 0.6f, 0.7f }, Color{ 40,70,90,200 });

    // ===== exposed roof trusses overhead to enclose the shop =====
    for (int t = -2; t <= 2; t++) { DrawModelEx(s_column, c + Vector3{ t * 6.0f, 7.5f, 0 }, Vector3{ 0,0,1 }, 0, Vector3{ 0.3f, 0.3f, boundary_radius * 2.0f }, woodDk);
        for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, c + Vector3{ t * 6.0f, 6.6f, s * 8.0f }, Vector3{ 1,0,0 }, s * 28.0f, Vector3{ 0.24f, 2.4f, 0.24f }, woodDk); }

    // additive: furnace glow, rising sparks, heat shimmer over the mouths
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.16f * sinf(s_time * 3 + i), 8, 8, Color{ 255, 140, 50, 40 });
    // the big furnace mouth throws a broad pulsing glow
    DrawSphereEx(fz + Vector3{ 0, 1.6f, 2.4f }, 1.6f + 0.3f * sinf(s_time * 2.4f), 10, 10, Color{ 255, 110, 36, 50 });
    // rising sparks + embers off the furnace and glory holes
    SetRandomSeed(4321);
    for (int i = 0; i < 40; i++) { float sx = rand_range(-9, 9), base = rand_range(0, 6); float fy = fmodf(s_time * 1.4f + i * 0.21f, 1.0f); Vector3 src = (i % 3 == 0) ? (c + Vector3{ sx * 0.3f, 1.0f, -11 }) : (c + Vector3{ (i & 1 ? 8.5f : -8.5f) + rand_range(-0.6f,0.6f), 1.3f, -4 }); DrawSphereEx(src + Vector3{ rand_range(-0.3f,0.3f), base * 0.4f + fy * 3.5f, rand_range(-0.3f,0.3f) }, 0.05f, 4, 4, Color{ 255, 170, 70, (unsigned char)(160 * (1.0f - fy)) }); }
    // heat shimmer rising in front of the melting mouth
    for (int i = 0; i < 10; i++) DrawSphereEx(fz + Vector3{ rand_range(-2.0f,2.0f), 1.0f + 4.0f * fabsf(sinf(s_time * 0.4f + i)), 2.6f }, rand_range(0.3f,0.7f), 6, 6, Color{ 255, 200, 150, 14 });
    EndBlendMode();
}

// ---- The Shipyard: a shipwright's slip with a great hull standing in frame ----
// DRY level: the centrepiece is a SKELETAL wooden ship under construction — a keel on
// blocks, ~10 curved frame ribs (a boat-shaped ribcage), a raked stem + sternpost, and the
// port side half-planked — sitting on greased launching ways that run down to a strip of
// harbour water. Scaffolding, sheer-legs, timber stacks, a steam box, a sawpit, a tar fire.
// Distinct silhouette: a hull-IN-FRAME (ribs + scaffold), not a finished/sunken ship.
static void draw_timberstack(Vector3 b, Color log, Color logEnd) {
    // logs stacked in a tapering pile, lying along z
    for (int r = 0; r < 3; r++) { int cnt = 4 - r; float yy = 0.4f + r * 0.6f;
        for (int i = 0; i < cnt; i++) { float xx = (i - (cnt - 1) / 2.0f) * 0.72f;
            DrawModelEx(s_cyl, b + Vector3{ xx, yy, 0 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.33f, 3.2f, 0.33f }, log);
            DrawModelEx(s_cyl, b + Vector3{ xx, yy, 1.6f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.35f, 0.06f, 0.35f }, logEnd); } }
}
static void draw_scaffold(Vector3 b, float len, Color pole, Color plank) {
    // a run of scaffolding: vertical poles + two staging planks running in z
    for (float z = -len * 0.5f; z <= len * 0.5f + 0.01f; z += 2.4f) { DrawModelEx(s_cyl, b + Vector3{ 0, 2.4f, z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 4.8f, 0.1f }, pole);
        DrawModelEx(s_cyl, b + Vector3{ 0, 4.4f, z }, Vector3{ 1,0,0 }, 24.0f, Vector3{ 0.08f, 2.8f, 0.08f }, pole); }   // diagonal braces
    for (int s = 0; s < 2; s++) { float y = 2.5f + s * 1.9f; DrawModelEx(s_column, b + Vector3{ 0, y, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.7f, 0.1f, len }, plank); }
}

static void build_shipyard() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    // the hull spine runs along z from the stern (-13) to the bow (+2); block it off
    for (int z = -12; z <= 2; z += 3) obstacles.push_back({ c + Vector3{ 0, 0, (float)z }, 2.7f });
    obstacles.push_back({ c + Vector3{ -12, 0, -2 }, 1.7f }); obstacles.push_back({ c + Vector3{ 12, 0, -2 }, 1.7f });   // timber stacks
    obstacles.push_back({ c + Vector3{ -11, 0, 7 }, 1.5f });   // steam box
    obstacles.push_back({ c + Vector3{ 9, 0, 8 }, 1.2f });     // tar fire
    obstacles.push_back({ c + Vector3{ 11, 0, 2 }, 1.6f });    // sawpit
    for (int s = -1; s <= 1; s += 2) obstacles.push_back({ c + Vector3{ s * 3.4f, 0, -3 }, 1.0f });   // sheer-legs feet
    // warm work-fires light the yard: tar cauldron, steam-box firebox, a brazier near the slip
    s_wisps.push_back(c + Vector3{ 9, 1.1f, 8 });
    s_wisps.push_back(c + Vector3{ -11, 0.9f, 9 });
    s_wisps.push_back(c + Vector3{ 0, 0.8f, 10 });
    s_wisps.push_back(c + Vector3{ -3.4f, 4.6f, -3 });   // a lantern up on the sheer-legs
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 6.0f }); }
}

static void draw_shipyard() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color yard{ 96,90,80,255 }, yardDk{ 78,72,62,255 }, cobble{ 110,108,104,255 }, oak{ 122,86,52,255 }, oakDk{ 96,66,40,255 }, oakHi{ 150,110,70,255 }, plank{ 138,104,66,255 }, iron{ 58,56,58,255 }, ironDk{ 42,40,42,255 }, rope{ 156,140,104,255 }, tar{ 30,28,30,255 };
    Color sea{ 54,78,96,235 }, seaDk{ 40,60,76,255 }, fire{ 255,120,40,255 }, ember{ 255,80,24,255 };

    // the packed-earth yard + a cobbled apron + a strip of grey harbour water at the front
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, yard);
    DrawModelEx(s_column, c + Vector3{ 0, 0.01f, 4 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2.0f, 0.5f, 12.0f }, cobble);
    DrawModelEx(s_column, c + Vector3{ 0, -0.05f, 15.5f }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.4f, 7.0f }, seaDk);   // harbour
    DrawModelEx(s_column, c + Vector3{ 0, 0.05f, 15.5f }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.3f, 7.0f }, sea);

    // ===== the launching ways: two greased timber tracks running down toward the water =====
    for (int s = -1; s <= 1; s += 2) { for (int i = 0; i < 14; i++) { float z = -12.0f + i * 1.9f; DrawModelEx(s_column, c + Vector3{ s * 0.95f, 0.18f - (z > 2 ? (z - 2) * 0.02f : 0), z }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.36f, 1.4f }, oakDk); } }
    // keel blocks under the keel
    for (int i = 0; i < 8; i++) DrawModelEx(s_column, c + Vector3{ 0, 0.3f, -12.0f + i * 2.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.6f, 0.7f }, oakDk);

    // ===== the hull in frame: keel + ribs + stem + sternpost + part-planked port side =====
    Vector3 hc = c;   // hull built about the centre, spine on x=0
    float zStern = -13.0f, zBow = 2.2f;
    // the keel (a long timber beam on the blocks) + an inner keelson
    draw_bone_seg(hc + Vector3{ 0, 0.65f, zStern }, hc + Vector3{ 0, 0.65f, zBow }, 0.34f, oakHi);
    draw_bone_seg(hc + Vector3{ 0, 1.0f, zStern + 1 }, hc + Vector3{ 0, 1.0f, zBow - 1 }, 0.18f, oak);
    const int NS = 10;
    Vector3 ribBilgeP[NS], ribTopP[NS], ribBilgeS[NS], ribTopS[NS];   // remembered outer points per station (port/starboard)
    for (int i = 0; i < NS; i++) {
        float t = (i + 0.5f) / NS;                       // 0..1 stern→bow
        float zc = zStern + 1.0f + t * (zBow - zStern - 1.6f);
        float f = 0.28f + 0.72f * sinf(t * PI);          // boat-shaped: narrow at the ends
        float beam = 2.7f * f, topbeam = 2.25f * f, ky = 0.7f;
        for (int s = -1; s <= 1; s += 2) {
            Vector3 keelPt = hc + Vector3{ 0, ky, zc };
            Vector3 bilge = hc + Vector3{ s * beam, 2.3f, zc };
            Vector3 top = hc + Vector3{ s * topbeam, 4.5f, zc };
            draw_bone_seg(keelPt, bilge, 0.16f, oak);     // floor → turn of bilge
            draw_bone_seg(bilge, top, 0.15f, oak);        // bilge → sheer (tumblehome inward)
            if (s < 0) { ribBilgeP[i] = bilge; ribTopP[i] = top; } else { ribBilgeS[i] = bilge; ribTopS[i] = top; }
        }
    }
    // a raked stem (curved prow) at the bow + a sternpost at the back + a planked transom
    draw_bone_seg(hc + Vector3{ 0, 0.65f, zBow }, hc + Vector3{ 0, 2.8f, zBow + 1.0f }, 0.28f, oakHi);
    draw_bone_seg(hc + Vector3{ 0, 2.8f, zBow + 1.0f }, hc + Vector3{ 0, 5.0f, zBow + 1.5f }, 0.24f, oakHi);
    draw_bone_seg(hc + Vector3{ 0, 0.65f, zStern }, hc + Vector3{ 0, 5.2f, zStern - 0.6f }, 0.30f, oakHi);
    for (int s = 0; s < 4; s++) DrawModelEx(s_column, hc + Vector3{ 0, 1.2f + s * 1.0f, zStern - 0.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.4f * (1.0f - s * 0.12f), 0.5f, 0.2f }, plank);   // transom boards
    // port-side planking: three runs of strakes following the rib points (the hull is half-clad)
    for (int lvl = 0; lvl < 3; lvl++) { float m = 0.34f + lvl * 0.33f;   // garboard → bilge
        for (int i = 0; i < NS - 1; i++) { Vector3 a = Vector3Lerp(hc + Vector3{ 0, 0.7f, ribBilgeP[i].z }, ribBilgeP[i], m);
            Vector3 b = Vector3Lerp(hc + Vector3{ 0, 0.7f, ribBilgeP[i + 1].z }, ribBilgeP[i + 1], m);
            draw_bone_seg(a, b, 0.2f, (lvl & 1) ? plank : oakHi); } }
    // starboard side left open, but two ribband battens tie the bare frames together
    for (int i = 0; i < NS - 1; i++) { draw_bone_seg(ribBilgeS[i], ribBilgeS[i + 1], 0.1f, oakDk); draw_bone_seg(ribTopS[i], ribTopS[i + 1], 0.1f, oakDk); }

    // ===== scaffolding along the clad port side + sheer-legs (a timber lifting tripod) =====
    draw_scaffold(c + Vector3{ -3.6f, 0, -4 }, 12.0f, oakDk, plank);
    // sheer-legs: two big raking poles meeting over the hull with a hanging block + rope
    Vector3 apex = c + Vector3{ 0, 7.6f, -3 };
    for (int s = -1; s <= 1; s += 2) draw_bone_seg(c + Vector3{ s * 3.4f, 0, -3 }, apex, 0.22f, oak);
    draw_bone_seg(c + Vector3{ 0, 0, -7 }, apex, 0.2f, oak);   // back stay leg
    draw_bone_seg(apex, apex + Vector3{ 0, -2.4f, 0.4f }, 0.08f, rope);   // fall
    DrawModelEx(s_column, apex + Vector3{ 0, -2.6f, 0.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.5f, 0.3f }, iron);   // tackle block
    draw_bone_seg(apex + Vector3{ 0, -3.0f, 0.4f }, c + Vector3{ 0, 3.0f, 0 }, 0.18f, oak);   // a frame timber being lifted

    // ===== timber stacks, a steam box, a sawpit, a tar fire, barrels, rope, an anchor =====
    draw_timberstack(c + Vector3{ -12, 0, -2 }, oak, oakHi);
    draw_timberstack(c + Vector3{ 12, 0, -3 }, oakDk, oak);
    // steam box (for bending planks): a long low box on a firebox, leaking steam
    Vector3 sb = c + Vector3{ -11, 0, 7 };
    DrawModelEx(s_column, sb + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 0.7f, 4.0f }, oak);
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, sb + Vector3{ s * 0.4f, 0.4f, 1.8f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.8f, 0.3f }, oakDk);   // legs
    DrawModelEx(s_column, sb + Vector3{ 0, 0.5f, 2.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.3f, 0.9f, 0.9f }, ironDk);   // firebox
    DrawModelEx(s_column, sb + Vector3{ 0, 0.4f, 2.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 0.5f, 0.6f }, ember);    // fire
    // sawpit: a trestle holding a squared log over a pit, with a pit-saw blade
    Vector3 sp = c + Vector3{ 11, 0, 2 };
    DrawModelEx(s_column, sp + Vector3{ 0, 0.05f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 0.4f, 1.4f }, ironDk);   // the pit (dark)
    for (int s = -1; s <= 1; s += 2) { DrawModelEx(s_column, sp + Vector3{ -1.3f, 0.8f, s * 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.4f, 0.3f }, oakDk); DrawModelEx(s_column, sp + Vector3{ 1.3f, 0.8f, s * 0.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 1.4f, 0.3f }, oakDk); }
    DrawModelEx(s_cyl, sp + Vector3{ 0, 1.7f, 0 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.45f, 3.2f, 0.45f }, oak);   // log being ripped
    DrawModelEx(s_column, sp + Vector3{ 0.4f, 1.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.04f, 1.0f, 2.0f }, iron);  // saw blade
    // tar cauldron over a fire (a warm light)
    Vector3 tc = c + Vector3{ 9, 0, 8 };
    for (int i = 0; i < 3; i++) DrawModelEx(s_cyl, tc + Vector3{ 0, 0.4f, 0 }, Vector3{ (float)((i - 1)), 0, 1 }, 30.0f, Vector3{ 0.1f, 1.0f, 0.1f }, iron);   // tripod legs
    DrawModelEx(s_dome, tc + Vector3{ 0, 1.0f, 0 }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 0.7f, 0.7f, 0.7f }, ironDk);   // cauldron bowl
    DrawModelEx(s_cyl, tc + Vector3{ 0, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.66f, 0.12f, 0.66f }, tar);          // tar surface
    DrawModelEx(s_cyl, tc + Vector3{ 0, 0.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 0.3f, 0.5f }, ember);           // fire under it
    // pitch barrels + a coil of rope + an anchor leaning on a post + a grindstone
    for (int i = 0; i < 3; i++) { Vector3 br = c + Vector3{ -8.0f + i * 1.4f, 0, 10 }; DrawModelEx(s_cyl, br + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.2f, 0.5f }, oakDk); DrawModelEx(s_cyl, br + Vector3{ 0, 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.45f, 0.08f, 0.45f }, tar); }
    for (int i = 0; i < 3; i++) DrawModelEx(s_torus, c + Vector3{ 5.5f, 0.2f + i * 0.18f, 10.5f }, Vector3{ 0,1,0 }, i * 20.0f, Vector3{ 0.6f - i * 0.06f, 0.6f - i * 0.06f, 0.6f - i * 0.06f }, rope);   // rope coil
    Vector3 ank = c + Vector3{ 4, 0, -8 };
    DrawModelEx(s_cyl, ank + Vector3{ 0, 1.4f, 0.3f }, Vector3{ 1,0,0 }, 20.0f, Vector3{ 0.12f, 2.8f, 0.12f }, iron);   // shank
    DrawModelEx(s_cyl, ank + Vector3{ 0, 2.5f, 0.7f }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 0.1f, 1.4f, 0.1f }, iron);     // stock
    for (int s = -1; s <= 1; s += 2) draw_bone_seg(ank + Vector3{ 0, 0.4f, 0.1f }, ank + Vector3{ s * 0.9f, 1.0f, 0.1f }, 0.12f, iron);   // flukes/arms
    DrawModelEx(s_cyl, c + Vector3{ -5, 0.6f, -7 }, Vector3{ 0,0,1 }, 90.0f, Vector3{ 0.8f, 0.18f, 0.8f }, cobble);   // grindstone disc
    // mooring bollards at the water's edge
    for (int s = -1; s <= 1; s += 2) { DrawModelEx(s_cyl, c + Vector3{ s * 6.0f, 0.5f, 12.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 1.0f, 0.4f }, oakDk); DrawModelEx(s_dome, c + Vector3{ s * 6.0f, 1.5f, 12.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.45f, 0.3f, 0.45f }, oak); }

    // additive: warm work-fires, sparks/steam, drifting sea mist, gulls over the harbour
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.45f + 0.16f * sinf(s_time * 3 + i), 8, 8, Color{ 255, 150, 60, 36 });
    DrawSphereEx(tc + Vector3{ 0, 0.5f, 0 }, 1.0f + 0.25f * sinf(s_time * 2.6f), 10, 10, Color{ 255, 110, 40, 50 });   // tar-fire glow
    // steam off the steam box + smoke off the tar fire
    for (int i = 0; i < 8; i++) { float fy = fmodf(s_time * 0.7f + i * 0.16f, 1.0f); DrawSphereEx(sb + Vector3{ rand_range(-0.5f,0.5f), 1.4f + fy * 3.0f, rand_range(-1.5f,1.5f) }, 0.3f + fy * 0.4f, 6, 6, Color{ 220, 220, 225, (unsigned char)(40 * (1.0f - fy)) }); }
    for (int i = 0; i < 6; i++) { float fy = fmodf(s_time * 0.9f + i * 0.2f, 1.0f); DrawSphereEx(tc + Vector3{ rand_range(-0.4f,0.4f), 1.2f + fy * 4.0f, rand_range(-0.4f,0.4f) }, 0.25f + fy * 0.5f, 5, 5, Color{ 60, 56, 56, (unsigned char)(70 * (1.0f - fy)) }); }
    // low sea mist drifting across the harbour strip
    for (int i = 0; i < 10; i++) DrawSphereEx(c + Vector3{ sinf(s_time * 0.2f + i) * 14.0f, 0.6f, 14.0f + cosf(i) * 2.0f }, rand_range(0.8f,1.6f), 6, 6, Color{ 200, 210, 220, 12 });
    // gulls wheeling over the slip
    for (int i = 0; i < 5; i++) { float t = s_time * 0.5f + i * 1.3f; DrawSphereEx(c + Vector3{ sinf(t) * 12.0f, 9.0f + cosf(t * 0.8f) * 1.5f, 6.0f + cosf(t) * 8.0f }, 0.1f, 4, 4, Color{ 210, 210, 215, 80 }); }
    EndBlendMode();
}

// ---- The Cathedral Close: the moonlit WEST FRONT of a Gothic cathedral ----
// DRY level: a twin-towered Gothic facade — two spired bell-towers flanking a gabled nave-front
// with a great glowing ROSE WINDOW over a triple portal, and rows of FLYING BUTTRESSES with
// pinnacles bracing the nave flanks. A churchyard close of cross-headstones spreads in front.
// Distinct silhouette: a Gothic EXTERIOR (spires + rose + flyers), not an interior nave / dome.
static void draw_pinnacle(Vector3 b, float h, Color stone, Color stoneDk) {
    DrawModelEx(s_column, b + Vector3{ 0, h * 0.3f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, h * 0.6f, 0.5f }, stone);     // shaft
    DrawModelEx(s_cone, b + Vector3{ 0, h * 0.85f, 0 }, Vector3{ 0,1,0 }, 45.0f, Vector3{ 0.46f, h * 0.55f, 0.46f }, stoneDk);   // spirelet
    for (int s = -1; s <= 1; s += 2) for (int z = -1; z <= 1; z += 2) DrawModelEx(s_dome, b + Vector3{ s * 0.28f, h * 0.62f, z * 0.28f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.12f, 0.18f, 0.12f }, stone);   // crockets
}
static void draw_lancet(Vector3 b, float w, float h, Color stone, Color glass) {
    // a tall pointed-arch window glowing with stained glass, facing +z
    DrawModelEx(s_column, b + Vector3{ 0, h * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ w + 0.3f, h + 0.2f, 0.25f }, stone);    // surround
    DrawModelEx(s_column, b + Vector3{ 0, h * 0.5f, 0.16f }, Vector3{ 0,1,0 }, 0, Vector3{ w, h * 0.92f, 0.16f }, glass);       // glass
    DrawModelEx(s_cone, b + Vector3{ 0, h + 0.05f, 0.16f }, Vector3{ 0,1,0 }, 0, Vector3{ w * 0.62f, 0.6f, 0.18f }, glass);     // pointed head
    DrawModelEx(s_column, b + Vector3{ 0, h * 0.5f, 0.2f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.08f, h * 0.92f, 0.16f }, stone);    // mullion
}

static void build_gothic() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    for (int s = -1; s <= 1; s += 2) obstacles.push_back({ c + Vector3{ s * 7.5f, 0, -12 }, 3.4f });   // bell towers
    obstacles.push_back({ c + Vector3{ 0, 0, -13 }, 4.5f });                                            // nave front mass
    for (int s = -1; s <= 1; s += 2) for (int j = 0; j < 3; j++) obstacles.push_back({ c + Vector3{ s * 6.4f, 0, -10.5f - j * 2.6f }, 1.1f });   // buttress piers
    for (int s = -1; s <= 1; s += 2) obstacles.push_back({ c + Vector3{ s * 10.0f, 0, 3 }, 1.2f });     // table tombs in the close
    // warm glows: rose window, three portals, two clerestory lancets, a belfry
    s_wisps.push_back(c + Vector3{ 0, 8.2f, -8.5f });
    s_wisps.push_back(c + Vector3{ 0, 2.0f, -8.5f });
    for (int s = -1; s <= 1; s += 2) s_wisps.push_back(c + Vector3{ s * 2.6f, 1.8f, -8.5f });
    for (int s = -1; s <= 1; s += 2) s_wisps.push_back(c + Vector3{ s * 5.2f, 8.5f, -12 });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 6.0f }); }
}

static void draw_gothic() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color flag{ 92,96,104,255 }, flagDk{ 74,78,86,255 }, stone{ 196,194,186,255 }, stoneDk{ 158,156,150,255 }, stoneSh{ 124,124,122,255 }, lead{ 70,72,78,255 }, slate{ 78,84,98,255 };
    Color glow{ 255,206,138,255 }, rose[4] = { { 220,90,80,255 }, { 90,130,210,255 }, { 230,200,90,255 }, { 150,90,180,255 } };

    // the flagstone churchyard close + a central paved path to the portal
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, flag);
    DrawModelEx(s_column, c + Vector3{ 0, 0.02f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 4.0f, 0.5f, boundary_radius * 2.0f }, flagDk);

    // ===== the two bell-towers flanking the front =====
    for (int s = -1; s <= 1; s += 2) {
        Vector3 tw = c + Vector3{ s * 7.5f, 0, -12 };
        DrawModelEx(s_column, tw + Vector3{ 0, 7.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 3.4f, 15.0f, 3.4f }, stone);   // shaft
        for (int b = 0; b < 4; b++) DrawModelEx(s_column, tw + Vector3{ 0, 2.5f + b * 3.4f, 1.71f }, Vector3{ 0,1,0 }, 0, Vector3{ 3.5f, 0.3f, 0.12f }, stoneDk);   // string courses
        for (int b = 0; b < 3; b++) for (int s2 = -1; s2 <= 1; s2 += 2) DrawModelEx(s_column, tw + Vector3{ s2 * 0.85f, 4.0f + b * 3.4f, 1.72f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.6f, 0.2f }, stoneSh);   // blind lancets
        DrawModelEx(s_column, tw + Vector3{ 0, 12.5f, 1.72f }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 2.6f, 0.2f }, lead);   // dark belfry opening
        // corner pinnacles + a tall octagonal spire
        for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) draw_pinnacle(tw + Vector3{ sx * 1.5f, 15.0f, sz * 1.5f }, 2.2f, stone, slate);
        DrawModelEx(s_cone, tw + Vector3{ 0, 19.5f, 0 }, Vector3{ 0,1,0 }, 22.5f, Vector3{ 2.2f, 6.0f, 2.2f }, slate);   // spire
        DrawModelEx(s_cone, tw + Vector3{ 0, 19.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.9f, 6.0f, 1.9f }, slate);
        DrawModelEx(s_dome, tw + Vector3{ 0, 22.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.6f, 0.3f }, glow);   // finial
        // a gargoyle waterspout off the front corners
        draw_bone_seg(tw + Vector3{ s * 1.6f, 11.0f, 1.5f }, tw + Vector3{ s * 2.6f, 10.6f, 2.3f }, 0.18f, stoneDk);
        DrawModelEx(s_dome, tw + Vector3{ s * 2.6f, 10.7f, 2.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.3f, 0.4f }, stoneSh);
    }

    // ===== the central nave-front: wall + gable + flèche, between the towers =====
    Vector3 nf = c + Vector3{ 0, 0, -12 };
    DrawModelEx(s_column, nf + Vector3{ 0, 5.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 9.2f, 11.0f, 3.4f }, stone);   // nave wall
    DrawModelEx(s_cone, nf + Vector3{ 0, 13.0f, 1.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 5.4f, 3.6f, 1.4f }, stoneDk);   // gable peak
    DrawModelEx(s_column, nf + Vector3{ 0, 14.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 6.0f, 0.5f }, slate);   // flèche
    DrawModelEx(s_cone, nf + Vector3{ 0, 18.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 2.4f, 0.6f }, slate);
    // the great ROSE WINDOW: a stone ring + radial tracery over a glowing stained disc
    Vector3 ro = c + Vector3{ 0, 8.2f, -10.25f };
    DrawModelEx(s_cyl, ro + Vector3{ 0, 0, 0.2f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 2.5f, 0.2f, 2.5f }, glow);    // glowing disc
    DrawModelEx(s_torus, ro + Vector3{ 0, 0, 0.35f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 2.6f, 2.6f, 2.6f }, stone);   // outer ring
    DrawModelEx(s_torus, ro + Vector3{ 0, 0, 0.36f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 1.5f, 1.5f, 1.5f }, stoneDk);   // inner ring
    for (int i = 0; i < 12; i++) { float a = i * (PI / 6.0f); DrawModelEx(s_column, ro + Vector3{ cosf(a) * 1.9f, sinf(a) * 1.9f, 0.36f }, Vector3{ 0,0,1 }, a * RAD2DEG, Vector3{ 0.12f, 2.2f, 0.12f }, stoneDk); }   // spokes
    // ===== the triple portal at the base (centre tallest), recessed pointed arches =====
    float pw[3] = { 2.2f, 1.5f, 1.5f }; float px[3] = { 0.0f, -3.3f, 3.3f }; float ph[3] = { 4.4f, 3.4f, 3.4f };
    for (int p = 0; p < 3; p++) { Vector3 d = nf + Vector3{ px[p], 0, 1.7f };
        for (int o = 0; o < 3; o++) { float ow = pw[p] + 0.5f - o * 0.22f; DrawModelEx(s_column, d + Vector3{ 0, ph[p] * 0.5f, -o * 0.18f }, Vector3{ 0,1,0 }, 0, Vector3{ ow, ph[p], 0.2f }, (o & 1) ? stoneDk : stone); DrawModelEx(s_cone, d + Vector3{ 0, ph[p] + 0.1f, -o * 0.18f }, Vector3{ 0,1,0 }, 0, Vector3{ ow * 0.7f, 1.0f, 0.2f }, (o & 1) ? stoneDk : stone); }   // archivolt orders
        DrawModelEx(s_column, d + Vector3{ 0, ph[p] * 0.5f - 0.3f, -0.7f }, Vector3{ 0,1,0 }, 0, Vector3{ pw[p], ph[p] - 0.6f, 0.2f }, glow);   // glowing doorway
        DrawModelEx(s_cone, d + Vector3{ 0, ph[p] - 0.2f, -0.7f }, Vector3{ 0,1,0 }, 0, Vector3{ pw[p] * 0.7f, 0.9f, 0.2f }, glow);
        for (int s = -1; s <= 1; s += 2) { DrawModelEx(s_cyl, d + Vector3{ s * (pw[p] * 0.5f + 0.2f), 1.3f, -0.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.16f, 2.6f, 0.16f }, stoneSh); DrawModelEx(s_dome, d + Vector3{ s * (pw[p] * 0.5f + 0.2f), 2.7f, -0.3f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 0.4f, 0.18f }, stoneSh); }   // jamb statues
    }
    DrawModelEx(s_column, nf + Vector3{ 0, 0.2f, 2.4f }, Vector3{ 0,1,0 }, 0, Vector3{ 5.0f, 0.4f, 1.6f }, stoneDk);   // portal steps
    DrawModelEx(s_column, nf + Vector3{ 0, 0.45f, 2.0f }, Vector3{ 0,1,0 }, 0, Vector3{ 4.2f, 0.4f, 1.0f }, stoneDk);

    // ===== flying buttresses bracing both nave flanks =====
    for (int s = -1; s <= 1; s += 2) for (int j = 0; j < 3; j++) {
        float zb = -10.5f - j * 2.6f;
        Vector3 pier = c + Vector3{ s * 6.4f, 0, zb };
        DrawModelEx(s_column, pier + Vector3{ 0, 3.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.3f, 6.0f, 1.3f }, stone);        // pier lower
        DrawModelEx(s_column, pier + Vector3{ 0, 6.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.0f, 1.4f, 1.0f }, stoneDk);      // set-off
        draw_pinnacle(pier + Vector3{ 0, 7.0f, 0 }, 2.6f, stone, slate);                                                    // pinnacle cap
        // the flyer: a sloping arch from the pier up to the clerestory wall
        Vector3 wall = c + Vector3{ s * 4.6f, 9.0f, zb };
        Vector3 ptop = pier + Vector3{ -s * 0.4f, 6.6f, 0 };
        draw_bone_seg(ptop, wall, 0.32f, stone);                                                                            // flyer top
        draw_bone_seg(ptop + Vector3{ 0, -1.2f, 0 }, wall + Vector3{ 0, -1.4f, 0 }, 0.16f, stoneDk);                        // flyer under-edge (open arch)
        DrawModelEx(s_column, c + Vector3{ s * 4.6f, 7.5f, zb }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 5.0f, 2.2f }, stone);  // clerestory wall buttress
    }
    // clerestory lancets glowing between the buttresses, on both nave flanks
    for (int s = -1; s <= 1; s += 2) for (int j = 0; j < 2; j++) { Vector3 lw = c + Vector3{ s * 4.95f, 8.0f, -11.8f - j * 2.6f }; DrawModelEx(s_column, lw, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 3.0f, 1.0f }, glow); }
    // low aisle roofs over the side aisles
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_cone, c + Vector3{ s * 5.4f, 5.6f, -13 }, Vector3{ 0,0,1 }, s * 60.0f, Vector3{ 1.6f, 1.4f, 8.0f }, slate);

    // ===== the churchyard close: cross-headstones, table tombs, a tall churchyard cross =====
    SetRandomSeed(7711);
    for (int i = 0; i < 14; i++) { float gx = rand_range(-15, 15), gz = rand_range(-4, 13); if (fabsf(gx) < 2.5f && gz < 6) continue;   // keep the path clear
        Vector3 g = c + Vector3{ gx, 0, gz }; float lean = rand_range(-8, 8);
        DrawModelEx(s_column, g + Vector3{ 0, 0.7f, 0 }, Vector3{ 0,0,1 }, lean, Vector3{ 0.5f, 1.4f, 0.18f }, stoneDk);   // headstone slab
        DrawModelEx(s_dome, g + Vector3{ 0, 1.4f, 0 }, Vector3{ 0,0,1 }, lean, Vector3{ 0.28f, 0.3f, 0.12f }, stoneDk); }
    for (int s = -1; s <= 1; s += 2) { Vector3 tomb = c + Vector3{ s * 10.0f, 0, 3 }; DrawModelEx(s_column, tomb + Vector3{ 0, 0.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.2f, 0.4f, 1.2f }, stone); for (int x = -1; x <= 1; x += 2) for (int z = -1; z <= 1; z += 2) DrawModelEx(s_column, tomb + Vector3{ x * 0.9f, 0.35f, z * 0.45f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.2f, 0.7f, 0.2f }, stoneSh); }   // table tombs
    Vector3 cross = c + Vector3{ -12, 0, 9 };   // a tall churchyard cross
    DrawModelEx(s_cyl, cross + Vector3{ 0, 2.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.22f, 4.0f, 0.22f }, stone);
    DrawModelEx(s_column, cross + Vector3{ 0, 3.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.6f, 0.3f, 0.3f }, stone);
    DrawModelEx(s_torus, cross + Vector3{ 0, 4.0f, 0 }, Vector3{ 0,0,1 }, 0, Vector3{ 0.7f, 0.7f, 0.7f }, stoneDk);

    // additive: rose-window stained glow + colored petals, portal/lancet glows, mist, bats
    BeginBlendMode(BLEND_ADDITIVE);
    DrawSphereEx(ro + Vector3{ 0, 0, 0.6f }, 2.6f + 0.2f * sinf(s_time * 1.6f), 12, 12, Color{ 255, 200, 120, 46 });
    for (int i = 0; i < 12; i++) { float a = i * (PI / 6.0f); Color pc = rose[i % 4]; DrawSphereEx(ro + Vector3{ cosf(a) * 1.5f, sinf(a) * 1.5f, 0.5f }, 0.4f, 6, 6, Color{ pc.r, pc.g, pc.b, 60 }); }   // petals
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.14f * sinf(s_time * 2 + i), 8, 8, Color{ 255, 200, 130, 34 });
    for (int i = 0; i < 12; i++) DrawSphereEx(c + Vector3{ sinf(s_time * 0.15f + i) * 15.0f, 0.5f, rand_range(-2, 13) }, rand_range(0.7f,1.5f), 6, 6, Color{ 150, 160, 190, 12 });   // ground mist
    for (int i = 0; i < 7; i++) { float t = s_time * 1.3f + i * 0.9f; DrawSphereEx(c + Vector3{ sinf(t) * 8.0f, 12.0f + cosf(t * 1.7f) * 3.0f, -12.0f + cosf(t) * 6.0f }, 0.12f, 4, 4, Color{ 30, 30, 40, 120 }); }   // bats round the spires
    EndBlendMode();
}

// ---- The Cliff Dwelling: an Ancestral-Puebloan cliff palace under a sandstone overhang ----
// DRY level: stacked adobe ROOM-BLOCKS in receding tiers nestle in a cliff alcove, their flat
// roofs bristling with protruding viga beams over T-shaped doorways; a square masonry TOWER and
// a round tower rise above; round subterranean KIVAS (cribbed-log roofs, ladders, hearth-glow)
// sit in the plaza. Distinct silhouette: terraced cliff masonry + kivas, not Petra/Zimbabwe/desert.
static void draw_ladder(Vector3 foot, Vector3 top, float halfw, Color pole, Color rung) {
    for (int s = -1; s <= 1; s += 2) draw_bone_seg(foot + Vector3{ s * halfw, 0, 0 }, top + Vector3{ s * halfw, 0, 0 }, 0.07f, pole);
    for (int i = 1; i < 6; i++) { float t = i / 6.0f; Vector3 p = Vector3Lerp(foot, top, t); draw_bone_seg(p + Vector3{ -halfw, 0, 0 }, p + Vector3{ halfw, 0, 0 }, 0.05f, rung); }
}
static void draw_kiva(Vector3 b, Color wall, Color beam, Color fire) {
    DrawModelEx(s_cyl, b + Vector3{ 0, 0.6f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.3f, 1.2f, 2.3f }, wall);   // sunk masonry ring
    DrawModelEx(s_cyl, b + Vector3{ 0, 1.2f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 2.0f, 0.12f, 2.0f }, beam);   // banquette
    for (int l = 0; l < 4; l++) { float y = 1.3f + l * 0.24f, rr = 1.9f - l * 0.42f, rot = l * 60.0f;   // cribbed-log roof, shrinking inward
        for (int k = 0; k < 3; k++) { rlPushMatrix(); rlTranslatef(b.x, b.y + y, b.z); rlRotatef(rot + k * 60.0f, 0, 1, 0); DrawModelEx(s_cyl, Vector3{ 0, 0, 0 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.15f, rr * 2.0f, 0.15f }, beam); rlPopMatrix(); } }
    DrawModelEx(s_column, b + Vector3{ 0, 2.18f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.1f, 0.6f }, fire);   // glowing roof hatch
}
static void draw_roomblock(Vector3 b, Vector3 size, Color wall, Color beam, Color dark, int feature) {
    DrawModelEx(s_column, b + Vector3{ 0, size.y * 0.5f, 0 }, Vector3{ 0,1,0 }, 0, size, wall);
    int nb = (int)(size.x / 0.8f); if (nb < 2) nb = 2;
    for (int i = 0; i < nb; i++) { float xx = -size.x * 0.5f + 0.5f + i * (size.x - 1.0f) / (nb - 1); DrawModelEx(s_cyl, b + Vector3{ xx, size.y - 0.2f, size.z * 0.5f + 0.3f }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.09f, 0.7f, 0.09f }, beam); }   // protruding vigas
    DrawModelEx(s_column, b + Vector3{ 0, size.y - 0.06f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ size.x + 0.1f, 0.12f, size.z + 0.1f }, beam);   // roof cap
    if (feature == 1) { DrawModelEx(s_column, b + Vector3{ 0, 0.8f, size.z * 0.5f + 0.03f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.6f, 0.2f }, dark); DrawModelEx(s_column, b + Vector3{ 0, 1.45f, size.z * 0.5f + 0.03f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.95f, 0.55f, 0.2f }, dark); }   // T-doorway
    else if (feature == 2) { DrawModelEx(s_column, b + Vector3{ 0, 0.85f, size.z * 0.5f + 0.03f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.5f, 1.6f, 0.2f }, Color{ 255,150,60,255 }); DrawModelEx(s_column, b + Vector3{ 0, 1.45f, size.z * 0.5f + 0.03f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.95f, 0.55f, 0.2f }, Color{ 255,150,60,255 }); }   // hearth-lit T-doorway
    else { DrawModelEx(s_column, b + Vector3{ 0, size.y * 0.62f, size.z * 0.5f + 0.03f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.42f, 0.42f, 0.2f }, dark); }   // small window
}

static void build_pueblo() {
    s_wisps.clear();
    Vector3 c = boundary_center;
    for (int i = 0; i < 5; i++) obstacles.push_back({ c + Vector3{ -8.0f + i * 4.0f, 0, -10 }, 2.2f });   // ground-tier room row
    obstacles.push_back({ c + Vector3{ 2, 0, -12 }, 2.2f });    // square tower
    obstacles.push_back({ c + Vector3{ -8, 0, -12 }, 1.8f });   // round tower
    for (int s = -1; s <= 1; s += 2) obstacles.push_back({ c + Vector3{ s * 5.0f, 0, -4 }, 2.4f });   // two kivas
    obstacles.push_back({ c + Vector3{ 0, 0, -5 }, 2.4f });     // central kiva
    // warm hearth/kiva fires light the alcove
    for (int s = -1; s <= 1; s += 2) s_wisps.push_back(c + Vector3{ s * 5.0f, 1.2f, -4 });
    s_wisps.push_back(c + Vector3{ 0, 1.2f, -5 });
    s_wisps.push_back(c + Vector3{ -3, 1.4f, -9.5f });
    s_wisps.push_back(c + Vector3{ 4, 3.8f, -11.0f });
    for (auto& w : s_wisps) { if ((int)s_crystal_lights.size() >= MAX_CRYSTAL_LIGHTS) break; s_crystal_lights.push_back(Vector4{ w.x, w.y, w.z, 6.0f }); }
}

static void draw_pueblo() {
    if (!s_has_column || !s_has_shapes) return;
    Vector3 c = boundary_center;
    Color tan{ 200,168,120,255 }, tanDk{ 172,140,96,255 }, cliff{ 178,122,76,255 }, cliffDk{ 122,82,54,255 }, cliffSh{ 92,62,42,255 }, adobe{ 208,166,120,255 }, adobeDk{ 170,128,88,255 }, plaster{ 222,196,158,255 }, beam{ 110,78,48,255 }, dark{ 36,28,22,255 };
    Color pot{ 156,84,54,255 }, potPaint{ 226,202,162,255 }, fire{ 255,150,60,255 }, ember{ 255,96,30,255 };

    // the sandy alcove ledge + a worn central plaza
    DrawModelEx(s_column, c + Vector3{ 0, -0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 6, 0.5f, boundary_radius * 2 + 6 }, tan);
    DrawModelEx(s_cyl, c + Vector3{ 0, 0.02f, -2 }, Vector3{ 0,1,0 }, 0, Vector3{ 11.0f, 1.0f, 11.0f }, tanDk);

    // ===== the back cliff: a great sandstone wall + an overhanging brow forming the alcove =====
    DrawModelEx(s_column, c + Vector3{ 0, 9.0f, -16 }, Vector3{ 0,1,0 }, 0, Vector3{ boundary_radius * 2 + 8, 24.0f, 6.0f }, cliff);
    for (int i = 0; i < 6; i++) DrawModelEx(s_column, c + Vector3{ rand_range(-18,18), rand_range(2,16), -13.0f }, Vector3{ 0,1,0 }, 0, Vector3{ rand_range(3,7), rand_range(2,5), 1.0f }, (i & 1) ? cliffDk : cliffSh);   // weathered strata/staining
    for (int s = -1; s <= 1; s += 2) DrawModelEx(s_column, c + Vector3{ s * 19.0f, 8.0f, -10 }, Vector3{ 0,1,0 }, 0, Vector3{ 6.0f, 22.0f, 14.0f }, cliffDk);   // side walls of the alcove
    DrawModelEx(s_column, c + Vector3{ 0, 15.5f, -10 }, Vector3{ 0,0,1 }, 0, Vector3{ boundary_radius * 2 + 8, 3.0f, 14.0f }, cliffSh);   // overhanging brow (alcove ceiling)
    DrawModelEx(s_cone, c + Vector3{ 0, 13.5f, -3.5f }, Vector3{ 1,0,0 }, 180.0f, Vector3{ 20.0f, 3.0f, 3.0f }, cliffDk);   // brow lip

    // ===== stacked adobe room-blocks in three receding tiers against the back wall =====
    int feat1[5] = { 1, 0, 2, 1, 0 };
    for (int i = 0; i < 5; i++) draw_roomblock(c + Vector3{ -8.0f + i * 4.0f, 0, -10 }, Vector3{ 3.4f, 2.8f, 2.8f }, (i & 1) ? adobe : plaster, beam, dark, feat1[i]);
    int feat2[4] = { 0, 1, 2, 0 };
    for (int i = 0; i < 4; i++) draw_roomblock(c + Vector3{ -6.0f + i * 4.0f, 2.8f, -11.6f }, Vector3{ 3.2f, 2.6f, 2.6f }, (i & 1) ? plaster : adobe, beam, dark, feat2[i]);
    for (int i = 0; i < 2; i++) draw_roomblock(c + Vector3{ -2.0f + i * 4.0f, 5.4f, -12.8f }, Vector3{ 3.0f, 2.4f, 2.4f }, (i & 1) ? adobe : plaster, beam, dark, i == 0 ? 2 : 0);

    // the famous square four-storey tower + a round tower
    for (int s2 = 0; s2 < 4; s2++) draw_roomblock(c + Vector3{ 2, 2.8f + s2 * 2.4f, -12.2f }, Vector3{ 2.4f, 2.4f, 2.4f }, (s2 & 1) ? plaster : adobe, beam, dark, s2 == 1 ? 2 : (s2 == 3 ? 3 : 0));
    DrawModelEx(s_cyl, c + Vector3{ -8, 3.0f, -12.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.9f, 6.0f, 1.9f }, adobeDk);   // round tower
    DrawModelEx(s_cyl, c + Vector3{ -8, 6.1f, -12.5f }, Vector3{ 0,1,0 }, 0, Vector3{ 1.95f, 0.3f, 1.95f }, beam);
    for (int i = 0; i < 6; i++) { float a = i * (PI / 3.0f); DrawModelEx(s_cyl, c + Vector3{ -8 + cosf(a) * 1.8f, 5.5f, -12.5f + sinf(a) * 1.8f }, Vector3{ 0,1,0 }, 0, Vector3{ 0.08f, 0.5f, 0.08f }, beam); }   // roof vigas

    // ===== round subterranean kivas in the plaza, each with a ladder + hearth glow =====
    for (int s = -1; s <= 1; s += 2) { Vector3 kv = c + Vector3{ s * 5.0f, 0, -4 }; draw_kiva(kv, adobeDk, beam, fire); draw_ladder(kv + Vector3{ 0.5f, 2.1f, 0.6f }, kv + Vector3{ 0.5f, 4.4f, -0.4f }, 0.3f, beam, beam); }
    draw_kiva(c + Vector3{ 0, 0, -5 }, adobeDk, beam, fire);
    draw_ladder(c + Vector3{ 0.5f, 2.1f, -4.4f }, c + Vector3{ 0.5f, 4.4f, -5.4f }, 0.3f, beam, beam);

    // ladders connecting the building tiers
    draw_ladder(c + Vector3{ -4, 0.2f, -8.4f }, c + Vector3{ -4, 3.0f, -9.6f }, 0.32f, beam, beam);
    draw_ladder(c + Vector3{ 4, 2.9f, -10.0f }, c + Vector3{ 4, 5.6f, -11.2f }, 0.32f, beam, beam);

    // ===== plaza life: painted pottery ollas, corn-grinding metates, water jars, drying corn =====
    Vector3 props[6] = { c + Vector3{ -3, 0, 2 }, c + Vector3{ 3, 0, 1 }, c + Vector3{ -7, 0, 4 }, c + Vector3{ 7, 0, 4 }, c + Vector3{ 0, 0, 5 }, c + Vector3{ -1, 0, -1 } };
    for (int i = 0; i < 6; i++) { Vector3 p = props[i];
        if (i % 3 == 0) { DrawModelEx(s_dome, p + Vector3{ 0, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.6f, 0.7f, 0.6f }, pot); DrawModelEx(s_cyl, p + Vector3{ 0, 0.95f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.3f, 0.2f, 0.3f }, pot); DrawModelEx(s_cyl, p + Vector3{ 0, 0.7f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.61f, 0.12f, 0.61f }, potPaint); }   // olla jar
        else if (i % 3 == 1) { DrawModelEx(s_column, p + Vector3{ 0, 0.25f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 1.2f, 0.4f, 0.7f }, tanDk); DrawModelEx(s_cyl, p + Vector3{ 0.3f, 0.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.18f, 0.2f, 0.18f }, cliffDk); }   // metate + mano
        else { DrawModelEx(s_cyl, p + Vector3{ 0, 0.4f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.8f, 0.4f }, pot); DrawModelEx(s_dome, p + Vector3{ 0, 0.8f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.4f, 0.3f, 0.4f }, pot); }   // water jar
    }
    // a couple of corn-drying racks (poles + hanging cobs)
    for (int s = -1; s <= 1; s += 2) { Vector3 rk = c + Vector3{ s * 9.0f, 0, 0 }; for (int p = 0; p < 2; p++) DrawModelEx(s_cyl, rk + Vector3{ p * 1.6f, 1.0f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.1f, 2.0f, 0.1f }, beam); DrawModelEx(s_cyl, rk + Vector3{ 0.8f, 1.9f, 0 }, Vector3{ 1,0,0 }, 90.0f, Vector3{ 0.07f, 1.8f, 0.07f }, beam); for (int k = 0; k < 4; k++) DrawModelEx(s_cyl, rk + Vector3{ k * 0.4f, 1.5f, 0 }, Vector3{ 0,1,0 }, 0, Vector3{ 0.09f, 0.5f, 0.09f }, Color{ 214,186,90,255 }); }

    // additive: kiva/hearth fire glows, rising smoke + sparks, warm dust in the sun, swallows
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 0; i < s_wisps.size(); i++) DrawSphereEx(s_wisps[i], 0.5f + 0.16f * sinf(s_time * 3 + i), 8, 8, Color{ 255, 140, 55, 40 });
    Vector3 kpos[3] = { c + Vector3{ -5,1.2f,-4 }, c + Vector3{ 5,1.2f,-4 }, c + Vector3{ 0,1.2f,-5 } };
    for (int j = 0; j < 3; j++) { for (int i = 0; i < 6; i++) { float fy = fmodf(s_time * 0.8f + i * 0.18f + j, 1.0f); DrawSphereEx(kpos[j] + Vector3{ rand_range(-0.3f,0.3f), 1.0f + fy * 4.0f, rand_range(-0.3f,0.3f) }, 0.15f + fy * 0.35f, 5, 5, Color{ 120, 96, 80, (unsigned char)(60 * (1.0f - fy)) }); } }   // kiva smoke
    for (int i = 0; i < 18; i++) DrawSphereEx(c + Vector3{ rand_range(-16,16), rand_range(1,12), rand_range(-10,8) }, 0.06f, 4, 4, Color{ 255, 220, 150, 26 });   // warm dust motes in the sun
    for (int i = 0; i < 8; i++) { float t = s_time * 1.4f + i * 0.8f; DrawSphereEx(c + Vector3{ sinf(t) * 13.0f, 8.0f + cosf(t * 1.5f) * 4.0f, -6.0f + cosf(t) * 7.0f }, 0.1f, 4, 4, Color{ 60, 50, 44, 90 }); }   // cliff swallows
    EndBlendMode();
}

void draw_water(Camera3D cam, Texture2D reflectTex, Vector2 screen) {
    if (g_level == LEVEL_DESERT || g_level == LEVEL_SANCTUM || g_level == LEVEL_BONES || g_level == LEVEL_LIB || g_level == LEVEL_CAMP || g_level == LEVEL_BRIDGE || g_level == LEVEL_MILL || g_level == LEVEL_THRONE || g_level == LEVEL_PFOREST || g_level == LEVEL_HAMLET || g_level == LEVEL_HENGE || g_level == LEVEL_SAWMILL || g_level == LEVEL_GAOL || g_level == LEVEL_TAR || g_level == LEVEL_VINE || g_level == LEVEL_QUARRY || g_level == LEVEL_DOCK || g_level == LEVEL_GLASS || g_level == LEVEL_APIARY || g_level == LEVEL_KILN || g_level == LEVEL_REEF || g_level == LEVEL_FOUNDRY || g_level == LEVEL_NAVE || g_level == LEVEL_WATERMILL || g_level == LEVEL_SALT || g_level == LEVEL_HIVE || g_level == LEVEL_BREW || g_level == LEVEL_BAMBOO || g_level == LEVEL_COLLIER || g_level == LEVEL_PLAZA || g_level == LEVEL_PUMPKIN || g_level == LEVEL_BELL || g_level == LEVEL_AVIARY || g_level == LEVEL_WEB || g_level == LEVEL_ARCHTREE || g_level == LEVEL_CHESS || g_level == LEVEL_MAZE || g_level == LEVEL_CRATER || g_level == LEVEL_TITAN || g_level == LEVEL_HOODOO || g_level == LEVEL_MOAI || g_level == LEVEL_PINES || g_level == LEVEL_GALLEON || g_level == LEVEL_SUNFLOWER || g_level == LEVEL_SPHINX || g_level == LEVEL_DAM || g_level == LEVEL_THEATRE || g_level == LEVEL_WHEEL || g_level == LEVEL_REDWOOD || g_level == LEVEL_BALLOON || g_level == LEVEL_SILO || g_level == LEVEL_ORGAN || g_level == LEVEL_HYPO || g_level == LEVEL_FORT || g_level == LEVEL_TRIUMPH || g_level == LEVEL_ORCHARD || g_level == LEVEL_LOOM || g_level == LEVEL_SAVANNA || g_level == LEVEL_MOSQUE || g_level == LEVEL_TOTEM || g_level == LEVEL_PAGODA || g_level == LEVEL_JANTAR || g_level == LEVEL_TERRACOTTA || g_level == LEVEL_BALLCOURT || g_level == LEVEL_PETRA || g_level == LEVEL_BAZAAR || g_level == LEVEL_SIEGE || g_level == LEVEL_CACTUS || g_level == LEVEL_ZIMBABWE || g_level == LEVEL_BASIL || g_level == LEVEL_PRINT || g_level == LEVEL_GER || g_level == LEVEL_ALCHEMY || g_level == LEVEL_ISHTAR || g_level == LEVEL_TANNERY || g_level == LEVEL_MUSEUM || g_level == LEVEL_GOMPA || g_level == LEVEL_BUDDHA || g_level == LEVEL_BADGIR || g_level == LEVEL_TOPIARY || g_level == LEVEL_GLASSWORKS || g_level == LEVEL_SHIPYARD || g_level == LEVEL_GOTHIC || g_level == LEVEL_PUEBLO) return;   // dry levels: their own draw_* lays a solid floor (sand / platform / ash / library / mud / bridge / grass / hall / forest / village / heath / yard / flagstones / cracked earth / terraced earth / quarry floor / dock deck / glasshouse tiles / meadow / brickyard / seabed / iron plate / cathedral flags / mill yard / salt crust / wax / brewhouse / moss / ashy clearing / cobbles / harvest field / foundry yard / cage floor / cave floor)
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
    if (s_has_shapes) { UnloadModel(s_cyl); UnloadModel(s_dome); UnloadModel(s_cone); UnloadModel(s_torus); }
    if (s_has_moon) { UnloadModel(s_moon); UnloadShader(s_moon_shader); }
    UnloadModel(s_sky);   UnloadShader(s_sky_shader);
    UnloadModel(s_water); UnloadShader(s_water_shader);
    s_crystals.clear();
    s_crystal_lights.clear();
    s_tombs.clear();
    s_trees.clear();
    s_wisps.clear();
    s_shrooms.clear();
    s_obelisks.clear();
    s_drums.clear();
    s_wrecks.clear();
    s_props.clear();
    s_isles.clear();
    s_gears.clear();
    s_torii.clear();
    s_lanterns.clear();
    s_blossoms.clear();
    s_bones.clear();
    s_ribcages.clear();
    s_skulls.clear();
    s_crys.clear();
    s_roots.clear();
    s_foliage.clear();
    s_frames.clear();
    s_carts.clear();
    s_veins.clear();
    s_scopes.clear();
    s_globes.clear();
    s_shelves.clear();
    s_lecterns.clear();
    s_tents.clear();
    s_banners.clear();
    s_arches.clear();
    s_statues.clear();
    s_bushes.clear();
    s_cypress.clear();
    s_stacks.clear();
    s_mills.clear();
    s_hay.clear();
    s_sarcos.clear();
    s_ptrees.clear();
    s_cottages.clear();
    s_sarsens.clear();
    s_willows.clear();
    s_logpiles.clear();
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
