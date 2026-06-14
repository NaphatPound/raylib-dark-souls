# The Great Dam — Level Design (LEVEL_DAM = 71)

A massive curved buttressed dam wall holding back a reservoir: spillway curtains cascade
down its face into a stilling basin, sluice outlets gush forward, a parapet and gatehouse
crown the crest, and a valve house squats in the basin amid the tailwater channels. The
sixty-eighth **non-boss** level — the reservoir's drowned dead ("The Submerged") rise in
the basin. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **hydroelectric-dam** silhouette (a curved
concrete wall + spillways + sluices + crest works) found in none of the other seventy-one
levels. A **DRY** level (the concrete basin apron is the floor). Deliberately distinct from
the Cascade (a *natural* rock cliff + waterfall + pool), the Sunken Aqueduct (a row of
*arches*), the Watermill (a small wheel + race), and the Chasm Bridge (a span over a void):
this is a single colossal man-made dam.

## Theme & palette
- **Mood:** vast, damp, industrial — bright daylight on wet concrete, spray in the air.
- **Light (render.cpp):** bright overcast `(0.34,0.80,0.32)`, cool neutral concrete light
  `(0.94,0.97,1.00)`, cool fill, cool spray haze (`~#A3B3BD`, density `0.0088`). Reuses
  `sky_ice.fs`; **DRY** (`draw_dam` lays the basin apron).
- **Glow:** cool spray / water `(0.66,0.84,0.92)`.

## Geometry (arena.cpp :: build_dam / draw_dam)
Reuses the lit `s_column` for nearly everything (dam panels / buttresses / courses / parapet
/ valve house / sluice mouths / channels / floor), `draw_cottage` (crest gatehouse), and
immediate-mode `DrawCube`/`DrawSphereEx` for the water.
1. **The dam wall** — 18 tall panels placed along a parabola (the wall curves, ends pulled
   downstream), each yawed to follow the curve, with buttress ribs every third panel and
   four horizontal courses; a crest parapet and a gatehouse on top. The dam face is a solid
   barrier (obstacle line sampled along it in `build_dam`).
2. **Reservoir** — a dark high water band held behind the crest.
3. **Spillways** — three alpha-blended water curtains pouring down the face, with additive
   scrolling droplets; **sluices** — dark outlet mouths at the base with additive jets
   firing forward into the basin.
4. **Basin** — the concrete apron floor + tailwater outflow channels + a central **valve
   house** (the landmark obstacle).
5. **Atmosphere** — additive spray glow + a band of mist hanging over the basin.

## Encounter (mobs.cpp)
- **8 "Submerged"** — the `Mob` horde, level-aware spawn set (`dam_defs`), in the open basin
  in front of the dam. Same lightweight AI, lock-on, aggregate HUD bar (named *The
  Submerged*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–70 and the `Boss` are untouched. `mob_level` now covers
  `… || SPHINX || DAM`. **No new global struct/vector** — the dam is a deterministic
  parabola loop shared by build (face obstacles) + draw; only `s_wisps` reused. Spray lights
  built in `build_dam` (so `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_dam` lays the basin apron.
- CLI: `dam` / `reservoir` / `spillway` launches straight into it.
