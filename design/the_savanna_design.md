# The Savanna — Level Design (LEVEL_SAVANNA = 84)

An African grassland under a golden sun: a colossal baobab with its swollen barrel trunk and
sparse "sky-root" crown dominates the plain, scattered flat-topped acacias and tall reddish
termite mounds dot the dry grass, a muddy watering hole gleams with reeds, and a kopje of
boulders shelters a sun-bleached ribcage while vultures wheel overhead. The eighty-first
**non-boss** level — the savanna's wandering dead ("The Parched") rise from the grass. Clear
them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **savanna** silhouette (baobab + flat-topped
acacias + termite mounds + watering hole) found in none of the other eighty-four levels. A
**DRY** level (dry grassland). The first African-grassland biome — distinct from every prior
tree/grassland level: the Orchard (blossom rows), Sunflower Fields (tall yellow), Vineyard
(trellises), Hanging Gardens (formal beds), Redwood Grove (giant trunks), Snowbound Pines
(conifers), the Archtree (one giant tree), and the Bamboo Grove (green stalks).

## Theme & palette
- **Mood:** vast, sun-baked, primal — heat shimmer over endless golden grass.
- **Light (render.cpp):** warm low golden sun `(0.42,0.70,0.32)`, golden daylight
  `(1.14,0.96,0.66)`, warm fill, golden dusty haze (`~#D1BC8F`, density `0.0078`). Reuses
  `sky_ice.fs`; **DRY** (`draw_savanna` lays the dry grassland).
- **Glow:** warm golden sun `(1.00,0.80,0.42)`.

## Geometry (arena.cpp :: draw_baobab / draw_acacia / savanna_layout / build_savanna / draw_savanna)
Reuses the lit `s_cyl` (baobab trunk / acacia umbrella / watering hole), `s_dome` (baobab
crown clumps + top / acacia canopy / grass tufts / kopje boulders), `s_cone` (termite
mounds), and `draw_bone_seg` (baobab branches / acacia trunk+branches / reeds / bleached
ribcage).
1. **Baobab** (`draw_baobab`) — a swollen barrel trunk + a domed top + seven stubby branches
   tipped with leaf clumps; the colossal central landmark obstacle.
2. **Acacias & mounds** — a jittered scatter (`savanna_layout`): flat-topped acacias
   (`draw_acacia`: a slender trunk + a wide umbrella canopy) and tall reddish termite-mound
   cones (every fifth entry), all obstacles.
3. **Watering hole** — a muddy pool with a rim and reeds.
4. **Kopje & bones** — a pile of boulders and a sun-bleached ribcage; dry-grass tufts.
5. **Atmosphere** — additive warm dust + heat shimmer, plus four wheeling vultures (dark
   opaque motes high above).

## Encounter (mobs.cpp)
- **8 "Parched"** — the `Mob` horde, level-aware spawn set (`savanna_defs`), on the plain.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Parched*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–83 and the `Boss` are untouched. `mob_level` now covers
  `… || LOOM || SAVANNA`. **No new global struct/vector** — `savanna_layout` fills a local
  `std::vector<Vector4>` (x, z, kind, param) shared by build (obstacles) + draw; watering
  hole / kopje fixed; only `s_wisps` reused. Warm lights built in `build_savanna` (so
  `build_crystals`/`draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_savanna` lays the dry grassland.
- CLI: `savanna` / `acacia` / `baobab` launches straight into it.
