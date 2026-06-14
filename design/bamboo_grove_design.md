# Bamboo Grove — Level Design (LEVEL_BAMBOO = 50)

A hushed grove of towering segmented bamboo, set around a zen garden of raked gravel
and mossy boulders, with stone lanterns, a small arched moon-bridge over a gravel "dry
stream", green dappled light-shafts and drifting leaves. The forty-seventh **non-boss**
level — the grove's blade-cut dead ("The Severed") rise among the culms. Clear them all
to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **bamboo grove** silhouette (dense
segmented culms + a zen rock garden + a moon-bridge) found in none of the other fifty
levels. Distinct from the Petrified Forest (dead twisted trees), the Overgrown Temple
(jungle ruins) and the Forgotten Shrine (vermillion torii avenue).

## Theme & palette
- **Mood:** serene yet watchful — cool green-filtered daylight slanting through a
  dense canopy, leaves drifting onto raked gravel.
- **Light (render.cpp):** dappled overhead sun, cool green-filtered light `(0.84,
  1.00, 0.78)`, soft green ambient, green forest haze (`~#76947 6`, density `0.0095`).
  Reuses `sky_ice.fs`; **dry** — its own mossy earth is the floor.
- **Glow:** green dappled / warm lantern `(0.60, 0.92, 0.50)`.

## Geometry (arena.cpp :: build_bamboo / draw_bamboo / draw_bamboo_stalk / bamboo_layout)
Reuses `s_cyl` (node rings / lantern posts), `s_dome` (boulders / moss / mounds),
`s_cone` (lantern caps), `s_column` (ground / gravel / bridge planks), `draw_bone_seg`
(culm segments / leaf sprays / bridge rails).
1. **Bamboo culms** — `draw_bamboo_stalk`: a tall segmented stalk (chained
   `draw_bone_seg`, slightly curved and tapering) with darker node rings between joints
   and leaf sprays at the top. 16 seeded clumps of 4 culms each; each clump a collision
   obstacle. The dense vertical forest is the signature.
2. **Zen garden** — a central raked-gravel circle with a **rock arrangement** (mossy
   boulders + moss cap); the central obstacle. A winding gravel "dry stream" leads to
   the entrance.
3. **Stone lanterns** — 4 (pedestal + fire box + cap); obstacles.
4. **Moon-bridge** — a small arched plank bridge (curved deck on posts) over the gravel
   stream.
5. **Atmosphere** — additive green dappled **light-shafts**, a field of drifting
   leaves, and warm lantern glow.

## Encounter (mobs.cpp)
- **8 "Severed"** — the `Mob` horde, level-aware spawn set (`bamboo_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Severed*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–49 and the `Boss` are untouched. `mob_level` now covers
  `… || BREW || BAMBOO`. **No new global struct/vector** — `bamboo_layout` fills a
  local `std::vector<Vector4>` (clumps: xyz + w=seed) shared by build+draw; lanterns at
  fixed positions; only `s_wisps` reused. Dappled lights built in `build_bamboo` (so
  `build_crystals`/`draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_bamboo` lays the mossy floor.
- CLI: `bamboo` / `grove` / `thicket` launches straight into it.
