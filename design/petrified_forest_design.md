# Petrified Forest — Level Design (LEVEL_PFOREST = 28)

A dead grove turned to stone: gnarled, fossilised trunks frozen mid-writhe over a
floor of grey ash. The twenty-fifth **non-boss** level — those the forest calcified
("The Calcified") shamble between the trunks. Clear them all to win; the bonfire then
ignites as usual.

New procedural geometry, not a recolour — a **petrified-woodland** silhouette
(twisted stone trees + a colossal fallen log + broken stumps) found in none of the
other twenty-eight levels.

## Theme & palette
- **Mood:** silent, fossilised, choking — drained overcast light through hanging
  ash dust, warm pinpoints of trapped amber resin the only colour.
- **Light (render.cpp):** flat overcast ash-light, pale drained sun
  `(0.94, 0.90, 0.82)`, dim grey ambient, hanging ash-dust fog (`~#8F8A80`,
  density `0.0125`). Reuses `sky_storm.fs`; **dry** — no water plane (its own
  ashen slab is the floor).
- **Glow:** amber resin `(0.90, 0.60, 0.30)`.

## Geometry (arena.cpp :: build_pforest / draw_pforest / draw_ptree)
Reuses `draw_bone_seg` (twisted trunks, branches, fallen-log, roots), `s_column`
(ground slab), `s_dome` (ash mounds / root-ball), `s_cyl` (broken stumps).
1. **Petrified trees** — 15 standing `draw_ptree` trunks: a chained `draw_bone_seg`
   spine that wiggles sinuously and tapers toward the crown, forking into stubby
   stone branches; shaped deterministically from a per-tree seed. Each trunk is a
   collision obstacle and is kept clear of the centre arena (r4) and the player
   spawn aisle (`z > ctr.z + 13`).
2. **Colossal fallen log** — a single fat `draw_bone_seg` lying across the centre,
   with a torn root-ball (`s_dome` + a fan of root `draw_bone_seg` twigs). The
   landmark and a central collision obstacle.
3. **Broken stumps** — six snapped `s_cyl` boles scattered in the mid-field.
4. **Ashen ground** — a wide `s_column` slab + scattered `s_dome` ash mounds.
5. **Amber resin** — additive glow pooled at marked tree bases and along the log,
   with slow drifting motes (animated).

## Encounter (mobs.cpp)
- **8 "Calcified"** — the `Mob` horde, level-aware spawn set (`pforest_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Calcified*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–27 and the `Boss` are untouched. `mob_level` now covers
  `… || SPRINGS || PFOREST`. New `PTree` struct + `s_ptrees` vector (cleared in
  `unload`); amber lights built in `build_pforest` (so `build_crystals`/
  `draw_crystals` early-return for this level).
- Dry level: `draw_water` early-returns; `draw_pforest` lays the ash floor.
- CLI: `petrified` / `stoneforest` / `woods` launches straight into it.
