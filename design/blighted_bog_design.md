# Blighted Bog — Level Design (LEVEL_BOG = 31)

A fetid swamp drowned in murky green water: weeping dead willows draped in moss,
cypress knees breaking the surface, lily pads, and a sagging witch's hut on stilts.
The twenty-eighth **non-boss** level — the drowned dead ("The Mired") rise from the
mire. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **swamp** silhouette (weeping willows +
cypress knees + a stilt hut over open water) found in none of the other thirty-one
levels. The first **WET** new level in a while (the last four were dry), so the
reused water plane returns as the murky floor.

## Theme & palette
- **Mood:** stagnant, choking, alive with rot — weak green-filtered light through a
  thick miasma, the dark water lit only by drifting swamp-gas wisps.
- **Light (render.cpp):** weak filtered daylight, sickly green-tinged light
  `(0.78, 0.84, 0.66)`, dim murky ambient, thick green miasma fog (`~#566B52`,
  density `0.0150` — the densest yet). Reuses `sky_storm.fs`; **WET** — the reused
  dark `water_storm` plane is the murky floor.
- **Glow:** green will-o'-wisp / swamp gas `(0.48, 0.78, 0.42)`.

## Geometry (arena.cpp :: build_bog / draw_bog / draw_willow)
Reuses `draw_bone_seg` (willow trunks/branches/moss/roots/reeds/ladder), `s_cyl`
(stump / lily pads / stilts), `s_dome` (mud banks / moss cap), `s_cone` (cypress
knees), `s_column` (deck), and **`draw_cottage`** (the raised cabin).
1. **Weeping willows** — 12 `draw_willow` trees: a gnarled tapering trunk (chained
   `draw_bone_seg`) crowned with branches that arc up then sweep down to the water,
   with hanging moss strands; each a collision obstacle, kept clear of the centre
   and the spawn aisle.
2. **Central great mossy stump** — a wide stump + domed moss cap + radiating roots;
   the landmark and central obstacle.
3. **Witch's stilt hut** — 4 stilts + a deck + a raised cabin (reusing
   `draw_cottage`) + a ladder; an off-centre landmark obstacle.
4. **Surface life** — cypress knees (small `s_cone` spikes), floating lily pads,
   reed tufts, and mud banks poking above the water (no ground slab — the water
   plane is the floor).
5. **Swamp gas** — additive green bubbles rising + drifting fireflies at the wisps.

## Encounter (mobs.cpp)
- **8 "Mired"** — the `Mob` horde, level-aware spawn set (`bog_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Mired*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–30 and the `Boss` are untouched. `mob_level` now covers
  `… || HENGE || BOG`. New `Willow` struct + `s_willows` vector (cleared in
  `unload`); swamp-gas lights built in `build_bog` (so `build_crystals`/
  `draw_crystals` early-return for this level).
- WET level: `draw_water` still draws the murky plane (NOT in the dry early-return);
  `draw_bog` lays only props over it.
- CLI: `bog` / `swamp` / `marsh` launches straight into it.
