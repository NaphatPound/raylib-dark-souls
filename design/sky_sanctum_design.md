# Sky Sanctum — Level Design (LEVEL_SANCTUM = 8)

A shattered stone sanctum floating high among the clouds, adrift in an endless sky.
The fifth **non-boss** level: spectral guardians ("The Forsaken") hold the sanctum.
Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a wholly *aerial/vertical* silhouette
(a floating circular island with a tapered rocky underside, satellite islands at
different heights, arched sky-bridges, and a monument ringed by orbiting shards)
found in none of the other eight levels. It is the second **dry** level (no water):
the great platform itself is the floor, the void falls away around its edges.

## Theme & palette
- **Mood:** serene but desolate — wind, drifting stone, soul-light. You fight on a
  fragment of a ruined temple suspended in open sky.
- **Light (render.cpp):** clean cool daylight, bright sky-blue ambient, airy pale
  cloud haze (`~#A8BCDC`, low density `0.0090`, so distant islands fade into the
  sky). Reuses `sky_ice.fs` as a cold high-altitude sky.
- **Soul-light:** pale blue-white point lights `(0.80, 0.88, 1.00)` from drifting
  motes light the platform + fighters.
- **No water / no moon:** `draw_water` early-returns; the floating platform is the
  solid floor.

## Geometry (arena.cpp :: build_sanctum / draw_sanctum, helper draw_island)
Reuses the lit primitives `s_cyl` (island discs / pedestal) and `s_column`
(bridges / spire / shards / debris).
1. **Main platform** — a great circular disc (the play floor) with a tapered,
   shrinking-disc rocky underside (the classic floating-island cone).
2. **Satellite islands** — ~7 smaller discs (each with its own tapered underside)
   drifting at various heights and tilts around the platform.
3. **Arched sky-bridges** — two catenary plank arches reaching off the rim.
4. **Central monument** — a stepped pedestal + tall spire + a glowing orb, ringed
   by **8 slowly orbiting stone shards** (animated via `s_time` — unique to this
   level). The monument is a collision obstacle.
5. **Drifting debris + soul-motes + cloud band** — small bobbing/spinning shards
   over the platform, additive blue-white motes, and a faint cloud band far below
   the void for aerial depth.

## Encounter (mobs.cpp)
- **7 "Forsaken"** — the `Mob` horde, level-aware spawn set (`sanctum_defs`),
  ringing the monument (clear of its obstacle). Same lightweight AI, lock-on,
  aggregate HUD bar (named *The Forsaken*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–7 and the `Boss` are untouched. `mob_level` now covers
  `GRAVEYARD || FUNGAL || DESERT || WRECK || SANCTUM`; the Boss object stays inert.
- The level draw is `draw_sanctum` (renamed to avoid clashing with the existing
  `draw_sky(Camera3D)` background-dome function).
- CLI: `sky` / `sanctum` launches straight into it.
