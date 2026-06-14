# The Ishtar Gate — Level Design (LEVEL_ISHTAR = 105)

The great Babylonian processional gate under a brilliant sun: twin massive crenellated **towers**
of deep-blue glazed brick flank a tall arched **passage**, banded with golden relief bulls and
dragons, approached down a paved **processional way** lined by blue walls of golden striding
lions, with winged **lamassu** guardians at the threshold, bronze braziers and banner poles. The
one hundred second **non-boss** level — the city's exiled dead have risen ("The Exiled"). Clear
them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **Babylonian-gate** silhouette (twin crenellated
towers + an arched passage + lion-walled processional walls + lamassu) found in none of the other
one hundred five levels. A **DRY** level (the paved way). A Mesopotamian monument — distinct from
the free-standing Roman **Triumphal Arch**, the stepped **Desert Ziggurat** and the Star Fort:
this is a deep gateway between two towers, fronted by a long walled approach.

## Theme & palette
- **Mood:** monumental, imperial, sun-blasted — lapis-blue brick and gold against the desert sky.
- **Light (render.cpp):** high desert sun `(0.40,0.78,0.32)`, brilliant warm daylight
  `(1.18,1.08,0.86)`, soft sky fill (lifts the blue), pale dusty haze (`~#D1C7A8`, density
  `0.0058`). Reuses `sky_ice.fs` (bright blue); **DRY** (`draw_ishtar` lays the way;
  `water_storm.fs` placeholder is unused).
- **Glow:** warm sun on glazed brick `(1.00,0.74,0.42)`.

## Geometry (arena.cpp :: draw_gate_tower / draw_lamassu / build_ishtar / draw_ishtar)
Reuses `s_column` (paving + path / tower bodies + panels + reliefs + crenellations / walls + lion
reliefs / header + jambs / lamassu bodies+heads+beards+wings / banners), the lit `s_cyl` (lamassu
legs / braziers / banner poles), and `s_cone` (the arched head of the opening). The blue is the
historical glaze; the *design* is the towers + reliefs + crenellations + processional walls.
1. **The processional way** — a paved brick floor with a central darker path band and paving
   joints.
2. **The processional walls** (obstacles) — blue brick along both sides with a top register line,
   golden striding-lion relief panels, and stepped crenellations.
3. **The gate** — two `draw_gate_tower` towers (blue body + recessed front panel + four golden
   relief-animal bands + stepped crenellations), a banded header lintel across the top, a dark
   arched passage opening with gold door-jambs (the towers are obstacles).
4. **Lamassu** (`draw_lamassu`, obstacles) — winged human-headed bull guardians with square
   beards and crowns, flanking the opening.
5. **Approach dressing** — bronze braziers and banner poles flanking the way; additive brazier
   fire + tower-top glow + dust motes.

## Encounter (mobs.cpp)
- **8 "Exiled"** — the `Mob` horde, level-aware spawn set (`ishtar_defs`), on the way. Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Exiled*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–104 and the `Boss` are untouched. `mob_level` now covers
  `… || FLOAT || ISHTAR`. **No new global struct/vector** — the gate is parametric; the tower +
  wall + lamassu obstacles and the lights are pushed in `build_ishtar`; dust is seeded; only
  `s_wisps` reused. New `draw_gate_tower` / `draw_lamassu` helpers. Lights built in `build_ishtar`
  (so `build_crystals` / `draw_crystals` early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_ishtar` lays
  the paved way.
- CLI: `ishtar` / `gate` / `babylon` launches straight into it.
