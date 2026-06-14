# The Canals — Level Design (LEVEL_CANAL = 76)

A Venetian waterway: tall pastel canal houses line the stone banks, humped stone bridges
arc over the green canal, red-and-white striped mooring poles stand in the water, black
gondolas drift moored, and a brick campanile rises behind. The seventy-third **non-boss**
level — the canal's drowned dead ("The Adrift") rise from the water. Clear them all to win;
the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **canal-city** silhouette (house-lined banks +
humped bridges + gondolas + a campanile over a central waterway) found in none of the other
seventy-six levels. A **WET** level (the reused green `water_storm` plane is the canal).
Deliberately distinct from the Stilt Village (fishing huts on stilts over a lagoon), the
Sunken Aqueduct (a vaulted cistern channel), and the Watermill (a wheel + race).

## Theme & palette
- **Mood:** warm, faded-grand, melancholy — golden afternoon on still green water.
- **Light (render.cpp):** warm afternoon sun `(0.40,0.74,0.32)`, warm golden daylight
  `(1.08,0.96,0.76)`, soft warm fill, warm haze (`~#B3B8A8`, density `0.0078`). Reuses
  `sky_ice.fs`; **WET** — the reused green `water_storm` plane is the canal.
- **Glow:** warm Venetian lamp / sun `(1.00,0.84,0.54)`.

## Geometry (arena.cpp :: draw_arched_bridge / draw_gondola / build_canal / draw_canal)
Reuses `draw_cottage` (tall canal houses), `s_column` (banks / bridge decks + railings +
piers / house windows / campanile / gondola hulls), `s_cyl` (striped mooring poles), and
`s_cone` (the campanile cap).
1. **Banks & houses** — raised stone fondamenta walkways down each side (the channel between
   is the water plane), with 5 tall pastel canal houses per bank (`draw_cottage`, five
   colours) and dark arched-window insets facing the water. The banks are solid walls — the
   obstacles in `build_canal` keep play to the water channel.
2. **Bridges** (`draw_arched_bridge`) — three humped stone bridges (a parabolic stepped deck
   + railings on two piers) arcing high enough to walk under.
3. **Campanile** — a tall brick bell-tower (stacked courses + a belfry + a pyramidal cap) at
   the back (the landmark backdrop; its base is an obstacle).
4. **Water details** — striped mooring poles, a central mooring cluster (the landmark
   obstacle), and three moored gondolas (`draw_gondola`: a slim black hull + a tall prow with
   a brass *ferro* comb).
5. **Atmosphere** — additive warm lamp glow + canal water sparkle + wheeling pigeons.

## Encounter (mobs.cpp)
- **8 "Adrift"** — the `Mob` horde, level-aware spawn set (`canal_defs`, kept within the
  channel), on the water. Same lightweight AI, lock-on, aggregate HUD bar (named *The
  Adrift*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–75 and the `Boss` are untouched. `mob_level` now covers
  `… || BALLOON || CANAL`. **No new global struct/vector** — banks/houses/bridges/campanile
  are deterministic; bank-wall obstacles are sampled along the channel in `build_canal`; only
  `s_wisps` reused. Lamp lights built in `build_canal` (so `build_crystals`/`draw_crystals`
  early-return).
- WET level: `draw_water` still draws the canal plane (NOT in the dry early-return);
  `draw_canal` lays the banks/houses/bridges/gondolas over it.
- CLI: `canal` / `canals` / `venice` launches straight into it.
