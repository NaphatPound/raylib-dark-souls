# The Bath House — Level Design (LEVEL_BATHS = 103)

A grand Roman *thermae* wreathed in steam: a turquoise bathing **pool** is ringed by a marble
**colonnade** carrying an architrave, with statues standing between the columns, a tiered marble
**fountain** in the centre, bronze braziers, marble **steps** descending into the water, a
lion-head wall **spout** pouring in, urns, and a great domed **caldarium** rising behind. The one
hundredth **non-boss** level — the bath's languid dead have risen ("The Languid"). Clear them all
to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **bath-house** silhouette (a colonnaded pool + a
domed caldarium + a tiered fountain) found in none of the other one hundred three levels. A
**WET** level — the reflective water plane *is* the warm bath the player fights in, with the
marble coping, colonnade and steps at the rim. A bathing complex — distinct from the **Sunken
Aqueduct** (a viaduct of water channels) and the **Geyser Springs** (mineral terrace-pools):
here the pool is a built, colonnaded bath.

## Theme & palette
- **Mood:** warm, hushed, vaporous — soft light and steam over still water.
- **Light (render.cpp):** soft light through the dome `(0.36,0.74,0.40)`, warm marble glow
  `(1.12,1.04,0.90)`, soft even fill, pale warm steam (`~#D1D1CC`, density `0.0095`). Reuses
  `sky_ice.fs` + `water_ice.fs` (turquoise). **WET** (`draw_water` runs; `draw_baths` lays the
  marble).
- **Glow:** warm brazier on marble `(1.00,0.66,0.34)`.

## Geometry (arena.cpp :: draw_statue / build_baths / draw_baths)
Reuses `s_column` (coping + mosaic / steps / statue plinths+torsos / caldarium drum + doorway /
spout / urns), the lit `s_cyl` (columns via the **reused `draw_petra_column`** / fountain tiers /
braziers / statue legs+arms / urns), `s_dome` (statue heads / fountain finial / caldarium dome /
spout / urn lids), and `s_torus` (the architrave ring).
1. **The pool** — the WET turquoise water (the fighting floor), with a marble coping ring and a
   terracotta/gold mosaic band on the bottom.
2. **The colonnade** — sixteen marble columns (`draw_petra_column`) + an `s_torus` architrave
   ring + four statues (`draw_statue`) between them; the ring is the confining obstacle (gap at
   the front steps).
3. **The fountain** (the central obstacle) — a tiered marble basin + pedestal + bowl + gold
   finial, jetting water.
4. **The caldarium** — a great domed marble hot-room rising behind the back colonnade, with an
   oculus finial and a dark arched doorway.
5. **Fittings** — bronze braziers, marble steps, a lion-head wall spout, urns; additive brazier
   fire + fountain + spout jets + steam rising off the whole bath.

## Encounter (mobs.cpp)
- **8 "Languid"** — the `Mob` horde, level-aware spawn set (`baths_defs`), in the bath. Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Languid*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–102 and the `Boss` are untouched. `mob_level` now covers
  `… || ALCHEMY || BATHS`. **No new global struct/vector** — the bath is parametric; the
  colonnade-ring + fountain obstacles and the lights are pushed in `build_baths`; steam is seeded;
  only `s_wisps` reused. A new `draw_statue` helper + the reused `draw_petra_column`. Lights built
  in `build_baths` (so `build_crystals` / `draw_crystals` early-return). Uses the global
  `water_level` for the mosaic + steam.
- WET level: `draw_water` is NOT in its dry early-return (the bath draws); `draw_crystals` IS.
- CLI: `baths` / `thermae` / `bathhouse` launches straight into it.
