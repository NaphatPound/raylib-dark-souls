# Beacon Coast — Level Design (LEVEL_BEACON = 22)

A storm-lashed rocky cape crowned by a great lighthouse. The nineteenth **non-boss**
level: castaways washed onto the rocks ("The Marooned") still haunt the surf. Clear
them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **coastal-lighthouse** silhouette with a
dramatic **animated sweeping beam**, found in none of the other twenty-two levels.
Reuses the water plane as the dark stormy sea.

## Theme & palette
- **Mood:** wild, wet, lonely — wind and surf, a lone lamp sweeping the dark.
- **Light (render.cpp):** dim cold sea-storm light, blue-grey ambient, wet sea haze
  (`~#4D5970`, density `0.0115`). Reuses `sky_storm.fs` and the dark `water_storm.fs`
  as the sea.
- **Glow:** warm lighthouse-lamp light `(0.98, 0.86, 0.58)` pooling on the rocks/sea.

## Geometry (arena.cpp :: build_beacon / draw_beacon)
Reuses `s_cyl` (tower / gallery / lamp room / mast), `s_cone` (roof / sea-stacks),
`s_dome` (breakwater boulders), `s_column` (boats), `DrawCube` (the beam).
1. **Lighthouse** — a tapered banded tower (red/white segments) + a dark gallery +
   a glass lamp room + a cone roof. The landmark and a collision obstacle.
2. **Rotating beam** — the lamp emits two opposite additive beams sweeping around
   (rotated by `s_time`), plus a pulsing lamp core.
3. **Sea-stacks** — seven jagged dark rock spires rising from the surf.
4. **Breakwater** — a ring of boulders around the perimeter.
5. **Wrecked rowboats** — two small tilted hulls with broken mast stubs.
6. **Surf foam** — additive foam pulsing at the breakwater waterline.

## Encounter (mobs.cpp)
- **7 "Marooned"** — the `Mob` horde, level-aware spawn set (`beacon_defs`), across
  the rocks. Same lightweight AI, lock-on, aggregate HUD bar (named *The Marooned*),
  and victory-on-clear.

## Integration notes
- Purely additive: levels 0–21 and the `Boss` are untouched. `mob_level` now covers
  `… || BRIDGE || BEACON`.
- Keeps the reused water plane as the stormy sea.
- CLI: `beacon` / `lighthouse` / `coast` launches straight into it.
