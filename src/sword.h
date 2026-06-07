#pragma once
#include "raylib.h"
class AnimController;

// Draw a longsword gripped in the model's right hand, following the hand bone's
// full transform at the current animation frame. pos/yaw place the model in world.
void draw_hand_sword(Model& model, AnimController& anim, Vector3 pos, float yaw_rad);

// World positions of the blade's base (above the guard) and tip, for the swing trail.
void sword_blade_segment(Model& model, AnimController& anim, Vector3 pos, float yaw_rad,
                         Vector3& out_base, Vector3& out_tip);
