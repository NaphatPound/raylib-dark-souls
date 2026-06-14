# Overgrown Temple — Level Design (LEVEL_TEMPLE = 13)

A ruined jungle temple swallowed by nature, half-drowned in a jade pool. The tenth
**non-boss** level: husks bound in the overgrowth ("The Rooted") guard the ruin.
Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — an **organic-overgrowth** silhouette: a
broken stepped temple pierced and draped by colossal gnarled tree roots, with vines,
ferns and jungle trees. Distinct from every other level; reuses the bone-segment
technique (`draw_bone_seg`) to build the curving roots and hanging vines.

## Theme & palette
- **Mood:** humid, green, reclaimed — the jungle has eaten the temple.
- **Light (render.cpp):** dappled green-filtered daylight, lush green ambient,
  humid green haze (`~#527052`, density `0.0120`). Reuses `sky_storm.fs` and the
  calm `water_storm.fs` as a jade reflecting pool.
- **Glow:** warm-green firefly point lights `(0.80, 0.92, 0.45)` drifting over the
  pool and ruin.

## Geometry (arena.cpp :: build_temple / draw_temple, helper draw_root)
Reuses `s_column` (temple tiers / lintel), `s_cyl` (pillars / trunks), `s_dome`
(fern bushes / tree canopies), and `draw_bone_seg` (roots / vines / toppled columns).
1. **Ruined temple** — three broken, offset/rotated stone tiers + six pillars
   (varied heights, some leaning/broken) + a leaning doorway arch + two toppled
   columns lying in the moss. The landmark and a collision obstacle.
2. **Colossal roots** — five gnarled roots arcing in from the jungle edge toward
   the ruin (`draw_root`: a chained arc of tapering cylinder segments with a gnarl
   wiggle).
3. **Hanging vines** — thin swaying segment-chains drooping from the root arches.
4. **Foliage** — ~16 lush fern/bush domes + a few jungle trees (trunk + big green
   canopy), scattered (entrance aisle + temple kept clear).

## Encounter (mobs.cpp)
- **7 "Rooted"** — the `Mob` horde, level-aware spawn set (`temple_defs`), around the
  ruin and grounds. Same lightweight AI, lock-on, aggregate HUD bar (named *The
  Rooted*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–12 and the `Boss` are untouched. `mob_level` now covers
  `GRAVEYARD || FUNGAL || DESERT || WRECK || SANCTUM || CLOCK || SHRINE || BONES ||
  GEODE || TEMPLE`.
- Keeps the reused water plane as the jade reflecting pool.
- CLI: `temple` / `jungle` / `overgrown` launches straight into it.
