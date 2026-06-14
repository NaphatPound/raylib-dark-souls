# The Cathedral Close — Level 116 (LEVEL_GOTHIC = 115)

The moonlit **west front of a Gothic cathedral**, seen from the churchyard close.
Two spired bell-towers flank a gabled nave-front crowned by a great glowing **rose
window** over a recessed **triple portal**; rows of **flying buttresses** with
pinnacles brace the nave flanks. A churchyard of leaning cross-headstones and table
tombs spreads across the close. A horde of "The Excommunicate" — shades cast out
onto the consecrated ground — stalk the yard. **No boss.**

## Why it is NOT a recolour / duplicate
Genuinely new procedural geometry. Its silhouette is a Gothic **EXTERIOR** — twin
spired towers + a rose window + flying buttresses — which nothing else owns, and is
geometrically unlike the three other sacred buildings:
- **Cathedral Nave** ("The Penitent") — an *interior* of columns and vaulting.
- **Frozen Cathedral** (boss arena) — a cold enclosed arena, not a façade.
- **Saint Basil's** ("The Devout") — clustered *onion domes*.
- **The Great Mosque** ("The Prostrate") — a single *dome + minaret*.
A pointed-arch façade with twin square towers, a traceried rose, and external
flyers is a completely different read from any dome or interior.

## Layout (DRY level — its own flagstone close, no water plane)
- **Two bell-towers** (`x = ±7.5`, back): 15-high stone shafts with string-courses,
  blind lancets and a dark belfry opening, capped by four corner **pinnacles** and a
  tall octagonal **spire** (to ~y 22) with a glowing finial; a **gargoyle** waterspout
  juts from each front corner.
- **Central nave-front**: an 11-high wall with a triangular **gable** and a slender
  **flèche** spire; flanked by low aisle roofs.
- **The rose window** (`y ≈ 8.2`): a glowing stained-glass disc behind a stone outer
  ring + inner ring + twelve radial **tracery spokes**.
- **Triple portal** at the base: three recessed pointed-arch doorways (centre tallest),
  each with three **archivolt orders**, a glowing interior, and flanking **jamb statues**;
  stone steps lead up to the central door.
- **Flying buttresses**: three per flank — outer piers (stepped, pinnacle-capped) with
  an open arched **flyer** (two `draw_bone_seg` edges) springing to the clerestory
  wall, which carries glowing lancets between the buttresses.
- **The close**: a central paved path, ~14 leaning **cross-headstones**, two **table
  tombs**, and a tall **churchyard cross** (ringed, Celtic-style).

## Mood / theming
- Cold **moonlit night**: pale-blue `lightCol {0.60,0.66,0.86}`, deep-blue ambient,
  cold blue night haze. `sky_ice` starlit backdrop; normal moon. DRY floor; no gem crystals.
- Point-light colour `pcol_by[115] = { 1.00, 0.74, 0.42 }` — warm **stained-glass
  candlelight** spilling from the rose, portals and lancets against the cold night.
- Additive pass: rose-window glow + twelve coloured **petals** (red/blue/gold/violet),
  portal/lancet glows, low churchyard **mist**, and **bats** wheeling round the spires.

## Enemies
`gothic_defs[]` — 8 "The Excommunicate", spread across the close in front of the
façade (path kept clear). Regular horde via `mob_level()` / the main.cpp OR-chain.
No boss; `boss.cpp` / `boss.h` untouched.

## CLI
`dark_souls_raylib.exe gothic` (aliases `buttress`, `rosewindow`).
HUD banner: **The Excommunicate**.

## Additive-only guarantee
Purely additive: a new `LEVEL_GOTHIC` enum, a new draw/build pair, two new helpers
(`draw_pinnacle`, `draw_lancet`), and one appended entry in each themed array + save
table. No existing level, the boss, or shared geometry was modified.
`git diff --stat -- src/boss.cpp src/boss.h` is empty; build is `BUILD_OK`.
