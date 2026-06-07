#pragma once
// The moonlit ruin: textured blood moon, arena glb, red crystals, a planar-
// reflective water surface, and gameplay collision (floor + boundary + obstacles).
#include "raylib.h"
#include <vector>

namespace arena {

struct Obstacle { Vector3 center; float radius; };

extern float   floor_y;
extern Vector3 boundary_center;
extern float   boundary_radius;
extern float   water_level;
extern std::vector<Obstacle> obstacles;

void load(Shader lit);
void update(float dt);
void draw_sky(Camera3D cam);                                      // gradient + moon-glow dome (backdrop)
void draw_world(Camera3D cam);                                    // reflectable scenery (no water)
void draw_water(Camera3D cam, Texture2D reflectTex, Vector2 screen);
void unload();
void resolve(Vector3& pos, float r);

}
