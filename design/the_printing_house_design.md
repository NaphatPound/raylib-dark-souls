# The Printing House — Level Design (LEVEL_PRINT = 100)

A dim, candlelit print workshop: heavy timber **platen presses** with their great screws and bar
levers stand in rows beneath an overhead canopy of freshly **printed sheets** hung to dry, while
slanted **type cases**, composing tables with imposing stones, stacked **paper reams**, ink
barrels and leather ink-balls fill the floor and a back shelf holds finished books. The
ninety-seventh **non-boss** level — the workshop's shades have risen ("The Redacted"). Clear them
all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **printing-workshop** silhouette (rows of screw
presses + a canopy of drying sheets) found in none of the other one hundred levels. A **DRY**
level (the plank floor). A printing **craft** scene — distinct from the **Forgotten Archive** (a
library of book-laden shelves) and the **Great Loom** (a textile workshop of warp threads): this
is the machinery of the press and reams of paper.

## Theme & palette
- **Mood:** close, warm, ink-stained — the hum of a workshop turning words into paper.
- **Light (render.cpp):** soft slanted window light `(0.36,0.70,0.42)`, warm lamp daylight
  `(1.04,0.92,0.74)`, dim warm fill, warm ink-and-dust haze (`~#948A75`, density `0.0095`).
  Reuses `sky_ice.fs`; **DRY** (`draw_print` lays the floor; `water_storm.fs` placeholder is
  unused).
- **Glow:** warm workshop lamp `(1.00,0.74,0.40)`.

## Geometry (arena.cpp :: draw_press / draw_typecase / build_print / draw_print)
Reuses `s_column` (floor + plank seams / press frames + beds + platens + flaps / type cabinets /
tables / paper reams / book shelf + books / proofing desk), the lit `s_cyl` (press screws + bar
levers / table legs / ink barrels / ink-ball handles), `s_dome` (lever knobs / ink-balls / ink
stains), and `draw_bone_seg` (the drying lines).
1. **Presses** (`draw_press`) — two uprights + a head beam + a base + a type bed + the great brass
   **screw** with an iron **bar lever** and a platen and a raised tympan flap. A great central
   press + four row presses (obstacles).
2. **Drying canopy** — six strung lines hung with gently-swaying printed sheets across the
   ceiling (the signature overhead visual).
3. **Type cases** (`draw_typecase`) — slanted cabinets with gridded type compartments along the
   back (obstacles), plus two composing tables with imposing stones.
4. **Stores** — stacked paper reams, ink barrels with iron hoops, leather ink-balls on a stand,
   a back shelf of finished books, a proofing desk with a candle and a fresh sheet.
5. **Atmosphere** — additive warm lamp glow + a candle flame + drifting paper dust.

## Encounter (mobs.cpp)
- **8 "Redacted"** — the `Mob` horde, level-aware spawn set (`print_defs`), among the presses.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Redacted*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–99 and the `Boss` are untouched. `mob_level` now covers
  `… || BASIL || PRINT`. **No new global struct/vector** — the workshop is parametric; the press
  + type-case obstacles and the lights are pushed in `build_print`; reams/barrels/dust are seeded;
  only `s_wisps` reused. New `draw_press` / `draw_typecase` helpers. Lights built in `build_print`
  (so `build_crystals` / `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_print` lays
  the plank floor.
- CLI: `printing` / `press` / `printworks` launches straight into it.
