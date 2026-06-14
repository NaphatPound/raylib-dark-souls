# The Topiary Garden — Level Design (LEVEL_TOPIARY = 112)

A formal clipped-hedge garden in bright sun: sculpted green **topiaries** — a peacock with a fan
tail, an elephant with a curling trunk, spiral and tiered cone-trees, sphere-on-stem hedges and a
clipped green archway — are arranged around a tiered marble **fountain**, amid a box **parterre**
of geometric flower beds, gravel cross-paths and marble urns on plinths. The one hundred ninth
**non-boss** level — the dead lie overgrown among the hedges ("The Overgrown"). Clear them all to
win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **topiary-garden** silhouette (recognisable shaped
hedge sculptures + a formal parterre) found in none of the other one hundred twelve levels. A
**DRY** level (the lawn). A formal garden of *shaped* hedges — distinct from the **Hedge Maze**
(tall enclosing corridor walls) and the terraced/vined **Hanging Gardens**: here the hedges are
clipped into figures and the layout is an open parterre.

## Theme & palette
- **Mood:** bright, manicured, playful — green sculpture and colour on a sunny lawn.
- **Light (render.cpp):** clear afternoon sun `(0.36,0.80,0.40)`, bright fresh daylight
  `(1.16,1.12,0.96)`, soft sky fill, soft green-tinged air (`~#D1DBCC`, density `0.0055`). Reuses
  `sky_ice.fs`; **DRY** (`draw_topiary` lays the lawn; `water_storm.fs` placeholder is unused).
- **Glow:** warm garden sun `(1.00,0.84,0.56)`.

## Geometry (arena.cpp :: draw_conetree / draw_spiraltopiary / build_topiary / draw_topiary)
Reuses `s_column` (lawn + gravel paths / parterre hedge borders / arch / urn stems), the lit
`s_cyl` (topiary trunks / fountain tiers / sphere stems / urn bases), `s_cone` (cone-trees /
spiral cones / peacock tail / arch underside), `s_dome` (fountain finial / flowers / peacock body
+ head / elephant head+ears / sphere topiaries / urn bowls / tail "eyes"), and `draw_bone_seg`
(the elephant's trunk).
1. **Topiaries** — a **peacock** (body + curved neck + head + a vertical fan of feathers with
   coloured eyes), an **elephant** (body + four legs + hump + head + curling trunk + ears), two
   **spiral** topiaries (`draw_spiraltopiary`: a cone with a helical groove), tiered **cone-trees**
   (`draw_conetree`) lining the paths, and **sphere-on-stem** hedges. The shaped sculptures are
   obstacles.
2. **The fountain** (the central obstacle) — a tiered marble basin + pedestal + bowl + gold
   finial, jetting water.
3. **The parterre** — a manicured lawn with a gravel cross-axis and round court, four quadrant
   box-hedge beds full of coloured flowers.
4. **The frame** — a clipped-hedge entrance archway at the front + marble urns at the path
   corners; additive sun glow + fountain jet + flitting butterflies.

## Encounter (mobs.cpp)
- **8 "Overgrown"** — the `Mob` horde, level-aware spawn set (`topiary_defs`), among the hedges.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Overgrown*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–111 and the `Boss` are untouched. `mob_level` now covers
  `… || BADGIR || TOPIARY`. **No new global struct/vector** — the garden is parametric; the
  fountain + topiary + arch obstacles and the lights are pushed in `build_topiary`; flowers/
  butterflies are seeded; only `s_wisps` reused. New `draw_conetree` / `draw_spiraltopiary`
  helpers. Lights built in `build_topiary` (so `build_crystals` / `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_topiary` lays
  the lawn.
- CLI: `topiary` / `parterre` / `clipped` launches straight into it.
