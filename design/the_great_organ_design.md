# The Great Organ — Level Design (LEVEL_ORGAN = 78)

A dim, hushed hall dominated by a colossal pipe organ: graduated metal pipe ranks rise in
towers and flats within a gilded wooden case, horizontal trumpet pipes fan forward, a
console of stepped keyboards and stop knobs sits before it on a red carpet aisle, and
candelabra burn in the gloom. The seventy-fifth **non-boss** level — the hall's risen dead
("The Discordant") rise in the nave. Clear them all to win; the bonfire then ignites.

New procedural geometry, not a recolour — a **pipe-organ** silhouette (a wall of graduated
metal pipes in a carved case + a keyboard console) found in none of the other seventy-eight
levels. A **DRY** level (stone hall floor). Deliberately distinct from the Cathedral Nave (a
Gothic interior of clustered piers, pointed arches and a rose window): here the dominant
feature is the organ itself — its tiered pipe façade — not the architecture.

## Theme & palette
- **Mood:** solemn, gilded, echoing — candlelight on metal pipes in a dark hall.
- **Light (render.cpp):** soft overhead `(0.30,0.80,0.34)`, warm candle/interior light
  `(0.92,0.82,0.64)`, dim interior fill, dark interior haze (`~#4D474D`, density `0.0100`).
  Reuses `sky_storm.fs`; **DRY** (`draw_organ` lays the hall floor).
- **Glow:** warm candle / gilt `(1.00,0.80,0.50)`.

## Geometry (arena.cpp :: build_organ / draw_organ)
Reuses the lit `s_cyl` (pipes / trumpet pipes / stop knobs / candelabra), `s_column`
(case / pilasters / cornice / floor / walls / carpet / console / manuals / bench / pipe
mouths), `s_cone` (pinnacle caps), and `s_dome` (candelabra cups).
1. **The organ** — a wooden case (plinth + side pilasters + cornice + gilt) housing **two
   pipe ranks** of 24 `s_cyl` pipes each, their heights set by a **three-tower / flat
   envelope** (`fmaxf` over tower distances) for the classic organ façade, with dark mouth
   slots and gilt pinnacle caps; horizontal **trumpet pipes (en chamade)** fan forward.
2. **The console** — a wooden case with three stepped white keyboard manuals, a row of gold
   stop knobs and a bench, at the centre of the hall (the landmark obstacle).
3. **The hall** — a stone floor, two side walls, and a red carpet aisle up to the organ;
   the organ case is a back barrier (sampled obstacles).
4. **Atmosphere** — additive warm gilt glow + flickering candle flames + drifting dust.

## Encounter (mobs.cpp)
- **8 "Discordant"** — the `Mob` horde, level-aware spawn set (`organ_defs`), in the nave.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Discordant*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–77 and the `Boss` are untouched. `mob_level` now covers
  `… || SILO || ORGAN`. **No new global struct/vector** — the organ/console/hall are
  deterministic; the case-barrier obstacles are sampled along the back in `build_organ`;
  only `s_wisps` reused. Candle lights built in `build_organ` (so `build_crystals`/
  `draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_organ` lays the hall floor.
- CLI: `organ` / `pipes` / `organhall` launches straight into it.
