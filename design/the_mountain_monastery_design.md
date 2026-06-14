# The Mountain Monastery — Level Design (LEVEL_GOMPA = 108)

A Tibetan *gompa* on a high ledge under a deep cold sky: whitewashed battered-wall buildings with
black trapezoidal **windows**, maroon bands and golden roofs surround a flagstone courtyard, a
great white **temple hall** and white **chortens** (stupas) rise at the back, a central pole
radiates a canopy of colourful **prayer-flag** strings, prayer wheels line a wall, butter lamps
burn, and snowy peaks ring the horizon. The one hundred fifth **non-boss** level — the order's
dead have risen ("The Cloistered"). Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **Himalayan-monastery** silhouette (battered white
walls + chortens + a canopy of prayer flags) found in none of the other one hundred eight levels.
A **DRY** level (the courtyard). A fresh Tibetan-Buddhist tradition — distinct from the Japanese
**Pagoda**, the Islamic **Great Mosque**, the Russian onion-domed **Saint Basil's** and the
Gothic **Cathedral Nave**: the prayer-flag canopy, the stupa form and the battered white walls
are new.

## Theme & palette
- **Mood:** serene, austere, thin-aired — colour and gold against white and snow.
- **Light (render.cpp):** crisp mountain sun `(0.36,0.80,0.36)`, clear bright daylight
  `(1.14,1.10,1.02)`, cool blue-sky fill, pale cold mountain haze (`~#D1D9E6`, density `0.0062`).
  Reuses `sky_ice.fs` (deep blue); **DRY** (`draw_gompa` lays the courtyard; `water_storm.fs`
  placeholder is unused).
- **Glow:** warm butter-lamp `(1.00,0.74,0.40)`.

## Geometry (arena.cpp :: draw_chorten / draw_flagstring / draw_tibhouse / build_gompa / draw_gompa)
Reuses `s_column` (courtyard / walls + maroon bands + windows + parapets / temple roof + doorway /
prayer flags / darchen pole), the lit `s_cyl` (roof ornaments / darchen / prayer wheels / lamp
stands), `s_cone` (chorten spires / peaks / roof finials / pole cap), `s_dome` (chorten domes +
sun-moon finials / peak snowcaps), and `draw_bone_seg` (the flag strings via `Vector3Lerp`).
1. **Tibetan houses** (`draw_tibhouse`) — battered white walls with maroon kemar bands,
   black-framed trapezoidal windows, flat-roof parapets and gold gyaltsen ornaments. The temple
   hall (with a golden roof, makara finials and a maroon doorway) + two side houses (obstacles).
2. **Chortens** (`draw_chorten`) — stepped base + dome + harmika + a 13-ring spire + a sun-moon
   finial; two flanking the temple (obstacles).
3. **Prayer flags** (`draw_flagstring`) — a central darchen pole radiates eight drooping strings
   of five-colour flags to the buildings, plus a long cross-string.
4. **Devotions** — a row of golden prayer wheels along a wall, butter-lamp stands, snowy peaks
   ringing the horizon; additive butter-lamp + temple glow + incense smoke + mountain mist.

## Encounter (mobs.cpp)
- **8 "Cloistered"** — the `Mob` horde, level-aware spawn set (`gompa_defs`), in the court. Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Cloistered*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–107 and the `Boss` are untouched. `mob_level` now covers
  `… || MUSEUM || GOMPA`. **No new global struct/vector** — the monastery is parametric; the
  temple + chorten + house + pole obstacles and the lights are pushed in `build_gompa`;
  peaks/mist are seeded; only `s_wisps` reused. New `draw_chorten` / `draw_flagstring` /
  `draw_tibhouse` helpers. Lights built in `build_gompa` (so `build_crystals` / `draw_crystals`
  early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_gompa` lays
  the courtyard.
- CLI: `monastery` / `gompa` / `stupa` launches straight into it.
