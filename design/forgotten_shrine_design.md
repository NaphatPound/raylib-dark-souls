# Forgotten Shrine — Level Design (LEVEL_SHRINE = 10)

A misty mountain shrine standing in a still reflecting pool. The seventh
**non-boss** level: lost pilgrim-husks ("The Lost") haunt the avenue and grounds.
Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — its signature is a **processional axis**:
an avenue of repeated vermillion torii gates leading to a tiered pagoda pavilion.
That composed-corridor structure (rather than the scatter-on-a-plane of most
levels) plus the gate/pagoda silhouette make it unlike any of the other ten. The
shrine stands *in* the reused reflective water (Itsukushima-style), so the gates
mirror in the pool.

## Theme & palette
- **Mood:** quiet, sacred, abandoned — soft dusk mist over a flooded shrine.
- **Light (render.cpp):** soft low dusk light, cool misty ambient, grey-blue
  mountain haze (`~#80858E`, density `0.0110`). Reuses `sky_storm.fs` and the calm
  `water.fs` as the reflecting pond.
- **Glow:** warm paper-lantern light `(0.98, 0.74, 0.42)` from the stone lanterns +
  pavilion, pooling on the water.

## Geometry (arena.cpp :: build_shrine / draw_shrine, helpers draw_torii / draw_lantern)
Reuses the lit primitives `s_cyl` (posts / lantern parts / pillars / trunks),
`s_column` (beams / boxes / roof tiers), `s_dome` (blossom canopies).
1. **Torii avenue** — six vermillion gates (round posts + kasagi/nuki beams,
   upward-swept ends, central plaque) along the z-axis through the pavilion, plus a
   larger grand entrance gate and one **iconic lone torii standing out in the water**.
2. **Pagoda pavilion** — a stepped stone base + wood floor + four vermillion
   pillars + a three-tier overhanging pagoda roof topped by a gold finial. The
   landmark and a collision obstacle.
3. **Stone lanterns** — ten classic lanterns (base + post + light box + cap)
   flanking the avenue; their boxes glow.
4. **Cherry-blossom trees** — three trunks with soft pink dome canopies for colour.

## Encounter (mobs.cpp)
- **7 "Lost"** — the `Mob` horde, level-aware spawn set (`shrine_defs`): a guardian
  on the path plus pilgrims across the grounds. Same lightweight AI, lock-on,
  aggregate HUD bar (named *The Lost*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–9 and the `Boss` are untouched. `mob_level` now covers
  `GRAVEYARD || FUNGAL || DESERT || WRECK || SANCTUM || CLOCK || SHRINE`.
- Keeps the reused water plane as the reflecting pond.
- CLI: `shrine` / `torii` launches straight into it.
