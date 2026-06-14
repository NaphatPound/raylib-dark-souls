# The Star Fort — Level Design (LEVEL_FORT = 80)

A hexagonal bastion fortress: sloped earthwork ramparts topped with crenellated stone walls
and projecting bastions enclose a parade ground, cannon batteries bristle from the fire
steps and a colours-flying flag mast, a well and cannonball pyramids stand in the yard, and
a dark gatehouse arch breaks the wall. The seventy-seventh **non-boss** level — the fort's
dead defenders ("The Garrison") rise in the parade ground. Clear them all to win; the
bonfire then ignites as usual.

New procedural geometry, not a recolour — a **bastion-fortress** silhouette (angular
crenellated ramparts + bastions + cannon batteries ringing a parade ground) found in none of
the other eighty levels. A **DRY** level (parade-ground floor). Deliberately distinct from
the Siege Camp (conical tents + a log palisade), the Gaol (a square cell courtyard), and the
Throne Hall (a colonnaded interior): this is an enclosing star-fort with cannon.

## Theme & palette
- **Mood:** grim, embattled, smoky — a grey siege day behind high stone walls.
- **Light (render.cpp):** flat overcast `(0.36,0.72,0.38)`, cool grey daylight
  `(0.90,0.88,0.84)`, even fill, grey siege haze (`~#949494`, density `0.0090`). Reuses
  `sky_storm.fs`; **DRY** (`draw_fort` lays the parade ground).
- **Glow:** warm cannon-fire / muzzle flash `(1.00,0.66,0.34)`.

## Geometry (arena.cpp :: build_fort / draw_fort)
Reuses the lit `s_column` (earthworks / walls / crenellations / bastions / floor / cannon
carriages / flag / gatehouse), `s_cyl` (flag mast / well), `s_dome` (cannonball pyramids),
and `draw_bone_seg` (cannon barrels).
1. **Ramparts** — six wall segments around a hexagon (radius 20): each a sloped earthwork
   base + a stone wall + crenellation merlons + two cannons on the fire step (a carriage + a
   `draw_bone_seg` barrel pointing outward). The walls are a solid barrier (obstacles sampled
   along each segment); **bastions** project at each vertex.
2. **Parade ground** — the floor, with a central **flag mast** (waving flag via `s_time`; the
   landmark obstacle), a **well**, and two **cannonball pyramids**.
3. **Gatehouse** — a wall block with a dark archway at the front vertex.
4. **Atmosphere** — additive periodic **cannon muzzle smoke** off the bastions + drifting
   battle smoke + warm glow.

## Encounter (mobs.cpp)
- **8 "Garrison"** — the `Mob` horde, level-aware spawn set (`fort_defs`), in the parade
  ground. Same lightweight AI, lock-on, aggregate HUD bar (named *The Garrison*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–79 and the `Boss` are untouched. `mob_level` now covers
  `… || HYPO || FORT`. **No new global struct/vector** — the hexagon vertices are recomputed
  identically in build + draw; the rampart obstacles are sampled along each segment; only
  `s_wisps` reused. Cannon-smoke lights built in `build_fort` (so `build_crystals`/
  `draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_fort` lays the parade ground.
- CLI: `fort` / `bastion` / `fortress` launches straight into it.
