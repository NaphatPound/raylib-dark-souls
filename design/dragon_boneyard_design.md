# Dragon Boneyard — Level Design (LEVEL_BONES = 11)

A desolate ash basin strewn with the colossal skeletons of fallen dragons. The
eighth **non-boss** level: carrion-husks ("The Carrion") pick at the great bones.
Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — an **organic skeletal** silhouette
(towering ribcages, horned skulls, scattered bones) found in none of the other
eleven levels. It introduces a new drawing technique: `draw_bone_seg` orients a lit
cylinder between two arbitrary world points, so curved ribs and horns are built from
chained segments. The third **dry** level (a pale ash bed, no water).

## Theme & palette
- **Mood:** still, deathly, vast — a graveyard of leviathans under a flat dead sky.
- **Light (render.cpp):** flat pale overcast, washed-out bone-white key, sickly
  grey-green ambient, pale deathly ash haze (`~#808A7A`, density `0.0120`). Reuses
  `sky_storm.fs`.
- **Glow:** sickly green corpse-light (marsh-gas) point lights `(0.66, 0.86, 0.40)`.
- **No water / no moon:** `draw_water` early-returns; a solid pale ash slab + low
  ash mounds form the floor.

## Geometry (arena.cpp :: build_bones / draw_bones, helpers draw_bone_seg / draw_ribcage / draw_skull)
Reuses the lit primitives `s_cyl` (bone segments), `s_dome` (crania, ash mounds),
`s_column` (snouts, jaws, ash floor).
1. **`draw_bone_seg`** — a lit cylinder oriented between two world points (axis =
   cross(up, dir), angle = acos(dot)). The building block for all curved bone.
2. **Ribcages** — a great central one (you fight within its cage) + two smaller
   scattered ones. Each: a spine bone + vertebra knobs + paired ribs that arc up
   and inward from chained segments (taller mid-cage, tapering upward).
3. **Skulls** — a great horned skull at the spine's head + a smaller one: a dome
   cranium + snout + dropped jaw + dark eye sockets + horns sweeping back (chained
   segments).
4. **Loose bones** — ~12 long bones lying at random angles across the ash.

## Encounter (mobs.cpp)
- **7 "Carrion"** — the `Mob` horde, level-aware spawn set (`bones_defs`): some
  inside the ribcage, others across the basin. Same lightweight AI, lock-on,
  aggregate HUD bar (named *The Carrion*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–10 and the `Boss` are untouched. `mob_level` now covers
  `GRAVEYARD || FUNGAL || DESERT || WRECK || SANCTUM || CLOCK || SHRINE || BONES`.
- CLI: `bones` / `boneyard` / `dragon` launches straight into it.
