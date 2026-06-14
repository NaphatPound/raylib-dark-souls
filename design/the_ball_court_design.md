# The Ball Court — Level Design (LEVEL_BALLCOURT = 92)

A Mesoamerican *juego de pelota* in the jungle heat: a long limestone playing **alley** runs
between two high walls — a sloped talud bench, a vertical upper face with a red-painted cornice,
and a **vertical stone goal-ring** mounted high on each side — while serpent-head sculptures
flank a tall stepped **pyramid-temple** at the back, carved stelae stand at the corners, and
vines creep over the walls. The eighty-ninth **non-boss** level — the court's shades have risen
("The Sacrificed"). Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **ball-court** silhouette (an I-shaped alley between
two ring walls + a pyramid-temple) found in none of the other ninety-two levels. A **DRY** level
(the limestone alley). Distinct from the **round seating bowls** of the Sunken Colosseum and the
Amphitheatre (circular, tiered benches, a central stage) and from the **convex stepped pyramid**
of the Desert Ziggurat — this is a *long straight alley between two parallel sloped walls bearing
goal-rings*, the classic Maya ball court.

## Theme & palette
- **Mood:** humid, ancient, sacred — a sun-baked court reclaimed by the jungle.
- **Light (render.cpp):** warm jungle sun `(0.40,0.78,0.34)`, warm limestone glow
  `(1.14,1.04,0.84)`, soft warm fill, humid green-gold haze (`~#BDC29E`, density `0.0078`).
  Reuses `sky_ice.fs`; **DRY** (`draw_ballcourt` lays the limestone; `water_storm.fs` placeholder
  is unused).
- **Glow:** warm tropical limestone `(1.00,0.78,0.48)`.

## Geometry (arena.cpp :: draw_serpenthead / build_ballcourt / draw_ballcourt)
Reuses `s_column` (alley + painted lines / talud benches / walls + cornices / reliefs / temple
tiers + stair + shrine + roof-comb / altar / stelae / serpent heads + fangs), the lit `s_cyl`
(none extra), `s_dome` (serpent eyes / stela tops / vine leaves), `s_torus` (the **vertical
goal-rings**, rotated 90° about Z so the hoop stands in the Y-Z plane), and `draw_bone_seg`
(vines).
1. **The alley** — a limestone floor with a painted centre line and two end-zone lines, the
   fighting floor.
2. **Ring walls ×2** (obstacles confining the alley) — a sloped talud bench + a vertical upper
   wall + a red cornice + relief panels + a **vertical stone goal-ring** (ochre rim + jade inner
   band) on a mounting block at centre height.
3. **Back pyramid-temple** (the tall landmark, an obstacle) — four stepped tiers + a central
   stair + a shrine with a dark doorway + a roof-comb crest, flanked by two serpent heads.
4. **Front altar platform** (behind the spawn) + four carved corner **stelae**.
5. **Jungle** — vines creeping over the wall tops; additive warm glow + flitting macaws + temple
   incense + dust motes.

## Encounter (mobs.cpp)
- **8 "Sacrificed"** — the `Mob` horde, level-aware spawn set (`ballcourt_defs`), in the alley.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Sacrificed*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–91 and the `Boss` are untouched. `mob_level` now covers
  `… || TERRACOTTA || BALLCOURT`. **No new global struct/vector** — the court is parametric; the
  two wall obstacle rows + the temple/altar obstacles + the lights are pushed in
  `build_ballcourt`; vines/macaws/motes are seeded; only `s_wisps` reused. A new `draw_serpenthead`
  helper (rlPushMatrix + local `DrawModelEx`s) draws each serpent sculpture. Lights built in
  `build_ballcourt` (so `build_crystals` / `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_ballcourt`
  lays the limestone floor.
- CLI: `ballcourt` / `pelota` / `maya` launches straight into it.
