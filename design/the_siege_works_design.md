# The Siege Works — Level Design (LEVEL_SIEGE = 95)

A field of war machines under smoke and overcast: towering **trebuchets** with raised throwing
arms and counterweights, torsion **catapults** on wheels, a giant **ballista**, a wheeled
**siege tower** with a lowered drawbridge and a covered **battering ram** are arrayed across
churned mud before a battered, breached **curtain wall**; palisade stakes, boulder piles and
braziers ring the ground. The ninety-second **non-boss** level — a broken army has risen among
the engines ("The Routed"). Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **war-machine** silhouette (trebuchets + catapults +
a siege tower + a battering ram before a breached wall) found in none of the other ninety-five
levels. A **DRY** level (the churned mud). A military-**engineering** scene of *engines*, not
tents — distinct from the **Siege Camp** (a field of tents), the **Star Fort** (a fortification)
and the **Triumphal Arch** (a victory monument).

## Theme & palette
- **Mood:** grim, smoke-choked, violent — the machinery of a siege at its height.
- **Light (render.cpp):** flat overcast light `(0.40,0.60,0.42)`, cold grey daylight
  `(0.92,0.90,0.86)`, dull cool fill, grey-brown battle smoke (`~#8F8A80`, high density `0.0110`).
  Reuses `sky_storm.fs` (an overcast war sky); **DRY** (`draw_siege` lays the mud; `water_storm.fs`
  placeholder is unused).
- **Glow:** warm brazier fire `(1.00,0.56,0.28)`.

## Geometry (arena.cpp :: draw_trebuchet / draw_catapult / draw_ballista / build_siege / draw_siege)
Reuses `s_column` (mud + ruts / wall + battlements / siege-tower body+floors / penthouse / frames /
barrels / banner), the lit `s_cyl` (posts / pedestals / bows / barrels / stakes via `s_cone`),
`s_cone` (ram head / palisade stakes), `s_dome` (boulders / spent shot / sling stones / rubble),
`s_torus` (catapult + tower wheels), and `draw_bone_seg` (trebuchet beams + A-frames / ram log +
chains / bowstrings).
1. **Trebuchets ×2** (`draw_trebuchet`) — a base sledge + an A-frame + a long raised throwing
   beam with a counterweight box and a boulder in the sling (obstacles).
2. **Catapults ×2** (`draw_catapult`) — a wheeled frame + uprights + a cocked throwing arm and
   bucket (obstacles).
3. **Ballista** (`draw_ballista`) — a pedestal + stock + a horizontal bow with a loaded bolt.
4. **Siege tower** (the tall landmark obstacle) — a wheeled timber tower with corner posts,
   floors, a lowered drawbridge and a banner.
5. **Battering ram** — a covered penthouse with a suspended iron-headed log on chains.
6. **The wall** — a battered stone curtain with battlements and a central breach full of rubble
   (obstacles).
7. **Ammunition & atmosphere** — boulder piles, barrels, palisade stakes; additive brazier fire +
   a drifting smoke column + embers + circling crows.

## Encounter (mobs.cpp)
- **8 "Routed"** — the `Mob` horde, level-aware spawn set (`siege_defs`), among the engines.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Routed*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–94 and the `Boss` are untouched. `mob_level` now covers
  `… || BAZAAR || SIEGE`. **No new global struct/vector** — the engines are parametric; the
  machine + wall obstacles and the lights are pushed in `build_siege`; ruts/shot/rubble/stakes/
  embers are seeded; only `s_wisps` reused. New `draw_trebuchet` / `draw_catapult` / `draw_ballista`
  rlgl-matrix helpers draw the machines. Lights built in `build_siege` (so `build_crystals` /
  `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_siege` lays
  the mud floor.
- CLI: `siegeworks` / `trebuchet` / `engines` launches straight into it.
