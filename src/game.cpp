#include "game.h"
#include "events.h"

Game g_game;
Events g_events;

void Game::set_state(State s) {
    if (state == s) return;
    state = s;
    state_changed.emit((int)s);
}

void Game::start_fight() {
    if (state == TITLE) {
        set_state(FIGHT);
        g_events.boss_aggro.emit();
    }
}

void Game::on_player_died() {
    if (state == DEAD || state == VICTORY) return;
    set_state(DEAD);
    player_died.emit();
}

void Game::on_boss_died() {
    if (state == VICTORY || state == DEAD) return;
    set_state(VICTORY);
    boss_died.emit();
}

void Game::request_restart() {
    state = TITLE;
    paused = false;
    restart_requested.emit();
    state_changed.emit((int)TITLE);
}
