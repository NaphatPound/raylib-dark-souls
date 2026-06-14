# The Cactus Forest — Level Design (LEVEL_CACTUS = 97)

A Sonoran desert under a blasting sun: towering **saguaro** cacti raise their arms and flower
crowns over a stand of **prickly pear**, fan-stemmed **ocotillo**, spiky **agave** rosettes and
fat **barrel cacti**, scattered red rocks and dry brush; a flat-topped red **mesa** stands at the
back and a bleached steer skull, a dead saguaro's ribs and tumbleweeds litter the hardpan. The
ninety-fourth **non-boss** level — sun-dried husks have risen among the cacti ("The Withered").
Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **saguaro-desert** silhouette (arm-raised columnar
cacti + companion succulents + a mesa) found in none of the other ninety-seven levels. A **DRY**
level (desert hardpan). A fresh desert **biome** — distinct from the **Desert Ziggurat** (a
stepped ruin), **The Oasis** (a turquoise pool ringed by date palms) and **The Hoodoos** (eroded
rock spires): this is a living cactus forest, no ruin, no water, no rock spires.

## Theme & palette
- **Mood:** hot, still, sun-blasted — a forest of green giants in a parched land.
- **Light (render.cpp):** high blasting sun `(0.38,0.84,0.30)`, brilliant warm daylight
  `(1.20,1.10,0.86)`, warm sand-bounce fill, pale heat haze (`~#D6C7A3`, low density `0.0055`).
  Reuses `sky_ice.fs` (bright blue); **DRY** (`draw_cactus` lays the hardpan; `water_storm.fs`
  placeholder is unused).
- **Glow:** warm desert sun `(1.00,0.78,0.44)`.

## Geometry (arena.cpp :: draw_saguaro / build_cactus / draw_cactus)
Reuses the lit `s_cyl` (saguaro trunks / barrel cacti / prickly-pear pads via `s_column`), `s_column`
(hardpan + cracks / ribs / prickly-pear pads / mesa tiers), `s_cone` (agave leaves), `s_dome`
(cactus tops + crowns + flowers / rocks / fruit / barrel tops / steer skull / tumbleweeds), and
`draw_bone_seg` (saguaro arms / ocotillo whips / brush / horns / dead saguaro ribs).
1. **Saguaros** (`draw_saguaro`) — a ribbed trunk + a rounded top + a ring of crown flowers +
   1–4 arms (out-then-up via `draw_bone_seg`, with flowered tips); nine in a fixed stand + a
   great central one (obstacles).
2. **Companions** (seeded) — prickly-pear pad clusters with fruit, ocotillo stem-fans with red
   tips, agave rosettes, and barrel cacti with flower crowns.
3. **The mesa** — a flat-topped red rock mass with an eroded spur, the back landmark (an obstacle).
4. **Desert dressing** — cracked hardpan, scattered rocks, dry brush, a bleached steer skull, a
   dead saguaro's ribs, drifting tumbleweeds.
5. **Atmosphere** — additive warm sun glow + heat shimmer + dust + circling vultures.

## Encounter (mobs.cpp)
- **8 "Withered"** — the `Mob` horde, level-aware spawn set (`cactus_defs`), among the cacti.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Withered*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–96 and the `Boss` are untouched. `mob_level` now covers
  `… || WHALING || CACTUS`. **No new global struct/vector** — the nine saguaro positions are a
  fixed array shared by build (obstacles) + draw; the central saguaro + mesa obstacles and the
  lights are pushed in `build_cactus`; companions/rocks/shimmer are seeded; only `s_wisps` reused.
  A new `draw_saguaro` helper draws each cactus. Lights built in `build_cactus` (so
  `build_crystals` / `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_cactus` lays
  the desert hardpan.
- CLI: `cactus` / `saguaro` / `sonoran` launches straight into it.
