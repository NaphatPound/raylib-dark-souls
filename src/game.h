#pragma once
// Global game-state machine (Godot's `Game` autoload).
#include "signal.h"
#include "actor.h"

// Selectable level / visual theme (set once at startup from the CLI).
enum Level { LEVEL_BLOODMOON = 0, LEVEL_FROZEN = 1 };
extern int g_level;

struct Game {
    enum State { TITLE, FIGHT, DEAD, VICTORY };

    State  state = TITLE;
    Actor* player = nullptr;
    Actor* boss = nullptr;
    bool   paused = false;

    Signal<int> state_changed;
    Signal<>    player_died;
    Signal<>    boss_died;
    Signal<>    restart_requested;   // main wires this to the full reset

    void set_state(State s);
    void start_fight();
    void on_player_died();
    void on_boss_died();
    void request_restart();
};

extern Game g_game;
