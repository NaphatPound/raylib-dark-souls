#pragma once
// Moonlit lighting shader wrapper (directional moon + ambient + red fog).
#include "raylib.h"

struct LitShader {
    Shader shader{};
    int loc_lightDir = -1, loc_lightColor = -1, loc_ambient = -1;
    int loc_viewPos = -1, loc_fogColor = -1, loc_fogDensity = -1, loc_emissive = -1;
    bool ok = false;

    void load();
    void unload();
    void set_frame(Vector3 viewPos);    // update per-frame view position
    void set_emissive(float e);
    void apply_to_model(Model& m);      // assign this shader to every material
};

extern LitShader g_lit;
