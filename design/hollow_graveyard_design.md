# Hollow Graveyard — Level Design (LEVEL_GRAVEYARD = 4)

A flooded, mist-choked necropolis under a pale moon. Unlike the other four
arenas, this level has **no boss** — instead a small **horde of shambling
Hollows** rises among the graves. Clear every Hollow to win; the bonfire then
ignites as in any other level.

This is *new procedural geometry*, not a recolour: a central mausoleum, leaning
tombstones in ragged rows, bare dead trees, a broken perimeter wall, and a
gateway arch — none of which appear in the Blood-Moon Ruin, Frozen Cathedral,
Volcanic Forge, or Sunken Colosseum.

## Theme & palette
- **Mood:** quiet dread. Stagnant shin-deep water, low green mist, a sickly
  moon. The dead simply walk toward you.
- **Light (render.cpp):** high pale moonlight, faint green-grey ambient, thick
  low fog. Fog colour `~#5C6E5A` (graveyard green-grey), density `0.0130`.
- **Wisps:** small green will-o'-the-wisp flames bob over scattered graves and
  pool their glow on the water (point lights, colour `(0.40, 0.95, 0.55)`).
- **Moon:** a full pale moon (`moon_surface.png`) low on the horizon.
- **Sky / water:** reuse the overcast storm dome + dark stagnant water shaders
  (the geometry, lighting, and wisps carry the new identity).

## Geometry (arena.cpp :: build_graveyard / draw_graveyard)
1. **Central mausoleum** — a stone cube (3.4³) with a cornice, a rotated roof
   cap, four corner pilasters and a dark recessed doorway facing the entrance.
   The level's one solid landmark; it is a collision obstacle.
2. **Tombstones** — a deterministic grid (`SetRandomSeed`) of ~50 leaning slabs,
   each with a base plinth and a 1-in-4 chance of a cross bar. The crypt footprint
   and the entrance aisle (player spawn, +Z) are kept clear.
3. **Dead trees** — seven bare tapered trunks around the perimeter, each with
   2–3 angular branches. No leaves.
4. **Broken perimeter wall** — a single low ruined ring (`r ≈ 22`, height
   1.6–2.6) with frequent gaps — deliberately unlike the Colosseum's six tall
   concentric tiers.
5. **Gateway arch** — twin pillars + lintel on the +Z side, framing the way in.

## Encounter (mobs.cpp)
- **6 Hollows**, weaker cousins of the Boss: ~50–70 HP, one attack, no phases,
  low poise. They share `enemy.glb` + the combat/anim systems but each owns its
  own model instance so they animate independently.
- They **sleep** scattered among the graves and wake when the player comes
  within ~15 m, so the horde engages in waves as you push inward.
- Lock-on (Tab) targets the nearest living Hollow. The HUD bar shows the
  **aggregate horde health**, named *The Hollow Horde*.
- **Victory** fires when the last Hollow falls → bonfire → rest to save, exactly
  like the boss levels (reuses `Game::on_boss_died`).

## Integration notes
- Purely additive: `LEVEL_BLOODMOON/FROZEN/FORGE/COLOSSEUM` and the `Boss` are
  untouched. The Boss object is simply never initialised, updated, or drawn on
  this level (`mob_level` guard in main.cpp).
- CLI: `grave` / `graveyard` launches straight into it.
