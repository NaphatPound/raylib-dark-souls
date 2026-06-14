# The Gambit — Level Design (LEVEL_CHESS = 59)

A surreal **colossal marble chessboard**: a crisp black-and-white checkered floor walked
by towering weathered stone chess pieces — rooks with crenellations, mitred bishops,
blocky knights, crowned queens, a cross-topped king — with two captured pieces toppled
across the board. The fifty-sixth **non-boss** level — the pieces taken from the board
("The Sacrificed") rise to fight. Clear them all to win; the bonfire then ignites.

New procedural geometry, not a recolour — a **giant chessboard** silhouette (checkered
tiles + six kinds of colossal chess pieces) found in none of the other fifty-nine levels.
A **DRY** level (the checkered board is the floor). Unlike any other arena in the game —
the closest relative is the Sentinel Court (humanoid statues), but these are abstract
game pieces on a checkerboard, an entirely different read.

## Theme & palette
- **Mood:** eerie, regal, surreal — a forgotten game of giants in a cool marble hall.
- **Light (render.cpp):** clean overhead hall light `(0.32,0.86,0.30)`, cool neutral
  marble white `(0.96,0.97,1.02)`, soft cool fill, pale cool haze (`~#949EB8`, density
  `0.0075` — crisp and airy). Reuses `sky_ice.fs`; **DRY** (`draw_chess` lays the board).
- **Glow:** cool royal blue-white `(0.70,0.78,0.95)`.

## Geometry (arena.cpp :: draw_chesspiece / chess_layout / build_chess / draw_chess)
Reuses the lit `s_cyl` (piece bodies / foot discs / rook towers), `s_dome` (heads /
collars / ball finials via a two-hemisphere `ball` lambda), `s_cone` (queen crown
points), and `s_column` (board tiles / base slab / rook merlons / knight head / king cross).
1. **The board** — a 9×9 grid of alternating white-marble / black-basalt `s_column` tiles
   over a dark base slab.
2. **Six piece kinds** (`draw_chesspiece`, kind 0–5): pawn (body + ball head), rook
   (tower + 6 crenellations), bishop (slim body + mitre + finial), knight (neck + tilted
   blocky horse-head via an rlgl frame), queen (tall body + collar + 8-point crown + orb),
   king (tallest body + crown + cross).
3. **Roster** — a back rank `R N B Q Q B N R`, two flanking colossal rooks, two side
   bishops, a pawn line, and the **colossal central king** (the landmark obstacle).
4. **Captured pieces** — two toppled pieces lying flat across the board (rlgl 90° frame).
5. **Atmosphere** — additive cool royal glow + drifting dust motes.

## Encounter (mobs.cpp)
- **8 "Sacrificed"** — the `Mob` horde, level-aware spawn set (`chess_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Sacrificed*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–58 and the `Boss` are untouched. `mob_level` now covers
  `… || ARCHTREE || CHESS`. **No new global struct/vector** — `chess_layout` fills a local
  `std::vector<Vector4>` (pieces: x, y=scale, z, w=kind) shared by build+draw; the board
  and toppled pieces are deterministic; only `s_wisps` reused. Royal lights built in
  `build_chess` (so `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_chess` lays the checkered board.
- CLI: `chess` / `gambit` / `checkmate` launches straight into it.
