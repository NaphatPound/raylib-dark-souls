# Chasm Bridge — Level Design (LEVEL_BRIDGE = 21)

A colossal ancient stone bridge spanning a bottomless chasm. The eighteenth
**non-boss** level: travellers trapped on the span ("The Stranded") still wander it.
Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **bridge-over-void** linear composition
found in none of the other twenty-one levels. The sixth **dry** level (the deck is
the floor; the void falls away on both sides).

## Theme & palette
- **Mood:** vertiginous, windswept, lonely — a long causeway into the mist over a
  bottomless drop.
- **Light (render.cpp):** flat pale stormy daylight, cool windy ambient, grey-blue
  chasm haze (`~#76808E`, density `0.0085`). Reuses `sky_storm.fs`.
- **Glow:** warm hanging-lantern point lights `(0.95, 0.72, 0.40)`.

## Geometry (arena.cpp :: build_bridge / draw_bridge)
Reuses `s_column` (deck / railings / towers / arch / gaps), `s_cyl` (balusters /
lantern ropes), `s_cone` (tower roofs), and `draw_bone_seg` (catenary chains).
1. **Deck** — a long railed stone slab (the floor) with plank seams and a couple of
   broken gaps. **Railing-wall obstacles** along both long edges confine play to the
   deck strip (the circular arena boundary can't, so a row of edge obstacles does).
2. **Gate-towers** — four tall towers (cone roofs, arrow slits) at the deck corners,
   framing the span.
3. **Great chains** — iron chains drooping in catenary arcs between the tower tops
   along each side (chained `draw_bone_seg` segments following a sag curve).
4. **Hanging lanterns** — boxes on ropes hung from the chains (the warm lights).
5. **Midspan gate-arch** — a ruined arch (two pier-obstacles + a crown) you pass
   through.
6. **Cloud band** — a faint additive cloud layer far below the void.

Also fixes a latent bug: `obstacles` is now `clear()`-ed at the top of each level
load (it previously accumulated across loads — stray obstacles bled between levels).

## Encounter (mobs.cpp)
- **7 "Stranded"** — the `Mob` horde, level-aware spawn set (`bridge_defs`) kept on
  the deck (|x| ≤ 7). Same lightweight AI, lock-on, aggregate HUD bar (named *The
  Stranded*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–20 and the `Boss` are untouched. `mob_level` now covers
  `… || GARDEN || BRIDGE`.
- CLI: `bridge` / `chasm` / `span` launches straight into it.
