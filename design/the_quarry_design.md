# The Quarry — Level Design (LEVEL_QUARRY = 37)

A stepped open-pit stone quarry: terraced rock benches rising to the rim, huge cut
megalith blocks, a timber shear-legs crane swinging a suspended block, a scaffold
ramp, and a half-carved colossus emerging from the back wall. The thirty-fourth
**non-boss** level — the quarry's crushed laborers ("The Hewn") rise from the rubble.
Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **quarry** silhouette (terraced rock
benches + cut-block stacks + a shear-legs crane + a half-carved colossus) found in
none of the other thirty-seven levels. Distinct from the Abandoned Mine (underground
timber shaft + carts) and the Druid Henge (finished standing stones).

## Theme & palette
- **Mood:** dusty, industrious, monumental — hazy high sun over pale cut rock, dust
  drifting, the only motion a stone block swinging on the crane.
- **Light (render.cpp):** high hazy sun, warm dusty daylight `(1.02, 0.96, 0.82)`,
  neutral stone ambient, pale rock-dust haze (`~#948C7A`, density `0.0090`). Reuses
  `sky_storm.fs`; **dry** — its own dusty quarry floor is the floor.
- **Glow:** warm work-lamp amber `(1.00, 0.80, 0.50)`.

## Geometry (arena.cpp :: build_quarry / draw_quarry / draw_crane)
Reuses `s_column` (floor / terraces / cut blocks / colossus / planks), `s_dome`
(rubble / colossus head), `draw_bone_seg` (crane legs / chain / scaffold rails).
1. **Terraced walls** — 3 concentric rings of cut rock benches (yawed `s_column`
   blocks) rising outward to the rim, the +z front arc left open as the entrance
   ramp.
2. **Central great cut block** — a big chisel-grooved megalith; the central obstacle.
3. **Cut-block stacks** — 3 small pyramids of megalith cubes; obstacles.
4. **Shear-legs crane** — `draw_crane`: a tripod of leaning timber legs + a boom from
   which a stone block hangs on a chain and **swings with `s_time`**. The animated
   signature; a landmark obstacle.
5. **Half-carved colossus** — a rough-hewn giant head + block emerging from the back
   wall (head `s_dome`, eye sockets, nose); a landmark obstacle.
6. **Dressing** — a leaning scaffold ramp, rubble patches, and additive work-lamp
   glow + drifting rock dust.

## Encounter (mobs.cpp)
- **8 "Hewn"** — the `Mob` horde, level-aware spawn set (`quarry_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Hewn*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–36 and the `Boss` are untouched. `mob_level` now covers
  `… || FLOE || QUARRY`. **No new global struct/vector** — all landmarks are at fixed
  positions (obstacles pushed in `build_quarry`, geometry drawn in `draw_quarry`);
  only `s_wisps` reused. Work-lamp lights built in `build_quarry` (so
  `build_crystals`/`draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_quarry` lays the quarry floor.
- CLI: `quarry` / `excavation` / `stonepit` launches straight into it.
