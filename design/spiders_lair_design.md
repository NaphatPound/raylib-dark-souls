# The Spider's Lair — Level Design (LEVEL_WEB = 56)

A dark webbed cavern: great orb-webs strung between jagged rock spires, pale egg sacs
clustered at the hubs, silk-wrapped cocoons hanging in the gloom, all in a sickly
pale-green murk. The fifty-third **non-boss** level — the lair's silk-bound dead ("The
Ensnared") rise from the webs. Clear them all to win; the bonfire then ignites as
usual.

New procedural geometry, not a recolour — a **webbed cavern** silhouette (strung
orb-webs + rock spires + cocoons) found in none of the other fifty-six levels. A
creepy fit for the Dark Souls vibe; distinct from every prior level.

## Theme & palette
- **Mood:** dank, suffocating, watchful — webs catching the dim light, sacs glistening
  and cocoons swaying.
- **Light (render.cpp):** dim cave light, sickly pale-green light `(0.62, 0.70, 0.62)`,
  dim dank ambient, dark cavern murk (`~#29332B`, density `0.0125`). Reuses
  `sky_storm.fs`; **dry** — its own cave floor is the floor.
- **Glow:** sickly pale silk `(0.70, 0.85, 0.70)`.

## Geometry (arena.cpp :: build_web / draw_web_lair / draw_web / draw_cocoon / web_layout)
Reuses `s_cone` (rock spires), `s_dome` (egg sacs / rubble / cocoon caps), `s_cyl`
(cocoon bodies / wrap bands), `s_column` (floor), and `draw_bone_seg` (every silk
strand / threads).
1. **Orb-webs** — `draw_web`: a vertical-plane web of 12 radial strands + 4 concentric
   rings (lit silk cylinders), yawed to face any direction. Webs are strung between
   consecutive **rock spires** and from the centre out, plus a **central great web**.
2. **Rock spires** — 8 seeded jagged `s_cone` spires (the web anchors) + a central hub
   spire; collision obstacles.
3. **Egg sacs** — pale `s_dome` clusters at several web hubs.
4. **Cocoons** — `draw_cocoon`: silk-wrapped bundles on threads, hung around the
   cavern.
5. **Atmosphere** — additive pale silk sheen at the central web + drifting motes.

## Encounter (mobs.cpp)
- **8 "Ensnared"** — the `Mob` horde, level-aware spawn set (`web_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Ensnared*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–55 and the `Boss` are untouched. `mob_level` now covers
  `… || AVIARY || WEB`. **No new global struct/vector** — `web_layout` fills a local
  `std::vector<Vector3>` of spire bases shared by build+draw; webs/cocoons are derived
  from it; only `s_wisps` reused. Silk lights built in `build_web` (so `build_crystals`/
  `draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_web_lair` lays the cave floor.
- CLI: `web` / `lair` / `spider` launches straight into it.
