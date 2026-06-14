# The Bell Yard — Level Design (LEVEL_BELL = 54)

A bell-founder's carillon yard: tall timber bell-frames hung with rows of swaying
bronze bells of varying size (each with a clapper, gently ringing), a colossal central
bell on a heavy frame, hanging bell-ropes, and a few cracked fallen bells. The
fifty-first **non-boss** level — the yard's deafened dead ("The Tolled") rise among the
bells. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **bell yard** silhouette (timber frames
hung with swinging bronze bells) found in none of the other fifty-four levels. Distinct
from the Clockwork Vault (rotating gears) and the Cathedral Nave (stained glass /
chandeliers).

## Theme & palette
- **Mood:** resonant, solemn, bronze — soft overcast light glinting off rows of bells
  swaying and tolling, sound-rings shimmering outward.
- **Light (render.cpp):** soft overcast sun, warm bronze-tinged light `(0.98, 0.90,
  0.76)`, neutral working ambient, pale overcast haze (`~#807A70`, density `0.0090`).
  Reuses `sky_storm.fs`; **dry** — its own stone yard is the floor.
- **Glow:** warm bronze `(1.00, 0.70, 0.35)`.

## Geometry (arena.cpp :: build_bell / draw_bellyard / draw_bell / draw_bellframe / bell_layout)
Reuses `s_cone` (bell skirts), `s_dome` (crowns / clappers), `s_cyl` (canon loops /
frame posts), `s_column` (yard floor), `draw_bone_seg` (frame beams / clapper rods /
ropes), and the rlgl matrix (each bell swings).
1. **Bells** — `draw_bell`: a flared `s_cone` skirt (wide rim at the bottom) + a domed
   crown + a canon loop + a clapper, hung from a point and **swinging with `s_time`**.
2. **Bell-frames** — `draw_bellframe`: a 4-post timber frame with beams, hung with
   three bells of varying size and rope tails. 4 `bell_layout` frames (obstacles).
3. **Central great bell** — a colossal bell on a heavy 4-post frame; the landmark
   obstacle.
4. **Fallen bells** — a few cracked, tipped bells lying in the yard.
5. **Sound FX** — additive warm bronze glow + expanding **sound-ring** shimmers
   radiating from the great bell.

## Encounter (mobs.cpp)
- **8 "Tolled"** — the `Mob` horde, level-aware spawn set (`bell_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Tolled*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–53 and the `Boss` are untouched. `mob_level` now covers
  `… || PUMPKIN || BELL`. **No new global struct/vector** — `bell_layout` fills a local
  `std::vector<Vector4>` (frames: xyz + w=yaw) shared by build+draw; the central
  bell/fallen bells are fixed/seeded; only `s_wisps` reused. Bronze lights built in
  `build_bell` (so `build_crystals`/`draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_bellyard` lays the stone floor.
- CLI: `bell` / `belfry` / `carillon` launches straight into it.
