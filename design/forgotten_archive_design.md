# Forgotten Archive — Level Design (LEVEL_LIB = 16)

A vast abandoned library, its shelves still heavy with mouldering books. The
thirteenth **non-boss** level: husks bound to the dead archive ("The Scholars")
shuffle the aisles. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — an **interior-archive** silhouette
(a grid of towering bookshelves forming aisles, filled with thousands of coloured
book-spines) found in none of the other sixteen levels. The fourth **dry** level
(a wooden floor, no water).

## Theme & palette
- **Mood:** hushed, dusty, scholarly decay — candlelight, drifting motes, leaning
  stacks of forgotten knowledge.
- **Light (render.cpp):** warm candle-lit parchment key, dim dusty ambient, dusty
  brown haze (`~#383329`, density `0.0100`). Reuses `sky_storm.fs`; the floor is a
  solid wood plank slab (dry).
- **Glow:** warm candlelight point lights `(0.98, 0.78, 0.46)`.

## Geometry (arena.cpp :: build_lib / draw_lib, helpers draw_shelf / draw_lectern)
Reuses `s_column` (shelves / books / walls / desks) and `s_cyl` (posts / candelabra).
1. **Bookshelves** — `draw_shelf` builds a frame (back + sides + top/bottom) with
   four shelf boards, each packed with a deterministic row of coloured book-spines
   (varied width/height/colour/tilt). Laid out in a grid of aisles; a few lean.
2. **Central reading rotunda** — a dais + ring of pillars + a pillar of stacked
   giant tomes. The landmark and a collision obstacle.
3. **Lecterns** — `draw_lectern` (base + post + slanted desk + open parchment
   pages), scattered.
4. **Candelabra** — five tall stands with three candle flames each (the warm lights).
5. **Drifting spectral tomes** — six glowing books bobbing through the air.
6. **Broken high walls** — a ruined stone wall ring suggesting the hall interior
   (the fog hides the missing ceiling).

## Encounter (mobs.cpp)
- **7 "Scholars"** — the `Mob` horde, level-aware spawn set (`lib_defs`), among the
  stacks. Same lightweight AI, lock-on, aggregate HUD bar (named *The Scholars*),
  and victory-on-clear.

## Integration notes
- Purely additive: levels 0–15 and the `Boss` are untouched. `mob_level` now covers
  `… || OBS || LIB`.
- CLI: `library` / `archive` / `lib` launches straight into it.
