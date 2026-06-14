# The Moai — Level Design (LEVEL_MOAI = 65)

A windswept coastal headland lined with colossal carved stone heads: a row of moai busts
standing on a stone ahu, gazing inland — some crowned with red topknots (pukao), a couple
toppled forward into the grass — under a cool overcast sky with sea mist drifting behind.
The sixty-second **non-boss** level — the island's watching dead ("The Vigil") rise to
defend the heads. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **moai / Easter-Island** silhouette (a row of
elongated carved-head busts on an ahu) found in none of the other sixty-five levels. A
**DRY** level (grassy headland). Deliberately distinct from the Sentinel Court (armed
humanoid guardian statues over moonlit water): moai are limbless busts — heavy brow, long
nose, deep eye sockets, jutting chin, long ears — on a green coastal bluff.

## Theme & palette
- **Mood:** lonely, windswept, watchful — pale stone under a grey Pacific sky.
- **Light (render.cpp):** soft diffuse overcast `(0.34,0.70,0.36)`, cool overcast daylight
  `(0.86,0.88,0.90)`, cool even fill, cool coastal haze (`~#A8B3B8`, density `0.0090`).
  Reuses `sky_storm.fs`; **DRY** (`draw_moai_isle` lays the grassy headland).
- **Glow:** cool coastal grey-white `(0.72,0.78,0.82)`.

## Geometry (arena.cpp :: draw_moai / build_moai / draw_moai_isle)
Reuses the lit `s_column` (torso/head/facial features/ahu/floor) and `s_cyl` (red topknot),
`s_dome` (tussock mounds / boulders).
1. **Moai** (`draw_moai`) — a buried torso + elongated head with the iconic heavy brow,
   long nose, deep dark eye sockets, jutting chin and long ears; optional red topknot; leans
   back slightly (all in one rlgl frame).
2. **The row** — 6 moai on a long stone ahu platform facing the player (+z), every other one
   crowned with a pukao, plus a **central great moai** (the landmark obstacle).
3. **Toppled** — two moai lying forward in the grass (rlgl 82° frame).
4. **Headland** — a grassy slab, 22 tussock mounds, scattered boulders.
5. **Atmosphere** — additive cool fill glow + a drifting sea-mist band along the far rim.

## Encounter (mobs.cpp)
- **8 "Vigil"** — the `Mob` horde, level-aware spawn set (`moai_defs`). Same lightweight AI,
  lock-on, aggregate HUD bar (named *The Vigil*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–64 and the `Boss` are untouched. `mob_level` now covers
  `… || HOODOO || MOAI`. **No new global struct/vector** — the row is a fixed array shared
  by build (obstacles) + draw; boulders are a seeded deterministic loop (seed 13100); only
  `s_wisps` reused. Cool lights built in `build_moai` (so `build_crystals`/`draw_crystals`
  early-return).
- DRY level: `draw_water` early-returns; `draw_moai_isle` lays the grassy headland.
- CLI: `moai` / `heads` / `island` launches straight into it.
