# The Terracotta Army — Level Design (LEVEL_TERRACOTTA = 91)

A dim, dusty excavation pit: ranks of standing **clay warrior statues** recede in rows between
rammed-earth baulks, a bronze **war-chariot** drawn by clay horses waits at the back, toppled
torsos and head fragments litter the dirt, broken **timber roof beams** span overhead and shafts
of dusty light fall through the gaps. The eighty-eighth **non-boss** level — the clay ranks have
risen ("The Buried"). Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **buried-army excavation** silhouette (rows of
soldier statues in earthen trenches under a timber roof) found in none of the other ninety-one
levels. A **DRY** level (the dirt pit). Distinct from every other statue/figure level: the
**Moai** (giant heads on a hillside), the **Fallen Titan** (one lying colossus), and the
**Sentinel Court** (a few standing stone guardians) — this is *ranks of many small humanoid
statues* receding into a dig.

## Theme & palette
- **Mood:** hushed, ancient, dusty — an unearthed army standing in the gloom of the pit.
- **Light (render.cpp):** soft raking light `(0.40,0.70,0.40)` into the trench, warm earthy
  daylight `(1.08,0.94,0.74)`, dim earthy fill, thick ochre dust haze (`~#A88F6B`, high density
  `0.0110`). Reuses `sky_ice.fs`; **DRY** (`draw_terracotta` lays the dirt; `water_storm.fs`
  placeholder is unused).
- **Glow:** warm dusty light `(1.00,0.74,0.42)`.

## Geometry (arena.cpp :: draw_warrior / build_terracotta / draw_terracotta)
Reuses the lit `s_cyl` (warrior robes / horse-cart wheels via `s_torus` / ramp railing / fallen
torsos), `s_column` (dirt floor / torsos+arms / earthen baulks+back wall / chariot / timber
beams / ramp), `s_dome` (heads / head fragments / horse heads), and `s_torus` (chariot wheels).
1. **The ranks** (`draw_warrior`, an rlgl-matrix helper: flared robe + armoured torso + arms +
   head + topknot) — five rows of nine warriors, staggered, receding into the pit, each row
   fronted by a low rammed-earth baulk; an occasional gap in the ranks.
2. **Bronze war-chariot** (the back obstacle) — a chariot box on `s_torus` wheels drawn by two
   clay horses (body + arched neck + head), against a tall back earthen wall.
3. **The dig** — toppled torsos lying down + head fragments scattered in the front area, an
   excavation ramp with a timber railing on one side.
4. **Roof** — partial broken timber roof beams (a cross-and-long grid) over the pit, one
   collapsed beam leaning.
5. **Atmosphere** — additive dusty light shafts (`DrawCylinderEx` cones from roof gaps to the
   floor) + drifting dust motes + a warm glow.

## Encounter (mobs.cpp)
- **8 "Buried"** — the `Mob` horde, level-aware spawn set (`terracotta_defs`, in the front/mid
  pit so the back ranks stay framed). Same lightweight AI, lock-on, aggregate HUD bar (named
  *The Buried*), victory-on-clear. (The risen mobs visually echo the clay statues.)

## Integration notes
- Purely additive: levels 0–90 and the `Boss` are untouched. `mob_level` now covers
  `… || JANTAR || TERRACOTTA`. **No new global struct/vector** — the ranks are parametric; the
  chariot/back-wall obstacle + the light-shaft lights are pushed in `build_terracotta`; toppled
  fragments + motes are seeded; only `s_wisps` reused. Lights built in `build_terracotta` (so
  `build_crystals` / `draw_crystals` early-return). A new `draw_warrior` helper (rlPushMatrix +
  local `DrawModelEx`s) draws each statue.
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_terracotta`
  lays the dirt floor.
- CLI: `terracotta` / `army` / `warriors` launches straight into it.
