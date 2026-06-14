# The Ger Camp — Level Design (LEVEL_GER = 101)

A Mongolian encampment on the open steppe: round felt **gers (yurts)** with their domed roofs,
crown smoke-rings and painted orange doors are scattered across green-gold grassland around a
sacred **ovoo** stone-cairn draped with blue prayer scarves and flags; a campfire with a tripod
cauldron burns at the centre, horses stand at a hitching line, tall horsehair **spirit-banners**
(tug) flank the cairn and an ox-cart waits by a ger. The ninety-eighth **non-boss** level — the
steppe's shades have risen ("The Windswept"). Clear them all to win; the bonfire then ignites as
usual.

New procedural geometry, not a recolour — a **steppe-camp** silhouette (round domed felt
dwellings + an ovoo cairn) found in none of the other one hundred one levels. A **DRY** level
(grassland). A fresh Central-Asian nomad culture — distinct from the **Siege Camp** (a field of
pointed military tents), the **Plague Hamlet** / **Stilt Village** (cubic/peaked huts) and **The
Savanna** (an African grassland biome): the round ger and the ovoo are the new shapes.

## Theme & palette
- **Mood:** wide, windswept, open — a small camp under an enormous sky.
- **Light (render.cpp):** high steppe sun `(0.40,0.80,0.34)`, warm clear daylight
  `(1.16,1.10,0.88)`, open-sky fill, pale grassland haze (`~#CCD1B3`, low density `0.0055`).
  Reuses `sky_ice.fs` (bright blue); **DRY** (`draw_ger` lays the grassland; `water_storm.fs`
  placeholder is unused).
- **Glow:** warm steppe campfire `(1.00,0.62,0.30)`.

## Geometry (arena.cpp :: draw_yurt / build_ger / draw_ger)
Reuses the lit `s_cyl` (ger walls + top bands + crowns / banner + cart-shaft + tripod poles /
horse legs / hitching posts), `s_cone` (ger roofs / banner finials), `s_dome` (cairn stones /
distant hills / cauldron / fire-ring stones), `s_column` (grass slab + tufts / ger doors / horse
bodies+heads / cart bed / flags), `s_torus` (cart wheels), and `draw_bone_seg` (roof poles /
grass tufts / ovoo poles / horsehair tassels / rope / horse necks).
1. **Gers** (`draw_yurt`) — a felt cylinder wall + a low conical roof + a painted top band + a
   crown smoke-ring + radial roof poles + a painted door with a dark gap. Six scattered
   (obstacles).
2. **The ovoo** (the back landmark obstacle) — a stacked-stone cairn + a cone of poles draped
   with blue scarf-bands and topped with flags.
3. **Camp life** — a central campfire with a tripod cauldron, a horse-hitching line with three
   horses, two tall horsehair spirit-banners (tug), and an ox-cart with big wheels (obstacle).
4. **The steppe** — green-gold grass with tufts and dirt patches, low distant hills; additive
   campfire glow + drifting dust + circling eagles.

## Encounter (mobs.cpp)
- **8 "Windswept"** — the `Mob` horde, level-aware spawn set (`ger_defs`), across the camp. Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Windswept*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–100 and the `Boss` are untouched. `mob_level` now covers
  `… || PRINT || GER`. **No new global struct/vector** — the six ger positions are a fixed array
  shared by build (obstacles) + draw; the ovoo + ox-cart obstacles and the lights are pushed in
  `build_ger`; grass/dust/hills are seeded; only `s_wisps` reused. A new `draw_yurt` helper draws
  each ger. Lights built in `build_ger` (so `build_crystals` / `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_ger` lays the
  grassland.
- CLI: `ger` / `yurt` / `steppe` launches straight into it.
