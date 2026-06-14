// Soulslike â€” Boss Arena, ported from Godot to raylib/C++.
// Top-level loop: lighting, pause, restart, and the player-vs-boss fight.
#include "raylib.h"
#include "rlgl.h"
#include "mathx.h"
#include "game.h"
#include "anim.h"
#include "assets.h"
#include "sword.h"
#include "events.h"
#include "audio_sys.h"
#include "fx.h"
#include "juice.h"
#include "render.h"
#include "arena.h"
#include "assets.h"
#include "player.h"
#include "boss.h"
#include "mobs.h"
#include "hud.h"
#include "screens.h"
#include "pause_menu.h"
#include "save.h"
#include <vector>
#include <cstdio>

// ---- start-menu + bonfire drawing helpers (defined below main) ----
static void draw_start_menu(int sel);
static void draw_bonfire(Vector3 pos, float t);

int main(int argc, char** argv) {
    bool auto_demo = false, diag = false, scenic = false, level_arg = false;
    for (int i = 1; i < argc; i++) {
        if (TextIsEqual(argv[i], "auto")) auto_demo = true;
        if (TextIsEqual(argv[i], "diag")) diag = true;
        if (TextIsEqual(argv[i], "scenic")) scenic = true;
        if (TextIsEqual(argv[i], "bloodmoon") || TextIsEqual(argv[i], "ruin") || TextIsEqual(argv[i], "moon")) { g_level = LEVEL_BLOODMOON; level_arg = true; }
        if (TextIsEqual(argv[i], "ice") || TextIsEqual(argv[i], "frozen")) { g_level = LEVEL_FROZEN; level_arg = true; }
        if (TextIsEqual(argv[i], "forge") || TextIsEqual(argv[i], "lava")) { g_level = LEVEL_FORGE; level_arg = true; }
        if (TextIsEqual(argv[i], "colosseum") || TextIsEqual(argv[i], "pit")) { g_level = LEVEL_COLOSSEUM; level_arg = true; }
        if (TextIsEqual(argv[i], "grave") || TextIsEqual(argv[i], "graveyard")) { g_level = LEVEL_GRAVEYARD; level_arg = true; }
        if (TextIsEqual(argv[i], "fungal") || TextIsEqual(argv[i], "grotto")) { g_level = LEVEL_FUNGAL; level_arg = true; }
        if (TextIsEqual(argv[i], "desert") || TextIsEqual(argv[i], "ziggurat")) { g_level = LEVEL_DESERT; level_arg = true; }
        if (TextIsEqual(argv[i], "cove") || TextIsEqual(argv[i], "wreck") || TextIsEqual(argv[i], "shipwreck")) { g_level = LEVEL_WRECK; level_arg = true; }
        if (TextIsEqual(argv[i], "sky") || TextIsEqual(argv[i], "sanctum")) { g_level = LEVEL_SANCTUM; level_arg = true; }
        if (TextIsEqual(argv[i], "clock") || TextIsEqual(argv[i], "vault") || TextIsEqual(argv[i], "clockwork")) { g_level = LEVEL_CLOCK; level_arg = true; }
        if (TextIsEqual(argv[i], "shrine") || TextIsEqual(argv[i], "torii")) { g_level = LEVEL_SHRINE; level_arg = true; }
        if (TextIsEqual(argv[i], "bones") || TextIsEqual(argv[i], "boneyard") || TextIsEqual(argv[i], "dragon")) { g_level = LEVEL_BONES; level_arg = true; }
        if (TextIsEqual(argv[i], "crystal") || TextIsEqual(argv[i], "cavern") || TextIsEqual(argv[i], "geode")) { g_level = LEVEL_GEODE; level_arg = true; }
        if (TextIsEqual(argv[i], "temple") || TextIsEqual(argv[i], "jungle") || TextIsEqual(argv[i], "overgrown")) { g_level = LEVEL_TEMPLE; level_arg = true; }
        if (TextIsEqual(argv[i], "mine") || TextIsEqual(argv[i], "mineshaft") || TextIsEqual(argv[i], "dig")) { g_level = LEVEL_MINE; level_arg = true; }
        if (TextIsEqual(argv[i], "observatory") || TextIsEqual(argv[i], "orrery") || TextIsEqual(argv[i], "astral")) { g_level = LEVEL_OBS; level_arg = true; }
        if (TextIsEqual(argv[i], "library") || TextIsEqual(argv[i], "archive") || TextIsEqual(argv[i], "lib")) { g_level = LEVEL_LIB; level_arg = true; }
        if (TextIsEqual(argv[i], "camp") || TextIsEqual(argv[i], "siege") || TextIsEqual(argv[i], "warcamp")) { g_level = LEVEL_CAMP; level_arg = true; }
        if (TextIsEqual(argv[i], "aqueduct") || TextIsEqual(argv[i], "viaduct") || TextIsEqual(argv[i], "cistern")) { g_level = LEVEL_AQUA; level_arg = true; }
        if (TextIsEqual(argv[i], "court") || TextIsEqual(argv[i], "sentinels") || TextIsEqual(argv[i], "statues")) { g_level = LEVEL_COURT; level_arg = true; }
        if (TextIsEqual(argv[i], "garden") || TextIsEqual(argv[i], "gardens") || TextIsEqual(argv[i], "bloom")) { g_level = LEVEL_GARDEN; level_arg = true; }
        if (TextIsEqual(argv[i], "bridge") || TextIsEqual(argv[i], "chasm") || TextIsEqual(argv[i], "span")) { g_level = LEVEL_BRIDGE; level_arg = true; }
        if (TextIsEqual(argv[i], "beacon") || TextIsEqual(argv[i], "lighthouse") || TextIsEqual(argv[i], "coast")) { g_level = LEVEL_BEACON; level_arg = true; }
        if (TextIsEqual(argv[i], "mill") || TextIsEqual(argv[i], "windmill") || TextIsEqual(argv[i], "fields")) { g_level = LEVEL_MILL; level_arg = true; }
        if (TextIsEqual(argv[i], "ossuary") || TextIsEqual(argv[i], "catacombs") || TextIsEqual(argv[i], "crypt")) { g_level = LEVEL_OSSUARY; level_arg = true; }
        if (TextIsEqual(argv[i], "fair") || TextIsEqual(argv[i], "fairground") || TextIsEqual(argv[i], "carnival")) { g_level = LEVEL_FAIR; level_arg = true; }
        if (TextIsEqual(argv[i], "throne") || TextIsEqual(argv[i], "hall") || TextIsEqual(argv[i], "kings")) { g_level = LEVEL_THRONE; level_arg = true; }
        if (TextIsEqual(argv[i], "springs") || TextIsEqual(argv[i], "geyser") || TextIsEqual(argv[i], "hotsprings")) { g_level = LEVEL_SPRINGS; level_arg = true; }
        if (TextIsEqual(argv[i], "petrified") || TextIsEqual(argv[i], "stoneforest") || TextIsEqual(argv[i], "woods")) { g_level = LEVEL_PFOREST; level_arg = true; }
        if (TextIsEqual(argv[i], "hamlet") || TextIsEqual(argv[i], "village") || TextIsEqual(argv[i], "plague")) { g_level = LEVEL_HAMLET; level_arg = true; }
        if (TextIsEqual(argv[i], "henge") || TextIsEqual(argv[i], "stonehenge") || TextIsEqual(argv[i], "circle")) { g_level = LEVEL_HENGE; level_arg = true; }
        if (TextIsEqual(argv[i], "bog") || TextIsEqual(argv[i], "swamp") || TextIsEqual(argv[i], "marsh")) { g_level = LEVEL_BOG; level_arg = true; }
        if (TextIsEqual(argv[i], "sawmill") || TextIsEqual(argv[i], "lumber") || TextIsEqual(argv[i], "timberyard")) { g_level = LEVEL_SAWMILL; level_arg = true; }
        if (TextIsEqual(argv[i], "gaol") || TextIsEqual(argv[i], "prison") || TextIsEqual(argv[i], "dungeon")) { g_level = LEVEL_GAOL; level_arg = true; }
        if (TextIsEqual(argv[i], "tar") || TextIsEqual(argv[i], "tarpit") || TextIsEqual(argv[i], "pitch")) { g_level = LEVEL_TAR; level_arg = true; }
        if (TextIsEqual(argv[i], "vineyard") || TextIsEqual(argv[i], "vine") || TextIsEqual(argv[i], "winery")) { g_level = LEVEL_VINE; level_arg = true; }
        if (TextIsEqual(argv[i], "floes") || TextIsEqual(argv[i], "glacier") || TextIsEqual(argv[i], "harbor")) { g_level = LEVEL_FLOE; level_arg = true; }
        if (TextIsEqual(argv[i], "quarry") || TextIsEqual(argv[i], "excavation") || TextIsEqual(argv[i], "stonepit")) { g_level = LEVEL_QUARRY; level_arg = true; }
        if (TextIsEqual(argv[i], "skydock") || TextIsEqual(argv[i], "airship") || TextIsEqual(argv[i], "dirigible")) { g_level = LEVEL_DOCK; level_arg = true; }
        if (TextIsEqual(argv[i], "conservatory") || TextIsEqual(argv[i], "glasshouse") || TextIsEqual(argv[i], "greenhouse")) { g_level = LEVEL_GLASS; level_arg = true; }
        if (TextIsEqual(argv[i], "apiary") || TextIsEqual(argv[i], "bees") || TextIsEqual(argv[i], "beeyard")) { g_level = LEVEL_APIARY; level_arg = true; }
        if (TextIsEqual(argv[i], "kiln") || TextIsEqual(argv[i], "pottery") || TextIsEqual(argv[i], "kilnyard")) { g_level = LEVEL_KILN; level_arg = true; }
        if (TextIsEqual(argv[i], "reef") || TextIsEqual(argv[i], "coral") || TextIsEqual(argv[i], "seabed")) { g_level = LEVEL_REEF; level_arg = true; }
        if (TextIsEqual(argv[i], "foundry") || TextIsEqual(argv[i], "ironworks") || TextIsEqual(argv[i], "smithy")) { g_level = LEVEL_FOUNDRY; level_arg = true; }
        if (TextIsEqual(argv[i], "nave") || TextIsEqual(argv[i], "cathedral") || TextIsEqual(argv[i], "basilica")) { g_level = LEVEL_NAVE; level_arg = true; }
        if (TextIsEqual(argv[i], "watermill") || TextIsEqual(argv[i], "millpond") || TextIsEqual(argv[i], "waterwheel")) { g_level = LEVEL_WATERMILL; level_arg = true; }
        if (TextIsEqual(argv[i], "salt") || TextIsEqual(argv[i], "saltpan") || TextIsEqual(argv[i], "saltflats")) { g_level = LEVEL_SALT; level_arg = true; }
        if (TextIsEqual(argv[i], "stilt") || TextIsEqual(argv[i], "fishingvillage") || TextIsEqual(argv[i], "lagoon")) { g_level = LEVEL_STILT; level_arg = true; }
        if (TextIsEqual(argv[i], "hive") || TextIsEqual(argv[i], "honeycomb") || TextIsEqual(argv[i], "nest")) { g_level = LEVEL_HIVE; level_arg = true; }
        if (TextIsEqual(argv[i], "brewery") || TextIsEqual(argv[i], "distillery") || TextIsEqual(argv[i], "vats")) { g_level = LEVEL_BREW; level_arg = true; }
        if (TextIsEqual(argv[i], "bamboo") || TextIsEqual(argv[i], "grove") || TextIsEqual(argv[i], "thicket")) { g_level = LEVEL_BAMBOO; level_arg = true; }
        if (TextIsEqual(argv[i], "collier") || TextIsEqual(argv[i], "charcoal") || TextIsEqual(argv[i], "meiler")) { g_level = LEVEL_COLLIER; level_arg = true; }
        if (TextIsEqual(argv[i], "plaza") || TextIsEqual(argv[i], "square") || TextIsEqual(argv[i], "fountainsquare")) { g_level = LEVEL_PLAZA; level_arg = true; }
        if (TextIsEqual(argv[i], "pumpkin") || TextIsEqual(argv[i], "patch") || TextIsEqual(argv[i], "hallow")) { g_level = LEVEL_PUMPKIN; level_arg = true; }
        if (TextIsEqual(argv[i], "bell") || TextIsEqual(argv[i], "belfry") || TextIsEqual(argv[i], "carillon")) { g_level = LEVEL_BELL; level_arg = true; }
        if (TextIsEqual(argv[i], "aviary") || TextIsEqual(argv[i], "birdcage") || TextIsEqual(argv[i], "rookery")) { g_level = LEVEL_AVIARY; level_arg = true; }
        if (TextIsEqual(argv[i], "web") || TextIsEqual(argv[i], "lair") || TextIsEqual(argv[i], "spider")) { g_level = LEVEL_WEB; level_arg = true; }
        if (TextIsEqual(argv[i], "lotus") || TextIsEqual(argv[i], "pond") || TextIsEqual(argv[i], "pavilion")) { g_level = LEVEL_LOTUS; level_arg = true; }
        if (TextIsEqual(argv[i], "archtree") || TextIsEqual(argv[i], "greattree") || TextIsEqual(argv[i], "worldtree")) { g_level = LEVEL_ARCHTREE; level_arg = true; }
        if (TextIsEqual(argv[i], "chess") || TextIsEqual(argv[i], "gambit") || TextIsEqual(argv[i], "checkmate")) { g_level = LEVEL_CHESS; level_arg = true; }
        if (TextIsEqual(argv[i], "maze") || TextIsEqual(argv[i], "hedge") || TextIsEqual(argv[i], "labyrinth")) { g_level = LEVEL_MAZE; level_arg = true; }
        if (TextIsEqual(argv[i], "falls") || TextIsEqual(argv[i], "waterfall") || TextIsEqual(argv[i], "cascade")) { g_level = LEVEL_FALLS; level_arg = true; }
        if (TextIsEqual(argv[i], "crater") || TextIsEqual(argv[i], "meteor") || TextIsEqual(argv[i], "star")) { g_level = LEVEL_CRATER; level_arg = true; }
        if (TextIsEqual(argv[i], "titan") || TextIsEqual(argv[i], "colossus") || TextIsEqual(argv[i], "giant")) { g_level = LEVEL_TITAN; level_arg = true; }
        if (TextIsEqual(argv[i], "hoodoos") || TextIsEqual(argv[i], "canyon") || TextIsEqual(argv[i], "badlands")) { g_level = LEVEL_HOODOO; level_arg = true; }
        if (TextIsEqual(argv[i], "moai") || TextIsEqual(argv[i], "heads") || TextIsEqual(argv[i], "island")) { g_level = LEVEL_MOAI; level_arg = true; }
        if (TextIsEqual(argv[i], "cavern") || TextIsEqual(argv[i], "dripstone") || TextIsEqual(argv[i], "limestone")) { g_level = LEVEL_CAVERN; level_arg = true; }
        if (TextIsEqual(argv[i], "pines") || TextIsEqual(argv[i], "snowforest") || TextIsEqual(argv[i], "winter")) { g_level = LEVEL_PINES; level_arg = true; }
        if (TextIsEqual(argv[i], "galleon") || TextIsEqual(argv[i], "ship") || TextIsEqual(argv[i], "deck")) { g_level = LEVEL_GALLEON; level_arg = true; }
        if (TextIsEqual(argv[i], "sunflower") || TextIsEqual(argv[i], "sunflowers") || TextIsEqual(argv[i], "sunfield")) { g_level = LEVEL_SUNFLOWER; level_arg = true; }
        if (TextIsEqual(argv[i], "sphinx") || TextIsEqual(argv[i], "pyramids") || TextIsEqual(argv[i], "giza")) { g_level = LEVEL_SPHINX; level_arg = true; }
        if (TextIsEqual(argv[i], "dam") || TextIsEqual(argv[i], "reservoir") || TextIsEqual(argv[i], "spillway")) { g_level = LEVEL_DAM; level_arg = true; }
        if (TextIsEqual(argv[i], "theatre") || TextIsEqual(argv[i], "amphitheatre") || TextIsEqual(argv[i], "stage")) { g_level = LEVEL_THEATRE; level_arg = true; }
        if (TextIsEqual(argv[i], "wheel") || TextIsEqual(argv[i], "ferris") || TextIsEqual(argv[i], "fairwheel")) { g_level = LEVEL_WHEEL; level_arg = true; }
        if (TextIsEqual(argv[i], "redwood") || TextIsEqual(argv[i], "redwoods") || TextIsEqual(argv[i], "sequoia")) { g_level = LEVEL_REDWOOD; level_arg = true; }
        if (TextIsEqual(argv[i], "balloon") || TextIsEqual(argv[i], "balloons") || TextIsEqual(argv[i], "festival")) { g_level = LEVEL_BALLOON; level_arg = true; }
        if (TextIsEqual(argv[i], "canal") || TextIsEqual(argv[i], "canals") || TextIsEqual(argv[i], "venice")) { g_level = LEVEL_CANAL; level_arg = true; }
        if (TextIsEqual(argv[i], "silo") || TextIsEqual(argv[i], "silos") || TextIsEqual(argv[i], "grain")) { g_level = LEVEL_SILO; level_arg = true; }
        if (TextIsEqual(argv[i], "organ") || TextIsEqual(argv[i], "pipes") || TextIsEqual(argv[i], "organhall")) { g_level = LEVEL_ORGAN; level_arg = true; }
        if (TextIsEqual(argv[i], "hypostyle") || TextIsEqual(argv[i], "karnak") || TextIsEqual(argv[i], "pillars")) { g_level = LEVEL_HYPO; level_arg = true; }
        if (TextIsEqual(argv[i], "fort") || TextIsEqual(argv[i], "bastion") || TextIsEqual(argv[i], "fortress")) { g_level = LEVEL_FORT; level_arg = true; }
        if (TextIsEqual(argv[i], "triumph") || TextIsEqual(argv[i], "arch") || TextIsEqual(argv[i], "quadriga")) { g_level = LEVEL_TRIUMPH; level_arg = true; }
        if (TextIsEqual(argv[i], "orchard") || TextIsEqual(argv[i], "blossom") || TextIsEqual(argv[i], "cherry")) { g_level = LEVEL_ORCHARD; level_arg = true; }
        if (TextIsEqual(argv[i], "loom") || TextIsEqual(argv[i], "weaver") || TextIsEqual(argv[i], "tapestry")) { g_level = LEVEL_LOOM; level_arg = true; }
        if (TextIsEqual(argv[i], "savanna") || TextIsEqual(argv[i], "acacia") || TextIsEqual(argv[i], "baobab")) { g_level = LEVEL_SAVANNA; level_arg = true; }
        if (TextIsEqual(argv[i], "mosque") || TextIsEqual(argv[i], "minaret") || TextIsEqual(argv[i], "dome")) { g_level = LEVEL_MOSQUE; level_arg = true; }
        if (TextIsEqual(argv[i], "totem") || TextIsEqual(argv[i], "totems") || TextIsEqual(argv[i], "longhouse")) { g_level = LEVEL_TOTEM; level_arg = true; }
        if (TextIsEqual(argv[i], "oasis") || TextIsEqual(argv[i], "palms") || TextIsEqual(argv[i], "spring")) { g_level = LEVEL_OASIS; level_arg = true; }
        if (TextIsEqual(argv[i], "pagoda") || TextIsEqual(argv[i], "torii") || TextIsEqual(argv[i], "zen")) { g_level = LEVEL_PAGODA; level_arg = true; }
        if (TextIsEqual(argv[i], "stepwell") || TextIsEqual(argv[i], "baori") || TextIsEqual(argv[i], "baoli")) { g_level = LEVEL_STEPWELL; level_arg = true; }
        if (TextIsEqual(argv[i], "jantar") || TextIsEqual(argv[i], "sundial") || TextIsEqual(argv[i], "yantra")) { g_level = LEVEL_JANTAR; level_arg = true; }
        if (TextIsEqual(argv[i], "terracotta") || TextIsEqual(argv[i], "army") || TextIsEqual(argv[i], "warriors")) { g_level = LEVEL_TERRACOTTA; level_arg = true; }
        if (TextIsEqual(argv[i], "ballcourt") || TextIsEqual(argv[i], "pelota") || TextIsEqual(argv[i], "maya")) { g_level = LEVEL_BALLCOURT; level_arg = true; }
        if (TextIsEqual(argv[i], "petra") || TextIsEqual(argv[i], "treasury") || TextIsEqual(argv[i], "khazneh")) { g_level = LEVEL_PETRA; level_arg = true; }
        if (TextIsEqual(argv[i], "bazaar") || TextIsEqual(argv[i], "souk") || TextIsEqual(argv[i], "market")) { g_level = LEVEL_BAZAAR; level_arg = true; }
        if (TextIsEqual(argv[i], "siegeworks") || TextIsEqual(argv[i], "trebuchet") || TextIsEqual(argv[i], "engines")) { g_level = LEVEL_SIEGE; level_arg = true; }
        if (TextIsEqual(argv[i], "whaling") || TextIsEqual(argv[i], "tryworks") || TextIsEqual(argv[i], "station")) { g_level = LEVEL_WHALING; level_arg = true; }
        if (TextIsEqual(argv[i], "cactus") || TextIsEqual(argv[i], "saguaro") || TextIsEqual(argv[i], "sonoran")) { g_level = LEVEL_CACTUS; level_arg = true; }
        if (TextIsEqual(argv[i], "zimbabwe") || TextIsEqual(argv[i], "enclosure") || TextIsEqual(argv[i], "conical")) { g_level = LEVEL_ZIMBABWE; level_arg = true; }
        if (TextIsEqual(argv[i], "basil") || TextIsEqual(argv[i], "onion") || TextIsEqual(argv[i], "kremlin")) { g_level = LEVEL_BASIL; level_arg = true; }
        if (TextIsEqual(argv[i], "printing") || TextIsEqual(argv[i], "press") || TextIsEqual(argv[i], "printworks")) { g_level = LEVEL_PRINT; level_arg = true; }
        if (TextIsEqual(argv[i], "ger") || TextIsEqual(argv[i], "yurt") || TextIsEqual(argv[i], "steppe")) { g_level = LEVEL_GER; level_arg = true; }
        if (TextIsEqual(argv[i], "alchemy") || TextIsEqual(argv[i], "alchemist") || TextIsEqual(argv[i], "athanor")) { g_level = LEVEL_ALCHEMY; level_arg = true; }
        if (TextIsEqual(argv[i], "baths") || TextIsEqual(argv[i], "thermae") || TextIsEqual(argv[i], "bathhouse")) { g_level = LEVEL_BATHS; level_arg = true; }
        if (TextIsEqual(argv[i], "floating") || TextIsEqual(argv[i], "river") || TextIsEqual(argv[i], "sampan")) { g_level = LEVEL_FLOAT; level_arg = true; }
        if (TextIsEqual(argv[i], "ishtar") || TextIsEqual(argv[i], "gate") || TextIsEqual(argv[i], "babylon")) { g_level = LEVEL_ISHTAR; level_arg = true; }
        if (TextIsEqual(argv[i], "tannery") || TextIsEqual(argv[i], "dye") || TextIsEqual(argv[i], "fez")) { g_level = LEVEL_TANNERY; level_arg = true; }
        if (TextIsEqual(argv[i], "museum") || TextIsEqual(argv[i], "fossil") || TextIsEqual(argv[i], "natural")) { g_level = LEVEL_MUSEUM; level_arg = true; }
        if (TextIsEqual(argv[i], "monastery") || TextIsEqual(argv[i], "gompa") || TextIsEqual(argv[i], "stupa")) { g_level = LEVEL_GOMPA; level_arg = true; }
        if (TextIsEqual(argv[i], "buddha") || TextIsEqual(argv[i], "daibutsu") || TextIsEqual(argv[i], "grotto")) { g_level = LEVEL_BUDDHA; level_arg = true; }
        if (TextIsEqual(argv[i], "angkor") || TextIsEqual(argv[i], "prasat") || TextIsEqual(argv[i], "templemount")) { g_level = LEVEL_ANGKOR; level_arg = true; }
        if (TextIsEqual(argv[i], "badgir") || TextIsEqual(argv[i], "windcatcher") || TextIsEqual(argv[i], "yazd")) { g_level = LEVEL_BADGIR; level_arg = true; }
        if (TextIsEqual(argv[i], "topiary") || TextIsEqual(argv[i], "parterre") || TextIsEqual(argv[i], "clipped")) { g_level = LEVEL_TOPIARY; level_arg = true; }
        if (TextIsEqual(argv[i], "glassworks") || TextIsEqual(argv[i], "glassblower") || TextIsEqual(argv[i], "hotshop")) { g_level = LEVEL_GLASSWORKS; level_arg = true; }
        if (TextIsEqual(argv[i], "shipyard") || TextIsEqual(argv[i], "slipway") || TextIsEqual(argv[i], "drydock")) { g_level = LEVEL_SHIPYARD; level_arg = true; }
        if (TextIsEqual(argv[i], "gothic") || TextIsEqual(argv[i], "buttress") || TextIsEqual(argv[i], "rosewindow")) { g_level = LEVEL_GOTHIC; level_arg = true; }
        if (TextIsEqual(argv[i], "pueblo") || TextIsEqual(argv[i], "cliffdwelling") || TextIsEqual(argv[i], "mesa")) { g_level = LEVEL_PUEBLO; level_arg = true; }
    }

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Soulslike - Boss Arena (raylib)");
    SetTargetFPS(60);
    InitAudioDevice();
    SetExitKey(KEY_NULL);             // Esc is pause, not quit

    // ---- model diagnostic: render both characters from a clear camera ----
    if (diag) {
        g_lit.load();
        g_fx.init();
        Model pm = LoadModel(assets::path("characters/player.glb").c_str());
        Model em = LoadModel(assets::path("characters/enemy.glb").c_str());
        AnimController pac, eac;
        pac.load(&pm, assets::path("characters/player.glb").c_str()); pac.set_anim("standing_idle");
        eac.load(&em, assets::path("characters/enemy.glb").c_str()); eac.set_anim("zombie_idle");
        for (int i = 0; i < pm.materialCount; i++) {
            Color c = pm.materials[i].maps[MATERIAL_MAP_DIFFUSE].color;
            TraceLog(LOG_WARNING, "PLAYER mat %d  color=%d,%d,%d,%d  tex=%u", i, c.r, c.g, c.b, c.a,
                     pm.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture.id);
        }
        for (int i = 0; i < em.materialCount; i++) {
            Color c = em.materials[i].maps[MATERIAL_MAP_DIFFUSE].color;
            TraceLog(LOG_WARNING, "ENEMY mat %d  color=%d,%d,%d,%d  tex=%u", i, c.r, c.g, c.b, c.a,
                     em.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture.id);
        }
        Camera3D cam{}; cam.position = { 0, 1.2f, 5.0f }; cam.target = { 0, 1.0f, 0 };
        cam.up = { 0, 1, 0 }; cam.fovy = 55; cam.projection = CAMERA_PERSPECTIVE;
        int f = 0;
        while (!WindowShouldClose() && f < 240) {
            float dt = GetFrameTime();
            pac.update(dt); eac.update(dt);
            if (f % 18 == 0) g_fx.ripple({ -1.1f, 0, 0 }, 4.5f);   // ripple test at the player's feet
            if (f % 30 == 0) g_fx.hit({ 1.1f, 1.1f, 0 }, 26.0f);   // blood test on the enemy
            g_fx.update(dt);
            g_lit.set_frame(cam.position);
            bool lit = (f / 80) % 2 == 0;            // alternate lit / default shader
            Shader def{ rlGetShaderIdDefault(), rlGetShaderLocsDefault() };
            for (int i = 0; i < pm.materialCount; i++) pm.materials[i].shader = lit ? g_lit.shader : def;
            for (int i = 0; i < em.materialCount; i++) em.materials[i].shader = lit ? g_lit.shader : def;
            BeginDrawing();
            ClearBackground(Color{ 40, 30, 40, 255 });
            BeginMode3D(cam);
            DrawModelEx(pm, { -1.1f, 0, 0 }, { 0, 1, 0 }, 0, { 1, 1, 1 }, WHITE);
            draw_hand_sword(pm, pac, { -1.1f, 0, 0 }, 0.0f);
            DrawModelEx(em, { 1.1f, 0, 0 }, { 0, 1, 0 }, 0, { 1, 1, 1 }, WHITE);
            DrawGrid(10, 1.0f);
            g_fx.draw(cam);
            EndMode3D();
            DrawText(TextFormat("DIAG  player(L) enemy(R)  shader=%s  frame=%d", lit ? "lit" : "default", f), 10, 10, 20, RAYWHITE);
            EndDrawing();
            f++;
            if (f == 40)  TakeScreenshot("diag_lit.png");
            if (f == 120) TakeScreenshot("diag_default.png");
        }
        pac.unload(); eac.unload(); UnloadModel(pm); UnloadModel(em);
        CloseAudioDevice(); CloseWindow();
        return 0;
    }

    // ---- scenic: look across the water toward the moon to check reflection ----
    if (scenic) {
        g_lit.load();
        arena::load(g_lit.shader);
        RenderTexture2D rfx = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
        Camera3D cam{}; cam.position = { 0, 9.0f, -8 }; cam.target = { 0, 7.0f, -120 };
        cam.up = { 0, 1, 0 }; cam.fovy = 62; cam.projection = CAMERA_PERSPECTIVE;
        int f = 0;
        while (!WindowShouldClose() && f < 120) {
            arena::update(GetFrameTime());
            // mirror the camera across the water plane (same FOV) so the reflection
            // texture can be sampled at each water fragment's own screen position.
            Camera3D mc = cam; float wy = arena::water_level;
            mc.position.y = 2 * wy - mc.position.y; mc.target.y = 2 * wy - mc.target.y; mc.up.y = -mc.up.y;
            BeginTextureMode(rfx);
                ClearBackground(Color{ 13, 6, 13, 255 });
                g_lit.set_frame(mc.position);
                g_lit.set_clip(arena::water_level, true);
                BeginMode3D(mc);
                    arena::draw_sky(mc);
                    rlDisableBackfaceCulling();
                    arena::draw_world(mc);
                    rlEnableBackfaceCulling();
                EndMode3D();
            EndTextureMode();
            g_lit.set_clip(0.0f, false);
            Vector2 scr{ (float)GetScreenWidth(), (float)GetScreenHeight() };
            BeginDrawing();
            ClearBackground(Color{ 13, 6, 13, 255 });
            g_lit.set_frame(cam.position);
            BeginMode3D(cam);
                arena::draw_sky(cam);
                arena::draw_world(cam);
                arena::draw_water(cam, rfx.texture, scr);
            EndMode3D();
            // preview the reflection render-texture (bottom-right corner)
            int pw = GetScreenWidth() / 4, ph = GetScreenHeight() / 4;
            DrawTexturePro(rfx.texture, { 0, 0, (float)rfx.texture.width, -(float)rfx.texture.height },
                           { (float)(GetScreenWidth() - pw - 8), (float)(GetScreenHeight() - ph - 8), (float)pw, (float)ph },
                           { 0, 0 }, 0, WHITE);
            DrawText("SCENIC: water reflection (inset = reflection texture)", 10, 10, 20, RAYWHITE);
            EndDrawing();
            f++;
            if (f == 60) TakeScreenshot("scenic.png");
        }
        UnloadRenderTexture(rfx); arena::unload(); g_lit.unload();
        CloseAudioDevice(); CloseWindow();
        return 0;
    }

    // ---- per-session systems; the level itself is loaded/unloaded on demand ----
    g_fx.init();
    g_audio.init();
    g_juice.init();
    g_juice.enabled = true;

    HUD hud; hud.init();
    Screens screens; screens.init();
    save_load();

    Player player;
    Boss boss;
    PauseMenu pause; pause.player = &player;

    const Vector3 PLAYER_SPAWN{ 0, 0, 16 }, BOSS_SPAWN{ 0, 0, -10 }, BONFIRE_POS{ 0, 0, 7 };

    bool mob_level = false;   // true on LEVEL_GRAVEYARD: a Hollow horde instead of the Boss

    g_game.restart_requested.connect([&]() {
        player.pos = PLAYER_SPAWN; player.reset_state();
        if (mob_level) mobs::reset();
        else { boss.pos = BOSS_SPAWN; boss.reset_state(); }
    });

    RenderTexture2D reflect = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    std::vector<Hurtbox*> boss_targets, player_targets;
    bool level_loaded = false;

    // ---- Blood-Moon bonfire intro: a cinematic "rise from rest" before the fight ----
    enum IntroPhase { INTRO_NONE, INTRO_SIT, INTRO_STAND };
    int   intro = INTRO_NONE;
    float intro_t = 0.0f;
    const Vector3 INTRO_BONFIRE{ 0.0f, 0.0f, 14.4f };   // ~1.6m in front of the player spawn
    Vector3 stand_cam0{}, stand_tgt0{};                 // camera pose captured when the player rises

    auto load_level = [&](int lvl) {
        g_level = lvl;
        mob_level = (lvl == LEVEL_GRAVEYARD || lvl == LEVEL_FUNGAL || lvl == LEVEL_DESERT || lvl == LEVEL_WRECK || lvl == LEVEL_SANCTUM || lvl == LEVEL_CLOCK || lvl == LEVEL_SHRINE || lvl == LEVEL_BONES || lvl == LEVEL_GEODE || lvl == LEVEL_TEMPLE || lvl == LEVEL_MINE || lvl == LEVEL_OBS || lvl == LEVEL_LIB || lvl == LEVEL_CAMP || lvl == LEVEL_AQUA || lvl == LEVEL_COURT || lvl == LEVEL_GARDEN || lvl == LEVEL_BRIDGE || lvl == LEVEL_BEACON || lvl == LEVEL_MILL || lvl == LEVEL_OSSUARY || lvl == LEVEL_FAIR || lvl == LEVEL_THRONE || lvl == LEVEL_SPRINGS || lvl == LEVEL_PFOREST || lvl == LEVEL_HAMLET || lvl == LEVEL_HENGE || lvl == LEVEL_BOG || lvl == LEVEL_SAWMILL || lvl == LEVEL_GAOL || lvl == LEVEL_TAR || lvl == LEVEL_VINE || lvl == LEVEL_FLOE || lvl == LEVEL_QUARRY || lvl == LEVEL_DOCK || lvl == LEVEL_GLASS || lvl == LEVEL_APIARY || lvl == LEVEL_KILN || lvl == LEVEL_REEF || lvl == LEVEL_FOUNDRY || lvl == LEVEL_NAVE || lvl == LEVEL_WATERMILL || lvl == LEVEL_SALT || lvl == LEVEL_STILT || lvl == LEVEL_HIVE || lvl == LEVEL_BREW || lvl == LEVEL_BAMBOO || lvl == LEVEL_COLLIER || lvl == LEVEL_PLAZA || lvl == LEVEL_PUMPKIN || lvl == LEVEL_BELL || lvl == LEVEL_AVIARY || lvl == LEVEL_WEB || lvl == LEVEL_LOTUS || lvl == LEVEL_ARCHTREE || lvl == LEVEL_CHESS || lvl == LEVEL_MAZE || lvl == LEVEL_FALLS || lvl == LEVEL_CRATER || lvl == LEVEL_TITAN || lvl == LEVEL_HOODOO || lvl == LEVEL_MOAI || lvl == LEVEL_CAVERN || lvl == LEVEL_PINES || lvl == LEVEL_GALLEON || lvl == LEVEL_SUNFLOWER || lvl == LEVEL_SPHINX || lvl == LEVEL_DAM || lvl == LEVEL_THEATRE || lvl == LEVEL_WHEEL || lvl == LEVEL_REDWOOD || lvl == LEVEL_BALLOON || lvl == LEVEL_CANAL || lvl == LEVEL_SILO || lvl == LEVEL_ORGAN || lvl == LEVEL_HYPO || lvl == LEVEL_FORT || lvl == LEVEL_TRIUMPH || lvl == LEVEL_ORCHARD || lvl == LEVEL_LOOM || lvl == LEVEL_SAVANNA || lvl == LEVEL_MOSQUE || lvl == LEVEL_TOTEM || lvl == LEVEL_OASIS || lvl == LEVEL_PAGODA || lvl == LEVEL_STEPWELL || lvl == LEVEL_JANTAR || lvl == LEVEL_TERRACOTTA || lvl == LEVEL_BALLCOURT || lvl == LEVEL_PETRA || lvl == LEVEL_BAZAAR || lvl == LEVEL_SIEGE || lvl == LEVEL_WHALING || lvl == LEVEL_CACTUS || lvl == LEVEL_ZIMBABWE || lvl == LEVEL_BASIL || lvl == LEVEL_PRINT || lvl == LEVEL_GER || lvl == LEVEL_ALCHEMY || lvl == LEVEL_BATHS || lvl == LEVEL_FLOAT || lvl == LEVEL_ISHTAR || lvl == LEVEL_TANNERY || lvl == LEVEL_MUSEUM || lvl == LEVEL_GOMPA || lvl == LEVEL_BUDDHA || lvl == LEVEL_ANGKOR || lvl == LEVEL_BADGIR || lvl == LEVEL_TOPIARY || lvl == LEVEL_GLASSWORKS || lvl == LEVEL_SHIPYARD || lvl == LEVEL_GOTHIC || lvl == LEVEL_PUEBLO);
        g_lit.load();
        arena::load(g_lit.shader);
        player.init(PLAYER_SPAWN); player.apply_shader(g_lit.shader);
        if (mob_level) {
            mobs::spawn(g_lit.shader);
            boss_targets = mobs::targets();        // the player strikes the Hollows
        } else {
            boss.init(BOSS_SPAWN); boss.apply_shader(g_lit.shader);
            boss_targets = { &boss.hurtbox };
        }
        player_targets = { &player.hurtbox };
        player.autopilot = auto_demo;
        g_game.set_state(Game::TITLE);
        level_loaded = true;

        // Blood-Moon opens resting at a bonfire; other levels start straight in.
        intro = (lvl == LEVEL_BLOODMOON && !auto_demo) ? INTRO_SIT : INTRO_NONE;
        intro_t = 0.0f;
        if (intro == INTRO_SIT) player.play_clip("sitting_idle");   // hold the seated pose
    };
    auto unload_level = [&]() {
        if (!level_loaded) return;
        player.unload();
        if (mob_level) mobs::unload(); else boss.unload();
        arena::unload(); g_lit.unload();
        level_loaded = false;
    };

    enum AppState { APP_MENU, APP_PLAY };
    int app = APP_MENU;
    int menu_sel = g_save.last_level;
    bool paused = false, bonfire_lit = false;
    long frame = 0;

    // CLI shortcut: `auto` or an explicit level arg skip the menu and play directly
    if (auto_demo || level_arg) { load_level(g_level); app = APP_PLAY; DisableCursor(); }

    while (!WindowShouldClose()) {
        float real_dt = GetFrameTime();
        if (real_dt > 0.05f) real_dt = 0.05f;
        g_audio.update(real_dt);

        // -------------------------------------------------------- START MENU
        if (app == APP_MENU) {
            if (IsCursorHidden()) EnableCursor();
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menu_sel = (menu_sel + 1) % NUM_LEVELS;
            if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) menu_sel = (menu_sel + NUM_LEVELS - 1) % NUM_LEVELS;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_E)) {
                load_level(menu_sel); app = APP_PLAY; bonfire_lit = false; paused = false; DisableCursor();
            }
            BeginDrawing();
            ClearBackground(Color{ 8, 6, 10, 255 });
            draw_start_menu(menu_sel);
            if (IsKeyPressed(KEY_F2)) TakeScreenshot("shot.png");
            EndDrawing();
            continue;
        }

        // -------------------------------------------------------- PLAY
        g_juice.update(real_dt);

        if (IsKeyPressed(KEY_ESCAPE) && intro == INTRO_NONE) {
            if (!(g_game.state == Game::DEAD || g_game.state == Game::VICTORY)) {
                paused = !paused;
                g_game.paused = paused;
                if (paused) { EnableCursor(); pause.open(); }
                else DisableCursor();
            }
        }

        bool near_bonfire = false;
        if (intro != INTRO_NONE) {
            // -------------------------------------------------- BONFIRE INTRO
            intro_t += real_dt;
            g_audio.fire_on();
            player.anim_tick(real_dt);   // advance the seated / rising clip only
            if (intro == INTRO_SIT) {
                // ease into an over-the-shoulder two-shot of the player + fire
                Vector3 cp{ 1.5f, 1.2f, 17.7f };    // camera: behind-right, slightly raised
                Vector3 ct{ 0.0f, 0.7f, 15.0f };    // look past the player toward the bonfire
                float k = Clamp(2.5f * real_dt, 0.0f, 1.0f);
                player.camera.position = Vector3Lerp(player.camera.position, cp, k);
                player.camera.target   = Vector3Lerp(player.camera.target, ct, k);
                bool pressed = GetKeyPressed() != 0 ||
                               IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
                               IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
                if (intro_t > 0.6f && pressed) {     // player chooses to rise
                    intro = INTRO_STAND; intro_t = 0.0f;
                    stand_cam0 = player.camera.position;
                    stand_tgt0 = player.camera.target;
                    player.play_clip_once("crouch_to_standing_idle");
                }
            } else {   // INTRO_STAND: rise and pan out to the normal third-person rig
                Vector3 pivot = player.pos + Vector3{ 0, 1.5f, 0 };
                Vector3 gp = pivot + Vector3{ 0, 0, 1 } * (2.0f * cosf(0.2f)) +
                             Vector3{ 0, 2.0f * sinf(0.2f), 0 };
                float k = Clamp(intro_t / 1.2f, 0.0f, 1.0f);
                float s = k * k * (3.0f - 2.0f * k);   // smoothstep ease-out
                player.camera.position = Vector3Lerp(stand_cam0, gp, s);
                player.camera.target   = Vector3Lerp(stand_tgt0, pivot, s);
                if (k >= 1.0f) {                       // hand off to normal play
                    intro = INTRO_NONE;
                    g_audio.fire_off();
                    player.play_clip("standing_idle");
                }
            }
        } else if (!paused) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsCursorHidden()) DisableCursor();

            player.handle_input();
            float dt = real_dt * g_juice.time_scale();
            player.update(dt, boss_targets);
            if (mob_level) {
                // the horde drives its own AI + player/mob separation + lock-on + victory
                mobs::update(dt, player_targets, player.pos, 0.4f);
            } else {
                boss.update(dt, player_targets);
                // body-vs-body separation (player can't overlap the boss)
                Vector3 d = player.pos - boss.pos; d.y = 0;
                float dist = Vector3Length(d);
                float mind = 0.4f + 0.45f;
                if (dist < mind && dist > 0.0001f) {
                    Vector3 push = Vector3Scale(Vector3Normalize(d), (mind - dist) * 0.5f);
                    player.pos = player.pos + push;
                    boss.pos = boss.pos - push;
                    arena::resolve(player.pos, 0.4f);
                    arena::resolve(boss.pos, 0.45f);
                }
            }

            arena::update(dt);
            g_fx.update(dt);
            hud.update(real_dt);

            if (g_game.state == Game::DEAD) {                 // die -> retry the level
                if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
                    g_game.request_restart();
            } else if (g_game.state == Game::VICTORY) {        // win -> a bonfire ignites
                bonfire_lit = true;
                Vector3 bd = player.pos - BONFIRE_POS; bd.y = 0;
                near_bonfire = Vector3Length(bd) < 2.6f;
                if (near_bonfire && (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))) {
                    g_save.beaten[g_level] = true;             // rest = save progress, return to menu
                    g_save.last_level = g_level;
                    save_write();
                    unload_level();
                    menu_sel = g_level; app = APP_MENU; bonfire_lit = false;
                }
            }
        }
        if (app == APP_MENU) continue;   // rested at the bonfire -> show the menu next frame

        // --- planar reflection pass: render scenery from a water-mirrored camera ---
        if (reflect.texture.width != GetScreenWidth() || reflect.texture.height != GetScreenHeight()) {
            UnloadRenderTexture(reflect);
            reflect = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
        }
        // mirror the camera across the water plane (same FOV) so the reflection
        // texture can be sampled at each water fragment's own screen position.
        Camera3D mcam = player.camera;
        float wy = arena::water_level;
        mcam.position.y = 2.0f * wy - mcam.position.y;
        mcam.target.y = 2.0f * wy - mcam.target.y;
        mcam.up.y = -mcam.up.y;
        BeginTextureMode(reflect);
            ClearBackground(Color{ 13, 6, 13, 255 });
            g_lit.set_frame(mcam.position);
            g_lit.set_clip(arena::water_level, true);   // only above-water geometry reflects
            BeginMode3D(mcam);
                arena::draw_sky(mcam);
                rlDisableBackfaceCulling();   // mirror flips winding -> draw reflected geo unculled
                arena::draw_world(mcam);
                player.draw();                // reflect the fighters in the water too
                if (mob_level) mobs::draw(); else boss.draw();
                g_fx.draw(mcam);              // and the blood/spark VFX
                rlEnableBackfaceCulling();
            EndMode3D();
        EndTextureMode();
        g_lit.set_clip(0.0f, false);          // main pass: no clip

        Vector2 screen{ (float)GetScreenWidth(), (float)GetScreenHeight() };
        BeginDrawing();
        ClearBackground(Color{ 13, 6, 13, 255 });
        g_lit.set_frame(player.camera.position);
        BeginMode3D(player.camera);
            arena::draw_sky(player.camera);
            arena::draw_world(player.camera);
            player.draw();
            if (mob_level) mobs::draw(); else boss.draw();
            arena::draw_water(player.camera, reflect.texture, screen);
            g_fx.draw(player.camera);
            if (bonfire_lit) draw_bonfire(BONFIRE_POS, (float)GetTime());
            if (intro != INTRO_NONE) draw_bonfire(INTRO_BONFIRE, (float)GetTime());
            // --- combat markers: drawn LAST (on top of water/FX) so their depth-test toggle
            //     can't disturb the water blending over the sword/reflection ---
            // deathblow marker: a pulsing red dot (white core) on a posture-broken boss.
            if (!mob_level && boss.executable()) {
                Vector3 dp = boss.pos + Vector3{ 0, 1.3f, 0 };
                float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 7.0f);
                rlDisableDepthTest();
                BeginBlendMode(BLEND_ADDITIVE);
                DrawSphere(dp, 0.22f + 0.13f * pulse, Color{ 255, 55, 40, (unsigned char)(170 * pulse) });
                EndBlendMode();
                DrawSphere(dp, 0.16f, Color{ 225, 0, 0, 255 });
                DrawSphere(dp, 0.075f, Color{ 255, 248, 240, 255 });
                rlEnableDepthTest();
            }
            // lock-on marker: a small white dot on the locked target's body (not a center reticle).
            if (Actor* lt = player.lock_on_target()) {
                Vector3 lp = lt->position() + Vector3{ 0, 1.1f, 0 };
                rlDisableDepthTest();
                BeginBlendMode(BLEND_ADDITIVE);
                DrawSphere(lp, 0.045f, Color{ 190, 210, 255, 90 });
                EndBlendMode();
                DrawSphere(lp, 0.025f, Color{ 255, 255, 255, 240 });
                rlEnableDepthTest();
            }
        EndMode3D();

        if (intro == INTRO_NONE) { hud.draw(); screens.draw(); }
        if (intro == INTRO_SIT && !paused) {                 // cinematic rise prompt
            int sw = GetScreenWidth(), sh = GetScreenHeight();
            const char* msg = "Press any key to rise";
            DrawText(msg, sw / 2 - MeasureText(msg, 24) / 2, sh - 90, 24, Color{ 255, 200, 120, 230 });
        }
        if (bonfire_lit && !paused) {                        // bonfire rest hint
            int sw = GetScreenWidth(), sh = GetScreenHeight();
            const char* msg = near_bonfire ? "Press  E  to rest at the bonfire"
                                           : "Walk to the bonfire to rest";
            Color mc = near_bonfire ? Color{ 255, 200, 110, 255 } : Color{ 200, 170, 120, 200 };
            DrawText(msg, sw / 2 - MeasureText(msg, 22) / 2, sh - 96, 22, mc);
        }
        if (paused) {
            pause.draw_and_update();
            if (pause.resume_requested) { paused = false; g_game.paused = false; DisableCursor(); }
            if (pause.quit_requested) {                      // quit to the start menu
                unload_level(); menu_sel = g_level; app = APP_MENU;
                paused = false; g_game.paused = false; bonfire_lit = false;
            }
        }
        if (IsKeyPressed(KEY_F2)) TakeScreenshot("shot.png");
        EndDrawing();
        if (app == APP_MENU) continue;   // quit-to-menu from pause

        frame++;
        if (auto_demo) {
            player.hurtbox.invulnerable = true;   // god-mode so the demo reaches phase-2 + victory
            static int p2_shot = 0, vic_shot = 0;
            if (frame % 120 == 0)
                TraceLog(LOG_INFO, "DBG f=%ld player_hp=%.0f boss_hp=%.0f boss_phase=%d boss_state=%d",
                         frame, player.health.current, boss.health.current, boss.phase, boss.state);
            if (frame == 90)  TakeScreenshot("demo_title.png");
            if (frame == 600) TakeScreenshot("demo_fight.png");
            if (boss.phase == 2 && !p2_shot) { TakeScreenshot("demo_phase2.png"); p2_shot = 1; }
            if (g_game.state == Game::VICTORY && !vic_shot) { TakeScreenshot("demo_victory.png"); vic_shot = 1; }
            if (vic_shot && frame % 120 == 0) break;   // a moment after victory
            if (frame >= 4000) break;
        }
    }

    unload_level();
    UnloadRenderTexture(reflect);
    assets::unload_all();
    g_audio.shutdown();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

// ---------------------------------------------------------------- menu + bonfire
static void draw_start_menu(int sel) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    const char* title = "DARK SOULS";
    DrawText(title, sw / 2 - MeasureText(title, 70) / 2, sh / 6, 70, Color{ 196, 38, 30, 255 });
    const char* sub = "raylib port  -  select your world";
    DrawText(sub, sw / 2 - MeasureText(sub, 20) / 2, sh / 6 + 84, 20, Color{ 150, 140, 132, 255 });

    int y = sh / 2 - 40;
    for (int i = 0; i < NUM_LEVELS; i++) {
        bool on = (i == sel);
        Color c = on ? Color{ 255, 222, 130, 255 } : Color{ 150, 142, 132, 255 };
        int fs = 32, x = sw / 2 - 230;
        if (on) DrawText(">", x - 34, y, fs, c);
        DrawText(level_name(i), x, y, fs, c);
        if (g_save.beaten[i])
            DrawText("- cleared", x + 300, y + 6, 22, Color{ 120, 190, 120, 255 });
        y += 56;
    }
    const char* hint = "W / S  or  Up / Down to choose      Enter  to enter";
    DrawText(hint, sw / 2 - MeasureText(hint, 18) / 2, sh - 70, 18, Color{ 120, 115, 110, 255 });
    const char* hint2 = "Beat the boss, then rest at the bonfire to save.";
    DrawText(hint2, sw / 2 - MeasureText(hint2, 16) / 2, sh - 44, 16, Color{ 95, 90, 86, 255 });
}

static void draw_bonfire(Vector3 pos, float t) {
    // ash mound + a coiled sword stuck in it
    DrawCylinderEx(pos, pos + Vector3{ 0, 0.16f, 0 }, 0.95f, 1.05f, 12, Color{ 32, 27, 25, 255 });
    DrawCylinderEx(pos + Vector3{ 0, 0.12f, 0 }, pos + Vector3{ 0, 0.45f, 0 }, 0.72f, 0.42f, 10, Color{ 22, 17, 15, 255 });
    DrawCylinderEx(pos + Vector3{ 0, 0.30f, 0 }, pos + Vector3{ 0.05f, 1.75f, 0 }, 0.045f, 0.018f, 6, Color{ 120, 122, 132, 255 });
    DrawCube(pos + Vector3{ 0.02f, 0.62f, 0 }, 0.24f, 0.05f, 0.05f, Color{ 96, 98, 108, 255 });   // guard
    // flames (additive, flickering)
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 7; i++) {
        float a = i * 0.92f;
        float fl = 0.5f + 0.5f * sinf(t * 7.0f + a * 2.3f);
        Vector3 fp = pos + Vector3{ 0.20f * sinf(a + t * 1.4f), 0.42f + (0.35f + 0.55f * fl) * 0.5f, 0.20f * cosf(a * 1.3f + t) };
        DrawSphereEx(fp, 0.17f + 0.11f * fl, 6, 6, Color{ 255, (unsigned char)(135 + 90 * fl), 28, 120 });
    }
    DrawSphereEx(pos + Vector3{ 0, 0.7f, 0 }, 1.15f, 8, 8, Color{ 255, 120, 30, 42 });   // glow
    EndBlendMode();
}
