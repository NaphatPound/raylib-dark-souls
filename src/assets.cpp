#include "assets.h"
#include <unordered_map>

namespace assets {

std::string path(const std::string& rel) {
    return std::string(ASSET_DIR) + "/" + rel;
}

static std::unordered_map<std::string, Texture2D> g_cache;

Texture2D texture(const std::string& rel) {
    auto it = g_cache.find(rel);
    if (it != g_cache.end()) return it->second;
    Texture2D t{};
    std::string p = path(rel);
    if (FileExists(p.c_str())) {
        t = LoadTexture(p.c_str());
        SetTextureFilter(t, TEXTURE_FILTER_BILINEAR);
    } else {
        Image img = GenImageColor(2, 2, WHITE);
        t = LoadTextureFromImage(img);
        UnloadImage(img);
    }
    g_cache[rel] = t;
    return t;
}

void unload_all() {
    for (auto& kv : g_cache) UnloadTexture(kv.second);
    g_cache.clear();
}

}
