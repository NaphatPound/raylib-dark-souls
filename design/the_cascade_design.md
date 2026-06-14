# The Cascade — Level Design (LEVEL_FALLS = 61)

A grand **waterfall**: a towering rugged cliff with curtains of water pouring off the top
into a turquoise plunge pool, midstream mossy boulders to fight around, churning foam,
scrolling droplets and rising mist. The fifty-eighth **non-boss** level — the plunge
pool's drowned dead ("The Drenched") rise from the spray. Clear them all to win; the
bonfire then ignites as usual.

New procedural geometry, not a recolour — a **waterfall-cliff** silhouette (a tall stacked
rock cliff + falling water curtains + a plunge pool) found in none of the other sixty-one
levels. A **WET** level (the reused turquoise `water_ice` plane is the plunge pool).
Distinct from the Geyser Springs (terraced vents), the Watermill (race + wheel), and the
Shipwreck Cove (foggy sea): this is a vertical cliff cascade.

## Theme & palette
- **Mood:** fresh, roaring, humid — cool daylight through a haze of spray.
- **Light (render.cpp):** bright fresh overhead `(0.28,0.82,0.36)`, cool fresh daylight
  `(0.92,0.98,1.02)`, cool misty fill, pale spray haze (`~#A8BDC7`, density `0.0095`).
  Reuses `sky_ice.fs`; **WET** — the reused turquoise `water_ice` plane is the pool.
- **Glow:** cool aqua spray `(0.62,0.85,0.95)`.

## Geometry (arena.cpp :: build_falls / draw_falls)
Reuses the lit `s_column` (cliff rock blocks), `s_dome` (mossy boulders / moss caps /
central great rock), and immediate-mode `DrawCube`/`DrawSphereEx` for the water effects.
1. **Cliff backdrop** — 14 columns of stacked, jittered, slightly-rotated rock blocks
   across the back, gently bowed, rising ~19u, with moss caps on the lower courses.
2. **Waterfall curtains** — 3 falling-water sheets (stacked alpha-blended `DrawCube`
   slices with a horizontal wobble) pouring from the cliff top into the pool.
3. **Water FX** — additive scrolling droplets down each curtain, churning foam discs at
   each base, and a band of slowly rising mist along the cliff foot.
4. **Pool** — the reused turquoise `water_ice` plane; 4 midstream mossy boulders (cover,
   obstacles) + a central great mossy rock (the landmark obstacle).

## Encounter (mobs.cpp)
- **8 "Drenched"** — the `Mob` horde, level-aware spawn set (`falls_defs`), in the pool
  between the player and the cliff. Same lightweight AI, lock-on, aggregate HUD bar (named
  *The Drenched*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–60 and the `Boss` are untouched. `mob_level` now covers
  `… || MAZE || FALLS`. **No new global struct/vector** — the cliff is a deterministic
  seeded loop, boulders/falls are fixed arrays shared by build+draw; only `s_wisps` reused.
  Spray lights built in `build_falls` (so `build_crystals`/`draw_crystals` early-return).
- WET level: `draw_water` still draws the pool plane (NOT in the dry early-return);
  `draw_falls` lays the cliff/boulders/cascade over it.
- CLI: `falls` / `waterfall` / `cascade` launches straight into it.
