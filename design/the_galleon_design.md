# The Galleon — Level Design (LEVEL_GALLEON = 68)

The storm-tossed main deck of a great sailing ship: a planked deck flanked by bulwarks,
three masts rigged with yards, furled sails and shrouds, a raised quarterdeck with the
ship's wheel, cannons run out along the rails, a bowsprit, barrels and a hatch — all
riding a dark heaving sea under driving rain. The sixty-fifth **non-boss** level — the
ship's drowned crew ("The Mutinous") rise on deck. Clear them all to win; the bonfire
then ignites as usual.

New procedural geometry, not a recolour — a **ship-deck** silhouette (deck + masts +
rigging + wheel + cannons over the sea) found in none of the other sixty-eight levels.
A **DRY** level (the planked deck is the floor; a dark sea slab surrounds it). Deliberately
distinct from the Shipwreck Cove (foggy cove of broken, beached hulls) and the Skydock
(airship mooring): here you fight aboard one intact ship at sea.

## Theme & palette
- **Mood:** pitching, salt-lashed, dramatic — a fight on deck in a gale.
- **Light (render.cpp):** low stormy light `(0.30,0.66,0.40)`, cold grey sea-storm light
  `(0.70,0.74,0.82)`, dim cool fill, grey sea spray (`~#667684`, density `0.0105`).
  Reuses `sky_storm.fs`; **DRY** (`draw_galleon` lays the deck over a dark sea slab).
- **Glow:** warm deck-lantern `(1.00,0.75,0.40)`.

## Geometry (arena.cpp :: build_galleon / draw_galleon)
Reuses the lit `s_column` (deck / hull / bow / bulwarks / sails / quarterdeck / cannon
carriages / hatch), `s_cyl` (masts / yards / bowsprit / wheel post / cannon barrels /
barrels), `s_torus` (the wheel rim), and `draw_bone_seg` (rigging shrouds).
1. **Hull & deck** — a dark surrounding sea slab, angled hull sides, a diamond bow, and the
   planked deck floor with plank lines and side bulwarks (the rails confine play).
2. **Masts** — fore/main/mizzen poles (the mainmast is the central obstacle), each with two
   yards, furled sails, and three rigging shrouds to the rails.
3. **Quarterdeck** — a raised stern deck with a vertical ship's wheel (`s_torus` + 4 spokes)
   on a binnacle post.
4. **Armament & clutter** — six cannons run out along the rails, barrels, a deck hatch, an
   angled bowsprit.
5. **Atmosphere** — additive warm lanterns + whitecap spray on the sea + driving rain.

## Encounter (mobs.cpp)
- **8 "Mutinous"** — the `Mob` horde, level-aware spawn set (`galleon_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Mutinous*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–67 and the `Boss` are untouched. `mob_level` now covers
  `… || PINES || GALLEON`. **No new global struct/vector** — all ship parts are fixed;
  bulwark obstacles are sampled along the rails in `build_galleon` (so play stays on the
  deck strip — the circular boundary can't bound a rectangle); only `s_wisps` reused.
  Lantern lights built in `build_galleon` (so `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns (the ship rides its own dark sea slab, not the
  reflective water plane); `draw_galleon` lays the deck.
- CLI: `galleon` / `ship` / `deck` launches straight into it.
