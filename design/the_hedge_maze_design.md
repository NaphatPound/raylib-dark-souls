# The Hedge Maze — Level Design (LEVEL_MAZE = 60)

A formal garden **labyrinth**: tall clipped-hedge walls forming corridors around an open
gravel clearing, with manicured topiary (ball-trees and cone spires), wrought garden
lamp posts, and a grand topiary centrepiece. The fifty-seventh **non-boss** level — those
who lost their way in the hedges ("The Wayward") rise to fight. Clear them all to win;
the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **hedge-maze** silhouette (corridor walls +
topiary + lamp posts around a central clearing) found in none of the other sixty levels.
A **DRY** level (grass floor + gravel clearing). Distinct from the Hanging Gardens (open
flowerbeds + fountain), the Bamboo Grove (zen rock garden), and the Grand Conservatory
(glass dome): this is a navigable maze of tall green walls.

## Theme & palette
- **Mood:** serene but faintly eerie — a warm late-afternoon garden you could get lost in.
- **Light (render.cpp):** soft late sun `(0.30,0.78,0.40)`, warm golden daylight
  `(1.00,0.97,0.82)`, soft green-shadow fill, gentle green garden haze (`~#8FA38A`,
  density `0.0085`). Reuses `sky_ice.fs`; **DRY** (`draw_maze` lays grass + a gravel disc).
- **Glow:** warm garden-lamp `(1.00,0.86,0.58)`.

## Geometry (arena.cpp :: draw_hedge / draw_topiary / maze_layout / build_maze / draw_maze)
Reuses the lit `s_column` (hedge wall bodies / floor / gravel via s_cyl), `s_dome` (bushy
hedge tops / ball-topiary / lamp globes), `s_cone` (cone-spire topiary), `s_cyl`
(planters / trunks / lamp posts / clearing disc).
1. **Hedge walls** (`draw_hedge`) — a box wall oriented by yaw with bushy dome clumps along
   the top; 13 segments (`maze_layout`) make long side walls, a far back wall, entrance
   flanks, inner corridor walls, back spurs, clearing-edge cover and corner accents. The
   front entrance lane (x≈0) and central clearing are deliberately left open for navigation.
2. **Topiary** (`draw_topiary`) — kind 0 stacked ball-tree (3 shrinking spheres on a
   trunk), kind 1 cone spire. A grand ball-tree on a stone pedestal marks the centre (the
   landmark obstacle); four cone topiaries flank the junctions.
3. **Lamps** — wrought lamp posts (s_cyl post + s_dome globe) at the four junctions.
4. **Atmosphere** — additive warm lamp glow + drifting pollen / butterflies.

## Encounter (mobs.cpp)
- **8 "Wayward"** — the `Mob` horde, level-aware spawn set (`maze_defs`), positioned in the
  clearing/corridors. Same lightweight AI, lock-on, aggregate HUD bar (named *The Wayward*),
  and victory-on-clear.

## Integration notes
- Purely additive: levels 0–59 and the `Boss` are untouched. `mob_level` now covers
  `… || CHESS || MAZE`. **No new global struct/vector** — `maze_layout` fills a local
  `std::vector<Vector4>` (walls: dx, dz, yaw, length) shared by build (obstacle spheres
  sampled along each wall) + draw (hedge geometry); topiary/lamps deterministic; only
  `s_wisps` reused. Lamp lights built in `build_maze` (so `build_crystals`/`draw_crystals`
  early-return).
- DRY level: `draw_water` early-returns; `draw_maze` lays the grass + gravel clearing.
- CLI: `maze` / `hedge` / `labyrinth` launches straight into it.
