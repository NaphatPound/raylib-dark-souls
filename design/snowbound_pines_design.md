# Snowbound Pines — Level Design (LEVEL_PINES = 67)

A winter conifer forest: snow-laden evergreens in layered green tiers, a frozen pond, a
log cabin with a smoking chimney, snowdrifts and a steady fall of snow under a cold bright
sky. The sixty-fourth **non-boss** level — the winter wood's frozen dead ("The Snowbound")
rise from the drifts. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **snowy-pinewood** silhouette (tiered conifers
+ frozen pond + log cabin) found in none of the other sixty-seven levels. A **DRY** level
(snowy ground). Deliberately distinct from the Frozen Floes (open sea-ice + bergs, no
trees), the Frozen Cathedral (an interior ice hall), and the Petrified Forest (dead grey
twisted trees): this is a living evergreen wood under fresh snow.

## Theme & palette
- **Mood:** crisp, hushed, cold-cozy — fresh powder and a warm cabin window.
- **Light (render.cpp):** bright winter sun `(0.32,0.80,0.30)`, cold bright snow-light
  `(0.92,0.96,1.06)`, bright cool snow fill, cold pale haze (`~#BDCCE6`, density `0.0085`).
  Reuses `sky_ice.fs`; **DRY** (`draw_pines` lays the snowy ground).
- **Glow:** cold blue-white snow `(0.70,0.82,0.95)` (with a warm cabin-window accent).

## Geometry (arena.cpp :: draw_conifer / pines_layout / build_pines / draw_pines)
Reuses the lit `s_cyl` (trunks / stumps / frozen-pond disc), `s_cone` (the conifer tiers +
their snow frosting), `s_column` (snowy floor / cabin via `draw_cottage` / chimney /
roof-snow), `s_dome` (snowdrifts / stump caps), and `draw_bone_seg` (a fallen log).
1. **Conifers** (`draw_conifer`) — a short trunk + 4–5 stacked shrinking green cone tiers,
   each frosted with a smaller white snow cone. 16 across the field + a central great pine
   (the landmark obstacle).
2. **Cabin** — a `draw_cottage` log cabin with a snow-laden roof ridge and a smoking
   chimney (obstacle).
3. **Pond** — a flat icy disc (frozen pond).
4. **Ground** — a snowy slab, 20 snowdrift mounds, a snow-capped stump, a fallen log.
5. **Atmosphere** — additive warm cabin window + soft cold glow + chimney smoke + a steady
   fall of snow.

## Encounter (mobs.cpp)
- **8 "Snowbound"** — the `Mob` horde, level-aware spawn set (`pines_defs`). Same lightweight
  AI, lock-on, aggregate HUD bar (named *The Snowbound*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–66 and the `Boss` are untouched. `mob_level` now covers
  `… || CAVERN || PINES`. **No new global struct/vector** — `pines_layout` fills a local
  `std::vector<Vector4>` (x, z, height) shared by build (obstacles) + draw; cabin/pond/stump
  fixed; only `s_wisps` reused. Cold lights built in `build_pines` (so `build_crystals`/
  `draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_pines` lays the snowy ground.
- CLI: `pines` / `snowforest` / `winter` launches straight into it.
