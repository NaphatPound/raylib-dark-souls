#pragma once
#include "raylib.h"

struct Hit;

// Common interface for the two combatants (player, boss). Mirrors the loose
// duck-typed Node coupling in the GDScript (is_dead / take_hit / parried / etc).
struct Actor {
    virtual ~Actor() = default;
    virtual Vector3 position() const = 0;
    virtual bool is_dead() const = 0;
    virtual void on_hurt(const Hit& h) {}     // a hurtbox passed us a hit -> resolve it
    virtual void parried() {}                  // boss only: the player parried our swing
    virtual void take_riposte(float dmg) {}    // boss only: the player's takedown finisher
};
