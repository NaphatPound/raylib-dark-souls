#pragma once
// Small math helpers that mirror the GDScript builtins used in the original
// (lerp_angle, move_toward, move_toward angular), built on raymath.
#include "raylib.h"
#include "raymath.h"
#include <cmath>

inline float wrap_pi(float a) {
    while (a >  PI) a -= 2.0f * PI;
    while (a < -PI) a += 2.0f * PI;
    return a;
}

// Angular lerp (shortest path), matching Godot lerp_angle.
inline float lerp_angle(float from, float to, float t) {
    return from + wrap_pi(to - from) * t;
}

// Move a scalar toward a target by at most `delta`, matching Godot move_toward.
inline float move_toward(float from, float to, float delta) {
    if (fabsf(to - from) <= delta) return to;
    return from + (to > from ? 1.0f : -1.0f) * delta;
}

// XZ-plane direction the actor faces given a yaw (yaw 0 -> +Z), used to build
// forward vectors the same way the GDScript did with Vector3(sin(a),0,cos(a)).
inline Vector3 yaw_forward(float yaw) {
    return Vector3{ sinf(yaw), 0.0f, cosf(yaw) };
}

inline Vector3 v3(float x, float y, float z) { return Vector3{ x, y, z }; }
inline Vector3 flat(Vector3 v) { v.y = 0.0f; return v; }

// Rotate a vector about +Y by `a` radians, matching Godot's Basis(Vector3.UP, a).
inline Vector3 rotate_y(Vector3 v, float a) {
    float c = cosf(a), s = sinf(a);
    return Vector3{ c * v.x + s * v.z, v.y, -s * v.x + c * v.z };
}

inline float rand_range(float lo, float hi) {
    return lo + (hi - lo) * (float)GetRandomValue(0, 100000) / 100000.0f;
}
