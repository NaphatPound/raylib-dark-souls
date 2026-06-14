# The Grand Plaza — Level Design (LEVEL_PLAZA = 52)

A sunlit civic square: a great multi-tiered cascading fountain at the centre, a ring
of arcade colonnade arches framing the square, corner obelisks, market stalls, iron
lamp posts and wheeling pigeons over a cobbled compass-rose pavement. The
forty-ninth **non-boss** level — the plaza's vanished townsfolk ("The Bygone") throng
the square once more. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **civic plaza** silhouette (a grand tiered
fountain + an arcade ring + obelisks) found in none of the other fifty-two levels.
Distinct from the Sentinel Court (statue avenue over water), the Ruined Throne Hall
(interior) and the Forsaken Fairground (carnival).

## Theme & palette
- **Mood:** open, civic, sunlit — bright clear daylight over pale cobbles, water
  sparkling off the fountain and pigeons wheeling overhead.
- **Light (render.cpp):** clear midday sun, warm bright daylight `(1.06, 1.00, 0.88)`,
  open bright ambient, light city haze (`~#A8ADA8`, density `0.0070`). Reuses
  `sky_ice.fs`; **dry** — its own cobbles are the floor.
- **Glow:** warm civic lamp / sun `(1.00, 0.85, 0.60)`.

## Geometry (arena.cpp :: build_plaza / draw_plaza / draw_grandfount)
Reuses `s_cyl` (fountain basins/stems / lamp posts), `s_dome` (finials / lamps),
`s_cone` (obelisks), `s_column` (cobbles / inlay / stalls), and **`draw_arch`** (the
Romanesque arcade ring).
1. **Grand fountain** — `draw_grandfount`: three stacked stone basins of decreasing
   size on stems, each holding a teal water disc, crowned with a gilded finial; the
   central landmark obstacle, with animated additive cascade rings + a central jet.
2. **Arcade ring** — 14 Romanesque `draw_arch` arches at R≈20 facing inward (front arc
   left open); each a collision obstacle.
3. **Obelisks** — 4 corner tapered-stone obelisks with gilt caps (obstacles).
4. **Civic dressing** — a cobbled compass-rose inlay (concentric rings + radial
   spokes), 4 striped-awning market stalls, and 6 iron lamp posts.
5. **Atmosphere** — additive fountain spray/cascades, warm lamp glow, and wheeling
   pigeons.

## Encounter (mobs.cpp)
- **8 "Bygone"** — the `Mob` horde, level-aware spawn set (`plaza_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Bygone*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–51 and the `Boss` are untouched. `mob_level` now covers
  `… || COLLIER || PLAZA`. **No new global struct/vector** — the arcade ring and
  obelisks are computed from fixed loops (obstacles in `build_plaza`, geometry in
  `draw_plaza`); only `s_wisps` reused. Lamp lights built in `build_plaza` (so
  `build_crystals`/`draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_plaza` lays the cobbled floor.
- CLI: `plaza` / `square` / `fountainsquare` launches straight into it.
