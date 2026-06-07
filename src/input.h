#pragma once
// Action mapping (Godot InputMap equivalent), built on raylib key/mouse queries.
#include "raylib.h"
#include "mathx.h"

namespace in {

inline bool act_pressed_attack_light() { return IsMouseButtonPressed(MOUSE_BUTTON_LEFT); }
inline bool act_pressed_attack_heavy() { return IsMouseButtonPressed(MOUSE_BUTTON_RIGHT); }
inline bool act_pressed_dodge()        { return IsKeyPressed(KEY_SPACE); }
inline bool act_pressed_heal()         { return IsKeyPressed(KEY_R); }
inline bool act_pressed_interact()     { return IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER); }
inline bool act_pressed_lock_on()      { return IsKeyPressed(KEY_TAB) || IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE); }
inline bool act_pressed_pause()        { return IsKeyPressed(KEY_ESCAPE); }

inline bool act_down_block()  { return IsKeyDown(KEY_Q); }
inline bool act_down_sprint() { return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT); }

// Godot Input.get_vector("move_left","move_right","move_forward","move_back"):
//   x = right - left,  y = back - forward   (clamped to unit length)
inline Vector2 move_vector() {
    float x = (IsKeyDown(KEY_D) ? 1.f : 0.f) - (IsKeyDown(KEY_A) ? 1.f : 0.f);
    float y = (IsKeyDown(KEY_S) ? 1.f : 0.f) - (IsKeyDown(KEY_W) ? 1.f : 0.f);
    Vector2 v{ x, y };
    float l = sqrtf(x * x + y * y);
    if (l > 1.0f) { v.x /= l; v.y /= l; }
    return v;
}

} // namespace in
