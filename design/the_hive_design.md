# The Hive — Level Design (LEVEL_HIVE = 48)

The cavernous interior of a colossal hornet hive: a great papery teardrop nest as the
queen's chamber, walls of true **hexagonal honeycomb cells**, hanging larval combs,
chitinous drone mounds, and a dense wasp swarm — all lit by a dim amber-wax glow. The
forty-fifth **non-boss** level — the hive's chitinous dead ("The Hivebound") rise from
the comb. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **hive interior** silhouette (a layered
paper nest + hexagonal honeycomb clusters + a swarm) found in none of the other
forty-eight levels. Distinct from the Apiary (a sunny outdoor bee-yard of box hives
and skeps) — this is the dark nest within.

## Theme & palette
- **Mood:** claustrophobic, resinous, droning — warm amber gloom, comb dripping, the
  constant shimmer of a wasp cloud.
- **Light (render.cpp):** dim filtered glow, warm amber light `(0.90, 0.66, 0.36)`,
  dim warm-brown ambient, dark waxy haze (`~#42301E`, density `0.0120`). Reuses
  `sky_storm.fs`; **dry** — its own waxy floor is the floor.
- **Glow:** amber wax `(1.00, 0.65, 0.25)`.

## Geometry (arena.cpp :: build_hive / draw_hive / draw_hexcell / draw_comb_cluster / draw_papernest / hive_layout)
Reuses `s_column` (floor / hex wall panels), `s_cone` (cell floors), `s_cyl` (nest
bands), `s_dome` (larval combs / mounds / wax patches), `draw_bone_seg` (comb cords).
1. **Honeycomb cells** — `draw_hexcell`: six lit wall panels arranged tangentially
   into a true hexagon tube with a dark recessed floor; `draw_comb_cluster` rings a
   centre cell with six neighbours into the classic comb pattern. 5 seeded clusters,
   each a collision obstacle.
2. **Great paper nest** — `draw_papernest`: a bulbous layered teardrop of staggered
   paper bands with a dark entrance hole; the central landmark obstacle.
3. **Hanging larval combs** — golden `s_dome` combs drooping on cords from above;
   chitinous drone **mounds** on the floor.
4. **Wasp swarm** — a dense additive cloud of fast-jittering amber motes (a `swarm`
   lambda) around the nest and every comb cluster — the signature — plus amber glow.

## Encounter (mobs.cpp)
- **8 "Hivebound"** — the `Mob` horde, level-aware spawn set (`hive_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Hivebound*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–47 and the `Boss` are untouched. `mob_level` now covers
  `… || STILT || HIVE`. **No new global struct/vector** — `hive_layout` fills a local
  `std::vector<Vector4>` (combs: xyz + w=scale) shared by build+draw; only `s_wisps`
  reused. Amber lights built in `build_hive` (so `build_crystals`/`draw_crystals`
  early-return).
- Dry level: `draw_water` early-returns; `draw_hive` lays the waxy floor.
- CLI: `hive` / `honeycomb` / `nest` launches straight into it.
