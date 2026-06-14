# The Triumphal Arch — Level Design (LEVEL_TRIUMPH = 81)

A colossal Roman triumphal arch: twin piers faced with engaged columns frame a voussoired
central passage, an inscribed gilt attic and relief panels rise above, and a bronze
**quadriga** (a four-horse chariot with its charioteer) crowns the monument — facing a paved
plaza walked by a processional way of eagle-capped columns, braziers and a captured-armour
trophy (tropaeum). The seventy-eighth **non-boss** level — the conquered dead ("The
Vanquished") rise in the plaza. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a single **triumphal-arch monument** silhouette
(a great twin-pier arch + quadriga over a triumphal way) found in none of the other
eighty-one levels. A **DRY** level (paved plaza). Deliberately distinct from the Grand Plaza
(a fountain + a ring of arcade arches), the Sunken Aqueduct (a row of vault arches), the
Cathedral Nave (Gothic interior), and the Chasm Bridge (a span): here one ornate, gilded,
free-standing monument dominates.

## Theme & palette
- **Mood:** imperial, gilded, victorious — warm late-afternoon sun on marble and bronze.
- **Light (render.cpp):** warm late sun `(0.40,0.74,0.34)`, warm imperial gold
  `(1.10,0.98,0.74)`, soft warm fill, warm golden haze (`~#B8B399`, density `0.0072`). Reuses
  `sky_ice.fs`; **DRY** (`draw_triumph` lays the plaza).
- **Glow:** warm imperial gold / brazier `(1.00,0.84,0.50)`.

## Geometry (arena.cpp :: build_triumph / draw_triumph)
Reuses the lit `s_column` (piers / passage / voussoirs / entablature / attic / panels /
chariot / charioteer / tropaeum / plaza), `s_cyl` (engaged + avenue columns / trophy post +
crossbar / braziers), `s_dome` (eagle caps / helmet / charioteer head), and `s_cone` (the
trophy crest).
1. **The arch** — twin piers with engaged columns, a dark central passage, a **voussoir ring**
   (12 blocks rotated about Z to follow the semicircle), an entablature + gilt cornice, an
   attic with a gilt **inscription panel**, and relief panels on the piers (the piers are
   obstacles).
2. **The quadriga** — a bronze chariot car + four horses (body + head) + a charioteer on top.
3. **The triumphal way** — a lighter paved avenue flanked by four eagle-capped columns
   (obstacles) and two braziers.
4. **The tropaeum** — a central trophy: a post + crossbar hung with a bronze cuirass, helmet
   and crest (the landmark obstacle).
5. **Atmosphere** — additive warm imperial glow + brazier flames + gilt sparkle + dust.

## Encounter (mobs.cpp)
- **8 "Vanquished"** — the `Mob` horde, level-aware spawn set (`triumph_defs`), in the plaza.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Vanquished*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–80 and the `Boss` are untouched. `mob_level` now covers
  `… || FORT || TRIUMPH`. **No new global struct/vector** — the arch / quadriga / avenue /
  tropaeum are all deterministic; pier + column obstacles fixed in `build_triumph`; only
  `s_wisps` reused. Imperial lights built in `build_triumph` (so `build_crystals`/
  `draw_crystals` early-return). Enum is `LEVEL_TRIUMPH` (not `ARCH`) to avoid confusion with
  the existing `draw_arch` helper and `LEVEL_ARCHTREE`.
- DRY level: `draw_water` early-returns; `draw_triumph` lays the plaza.
- CLI: `triumph` / `arch` / `quadriga` launches straight into it.
