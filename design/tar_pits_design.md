# The Tar Pits — Level Design (LEVEL_TAR = 34)

A desolate field of bubbling black pitch lakes worked by creaking "nodding donkey"
pump derricks, dotted with tar-blackened dead trees and venting acrid sulfur. The
thirty-first **non-boss** level — the tar-drowned dead ("The Engulfed") drag
themselves free. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **tar pits** silhouette (glossy black
pools + rocking pumpjacks + a central tar geyser) found in none of the other
thirty-four levels. Distinct from the Boneyard (dry ash + full skeletons) and the
Bog (green swamp water).

## Theme & palette
- **Mood:** hot, acrid, slow — a brown-yellow sulfurous haze over near-black pools
  that swell and pop, the only motion the nodding pumps and rising fumes.
- **Light (render.cpp):** low hot haze-filtered sun, warm brown-amber light
  `(0.96, 0.80, 0.52)`, dim warm ambient, brown-yellow sulfurous haze (`~#665744`,
  density `0.0135`). Reuses `sky_storm.fs`; **dry** — its own cracked-earth slab is
  the floor.
- **Glow:** amber-sulfur seep `(0.95, 0.66, 0.30)`.

## Geometry (arena.cpp :: build_tar / draw_tar / draw_pump / tar_layout)
A fixed/seeded layout (`tar_layout`) is shared by `build_tar` (obstacles + lights)
and `draw_tar` (geometry). Reuses `s_cyl` (pools / wellheads / crank), `s_dome`
(mounds / geyser), `s_column` (ground / pump frame), `draw_bone_seg` (Samson legs),
and **`draw_ptree`** (the petrified-tree trunk, drawn near-black for tar-soaked dead
trees).
1. **Tar pools** — overlapping near-black flat discs forming irregular lakes, with an
   additive moving sheen and **swelling/popping bubbles** (animated).
2. **Pumpjacks** — 3 `draw_pump` "nodding donkeys": a Samson A-frame carrying a
   walking beam that **rocks about its pivot with `s_time`** (horsehead front,
   counterweight crank rear) over a wellhead. The animated signature; each a
   collision obstacle (drawn with a nested rlgl matrix for the rocking beam).
3. **Central tar geyser** — a pitch mound + vent erupting a dark additive jet.
4. **Dead trees** — 9 tar-blackened `draw_ptree` trunks (collision obstacles).
5. **Dressing** — asphalt seep mounds, dried-earth patches, and additive sulfur
   smoke at the seeps.

## Encounter (mobs.cpp)
- **8 "Engulfed"** — the `Mob` horde, level-aware spawn set (`tar_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Engulfed*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–33 and the `Boss` are untouched. `mob_level` now covers
  `… || GAOL || TAR`. **No new global struct/vector** — `tar_layout` fills local
  `std::vector<Vector4>` lists shared by build+draw; only `s_wisps` reused. Sulfur
  lights built in `build_tar` (so `build_crystals`/`draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_tar` lays the cracked-earth floor.
- CLI: `tar` / `tarpit` / `pitch` launches straight into it.
