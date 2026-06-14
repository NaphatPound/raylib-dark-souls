# Colliers' Wood — Level Design (LEVEL_COLLIER = 51)

A charcoal-burner's camp in a smoke-hazed forest clearing: great turf-covered earthen
charcoal mounds (meilers) smouldering from their chimney vents, stacks of cordwood,
black charcoal piles, a turf-roofed collier's hut, and axes sunk in stumps. The
forty-eighth **non-boss** level — the camp's burnt dead ("The Charred") rise from the
ash. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **charcoal-burner's camp** silhouette
(smoking turf meilers + cordwood stacks) found in none of the other fifty-one levels.
Distinct from the Kiln Yard (brick bottle-ovens), the Timber Sawmill (a log yard +
blade) and the Foundry.

## Theme & palette
- **Mood:** acrid, slow, smouldering — grey woodsmoke hanging thick over earthen
  mounds glowing red at their vents.
- **Light (render.cpp):** hazy filtered sun, warm smoke-filtered light `(0.92, 0.82,
  0.66)`, dim warm ambient, grey woodsmoke haze (`~#766A5C`, density `0.0130`). Reuses
  `sky_storm.fs`; **dry** — its own ashy clearing is the floor.
- **Glow:** ember-red charcoal `(1.00, 0.50, 0.20)`.

## Geometry (arena.cpp :: build_collier / draw_collier / draw_meiler / collier_layout)
Reuses `s_cyl` (cordwood drum / chimney / stumps), `s_dome` (turf domes / charcoal
piles / hut turf-roof), `s_column` (floor / scorch rings / axe heads), `draw_bone_seg`
(axe hafts), and **`draw_logpile`** (cordwood stacks) + **`draw_cottage`** (the hut).
1. **Charcoal mounds (meilers)** — `draw_meiler`: a stacked-cordwood drum buried under
   a turf dome with a turf patch and a central chimney vent. A central great meiler +
   5 seeded smaller ones; each a collision obstacle, smouldering with an additive vent
   ember + a rising smoke column.
2. **Cordwood stacks** — `draw_logpile` pyramids of split logs; black **charcoal
   piles** scattered.
3. **Collier's hut** — a `draw_cottage` cabin under a turf dome roof (obstacle).
4. **Tools** — axes sunk in chopping stumps.
5. **Smoke FX** — additive vent embers + grey smoke columns from every mound.

## Encounter (mobs.cpp)
- **8 "Charred"** — the `Mob` horde, level-aware spawn set (`collier_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Charred*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–50 and the `Boss` are untouched. `mob_level` now covers
  `… || BAMBOO || COLLIER`. **No new global struct/vector** — `collier_layout` fills a
  local `std::vector<Vector4>` (mounds: xyz + w=scale) shared by build+draw; the
  hut/stacks are fixed; only `s_wisps` reused. Ember lights built in `build_collier`
  (so `build_crystals`/`draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_collier` lays the ashy floor.
- CLI: `collier` / `charcoal` / `meiler` launches straight into it.
