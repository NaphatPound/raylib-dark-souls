# The Salt Pans — Level Design (LEVEL_SALT = 46)

A blinding-white salt works: a geometric grid of shallow brine evaporation ponds
divided by raised salt berms, conical harvested-salt piles, jagged salt-crust shards,
a salt-works hut with rakes and a cart, shimmering in the heat. The forty-third
**non-boss** level — the salt-pans' brined dead ("The Encrusted") rise from the
crust. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **salt-pan** silhouette (a grid of brine
mirror-ponds + white salt cones) found in none of the other forty-six levels. Distinct
from the Desert Ziggurat (sand ruins), the Crystal Cavern (cave gems) and the Geyser
Springs (geothermal terraces).

## Theme & palette
- **Mood:** stark, blinding, parched — a harsh white glare over pale brine squares,
  heat shimmering off the crust.
- **Light (render.cpp):** harsh high sun, blinding white light `(1.16, 1.12, 1.04)` —
  the brightest yet, bright bounced ambient, pale heat haze (`~#CCD1D1`, density
  `0.0070`). Reuses `sky_ice.fs`; **dry** — its own salt crust is the floor.
- **Glow:** pale salt-white / brine sheen `(1.00, 0.92, 0.86)`.

## Geometry (arena.cpp :: build_salt / draw_salt / salt_layout)
Reuses `s_column` (salt crust / pond squares / berms / cart / hut), `s_cone` (salt
piles / crust shards), `s_dome` (cart load), `draw_bone_seg` (rakes), and
**`draw_cottage`** (the salt-works hut).
1. **Brine-pond grid** — a 5×5 grid of shallow alternating pale-teal/pink brine
   squares over the white crust, separated by raised white **salt berms** (low walls
   along the grid lines). The signature.
2. **Salt piles** — 9 `salt_layout` conical white piles (a bright crystal inner cone)
   + a **central great pile**; the larger piles (and the centre) are obstacles.
3. **Salt-crust shards** — jagged white `s_cone` clusters at berm crossings.
4. **Salt-works hut** — a scaled `draw_cottage` (obstacle) with leaning rakes; a
   handcart of salt.
5. **Heat FX** — additive brine-pond glints + rising heat shimmer + pale sheen glow.

## Encounter (mobs.cpp)
- **8 "Encrusted"** — the `Mob` horde, level-aware spawn set (`salt_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Encrusted*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–45 and the `Boss` are untouched. `mob_level` now covers
  `… || WATERMILL || SALT`. **No new global struct/vector** — `salt_layout` fills a
  local `std::vector<Vector4>` (piles: xyz + w=scale) shared by build+draw; the
  pond-grid/berms are a fixed loop; only `s_wisps` reused. Sheen lights built in
  `build_salt` (so `build_crystals`/`draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_salt` lays the salt-crust floor (the
  ponds are their own shallow brine slabs).
- CLI: `salt` / `saltpan` / `saltflats` launches straight into it.
