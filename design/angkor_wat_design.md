# Angkor Wat — Level Design (LEVEL_ANGKOR = 110)

The Khmer temple-mountain in the humid tropics: the iconic quincunx of five tapering, ribbed
**lotus-bud prasat towers** — a tall central one and four corner towers — rises from a stepped,
galleried sandstone platform, approached by a stone **causeway** flanked by multi-headed **naga**
serpent balustrades, with library buildings, creeping jungle vines and lily pads on the broad
reflecting **moat**. The one hundred seventh **non-boss** level — the jungle-swallowed dead have
risen ("The Devoured"). Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **temple-mountain** silhouette (five lotus-bud towers
on galleried terraces in a moat) found in none of the other one hundred ten levels. A **WET**
level — the reflecting moat is the water plane, with the raised temple platform (the fighting
floor) on it. The Khmer **lotus-bud / corncob tower** shape is distinct from the Japanese
**Pagoda** (tiered curved roofs), the Tibetan **chortens**, the Russian onion domes of **Saint
Basil's**, the conical tower of the **Great Enclosure** and the smooth dome of the **Mosque**.

## Theme & palette
- **Mood:** ancient, sun-warmed, jungle-hushed — grey sandstone mirrored in still green water.
- **Light (render.cpp):** warm jungle sun `(0.40,0.74,0.36)`, warm sandstone glow
  `(1.14,1.06,0.84)`, soft warm fill, humid green-gold haze (`~#C7CCAE`, density `0.0072`).
  Reuses `sky_ice.fs` + `water_ice.fs` (the reflecting moat). **WET** (`draw_water` runs;
  `draw_angkor` lays the platform).
- **Glow:** warm sun on sandstone `(1.00,0.74,0.42)`.

## Geometry (arena.cpp :: draw_prasat / build_angkor / draw_angkor)
Reuses `s_column` (platform + terraces / tier bodies + cornice lips + redentations / causeway /
naga railings + hoods / libraries / gallery beams), the lit `s_cyl` (gallery colonnade), `s_cone`
(tower finials / naga cobra-hood heads), `s_dome` (lotus-bud caps / vine clumps / lily pads), and
`draw_bone_seg` (creeping jungle vines).
1. **The platform** — a raised galleried sandstone terrace (the fighting floor) with stepped
   edges and a pillared gallery colonnade, on the WET moat.
2. **Prasat towers** (`draw_prasat`) — a base + eight receding tapering tiers (each with a
   cornice lip and four corner redentations, the corncob look) capped by a lotus-bud and a
   finial; a tall central tower + four corner towers (obstacles).
3. **The causeway** — a stone bridge across the moat flanked by serpent-body balustrades ending
   in seven-headed cobra **naga** hoods.
4. **The setting** — two library buildings, creeping jungle vines on the stone, lily pads on the
   moat; additive tower glow + moat sparkle + birds.

## Encounter (mobs.cpp)
- **8 "Devoured"** — the `Mob` horde, level-aware spawn set (`angkor_defs`), among the towers.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Devoured*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–109 and the `Boss` are untouched. `mob_level` now covers
  `… || BUDDHA || ANGKOR`. **No new global struct/vector** — the temple is parametric; the five
  tower obstacles and the lights are pushed in `build_angkor`; vines/pads/sparkle are seeded;
  only `s_wisps` reused. A new `draw_prasat` helper. Lights built in `build_angkor` (so
  `build_crystals` / `draw_crystals` early-return). Uses the global `water_level` for the lily
  pads + moat sparkle.
- WET level: `draw_water` is NOT in its dry early-return (the moat draws); `draw_crystals` IS.
- CLI: `angkor` / `prasat` / `templemount` launches straight into it.
