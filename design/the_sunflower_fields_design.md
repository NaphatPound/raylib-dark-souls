# The Sunflower Fields — Level Design (LEVEL_SUNFLOWER = 69)

A sunny field of towering sunflowers: tall leafy stalks crowned with big yellow-petalled,
brown-centered heads all turned to the sun, over furrowed soil rows, with a lone scarecrow
keeping watch. The sixty-sixth **non-boss** level — the field's withered dead ("The Wilted")
shamble between the stalks. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **giant-sunflower-field** silhouette (rows of
tall stalks + broad heads) found in none of the other sixty-nine levels. A **DRY** level
(tilled soil). Distinct from the Windmill Fields (golden wheat tufts + windmills), the
Hanging Gardens (low formal flowerbeds), and the Vineyard (grape trellises): these are
person-height-plus flowers you weave between.

## Theme & palette
- **Mood:** warm, drowsy, faintly eerie — high summer sun on a sea of yellow faces.
- **Light (render.cpp):** warm high sun `(0.36,0.78,0.34)`, warm golden daylight
  `(1.12,1.00,0.70)`, warm fill, warm hazy gold (`~#BDBD8A`, density `0.0072`). Reuses
  `sky_ice.fs` (the bright-blue sky that sets off the yellow); **DRY** (`draw_sunflower_field`
  lays the tilled soil).
- **Glow:** warm golden sun `(1.00,0.85,0.40)`.

## Geometry (arena.cpp :: draw_sunflower / sunflower_layout / build_sunflower / draw_sunflower_field)
Reuses the lit `s_cyl` (head sepal/seed discs), `s_dome` (broad leaves / scarecrow head),
`s_column` (soil / furrows / petals / scarecrow post+arms), `s_cone` (scarecrow hat), and
`draw_bone_seg` (the leaning stalk).
1. **Sunflowers** (`draw_sunflower`) — a leaning stalk (`draw_bone_seg`) + two broad leaves,
   crowned with a big head tilted on an rlgl frame to face the sun: a green sepal back, a
   ring of 12 yellow petal-boxes, and a brown seed disc. ~21 in a jittered grid + a central
   great sunflower (the landmark obstacle); each head turned a slightly different way.
2. **Field** — a tilled soil slab with furrow rows.
3. **Scarecrow** — a post + crossarm, a burlap head and a pointed hat.
4. **Atmosphere** — additive warm sun glow + drifting pollen / bees.

## Encounter (mobs.cpp)
- **8 "Wilted"** — the `Mob` horde, level-aware spawn set (`sunflower_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Wilted*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–68 and the `Boss` are untouched. `mob_level` now covers
  `… || GALLEON || SUNFLOWER`. **No new global struct/vector** — `sunflower_layout` fills a
  local `std::vector<Vector4>` (x, z, height, yaw) shared by build (stalk obstacles) + draw;
  scarecrow fixed; only `s_wisps` reused. Warm lights built in `build_sunflower` (so
  `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_sunflower_field` lays the tilled soil.
- CLI: `sunflower` / `sunflowers` / `sunfield` launches straight into it.
