# The Foundry — Level Design (LEVEL_FOUNDRY = 43)

A working ironworks lit by molten metal: a tall blast-furnace cupola pouring glowing
iron, a great timber trip-hammer that rises and slams an anvil in showers of sparks,
molten runners feeding casting molds, anvils, a quench trough and stacks of ingots.
The fortieth **non-boss** level — the foundry's molten dead ("The Smelted") rise from
the slag. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **foundry/ironworks** silhouette (blast
furnace + trip-hammer + molten runners + casting molds) found in none of the other
forty-three levels. Distinct from the Volcanic Forge (a natural-lava boss arena) and
the Clockwork Vault (brass gears).

## Theme & palette
- **Mood:** dark, hot, hammering — soot-black gloom split by molten-orange furnace
  light and the rhythmic clang-and-spark of the trip-hammer.
- **Light (render.cpp):** dim high vents, weak warm light `(0.70, 0.60, 0.52)`, dim
  smoky ambient, dark soot haze (`~#332828`, density `0.0125`). Reuses `sky_storm.fs`;
  **dry** — its own iron-plate floor is the floor.
- **Glow:** molten-iron orange `(1.00, 0.50, 0.15)`.

## Geometry (arena.cpp :: build_foundry / draw_foundry / draw_furnace / draw_triphammer)
Reuses `s_cyl` (furnace rings / tuyeres), `s_column` (floor / anvils / molds / ingots
/ helve / head / trough), `s_dome` (slag), `draw_bone_seg` (A-frame supports), the
rlgl matrix (pivoting hammer), and additive `DrawCube`/`DrawSphereEx` (molten + smoke
+ sparks).
1. **Blast furnace** — `draw_furnace`: a stack of tapering brick/iron rings + a
   charging chimney + iron hoops + tuyere pipes + a dark tap-hole. The central
   landmark obstacle, pouring an additive molten tap-glow.
2. **Trip-hammer** — `draw_triphammer`: a timber A-frame carrying a pivoting helve
   with a heavy iron head that **rises slowly then SLAMS** (a cam cycle on `s_time`)
   onto an anvil; spark bursts fire in the additive pass on impact. A landmark
   obstacle.
3. **Casting & metal** — 2 sand casting molds with glowing molten pools, a glowing
   molten runner channel from the furnace, ingot-bar pyramids, a second anvil and a
   quench trough.
4. **Yard** — an iron-plate floor with plate seams + scattered slag mounds.
5. **Heat FX** — additive furnace pour, molten runners/molds, chimney soot-smoke,
   anvil sparks, and molten point-glow.

## Encounter (mobs.cpp)
- **8 "Smelted"** — the `Mob` horde, level-aware spawn set (`foundry_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Smelted*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–42 and the `Boss` are untouched. `mob_level` now covers
  `… || REEF || FOUNDRY`. **No new global struct/vector** — furnace/hammer/anvils/molds
  at fixed positions (obstacles in `build_foundry`, geometry in `draw_foundry`); only
  `s_wisps` reused. Molten lights built in `build_foundry` (so `build_crystals`/
  `draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_foundry` lays the iron-plate floor.
- CLI: `foundry` / `ironworks` / `smithy` launches straight into it.
