# The Sphinx — Level Design (LEVEL_SPHINX = 70)

A Giza necropolis at dawn: a colossal reclining **sphinx** — a lion's body with forelegs
and paws, a rising chest, and a pharaoh's head in the nemes headdress — gazing out over a
sand plain, flanked by smooth great **pyramids**, a processional avenue of ram-sphinx
plinths and gilt-capped obelisks. The sixty-seventh **non-boss** level — the necropolis'
risen dead ("The Embalmed") rise from the sand. Clear them all to win; the bonfire ignites.

New procedural geometry, not a recolour — a **sphinx + pyramids** silhouette (a reclining
beast-monument with a headdressed head + smooth pyramids) found in none of the other
seventy levels. A **DRY** level (sand plain). Deliberately distinct from the Desert
Ziggurat (a *stepped* pyramid + obelisks + a toppled colossus) and the Fallen Titan (a
*shattered* humanoid statue in pieces): here an intact, animal-bodied colossus and smooth
pyramids, under a cool dawn sky rather than orange dusk.

## Theme & palette
- **Mood:** ancient, golden, watchful — first light raking across the sandstone.
- **Light (render.cpp):** low raking dawn sun `(0.52,0.50,0.30)`, warm gold light
  `(1.14,0.92,0.62)`, cool dawn fill, pale warm desert haze (`~#B8B8A8`, density `0.0072`).
  Reuses `sky_ice.fs` (a pale dawn sky — the cool-sky/warm-sun contrast sets it apart from
  the orange-dusk desert); **DRY** (`draw_sphinx_necropolis` lays the sand).
- **Glow:** warm dawn-gold sandstone `(1.00,0.82,0.46)`.

## Geometry (arena.cpp :: draw_sphinx / build_sphinx / draw_sphinx_necropolis)
Reuses the lit `s_column` (every sphinx block / plinths / ruined blocks / sand), `s_cone`
(smooth pyramids / obelisks / gilt caps), `s_dome` (dunes / ram-sphinx heads).
1. **The sphinx** (`draw_sphinx`) — a reclining lion body + haunches + two forelegs with
   paws + a rising chest + neck + a pharaoh head wearing the nemes headcloth (flaring block
   + side lappets), with brow, dark eye insets, nose and a false beard; faces the player
   (+z). The central landmark obstacle (plus paw/haunch obstacles in `build_sphinx`).
2. **Pyramids** — three smooth `s_cone` pyramids behind, the largest with a faint outline.
3. **Avenue** — two rows of ram-sphinx plinths leading up to gilt-capped obelisks flanking
   the approach.
4. **Plain** — a sand slab, 20 ripple dunes, scattered ruined blocks.
5. **Atmosphere** — additive warm dawn glow + drifting sand + heat shimmer.

## Encounter (mobs.cpp)
- **8 "Embalmed"** — the `Mob` horde, level-aware spawn set (`sphinx_defs`), spawning in the
  open forecourt around and beside the sphinx. Same lightweight AI, lock-on, aggregate HUD
  bar (named *The Embalmed*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–69 and the `Boss` are untouched. `mob_level` now covers
  `… || SUNFLOWER || SPHINX`. **No new global struct/vector** — the sphinx/pyramids/avenue
  are deterministic; dunes/ruins are seeded loops; only `s_wisps` reused. Dawn lights built
  in `build_sphinx` (so `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_sphinx_necropolis` lays the sand.
- CLI: `sphinx` / `pyramids` / `giza` launches straight into it.
