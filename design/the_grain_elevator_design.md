# The Grain Elevator — Level Design (LEVEL_SILO = 77)

A dusty prairie grain works: a cluster of tall concrete silos with conical roofs, a towering
headhouse work-tower, an elevated conveyor gallery running along the silo tops, a loading
chute down to a grain wagon, golden grain piles and a weathered red barn. The seventy-fourth
**non-boss** level — the works' shambling dead ("The Husked") rise in the yard. Clear them
all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **grain-elevator / silo-cluster** silhouette (a
group of tall cylinders + a headhouse + a top gallery) found in none of the other
seventy-seven levels. A **DRY** level (dusty yard). Deliberately distinct from the Windmill
Fields (windmills + a barn over open farmland): this is a vertical industrial silo complex.

## Theme & palette
- **Mood:** sun-baked, dusty, still — golden prairie light and grain dust in the air.
- **Light (render.cpp):** warm prairie sun `(0.44,0.72,0.34)`, warm golden daylight
  `(1.10,1.00,0.78)`, warm fill, warm grain-dust haze (`~#C7BD9E`, density `0.0082`). Reuses
  `sky_ice.fs`; **DRY** (`draw_silo_yard` lays the dusty yard).
- **Glow:** warm grain / dusty sun `(1.00,0.85,0.50)`.

## Geometry (arena.cpp :: draw_silo / build_silo / draw_silo_yard)
Reuses the lit `s_cyl` (silo bodies + hoop bands + wagon wheels), `s_cone` (silo roofs),
`s_column` (headhouse / gallery / chute / wagon / floor / windows), `s_dome` (grain piles),
and `draw_cottage` (the barn).
1. **Silo cluster** (`draw_silo`) — a tall hoop-banded cylinder with a conical roof + a chute
   vent; five of them (a central great silo = the landmark obstacle + four around it).
2. **Headhouse** — a tall work-tower with windows + a metal cap at the back (obstacle).
3. **Gallery & chute** — an elevated conveyor gallery box along the silo tops and a loading
   chute dropping to a grain wagon (a box on four wheels; obstacle) in the yard.
4. **Yard** — a dusty slab, eight golden grain piles, and a weathered red barn (obstacle).
5. **Atmosphere** — additive drifting grain-dust motes + warm glow.

## Encounter (mobs.cpp)
- **8 "Husked"** — the `Mob` horde, level-aware spawn set (`silo_defs`), in the yard around
  the silos. Same lightweight AI, lock-on, aggregate HUD bar (named *The Husked* — also a nod
  to the game's "Husk" enemies), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–76 and the `Boss` are untouched. `mob_level` now covers
  `… || CANAL || SILO`. **No new global struct/vector** — the silo cluster, headhouse, gallery,
  wagon and barn are all fixed; grain piles are a seeded loop; only `s_wisps` reused. Warm
  lights built in `build_silo` (so `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_silo_yard` lays the dusty yard.
- CLI: `silo` / `silos` / `grain` launches straight into it.
