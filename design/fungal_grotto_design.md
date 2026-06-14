# Fungal Grotto — Level Design (LEVEL_FUNGAL = 5)

A bioluminescent underground cavern of giant glowing mushrooms. The second
**non-boss** level: a pack of **Sporeborn** (Hollow-type enemies) shambles between
the stalks. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — organic round mushroom shapes (lit
cylinders + hemispheres) found in none of the other five levels (which use rocks,
colonnades, concentric rings, or tombstones).

## Theme & palette
- **Mood:** a damp, enclosed cave lit only by the fungi themselves. No moon, no sky
  to speak of — the glow comes from the caps and floating spores.
- **Light (render.cpp):** dim diffuse cave light, teal-cyan ambient, deep teal fog
  (`~#173B3B`, density `0.0150`). Darker than every other level so the
  bioluminescence pops.
- **Glow colour:** bioluminescent teal `(0.30, 0.92, 0.85)` for the point lights
  (caps + ground spore pods) that pool on the water and light the fighters.
- **Sky / water:** reuse the dark overcast dome + still water shaders; the teal
  fog and mushroom glow carry the identity.

## Geometry (arena.cpp :: build_fungal / draw_fungal)
Uses two new lit primitives loaded in `load()`: `s_cyl` (GenMeshCylinder) and
`s_dome` (GenMeshHemiSphere) — generic, reusable for future levels.
1. **Colossal central mushroom** — stalk H≈9.5, cap R≈6. The landmark and a
   collision obstacle.
2. **Giant + medium mushrooms** — a deterministic scatter (`SetRandomSeed`) of
   ~9, each a leaning cylinder stalk + a hemisphere cap (teal or violet), with a
   thin darker gill disk under the cap and an additive under-cap glow halo. The
   entrance aisle (+Z, player spawn) is kept clear.
3. **Ground spore pods** — low floor-level glow points.
4. **Drifting spore motes** — small additive teal specks bobbing around every
   glow point.

## Encounter (mobs.cpp)
- **7 Sporeborn** — the `Mob` horde, level-aware spawn set (`fungal_defs`):
  denser and slightly hardier than the graveyard pack (52–78 HP), with a tougher
  *grotto warden* deep in. Same lightweight AI, lock-on, aggregate HUD bar
  (named *The Sporeborn*), and victory-on-clear flow.

## Integration notes
- Purely additive: levels 0–4 and the `Boss` are untouched. `mob_level` is now
  `LEVEL_GRAVEYARD || LEVEL_FUNGAL`; the Boss object stays inert here.
- CLI: `fungal` / `grotto` launches straight into it.
