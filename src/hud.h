#pragma once
// In-game HUD: player HP/stamina bars, estus pips, boss HP bar, lock-on reticle,
// takedown prompt. Driven by the Events bus. Ports hud.gd.
struct HUD {
    void init();
    void update(float dt);
    void draw();

    float hp = 1.0f, sp = 1.0f, boss_hp = 1.0f;
    int   flask = 4, flask_max = 4;
    int   boss_phase = 1;
    bool  boss_visible = false;
    bool  reticle = false;
    bool  prompt = false;
    float t = 0.0f;
};
