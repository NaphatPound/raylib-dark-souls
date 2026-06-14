# Druid Henge — Level Design (LEVEL_HENGE = 30)

A megalithic stone circle on a misty moor: a great outer ring of sarsens (many
capped with lintels) enclosing an inner horseshoe of towering trilithons around a
central altar. The twenty-seventh **non-boss** level — the cult bound to the stones
("The Devout") rise to defend them. Clear them all to win; the bonfire then ignites
as usual.

New procedural geometry, not a recolour — a **megalithic henge** silhouette
(trilithons = paired uprights bridged by a lintel, an inner horseshoe, an altar
stone) found in none of the other thirty levels.

## Theme & palette
- **Mood:** ancient, hushed, charged — pale blue dusk over a moorland mist, the
  stones lit from below by a cold ley-line energy at the altar.
- **Light (render.cpp):** low cold dusk sun, pale blue twilight `(0.74, 0.80, 0.92)`,
  cool blue ambient, moorland mist (`~#6B7A8F`, density `0.0115`). Reuses
  `sky_ice.fs`; **dry** — its own heath turf is the floor.
- **Glow:** pale ley-line cyan `(0.45, 0.85, 0.92)`.

## Geometry (arena.cpp :: build_henge / draw_henge / draw_megalith)
Reuses `s_column` (every stone + lintel + altar/ground), `s_dome` (barrow mounds);
the new helper `draw_megalith` roots a lit stone box at the ground and, for
leaning/fallen stones, tilts it via the rlgl matrix so it still takes light + fog.
1. **Outer sarsen ring** — 12 uprights on R≈16 (the spawn aisle left open), most
   capped with a lintel (read as the continuous lintelled ring); a couple lean.
   Each upright is a collision obstacle.
2. **Inner horseshoe** — 5 great **trilithons** (taller paired uprights + capstone)
   arced across the far side, opening toward the player; obstacles.
3. **Central altar** — a flat slab on two low plinths; the ritual focus and a
   central obstacle.
4. **Outliers** — a leaning heel stone outside the ring and a couple of toppled
   sarsens lying in the grass (tilted via the matrix).
5. **Heath** — a turf ground slab + low grassy barrow mounds.
6. **Ley energy** — a soft additive vertical beam rising from the altar, ground-rune
   glow at several stone feet, and drifting motes (all pulsing with `s_time`).

## Encounter (mobs.cpp)
- **8 "Devout"** — the `Mob` horde, level-aware spawn set (`henge_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Devout*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–29 and the `Boss` are untouched. `mob_level` now covers
  `… || HAMLET || HENGE`. New `Sarsen` struct + `s_sarsens` vector (cleared in
  `unload`); ley lights built in `build_henge` (so `build_crystals`/`draw_crystals`
  early-return for this level).
- Dry level: `draw_water` early-returns; `draw_henge` lays the heath floor.
- CLI: `henge` / `stonehenge` / `circle` launches straight into it.
