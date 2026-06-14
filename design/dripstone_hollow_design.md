# Dripstone Hollow — Level Design (LEVEL_CAVERN = 66)

A limestone cavern: hanging **stalactites** drip from a dark stone ceiling toward rising
**stalagmites**, the two meeting in great floor-to-ceiling columns over a still teal
underground pool, with falling water drips and cool mineral mist. The sixty-third
**non-boss** level — the pool's drowned dead ("The Sunken") rise from the dark water.
Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **dripstone-cavern** silhouette (stalactites +
stalagmites + columns + a ceiling, over a reflecting pool) found in none of the other
sixty-six levels. A **WET** level (the reused teal `water_ice` plane is the cave pool).
Deliberately distinct from the Crystal Cavern (colossal glowing magenta/cyan gems): this is
matte pale-tan limestone dripstone in a dim, cool, enclosed hollow.

## Theme & palette
- **Mood:** still, cold, echoing — faint light from a hidden opening above.
- **Light (render.cpp):** faint overhead `(0.20,0.84,0.30)`, dim cool cave light
  `(0.70,0.74,0.78)`, dark cool fill, dark teal-grey murk (`~#42525C`, density `0.0120`).
  Reuses `sky_storm.fs` (mostly hidden by the ceiling); **WET** — the reused teal
  `water_ice` plane is the cave pool.
- **Glow:** cool teal dripstone `(0.55,0.80,0.85)`.

## Geometry (arena.cpp :: draw_dripcone / build_cavern / draw_cavern)
Reuses the lit `s_cone` (the dripstone cones — apex-down for stalactites, apex-up for
stalagmites), `s_column` (ceiling slab), `s_cyl` (the joint of each column), `s_dome`
(stone rim banks).
1. **Ceiling** — a dark stone slab high overhead (H≈15) so the stalactites have a roof.
2. **Stalactites** (`draw_dripcone(down)`) — 26 cones hanging from the ceiling.
3. **Stalagmites** (`draw_dripcone(up)`) — 18 cones rising from the pool floor (big ones
   are obstacles).
4. **Columns** — 4 great floor-to-ceiling columns (stalagmite + stalactite + a cyl joint;
   obstacles) and a central great column (the landmark obstacle).
5. **Pool & banks** — the reused teal `water_ice` plane; 16 stone rim banks (`s_dome`).
6. **Atmosphere** — additive falling water drips (from the stalactite tips), mineral glow,
   and cool mist hanging over the pool.

## Encounter (mobs.cpp)
- **8 "Sunken"** — the `Mob` horde, level-aware spawn set (`cavern_defs`). Same lightweight
  AI, lock-on, aggregate HUD bar (named *The Sunken*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–65 and the `Boss` are untouched. `mob_level` now covers
  `… || MOAI || CAVERN`. **No new global struct/vector** — columns are a fixed array shared
  by build (obstacles) + draw; stalactites/stalagmites are seeded deterministic loops
  (seeds 14100/14200, drips reuse 14100); only `s_wisps` reused. Teal lights built in
  `build_cavern` (so `build_crystals`/`draw_crystals` early-return).
- WET level: `draw_water` still draws the pool plane (NOT in the dry early-return);
  `draw_cavern` lays the ceiling/dripstone over it.
- CLI: `cavern` / `dripstone` / `limestone` launches straight into it.
