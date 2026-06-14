# The Whaling Station — Level Design (LEVEL_WHALING = 96)

A dead try-works on a cold, fog-bound harbour: iron **try-pots** smoke in their brick furnaces
under a timber shed and chimney, a great **whale ribcage** arches over the flensing platform, a
**slipway** ramps down into the oily sea, timber pilings carry a moored **whaleboat**, and oil
barrels and harpoon racks stand on the plank wharf. The ninety-third **non-boss** level — the
flayed dead have risen on the bloody wharf ("The Flensed"). Clear them all to win; the bonfire
then ignites as usual.

New procedural geometry, not a recolour — a **whaling-station** silhouette (try-pots + a whale
ribcage + a slipway + a whaleboat on a harbour) found in none of the other ninety-six levels. A
**WET** level — the reflective water plane *is* the cold harbour, with a stone-and-timber wharf
(the fighting floor) and the slipway running into it. A maritime-**industrial** scene — distinct
from the **Dragon Boneyard** (a field of dragon skeletons; here the bone is one ribcage being
*processed* at a working try-works), the **Skydock** (an airship dock) and the **Shipwreck Cove**.

## Theme & palette
- **Mood:** grim, greasy, cold — a slaughter-yard of the sea wreathed in fog.
- **Light (render.cpp):** flat cold daylight `(0.40,0.62,0.40)`, pale cold light
  `(0.86,0.90,0.94)`, cool damp fill, thick cold sea fog (`~#9EA8B3`, high density `0.0120`).
  Reuses `sky_storm.fs` + `water_storm.fs` (a cold grey sea). **WET** (`draw_water` runs;
  `draw_whaling` lays the wharf).
- **Glow:** warm try-pot fire `(1.00,0.52,0.24)`.

## Geometry (arena.cpp :: draw_trypot / build_whaling / draw_whaling)
Reuses `s_column` (wharf + plank seams / slipway / shed roof / chimney / flensing platform / tool
rack / boat hull), the lit `s_cyl` (try-pots / pilings / shed posts / barrels / harpoons / boat
harpoon), `s_cone` (whale skull / boat prow), `s_dome` (try-pot fires drawn additively), and
`draw_bone_seg` (the ribcage ribs + backbone).
1. **The wharf** — a stone-and-timber platform (the fighting floor) raised on the WET harbour,
   water showing at the rim and down the slipway.
2. **Try-works** — three `draw_trypot` units (brick furnace + dark firebox + iron try-pot + oily
   surface) under a shed with posts and a chimney (obstacles).
3. **The whale carcass** — a great ribcage of curved ribs + a backbone arching over a flensing
   platform, with a tapering skull (an obstacle).
4. **The water** — a slipway ramping into the sea, jetty + slipway pilings, a moored whaleboat
   (hull + prow + a stowed harpoon).
5. **Stores** — oil barrels with iron hoops, a leaning-harpoon tool rack; additive try-pot fire +
   greasy chimney smoke + drifting sea fog + wheeling gulls.

## Encounter (mobs.cpp)
- **8 "Flensed"** — the `Mob` horde, level-aware spawn set (`whaling_defs`), on the wharf. Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Flensed*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–95 and the `Boss` are untouched. `mob_level` now covers
  `… || SIEGE || WHALING`. **No new global struct/vector** — the station is parametric; the
  try-pot + shed + carcass + barrel obstacles and the lights are pushed in `build_whaling`;
  barrels/fog are seeded; only `s_wisps` reused. A new `draw_trypot` helper draws each furnace.
  Lights built in `build_whaling` (so `build_crystals` / `draw_crystals` early-return). Uses the
  global `water_level` to sit the whaleboat + pilings on the harbour surface.
- WET level: `draw_water` is NOT in its dry early-return (the sea draws); `draw_crystals` IS.
- CLI: `whaling` / `tryworks` / `station` launches straight into it.
