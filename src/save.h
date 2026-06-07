#pragma once
// Tiny persistent save: which level bosses have been beaten (shown as ✓ in the menu)
// plus the last level picked. Written when the player rests at a bonfire.
#include <string>

constexpr int NUM_LEVELS = 4;

struct SaveData {
    bool beaten[NUM_LEVELS] = { false, false, false, false };
    int  last_level = 0;
};

extern SaveData g_save;

void save_load();    // read savegame.txt into g_save (missing file = fresh save)
void save_write();   // write g_save to savegame.txt
const char* level_name(int lvl);
