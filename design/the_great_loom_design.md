# The Great Loom — Level Design (LEVEL_LOOM = 83)

A weaver's hall dominated by a colossal upright loom: a heavy timber frame strung with a
dense curtain of warp threads, a banded tapestry growing on the cloth beam, a shuttle on the
race — flanked by slowly-turning spinning wheels, baskets of coloured yarn, hanging skeins on
a peg rack, and a central yarn-winding swift. The eightieth **non-boss** level — the hall's
risen dead ("The Unraveled") rise among the looms. Clear them all to win; the bonfire ignites.

New procedural geometry, not a recolour — a **loom / weaving-hall** silhouette (a giant warp
frame + woven tapestry + spinning wheels) found in none of the other eighty-three levels. A
**DRY** level (plank workshop floor). The first textile-craft level — distinct from every
other interior and craft scene (Sawmill blade, Kiln ovens, Foundry hammer, Brewery still,
Organ pipes, Hypostyle columns).

## Theme & palette
- **Mood:** warm, intricate, hushed — lamplight on coloured thread and timber.
- **Light (render.cpp):** soft workshop light `(0.30,0.80,0.36)`, warm lamp glow
  `(0.96,0.86,0.68)`, dim warm fill, dark warm interior haze (`~#574B47`, density `0.0098`).
  Reuses `sky_storm.fs`; **DRY** (`draw_loom` lays the plank floor).
- **Glow:** warm workshop lamp `(1.00,0.82,0.55)`.

## Geometry (arena.cpp :: build_loom / draw_loom)
Reuses the lit `s_column` (uprights / tapestry bands / shuttle / floor / spinning-wheel
bases), `draw_bone_seg` (warp + cloth beams / the 44 warp threads / shed bar / hanging
skeins), `s_torus` (spinning-wheel rims / swift hank), `s_cyl` (spindles / spokes / baskets /
swift post), and `s_dome` (balls of yarn).
1. **The loom** — two massive uprights + a warp beam + a cloth beam, a **curtain of 44 warp
   threads** strung between them, a shed/heddle bar across the middle, a **five-band woven
   tapestry** on the cloth beam, and a shuttle (the uprights are obstacles).
2. **Spinning wheels** — two animated wheels (`s_torus` rim + six spokes in an rlgl frame)
   that **spin via `s_time`** on stands (obstacles).
3. **Yarn** — five baskets of coloured yarn balls and a peg rack of hanging skeins.
4. **The swift** — a central yarn-winding swift (a post + a spinning `s_torus`/cross-arm hank)
   — the landmark obstacle.
5. **Atmosphere** — additive warm lamp glow + drifting lint motes.

## Encounter (mobs.cpp)
- **8 "Unraveled"** — the `Mob` horde, level-aware spawn set (`loom_defs`), in the hall. Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Unraveled*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–82 and the `Boss` are untouched. `mob_level` now covers
  `… || ORCHARD || LOOM`. **No new global struct/vector** — the loom / wheels / swift / yarn
  are all deterministic; obstacles fixed in `build_loom`; only `s_wisps` reused. Lamp lights
  built in `build_loom` (so `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_loom` lays the plank floor.
- CLI: `loom` / `weaver` / `tapestry` launches straight into it.
