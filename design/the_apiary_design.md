# The Apiary — Level Design (LEVEL_APIARY = 40)

A sunlit bee-yard in a wildflower meadow: rows of stacked wooden box-hives and
conical straw skeps around a great ancient hollow bee-tree dripping golden comb, the
air thick with a swarming cloud of bees. The thirty-seventh **non-boss** level — the
apiary's stung dead ("The Swarmed") rise among the hives. Clear them all to win; the
bonfire then ignites as usual.

New procedural geometry, not a recolour — an **apiary** silhouette (box-hives + straw
skeps + a hollow bee-tree + a bee swarm) found in none of the other forty levels.
Distinct from the Plague Hamlet (cottages), Windmill Fields (farmland) and Sunlit
Vineyard (trellis rows).

## Theme & palette
- **Mood:** warm, drowsy, alive — golden midday over a flowering meadow, the constant
  shimmer of a bee cloud and a drift of smoker-haze.
- **Light (render.cpp):** warm midday sun, golden meadow light `(1.10, 1.02, 0.82)`,
  bright warm ambient, soft golden-green haze (`~#BCC79E`, density `0.0070`). Reuses
  `sky_ice.fs` (clear blue); **dry** — its own meadow is the floor.
- **Glow:** warm honey-gold `(1.00, 0.82, 0.40)`.

## Geometry (arena.cpp :: build_apiary / draw_apiary / draw_hive / draw_skep / apiary_layout)
Reuses `s_column` (meadow / hive boxes / stands), `s_cyl` (skep coils / smoker),
`s_dome` (grass / blooms / comb / skep cap), `s_cone` (smoker top), and
**`draw_ptree`** (the bee-tree trunk, in brown).
1. **Box hives** — 9 `draw_hive` stacked wooden hives (stand + 2–4 supers + a
   telescoping lid + an entrance slot), seeded across the meadow; each a collision
   obstacle.
2. **Straw skeps** — 3 `draw_skep` conical coiled hives; obstacles.
3. **Bee-tree** — a great ancient hollow tree (reused `draw_ptree` trunk) with a dark
   hollow and golden `s_dome` comb lobes; the central landmark obstacle.
4. **Meadow** — grass tufts + colorful wildflower patches; a smoker prop venting an
   additive smoke plume.
5. **Bee swarm** — a dense additive cloud of fast-jittering yellow motes (a `swarm`
   lambda) around the bee-tree and every hive — the signature animation — plus warm
   honey glow.

## Encounter (mobs.cpp)
- **8 "Swarmed"** — the `Mob` horde, level-aware spawn set (`apiary_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Swarmed*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–39 and the `Boss` are untouched. `mob_level` now covers
  `… || GLASS || APIARY`. **No new global struct/vector** — `apiary_layout` fills a
  local `std::vector<Vector4>` (hives: xyz + w=yaw) shared by build+draw; skeps are at
  fixed positions; only `s_wisps` reused. Honey lights built in `build_apiary` (so
  `build_crystals`/`draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_apiary` lays the meadow floor.
- CLI: `apiary` / `bees` / `beeyard` launches straight into it.
