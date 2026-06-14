# The Watermill — Level Design (LEVEL_WATERMILL = 45)

A pastoral mill-yard built along a millrace: a stone-and-timber millhouse turning a
great paddled **water-wheel** that dips into the rushing channel, a weir, a plank
footbridge, a central millstone display, grain sacks and reed-fringed banks. The
forty-second **non-boss** level — the mill's drowned dead ("The Sodden") rise from
the race. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **watermill** silhouette (a turning
paddled water-wheel on a millhouse + a millrace + a weir) found in none of the other
forty-five levels. Distinct from the Windmill Fields (rotating sails on farmland) and
the Timber Sawmill (a circular blade in a timber yard).

## Theme & palette
- **Mood:** calm, rural, working — soft overcast daylight over a green river-yard, the
  steady creak-and-splash of the turning wheel.
- **Light (render.cpp):** soft overcast sun, warm pastoral light `(1.00, 0.96, 0.84)`,
  gentle ambient, soft river haze (`~#9EA899`, density `0.0085`). Reuses
  `sky_storm.fs`; **dry** — its own mill-yard is the floor (the millrace is a recessed
  teal water strip, not the reused water plane).
- **Glow:** warm pastoral lantern `(1.00, 0.86, 0.56)`.

## Geometry (arena.cpp :: build_watermill / draw_watermill / draw_waterwheel)
Reuses `s_torus` (wheel rims), `s_cyl` (hub / millstones / bridge piers / barrels),
`s_column` (yard / race / mill house / weir / bridge), `s_dome` (grass / sacks),
`draw_bone_seg` (spokes / wheel posts / reeds), and **`draw_cottage`** (the mill house).
1. **Water-wheel** — `draw_waterwheel`: two lit `s_torus` rims joined by 8 spokes and a
   ring of 12 paddle boards, **spun about its axle by `s_time`**, mounted on posts so
   its lower paddles dip into the race. The animated signature.
2. **Mill house** — a scaled-up `draw_cottage` in stone with a pitched roof; the
   landmark obstacle.
3. **Millrace** — a recessed teal water strip with stone kerbs running the yard, a
   **weir** sluice upstream, and a plank **footbridge** (piers + deck + handrails)
   crossing it.
4. **Centre** — a stacked **millstone** display on a spindle (the central obstacle).
5. **Yard** — grass banks, grain sacks + barrels, reed clumps along the race.
6. **Water FX** — additive flowing-race shimmer, falling water at the weir, wheel
   splash, and warm lantern glow.

## Encounter (mobs.cpp)
- **8 "Sodden"** — the `Mob` horde, level-aware spawn set (`watermill_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Sodden*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–44 and the `Boss` are untouched. `mob_level` now covers
  `… || NAVE || WATERMILL`. **No new global struct/vector** — mill/wheel/bridge at
  fixed positions; only `s_wisps` reused. Lantern lights built in `build_watermill` (so
  `build_crystals`/`draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_watermill` lays the mill-yard floor (the
  race is its own teal strip + additive flow).
- CLI: `watermill` / `millpond` / `waterwheel` launches straight into it.
