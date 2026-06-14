# The Great Mosque — Level Design (LEVEL_MOSQUE = 85)

A sunlit mosque: a great blue dome on a drum crowns a prayer hall behind a grand pointed
iwan portal, four tall slender minarets with balconies and crescent finials rise at the
corners, a pointed-arch arcade runs down each side, and a tiered ablution fountain plays at
the centre of a geometric tile courtyard. The eighty-second **non-boss** level — the mosque's
risen faithful ("The Prostrate") rise in the courtyard. Clear them all to win; the bonfire
then ignites as usual.

New procedural geometry, not a recolour — an **Islamic-architecture** silhouette (a great
dome + crescent-tipped minarets + pointed arches + a geometric tile courtyard) found in none
of the other eighty-five levels. A **DRY** level (tile courtyard). Deliberately distinct from
the Cathedral Nave (Gothic), the Amphitheatre (Greek), the Hypostyle Hall (Egyptian columns),
the Triumphal Arch (Roman), and the Great Organ — a wholly different architectural tradition.

## Theme & palette
- **Mood:** serene, radiant, geometric — warm afternoon light on tile, gold and a blue dome.
- **Light (render.cpp):** warm afternoon sun `(0.38,0.74,0.34)`, warm golden daylight
  `(1.10,0.98,0.78)`, soft warm fill, soft warm haze (`~#BDBDB3`, density `0.0070`). Reuses
  `sky_ice.fs`; **DRY** (`draw_mosque` lays the tile courtyard).
- **Glow:** warm gold / lamp `(1.00,0.85,0.55)`.

## Geometry (arena.cpp :: draw_minaret / build_mosque / draw_mosque)
Reuses the lit `s_column` (hall base / iwan / floor / tiles / arcade architraves), `s_cyl`
(drum / minaret shafts + balconies / arcade columns / fountain), `s_dome` (the great dome /
minaret caps / finials), and `s_cone` (pointed-arch heads).
1. **Prayer hall** — a cubic base + a gilt base-band + a drum + a **great blue dome** (with a
   faint outer shell) + a crescent finial, and a grand **pointed iwan portal** (`s_cone`
   arch head over a dark recess). The hall front is a barrier (obstacles).
2. **Minarets** (`draw_minaret`) — four slender banded shafts with a balcony, an upper shaft,
   a domed cap and a crescent finial, at the courtyard corners (obstacles).
3. **Arcade** — a pointed-arch colonnade (column + architrave + `s_cone` arch fill) down each
   side of the courtyard.
4. **Courtyard** — a checkered geometric **tile floor** with a gold **star-rosette** centre
   and a tiered **ablution fountain** (the landmark obstacle) with a water jet.
5. **Atmosphere** — additive warm/hanging-lamp glow + the fountain jet + dust motes.

## Encounter (mobs.cpp)
- **8 "Prostrate"** — the `Mob` horde, level-aware spawn set (`mosque_defs`), in the courtyard.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Prostrate*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–84 and the `Boss` are untouched. `mob_level` now covers
  `… || SAVANNA || MOSQUE`. **No new global struct/vector** — the hall / dome / minarets /
  arcade / fountain are all deterministic; hall-front + minaret obstacles fixed in
  `build_mosque`; only `s_wisps` reused. Lamp lights built in `build_mosque` (so
  `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_mosque` lays the tile courtyard.
- CLI: `mosque` / `minaret` / `dome` launches straight into it.
