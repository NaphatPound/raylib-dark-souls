# The Wind Towers — Level Design (LEVEL_BADGIR = 111)

A Persian desert town (Yazd) baking under a hard sun: tall rectangular adobe **badgirs**
(wind-catchers) with their slatted vents rise over packed, flat-roofed mud-brick **houses**, a
great beehive **yakhchal** (ice-house) cone stands at the back, a domed cistern hums beside the
alleys, date palms throw thin shade and clay water-jars line the lanes. The one hundred eighth
**non-boss** level — the heat-killed dead have risen ("The Sun-Struck"). Clear them all to win;
the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **windcatcher-town** silhouette (square slatted
wind-towers + flat-roof adobe + a beehive ice-house cone) found in none of the other one hundred
eleven levels. A **DRY** level (the packed earth). A fresh Persian vernacular — the square
slatted **badgir** tower and the tall ring-coursed **yakhchal** cone are distinct from every
other tower: the round mosque **minaret**, the cylindrical **Grain Silo**, the carved **Totem**
poles, the timber **siege tower** and the blue-glazed **Ishtar** towers.

## Theme & palette
- **Mood:** sun-blasted, hushed, dusty — pale adobe and deep shade in the heat.
- **Light (render.cpp):** high blazing sun `(0.40,0.82,0.30)`, brilliant warm daylight
  `(1.20,1.10,0.86)`, warm earthy fill, pale dusty heat haze (`~#D6C7A4`, low density `0.0055`).
  Reuses `sky_ice.fs` (bright blue); **DRY** (`draw_badgir` lays the earth; `water_storm.fs`
  placeholder is unused).
- **Glow:** hot sun on adobe `(1.00,0.78,0.46)`.

## Geometry (arena.cpp :: draw_windtower / draw_adobehouse / build_badgir / draw_badgir)
Reuses `s_column` (earth floor + alleys / tower shafts + vent recesses + mullions + caps / house
walls + parapets + doorways / clay jars / yakhchal entrance), the lit `s_cyl` (yakhchal base +
ring courses + vent / cistern), `s_cone` (the yakhchal dome / house door arches), `s_dome` (roof
domes / cistern dome), and the **reused `draw_palm`**.
1. **Badgirs** (`draw_windtower`) — a tall square adobe shaft with a dark vent recess and light
   vertical mullions (the slatted catcher) near the top and an overhanging cap; six at varying
   heights, some standalone, some atop houses (obstacles).
2. **The yakhchal** (the back landmark obstacle) — a cylindrical base + a tall ring-coursed adobe
   cone (the beehive ice-house) with a top vent and an entrance.
3. **Houses** (`draw_adobehouse`) — flat-roofed mud-brick blocks with parapets and arched
   doorways, clustered and terraced, some with rooftop domes (obstacles).
4. **The town** — a low domed cistern (ab anbar) with its own badgir, date palms, clay
   water-jars; additive sun glow + drifting heat shimmer + pigeons.

## Encounter (mobs.cpp)
- **8 "Sun-Struck"** — the `Mob` horde, level-aware spawn set (`badgir_defs`), in the alleys.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Sun-Struck*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–110 and the `Boss` are untouched. `mob_level` now covers
  `… || ANGKOR || BADGIR`. **No new global struct/vector** — the town is parametric; the yakhchal +
  tower + house obstacles and the lights are pushed in `build_badgir`; alleys/jars/shimmer are
  seeded; only `s_wisps` reused. New `draw_windtower` / `draw_adobehouse` helpers + the reused
  `draw_palm`. Lights built in `build_badgir` (so `build_crystals` / `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_badgir` lays
  the earth.
- CLI: `badgir` / `windcatcher` / `yazd` launches straight into it.
