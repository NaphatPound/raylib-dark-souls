# Stilt Village — Level Design (LEVEL_STILT = 47)

A fishing hamlet raised over a brackish lagoon: huts perched on tall stilts, linked by
rickety plank walkways radiating from a central stilt longhouse, hung with net-drying
frames and fish racks, dotted with moored coracles, buoys and sandbars. The
forty-fourth **non-boss** level — the drowned villagers ("The Brackish") rise from
the shallows. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **stilt fishing village** silhouette
(stilt-huts + plank walkways + net frames over open water) found in none of the other
forty-seven levels. A **WET** level (the reused water plane is the lagoon). Distinct
from the Blighted Bog (swamp willows, a single stilt hut), the Shipwreck Cove (broken
hulls) and the Watermill (race).

## Theme & palette
- **Mood:** salt-worn, hushed, coastal — soft overcast light over a still lagoon, nets
  swaying and lanterns glowing in the hut windows.
- **Light (render.cpp):** soft overcast coastal sun, warm-grey daylight `(0.96, 0.92,
  0.84)`, soft sea ambient, pale brackish sea haze (`~#949E9E`, density `0.0100`).
  Reuses `sky_storm.fs`; **WET** — the reused `water_storm` plane is the lagoon.
- **Glow:** warm fisher-lantern `(1.00, 0.80, 0.50)`.

## Geometry (arena.cpp :: build_stilt / draw_stilt / draw_stilthut / draw_net / stilt_layout)
Reuses `s_cyl` (stilts / posts / buoys), `s_column` (decks / walkways), `s_dome`
(sandbars / coracles / fish / buoys), `draw_bone_seg` (ladders / net strands / racks),
and **`draw_cottage`** (the raised cabins).
1. **Stilt huts** — 6 `draw_stilthut` huts: 4 stilts + a deck + a raised cabin (reused
   `draw_cottage`) + a ladder; collision obstacles. Plus a **central stilt longhouse**
   (the landmark obstacle).
2. **Plank walkways** — decks (on posts) radiating from the longhouse to each hut.
3. **Net-drying frames** — `draw_net`: posts + a beam hung with a draped rope mesh
   (sagging vertical + horizontal strands).
4. **Lagoon life** — fish-drying racks, moored coracles (upturned bowls), bobbing
   buoys, and muddy sandbars breaking the surface (no ground slab — the water is the
   floor).
5. **Atmosphere** — additive warm lantern glow + lagoon ripple sparkle.

## Encounter (mobs.cpp)
- **8 "Brackish"** — the `Mob` horde, level-aware spawn set (`stilt_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Brackish*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–46 and the `Boss` are untouched. `mob_level` now covers
  `… || SALT || STILT`. **No new global struct/vector** — `stilt_layout` fills a local
  `std::vector<Vector4>` (huts: xyz + w=yaw) shared by build+draw; only `s_wisps`
  reused. Lantern lights built in `build_stilt` (so `build_crystals`/`draw_crystals`
  early-return).
- WET level: `draw_water` still draws the lagoon plane (NOT in the dry early-return);
  `draw_stilt` lays only the village over it.
- CLI: `stilt` / `fishingvillage` / `lagoon` launches straight into it.
