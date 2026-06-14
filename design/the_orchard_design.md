# The Orchard — Level Design (LEVEL_ORCHARD = 82)

A fruit orchard in full spring bloom: rows of small trees crowned with rounded pink-and-white
blossom canopies, a leaning ladder, harvest baskets of apples, a handcart and a well, paper
lanterns strung between the trees, and a heavy, drifting storm of blossom petals. The
seventy-ninth **non-boss** level — the orchard's withered dead ("The Fallow") rise among the
trees. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **blossoming-orchard** silhouette (rows of round
blossom-canopy fruit trees under a petal storm) found in none of the other eighty-two levels.
A **DRY** level (grassy floor). Deliberately distinct from the Sunlit Vineyard (grape
trellises), the Sunflower Fields (tall yellow flowers), the Hanging Gardens (formal flower
beds), Snowbound Pines (snowy conifers), the Redwood Grove (giant trunks), and the Bamboo
Grove (green stalks).

## Theme & palette
- **Mood:** gentle, fleeting, lovely — soft spring sun and a sky of falling petals.
- **Light (render.cpp):** soft warm spring sun `(0.34,0.78,0.36)`, gentle warm daylight
  `(1.08,1.00,0.86)`, soft bright fill, soft pink-tinged haze (`~#CCBDC2`, density `0.0072`).
  Reuses `sky_ice.fs`; **DRY** (`draw_orchard` lays the grass).
- **Glow:** soft warm blossom-pink `(1.00,0.82,0.82)`.

## Geometry (arena.cpp :: draw_fruittree / orchard_layout / build_orchard / draw_orchard)
Reuses `draw_bone_seg` (trunks + branches), `s_dome` (blossom/leaf canopy puffs / grass tufts
/ apples / lantern globes), `s_cyl` (baskets / well / cart wheels), and `s_column` (floor /
ladder / cart bed).
1. **Fruit trees** (`draw_fruittree`) — a leaning trunk + three branches + a rounded canopy of
   eight blossom/leaf `s_dome` puffs. A jittered grid of trees (every other one a slightly
   different blossom shade) + a central great tree (the landmark obstacle).
2. **Orchard clutter** — a leaning ladder, two apple baskets, a handcart, a well.
3. **Lanterns** — five paper lanterns strung in a catenary between two trees.
4. **Atmosphere** — additive lantern glow + a **heavy storm of 48 drifting blossom petals** +
   soft pollen.

## Encounter (mobs.cpp)
- **8 "Fallow"** — the `Mob` horde, level-aware spawn set (`orchard_defs`), among the trees.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Fallow*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–81 and the `Boss` are untouched. `mob_level` now covers
  `… || TRIUMPH || ORCHARD`. **No new global struct/vector** — `orchard_layout` fills a local
  `std::vector<Vector4>` (x, z, height) shared by build (trunk obstacles) + draw; clutter
  fixed; only `s_wisps` reused. Warm lights built in `build_orchard` (so `build_crystals`/
  `draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_orchard` lays the grass.
- CLI: `orchard` / `blossom` / `cherry` launches straight into it (avoids `bloom`, the Hanging
  Gardens' alias).
