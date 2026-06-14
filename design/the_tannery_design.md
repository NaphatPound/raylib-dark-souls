# The Tannery — Level Design (LEVEL_TANNERY = 106)

A Moroccan dye-works (Chouara, Fez) baking under a hard sun: a honeycomb of round stone **dye
pits** brims with vivid saffron-yellow, poppy-red, indigo-blue, mint-green and tan-brown dye,
while animal **hides** dry on racks and along the parapets of tall terraced mud-brick walls, and
baskets of raw pigment and stacks of cured hides clutter the earthen working floor. The one
hundred third **non-boss** level — the dyers' dead have risen ("The Steeped"). Clear them all to
win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **dye-works** silhouette (a honeycomb of round
colour-pits + drying hides + terraced mud-brick) found in none of the other one hundred six
levels. A **DRY** level (the earthen floor). A leather-craft scene — distinct from **The Salt
Pans** (a grid of *square white* evaporation ponds): the tannery's pits are *round and vividly
coloured*, hung about with hides.

## Theme & palette
- **Mood:** hot, pungent, dazzling — a quilt of colour in the white glare.
- **Light (render.cpp):** high blazing sun `(0.40,0.82,0.30)`, brilliant warm daylight
  `(1.18,1.08,0.84)`, warm earthy fill, pungent dusty haze (`~#D1C7A4`, density `0.0058`).
  Reuses `sky_ice.fs` (bright blue); **DRY** (`draw_tannery` lays the earth; `water_storm.fs`
  placeholder is unused).
- **Glow:** hot sun on earth `(1.00,0.78,0.46)`.

## Geometry (arena.cpp :: draw_dyepit / draw_hiderack / build_tannery / draw_tannery)
Reuses the lit `s_cyl` (pit rims + brimming dye / rack posts+rails / pigment baskets), `s_dome`
(dye meniscus / basket lids), and `s_column` (earth floor + stains / draped hides / terraced mud
walls + parapets + windows / roof hides / cured-hide stacks).
1. **Dye pits** (`draw_dyepit`) — a raised stone rim + a brimming coloured dye surface proud of
   the rim with a slight meniscus so the colour reads from above. A 5×3 grid at the back + two
   front pits (obstacles).
2. **Drying hides** (`draw_hiderack`) — two posts + a rail draped with three hides; four racks in
   the working area, plus rows of hides along the wall parapets and back roofline.
3. **The walls** — tall terraced mud-brick side walls with parapets and a back wall with dark
   windows, hides drying on the rooftops.
4. **Stores** — baskets of raw pigment (colour-topped) and a stack of cured hides; additive sun
   glow + drifting dust + flies over the pits.

## Encounter (mobs.cpp)
- **8 "Steeped"** — the `Mob` horde, level-aware spawn set (`tannery_defs`, in the front working
  area so the colour-pit grid stays framed behind). Same lightweight AI, lock-on, aggregate HUD
  bar (named *The Steeped*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–105 and the `Boss` are untouched. `mob_level` now covers
  `… || ISHTAR || TANNERY`. **No new global struct/vector** — the works is parametric; the
  dye-pit obstacles and the lights are pushed in `build_tannery`; stains/baskets/dust are seeded;
  only `s_wisps` reused. New `draw_dyepit` / `draw_hiderack` helpers. Lights built in
  `build_tannery` (so `build_crystals` / `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_tannery`
  lays the earthen floor.
- CLI: `tannery` / `dye` / `fez` launches straight into it.
