#pragma once
// Title hint + YOU DIED / VICTORY overlays. Ports screens.gd.
struct Screens {
    void init();
    void draw();
    int state = 0;   // Game::State
};
