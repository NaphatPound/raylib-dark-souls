# The Amphitheatre — Level Design (LEVEL_THEATRE = 72)

A sunlit Greek theatre: a tiered semicircular stone **cavea** (seating bowl) wraps a flat
**orchestra** fight-floor with a central altar, all facing a tall ornate **scaenae frons**
(stage building) of two storeys of marble columns, niche statues, an entablature, a
pediment and a central grand doorway, with braziers burning before it. The sixty-ninth
**non-boss** level — the theatre's risen players ("The Chorus") rise in the orchestra.
Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **classical-theatre** silhouette (tiered
horseshoe seating + a multi-storey columned stage facade) found in none of the other
seventy-two levels. A **DRY** level (the orchestra floor). Deliberately distinct from the
Sunken Colosseum (an *enclosed oval* amphitheatre arena): the theatre is an *open
horseshoe* whose dominant feature is the tall ornate stage building (scaenae frons) — a
thing the colosseum entirely lacks.

## Theme & palette
- **Mood:** golden, grand, classical — late-afternoon sun on weathered marble.
- **Light (render.cpp):** warm afternoon sun `(0.42,0.72,0.34)`, warm marble daylight
  `(1.10,1.00,0.80)`, soft warm fill, warm golden haze (`~#C2BCA3`, density `0.0066`).
  Reuses `sky_ice.fs` (bright blue sky on the marble); **DRY** (`draw_theatre` lays the
  orchestra floor).
- **Glow:** warm marble sun / sconce `(1.00,0.86,0.56)` (braziers burn orange).

## Geometry (arena.cpp :: build_theatre / draw_theatre)
Reuses the lit `s_column` (seating tiers / floor / stage blocks / entablatures / statues /
doorway), `s_cyl` (the altar / all columns / braziers / a fallen drum), `s_dome` (statue
heads), `s_cone` (the pediment).
1. **Cavea** — 6 concentric arc tiers of stepped marble seating (each a solid riser with a
   darker seat-edge line) wrapping ~260° around the orchestra, with a **front aisle gap**
   at θ≈0 for the player's descent.
2. **Orchestra** — the open floor with a central **thymele altar** (the landmark obstacle).
3. **Scaenae frons** — the stage building: a raised proscenium, a back wall, seven
   ground-floor columns (base + capital) under an entablature, five upper-storey columns +
   entablature + a pediment, niche statues, and a central dark grand doorway.
4. **Props** — two braziers and a fallen column drum in the orchestra.
5. **Atmosphere** — additive warm sun glow + flickering brazier fire + drifting dust motes.

## Encounter (mobs.cpp)
- **8 "Chorus"** — the `Mob` horde, level-aware spawn set (`theatre_defs`), in the
  orchestra. Same lightweight AI, lock-on, aggregate HUD bar (named *The Chorus*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–71 and the `Boss` are untouched. `mob_level` now covers
  `… || DAM || THEATRE`. **No new global struct/vector** — the seating is a deterministic
  arc loop shared by build (inner-wall obstacles + a front aisle gap) + draw; the scaenae
  frons is fixed; only `s_wisps` reused. Sun/brazier lights built in `build_theatre` (so
  `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_theatre` lays the orchestra floor.
- CLI: `theatre` / `amphitheatre` / `stage` launches straight into it.
