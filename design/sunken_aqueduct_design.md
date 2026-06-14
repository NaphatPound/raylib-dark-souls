# Sunken Aqueduct — Level Design (LEVEL_AQUA = 18)

A vast flooded cistern where a great aqueduct marches across black water. The
fifteenth **non-boss** level: things that crawled from the channel ("The Wretched")
infest the arcades. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **vaulted-arcade** architectural
silhouette (repeating semicircular arches forming a viaduct) found in none of the
other eighteen levels. Reuses the water plane as the dark central channel.

## Theme & palette
- **Mood:** dank, echoing, drowned — dripping stone, still black water, green algae
  glowing on the waterline.
- **Light (render.cpp):** dim cool shaft light, green-grey cistern ambient, dark
  green-grey damp haze (`~#26332E`, density `0.0120`). Reuses `sky_storm.fs` and the
  dark `water_storm.fs` channel.
- **Glow:** green algae point lights `(0.40, 0.85, 0.55)` pooling at the waterline.

## Geometry (arena.cpp :: build_aqua / draw_aqua, helper draw_arch)
Reuses `s_column` (piers / voussoirs / entablature / walls) and `s_cyl` (pipes).
1. **Vaulted arches** — `draw_arch` builds two piers + a semicircular ring of
   voussoir blocks (each rotated to follow the curve) + an entablature on top.
2. **Two arcades** — six arches each flanking the central channel (their piers
   overlap into a continuous arcade you can see through).
3. **Grand central arch** — a larger arch spanning the channel; its two piers are
   collision obstacles (the channel passes through).
4. **Dripping pipes** — horizontal pipes jutting from some piers.
5. **Broken brick walls** — a ruined perimeter wall ring.
6. **Algae glow** — green light pooling on the dark water at each pier.

## Encounter (mobs.cpp)
- **7 "Wretched"** — the `Mob` horde, level-aware spawn set (`aqua_defs`), wading the
  channel and arcades. Same lightweight AI, lock-on, aggregate HUD bar (named *The
  Wretched*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–17 and the `Boss` are untouched. `mob_level` now covers
  `… || CAMP || AQUA`.
- Keeps the reused water plane as the dark flooded channel.
- CLI: `aqueduct` / `viaduct` / `cistern` launches straight into it.
