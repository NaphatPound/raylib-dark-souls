# The Skydock — Level Design (LEVEL_DOCK = 38)

A high-altitude airship mooring yard: a great moored dirigible bobs overhead, tethered
by ropes to a central mast and corner masts, with a control tower, cargo and a cloud
band drifting far below. The thirty-fifth **non-boss** level — the skydock's lost crew
("The Becalmed") man their posts no more. Clear them all to win; the bonfire then
ignites as usual.

New procedural geometry, not a recolour — a **skydock** silhouette (a moored airship +
mooring masts + a cloud band) found in none of the other thirty-eight levels. Distinct
from the Sky Sanctum (floating rock islands) and the Beacon Coast (lighthouse).

## Theme & palette
- **Mood:** lofty, clean, breezy — bright thin air, a vast silver airship swaying at
  its moorings, clouds drifting past the platform edge.
- **Light (render.cpp):** bright high sun, clean daylight `(1.06, 1.02, 0.96)`, bright
  cool sky ambient, pale-blue altitude haze (`~#BCD1EB`, density `0.0070` — thin air).
  Reuses `sky_ice.fs`; **dry** — its own dock deck is the floor.
- **Glow:** warm signal-lamp `(1.00, 0.78, 0.45)`.

## Geometry (arena.cpp :: build_dock / draw_dock / draw_airship / draw_prop)
Reuses `s_cyl` (masts / gas cylinders), `s_dome` (mast caps / prop hub / airship
caps), `s_column` (deck / planks / gondola / tower / cargo), `draw_bone_seg`
(envelope body / struts / mooring ropes).
1. **The airship** — `draw_airship`: an elongated gas envelope (a fat `draw_bone_seg`
   capsule with domed nose/tail caps + a ridge stripe), three tail fins, a hanging
   gondola with windows and roof, mooring struts, and **two spinning propellers**
   (`draw_prop`: hub + 3 blades turning with `s_time`). The whole ship **bobs gently**
   with `s_time`.
2. **Mooring masts** — a tall central mast (the ship's nose tethers to it; the central
   obstacle) + 4 corner masts with signal-lamp caps, all roped to the airship.
3. **Control tower** — a glazed-cab tower (obstacle).
4. **Dock platform** — a stone-and-timber deck with plank seams and a parapet rim
   (the +z front arc left open).
5. **Cargo & sky** — crates, barrels, gas cylinders; an additive **cloud band**
   drifting around the platform edge (swaying with `s_time`) to sell the altitude, plus
   blinking warm signal lamps.

## Encounter (mobs.cpp)
- **8 "Becalmed"** — the `Mob` horde, level-aware spawn set (`dock_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Becalmed*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–37 and the `Boss` are untouched. `mob_level` now covers
  `… || QUARRY || DOCK`. **No new global struct/vector** — masts/airship at fixed
  positions (obstacles in `build_dock`, geometry in `draw_dock`); only `s_wisps`
  reused. Signal lights built in `build_dock` (so `build_crystals`/`draw_crystals`
  early-return).
- Dry level: `draw_water` early-returns; `draw_dock` lays the dock deck.
- CLI: `skydock` / `airship` / `dirigible` launches straight into it.
