# Saint Basil's — Level Design (LEVEL_BASIL = 99) — the 100th level

The Russian onion-dome cathedral on the cold square: a cluster of brick chapel-towers, each
crowned by a distinct **bulbous, ribbed, colourful onion dome** with a gold cross, rings a tall
central **tent-spire** tower; the whole stands on a raised stone platform with a grand staircase,
on a cobbled square dusted with snow. The ninety-sixth **non-boss** level (and the **100th level
overall**) — the cathedral's shades have risen ("The Devout"). Clear them all to win; the bonfire
then ignites as usual.

New procedural geometry, not a recolour — an **onion-dome cathedral** silhouette (a cluster of
bulbous ribbed domes around a tent spire) found in none of the other ninety-nine levels. A **DRY**
level (the cobbled square). The onion-dome *shape* (narrow at the base, bulging at the belly,
pinched to a point) is genuinely new — distinct from the smooth single hemisphere of **The Great
Mosque** (and its minarets), the Gothic spires of the **Cathedral Nave**, and the conical stone
tower of **The Great Enclosure**.

## Theme & palette
- **Mood:** festive, frigid, grand — a riot of colour against a pale winter sky.
- **Light (render.cpp):** clear winter sun `(0.36,0.80,0.34)`, bright cool daylight
  `(1.12,1.08,1.02)`, cool blue-sky fill, pale cold haze (`~#CCD1DB`, density `0.0058`). Reuses
  `sky_ice.fs` (bright blue); **DRY** (`draw_basil` lays the square; `water_storm.fs` placeholder
  is unused).
- **Glow:** warm candle-window glow `(1.00,0.80,0.50)`.

## Geometry (arena.cpp :: draw_onion_dome / build_basil / draw_basil)
Reuses `s_cyl` (drums / platform / cornices / finial necks), `s_column` (square + cobble pattern /
snow patches / windows / staircase / cross arms), `s_cone` (kokoshnik gables / the tent spire /
the onion's pointed top), `s_dome` (the onion belly via two flipped hemispheres / finial balls),
and `draw_bone_seg` (the onion + tent ribs).
1. **Onion domes** (`draw_onion_dome`) — a brick drum with arched windows + a cornice + kokoshnik
   gables, then the **bulbous onion** (two flipped `s_dome` hemispheres for the wider-than-drum
   belly + an `s_cone` taper to the point), vertical ribs hugging it, and a gold neck + ball +
   cross. Eight chapels, each a different dome colour/rib, ring the centre (obstacles).
2. **Central tent tower** (the landmark obstacle) — a tall brick drum + an octagonal tent spire
   with ribs + a small crowning gold onion.
3. **The platform** — a raised two-tier stone base with a grand front staircase.
4. **The square** — cobbles with an inlaid grid and scattered snow patches; additive warm window
   glow + drifting snow + a few pigeons.
- Note: at eye level the tall domes frame high in the autopilot demo (the player camera tilts up
  in-game); the cathedral sits at the back so the horde fights on the open square in front.

## Encounter (mobs.cpp)
- **8 "Devout"** — the `Mob` horde, level-aware spawn set (`basil_defs`), on the square in front
  of the cathedral. Same lightweight AI, lock-on, aggregate HUD bar (named *The Devout*),
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–98 and the `Boss` are untouched. `mob_level` now covers
  `… || ZIMBABWE || BASIL`. **No new global struct/vector** — the cathedral is parametric; the
  central-tower + chapel obstacles and the lights are pushed in `build_basil`; snow/pigeons are
  seeded; only `s_wisps` reused. A new `draw_onion_dome` helper draws each dome. Lights built in
  `build_basil` (so `build_crystals` / `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_basil` lays
  the cobbled square.
- CLI: `basil` / `onion` / `kremlin` launches straight into it.
