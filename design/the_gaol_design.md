# The Gaol — Level Design (LEVEL_GAOL = 33)

A grim torch-lit prison courtyard ringed by iron-barred stone cells, hung with
swaying iron gibbet cages and dominated by a central gallows scaffold. The thirtieth
**non-boss** level — the prisoners ("The Condemned") break their chains. Clear them
all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **gaol** silhouette (barred cells +
hanging gibbet cages + a gallows) found in none of the other thirty-three levels.
Distinct from the Throne Hall (open colonnade), Ossuary (bone crypt) and Library
(bookshelves).

## Theme & palette
- **Mood:** oppressive, shadowed, cruel — cold stone in weak skylight, the courtyard
  lit by guttering torch braziers and the slow creak of swaying cages.
- **Light (render.cpp):** weak cold skylight, pale cold stone light `(0.66, 0.66,
  0.72)`, dim shadowed ambient, dark dungeon murk fog (`~#282628`, density `0.0115`).
  Reuses `sky_storm.fs`; **dry** — its own flagstones are the floor.
- **Glow:** orange torch / brazier `(1.00, 0.58, 0.24)`.

## Geometry (arena.cpp :: build_gaol / draw_gaol / draw_cell / draw_gibbet / gaol_layout)
A fixed perimeter layout (`gaol_layout`) is shared by `build_gaol` (obstacles +
torch lights) and `draw_gaol` (geometry) so the two never drift. Reuses `s_column`
(walls / rails / scaffold / flagstones), `s_cyl` (bars / posts / braziers),
`s_dome` (brazier bowls / cage occupants), `draw_bone_seg` (chains / nooses).
1. **Cells** — 11 `draw_cell` stone cells (back row + both side walls, the front
   left open as the spawn aisle): back + side walls + roof + an **iron-barred front**
   (top & bottom rails + 7 vertical bars), drawn in a yawed rlgl frame so the bars
   face the courtyard. Each cell is a collision obstacle.
2. **Gibbets** — 4 `draw_gibbet` hanging cages: an L-frame (post + arm) from which an
   iron cage hangs on a chain and **sways with `s_time`**, a dark slumped occupant
   inside. The posts are small obstacles.
3. **Central gallows scaffold** — a raised platform on legs + steps + an upright frame
   with a crossbeam and two swaying nooses. The landmark and central obstacle.
4. **Surround** — a broken outer stone wall behind the cells, worn flagstone patches.
5. **Torchlight** — stone braziers (bowl on a post) with additive flickering flames,
   warm glow, and drifting embers.

## Encounter (mobs.cpp)
- **8 "Condemned"** — the `Mob` horde, level-aware spawn set (`gaol_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Condemned*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–32 and the `Boss` are untouched. `mob_level` now covers
  `… || SAWMILL || GAOL`. **No new global struct/vector** — the layout is computed on
  the fly (local `std::vector<Vector4>`); only `s_wisps` is reused (already cleared).
  Torch lights built in `build_gaol` (so `build_crystals`/`draw_crystals`
  early-return for this level).
- Dry level: `draw_water` early-returns; `draw_gaol` lays the flagstone floor.
- CLI: `gaol` / `prison` / `dungeon` launches straight into it.
