# The Hoodoos — Level Design (LEVEL_HOODOO = 64)

A red-rock badlands canyon: a field of tall, banded sandstone **hoodoo** spires, each
tapering and pinched into wavy bands and topped with a wider balanced caprock, set among
flat-topped mesa walls, a natural rock arch, ripple dunes and dust devils under a bright
desert sky. The sixty-first **non-boss** level — the canyon's weathered dead ("The
Eroded") rise from the sands. Clear them all to win; the bonfire then ignites.

New procedural geometry, not a recolour — a **hoodoo / badlands** silhouette (tapering
banded rock spires with caprocks + mesas + a rock arch) found in none of the other
sixty-four levels. A **DRY** level (sandy canyon floor). Distinct from the Desert
Ziggurat (a stepped pyramid), the Quarry (cut stone benches), the Petrified Forest (dead
trees), the Fallen Titan (statue ruins), and the Crater (impact basin).

## Theme & palette
- **Mood:** vast, sun-baked, ancient — red rock glowing under a clear blue sky.
- **Light (render.cpp):** warm sandstone sun `(0.40,0.74,0.30)`, warm orange daylight
  `(1.10,0.82,0.56)`, warm rock fill, warm dusty haze (`~#C7A885`, density `0.0070` —
  clear bright air). Reuses `sky_ice.fs` (the bright-blue sky that sets off the red rock);
  **DRY** (`draw_hoodoos` lays the sandy floor).
- **Glow:** warm sandstone orange `(1.00,0.62,0.34)`.

## Geometry (arena.cpp :: draw_hoodoo / hoodoo_layout / build_hoodoo / draw_hoodoos)
Reuses the lit `s_cyl` (banded spire discs / caprocks / arch piers), `s_dome` (caprock
tops / dunes / scrub), `s_column` (floor / mesa blocks + flat caps), and `draw_bone_seg`
(the arch lintel).
1. **Hoodoos** (`draw_hoodoo`) — a stack of `s_cyl` discs whose radius tapers overall and
   ripples with a `sin` pinch/bulge to read as sedimentary bands, leaning slightly, topped
   with a wider balanced caprock (cyl + dome). 14 across the field + a central great spire
   (the landmark obstacle).
2. **Mesas** — 10 flat-topped butte blocks with cap slabs along the back rim.
3. **Rock arch** — two `s_cyl` piers bridged by a `draw_bone_seg` lintel.
4. **Floor** — a sandy slab, 18 ripple dunes, sparse desert scrub.
5. **Atmosphere** — additive warm fill glow, 2 swirling dust devils, heat-shimmer motes.

## Encounter (mobs.cpp)
- **8 "Eroded"** — the `Mob` horde, level-aware spawn set (`hoodoo_defs`). Same lightweight
  AI, lock-on, aggregate HUD bar (named *The Eroded*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–63 and the `Boss` are untouched. `mob_level` now covers
  `… || TITAN || HOODOO`. **No new global struct/vector** — `hoodoo_layout` fills a local
  `std::vector<Vector4>` (x, z, height, baseR) shared by build (obstacles) + draw; mesas/
  arch/dunes are seeded deterministic loops; only `s_wisps` reused. Warm lights built in
  `build_hoodoo` (so `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_hoodoos` lays the sandy floor.
- CLI: `hoodoos` / `canyon` / `badlands` launches straight into it.
