# Clockwork Vault — Level Design (LEVEL_CLOCK = 9)

A dim brass mechanism hall, deep underground, full of slowly turning machinery.
The sixth **non-boss** level: husks ground into the gears ("The Rusted") still
shamble between the cogs. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **mechanical, moving** silhouette
(giant rotating gears, a central engine core, brass pillars and pipes) found in
none of the other nine levels. Its signature is *animation*: the gears actually
turn at different speeds and directions.

## Theme & palette
- **Mood:** the groaning heart of some vast forgotten machine — warm furnace
  light, soot, hissing steam, brass and copper turning in the dark.
- **Light (render.cpp):** low warm furnace key light, dim warm metal ambient,
  sooty brown-black haze (`~#28231F`, density `0.0095`). Reuses `sky_storm.fs`
  (dark) and `water_storm.fs` as a dark oily reflective floor.
- **Glow:** warm furnace point lights `(0.98, 0.62, 0.22)` from the core + vents.

## Geometry (arena.cpp :: build_clock / draw_clock, helper draw_gear)
Reuses the lit primitives `s_cyl` (gear hubs / axle / pillars / pipes) and
`s_column` (gear teeth / spokes).
1. **Gears** — a `Gear` is a lit-cylinder hub + lit-cube teeth around the rim +
   three spokes, spinning about its own axis. `draw_gear` supports **flat** cogs
   (spin about Y, floating at varied heights) and **upright** wall-cogs (tipped
   90° so they spin about world Z). ~7 free-standing gears at varied sizes,
   speeds, and directions (`s_time * speed`).
2. **Central engine** — a tall iron axle carrying a stack of three counter-rotating
   flat gears + a glowing amber core orb. The landmark and a collision obstacle.
3. **Brass pillars** — six iron columns with copper cap fittings + a pipe stub,
   ringing the hall.
4. **Steam & embers** — additive rising steam puffs and vent ember glows; the core
   pulses.

## Encounter (mobs.cpp)
- **7 "Rusted"** — the `Mob` horde, level-aware spawn set (`clock_defs`), among the
  machinery (clear of the central core). Same lightweight AI, lock-on, aggregate
  HUD bar (named *The Rusted*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–8 and the `Boss` are untouched. `mob_level` now covers
  `GRAVEYARD || FUNGAL || DESERT || WRECK || SANCTUM || CLOCK`; the Boss stays inert.
- Keeps the reused water plane as the (dark, oily) reflective floor.
- CLI: `clock` / `vault` / `clockwork` launches straight into it.
