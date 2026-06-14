# The Oasis — Level Design (LEVEL_OASIS = 87)

A desert spring under a brilliant sun: a turquoise **pool** wells up from a tumble of wet rocks
at its heart, ringed by golden **dunes**, leaning **date palms**, **reed** and cattail beds, a
low black **Bedouin tent** on the bank and a resting **camel**. The eighty-fourth **non-boss**
level — parched wanderers ("The Thirsting") have risen at the water. Clear them all to win; the
bonfire then ignites as usual.

New procedural geometry, not a recolour — a **desert-oasis** silhouette (an open turquoise basin
fringed by date palms, dunes, reeds, a tent and a camel) found in none of the other eighty-seven
levels. A **WET** level — the reflective water plane *is* the oasis pool the player fights on
(like the Lotus Pond and the Stilt Village), with the banks/dunes/vegetation laid only around the
rim so the centre stays open water. A distinct desert scene from the dry **Desert Ziggurat**
(stepped ruin), the **Savanna** (acacia grassland) and the indoor palm **Conservatory** (palms
under a glass dome).

## Theme & palette
- **Mood:** shimmering, sun-blasted, hushed — cool water in a sea of hot sand.
- **Light (render.cpp):** high desert sun `(0.36,0.82,0.30)`, brilliant warm sunlight
  `(1.18,1.06,0.82)`, warm sand-bounce fill, pale heat haze (`~#DBCCA8`, low density `0.0060`
  so the far dunes haze out). Reuses `sky_ice.fs` (bright blue) + `water_ice.fs` (turquoise).
  **WET** (`draw_water` runs; `draw_oasis` lays only the rim).
- **Glow:** warm desert sun `(1.00,0.80,0.42)`.

## Geometry (arena.cpp :: build_oasis / draw_oasis)
Reuses `s_dome` (dunes / sandy banks / wellspring rocks / camel hump), the lit `s_cyl` (tent
poles / water jars / camel legs), `s_column` (tent cloth + walls / rug / cattail heads / camel
body+head), `draw_bone_seg` (reeds / camel neck), and the **reused `draw_palm`** helper from the
Conservatory (curving trunk + arching fronds + date clusters).
1. **The pool** — the WET water plane (turquoise `water_ice`), the open fighting floor.
2. **Wellspring** — a tumble of wet rocks at centre (the landmark obstacle) with a reed tuft and
   bubbles welling up.
3. **Dunes & banks** — a tall outer ring of golden dune mounds + a lower inner ring of sandy
   shore mounds, leaving the centre open water.
4. **Vegetation** — 8 date palms ringing the pool (obstacles) + five reed/cattail clusters at
   the water's edge.
5. **Camp life** — a low black goat-hair Bedouin tent with a sloped roof, open front, rug and
   water jars (obstacle), and a resting dromedary camel beside it.
6. **Atmosphere** — additive sun-sparkle, the wellspring bubbling, and heat shimmer over the dunes.

## Encounter (mobs.cpp)
- **8 "Thirsting"** — the `Mob` horde, level-aware spawn set (`oasis_defs`), risen across the
  pool. Same lightweight AI, lock-on, aggregate HUD bar (named *The Thirsting*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–86 and the `Boss` are untouched. `mob_level` now covers
  `… || TOTEM || OASIS`. **No new global struct/vector** — the 8 palm positions are a fixed array
  shared by build (obstacles) + draw; dunes/reeds seeded deterministically; tent/camel/rocks fixed;
  only `s_wisps` reused. Spring/palm/tent lights built in `build_oasis` (so `build_crystals` /
  `draw_crystals` early-return).
- WET level: `draw_water` is NOT in its dry early-return (the pool draws); `draw_crystals` IS.
- CLI: `oasis` / `palms` / `spring` launches straight into it.
