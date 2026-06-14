# The Glassworks — Level 114 (LEVEL_GLASSWORKS = 113)

A glassblower's hot shop at night, lit not by the moon but by furnace fire. The
arena is the working floor of a pre-industrial glasshouse: a great brick melting
furnace breathes a glowing arched mouth at the back while smaller reheating
furnaces, blowing benches and racks of finished, colourful blown vessels ring the
soot-darkened apron. A horde of "The Vitrified" — hollows fused half to molten
glass — work the shop. **No boss.**

## Why it is NOT a recolour / duplicate
This is genuinely new procedural geometry, not an existing level with a new
palette. Its silhouette is **industrial brick furnaces with glowing apertures + a
tapering chimney + tall shelved racks of glass vessels** — a profile no prior
level owns:
- Distinct from **The Kiln / Collier / Foundry** (those are pottery/charcoal/iron
  furnaces): here the heat product is *glass* — cylindrical glory-holes, blowpipe
  benches with molten gathers, and racks of translucent coloured vessels are the
  defining props, not crucibles or charcoal clamps.
- Distinct from **The Topiary Garden** (its DRY-floor neighbour): hard brick mass
  + fire glow vs. clipped-hedge sculpture.
- Distinct from **The Alchemy** lab: a working hot-shop with a roomful of furnaces
  and roof trusses, not a scholar's bench of retorts and phials.

## Layout (DRY level — its own solid brick floor, no water plane)
- **The great melting furnace** (back centre, `z = -13`): an 8-wide brick mass
  with course-banding, a recessed glowing arched **melting mouth** (dark bore +
  bright molten interior + arch ring), capped by a tapering brick **chimney** that
  rises ~10 units with a lit lip. The dominant landmark.
- **Two glory-hole reheating furnaces** flanking the apron (`±8.5, z = -4`): squat
  brick drums on iron legs with a glowing round port bored through, a domed crown
  and a short flue. Sparks rise off both.
- **Two glassblowers' benches** (`±4.5, z = 3`): timber rail + steel arm-rails, a
  blowpipe lying across carrying a glowing molten **gather** on its end.
- **A steel marver table** between the benches for rolling the gather.
- **Two tall vessel racks** (`±12, z = 6`): four-shelf timber racks of blown
  bottles, vases and goblets in blue / green / red / amber / violet / clear glass.
- **An annealing oven** (back corner `9, -11`): brick box with a small glowing door.
- **Sand barrels (hooped), cullet crates, and a quenching water trough** scattered
  on the apron; **exposed roof trusses** overhead enclose the shop.

## Mood / theming
- DRY floor (dim brick + soot apron), `sky_storm` night, no gem crystals.
- Point-light colour `pcol_by[113] = { 1.00, 0.50, 0.22 }` — hot furnace orange.
- Lights at the furnace mouth, chimney top, both glory-holes and both molten
  gathers. Additive pass: a broad pulsing furnace glow, ~40 rising sparks/embers
  off the furnace and glory-holes, and heat shimmer in front of the melting mouth.

## Enemies
`glassworks_defs[]` — 8 "The Vitrified", spread `z = 3 … -9` across the apron and
between the furnaces. Regular horde, routed through `mob_level()` / the main.cpp
OR-chain. No boss; `boss.cpp` / `boss.h` untouched.

## CLI
`dark_souls_raylib.exe glassworks` (aliases `glassblower`, `hotshop`).
HUD banner: **The Vitrified**.

## Additive-only guarantee
Purely additive: a new `LEVEL_GLASSWORKS` enum, a new draw/build pair, and one
appended entry in each themed array + save table. No existing level, the boss, or
any shared geometry was modified. `git diff --stat -- src/boss.cpp src/boss.h`
is empty; build is `BUILD_OK`.
