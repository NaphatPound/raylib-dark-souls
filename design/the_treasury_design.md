# The Treasury (Petra) — Level Design (LEVEL_PETRA = 93)

The rose-red rock-cut facade of **Al-Khazneh** at the end of a narrow canyon: two stories of
carved columns rise against a towering sandstone cliff — a lower colonnade with a pediment and
dark tomb doorways, an upper **broken pediment** split around a central round **tholos** crowned
by the famous urn — while the tall walls of the **Siq** lead the eye in and rubble litters the
sandy forecourt. The ninetieth **non-boss** level — the tomb's shades have risen ("The
Entombed"). Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **rock-cut facade** silhouette (an ornate temple
front *carved into a cliff* at the head of a canyon) found in none of the other ninety-three
levels. A **DRY** level (the sandy forecourt). Distinct from every **free-standing** classical
level — the Gothic **Cathedral Nave**, the **Sentinel Court**, the Egyptian **Hypostyle Hall**,
the Greek **Amphitheatre** and the Roman **Triumphal Arch**: this facade is *carved into living
rock*, framed by canyon walls, with the broken-pediment tholos found nowhere else.

## Theme & palette
- **Mood:** awed, sun-baked, ancient — a lost temple glowing in the canyon light.
- **Light (render.cpp):** low sun `(0.45,0.62,0.40)` raking the facade, warm golden light
  `(1.18,0.96,0.74)` on red stone, deep warm canyon shadow, rose-red dust haze (`~#BD8F70`,
  density `0.0072`). Reuses `sky_ice.fs` (a bright sliver of sky above the canyon); **DRY**
  (`draw_petra` lays the sand; `water_storm.fs` placeholder is unused).
- **Glow:** warm torch on rose rock `(1.00,0.66,0.40)`.

## Geometry (arena.cpp :: draw_petra_column / build_petra / draw_petra)
Reuses `s_column` (forecourt / cliff + rugged rock blocks / facade backing + entablatures +
pediment rakes + doorways / canyon walls / rubble + obelisk / column bases+capitals), the lit
`s_cyl` (column shafts / tholos platform / urn neck / fallen column), `s_cone` (tholos conical
roof / obelisk cap), and `s_dome` (urn body + finial).
1. **The back cliff** — a massive rose-red rock mass with rugged stacked blocks, the facade
   carved into its front (an obstacle).
2. **Lower story** — six columns (`draw_petra_column`: base + shaft + capital) + an entablature
   + a low pediment + a central tomb doorway flanked by two side doorways (columns are obstacles).
3. **Upper story** — two flanking column-pavilions + an upper entablature + a **broken (split)
   pediment** around a central **tholos** (a ring of six columns + a conical roof + the urn).
4. **The Siq** — tall rough rose-red canyon walls down both sides (obstacles) leading to the
   facade.
5. **Forecourt** — a toppled column, scattered blocks, a standing obelisk; additive torch glow
   in the doorways + dusty light + motes.

## Encounter (mobs.cpp)
- **8 "Entombed"** — the `Mob` horde, level-aware spawn set (`petra_defs`), in the forecourt.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Entombed*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–92 and the `Boss` are untouched. `mob_level` now covers
  `… || BALLCOURT || PETRA`. **No new global struct/vector** — the facade is parametric; the
  facade-column + cliff + canyon-wall obstacles and the lights are pushed in `build_petra`;
  rubble + rock variation + motes are seeded; only `s_wisps` reused. A new `draw_petra_column`
  helper draws each column. Lights built in `build_petra` (so `build_crystals` / `draw_crystals`
  early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_petra` lays
  the sandy forecourt.
- CLI: `petra` / `treasury` / `khazneh` launches straight into it.
