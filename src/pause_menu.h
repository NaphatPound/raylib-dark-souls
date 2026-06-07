#pragma once
// Pause + settings overlay: Resume / Retry / Quit and sliders for mouse
// sensitivity and Master/Music/SFX volume. Ports pause_menu.gd.
#include "raylib.h"

class Player;

struct PauseMenu {
    Player* player = nullptr;
    bool quit_requested = false;
    bool resume_requested = false;

    void open();
    // Returns false while open; the caller closes on resume/retry/quit.
    void draw_and_update();

private:
    int active_slider = -1;
};
