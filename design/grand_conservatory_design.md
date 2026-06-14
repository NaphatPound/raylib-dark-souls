# Grand Conservatory — Level Design (LEVEL_GLASS = 39)

A Victorian iron-and-glass domed palm house: a meridian-ribbed glass dome over
quadrant flower beds, towering palms, hanging baskets and a giant central palm. The
thirty-sixth **non-boss** level (and the 40th overall) — the choked dead of the
overgrown glasshouse ("The Overgrown") stir among the planting. Clear them all to
win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **domed iron-and-glass conservatory**
silhouette (meridian dome ribs + latitude rings + glazed panes + palms) found in
none of the other thirty-nine levels. Distinct from the Hanging Gardens (open formal
water-garden) and the Sunlit Vineyard (trellis rows).

## Theme & palette
- **Mood:** bright, humid, lush — overhead sun pouring through glass onto dense green
  planting, pollen drifting in the warm air.
- **Light (render.cpp):** high sun through glass, bright warm-white daylight
  `(1.08, 1.06, 0.92)`, bright humid green ambient, soft warm-green humidity fog
  (`~#B8CCB2`, density `0.0075`). Reuses `sky_ice.fs` (bright); **dry** — its own
  tiled floor is the floor.
- **Glow:** sun-through-glass green-gold `(0.72, 0.96, 0.66)`.

## Geometry (arena.cpp :: build_glass / draw_glass / draw_palm / glass_layout)
Reuses `s_column` (floor/tiles/beds/base ring), `s_dome` (apex hub / blooms / leaf
blades / baskets / fruit), `s_cyl` (finial), `draw_bone_seg` (dome ribs / latitude
rings / palm trunks & fronds / basket cords).
1. **Iron dome** — 10 meridian ribs (quarter-arc `draw_bone_seg` chains from the base
   ring up to the apex) + 2 latitude rings + an apex hub & finial.
2. **Glazed panes** — an additive pale-cyan shimmer over each dome cell (the front
   arc near the floor left open as the entrance).
3. **Base ring** — a low stone planter wall ringing the hall.
4. **Flower beds** — 4 quadrant soil boxes brimming with colored `s_dome` blooms.
5. **Palms** — 7 `draw_palm` trees (curving trunk + a ring of arching fronds with
   leaf blades + hanging fruit; collision obstacles) + a **giant central palm** (the
   centerpiece and central obstacle).
6. **Hanging baskets** — 6 baskets on cords from the lower latitude ring.
7. **Atmosphere** — additive warm sun-through-glass glow + drifting pollen motes.

## Encounter (mobs.cpp)
- **8 "Overgrown"** — the `Mob` horde, level-aware spawn set (`glass_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Overgrown*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–38 and the `Boss` are untouched. `mob_level` now covers
  `… || DOCK || GLASS`. **No new global struct/vector** — `glass_layout` fills a local
  `std::vector<Vector4>` (palms: xyz + w=height) shared by build+draw; only `s_wisps`
  reused. Glasshouse lights built in `build_glass` (so `build_crystals`/
  `draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_glass` lays the tiled floor.
- CLI: `conservatory` / `glasshouse` / `greenhouse` launches straight into it.
