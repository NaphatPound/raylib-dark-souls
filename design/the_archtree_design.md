# The Archtree — Level Design (LEVEL_ARCHTREE = 58)

A single **colossal sacred tree** dominating the whole arena: a massive tapering trunk
rising into a broad spreading canopy, splayed root buttresses you weave between, bracket
mushrooms climbing the bark, hanging vines, satellite saplings, fireflies and falling
leaves over a mossy forest floor. The fifty-fifth **non-boss** level — husks that have
grown rooted to the tree ("The Grafted") rise to defend it. Clear them all to win; the
bonfire then ignites as usual.

New procedural geometry, not a recolour — a **one-giant-tree** silhouette (massive
central trunk + root buttresses + high canopy of foliage masses) found in none of the
other fifty-eight levels. A **DRY** level (its own mossy floor slab; the water plane is
suppressed). Deliberately distinct from the Overgrown Temple (stone ruins pierced by
roots), the Petrified Forest (many scattered dead trees), and the Blighted Bog (weeping
willows over water): here there is **one** vast tree, vertical and singular.

## Theme & palette
- **Mood:** ancient, sacred, alive — soft sun filtering through a high canopy onto moss.
- **Light (render.cpp):** soft filtered sun `(0.26,0.80,0.34)`, warm green-gold daylight
  `(0.96,0.98,0.78)`, mossy green ambient, soft green forest haze (`~#7F9E7F`, density
  `0.0090`). Reuses `sky_ice.fs`; **DRY** (`draw_archtree` lays a mossy floor slab).
- **Glow:** sacred green-gold sap `(0.78,0.95,0.52)`.

## Geometry (arena.cpp :: build_archtree / draw_archtree / archtree_layout)
Reuses `draw_bone_seg` (trunk segments / root buttresses / canopy limbs / vines),
`s_dome` (foliage masses / crown cap / moss mounds / bracket mushrooms), `s_column`
(floor slab), and **`draw_ptree`** (satellite saplings).
1. **Colossal trunk** — 9 stacked tapering `draw_bone_seg` segments (3.4u → 0.4u radius)
   leaning slightly, rising 20u; the central landmark obstacle.
2. **Root buttresses** — 6 two-segment roots sweeping out from the trunk base and curving
   down into the ground; their tips are obstacles.
3. **Canopy** — 8 limbs radiating from the crown, each tipped with 3 foliage domes and a
   hanging vine strand, plus a broad cap of overlapping crown domes (high above eye level).
4. **Bracket mushrooms** — domes climbing the trunk at varied heights.
5. **Saplings** — 7 `draw_ptree` satellites with foliage domes (obstacles).
6. **Floor & atmosphere** — mossy slab + moss mounds; additive sap motes, fireflies, and
   drifting falling leaves.

## Encounter (mobs.cpp)
- **8 "Grafted"** — the `Mob` horde, level-aware spawn set (`archtree_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Grafted*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–57 and the `Boss` are untouched. `mob_level` now covers
  `… || LOTUS || ARCHTREE`. **No new global struct/vector** — `archtree_layout` fills a
  local `std::vector<Vector4>` (saplings: xyz + w=height) shared by build+draw; trunk,
  roots and canopy are deterministic loops; only `s_wisps` reused. Sap lights built in
  `build_archtree` (so `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_archtree` lays the mossy floor.
- CLI: `archtree` / `greattree` / `worldtree` launches straight into it.
