# Siege Camp — Level Design (LEVEL_CAMP = 17)

An abandoned besieging army's encampment on a churned muddy field. The fourteenth
**non-boss** level: a broken army's stragglers ("The Routed") still hold the camp.
Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **military-encampment** silhouette
(rows of conical tents, banners, a sharpened-log palisade) found in none of the
other seventeen levels. The fifth **dry** level (a mud field, no water).

## Theme & palette
- **Mood:** grim aftermath — flapping banners, guttering campfires, a routed host's
  leavings under a cold overcast dusk.
- **Light (render.cpp):** flat pale overcast key, neutral grey ambient, grey
  battlefield haze (`~#666668`, density `0.0100`). Reuses `sky_storm.fs`; the floor
  is a solid mud slab (dry).
- **Glow:** warm campfire point lights `(0.98, 0.60, 0.25)`.

## Geometry (arena.cpp :: build_camp / draw_camp, helper draw_tent)
Reuses `s_cone` (tent canvas / palisade tips), `s_cyl` (poles / logs / spears),
`s_column` (flags / entrances / gate / racks), `s_dome` (mud mounds / fire stones).
1. **Tents** — `draw_tent` (conical s_cone canvas + center pole + dark entrance).
   ~11 in faded canvas colours, scattered; some collapsed (tilted).
2. **Central command pavilion** — a large tent on a raised platform, flanked by
   banners. The landmark and a collision obstacle.
3. **Banner poles** — tall poles with swaying heraldic flags (red/blue/gold).
4. **Palisade** — a ring of sharpened logs (cylinder + cone tip) with a gate (twin
   posts + lintel) toward the +Z entrance.
5. **Weapon racks** — A-frames with rows of leaning spears.
6. **Campfires** — stone rings + crossed logs + flickering flames (the warm lights).

## Encounter (mobs.cpp)
- **7 "Routed"** — the `Mob` horde, level-aware spawn set (`camp_defs`), among the
  tents, with a tougher captain deep in. Same lightweight AI, lock-on, aggregate
  HUD bar (named *The Routed*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–16 and the `Boss` are untouched. `mob_level` now covers
  `… || LIB || CAMP`.
- CLI: `camp` / `siege` / `warcamp` launches straight into it.
