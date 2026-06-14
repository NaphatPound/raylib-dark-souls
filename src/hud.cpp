#include "hud.h"
#include "events.h"
#include "game.h"
#include "raylib.h"
#include <cmath>

void HUD::init() {
    g_events.player_health_changed.connect([this](float c, float m) { hp = m > 0 ? c / m : 0; });
    g_events.player_stamina_changed.connect([this](float c, float m) { sp = m > 0 ? c / m : 0; });
    g_events.boss_health_changed.connect([this](float c, float m) { boss_hp = m > 0 ? c / m : 0; });
    g_events.boss_phase_changed.connect([this](int p) { boss_phase = p; });
    g_events.boss_aggro.connect([this]() { boss_visible = true; });
    g_events.lock_on_changed.connect([this](Actor* a) { reticle = (a != nullptr); });
    g_events.player_flask_changed.connect([this](int c, int m) { flask = c; flask_max = m; });
    g_events.boss_posture_changed.connect([this](float c, float m) { boss_posture = m > 0 ? c / m : 0; });
    g_game.state_changed.connect([this](int s) {
        if (s == Game::TITLE) { boss_visible = false; reticle = false; boss_posture = 0.0f; }
    });
}

void HUD::update(float dt) { t += dt; }

static void bar(int x, int y, int w, int h, float frac, Color fill) {
    DrawRectangle(x - 2, y - 2, w + 4, h + 4, Color{ 0, 0, 0, 140 });
    DrawRectangle(x, y, (int)(w * (frac < 0 ? 0 : frac > 1 ? 1 : frac)), h, fill);
}

void HUD::draw() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    // player HP + stamina
    bar(24, 24, 356, 18, hp, Color{ 178, 30, 30, 255 });
    bar(24, 52, 356, 10, sp, Color{ 114, 191, 64, 255 });
    // estus pips
    for (int i = 0; i < flask_max; i++) {
        int px = 24 + i * 23, py = 72;
        Color c = (i < flask) ? Color{ 242, 166, 38, 255 } : Color{ 46, 36, 13, 180 };
        DrawRectangle(px, py, 18, 18, c);
    }
    // boss HP bar (bottom centre)
    if (boss_visible) {
        int bw = 760, bx = sw / 2 - bw / 2, by = sh - 92;
        const char* name = (g_level == LEVEL_GRAVEYARD) ? "The Hollow Horde"
                         : (g_level == LEVEL_FUNGAL) ? "The Sporeborn"
                         : (g_level == LEVEL_DESERT) ? "The Buried Dead"
                         : (g_level == LEVEL_WRECK) ? "The Drowned"
                         : (g_level == LEVEL_SANCTUM) ? "The Forsaken"
                         : (g_level == LEVEL_CLOCK) ? "The Rusted"
                         : (g_level == LEVEL_SHRINE) ? "The Lost"
                         : (g_level == LEVEL_BONES) ? "The Carrion"
                         : (g_level == LEVEL_GEODE) ? "The Petrified"
                         : (g_level == LEVEL_TEMPLE) ? "The Rooted"
                         : (g_level == LEVEL_MINE) ? "The Delvers"
                         : (g_level == LEVEL_OBS) ? "The Watchers"
                         : (g_level == LEVEL_LIB) ? "The Scholars"
                         : (g_level == LEVEL_CAMP) ? "The Routed"
                         : (g_level == LEVEL_AQUA) ? "The Wretched"
                         : (g_level == LEVEL_COURT) ? "The Faithless"
                         : (g_level == LEVEL_GARDEN) ? "The Withered"
                         : (g_level == LEVEL_BRIDGE) ? "The Stranded"
                         : (g_level == LEVEL_BEACON) ? "The Marooned"
                         : (g_level == LEVEL_MILL) ? "The Reaped"
                         : (g_level == LEVEL_OSSUARY) ? "The Interred"
                         : (g_level == LEVEL_FAIR) ? "The Revelers"
                         : (g_level == LEVEL_THRONE) ? "The Pretenders"
                         : (g_level == LEVEL_SPRINGS) ? "The Scalded"
                         : (g_level == LEVEL_PFOREST) ? "The Calcified"
                         : (g_level == LEVEL_HAMLET) ? "The Afflicted"
                         : (g_level == LEVEL_HENGE) ? "The Devout"
                         : (g_level == LEVEL_BOG) ? "The Mired"
                         : (g_level == LEVEL_SAWMILL) ? "The Splintered"
                         : (g_level == LEVEL_GAOL) ? "The Condemned"
                         : (g_level == LEVEL_TAR) ? "The Engulfed"
                         : (g_level == LEVEL_VINE) ? "The Trodden"
                         : (g_level == LEVEL_FLOE) ? "The Frostbitten"
                         : (g_level == LEVEL_QUARRY) ? "The Hewn"
                         : (g_level == LEVEL_DOCK) ? "The Becalmed"
                         : (g_level == LEVEL_GLASS) ? "The Overgrown"
                         : (g_level == LEVEL_APIARY) ? "The Swarmed"
                         : (g_level == LEVEL_KILN) ? "The Fired"
                         : (g_level == LEVEL_REEF) ? "The Fathomless"
                         : (g_level == LEVEL_FOUNDRY) ? "The Smelted"
                         : (g_level == LEVEL_NAVE) ? "The Penitent"
                         : (g_level == LEVEL_WATERMILL) ? "The Sodden"
                         : (g_level == LEVEL_SALT) ? "The Encrusted"
                         : (g_level == LEVEL_STILT) ? "The Brackish"
                         : (g_level == LEVEL_HIVE) ? "The Hivebound"
                         : (g_level == LEVEL_BREW) ? "The Soused"
                         : (g_level == LEVEL_BAMBOO) ? "The Severed"
                         : (g_level == LEVEL_COLLIER) ? "The Charred"
                         : (g_level == LEVEL_PLAZA) ? "The Bygone"
                         : (g_level == LEVEL_PUMPKIN) ? "The Harvested"
                         : (g_level == LEVEL_BELL) ? "The Tolled"
                         : (g_level == LEVEL_AVIARY) ? "The Plumed"
                         : (g_level == LEVEL_WEB) ? "The Ensnared"
                         : (g_level == LEVEL_LOTUS) ? "The Stilled"
                         : (g_level == LEVEL_ARCHTREE) ? "The Grafted"
                         : (g_level == LEVEL_CHESS) ? "The Sacrificed"
                         : (g_level == LEVEL_MAZE) ? "The Wayward"
                         : (g_level == LEVEL_FALLS) ? "The Drenched"
                         : (g_level == LEVEL_CRATER) ? "The Stricken"
                         : (g_level == LEVEL_TITAN) ? "The Forgotten"
                         : (g_level == LEVEL_HOODOO) ? "The Eroded"
                         : (g_level == LEVEL_MOAI) ? "The Vigil"
                         : (g_level == LEVEL_CAVERN) ? "The Sunken"
                         : (g_level == LEVEL_PINES) ? "The Snowbound"
                         : (g_level == LEVEL_GALLEON) ? "The Mutinous"
                         : (g_level == LEVEL_SUNFLOWER) ? "The Wilted"
                         : (g_level == LEVEL_SPHINX) ? "The Embalmed"
                         : (g_level == LEVEL_DAM) ? "The Submerged"
                         : (g_level == LEVEL_THEATRE) ? "The Chorus"
                         : (g_level == LEVEL_WHEEL) ? "The Suspended"
                         : (g_level == LEVEL_REDWOOD) ? "The Mossgrown"
                         : (g_level == LEVEL_BALLOON) ? "The Untethered"
                         : (g_level == LEVEL_CANAL) ? "The Adrift"
                         : (g_level == LEVEL_SILO) ? "The Husked"
                         : (g_level == LEVEL_ORGAN) ? "The Discordant"
                         : (g_level == LEVEL_HYPO) ? "The Engraved"
                         : (g_level == LEVEL_FORT) ? "The Garrison"
                         : (g_level == LEVEL_TRIUMPH) ? "The Vanquished"
                         : (g_level == LEVEL_ORCHARD) ? "The Fallow"
                         : (g_level == LEVEL_LOOM) ? "The Unraveled"
                         : (g_level == LEVEL_SAVANNA) ? "The Parched"
                         : (g_level == LEVEL_MOSQUE) ? "The Prostrate"
                         : (g_level == LEVEL_TOTEM) ? "The Ancestral"
                         : (g_level == LEVEL_OASIS) ? "The Thirsting"
                         : (g_level == LEVEL_PAGODA) ? "The Penitent"
                         : (g_level == LEVEL_STEPWELL) ? "The Drowned"
                         : (g_level == LEVEL_JANTAR) ? "The Timeless"
                         : (g_level == LEVEL_TERRACOTTA) ? "The Buried"
                         : (g_level == LEVEL_BALLCOURT) ? "The Sacrificed"
                         : (g_level == LEVEL_PETRA) ? "The Entombed"
                         : (g_level == LEVEL_BAZAAR) ? "The Covetous"
                         : (g_level == LEVEL_SIEGE) ? "The Routed"
                         : (g_level == LEVEL_WHALING) ? "The Flensed"
                         : (g_level == LEVEL_CACTUS) ? "The Withered"
                         : (g_level == LEVEL_ZIMBABWE) ? "The Dethroned"
                         : (g_level == LEVEL_BASIL) ? "The Devout"
                         : (g_level == LEVEL_PRINT) ? "The Redacted"
                         : (g_level == LEVEL_GER) ? "The Windswept"
                         : (g_level == LEVEL_ALCHEMY) ? "The Transmuted"
                         : (g_level == LEVEL_BATHS) ? "The Languid"
                         : (g_level == LEVEL_FLOAT) ? "The Adrift"
                         : (g_level == LEVEL_ISHTAR) ? "The Exiled"
                         : (g_level == LEVEL_TANNERY) ? "The Steeped"
                         : (g_level == LEVEL_MUSEUM) ? "The Exhibited"
                         : (g_level == LEVEL_GOMPA) ? "The Cloistered"
                         : (g_level == LEVEL_BUDDHA) ? "The Awakened"
                         : (g_level == LEVEL_ANGKOR) ? "The Devoured"
                         : (g_level == LEVEL_BADGIR) ? "The Sun-Struck"
                         : (g_level == LEVEL_TOPIARY) ? "The Overgrown"
                         : (g_level == LEVEL_GLASSWORKS) ? "The Vitrified"
                         : (g_level == LEVEL_SHIPYARD) ? "The Unlaunched"
                         : (g_level == LEVEL_GOTHIC) ? "The Excommunicate"
                         : (g_level == LEVEL_PUEBLO) ? "The Vanished"
                         : (boss_phase >= 2 ? "Aberrant Husk - Enraged" : "Aberrant Husk");
        int tw = MeasureText(name, 20);
        DrawText(name, sw / 2 - tw / 2, by - 26, 20, Color{ 217, 204, 178, 255 });
        Color fill = boss_phase >= 2 ? Color{ 204, 64, 20, 255 } : Color{ 140, 26, 31, 255 };
        bar(bx, by, bw, 16, boss_hp, fill);
        // posture bar directly under the HP bar; glows hot as it nears a break
        int pw = bw - 120, px = sw / 2 - pw / 2, py = by + 22;
        bool full = boss_posture >= 0.999f;
        unsigned char pulse = (unsigned char)(200 + 55 * sinf(t * 10.0f));
        Color pfill = full ? Color{ 255, 226, 120, pulse }
                           : Color{ (unsigned char)(210 + 45 * boss_posture), 170, 60, 255 };
        bar(px, py, pw, 8, boss_posture, pfill);
    }
    // lock-on marker is drawn in 3D on the target's body (see main.cpp), not a center reticle
}
