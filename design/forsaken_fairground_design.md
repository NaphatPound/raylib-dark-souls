# Forsaken Fairground — Level Design (LEVEL_FAIR = 25)

A derelict, rain-slicked carnival under a murky dusk. The twenty-second **non-boss**
level: the crowd that never left ("The Revelers") still mills among the stalls. Clear
them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **carnival** silhouette with a turning
carousel, found in none of the other twenty-five levels. Whimsical-creepy; reuses the
Siege Camp's `draw_tent` for the striped tents.

## Theme & palette
- **Mood:** abandoned, garish, eerie — a still-turning carousel, faded bunting, and
  colored bulbs glowing over wet ground.
- **Light (render.cpp):** dim faded dusk, murky violet-grey ambient, murky haze
  (`~#524C5C`, density `0.0090` so the lights pop). Reuses `sky_storm.fs` and the dark
  `water_storm.fs` as the wet, reflective ground.
- **Glow:** warm string-light fill `(0.95, 0.70, 0.55)`; the bulbs themselves are
  drawn in four colors.

## Geometry (arena.cpp :: build_fair / draw_fair)
Reuses `s_cyl` (carousel base/pole/horse legs/light-poles), `s_cone` (canopy), `s_column`
(horse bodies / stalls / bunting / awnings), `draw_tent` (camp helper).
1. **Carousel** — a base + central pole + a striped conical canopy (slowly turning)
   + six bobbing horses on poles, the whole ring spun by `s_time`. Landmark + obstacle.
2. **Striped tents** — eight bright carnival tents (red/blue/yellow/green/white) via
   `draw_tent`; some collapsed.
3. **Bunting** — sagging triangular flags strung between ten light-poles.
4. **Game stalls** — three counters with striped awnings on posts.
5. **String-lights** — colored additive bulbs ringing the fair (the lights).

## Encounter (mobs.cpp)
- **7 "Revelers"** — the `Mob` horde, level-aware spawn set (`fair_defs`), among the
  stalls. Same lightweight AI, lock-on, aggregate HUD bar (named *The Revelers*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–24 and the `Boss` are untouched. `mob_level` now covers
  `… || OSSUARY || FAIR`. Reuses the camp's `Tent`/`s_tents` + `draw_tent` (no new
  struct; `s_tents`/`s_wisps` are already cleared on unload).
- CLI: `fair` / `fairground` / `carnival` launches straight into it.
