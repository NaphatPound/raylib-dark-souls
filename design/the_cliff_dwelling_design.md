# The Cliff Dwelling — Level 117 (LEVEL_PUEBLO = 116)

An Ancestral-Puebloan **cliff palace** tucked into a sandstone alcove at desert dusk.
Stacked adobe **room-blocks** climb the back wall in receding tiers, their flat roofs
bristling with protruding **viga beams** over **T-shaped doorways**; a square
four-storey **tower** and a round tower rise above; round subterranean **kivas** with
cribbed-log roofs, ladders and hearth-glow sit in the plaza. A horde of "The
Vanished" — the long-departed dead astir on the ledge — hold the dwelling. **No boss.**

## Why it is NOT a recolour / duplicate
Genuinely new procedural geometry. Its silhouette is **terraced cliff masonry under
an overhang — stacked room-blocks + vigas + round kivas + ladders** — which nothing
else owns:
- **The Treasury / Petra** ("The Entombed") — a *carved rock-cut façade*, not built masonry.
- **The Great Enclosure / Zimbabwe** ("The Dethroned") — *curved dry-stone walls*, no tiers/kivas.
- **The Terracotta Army** — *buried warrior pits*.
- The desert levels (Ziggurat, Sphinx, Oasis, Cactus) — dunes/monuments, not a cliff dwelling.
The defining read — adobe boxes stepping up into a cliff alcove, T-doors, kivas with
ladders poking out — is unique here.

## Layout (DRY level — its own sandy alcove ledge, no water plane)
- **The back cliff**: a great sandstone wall (~24 high) with weathered strata-staining,
  flanking alcove side-walls, and an **overhanging brow** (the alcove ceiling) with a
  shadowed lip — the dwelling nestles beneath it.
- **Three receding tiers of room-blocks** (`draw_roomblock`): a ground row of 5, a
  second tier of 4 stepped back and up, a top tier of 2 — alternating adobe/plaster
  walls, **protruding viga beams** under each roof cap, and **T-shaped doorways**
  (some glowing with hearth-light) or small windows.
- **A square four-storey tower** (the famous Mesa-Verde tower) + a **round tower** with
  a ring of roof vigas.
- **Three round kivas** in the plaza (`draw_kiva`): a sunk masonry ring, a banquette, a
  **cribbed-log roof** (four shrinking layers of crossed logs via `rlPushMatrix`/
  `rlRotatef`), a glowing roof hatch, and a **ladder** poking out.
- **Ladders** (`draw_ladder`) connecting the building tiers.
- **Plaza life**: painted **ollas**, corn-grinding **metates** with manos, water jars,
  and two **corn-drying racks** hung with cobs.

## Mood / theming
- Warm low **desert dusk** in shadow: golden `lightCol {1.16,0.92,0.64}`, warm alcove
  ambient, dusty haze. `sky_forge` warm sunset backdrop; normal moon. DRY; no gem crystals.
- Point-light colour `pcol_by[116] = { 1.00, 0.58, 0.26 }` — warm **kiva-fire** glow.
- Additive pass: kiva/hearth fire glows, rising **kiva smoke**, warm **dust motes** in
  the raking sun, and **cliff swallows** darting round the alcove.

## Enemies
`pueblo_defs[]` — 8 "The Vanished", spread across the plaza in front of the dwelling.
Regular horde via `mob_level()` / the main.cpp OR-chain. No boss; `boss.cpp` /
`boss.h` untouched.

## CLI
`dark_souls_raylib.exe pueblo` (aliases `cliffdwelling`, `mesa`).
HUD banner: **The Vanished**.

## Additive-only guarantee
Purely additive: a new `LEVEL_PUEBLO` enum, a new draw/build pair, three new helpers
(`draw_ladder`, `draw_kiva`, `draw_roomblock`), and one appended entry in each themed
array + save table. No existing level, the boss, or shared geometry was modified.
`git diff --stat -- src/boss.cpp src/boss.h` is empty; build is `BUILD_OK`.
