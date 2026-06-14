# The Alchemist's Laboratory — Level Design (LEVEL_ALCHEMY = 102)

A cluttered, fume-choked arcane chamber: a great domed **athanor** furnace glows at the back,
bubbling glass **alembics and retorts** drip luminous green, purple and amber elixirs along the
benches, shelves of glowing **potion jars** line the walls, a chalk **pentagram** ringed with
candles is inscribed on the dark flagstones, a glowing **orb** floats on a central pedestal, and
a stuffed crocodile and a skeleton hang from the rafters above a workbench of crystal ball and
grimoires. The ninety-ninth **non-boss** level — the lab's failed experiments have risen ("The
Transmuted"). Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — an **alchemy-laboratory** silhouette (an athanor +
distillation glassware + jar shelves + a pentagram) found in none of the other one hundred two
levels. A **DRY** level (flagstones). An arcane-**craft** scene with **eerie green point-lighting**
(`pcol = (0.45,0.92,0.55)`, the first green light in the set) — distinct from **The Brewery**
(beer fermenting), **The Foundry** (molten metal) and the **Forgotten Archive** (books): here the
glow comes from glowing concoctions, not fire or daylight.

## Theme & palette
- **Mood:** secretive, noxious, uncanny — colour-glow apparatus in a smoke-veiled gloom.
- **Light (render.cpp):** weak slanted lamplight `(0.30,0.66,0.42)`, dim sickly daylight
  `(0.84,0.86,0.74)`, murky green-grey fill, dark vaporous haze (`~#5C6B66`, high density
  `0.0115`). Reuses `sky_storm.fs` (dark enclosed); **DRY** (`draw_alchemy` lays the flagstones).
- **Glow:** eerie green elixir `(0.45,0.92,0.55)`.

## Geometry (arena.cpp :: draw_alembic / draw_jarshelf / build_alchemy / draw_alchemy)
Reuses `s_cyl` (alembic braziers / jars / pedestal / chimney / workbench legs / mortar), `s_dome`
(alembic bulbs+heads+flasks via flipped hemispheres / athanor dome / crystal ball / cauldron),
`s_column` (floor + flagstone grid / athanor body + fire door / shelves / workbench / croc /
grimoire stacks), `s_cone` (croc tail), `s_torus` (the two pentagram rings), and `draw_bone_seg`
(alembic beaks / pentagram star lines / tripod legs / hanging cords + skeleton ribs).
1. **Alembics** (`draw_alembic`) — a brazier + a glass bulb (two flipped hemispheres) holding a
   coloured elixir + an alembic head + a curved beak dripping into a receiving flask. Three on
   each of two benches (obstacles) + one atop the athanor.
2. **The athanor** (the back landmark obstacle) — a domed stone furnace with a chimney and a
   glowing fire-door.
3. **The pentagram** — two chalk `s_torus` rings + a five-pointed star (every-2nd-vertex
   `draw_bone_seg` lines) ringed with candles, with a glowing orb on a central pedestal (obstacle).
4. **Apparatus & decor** — `draw_jarshelf` shelves of six-colour potion jars, a tripod cauldron,
   a workbench with mortar/crystal-ball/grimoire (obstacle), a hanging stuffed crocodile and
   skeleton, stacks of grimoires.
5. **Atmosphere** — additive orb + elixir + cauldron glows (green/purple/amber), the athanor
   fire, and rising green fumes.

## Encounter (mobs.cpp)
- **8 "Transmuted"** — the `Mob` horde, level-aware spawn set (`alchemy_defs`), among the
  apparatus. Same lightweight AI, lock-on, aggregate HUD bar (named *The Transmuted*),
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–101 and the `Boss` are untouched. `mob_level` now covers
  `… || GER || ALCHEMY`. **No new global struct/vector** — the lab is parametric; the athanor +
  pedestal + bench + workbench + cauldron obstacles and the lights are pushed in `build_alchemy`;
  grimoires/fumes are seeded; only `s_wisps` reused. New `draw_alembic` / `draw_jarshelf` helpers.
  Lights built in `build_alchemy` (so `build_crystals` / `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_alchemy` lays
  the flagstones.
- CLI: `alchemy` / `alchemist` / `athanor` launches straight into it.
