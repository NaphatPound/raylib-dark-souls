# The Lotus Pond — Level Design (LEVEL_LOTUS = 57)

A serene water-garden: a still pond carpeted with giant lily pads and pink lotus
blooms, a central stilted pavilion under a flared two-tier pagoda roof, zigzag plank
bridges over the water, stepping stones, reed banks, eave lanterns and darting koi.
The fifty-fourth **non-boss** level — the pond's drowned dead ("The Stilled") rise
from the still water. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **lotus pond** silhouette (lily pads +
blooms + a pagoda-roofed water pavilion + zigzag bridges) found in none of the other
fifty-seven levels. A **WET** level (the reused turquoise `water_ice` plane is the
pond). Distinct from the Hanging Gardens (dry formal flowerbeds), the Sunken Aqueduct
and the Blighted Bog.

## Theme & palette
- **Mood:** tranquil, contemplative — soft warm daylight on still water, blossoms
  open, koi flashing and petals drifting.
- **Light (render.cpp):** soft warm sun, gentle warm daylight `(1.02, 0.96, 0.84)`,
  soft bright ambient, soft misty haze (`~#9EADA4`, density `0.0080`). Reuses
  `sky_ice.fs`; **WET** — the reused turquoise `water_ice` plane is the pond.
- **Glow:** warm lantern / blossom-pink `(1.00, 0.74, 0.64)`.

## Geometry (arena.cpp :: build_lotus / draw_lotus_pond / draw_lotus / draw_pavilion / lotus_layout)
Reuses `s_cyl` (lily pads / stilts / posts / stepping stones), `s_dome` (buds / mud
banks / eave lanterns / finial), `s_cone` (lotus petals / pagoda roof tiers),
`s_column` (deck / plank bridges), `draw_bone_seg` (reeds).
1. **Lily pads** — 26 `draw_lotus` floating pads, every fourth bearing a pink lotus
   bloom (a bud ringed by flared petals).
2. **Water pavilion** — `draw_pavilion`: an open stilted deck under a **two-tier flared
   pagoda roof** with a finial and eave lanterns; the central landmark obstacle.
3. **Bridges & stones** — stepping stones (obstacles) linked to the pavilion deck by
   **zigzag plank walkways** on posts.
4. **Banks** — reed clumps and mud banks at the rim (no ground slab — the water plane
   is the pond).
5. **Atmosphere** — additive warm lantern glow, darting **koi**, drifting **petals**.

## Encounter (mobs.cpp)
- **8 "Stilled"** — the `Mob` horde, level-aware spawn set (`lotus_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Stilled*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–56 and the `Boss` are untouched. `mob_level` now covers
  `… || WEB || LOTUS`. **No new global struct/vector** — `lotus_layout` fills a local
  `std::vector<Vector4>` (pads: xyz + w=radius) shared by build+draw; pavilion/stones
  fixed; only `s_wisps` reused. Lantern lights built in `build_lotus` (so
  `build_crystals`/`draw_crystals` early-return).
- WET level: `draw_water` still draws the pond plane (NOT in the dry early-return);
  `draw_lotus_pond` lays only the pads/pavilion/bridges over it.
- CLI: `lotus` / `pond` / `pavilion` launches straight into it.
