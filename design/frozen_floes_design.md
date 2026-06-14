# Frozen Floes — Level Design (LEVEL_FLOE = 36)

A glacial sea of cracked pack-ice: pale floes riding dark turquoise water, towering
jagged icebergs, serac ice-spikes and a frozen jetty, all under a shifting aurora.
The thirty-third **non-boss** level — the ice-locked dead ("The Frostbitten") break
the surface. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **frozen-sea** silhouette (floating ice
floes + jagged icebergs + aurora) found in none of the other thirty-six levels. A
**WET** level (the reused turquoise `water_ice` plane is the sea). Distinct from the
Frozen Cathedral boss arena (an enclosed ice hall) and the Shipwreck Cove (foggy
cove with hulls).

## Theme & palette
- **Mood:** vast, cold, still — pale arctic light over a glittering frozen sea, snow
  drifting and an aurora rippling overhead.
- **Light (render.cpp):** pale low arctic sun, cold blue-white light `(0.82, 0.90,
  1.00)`, bright cold ambient, icy blue-grey haze (`~#A8BCD6`, density `0.0090`).
  Reuses `sky_ice.fs`; **WET** — the reused turquoise `water_ice` plane is the sea.
- **Glow:** pale ice-cyan / aurora `(0.60, 0.85, 1.00)`.

## Geometry (arena.cpp :: build_floes / draw_floes / draw_iceberg / ice_layout)
A seeded layout (`ice_layout`) is shared by `build_floes` (obstacles + lights) and
`draw_floes` (geometry). Reuses `s_cyl` (floes / jetty posts), `s_dome` (snow drifts
/ iceberg shelves & caps), `s_cone` (iceberg peaks / seracs), `s_column` (jetty
deck).
1. **Pack-ice floes** — 16 flat pale slabs (overlapping `s_cyl` discs at sea level),
   some topped with an `s_dome` snow drift. No ground slab — the water plane is the
   floor.
2. **Icebergs** — 5 `draw_iceberg` masses + a central great berg: a submerged base
   shelf + a main `s_cone` peak + leaning secondary peaks + a faceted cap, in pale
   ice tones. Each (and the central berg) a collision obstacle.
3. **Seracs** — 7 clusters of tall thin `s_cone` ice spikes.
4. **Frozen jetty** — a plank deck on posts reaching in from the near edge.
5. **Aurora & snow** — additive shifting aurora ribbons high in the sky (swaying
   with `s_time`), pale ice glints, and a field of falling snow specks.

## Encounter (mobs.cpp)
- **8 "Frostbitten"** — the `Mob` horde, level-aware spawn set (`floe_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Frostbitten*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–35 and the `Boss` are untouched. `mob_level` now covers
  `… || VINE || FLOE`. **No new global struct/vector** — `ice_layout` fills local
  `std::vector<Vector4>` lists (floes: xyz+w=radius; bergs: xyz+w=size) shared by
  build+draw; only `s_wisps` reused. Ice lights built in `build_floes` (so
  `build_crystals`/`draw_crystals` early-return).
- WET level: `draw_water` still draws the turquoise plane (NOT in the dry
  early-return); `draw_floes` lays only ice over it.
- CLI: `floes` / `glacier` / `harbor` launches straight into it.
