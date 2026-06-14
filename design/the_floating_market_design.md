# The Floating Market — Level Design (LEVEL_FLOAT = 104)

A tropical river market in the humid sun: colourful vendor **boats (sampans)** laden with fruit
pyramids, flowers and woven baskets — each with a conical-hatted vendor and a paddle — cluster
around a central wooden **market pier**, while riverside **stilt-shops** under bright awnings,
coconut palms, strings of lanterns and floating lily pads ring the turquoise water. The one
hundred first **non-boss** level — the river's drowned dead have risen ("The Adrift"). Clear them
all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **floating-market** silhouette (laden vendor boats +
a market pier + stilt-shops on a river) found in none of the other one hundred four levels. A
**WET** level — the turquoise river plane is the water, with the wooden pier (the fighting floor)
raised on it. A commerce-on-water scene — distinct from **The Canals** (Venetian stone houses +
gondolas on canals), the **Stilt Village** (huts on stilts, no market) and the dry **Grand
Bazaar**: here the focus is the cluster of colourful produce-laden boats.

## Theme & palette
- **Mood:** warm, bustling, humid — a riot of colour and produce on slow brown-green water.
- **Light (render.cpp):** bright tropical sun `(0.40,0.78,0.34)`, warm golden daylight
  `(1.16,1.06,0.84)`, warm humid fill, green-gold river haze (`~#C2CCB3`, density `0.0072`).
  Reuses `sky_ice.fs` + `water_ice.fs` (turquoise). **WET** (`draw_water` runs; `draw_float`
  lays the pier).
- **Glow:** warm lantern over the river `(1.00,0.70,0.36)`.

## Geometry (arena.cpp :: draw_boat / draw_stiltshop / build_float / draw_float)
Reuses `s_column` (pier + plank seams / boat hulls / shop platforms+walls / produce baskets),
the lit `s_cyl` (pier posts / boat baskets+vendors / shop stilts), `s_cone` (boat prows+sterns /
produce pyramids / vendor hats / shop roofs), `s_dome` (fruit / lanterns / lily pads), the
**reused `draw_palm`** helper, and `draw_bone_seg` (lantern strings / boat paddles).
1. **The pier** — a raised wooden market deck (the fighting floor) with plank seams, four
   lantern-topped corner posts, lantern strings and a row of hung lanterns.
2. **Vendor boats** (`draw_boat`) — eight colourful sampans (hull + raised prow/stern + three
   produce pyramids with fruit + a basket + a conical-hatted vendor + a paddle) ringing the pier
   on the river (obstacles).
3. **Stilt-shops** (`draw_stiltshop`) — three riverside shops on stilts with back walls, peaked
   roofs, bright awnings and a lantern (obstacles).
4. **The river** — four coconut palms on the banks, scattered floating lily pads; additive
   lantern glow + river sparkle + drifting dragonflies.

## Encounter (mobs.cpp)
- **8 "Adrift"** — the `Mob` horde, level-aware spawn set (`float_defs`), on the pier. Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Adrift*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–103 and the `Boss` are untouched. `mob_level` now covers
  `… || BATHS || FLOAT`. **No new global struct/vector** — the market is parametric; the boat-ring
  + stilt-shop obstacles and the lights are pushed in `build_float`; lily pads/sparkle are seeded;
  only `s_wisps` reused. New `draw_boat` / `draw_stiltshop` helpers + the reused `draw_palm`.
  Lights built in `build_float` (so `build_crystals` / `draw_crystals` early-return). Uses the
  global `water_level` to sit the boats + lily pads on the river.
- WET level: `draw_water` is NOT in its dry early-return (the river draws); `draw_crystals` IS.
- CLI: `floating` / `river` / `sampan` launches straight into it.
