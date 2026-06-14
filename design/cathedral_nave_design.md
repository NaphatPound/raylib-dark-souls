# Cathedral Nave — Level Design (LEVEL_NAVE = 44)

A grand Gothic cathedral interior: two arcades of clustered piers joined by **pointed**
arches into a vaulted nave, tall stained-glass lancet windows, a great **rose window**
over an altar, pews, swaying ring-chandeliers and a baptismal font. The forty-first
**non-boss** level — the cathedral's faithful dead ("The Penitent") rise between the
pews. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **Gothic cathedral** silhouette
(clustered piers + pointed-arch vault + rose window) found in none of the other
forty-four levels. Distinct from the Ruined Throne Hall (a flat colonnade hall) and
the Frozen Cathedral boss arena (a re-themed `boss_arena.glb`).

## Theme & palette
- **Mood:** hushed, reverent, dim — cool stone in soft clerestory light, jewelled
  pools of colour from the glass and the warm flicker of candles.
- **Light (render.cpp):** soft clerestory light, pale stone light `(0.80, 0.76,
  0.70)`, dim reverent ambient, shadowed nave haze (`~#383340`, density `0.0090`).
  Reuses `sky_storm.fs` (interior); **dry** — its own flagstones are the floor.
- **Glow:** warm candle / stained-glass `(1.00, 0.72, 0.42)`.

## Geometry (arena.cpp :: build_nave / draw_nave / draw_pier / draw_gothic_arch / nave_piers)
Reuses `s_cyl` (pier shafts / font / radial tracery), `s_column` (floor / walls /
pews / altar / capitals), `s_dome` (font basin), `s_torus` (rose-window ring /
chandeliers), `draw_bone_seg` (the pointed-arch chains / chandelier cords).
1. **Pier arcades** — `nave_piers`: two rows of 5 clustered `draw_pier` columns (core
   + 4 shafts + base + capital) flanking the central aisle; each a collision obstacle.
2. **Pointed arches** — `draw_gothic_arch`: two arcs springing from adjacent pier tops
   to a peak, drawn as cylinder chains — **transverse** arches span the aisle and
   **longitudinal** arches run each row, forming a vaulted nave.
3. **Rose window** — a great `s_torus` tracery ring + 12 radial spokes on the chancel
   back wall, filled with additive stained-glass; the landmark.
4. **Side walls** with tall stained-glass **lancet** bays; a carpet aisle runner.
5. **Furnishings** — an altar on steps + a central baptismal font (the central
   obstacle) + rows of pews + swaying ring-**chandeliers**.
6. **Light FX** — additive rose-window & lancet glass glow + flickering candle flames.

## Encounter (mobs.cpp)
- **8 "Penitent"** — the `Mob` horde, level-aware spawn set (`nave_defs`, kept within
  the aisle). Same lightweight AI, lock-on, aggregate HUD bar (named *The Penitent*),
  and victory-on-clear.

## Integration notes
- Purely additive: levels 0–43 and the `Boss` are untouched. `mob_level` now covers
  `… || FOUNDRY || NAVE`. **No new global struct/vector** — `nave_piers` fills a local
  `std::vector<Vector3>` shared by build+draw; altar/font/rose at fixed positions; only
  `s_wisps` reused. Candle/glass lights built in `build_nave` (so `build_crystals`/
  `draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_nave` lays the flagstone floor.
- CLI: `nave` / `cathedral` / `basilica` launches straight into it.
