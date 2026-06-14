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
        case 4:  return "Hollow Graveyard";
        case 5:  return "Fungal Grotto";
        case 6:  return "Desert Ziggurat";
        case 7:  return "Shipwreck Cove";
        case 8:  return "Sky Sanctum";
        case 9:  return "Clockwork Vault";
        case 10: return "Forgotten Shrine";
        case 11: return "Dragon Boneyard";
        case 12: return "Crystal Cavern";
        case 13: return "Overgrown Temple";
        case 14: return "Abandoned Mine";
        case 15: return "Astral Observatory";
        case 16: return "Forgotten Archive";
        case 17: return "Siege Camp";
        case 18: return "Sunken Aqueduct";
        case 19: return "Sentinel Court";
        case 20: return "Hanging Gardens";
        case 21: return "Chasm Bridge";
        case 22: return "Beacon Coast";
        case 23: return "Windmill Fields";
        case 24: return "Ossuary Catacombs";
        case 25: return "Forsaken Fairground";
        case 26: return "Ruined Throne Hall";
        case 27: return "Geyser Springs";
        case 28: return "Petrified Forest";
        case 29: return "Plague Hamlet";
        case 30: return "Druid Henge";
        case 31: return "Blighted Bog";
        case 32: return "Timber Sawmill";
        case 33: return "The Gaol";
        case 34: return "The Tar Pits";
        case 35: return "Sunlit Vineyard";
        case 36: return "Frozen Floes";
        case 37: return "The Quarry";
        case 38: return "The Skydock";
        case 39: return "Grand Conservatory";
        case 40: return "The Apiary";
        case 41: return "The Kiln Yard";
        case 42: return "The Coral Reef";
        case 43: return "The Foundry";
        case 44: return "Cathedral Nave";
        case 45: return "The Watermill";
        case 46: return "The Salt Pans";
        case 47: return "Stilt Village";
        case 48: return "The Hive";
        case 49: return "The Brewery";
        case 50: return "Bamboo Grove";
        case 51: return "Colliers' Wood";
        case 52: return "The Grand Plaza";
        case 53: return "Pumpkin Patch";
        case 54: return "The Bell Yard";
        case 55: return "The Aviary";
        case 56: return "The Spider's Lair";
        case 57: return "The Lotus Pond";
        case 58: return "The Archtree";
        case 59: return "The Gambit";
        case 60: return "The Hedge Maze";
        case 61: return "The Cascade";
        case 62: return "The Crater";
        case 63: return "The Fallen Titan";
        case 64: return "The Hoodoos";
        case 65: return "The Moai";
        case 66: return "Dripstone Hollow";
        case 67: return "Snowbound Pines";
        case 68: return "The Galleon";
        case 69: return "The Sunflower Fields";
        case 70: return "The Sphinx";
        case 71: return "The Great Dam";
        case 72: return "The Amphitheatre";
        case 73: return "The Great Wheel";
        case 74: return "The Redwood Grove";
        case 75: return "The Balloon Festival";
        case 76: return "The Canals";
        case 77: return "The Grain Elevator";
        case 78: return "The Great Organ";
        case 79: return "The Hypostyle Hall";
        case 80: return "The Star Fort";
        case 81: return "The Triumphal Arch";
        case 82: return "The Orchard";
        case 83: return "The Great Loom";
        case 84: return "The Savanna";
        case 85: return "The Great Mosque";
        case 86: return "The Totem Village";
        case 87: return "The Oasis";
        case 88: return "The Pagoda";
        case 89: return "The Stepwell";
        case 90: return "The Jantar Mantar";
        case 91: return "The Terracotta Army";
        case 92: return "The Ball Court";
        case 93: return "The Treasury";
        case 94: return "The Grand Bazaar";
        case 95: return "The Siege Works";
        case 96: return "The Whaling Station";
        case 97: return "The Cactus Forest";
        case 98: return "The Great Enclosure";
        case 99: return "Saint Basil's";
        case 100: return "The Printing House";
        case 101: return "The Ger Camp";
        case 102: return "The Alchemist's Lab";
        case 103: return "The Bath House";
        case 104: return "The Floating Market";
        case 105: return "The Ishtar Gate";
        case 106: return "The Tannery";
        case 107: return "The Natural History Hall";
        case 108: return "The Mountain Monastery";
        case 109: return "The Great Buddha";
        case 110: return "Angkor Wat";
        case 111: return "The Wind Towers";
        case 112: return "The Topiary Garden";
        case 113: return "The Glassworks";
        case 114: return "The Shipyard";
        case 115: return "The Cathedral Close";
        case 116: return "The Cliff Dwelling";
        default: return "Unknown";
    }
}

void save_load() {
    g_save = SaveData{};
    if (!FileExists(SAVE_PATH)) return;
    char* txt = LoadFileText(SAVE_PATH);   // raylib, NUL-terminated
    if (!txt) return;
    int b[NUM_LEVELS] = { 0 }, last = 0;
    if (sscanf(txt, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5], &b[6], &b[7], &b[8], &b[9], &b[10], &b[11], &b[12], &b[13], &b[14], &b[15], &b[16], &b[17], &b[18], &b[19], &b[20], &b[21], &b[22], &b[23], &b[24], &b[25], &b[26], &b[27], &b[28], &b[29], &b[30], &b[31], &b[32], &b[33], &b[34], &b[35], &b[36], &b[37], &b[38], &b[39], &b[40], &b[41], &b[42], &b[43], &b[44], &b[45], &b[46], &b[47], &b[48], &b[49], &b[50], &b[51], &b[52], &b[53], &b[54], &b[55], &b[56], &b[57], &b[58], &b[59], &b[60], &b[61], &b[62], &b[63], &b[64], &b[65], &b[66], &b[67], &b[68], &b[69], &b[70], &b[71], &b[72], &b[73], &b[74], &b[75], &b[76], &b[77], &b[78], &b[79], &b[80], &b[81], &b[82], &b[83], &b[84], &b[85], &b[86], &b[87], &b[88], &b[89], &b[90], &b[91], &b[92], &b[93], &b[94], &b[95], &b[96], &b[97], &b[98], &b[99], &b[100], &b[101], &b[102], &b[103], &b[104], &b[105], &b[106], &b[107], &b[108], &b[109], &b[110], &b[111], &b[112], &b[113], &b[114], &b[115], &b[116], &last) >= NUM_LEVELS) {
        for (int i = 0; i < NUM_LEVELS; i++) g_save.beaten[i] = b[i] != 0;
        if (last >= 0 && last < NUM_LEVELS) g_save.last_level = last;
    }
    UnloadFileText(txt);
}

void save_write() {
    char buf[1392];
    snprintf(buf, sizeof(buf), "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
             g_save.beaten[0] ? 1 : 0, g_save.beaten[1] ? 1 : 0,
             g_save.beaten[2] ? 1 : 0, g_save.beaten[3] ? 1 : 0,
             g_save.beaten[4] ? 1 : 0, g_save.beaten[5] ? 1 : 0,
             g_save.beaten[6] ? 1 : 0, g_save.beaten[7] ? 1 : 0,
             g_save.beaten[8] ? 1 : 0, g_save.beaten[9] ? 1 : 0,
             g_save.beaten[10] ? 1 : 0, g_save.beaten[11] ? 1 : 0,
             g_save.beaten[12] ? 1 : 0, g_save.beaten[13] ? 1 : 0,
             g_save.beaten[14] ? 1 : 0, g_save.beaten[15] ? 1 : 0,
             g_save.beaten[16] ? 1 : 0, g_save.beaten[17] ? 1 : 0,
             g_save.beaten[18] ? 1 : 0, g_save.beaten[19] ? 1 : 0,
             g_save.beaten[20] ? 1 : 0, g_save.beaten[21] ? 1 : 0,
             g_save.beaten[22] ? 1 : 0, g_save.beaten[23] ? 1 : 0,
             g_save.beaten[24] ? 1 : 0, g_save.beaten[25] ? 1 : 0,
             g_save.beaten[26] ? 1 : 0, g_save.beaten[27] ? 1 : 0,
             g_save.beaten[28] ? 1 : 0, g_save.beaten[29] ? 1 : 0,
             g_save.beaten[30] ? 1 : 0, g_save.beaten[31] ? 1 : 0,
             g_save.beaten[32] ? 1 : 0, g_save.beaten[33] ? 1 : 0,
             g_save.beaten[34] ? 1 : 0, g_save.beaten[35] ? 1 : 0,
             g_save.beaten[36] ? 1 : 0, g_save.beaten[37] ? 1 : 0,
             g_save.beaten[38] ? 1 : 0, g_save.beaten[39] ? 1 : 0,
             g_save.beaten[40] ? 1 : 0, g_save.beaten[41] ? 1 : 0,
             g_save.beaten[42] ? 1 : 0, g_save.beaten[43] ? 1 : 0,
             g_save.beaten[44] ? 1 : 0, g_save.beaten[45] ? 1 : 0,
             g_save.beaten[46] ? 1 : 0, g_save.beaten[47] ? 1 : 0,
             g_save.beaten[48] ? 1 : 0, g_save.beaten[49] ? 1 : 0,
             g_save.beaten[50] ? 1 : 0, g_save.beaten[51] ? 1 : 0,
             g_save.beaten[52] ? 1 : 0, g_save.beaten[53] ? 1 : 0,
             g_save.beaten[54] ? 1 : 0, g_save.beaten[55] ? 1 : 0,
             g_save.beaten[56] ? 1 : 0, g_save.beaten[57] ? 1 : 0,
             g_save.beaten[58] ? 1 : 0, g_save.beaten[59] ? 1 : 0,
             g_save.beaten[60] ? 1 : 0, g_save.beaten[61] ? 1 : 0,
             g_save.beaten[62] ? 1 : 0, g_save.beaten[63] ? 1 : 0,
             g_save.beaten[64] ? 1 : 0, g_save.beaten[65] ? 1 : 0,
             g_save.beaten[66] ? 1 : 0, g_save.beaten[67] ? 1 : 0,
             g_save.beaten[68] ? 1 : 0, g_save.beaten[69] ? 1 : 0,
             g_save.beaten[70] ? 1 : 0, g_save.beaten[71] ? 1 : 0,
             g_save.beaten[72] ? 1 : 0, g_save.beaten[73] ? 1 : 0,
             g_save.beaten[74] ? 1 : 0, g_save.beaten[75] ? 1 : 0,
             g_save.beaten[76] ? 1 : 0, g_save.beaten[77] ? 1 : 0,
             g_save.beaten[78] ? 1 : 0, g_save.beaten[79] ? 1 : 0,
             g_save.beaten[80] ? 1 : 0, g_save.beaten[81] ? 1 : 0,
             g_save.beaten[82] ? 1 : 0, g_save.beaten[83] ? 1 : 0,
             g_save.beaten[84] ? 1 : 0, g_save.beaten[85] ? 1 : 0,
             g_save.beaten[86] ? 1 : 0, g_save.beaten[87] ? 1 : 0,
             g_save.beaten[88] ? 1 : 0, g_save.beaten[89] ? 1 : 0,
             g_save.beaten[90] ? 1 : 0, g_save.beaten[91] ? 1 : 0,
             g_save.beaten[92] ? 1 : 0, g_save.beaten[93] ? 1 : 0,
             g_save.beaten[94] ? 1 : 0, g_save.beaten[95] ? 1 : 0,
             g_save.beaten[96] ? 1 : 0, g_save.beaten[97] ? 1 : 0,
             g_save.beaten[98] ? 1 : 0, g_save.beaten[99] ? 1 : 0,
             g_save.beaten[100] ? 1 : 0, g_save.beaten[101] ? 1 : 0,
             g_save.beaten[102] ? 1 : 0, g_save.beaten[103] ? 1 : 0,
             g_save.beaten[104] ? 1 : 0, g_save.beaten[105] ? 1 : 0,
             g_save.beaten[106] ? 1 : 0, g_save.beaten[107] ? 1 : 0,
             g_save.beaten[108] ? 1 : 0, g_save.beaten[109] ? 1 : 0,
             g_save.beaten[110] ? 1 : 0, g_save.beaten[111] ? 1 : 0,
             g_save.beaten[112] ? 1 : 0, g_save.beaten[113] ? 1 : 0,
             g_save.beaten[114] ? 1 : 0, g_save.beaten[115] ? 1 : 0,
             g_save.beaten[116] ? 1 : 0,
             g_save.last_level);
    SaveFileText(SAVE_PATH, buf);          // raylib
}
