# The Great Buddha — Level Design (LEVEL_BUDDHA = 109)

A colossal seated, meditating **Buddha** of weathered green-bronze sits serene on a lotus
pedestal — legs crossed, hands in the dhyana mudra, a golden halo behind a calm face with its
ushnisha and long ears — enshrined in a great arched **grotto** carved into a tan rock cliff,
flanked by attendant bodhisattvas, with a smoking bronze incense-burner, stone lanterns, braziers
and prayer-stone cairns in the courtyard before it. The one hundred sixth **non-boss** level — the
dead have risen at the Buddha's feet ("The Awakened"). Clear them all to win; the bonfire then
ignites as usual.

New procedural geometry, not a recolour — a **seated-colossus** silhouette (a giant meditating
Buddha + a cliff grotto + bodhisattvas) found in none of the other one hundred nine levels. A
**DRY** level (the courtyard). A distinct statue level — the *seated, meditating* pose sets it
apart from every other colossus: the **Moai** (giant heads), the **Fallen Titan** (a lying
colossus), the **Terracotta Army** (standing ranks), the **Sphinx** (a reclining lion) and the
museum's mounted **T-rex**.

## Theme & palette
- **Mood:** serene, gilded, hushed — incense and bronze in the grotto's warm light.
- **Light (render.cpp):** soft warm afternoon sun `(0.36,0.74,0.42)`, warm gilded light
  `(1.14,1.04,0.84)`, soft warm fill, warm incense haze (`~#C7BD9E`, density `0.0068`). Reuses
  `sky_ice.fs`; **DRY** (`draw_buddha` lays the courtyard; `water_storm.fs` placeholder is unused).
- **Glow:** warm gilded incense `(1.00,0.74,0.42)`.

## Geometry (arena.cpp :: draw_seated_buddha / build_buddha / draw_buddha)
Reuses `s_cyl` (lotus pedestal / bodhisattva bodies / burner / lantern posts), `s_column` (legs +
torso + robe folds + face features + long ears / courtyard + path / cliff + niche / offering
table / lantern boxes), `s_cone` (lotus petals / niche arch / lantern roofs), `s_dome` (knees +
hands + shoulders + head + ushnisha + urna / cairns / burner lid), `s_torus` (the halo/nimbus),
and `draw_bone_seg` (none — all blocks).
1. **The Buddha** (`draw_seated_buddha`, the central obstacle) — a lotus pedestal with petals,
   crossed legs/lap, cupped hands (dhyana mudra), a robed torso with fold-lines, shoulders, a
   serene head (closed eyes, nose, mouth, urna), the ushnisha topknot, long ears and a golden
   halo. **Weathered green-bronze** so it reads against the tan rock.
2. **The grotto** — a tan rock cliff with a great arched niche (rugged stacked blocks) framing
   the Buddha.
3. **Attendants** — two smaller bodhisattva statues with haloes flanking the Buddha (obstacles).
4. **The court** — a bronze incense burner (obstacle) + an offering table, prayer-stone cairns,
   and stone lanterns lining the approach.
5. **Atmosphere** — additive gilded glow + incense embers and smoke + braziers + falling petals.
- Note: the centre-back colossus frames best when faced directly; the fixed eye-level autopilot
  camera angles toward the foreground court (the green bodhisattvas confirm the statuary reads).

## Encounter (mobs.cpp)
- **8 "Awakened"** — the `Mob` horde, level-aware spawn set (`buddha_defs`), in the court. Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Awakened*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–108 and the `Boss` are untouched. `mob_level` now covers
  `… || GOMPA || BUDDHA`. **No new global struct/vector** — the shrine is parametric; the Buddha +
  bodhisattva + burner obstacles and the lights are pushed in `build_buddha`; rock/cairns/petals
  are seeded; only `s_wisps` reused. A new `draw_seated_buddha` helper. Lights built in
  `build_buddha` (so `build_crystals` / `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_buddha` lays
  the courtyard.
- CLI: `buddha` / `daibutsu` / `grotto` launches straight into it.
