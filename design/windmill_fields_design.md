# Windmill Fields — Level Design (LEVEL_MILL = 23)

A windswept farmland of old windmills. The twentieth **non-boss** level: field-hands
turned husk ("The Reaped") still walk the rows. Clear them all to win; the bonfire
then ignites as usual.

New procedural geometry, not a recolour — a **pastoral-farmland** silhouette with
**turning windmill sails** (animated), found in none of the other twenty-three levels.
The seventh **dry** level (a grass field).

## Theme & palette
- **Mood:** breezy, rustic, faded harvest — turning sails, hay in the fields, a
  lonely barn under a grey sky.
- **Light (render.cpp):** soft warm-neutral overcast, soft outdoor ambient, pale
  windy haze (`~#9A9E8E`, low density `0.0060`). Reuses `sky_storm.fs`; the floor is
  a solid grass slab (dry).
- **Glow:** warm barn-window light `(0.95, 0.85, 0.58)`.

## Geometry (arena.cpp :: build_mill / draw_mill, helper draw_windmill)
Reuses `s_cyl` (mill towers / hay bales / fence posts / scarecrow posts), `s_cone`
(mill caps / haystacks / wheat / scarecrow... ), `s_column` (sail arms / barn /
fence rails / door), `s_dome` (mounds / sack heads).
1. **Windmills** — `draw_windmill` builds a tapered tower + cone cap + an axle + a
   **rotating 4-sail cross** (arms + lattice sail panels) spun by `s_time`. A big
   central one (obstacle) + three scattered, varied size and spin direction.
2. **Barn** — a gable-roofed body + door.
3. **Split-rail fences** — three runs of posts + rails marking the fields.
4. **Hay** — round bales (cylinders on their side) and conical haystacks.
5. **Scarecrows** — post + cross-arm + sack head.
6. **Wheat tufts** — scattered golden cones; rolling grass mounds.

## Encounter (mobs.cpp)
- **7 "Reaped"** — the `Mob` horde, level-aware spawn set (`mill_defs`), across the
  fields. Same lightweight AI, lock-on, aggregate HUD bar (named *The Reaped*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–22 and the `Boss` are untouched. `mob_level` now covers
  `… || BEACON || MILL`.
- CLI: `mill` / `windmill` / `fields` launches straight into it.
