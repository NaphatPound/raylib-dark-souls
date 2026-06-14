# Shipwreck Cove — Level Design (LEVEL_WRECK = 7)

A foggy, moonlit cove littered with the wrecks of drowned ships. The fourth
**non-boss** level: a horde of waterlogged husks ("The Drowned") wades the
shallows. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a wholly nautical silhouette (listing
ship hulls, leaning masts with crossbeams, plank piers on pilings, floating
barrels/crates) found in none of the other seven levels. It brings the reflective
water + a pale moon back (after the dry Desert Ziggurat), but the wreck geometry
makes it unmistakably its own place.

## Theme & palette
- **Mood:** still, cold, drowned. A pale moon over thick sea-mist; broken ships
  list half-submerged in glassy water.
- **Light (render.cpp):** cold blue-white moonlight, cool sea-mist ambient, thick
  blue-grey sea fog (`~#4D616F`, density `0.0110`). Reuses `sky_storm.fs` (overcast)
  with a pale moon disc, and `water.fs` (calm reflective, moon glade toward the
  drawn moon).
- **Lanterns:** warm hanging lantern lights `(0.95, 0.72, 0.38)` on the masts pool
  on the water and light nearby wrecks + fighters.

## Geometry (arena.cpp :: build_wreck / draw_wreck)
Reuses the lit primitives `s_column` (hull/deck/crate) and `s_cyl` (mast/piling/
barrel).
1. **Great central wreck** — a long listing hull + tall leaning mast + crossbeam.
   The landmark and a collision obstacle.
2. **Scattered wrecks** — ~3 smaller hulls, each rolled (listing) at a random
   angle, half-sunk, some masted, with broken rib planks fanning up one side.
3. **Piers** — two plank decks on two rows of pilings, reaching out from the
   shallows over the water.
4. **Barrels & crates** — floating props (cylinders + cubes) scattered around.
5. **Lanterns** — additive warm glows hung at the mast tops.

## Encounter (mobs.cpp)
- **7 "Drowned"** — the `Mob` horde, level-aware spawn set (`wreck_defs`), wading
  the shallows around the wrecks (clear of the central hull). Same lightweight AI,
  lock-on, aggregate HUD bar (named *The Drowned*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–6 and the `Boss` are untouched. `mob_level` now covers
  `GRAVEYARD || FUNGAL || DESERT || WRECK`; the Boss object stays inert here.
- CLI: `cove` / `wreck` / `shipwreck` launches straight into it.
