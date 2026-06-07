#include "save.h"
#include "raylib.h"
#include <cstdio>

SaveData g_save;

static const char* SAVE_PATH = "savegame.txt";

const char* level_name(int lvl) {
    switch (lvl) {
        case 0:  return "Blood-Moon Ruin";
        case 1:  return "Frozen Cathedral";
        case 2:  return "Volcanic Forge";
        case 3:  return "Sunken Colosseum";
        default: return "Unknown";
    }
}

void save_load() {
    g_save = SaveData{};
    if (!FileExists(SAVE_PATH)) return;
    char* txt = LoadFileText(SAVE_PATH);   // raylib, NUL-terminated
    if (!txt) return;
    int b[NUM_LEVELS] = { 0 }, last = 0;
    if (sscanf(txt, "%d %d %d %d %d", &b[0], &b[1], &b[2], &b[3], &last) >= NUM_LEVELS) {
        for (int i = 0; i < NUM_LEVELS; i++) g_save.beaten[i] = b[i] != 0;
        if (last >= 0 && last < NUM_LEVELS) g_save.last_level = last;
    }
    UnloadFileText(txt);
}

void save_write() {
    char buf[96];
    snprintf(buf, sizeof(buf), "%d %d %d %d %d\n",
             g_save.beaten[0] ? 1 : 0, g_save.beaten[1] ? 1 : 0,
             g_save.beaten[2] ? 1 : 0, g_save.beaten[3] ? 1 : 0, g_save.last_level);
    SaveFileText(SAVE_PATH, buf);          // raylib
}
