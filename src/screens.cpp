#include "screens.h"
#include "game.h"
#include "raylib.h"

void Screens::init() {
    state = g_game.state;
    g_game.state_changed.connect([this](int s) { state = s; });
}

static void center_text(const char* txt, int y, int size, Color col) {
    int w = MeasureText(txt, size);
    DrawText(txt, GetScreenWidth() / 2 - w / 2, y, size, col);
}

void Screens::draw() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    switch (state) {
        case Game::TITLE:
            center_text("WASD move  -  mouse camera  -  LMB attack  -  Space dodge  -  Q block  -  R heal  -  Tab lock-on",
                        sh / 2 + 60, 18, Color{ 217, 217, 217, 220 });
            center_text("Walk forward to wake the Husk", sh / 2 + 90, 20, Color{ 200, 180, 160, 220 });
            break;
        case Game::FIGHT:
            break;
        case Game::DEAD:
            DrawRectangle(0, 0, sw, sh, Color{ 64, 0, 0, 140 });
            center_text("YOU DIED", sh / 2 - 50, 80, Color{ 191, 26, 26, 255 });
            center_text("Press  [E]  or  [Enter]  to retry", sh / 2 + 50, 22, Color{ 217, 217, 217, 255 });
            break;
        case Game::VICTORY:
            DrawRectangle(0, 0, sw, sh, Color{ 51, 41, 5, 70 });
            center_text("VICTORY", sh / 6, 80, Color{ 217, 184, 77, 255 });
            center_text("The Husk falls.  Rest at the bonfire to save.", sh / 6 + 86, 22, Color{ 217, 217, 217, 230 });
            break;
    }
}
