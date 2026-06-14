# The Redwood Grove — Level Design (LEVEL_REDWOOD = 74)

A cathedral of colossal ancient sequoias: enormous reddish trunks with flared, buttressed
bases rise out of sight into a green mist, dappled god-ray light shafts slant down through
the canopy onto a mossy floor of ferns, and a giant fallen log with a torn root-ball lies
across the grove. The seventy-first **non-boss** level — the grove's lost dead ("The
Mossgrown") rise among the trunks. Clear them all to win; the bonfire then ignites.

New procedural geometry, not a recolour — a **giant-redwood-grove** silhouette (a handful
of colossal thick trunks rising into fog) found in none of the other seventy-four levels.
A **DRY** level (mossy floor). Deliberately distinct from the Archtree (a *single* tree with
a visible spreading canopy), Snowbound Pines (small snow-laden conifers), and the Petrified
Forest (many *thin, dead, twisted* stone trees): these are a few *living, colossal* columns.

## Theme & palette
- **Mood:** hushed, ancient, awed — soft filtered light in a vast green nave of trees.
- **Light (render.cpp):** soft light from high above `(0.34,0.84,0.30)`, green-gold filtered
  daylight `(0.96,0.98,0.78)`, cool green shade, misty green-grey haze (`~#8FA38A`, density
  `0.0092`). Reuses `sky_ice.fs`; **DRY** (`draw_redwood_grove` lays the mossy floor).
- **Glow:** soft green-gold grove light `(0.80,0.95,0.60)`.

## Geometry (arena.cpp :: draw_redwood / redwood_layout / build_redwood / draw_redwood_grove)
Reuses the lit `s_cyl` (root flares), `s_column` (buttress wedges / floor / fire-hollow),
`draw_bone_seg` (the tall tapering trunk segments + the fallen log), `s_dome` (moss mounds /
ferns / the torn root-ball), and immediate-mode `DrawCube` for the light shafts.
1. **Redwoods** (`draw_redwood`) — a flared root base + 5 tilted buttress wedges + a tall
   trunk of 8 stacked slightly-tapering `draw_bone_seg` segments (18–26u high). 6 colossal
   trunks (radius 2–3.2, spaced ≥7 apart) + a central great redwood with a dark fire-hollow
   (the landmark obstacle).
2. **Fallen log** — a huge horizontal `draw_bone_seg` trunk with an `s_dome` root-ball.
3. **Floor** — a mossy slab, 24 litter mounds, 16 fern clusters.
4. **Atmosphere** — additive dappled **god-ray light shafts** (slanted columns of faint
   cubes) + drifting pollen motes + soft glow.

## Encounter (mobs.cpp)
- **8 "Mossgrown"** — the `Mob` horde, level-aware spawn set (`redwood_defs`), among the
  trunks. Same lightweight AI, lock-on, aggregate HUD bar (named *The Mossgrown*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–73 and the `Boss` are untouched. `mob_level` now covers
  `… || WHEEL || REDWOOD`. **No new global struct/vector** — `redwood_layout` fills a local
  `std::vector<Vector4>` (x, z, height, radius) shared by build (obstacles) + draw; the
  fallen log fixed; only `s_wisps` reused. Grove lights built in `build_redwood` (so
  `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_redwood_grove` lays the mossy floor.
- CLI: `redwood` / `redwoods` / `sequoia` launches straight into it (avoids `grove`, which is
  the Bamboo Grove's alias).
