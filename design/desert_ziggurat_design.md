# Desert Ziggurat — Level Design (LEVEL_DESERT = 6)

A sun-scorched ruin under a warm dusk sky. The third **non-boss** level: a pack of
desiccated husks ("The Buried Dead") rises from the sand. Clear them all to win;
the bonfire then ignites as usual.

New procedural geometry, not a recolour — a solid stepped ziggurat, tilted
half-fallen obelisks, broken column drums, and a toppled colossus. And it is the
first **dry** level: it skips the water plane entirely and lays down its own solid
sand floor (the other five flooded levels use the reflective water plane as ground).

## Theme & palette
- **Mood:** still, baking desert dusk over a forgotten necropolis-city.
- **Light (render.cpp):** low raking amber sun, warm sand-bounce ambient, sandy
  dusk haze (`~#9A7D57`, low density `0.0062` — clear desert air). Reuses the warm
  `sky_forge.fs` dome as a dusk sky.
- **Braziers:** warm torchlight `(0.95, 0.66, 0.30)` point lights at the ziggurat
  corners light the nearby ruins + fighters.
- **No water / no moon:** `draw_water` early-returns for this level; the floor is a
  solid lit sand slab + low dune mounds.

## Geometry (arena.cpp :: build_desert / draw_desert)
1. **Central stepped ziggurat** — 5 solid lit-cube tiers + a capstone shrine.
   The landmark and a collision obstacle. Deliberately SOLID, unlike the
   Colosseum's hollow concentric wall rings.
2. **Tilted obelisks** — ~9 tall tapered shafts with pyramidion caps, leaned at
   random angles (half-fallen), scattered (entrance aisle kept clear).
3. **Broken column drums** — short stacks (1–3) of lit-cylinder drums with slight
   leans, suggesting collapsed colonnades.
4. **Toppled colossus** — two large leaning slabs + a half-buried dome "head"
   near the back, hinting at a fallen giant statue.
5. **Sand floor + dune mounds** — a wide lit slab (top at y=0) with flattened
   wide hemisphere dunes for relief.

## Encounter (mobs.cpp)
- **7 "Buried Dead"** — the `Mob` horde, level-aware spawn set (`desert_defs`),
  spread wide around the ziggurat (clear of its large obstacle). Same lightweight
  AI, lock-on, aggregate HUD bar (named *The Buried Dead*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–5 and the `Boss` are untouched. `mob_level` now covers
  `GRAVEYARD || FUNGAL || DESERT`; the Boss object stays inert here.
- Reuses the lit primitives `s_column`/`s_cyl`/`s_dome`.
- CLI: `desert` / `ziggurat` launches straight into it.
