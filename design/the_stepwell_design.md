# The Stepwell — Level Design (LEVEL_STEPWELL = 89)

A flooded Indian **baori** under a bright sandstone sun: a square funnel of crisscrossing stone
**stairs** descends on three sides to a turquoise **pool** at the bottom, faced on the fourth
side by a multi-story pillared **pavilion** crowned with domed **chhatri** kiosks; greenery
sprouts from the cracks and dust drifts in the warm light. The eighty-sixth **non-boss** level —
shades risen from the water ("The Drowned") wait at the bottom. Clear them all to win; the
bonfire then ignites as usual.

New procedural geometry, not a recolour — a **stepwell** silhouette (a square crisscross-stair
funnel + a pillared pavilion + chhatris, with water at the base) found in none of the other
eighty-nine levels. A **WET** level — the reflective water plane *is* the pool at the bottom of
the well (the fighting floor), with the terraced stair walls rising around the rim. Distinct from
the **round seating bowls** of the Amphitheatre and Sunken Colosseum (smooth concentric benches,
circular, a central stage) and from the **convex stepped pyramid** of the Desert Ziggurat — this
is a *square, concave, descending* stair funnel with a flooded floor and an Indian pavilion.

## Theme & palette
- **Mood:** ancient, sunken, hushed — cool water at the bottom of a deep sandstone well.
- **Light (render.cpp):** bright high sun `(0.42,0.78,0.30)`, warm sandstone glow
  `(1.16,1.02,0.80)`, warm bounced fill for the deep court, dusty golden haze (`~#CCB894`,
  density `0.0062`). Reuses `sky_ice.fs` (clear) + `water_ice.fs` (turquoise). **WET**
  (`draw_water` runs; `draw_stepwell` lays only the stone).
- **Glow:** warm torch / sandstone `(1.00,0.66,0.34)`.

## Geometry (arena.cpp :: draw_chhatri / build_stepwell / draw_stepwell)
Reuses `s_column` (terrace treads / crisscross step-blocks / pavilion slabs+walls+lintels /
chhatri roofs), the lit `s_cyl` (pavilion + chhatri pillars / finials), `s_cone` (pavilion
pointed-arch shading), `s_dome` (chhatri domes / crack greenery), and `draw_bone_seg` (the
wellhead kerb ring).
1. **The pool** — the WET water plane (turquoise `water_ice`), the open fighting floor at the
   bottom of the well, ringed by a low stone wellhead kerb.
2. **Stair walls** — nine terraced tiers rising on the back (−z) and both sides (±x), each a
   square tread bar with a row of crisscross step-blocks offset half a step per tier (the
   characteristic zigzag).
3. **Pavilion** — a four-story pillared gallery on the fourth side (+z, behind the spawn), each
   story with a pillar row + lintel + pointed-arch shade + back wall, crowned by three chhatri
   kiosks; two more chhatris where the back stair walls meet.
4. **Life** — greenery sprouting from the cracks, warm dust motes, torch glow on the tiers.

## Encounter (mobs.cpp)
- **8 "Drowned"** — the `Mob` horde, level-aware spawn set (`stepwell_defs`), at the flooded
  bottom. Same lightweight AI, lock-on, aggregate HUD bar (named *The Drowned*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–88 and the `Boss` are untouched. `mob_level` now covers
  `… || PAGODA || STEPWELL`. **No new global struct/vector** — the terraces/pavilion/chhatris are
  parametric (fixed constants), the confining obstacle ring + pavilion obstacle are pushed in
  `build_stepwell`, greenery is seeded; only `s_wisps` reused. Torch + pool lights built in
  `build_stepwell` (so `build_crystals` / `draw_crystals` early-return). A new `draw_chhatri`
  helper (pillars + roof slab + dome + finial) is shared by the pavilion + corner kiosks.
- WET level: `draw_water` is NOT in its dry early-return (the pool draws); `draw_crystals` IS.
- A ring of obstacles confines the player to the flooded bottom (spawn sits safely inside it).
- CLI: `stepwell` / `baori` / `baoli` launches straight into it.
