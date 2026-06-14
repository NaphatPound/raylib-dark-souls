# The Balloon Festival — Level Design (LEVEL_BALLOON = 75)

A dawn launch field of hot-air balloons: colourful striped envelopes bob gently on riser
ropes over wicker baskets, burner flames roaring up into them, a half-inflated envelope
draped on the grass, fuel cylinders and ground tethers about — and more balloons drift far
off in the morning mist. The seventy-second **non-boss** level — the festival's lost dead
("The Untethered") rise in the field. Clear them all to win; the bonfire then ignites.

New procedural geometry, not a recolour — a **hot-air-balloon-festival** silhouette (round
striped envelopes + baskets + burners) found in none of the other seventy-five levels. A
**DRY** level (grassy launch field). Deliberately distinct from the Skydock (a single
*elongated* dirigible + gondola high overhead) and the Great Wheel (a *rotating* Ferris
wheel): these are many *round* tethered balloons kept low on the field.

## Theme & palette
- **Mood:** bright, hopeful, festive — first light over a meadow of rising colour.
- **Light (render.cpp):** low warm dawn sun `(0.42,0.66,0.40)`, warm dawn glow
  `(1.06,0.94,0.74)`, cool morning fill, pale morning mist (`~#BDC2C7`, density `0.0078`).
  Reuses `sky_ice.fs`; **DRY** (`draw_balloon_field` lays the grass).
- **Glow:** warm burner-flame `(1.00,0.70,0.35)`.

## Geometry (arena.cpp :: draw_balloon / balloon_layout / build_balloon / draw_balloon_field)
Reuses the lit `s_dome` (envelopes via two hemispheres / grass tufts / the half-inflated
envelope), `s_cyl` (stripe band / fuel cylinders), `s_column` (baskets / floor), and
`draw_bone_seg` (riser ropes + ground tethers).
1. **Balloons** (`draw_balloon`) — a striped envelope (upper + lower dome + a stripe band)
   that **bobs via `s_time`** over a wicker basket, rigged with four riser ropes and two
   ground tethers. Kept low (envelope centre ~4.6u) so they frame at eye level. 6 across the
   field in five festive colours + a central great balloon (the landmark obstacle).
2. **Field clutter** — a half-inflated envelope on the grass, fuel cylinders, grass tufts.
3. **Atmosphere** — additive flickering **burner flames** firing up into each envelope +
   morning glow + faint **distant drifting balloons** high on the horizon + ground mist.

## Encounter (mobs.cpp)
- **8 "Untethered"** — the `Mob` horde, level-aware spawn set (`balloon_defs`), among the
  balloons. Same lightweight AI, lock-on, aggregate HUD bar (named *The Untethered*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–74 and the `Boss` are untouched. `mob_level` now covers
  `… || REDWOOD || BALLOON`. **No new global struct/vector** — `balloon_layout` fills a local
  `std::vector<Vector4>` (x, z, scale, colorIndex) shared by build (basket obstacles) + draw;
  field clutter fixed; only `s_wisps` reused. Burner lights built in `build_balloon` (so
  `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_balloon_field` lays the grass.
- CLI: `balloon` / `balloons` / `festival` launches straight into it.
