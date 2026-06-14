# Hanging Gardens — Level Design (LEVEL_GARDEN = 20)

A lush, sunlit formal water-garden. The seventeenth **non-boss** level: gardeners
gone to seed ("The Withered") still tend the beds. Clear them all to win; the
bonfire then ignites as usual.

New procedural geometry, not a recolour — an **ornamental-garden** silhouette
(tiered fountain, formal flower-beds, trellis rose-walk, cypress topiary) found in
none of the other twenty levels. Deliberately the most **colourful** level — a warm,
blooming contrast to the dark/grey levels around it.

## Theme & palette
- **Mood:** serene, bright, overgrown-genteel — petals on the breeze, fountains
  trickling, blooms everywhere.
- **Light (render.cpp):** warm golden-hour sun, soft bright ambient, pale airy haze
  (`~#9EA899`, low density `0.0055` — clear and sunny). Reuses the cool `sky_ice.fs`
  as a pale sky and the cool `water_ice.fs` as the reflecting pool.
- **Glow:** warm sunlit fill `(0.92, 0.82, 0.60)`.

## Geometry (arena.cpp :: build_garden / draw_garden)
Reuses `s_cyl` (fountain basins / posts), `s_dome` (flower bushes / roses),
`s_cone` (cypress), `s_column` (bed borders / trellis / hedges).
1. **Central tiered fountain** — a dais + stacked basins + jet pillar. The landmark
   and a collision obstacle.
2. **Four quadrant flower-beds** — thin low stone borders enclosing clusters of
   coloured bloom-domes (six flower colours).
3. **Trellis rose-walk** — four arches along the entrance, dripping pink roses.
4. **Cypress topiary ring** — eight tall dark-green cones around the perimeter.
5. **Hedge border** — a low green ring framing the garden.
6. **Fountain jets + drifting petals** — additive droplets and ~16 floating petals.

## Encounter (mobs.cpp)
- **7 "Withered"** — the `Mob` horde, level-aware spawn set (`garden_defs`), among
  the beds. Same lightweight AI, lock-on, aggregate HUD bar (named *The Withered*),
  and victory-on-clear.

## Integration notes
- Purely additive: levels 0–19 and the `Boss` are untouched. `mob_level` now covers
  `… || COURT || GARDEN`.
- Keeps the reused water plane as the cool reflecting pool.
- CLI: `garden` / `gardens` / `bloom` launches straight into it.
