# The Hypostyle Hall — Level Design (LEVEL_HYPO = 79)

A Karnak-style forest of colossal carved columns: thick banded sandstone shafts with
hieroglyph rings flare into papyrus-bell capitals supporting heavy architrave beams, shafts
of light stream down from the clerestory between them, and a gilded altar stands at the
centre amid fallen column drums. The seventy-sixth **non-boss** level — the hall's risen
dead ("The Engraved") rise among the columns. Clear them all to win; the bonfire ignites.

New procedural geometry, not a recolour — a **hypostyle / column-forest** silhouette (a dense
grid of giant papyrus-capital columns under architraves, lit by clerestory shafts) found in
none of the other seventy-nine levels. A **DRY** level (stone hall floor). Deliberately
distinct from the Cathedral Nave (Gothic clustered piers + pointed arches), the Amphitheatre
(Greek tiered seating + scaenae frons), the Overgrown Temple (jungle ruins), and the open
Desert Ziggurat: this is a roofed Egyptian hall whose awe comes from the sheer density and
scale of its columns.

## Theme & palette
- **Mood:** vast, solemn, golden-dim — light falling in bars through a forest of stone.
- **Light (render.cpp):** light from the clerestory `(0.30,0.84,0.30)`, warm sandstone light
  `(1.00,0.86,0.60)`, dim warm fill, dark warm interior haze (`~#5C5242`, density `0.0098`).
  Reuses `sky_storm.fs`; **DRY** (`draw_hypo` lays the hall floor).
- **Glow:** warm sandstone / light-shaft `(1.00,0.82,0.45)`.

## Geometry (arena.cpp :: draw_hypocolumn / hypo_layout / build_hypo / draw_hypo)
Reuses the lit `s_cyl` (column bases / shafts / hieroglyph bands / fallen drums), `s_cone`
(the flared papyrus-bell capitals), and `s_column` (floor / hieroglyph slabs / abaci /
architrave beams / side roof slabs / altar).
1. **Columns** (`draw_hypocolumn`) — a base + a banded shaft (four hieroglyph rings) + a
   papyrus-bell capital (a 180°-flared `s_cone`, wide at the top) + an abacus. A 4×3 grid of
   12 colossal columns (~19u tall), all obstacles, with the central aisle left open.
2. **Roof** — architrave beams along the column rows + side roof slabs, leaving the central
   nave open to a clerestory.
3. **Floor & altar** — a hieroglyph-slab grid floor and a gilded central altar (the landmark
   obstacle); fallen column drums scattered.
4. **Atmosphere** — additive **god-ray light shafts** streaming down the nave + drifting
   dust motes + warm glow.

## Encounter (mobs.cpp)
- **8 "Engraved"** — the `Mob` horde, level-aware spawn set (`hypo_defs`), among the columns.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Engraved*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–78 and the `Boss` are untouched. `mob_level` now covers
  `… || ORGAN || HYPO`. **No new global struct/vector** — `hypo_layout` fills a local
  `std::vector<Vector3>` of column positions shared by build (obstacles) + draw; roof/altar
  fixed; only `s_wisps` reused. Light-shaft glows built in `build_hypo` (so `build_crystals`/
  `draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_hypo` lays the hall floor.
- CLI: `hypostyle` / `karnak` / `pillars` launches straight into it.
