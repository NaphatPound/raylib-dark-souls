# Ossuary Catacombs — Level Design (LEVEL_OSSUARY = 24)

A candlelit, half-flooded crypt whose every wall is built of the dead. The
twenty-first **non-boss** level: the dead who would not stay ("The Interred") stir
among the bones. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **macabre-ossuary** silhouette (walls of
stacked skulls and bone, a skull reliquary, sarcophagi) found in none of the other
twenty-four levels. Distinct from the **Dragon Boneyard** (colossal dragon ribcages
on an open ash field) — this is enclosed, ordered, human-scale bone architecture.

## Theme & palette
- **Mood:** hushed, sacred, dreadful — candlelight on a thousand skulls, reflected
  in still black water.
- **Light (render.cpp):** dim warm crypt key, dim warm ambient, near-black dusty
  haze (`~#1F1C19`, density `0.0120`). Reuses `sky_storm.fs` and the dark
  `water_storm.fs` as the flooded floor (the bones mirror in it).
- **Glow:** warm candle light `(0.98, 0.72, 0.38)`.

## Geometry (arena.cpp :: build_ossuary / draw_ossuary, helper draw_bonewall)
Reuses `s_dome` (skulls / effigy heads), `s_cyl` (bone courses / candle stands /
reliquary core), `s_column` (sarcophagi), `s_cone` (urn), `draw_bone_seg` (piles).
1. **Skull-walls** — `draw_bonewall` fills a panel with rows of `s_dome` skulls
   interleaved with horizontal long-bone courses; a ring of these forms the crypt
   perimeter (gap toward the +Z entrance).
2. **Central reliquary** — a core ringed by five stacked rings of skulls, crowned
   by an urn. The landmark and a collision obstacle.
3. **Sarcophagi** — four stone coffins with lids and simple reclining effigies.
4. **Candle-stands** — five iron stands with flames (the warm lights).
5. **Bone piles** — scattered loose long bones + a skull.

## Encounter (mobs.cpp)
- **7 "Interred"** — the `Mob` horde, level-aware spawn set (`ossuary_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Interred*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–23 and the `Boss` are untouched. `mob_level` now covers
  `… || MILL || OSSUARY`.
- Keeps the reused water plane as the flooded crypt floor.
- CLI: `ossuary` / `catacombs` / `crypt` launches straight into it.
