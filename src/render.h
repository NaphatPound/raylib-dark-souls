#pragma once
// Moonlit lighting shader wrapper (directional moon + ambient + red fog).
#include "raylib.h"

struct LitShader {
    Shader shader{};
    int loc_lightDir = -1, loc_lightColor = -1, loc_ambient = -1;
    int loc_viewPos = -1, loc_fogColor = -1, loc_fogDensity = -1, loc_emissive = -1;
    int loc_plightN = -1, loc_plights = -1, loc_plightColor = -1;
    int loc_clipY = -1, loc_clipBelow = -1;
    bool ok = false;

    void load();
    void unload();
    void set_frame(Vector3 viewPos);    // update per-frame view position
    void set_emissive(float e);
    void set_point_lights(const Vector4* lights, int count, Vector3 color);  // crystal glow on geometry
    void set_clip(float water_y, bool below);   // reflection-pass clip plane (only above-water reflects)
    void apply_to_model(Model& m);      // assign this shader to every material
};

extern LitShader g_lit;
