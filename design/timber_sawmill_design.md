# Timber Sawmill — Level Design (LEVEL_SAWMILL = 32)

A working logging yard: pyramidal stacks of felled logs, an open timber-framed mill
shed with a giant spinning circular sawblade mid-cut, a log-crane gantry, plank
piles and cut stumps over a sawdust-strewn dirt yard. The twenty-ninth **non-boss**
level — the mill's maimed dead ("The Splintered") work no more. Clear them all to
win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **timber yard** silhouette (log-stack
pyramids + a sawblade shed + a log crane) found in none of the other thirty-two
levels. Distinct from the Clockwork Vault (indoor brass gears) and Windmill Fields
(windmill sails on farmland) despite all three having a rotating part.

## Theme & palette
- **Mood:** busy, mundane, faintly grim — soft overcast daylight, fresh sawn wood
  and drifting sawdust, the only animation the bright steel blade tearing a log.
- **Light (render.cpp):** soft daylight, warm sawdust-lit day `(1.00, 0.92, 0.78)`,
  neutral working ambient, pale sawdust haze (`~#8C857A`, density `0.0095` — light).
  Reuses `sky_storm.fs`; **dry** — its own dirt yard is the floor.
- **Glow:** warm work-lamp amber `(0.95, 0.74, 0.42)`.

## Geometry (arena.cpp :: build_lumber / draw_lumber / draw_logpile / draw_sawblade)
Reuses `draw_bone_seg` (every log + crane posts + chain + table log), `s_cyl`
(sawblade disc / stumps / end-caps), `s_column` (planks / shed frame / roof / table),
`s_dome` (sawdust patches).
1. **Log piles** — `draw_logpile`: a pyramidal stack of horizontal logs (each a lit
   `draw_bone_seg` cylinder with a pale sawn end-cap), tapering row by row. A big
   central stack + ~5 scattered yard stacks, each a collision obstacle.
2. **Mill shed** — an open timber-framed shed with corner posts + pitched roof + back
   wall, housing a saw table with a **log mid-cut** and the **`draw_sawblade`**: a
   thin lit steel disc standing upright, spun about its axle by `s_time`, ringed with
   18 cutting teeth. The animated signature; the shed is a landmark obstacle.
3. **Log crane** — a leaning two-post gantry + beam with a swaying chain and hook
   block (animated).
4. **Yard dressing** — cut stumps (with pale top rings), low plank stacks, sawdust
   patches.
5. **Work-lamps** — additive warm glow at the shed + stacks, with drifting sawdust
   motes.

## Encounter (mobs.cpp)
- **8 "Splintered"** — the `Mob` horde, level-aware spawn set (`lumber_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Splintered*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–31 and the `Boss` are untouched. `mob_level` now covers
  `… || BOG || SAWMILL`. New `LogPile` struct + `s_logpiles` vector (cleared in
  `unload`); work-lamp lights built in `build_lumber` (so `build_crystals`/
  `draw_crystals` early-return for this level).
- Dry level: `draw_water` early-returns; `draw_lumber` lays the dirt floor.
- CLI: `sawmill` / `lumber` / `timberyard` launches straight into it.
