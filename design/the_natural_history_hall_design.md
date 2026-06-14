# The Natural History Hall — Level Design (LEVEL_MUSEUM = 107)

A grand museum in soft, dusty light: a mounted, articulated **T-rex skeleton** rears on its
plinth at the centre of a marble rotunda beneath a skylight beam, ringed by tall glass **display
cases** of specimens, a marble **colonnade** carrying a gallery balcony, taxidermy mounts (a
standing bear, an antlered deer), a great brass-mounted **globe**, and ammonite fossil panels.
The one hundred fourth **non-boss** level — the collection's dead have risen ("The Exhibited").
Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **natural-history-museum** silhouette (a mounted
articulated dinosaur skeleton + glass cases + a colonnaded rotunda) found in none of the other
one hundred seven levels. A **DRY** level (the gallery floor). A museum interior — distinct from
the **Dragon Boneyard** (scattered, half-buried dragon skeletons in an *outdoor graveyard*): here
the skeleton is *one articulated specimen mounted on an armature* indoors among cases and exhibits.

## Theme & palette
- **Mood:** hushed, reverent, dusty — sunbeams and old bones under a high glass roof.
- **Light (render.cpp):** soft skylight `(0.34,0.80,0.40)`, cool neutral daylight
  `(1.06,1.02,0.92)`, even soft fill, dim dusty haze (`~#A8A8A8`, density `0.0085`). Reuses
  `sky_ice.fs`; **DRY** (`draw_museum` lays the floor; `water_storm.fs` placeholder is unused).
- **Glow:** soft warm gallery light `(1.00,0.82,0.56)`.

## Geometry (arena.cpp :: draw_trex / draw_case / build_museum / draw_museum)
Reuses `s_column` (floor + cabinets / skull+jaw / taxidermy / fossil panels), the lit `s_cyl`
(globe stand / bear arms / specimen jars), `s_dome` (skull joints / specimens / globe halves /
ammonites / bear head), `s_cone` (T-rex teeth), `s_torus` (the gallery balcony rail / globe
meridian), the **reused `draw_petra_column`** (the colonnade), and `draw_bone_seg` (every
skeleton bone via `Vector3Lerp` for the ribs + the armature supports + antlers + floor inlay).
1. **The T-rex** (`draw_trex`, the central obstacle) — a plinth, two clawed legs, a back, a
   raised neck and toothed skull with a hinged jaw, a long counter-balancing tail, ribs hung off
   the spine, tiny arms, and steel armature supports. Aged-tan bone so it reads against the marble.
2. **Display cases** (`draw_case`) — eight glass cabinets on wood bases with coloured specimens
   ring the hall (obstacles).
3. **The architecture** — a marble rotunda with radial inlay, twelve `draw_petra_column` columns,
   and an `s_torus` gallery balcony rail (the colonnade confines the hall).
4. **Exhibits** — a standing taxidermy bear, an antlered deer, a brass-mounted globe, and
   ammonite fossil panels on the piers.
5. **Atmosphere** — a `DrawCylinderEx` skylight beam onto the skeleton, soft case glow, and dust
   motes rising in the light.
- Note: the centre-mounted skeleton frames best as the player approaches it; the fixed eye-level
  autopilot camera, standing at the rotunda's edge, does not look up at it.

## Encounter (mobs.cpp)
- **8 "Exhibited"** — the `Mob` horde, level-aware spawn set (`museum_defs`), among the displays.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Exhibited*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–106 and the `Boss` are untouched. `mob_level` now covers
  `… || TANNERY || MUSEUM`. **No new global struct/vector** — the hall is parametric; the
  skeleton-plinth + case + column obstacles and the lights are pushed in `build_museum`; motes are
  seeded; only `s_wisps` reused. New `draw_trex` / `draw_case` helpers + the reused
  `draw_petra_column`. Lights built in `build_museum` (so `build_crystals` / `draw_crystals`
  early-return).
- DRY level: it IS in both the `draw_water` and `draw_crystals` early-returns; `draw_museum` lays
  the gallery floor.
- CLI: `museum` / `fossil` / `natural` launches straight into it.
