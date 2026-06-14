# The Kiln Yard — Level Design (LEVEL_KILN = 41)

A potter's works dominated by great bulbous brick **bottle-kilns** glowing with
firing-heat and trailing smoke, ringed by drying racks of pots, a spinning potter's
wheel, clay pits and stacked fired vessels. The thirty-eighth **non-boss** level —
the kiln yard's scorched dead ("The Fired") rise among the ovens. Clear them all to
win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **bottle-kiln pottery** silhouette (brick
bottle-ovens + drying racks + a potter's wheel) found in none of the other forty-one
levels. Distinct from the Volcanic Forge (lava boss arena), the Timber Sawmill and
the Windmill Fields.

## Theme & palette
- **Mood:** hot, dusty, industrious — firelit brick-dust haze, the orange flicker of
  kiln doors and plumes of smoke from the bottle-oven chimneys.
- **Light (render.cpp):** hazy overcast sun, warm firelit-dust light `(1.02, 0.88,
  0.70)`, warm working ambient, brick-dust + kiln-smoke haze (`~#85766A`, density
  `0.0110`). Reuses `sky_storm.fs`; **dry** — its own brickyard is the floor.
- **Glow:** hot kiln-fire orange `(1.00, 0.55, 0.20)`.

## Geometry (arena.cpp :: build_kiln / draw_kiln / draw_bottle_oven / kiln_layout)
Reuses `s_cyl` (kiln rings / chimney / hoops / pots / wheel), `s_dome` (clay mounds /
ash), `s_column` (floor / firing door / shelves), and the rlgl matrix (spinning
wheel).
1. **Bottle-kilns** — 4 `draw_bottle_oven` ovens + a central great kiln: a
   bottle-profile body of stacked brick rings narrowing to a chimney neck, with iron
   banding hoops and a dark firing door. The central kiln is the landmark obstacle;
   all are obstacles.
2. **Drying racks** — 2 racks with posts + 2 shelves laden with rows of pots.
3. **Potter's wheel** — a pedestal with a **spinning wheel** (rlgl matrix turned by
   `s_time`) forming a pot.
4. **Yard** — clay pits (dark slabs) + clay mounds, stacked fired-pot pyramids, ash
   patches.
5. **Kiln fire** — additive flickering orange door-glow + rising chimney smoke at
   every kiln + ember motes (a `kilnfire` lambda).

## Encounter (mobs.cpp)
- **8 "Fired"** — the `Mob` horde, level-aware spawn set (`kiln_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Fired*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–40 and the `Boss` are untouched. `mob_level` now covers
  `… || APIARY || KILN`. **No new global struct/vector** — `kiln_layout` fills a local
  `std::vector<Vector4>` (kilns: xyz + w=scale) shared by build+draw; racks/wheel at
  fixed positions; only `s_wisps` reused. Kiln lights built in `build_kiln` (so
  `build_crystals`/`draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_kiln` lays the brickyard floor.
- CLI: `kiln` / `pottery` / `kilnyard` launches straight into it.
