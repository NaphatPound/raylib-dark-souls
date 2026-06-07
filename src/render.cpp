#include "render.h"
#include "assets.h"
#include "game.h"
#include "raymath.h"

LitShader g_lit;

void LitShader::load() {
    shader = LoadShader(assets::path("shaders/lit.vs").c_str(),
                        assets::path("shaders/lit.fs").c_str());
    ok = (shader.id != 0);
    if (!ok) return;
    shader.locs[SHADER_LOC_MATRIX_MODEL]  = GetShaderLocation(shader, "matModel");
    shader.locs[SHADER_LOC_MATRIX_NORMAL] = GetShaderLocation(shader, "matNormal");
    loc_lightDir   = GetShaderLocation(shader, "uLightDir");
    loc_lightColor = GetShaderLocation(shader, "uLightColor");
    loc_ambient    = GetShaderLocation(shader, "uAmbient");
    loc_viewPos    = GetShaderLocation(shader, "uViewPos");
    loc_fogColor   = GetShaderLocation(shader, "uFogColor");
    loc_fogDensity = GetShaderLocation(shader, "uFogDensity");
    loc_emissive   = GetShaderLocation(shader, "uEmissive");
    loc_plightN    = GetShaderLocation(shader, "uPLightN");
    loc_plights    = GetShaderLocation(shader, "uPLights");
    loc_plightColor= GetShaderLocation(shader, "uPLightColor");
    loc_clipY      = GetShaderLocation(shader, "uClipY");
    loc_clipBelow  = GetShaderLocation(shader, "uClipBelow");

    Vector3 lightDir = Vector3Normalize({ 0.12f, 0.82f, -0.58f });  // toward the moon
    Vector3 lightCol = { 1.45f, 0.92f, 0.84f };                      // rose moonlight
    Vector3 ambient  = { 1.15f, 0.78f, 0.74f };                      // soft rose ambient (lifted)
    Vector3 fogCol   = { 0.24f, 0.09f, 0.10f };
    float   fogDen   = 0.0030f;
    if (g_level == LEVEL_FROZEN) {                                   // cold frozen-cathedral lighting (per design spec)
        lightDir = Vector3Normalize({ 0.35f, 0.82f, 0.45f });        // high pale overcast light
        lightCol = { 0.90f, 1.05f, 1.22f };                          // cool blue-white
        ambient  = { 0.70f, 0.84f, 1.05f };                          // pale icy ambient (lifted)
        fogCol   = { 0.72f, 0.83f, 0.92f };                          // milky cold haze (#D8EEF3-ish)
        fogDen   = 0.0120f;                                          // low-hanging cold mist
    }
    float   emissive = 0.0f;
    SetShaderValue(shader, loc_lightDir, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_lightColor, &lightCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_ambient, &ambient, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_fogColor, &fogCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_fogDensity, &fogDen, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, loc_emissive, &emissive, SHADER_UNIFORM_FLOAT);
    set_clip(0.0f, false);   // reflection clip off by default (main pass)
}

void LitShader::unload() { if (ok) UnloadShader(shader); }

void LitShader::set_frame(Vector3 viewPos) {
    if (!ok) return;
    SetShaderValue(shader, loc_viewPos, &viewPos, SHADER_UNIFORM_VEC3);
}

void LitShader::set_emissive(float e) {
    if (!ok) return;
    SetShaderValue(shader, loc_emissive, &e, SHADER_UNIFORM_FLOAT);
}

void LitShader::set_point_lights(const Vector4* lights, int count, Vector3 color) {
    if (!ok) return;
    if (count > 32) count = 32;
    SetShaderValue(shader, loc_plightN, &count, SHADER_UNIFORM_INT);
    SetShaderValue(shader, loc_plightColor, &color, SHADER_UNIFORM_VEC3);
    if (count > 0)
        SetShaderValueV(shader, loc_plights, lights, SHADER_UNIFORM_VEC4, count);
}

void LitShader::set_clip(float water_y, bool below) {
    if (!ok) return;
    int b = below ? 1 : 0;
    SetShaderValue(shader, loc_clipBelow, &b, SHADER_UNIFORM_INT);
    SetShaderValue(shader, loc_clipY, &water_y, SHADER_UNIFORM_FLOAT);
}

void LitShader::apply_to_model(Model& m) {
    if (!ok) return;
    for (int i = 0; i < m.materialCount; i++) m.materials[i].shader = shader;
}
