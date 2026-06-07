#include "pause_menu.h"
#include "player.h"
#include "audio_sys.h"
#include "game.h"
#include "raylib.h"

void PauseMenu::open() { resume_requested = false; quit_requested = false; active_slider = -1; }

static bool button(Rectangle r, const char* label) {
    Vector2 m = GetMousePosition();
    bool hover = CheckCollisionPointRec(m, r);
    DrawRectangleRec(r, hover ? Color{ 70, 24, 26, 255 } : Color{ 40, 16, 18, 255 });
    DrawRectangleLinesEx(r, 2, Color{ 128, 31, 31, 255 });
    int tw = MeasureText(label, 20);
    DrawText(label, (int)(r.x + r.width / 2 - tw / 2), (int)(r.y + r.height / 2 - 10), 20, Color{ 220, 210, 200, 255 });
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// returns the (possibly updated) value; sets *grabbed when this slider owns the drag
static float slider(Rectangle r, const char* label, float v, float lo, float hi, float step, int id, int& active) {
    DrawText(label, (int)r.x, (int)r.y - 20, 18, Color{ 205, 200, 184, 255 });
    DrawRectangleRec(r, Color{ 30, 24, 26, 255 });
    float frac = (v - lo) / (hi - lo);
    DrawRectangle((int)r.x, (int)r.y, (int)(r.width * frac), (int)r.height, Color{ 150, 60, 50, 255 });
    float hx = r.x + r.width * frac;
    DrawRectangle((int)hx - 4, (int)r.y - 3, 8, (int)r.height + 6, Color{ 230, 200, 120, 255 });
    Vector2 m = GetMousePosition();
    Rectangle hit = { r.x - 6, r.y - 6, r.width + 12, r.height + 12 };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, hit)) active = id;
    if (active == id && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float f = (m.x - r.x) / r.width;
        if (f < 0) f = 0; if (f > 1) f = 1;
        v = lo + f * (hi - lo);
        if (step > 0.0f) v = lo + roundf((v - lo) / step) * step;   // quantize like Godot HSlider
    }
    return v;
}

void PauseMenu::draw_and_update() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, Color{ 0, 0, 0, 168 });

    float px = sw / 2 - 230, py = sh / 2 - 220;
    DrawRectangle((int)px, (int)py, 460, 440, Color{ 26, 10, 13, 245 });
    DrawRectangleLines((int)px, (int)py, 460, 440, Color{ 128, 31, 31, 255 });
    const char* title = "PAUSED";
    int tw = MeasureText(title, 44);
    DrawText(title, sw / 2 - tw / 2, (int)py + 24, 44, Color{ 217, 191, 166, 255 });

    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) active_slider = -1;

    float sx = px + 40, sliderw = 380;
    float y = py + 110;
    if (player) {
        float v = slider({ sx, y, sliderw, 16 }, "Mouse Sensitivity", player->get_mouse_sensitivity(),
                         0.0008f, 0.008f, 0.0001f, 0, active_slider);
        player->set_mouse_sensitivity(v);
    }
    y += 56;
    g_audio.master = slider({ sx, y, sliderw, 16 }, "Master Volume", g_audio.master, 0.0f, 1.0f, 0.01f, 1, active_slider);
    y += 56;
    g_audio.music = slider({ sx, y, sliderw, 16 }, "Music Volume", g_audio.music, 0.0f, 1.0f, 0.01f, 2, active_slider);
    y += 56;
    g_audio.sfx_vol = slider({ sx, y, sliderw, 16 }, "SFX Volume", g_audio.sfx_vol, 0.0f, 1.0f, 0.01f, 3, active_slider);

    y += 44;
    if (button({ sx, y, sliderw, 38 }, "Resume")) resume_requested = true;
    y += 46;
    if (button({ sx, y, sliderw, 38 }, "Retry"))  { g_game.request_restart(); resume_requested = true; }
    y += 46;
    if (button({ sx, y, sliderw, 38 }, "Quit"))   quit_requested = true;
}
