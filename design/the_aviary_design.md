# The Aviary — Level Design (LEVEL_AVIARY = 55)

A colossal iron birdcage: a tall cylindrical cage of vertical bars (ringed by hoops,
roofed by a converging domed cap) enclosing the arena, with bare perch-trees, giant
woven nests of eggs, hanging seed-feeders, perched and wheeling birds, and a litter of
feathers. The fifty-second **non-boss** level — the cage's flightless dead ("The
Plumed") rise among the nests. Clear them all to win; the bonfire then ignites as
usual.

New procedural geometry, not a recolour — a **giant barred cage** silhouette (vertical
cage bars + a converging dome cap + woven nests) found in none of the other fifty-five
levels. Distinct from the Grand Conservatory (a low glass-paned dome with palms) and
the Gaol (square barred cells).

## Theme & palette
- **Mood:** hushed, dusty, watched — soft light slanting through tall bars, feathers
  drifting and birds wheeling under the cage roof.
- **Light (render.cpp):** soft light through bars, warm dusty light `(0.92, 0.86,
  0.72)`, dim shaded ambient, dusty cage haze (`~#6B665C`, density `0.0105`). Reuses
  `sky_storm.fs`; **dry** — its own cage floor is the floor.
- **Glow:** warm dusty cage light `(0.90, 0.80, 0.58)`.

## Geometry (arena.cpp :: build_aviary / draw_aviary / draw_nest / aviary_layout)
Reuses `s_cyl` (cage bars / feeder cups / nest posts / finial), `s_dome` (nests /
eggs / birds / cap apex / floor patches), `draw_bone_seg` (cage hoops & dome arcs /
nest twigs / feeder chains), and **`draw_ptree`** (the bare perch-trees).
1. **The cage** — 34 tall vertical iron bars at R≈22, three horizontal hoops, and a
   17-meridian converging **domed cap** topped by a finial. The signature.
2. **Nests** — `draw_nest`: a twig bowl on a post with a dark hollow, eggs and twig
   pokes. A **central great nest** (the landmark obstacle) + seeded nests.
3. **Perch-trees** — bare `draw_ptree` trees for the birds; collision obstacles.
4. **Aviary life** — hanging seed-feeders on chains, perched birds on the middle hoop,
   straw and feather litter.
5. **Atmosphere** — additive wheeling pale **birds**, dusty light shafts, and warm
   cage glow.

## Encounter (mobs.cpp)
- **8 "Plumed"** — the `Mob` horde, level-aware spawn set (`aviary_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Plumed*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–54 and the `Boss` are untouched. `mob_level` now covers
  `… || BELL || AVIARY`. **No new global struct/vector** — `aviary_layout` fills a
  local `std::vector<Vector4>` (nodes: xyz, w<0 = perch-tree of height -w, w>0 = nest
  of scale w) shared by build+draw; the cage is a fixed loop; only `s_wisps` reused.
  Cage lights built in `build_aviary` (so `build_crystals`/`draw_crystals`
  early-return).
- Dry level: `draw_water` early-returns; `draw_aviary` lays the cage floor.
- CLI: `aviary` / `birdcage` / `rookery` launches straight into it.
