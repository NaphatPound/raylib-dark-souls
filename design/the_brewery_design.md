# The Brewery — Level Design (LEVEL_BREW = 49)

A working distillery in warm coppery gloom: a great bulbous **copper still** with a
swan-neck pipe arcing to a condenser, rows of hoop-bound fermentation vats (some open
and frothing), wide mash tuns, cask-stack pyramids, grain sacks, copper piping and
hop garlands. The forty-sixth **non-boss** level (and the **50th level overall**) —
the brewery's drowned-in-drink dead ("The Soused") rise among the vats. Clear them all
to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **brewery/distillery** silhouette (a
copper pot still + fermentation vats + mash tuns) found in none of the other
forty-nine levels. Distinct from the Kiln Yard (bottle ovens), the Foundry (blast
furnace + trip-hammer) and the Sawmill.

## Theme & palette
- **Mood:** warm, malty, dim — coppery firelight off the still, steam curling and froth
  brimming over open vats.
- **Light (render.cpp):** dim hall light, warm coppery light `(0.92, 0.78, 0.60)`,
  warm working ambient, malty steam haze (`~#4C4238`, density `0.0105`). Reuses
  `sky_storm.fs`; **dry** — its own brewhouse floor is the floor.
- **Glow:** warm copper / furnace `(1.00, 0.68, 0.36)`.

## Geometry (arena.cpp :: build_brew / draw_brew / draw_still / draw_vat / brew_layout)
Reuses `s_cyl` (still pot/neck/vats/tuns/pipes), `s_dome` (onion bulb / vat lids /
sacks / hops), `s_column` (floor), `draw_bone_seg` (swan-neck pipe / copper runs / hop
cords), and **`draw_logpile`** (cask-stack pyramids).
1. **Copper still** — `draw_still`: a brick furnace base + a bulbous onion pot + a
   tapering swan-neck rising to an arcing lyne pipe that drops into a wooden condenser
   (worm tub). The central landmark obstacle, with an additive furnace glow + steam.
2. **Fermentation vats** — `draw_vat`: 8 hoop-bound upright casks (every third left
   open and brimming with additive froth); collision obstacles, linked to the still by
   copper pipes.
3. **Brewing floor** — wide shallow mash tuns, cask-stack pyramids, grain sacks, and
   hop garlands strung along the wall.
4. **Heat FX** — additive furnace glow, rising still-steam, vat froth, and copper
   glints.

## Encounter (mobs.cpp)
- **8 "Soused"** — the `Mob` horde, level-aware spawn set (`brew_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Soused*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–48 and the `Boss` are untouched. `mob_level` now covers
  `… || HIVE || BREW`. **No new global struct/vector** — `brew_layout` fills a local
  `std::vector<Vector4>` (vats: xyz + w=scale) shared by build+draw; the still/tuns are
  fixed/seeded; only `s_wisps` reused. Copper lights built in `build_brew` (so
  `build_crystals`/`draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_brew` lays the brewhouse floor.
- CLI: `brewery` / `distillery` / `vats` launches straight into it.
