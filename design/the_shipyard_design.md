# The Shipyard — Level 115 (LEVEL_SHIPYARD = 114)

A shipwright's slip at grey dawn. The centrepiece is a great wooden ship caught
mid-construction: a **hull standing in frame** — keel, curved ribs, stem and
sternpost — its port side only half-planked, sitting on greased launching ways
that run down to a strip of harbour water. The yard around it is alive with the
gear of the trade. A horde of "The Unlaunched" — shipwrights who never floated
their hull — work the slip. **No boss.**

## Why it is NOT a recolour / duplicate
Genuinely new procedural geometry. Its silhouette is a **skeletal ship under
construction — a boat-shaped ribcage of frames + scaffolding + sheer-legs over a
slipway** — which no prior level owns. Crucially it is distinct from every other
boat level because the ship is *being built*, not finished or wrecked:
- **The Galleon** ("The Mutinous") — a whole finished sailing ship.
- **Shipwreck Cove** ("The Drowned") — a *sunken / broken* hull.
- **The Skydock** ("The Becalmed") — a harbour *pier* with crates and bollards.
- **The Whaling Station** ("The Flensed") — *flensing a whale carcass*, not building.
A hull-in-frame (open ribs, part-planking, a lifting tripod, launching ways) is a
completely different read from any of those.

## Layout (DRY level — its own packed-earth + cobbled yard, no water plane)
- **The hull in frame** (centre, spine on `x = 0`, stern `z ≈ -13` → bow `z ≈ +2`):
  - **Keel** on keel-blocks + an inner **keelson**.
  - **10 frame ribs**, each two segments (floor→bilge, bilge→sheer with tumblehome),
    their half-beam following `0.28 + 0.72·sin(πt)` so the hull is fat amidships and
    fine at the ends — a true boat shape.
  - A **raked stem** (curved prow) at the bow, a tall **sternpost** + planked
    **transom** at the back.
  - **Port side half-planked** — three runs of strakes (garboard → bilge) following
    the rib points; **starboard left open** but tied by two **ribband battens**.
- **Launching ways**: two greased timber tracks under and ahead of the keel,
  descending toward a grey **harbour-water strip** at the front, with mooring bollards.
- **Sheer-legs**: a timber lifting tripod over the hull with a tackle block, a fall,
  and a frame timber slung mid-lift; **scaffolding** with staging planks down the
  clad port side.
- **Yard gear**: two **timber stacks** (logs in tapering piles), a **steam box** on a
  firebox (for bending planks, leaking steam), a **sawpit** (trestle + squared log +
  pit-saw), a **tar cauldron** over a fire, pitch barrels, a rope coil, a leaning
  **anchor**, and a **grindstone**.

## Mood / theming
- Cool overcast **sea-dawn**: `lightCol {0.86,0.86,0.82}`, blue-grey ambient, salt-air
  mist fog. `sky_storm` backdrop. DRY floor; no gem crystals.
- Point-light colour `pcol_by[114] = { 1.00, 0.62, 0.30 }` — the warm **tar-fire /
  brazier** glow that lights the working yard against the cool dawn.
- Additive pass: tar-fire glow, steam off the steam box, dark smoke off the tar,
  low sea mist drifting across the harbour, gulls wheeling over the slip.

## Enemies
`shipyard_defs[]` — 8 "The Unlaunched", spread across the front apron and the
flanks of the hull (avoiding the blocked keel spine). Regular horde via
`mob_level()` / the main.cpp OR-chain. No boss; `boss.cpp` / `boss.h` untouched.

## CLI
`dark_souls_raylib.exe shipyard` (aliases `slipway`, `drydock`).
HUD banner: **The Unlaunched**.

## Additive-only guarantee
Purely additive: a new `LEVEL_SHIPYARD` enum, a new draw/build pair, two new helper
funcs (`draw_timberstack`, `draw_scaffold`), and one appended entry in each themed
array + save table. No existing level, the boss, or shared geometry was modified.
`git diff --stat -- src/boss.cpp src/boss.h` is empty; build is `BUILD_OK`.
