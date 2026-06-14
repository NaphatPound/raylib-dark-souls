# Sentinel Court — Level Design (LEVEL_COURT = 19)

A solemn moonlit court lined with colossal stone guardians. The sixteenth
**non-boss** level: husks kneeling among the statues ("The Faithless") still stir.
Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **figurative-statuary** silhouette
(towering humanoid guardian statues) found in none of the other nineteen levels.
Reuses the water plane as a dark reflecting floor that mirrors the giants.

## Theme & palette
- **Mood:** sacred, silent, watched — giant sentinels stand vigil over a still
  dark court under a pale moon.
- **Light (render.cpp):** cool moonlight, grey-blue ambient, cool grey-blue haze
  (`~#3D4759`, density `0.0100`). Reuses `sky_storm.fs` + a pale moon disc, and the
  dark `water_storm.fs` reflecting floor.
- **Glow:** warm brazier point lights `(0.95, 0.62, 0.28)`.

## Geometry (arena.cpp :: build_court / draw_court, helper draw_sentinel)
Reuses `s_column` (pedestal / limbs / torso / sword / debris / colonnade) and
`s_dome` (helmeted heads / fallen heads).
1. **Colossal sentinels** — `draw_sentinel` builds a pedestal + plinth + legs +
   torso + pauldrons + arms + a greatsword held point-down + a helmeted dome head
   (or a broken neck stump when headless).
2. **Avenue** — eight sentinels in two rows facing inward across the avenue; about
   a quarter are headless.
3. **Colossal central sentinel** — a 1.5× guardian facing the entrance. The
   landmark and a collision obstacle.
4. **Fallen-statue debris** — toppled heads, broken column drums, and rubble.
5. **Braziers** — four warm fire stands flanking the avenue (the lights).
6. **Broken colonnade** — a ruined perimeter ring of columns.

## Encounter (mobs.cpp)
- **7 "Faithless"** — the `Mob` horde, level-aware spawn set (`court_defs`), among
  the sentinels with a tougher knight-husk deep in. Same lightweight AI, lock-on,
  aggregate HUD bar (named *The Faithless*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–18 and the `Boss` are untouched. `mob_level` now covers
  `… || AQUA || COURT`. `LEVEL_COURT` added to the moon-draw set (pale moon).
- Keeps the reused water plane as the dark reflecting floor.
- CLI: `court` / `sentinels` / `statues` launches straight into it.
