#pragma once
// Title hint + YOU DIED / VICTORY overlays. Ports screens.gd.
struct Screens {
    void init();
    void draw();
    int state = 0;          // Game::State
    double victory_start = -1.0;   // GetTime() when VICTORY began; overlay hides after 5s
};
