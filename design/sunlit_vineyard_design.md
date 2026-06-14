# Sunlit Vineyard — Level Design (LEVEL_VINE = 35)

A terraced hillside vineyard at golden hour: long parallel rows of grape trellis
heavy with fruit, a central screw wine-press, stacked oak barrels, a Tuscan
farmhouse and ranks of cypress. The thirty-second **non-boss** level — the
vineyard's crushed dead ("The Trodden") rise among the vines. Clear them all to win;
the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **vineyard** silhouette (parallel trellis
rows + a screw press + barrel pyramids + cypress) found in none of the other
thirty-five levels. A warm, bright counterpoint to the recent grim levels; distinct
from the Hanging Gardens (formal flowerbeds), Windmill Fields (windmills) and Plague
Hamlet (cottages).

## Theme & palette
- **Mood:** warm, ripe, golden — low amber sun raking across the rows, dust and
  gnats hanging in the light.
- **Light (render.cpp):** low warm sun, rich golden light `(1.10, 0.92, 0.66)`, warm
  soft ambient, hazy golden dusk fog (`~#998566`, density `0.0080` — light/clear).
  Reuses `sky_forge.fs` (warm amber dusk); **dry** — its own terraced earth is the
  floor.
- **Glow:** warm golden-hour lamp `(1.00, 0.82, 0.50)`.

## Geometry (arena.cpp :: build_vine / draw_vine / draw_trellis / vine_layout)
Reuses `s_cyl` (posts / press vat / barrels / cypress trunks), `s_dome` (foliage /
grass), `s_cone` (grape clusters / cypress crowns), `s_torus` (press capstan wheel),
`draw_bone_seg` (trellis wires), and **`draw_logpile`** (barrel pyramids) +
**`draw_cottage`** (the farmhouse).
1. **Trellis rows** — 5 `draw_trellis` rows running along +x (the central press row
   skipped): posts joined by three wires, leafy clumps and hanging purple grape
   clusters along each span.
2. **Wine press** — a big oak vat + a screw (post) + a press beam + an `s_torus`
   capstan wheel; the central landmark obstacle.
3. **Barrel stacks** — 3 oak-barrel pyramids (reusing `draw_logpile` with short fat
   "logs"); obstacles.
4. **Farmhouse** — a scaled-up `draw_cottage` Tuscan villa; obstacle.
5. **Cypress** — 8 tall thin `s_cone` trees lining the field (seeded; obstacles), plus
   grape crates near the press and terrace retaining banks.
6. **Golden glow** — additive warm lamp glow + drifting dust/gnats.

## Encounter (mobs.cpp)
- **8 "Trodden"** — the `Mob` horde, level-aware spawn set (`vine_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Trodden*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–34 and the `Boss` are untouched. `mob_level` now covers
  `… || TAR || VINE`. **No new global struct/vector** — `vine_layout` fills a local
  `std::vector<Vector4>` (cypress: xyz + w=height) shared by build+draw; only
  `s_wisps` reused. Golden lamps built in `build_vine` (so `build_crystals`/
  `draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_vine` lays the terraced floor.
- CLI: `vineyard` / `vine` / `winery` launches straight into it.
