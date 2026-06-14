# The Fallen Titan — Level Design (LEVEL_TITAN = 63)

An Ozymandias ruin: a colossal shattered statue strewn across a desolate dusk plain —
a giant stone **hand** reaching up out of the earth, a toppled **head** with a broken
laurel crown, a shattered **torso**, fallen limb-columns and scattered rubble, under a
warm hazy sunset. The sixtieth **non-boss** level — the fallen god's last faithful
("The Forgotten") rise to defend the wreck. Clear them all to win; the bonfire ignites.

New procedural geometry, not a recolour — a **toppled-colossus** silhouette (reaching
hand + fallen head + broken torso) found in none of the other sixty-three levels. A
**DRY** level (barren cracked plain). Deliberately distinct from the Sentinel Court
(rows of standing, intact guardian statues): here a single titan lies shattered in ruin.

## Theme & palette
- **Mood:** melancholic, grand, desolate — "lone and level sands stretch far away."
- **Light (render.cpp):** low warm raking dusk sun `(0.42,0.58,0.30)`, warm amber light
  `(1.04,0.84,0.58)`, dusty warm-grey fill, dusty haze (`~#998A6B`, density `0.0090`).
  Reuses `sky_forge.fs`; **DRY** (`draw_titan` lays the barren plain).
- **Glow:** warm dusty amber `(0.88,0.74,0.52)`.

## Geometry (arena.cpp :: draw_giant_hand / draw_giant_head / draw_titan_torso / build_titan / draw_titan)
Reuses the lit `s_column` (palm/fingers/face/torso slabs/floor/cracks), `s_cyl`
(forearm/rubble drums), `s_dome` (cranium via two-hemisphere trick/eyes/boulders),
`s_cone` (nose/laurel points), and `draw_bone_seg` (fallen limb-columns).
1. **The reaching hand** (`draw_giant_hand`) — a half-buried forearm + palm slab + four
   standing curled fingers + a thumb (all in one rlgl frame); the central landmark obstacle.
2. **The toppled head** (`draw_giant_head`) — a cracked cranium + face block, brow ridge,
   dark eye sockets, a nose wedge, and a broken laurel crown of `s_cone` points (obstacle).
3. **The shattered torso** (`draw_titan_torso`) — a chest block + broken neck stump + two
   toga-fold slabs (obstacle).
4. **Debris** — two fallen limb-columns (`draw_bone_seg`), 6 rubble drums (obstacles) +
   10 boulders, and a network of dark ground cracks.
5. **Atmosphere** — additive dusty warm motes + a field of drifting sand.

## Encounter (mobs.cpp)
- **8 "Forgotten"** — the `Mob` horde, level-aware spawn set (`titan_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Forgotten*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–62 and the `Boss` are untouched. `mob_level` now covers
  `… || CRATER || TITAN`. **No new global struct/vector** — landmarks are fixed; rubble is
  a deterministic seeded loop (seed 9100) shared by build (obstacles) + draw; only
  `s_wisps` reused. Dusty lights built in `build_titan` (so `build_crystals`/`draw_crystals`
  early-return). Enum is `LEVEL_TITAN` (not `COLOSSUS`) to avoid confusion with the existing
  `LEVEL_COLOSSEUM`/`draw_colosseum`.
- DRY level: `draw_water` early-returns; `draw_titan` lays the barren plain.
- CLI: `titan` / `colossus` / `giant` launches straight into it.
