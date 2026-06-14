# Geyser Springs — Level Design (LEVEL_SPRINGS = 27)

A steaming geothermal basin of terraced mineral pools. The twenty-fourth **non-boss**
level: those the springs claimed ("The Scalded") wade the shallows. Clear them all to
win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **geothermal hot-springs** silhouette
(travertine terrace pools + steam columns + a geyser) found in none of the other
twenty-seven levels.

## Theme & palette
- **Mood:** primordial, hushed, hot — drifting steam over still turquoise pools and
  cream-orange mineral terraces.
- **Light (render.cpp):** pale misty daylight through steam, bright steamy ambient,
  warm-white steam haze (`~#9EA39E`, density `0.0110`). Reuses `sky_storm.fs`; the
  floor is the reused **turquoise `water_ice.fs`** spring water.
- **Glow:** warm mineral fill `(0.85, 0.82, 0.70)`.

## Geometry (arena.cpp :: build_springs / draw_springs)
Reuses `s_torus` (terrace rims), `s_dome` (mineral mounds), `s_cyl` (mud pots / vent),
`s_cone` (geyser cone).
1. **Travertine terrace rims** — five concentric low flattened `s_torus` ledges
   (cream/orange) rising outward, framing the pools.
2. **Central geyser** — a mineral cone + dark vent, erupting a tall pulsing additive
   water/steam jet (animated). The landmark and a collision obstacle.
3. **Steam columns** — additive rising plumes at six vents around the pools.
4. **Mineral mounds** — cream/orange travertine domes; **mud pots** — dark shallow
   pools, scattered.

## Encounter (mobs.cpp)
- **7 "Scalded"** — the `Mob` horde, level-aware spawn set (`springs_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Scalded*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–26 and the `Boss` are untouched. `mob_level` now covers
  `… || THRONE || SPRINGS`. No new struct/unload (vents use the shared `s_wisps`).
- Keeps the reused water plane as the turquoise spring water.
- CLI: `springs` / `geyser` / `hotsprings` launches straight into it.
