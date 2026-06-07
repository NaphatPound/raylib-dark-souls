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
    g_events.riposte_available.connect([this](Actor*) { prompt = true; });
    g_events.riposte_ended.connect([this]() { prompt = false; });
    g_game.state_changed.connect([this](int s) {
        if (s == Game::TITLE) { boss_visible = false; reticle = false; prompt = false; }
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
        const char* name = boss_phase >= 2 ? "Aberrant Husk - Enraged" : "Aberrant Husk";
        int tw = MeasureText(name, 20);
        DrawText(name, sw / 2 - tw / 2, by - 26, 20, Color{ 217, 204, 178, 255 });
        Color fill = boss_phase >= 2 ? Color{ 204, 64, 20, 255 } : Color{ 140, 26, 31, 255 };
        bar(bx, by, bw, 16, boss_hp, fill);
    }
    // lock-on reticle
    if (reticle) DrawRectangle(sw / 2 - 5, sh / 2 - 5, 10, 10, Color{ 255, 255, 255, 217 });
    // takedown prompt
    if (prompt) {
        unsigned char a = (unsigned char)(255 * (0.6f + 0.4f * sinf(t * 9.0f)));
        const char* txt = "[ E ]  TAKEDOWN";
        int fw = MeasureText(txt, 30);
        DrawText(txt, sw / 2 - fw / 2 + 2, sh / 2 - 108 + 2, 30, Color{ 0, 0, 0, a });
        DrawText(txt, sw / 2 - fw / 2, sh / 2 - 110, 30, Color{ 255, 219, 82, a });
    }
}
