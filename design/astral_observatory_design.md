# Astral Observatory — Level Design (LEVEL_OBS = 15)

A moonlit stargazer's platform crowned by a great armillary sphere. The twelfth
**non-boss** level: stargazers turned husk ("The Watchers") still keep their vigil.
Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **celestial-mechanism** silhouette
(a giant gyroscopic armillary of concentric rings spinning on different axes, with
orbiting planets, telescopes, and globes) found in none of the other fifteen levels.
Adds a new reusable lit primitive `s_torus` (`GenMeshTorus`). Its motion is unlike
the Sanctum's orbiting shards or the Clock's flat gears: concentric rings each
spinning about its own tilted axis.

## Theme & palette
- **Mood:** serene, cosmic, watchful — moon, stars, slow turning brass.
- **Light (render.cpp):** cool moonlight, night-blue ambient, deep night-blue haze
  (`~#171C33`, density `0.0090` — clear so the stars read). Reuses `sky.fs` + a pale
  moon disc; the floor is the reused reflective `water.fs`.
- **Glow:** warm sun-orb / globe point lights `(0.92, 0.86, 0.66)`.

## Geometry (arena.cpp :: build_obs / draw_obs)
Reuses `s_cyl` (axle / telescope tubes & legs / pedestals) and the new `s_torus`
(armillary rings + planet rings); planets/sun/stars are `DrawSphereEx`.
1. **Armillary sphere** — a plinth + central axle + four concentric lit tori at
   different tilts, each spinning about its own axis (`s_time`), around a glowing
   additive **sun-orb** with **three planets** orbiting on the ecliptic. The
   landmark and a collision obstacle.
2. **Telescopes** — four on splayed tripods, tubes tilted skyward with an aperture.
3. **Celestial globes** — five pedestals topped by coloured globes, some with a
   brass ring (ringed planets).
4. **Starfield** — ~80 deterministic twinkling additive stars high in the dome,
   under the pale moon.

## Encounter (mobs.cpp)
- **7 "Watchers"** — the `Mob` horde, level-aware spawn set (`obs_defs`), around the
  armillary. Same lightweight AI, lock-on, aggregate HUD bar (named *The Watchers*),
  and victory-on-clear.

## Integration notes
- Purely additive: levels 0–14 and the `Boss` are untouched. `mob_level` now covers
  `… || MINE || OBS`. `LEVEL_OBS` added to the moon-draw set (pale moon).
- Keeps the reused water plane as the reflective floor.
- CLI: `observatory` / `orrery` / `astral` launches straight into it.
