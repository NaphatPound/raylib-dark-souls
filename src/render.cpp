#include "render.h"
#include "assets.h"
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

    Vector3 lightDir = Vector3Normalize({ 0.12f, 0.82f, -0.58f });  // toward the moon
    Vector3 lightCol = { 1.45f, 0.92f, 0.84f };                      // rose moonlight
    Vector3 ambient  = { 1.15f, 0.78f, 0.74f };                      // soft rose ambient (lifted)
    Vector3 fogCol   = { 0.24f, 0.09f, 0.10f };
    float   fogDen   = 0.0030f;
    float   emissive = 0.0f;
    SetShaderValue(shader, loc_lightDir, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_lightColor, &lightCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_ambient, &ambient, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_fogColor, &fogCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, loc_fogDensity, &fogDen, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, loc_emissive, &emissive, SHADER_UNIFORM_FLOAT);
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

void LitShader::apply_to_model(Model& m) {
    if (!ok) return;
    for (int i = 0; i < m.materialCount; i++) m.materials[i].shader = shader;
}
