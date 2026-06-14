# Plague Hamlet — Level Design (LEVEL_HAMLET = 29)

An abandoned plague-stricken village: pitched-roof timber cottages huddled around a
stone well, overlooked by a steepled chapel. The twenty-sixth **non-boss** level —
the plague-dead ("The Afflicted") shamble the lanes. Clear them all to win; the
bonfire then ignites as usual.

New procedural geometry, not a recolour — a **village** silhouette (clustered
A-frame cottages + a pointed chapel steeple + a roofed well) found in none of the
other twenty-nine levels.

## Theme & palette
- **Mood:** hushed, sickly, abandoned — sallow overcast light and a green-grey
  miasma hanging between the houses; the only warm points are guttering
  plague-lanterns.
- **Light (render.cpp):** low overcast sun, sallow greenish daylight
  `(0.86, 0.88, 0.74)`, murky ambient, sickly green-grey miasma fog (`~#80877A`,
  density `0.0130`). Reuses `sky_storm.fs`; **dry** — no water plane (its own dirt
  slab + cobbled square is the floor).
- **Glow:** sickly plague-lantern green `(0.70, 0.82, 0.42)`.

## Geometry (arena.cpp :: build_hamlet / draw_hamlet / draw_cottage)
Reuses `s_column` (walls/roof slabs/ground), `s_cyl` (well ring / barrels),
`s_cone` (chapel spire), `s_dome` (mud puddles), `draw_bone_seg` (fence rails); the
new **pitched-roof** trick draws two lit slabs tilted ±θ about the local ridge axis
inside a yawed `rlPushMatrix` frame (same matrix-stack technique as the leaned
headstones), so cottages take light + fog at any facing.
1. **Cottages** — 9 `draw_cottage` timber houses: walls + dark corner posts + a
   recessed doorway + an A-frame roof + ridge beam + chimney, each facing roughly
   toward the square and a collision obstacle. Kept clear of the well-square (r6)
   and the player spawn aisle (`z > ctr.z + 13`).
2. **Central well** — stone ring + dark shaft + a little gabled roof on two posts +
   a bucket rope. The landmark and a central collision obstacle.
3. **Chapel** — a stone nave with a pitched roof and a tall bell tower topped by a
   tapered `s_cone` spire and an iron cross (a fixed landmark across the square;
   obstacle).
4. **Dressing** — a cobbled central square over the dirt slab, mud puddles, short
   broken fence runs (posts + `draw_bone_seg` rails) with barrels.
5. **Plague glow** — additive sickly-green lantern light at the well, chapel door,
   and several cottage doors, with drifting motes/flies (animated).

## Encounter (mobs.cpp)
- **8 "Afflicted"** — the `Mob` horde, level-aware spawn set (`hamlet_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Afflicted*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–28 and the `Boss` are untouched. `mob_level` now covers
  `… || PFOREST || HAMLET`. New `Cottage` struct + `s_cottages` vector (cleared in
  `unload`); plague lights built in `build_hamlet` (so `build_crystals`/
  `draw_crystals` early-return for this level).
- Dry level: `draw_water` early-returns; `draw_hamlet` lays the dirt/cobble floor.
- CLI: `hamlet` / `village` / `plague` launches straight into it.
