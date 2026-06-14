# The Pagoda — Level Design (LEVEL_PAGODA = 88)

A serene temple in the golden afternoon: a **five-tier pagoda** of stacked white-plaster bodies
and wide overhanging tiled roofs climbs to a gold **sorin** finial, a vermilion **torii gate**
marks the entrance, stone **lanterns** glow along the paths, **cherry trees** drop petals over a
**raked-gravel zen garden** of moss patches and rock rings. The eighty-fifth **non-boss** level —
the temple's restless shades ("The Penitent") rise in the garden. Clear them all to win; the
bonfire then ignites as usual.

New procedural geometry, not a recolour — a **East-Asian pagoda-temple** silhouette (a tapering
tower of stacked curved roofs + a torii + stone lanterns + sakura) found in none of the other
eighty-eight levels. A **DRY** level (the raked-gravel garden). A wholly new architectural
tradition — distinct from every other temple/sacred level: the Islamic-dome **Great Mosque**, the
Gothic **Cathedral Nave**, the Greek **Amphitheatre**, the Egyptian **Hypostyle Hall**, the
Pacific-NW **Totem Village**, and the **Bamboo Grove** (a bamboo forest, no building).

## Theme & palette
- **Mood:** calm, contemplative, sunlit — incense and falling blossom in a temple garden.
- **Light (render.cpp):** warm low afternoon sun `(0.40,0.66,0.42)`, soft warm gold
  `(1.12,0.98,0.80)`, gentle warm fill, soft cherry-blossom haze (`~#DBC7C7`, density `0.0072`).
  Reuses `sky_ice.fs` (clear sky); **DRY** (`draw_pagoda` lays the gravel; `water_storm.fs`
  placeholder is unused).
- **Glow:** warm lantern light `(1.00,0.72,0.40)`.

## Geometry (arena.cpp :: build_pagoda / draw_pagoda)
Reuses `s_column` (tier bodies / roof slabs / torii beams / lantern fire boxes / balustrades),
the lit `s_cyl` (corner posts / torii pillars / lantern posts / finial mast), `s_cone` (roof hip
caps / upturned eave tips / lantern roofs), `s_dome` (rocks / moss / blossom canopies / jewels),
`s_torus` (raked-gravel rings + the sorin's stacked rings), and `draw_bone_seg` (cherry trunks).
1. **The pagoda** (the central obstacle) — five tiers, each a plastered body with dark corner
   posts, a red balustrade band, and a wide overhanging tiled roof (slab + hip cap + four
   upturned corner eave-tips); topped by a gold sorin (mast + stacked rings + jewel).
2. **Torii gate** — two pillars + a curved kasagi top beam with upturned ends + a nuki beam + a
   gold plaque, at the garden entrance toward the player spawn.
3. **Stone lanterns** (ishidoro) — five, each base + post + glowing fire box + roof + finial
   (obstacles); the front one set off the spawn path so the pagoda frames cleanly.
4. **Garden** — a raked-gravel floor with two feature rocks ringed by concentric raked tori,
   moss patches, and seven cherry-blossom trees around the rim.
5. **Atmosphere** — additive lantern glow + drifting cherry petals + a thread of incense smoke.

## Encounter (mobs.cpp)
- **8 "Penitent"** — the `Mob` horde, level-aware spawn set (`pagoda_defs`), risen in the garden.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Penitent*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–87 and the `Boss` are untouched. `mob_level` now covers
  `… || OASIS || PAGODA`. **No new global struct/vector** — the 5 lantern positions are a fixed
  array shared by build (obstacles + lights) + draw; the pagoda/torii fixed; rocks/moss/sakura
  seeded; only `s_wisps` reused. Lantern + pagoda lights built in `build_pagoda` (so
  `build_crystals` / `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_pagoda`
  lays the gravel floor.
- CLI: `pagoda` / `torii` / `zen` launches straight into it.
