# The Jantar Mantar — Level Design (LEVEL_JANTAR = 90)

A field of giant open-air astronomical instruments under a bright clear midday: a colossal
stepped triangular sundial gnomon (**Samrat Yantra**) rises at the back, three cylindrical
**Rama Yantra** instruments (a ring of pillars around a central gnomon) stand on the plaza, a
**Jai Prakash** bowl with a crosshair wire and small angled **sundials** are scattered across a
cream stone court inlaid with a meridian and measurement lines. The eighty-seventh **non-boss**
level — shades drift among the instruments ("The Timeless"). Clear them all to win; the bonfire
then ignites as usual.

New procedural geometry, not a recolour — a **masonry observatory** silhouette (a giant
triangular gnomon + cylindrical yantras + a sundial plaza) found in none of the other ninety
levels. A **DRY** level (the stone plaza). **Deliberately distinct from the Astral Observatory**,
which is a brass **armillary sphere** (concentric spinning rings + telescope + orbiting planets):
this level uses **no armillary/ring geometry at all** — only the big stone instruments of Jaipur's
Jantar Mantar (triangular sundial, cylinders, bowl, angled dials).

## Theme & palette
- **Mood:** still, sun-blasted, precise — vast stone instruments measuring the noon sun.
- **Light (render.cpp):** high midday sun `(0.35,0.86,0.30)` for sharp shadows, bright warm
  daylight `(1.18,1.08,0.86)`, even warm fill, very faint dusty haze (`~#D2C7A8`, low density
  `0.0050`). Reuses `sky_ice.fs` (clear); **DRY** (`draw_jantar` lays the stone; `water_storm.fs`
  placeholder is unused).
- **Glow:** warm midday stone `(1.00,0.80,0.46)`.

## Geometry (arena.cpp :: draw_rama / build_jantar / draw_jantar)
Reuses `s_column` (plaza / gnomon slabs+treads / quadrant walls / Rama walls / dial gnomons /
ticks), the lit `s_cyl` (Rama pillars + central gnomons / Jai Prakash drum / dial bases),
`s_dome` (gnomon tips / bowl bead), and `draw_bone_seg` (floor measurement lines / crosshair
wires). **No `s_torus` / armillary rings** — kept clear of the observatory's signature.
1. **Samrat Yantra** (the back landmark, an obstacle) — a colossal right-triangle gnomon built
   from stacked slabs of decreasing length (vertical back + stepped hypotenuse), with ochre edge
   strips, stair treads climbing it, and two curved quadrant scale walls with hour ticks at its
   base.
2. **Rama Yantra ×3** (centre + two flanking, obstacles) — a low circular wall + a ring of
   pillars + radial floor lines + a central gnomon (the `draw_rama` helper).
3. **Jai Prakash** (an obstacle) — a stone drum with a dark bowl interior and a crosshair wire.
4. **Small sundials ×3** — a round base + a tilted gnomon + a ring of hour ticks.
5. **Plaza** — cream stone inlaid with a N–S meridian line and radial measurement lines; warm
   gnomon-tip glow + dust motes.

## Encounter (mobs.cpp)
- **8 "Timeless"** — the `Mob` horde, level-aware spawn set (`jantar_defs`, spread across the
  front/mid plaza so the back instruments stay clear). Same lightweight AI, lock-on, aggregate
  HUD bar (named *The Timeless*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–89 and the `Boss` are untouched. `mob_level` now covers
  `… || STEPWELL || JANTAR`. **No new global struct/vector** — the instruments are parametric
  constants; their obstacles + the lights are pushed in `build_jantar`; dust is seeded; only
  `s_wisps` reused. A new `draw_rama` helper draws each Rama Yantra. Lights built in
  `build_jantar` (so `build_crystals` / `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_jantar`
  lays the stone plaza.
- CLI: `jantar` / `sundial` / `yantra` launches straight into it.
