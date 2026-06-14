# Ruined Throne Hall — Level Design (LEVEL_THRONE = 26)

A grand fallen royal hall. The twenty-third **non-boss** level: those who would seize
the empty throne ("The Pretenders") contest it. Clear them all to win; the bonfire
then ignites as usual.

New procedural geometry, not a recolour — a **royal-hall** composition (colonnade +
carpet + raised throne) found in none of the other twenty-six levels. The eighth
**dry** level. Reuses the Court's `draw_sentinel` (guardians) and the Observatory's
`s_torus` (chandeliers).

## Theme & palette
- **Mood:** faded majesty — dust, gold, and torchlight over a long-empty throne.
- **Light (render.cpp):** dim warm gold sconce-light, dim warm regal ambient, dark
  warm haze (`~#2B2622`, density `0.0090`). Reuses `sky_storm.fs`; the floor is a
  solid stone slab (dry) with a red carpet runner.
- **Glow:** warm regal sconce light `(0.98, 0.80, 0.45)`.

## Geometry (arena.cpp :: build_throne / draw_throne, helper draw_column)
Reuses `s_column` (columns / throne / carpet / banners / dais), `s_cyl` (column
shafts / sconces / chandelier candles), `s_cone` (throne finials), `s_torus`
(chandelier rings), `draw_sentinel` (guardians), `draw_bone_seg` (chandelier chain).
1. **Colonnade** — two rows of four ornate columns (base + shaft + capital + abacus)
   flanking the central aisle.
2. **Throne** — a raised three-step dais with a great gilded throne (seat + tall back
   + arms + finials + crown). The landmark and a collision obstacle.
3. **Carpet** — a long red runner from the entrance to the dais.
4. **Guardian statues** — two `draw_sentinel` knights flanking the throne.
5. **Banners** — tattered royal banners swaying from the columns.
6. **Chandeliers** — two fallen `s_torus` rings with candle stubs + one crooked
   hanging chandelier on a chain; warm sconce-stands light the hall.

## Encounter (mobs.cpp)
- **7 "Pretenders"** — the `Mob` horde, level-aware spawn set (`throne_defs`), down
  the hall with a tougher usurper-knight before the throne. Same lightweight AI,
  lock-on, aggregate HUD bar (named *The Pretenders*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–25 and the `Boss` are untouched. `mob_level` now covers
  `… || FAIR || THRONE`. No new struct/unload (sconces use the shared `s_wisps`).
- CLI: `throne` / `hall` / `kings` launches straight into it.
