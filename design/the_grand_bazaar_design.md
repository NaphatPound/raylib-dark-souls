# The Grand Bazaar — Level Design (LEVEL_BAZAAR = 94)

A covered Middle-Eastern **souk** in warm lantern-light: rows of merchant **stalls** piled with
wares line the lanes under striped cloth **awnings**, hung with lanterns and carpets; stone
**archways** span the central lane; a domed crossing (qaysariyya) with a small fountain stands at
the heart, and spice pyramids, pottery and rolled carpets spill across the flagstones. The
ninety-first **non-boss** level — the market's shades have risen ("The Covetous"). Clear them all
to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **covered-market** silhouette (lanes of awninged
stalls + a domed crossing + archways) found in none of the other ninety-four levels. A **DRY**
level (the market floor). A **commerce** scene, not a temple — distinct from the open civic
**Grand Plaza** (a bare square + a column/statue), the **Forsaken Fairground** (rides + big-top
tents) and the **Siege Camp** (military tents): this is a roofed market of laden stalls and
hanging wares.

## Theme & palette
- **Mood:** warm, crowded, hushed — a labyrinth of goods under filtered light.
- **Light (render.cpp):** soft light `(0.35,0.72,0.40)` through the awnings, warm filtered
  daylight `(1.10,0.96,0.74)`, dim warm interior fill, warm spice-dust haze (`~#A89476`, density
  `0.0090`). Reuses `sky_ice.fs`; **DRY** (`draw_bazaar` lays the floor; `water_storm.fs`
  placeholder is unused).
- **Glow:** warm lantern light `(1.00,0.70,0.36)`.

## Geometry (arena.cpp :: draw_stall / build_bazaar / draw_bazaar)
Reuses `s_column` (floor + carpet patches / stall counters+shelves / awnings / arch lintels /
hung carpets / rolled carpets), the lit `s_cyl` (stall posts / arch + dome piers / drum /
fountain / spice baskets / jars), `s_cone` (arch fills / spice pyramids), `s_dome` (piled wares /
the dome / hanging lanterns), and `s_torus` (the dome's cornice ring).
1. **Stalls** (`draw_stall`, an rlgl-matrix helper: counter + back shelf + posts + awning +
   piled wares + a jar + a hung carpet + a lantern) — two rows of six flanking the central lane
   (obstacles).
2. **Awnings** — striped cloth strips stretched over the side lanes behind each row.
3. **Archways** — two stone arches (piers + lintel + keystone fill) spanning the central lane.
4. **Domed crossing** (the central obstacle) — eight piers + a cornice ring + a drum + a dome +
   a finial, over a small fountain.
5. **Goods** — spice pyramids in baskets, rolled carpets; additive lantern glow + light shafts
   through awning gaps (`DrawCylinderEx`) + spice motes.

## Encounter (mobs.cpp)
- **8 "Covetous"** — the `Mob` horde, level-aware spawn set (`bazaar_defs`), among the stalls.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Covetous*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–93 and the `Boss` are untouched. `mob_level` now covers
  `… || PETRA || BAZAAR`. **No new global struct/vector** — the market is parametric; the central
  dome + stall-row obstacles + the lights are pushed in `build_bazaar`; carpets/goods/motes are
  seeded; only `s_wisps` reused. A new `draw_stall` helper (rlPushMatrix + local `DrawModelEx`s)
  draws each stall. Lights built in `build_bazaar` (so `build_crystals` / `draw_crystals`
  early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_bazaar` lays
  the market floor.
- CLI: `bazaar` / `souk` / `market` launches straight into it.
