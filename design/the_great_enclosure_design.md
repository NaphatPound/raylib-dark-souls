# The Great Enclosure — Level Design (LEVEL_ZIMBABWE = 98)

An abandoned Sub-Saharan royal court in the African afternoon: a tall curved **dry-stone wall**
of mortarless granite — coursed and crowned with a chevron pattern — rings the solid, tapering
**Conical Tower** and its lesser companion, with a narrow parallel passage, round **daga huts**
under conical thatch, soapstone **Zimbabwe-bird** monoliths and granite boulders all around. The
ninety-fifth **non-boss** level — the court's shades have risen ("The Dethroned"). Clear them all
to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **dry-stone enclosure** silhouette (a high curved
mortarless wall + a solid stone cone) found in none of the other ninety-eight levels. A **DRY**
level (dry African ground). A fresh African **architecture** (Great Zimbabwe) — distinct from
**The Savanna** (an African grassland *biome*, no buildings), the **Desert Ziggurat** (a stepped
Mesopotamian pyramid) and the **Grain Elevator/Silo** (cylindrical timber/metal): this is a
curved granite wall around a coursed stone cone.

## Theme & palette
- **Mood:** regal, weathered, hushed — a deserted stone city under a golden sky.
- **Light (render.cpp):** warm low afternoon sun `(0.42,0.74,0.36)`, golden light on grey
  granite `(1.16,1.04,0.80)`, warm fill, dry golden haze (`~#C7B894`, density `0.0065`). Reuses
  `sky_ice.fs`; **DRY** (`draw_zimbabwe` lays the ground; `water_storm.fs` placeholder is unused).
- **Glow:** warm afternoon granite `(1.00,0.74,0.42)`.

## Geometry (arena.cpp :: draw_conical_tower / draw_daga_hut / draw_zim_bird / build_zimbabwe / draw_zimbabwe)
Reuses `s_column` (ground / wall blocks + chevron course + capstones / hut doorways / bird
monoliths+wings), the lit `s_cyl` (tower courses / hut walls), `s_cone` (hut thatch / bird beaks),
and `s_dome` (tower tops / granite boulders / bird bodies+heads).
1. **The enclosure wall** — a curved ring of coursed granite blocks (44 segments, an entrance gap
   at the front), height-varied, with an alternating chevron top course and capstones (obstacles).
2. **The Conical Tower** (`draw_conical_tower`, the landmark obstacle) — a solid tapering cone of
   16 stacked stone courses (slightly rotated, alternating shade) with a rounded top; plus a
   lesser tower beside it.
3. **The narrow passage** — a short inner curved wall forming the famous parallel passage to the
   towers.
4. **Daga huts** (`draw_daga_hut`) — round mud walls with dark doorways under conical thatch roofs
   (obstacles).
5. **Soapstone birds** (`draw_zim_bird`) — bird sculptures on monoliths flanking the way in.
6. **The kopje** — granite boulders inside and rising beyond the wall; additive hut/tower glow +
   a hut hearth + dry dust + circling birds.

## Encounter (mobs.cpp)
- **8 "Dethroned"** — the `Mob` horde, level-aware spawn set (`zimbabwe_defs`), in the court.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Dethroned*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–97 and the `Boss` are untouched. `mob_level` now covers
  `… || CACTUS || ZIMBABWE`. **No new global struct/vector** — the enclosure is parametric; the
  wall-ring + tower + hut obstacles and the lights are pushed in `build_zimbabwe`; grass/boulders/
  dust are seeded; only `s_wisps` reused. New `draw_conical_tower` / `draw_daga_hut` /
  `draw_zim_bird` helpers. Lights built in `build_zimbabwe` (so `build_crystals` / `draw_crystals`
  early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_zimbabwe`
  lays the ground.
- CLI: `zimbabwe` / `enclosure` / `conical` launches straight into it.
