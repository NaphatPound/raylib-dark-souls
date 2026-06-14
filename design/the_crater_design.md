# The Crater — Level Design (LEVEL_CRATER = 62)

A meteor-impact basin: a scorched crater ringed by a raised rim of upthrust rock, with a
colossal half-buried **fallen star** glowing with eerie violet crack-veins at its heart,
smoking fumarole vents, radial scorch streaks, scattered ejecta boulders, and a haze of
drifting violet embers. The fifty-ninth **non-boss** level — those felled by the star's
fall ("The Stricken") rise to fight. Clear them all to win; the bonfire then ignites.

New procedural geometry, not a recolour — a **crater / fallen-star** silhouette (raised
rim ring + a glowing embedded meteor + fumaroles) found in none of the other sixty-two
levels. A **DRY** level (scorched crater floor). Its eerie cold-violet glow sets it apart
from every warm-orange industrial level (forge, foundry, kiln, collier, tar).

## Theme & palette
- **Mood:** ominous, otherworldly, still-smouldering — a dark basin lit by an alien glow.
- **Light (render.cpp):** faint cold overhead `(0.20,0.78,0.30)`, dim violet-tinged
  moonlight `(0.62,0.58,0.74)`, dark violet ambient, deep dark haze (`~#332E47`, density
  `0.0110`). Reuses `sky_storm.fs`; **DRY** (`draw_crater` lays the scorched floor).
- **Glow:** eerie violet fallen-star `(0.80,0.45,1.00)`.

## Geometry (arena.cpp :: build_crater / draw_crater)
Reuses the lit `s_column` (crater-rim blocks / scorch streaks / floor), `s_dome` (the
meteor sphere via the two-hemisphere trick / ejecta boulders / fumarole mounds), `s_cyl`
(vent throats / blast-scorch disc), and `draw_bone_seg` (jagged meteor shards).
1. **Crater floor** — a dark scorched slab + a darker central blast-scorch disc.
2. **Raised rim** — 28 rock blocks ringing the perimeter, each tilted outward (rlgl frame).
3. **Fallen star** — a half-buried cracked sphere (upper dome + shallow flipped dome) with
   7 jagged `draw_bone_seg` shards; the central landmark obstacle.
4. **Scatter** — 10 radial scorch streaks, 12 ejecta boulders (large ones obstacles), and
   4 smoking fumarole vents.
5. **Atmosphere** — additive glowing crack-veins + a pulsing violet core + fumarole
   smoke/glow + a field of drifting violet embers.

## Encounter (mobs.cpp)
- **8 "Stricken"** — the `Mob` horde, level-aware spawn set (`crater_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Stricken*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–61 and the `Boss` are untouched. `mob_level` now covers
  `… || FALLS || CRATER`. **No new global struct/vector** — the rim/boulders/streaks are
  deterministic seeded loops shared by build (the >1.3-scale ejecta become obstacles, same
  seed 8200) + draw; only `s_wisps` reused. Star/vent lights built in `build_crater` (so
  `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_crater` lays the scorched floor.
- CLI: `crater` / `meteor` / `star` launches straight into it.
