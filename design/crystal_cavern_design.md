# Crystal Cavern — Level Design (LEVEL_GEODE = 12)

A dark underground cavern of colossal glowing crystal formations, mirrored in a
still cool pool. The ninth **non-boss** level: crystal-encrusted husks
("The Petrified") lurk among the shards. Clear them all to win; the bonfire then
ignites as usual.

New procedural geometry, not a recolour — a **faceted-mineral** silhouette
(sharp prismatic crystal clusters) distinct from every other level (and from the
small red gem clusters of levels 0–2, which are tiny `DrawCubeV` shards — here the
crystals are colossal lit hex cones forming the environment). Adds a new reusable
lit primitive `s_cone` (`GenMeshCone(1,1,6)` = hex cone).

## Theme & palette
- **Mood:** hushed, dark, luminous — the only light is the crystals themselves.
- **Light (render.cpp):** faint cool key, deep amethyst ambient, near-black violet
  cave haze (`~#171026`, density `0.0130`). Reuses `sky_storm.fs`; the floor is the
  reused **cool reflective `water_ice.fs`** pool so the crystals mirror.
- **Glow:** amethyst point lights `(0.62, 0.46, 0.96)` from the clusters pool on
  the water and light the rock + fighters.

## Geometry (arena.cpp :: build_geode / draw_geode)
Reuses `s_dome` (dark rocky mounds) and the new `s_cone` (crystal shards).
1. **Crystal clusters** — `add_cluster` fans 4–7 faceted shards (tilted lit hex
   cones) from a base, each tinted amethyst / magenta / cyan with an additive tip
   glow. A giant central geode (landmark + obstacle), ~10 floor clusters jutting
   from dark rocky mounds, and 6 **hanging ceiling clusters** (flipped to point
   down, like crystalline stalactites high overhead).
2. **Rocky mounds** — dark lit hemispheres the floor crystals grow from, plus a
   base mound under the central geode.

## Encounter (mobs.cpp)
- **7 "Petrified"** — the `Mob` horde, level-aware spawn set (`geode_defs`), among
  the shards. Same lightweight AI, lock-on, aggregate HUD bar (named *The
  Petrified*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–11 and the `Boss` are untouched. `mob_level` now covers
  `GRAVEYARD || FUNGAL || DESERT || WRECK || SANCTUM || CLOCK || SHRINE || BONES ||
  GEODE`. Functions named `build_geode`/`draw_geode` to avoid colliding with the
  existing `build_crystals`/`draw_crystals` (the small gem system).
- CLI: `crystal` / `cavern` / `geode` launches straight into it.
