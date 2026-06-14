# Abandoned Mine — Level Design (LEVEL_MINE = 14)

A dark, flooded mineshaft long abandoned. The eleventh **non-boss** level: husks
still working the dead seam ("The Delvers") shamble among the timbers. Clear them
all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **rustic-industrial** silhouette
(receding timber support frames, mine-cart rails + carts, a pit-head winch tower)
distinct from every other level, including the brass Clockwork Vault.

## Theme & palette
- **Mood:** cramped, dark, derelict — dripping timbers and lantern-light over black
  flood-water, a seam of gold ore the only colour.
- **Light (render.cpp):** dim warm lantern key, dim earthy ambient, near-black dust
  haze (`~#1C1917`, density `0.0110`). Reuses `sky_storm.fs` and the dark
  `water_storm.fs` as the flooded floor.
- **Glow:** gold ore / lantern point lights `(0.98, 0.70, 0.30)`.

## Geometry (arena.cpp :: build_mine / draw_mine, helpers draw_frame / draw_cart)
Reuses `s_cyl` (posts/wheels/pulley/scaffold legs), `s_column` (beams/rails/ties/
cart bodies/platforms), `s_dome` (ore-rock mounds), `s_cone` (gold ore shards), and
`draw_bone_seg` (leaning winch-tower posts).
1. **Timber support frames** — `draw_frame` (two round posts + a top beam + angled
   braces). A receding avenue of six down the shaft + three scattered side frames.
2. **Pit-head winch tower** — four inward-leaning posts + a platform + a pulley
   wheel over a dark pit rim. The landmark and a collision obstacle.
3. **Rail tracks** — two `draw_track` runs (twin iron rails + wooden cross-ties).
4. **Ore carts** — `draw_cart` (box body on four wheels); three upright, one tipped.
5. **Scaffolds** — two plank platforms on posts.
6. **Ore veins** — dark rock mounds studded with small glowing gold crystal shards.

## Encounter (mobs.cpp)
- **7 "Delvers"** — the `Mob` horde, level-aware spawn set (`mine_defs`), among the
  timbers. Same lightweight AI, lock-on, aggregate HUD bar (named *The Delvers*),
  and victory-on-clear.

## Integration notes
- Purely additive: levels 0–13 and the `Boss` are untouched. `mob_level` now covers
  `… || TEMPLE || MINE`.
- Keeps the reused water plane as the dark flood-water floor.
- CLI: `mine` / `mineshaft` / `dig` launches straight into it.
