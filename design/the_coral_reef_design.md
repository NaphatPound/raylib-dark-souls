# The Coral Reef — Level Design (LEVEL_REEF = 42)

A vivid undersea reef on the seabed: branching staghorn, brain, fan and tube corals,
swaying kelp forests and sea anemones, rising bubble columns, drifting light-shafts
and a darting fish school, all in a deep blue-green hush. The thirty-ninth **non-boss**
level — the reef's drowned dead ("The Fathomless") rise from the sand. Clear them all
to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **coral reef** silhouette (four coral
kinds + kelp + anemones + a fish school) found in none of the other forty-two levels.
The "underwater" feel comes from a thick deep blue-green fog + rising bubbles + light
shafts (not a literal water plane). Distinct from the Sunken Aqueduct and the Blighted
Bog.

## Theme & palette
- **Mood:** hushed, weightless, luminous — dim light filtering from far above onto a
  riot of coral colour, bubbles streaming up and a school of fish wheeling past.
- **Light (render.cpp):** dim down-light, cool blue-green light `(0.62, 0.82, 0.92)`,
  submerged teal ambient, deep blue-green water fog (`~#296680`, density `0.0170` —
  the thickest yet, selling the depth). Reuses `sky_ice.fs`; **dry** — its own sandy
  seabed is the floor.
- **Glow:** bioluminescent cyan `(0.40, 0.80, 0.92)`.

## Geometry (arena.cpp :: build_reef / draw_reef / reef_layout)
Reuses `s_dome` (brain coral / anemone bulbs / sand ripples / fan blade), `s_cyl`
(tube coral / fan stalks / amphorae / column), `draw_bone_seg` (kelp / anemone
tentacles), and **`draw_ptree`** (the branching staghorn coral).
1. **Corals** — 15 `reef_layout` formations in four kinds (index%4): branching
   **staghorn** (reused `draw_ptree` in orange), **brain** coral (`s_dome` heads),
   **fan** coral (a stalk + a flat blade that **sways with `s_time`**), and **tube**
   coral clusters; each a collision obstacle.
2. **Central great coral head** — a big brain dome crowned with staghorn branches; the
   landmark obstacle.
3. **Kelp** — 9 clumps of tall strands that **sway with `s_time`**; **anemones** — 6
   bulbs with wavy radiating tentacles.
4. **Ruin hint** — a sunken amphora and a toppled column on the sand.
5. **Underwater FX** — additive drifting light-shafts, rising bubble columns, a
   darting **fish school** (a wheeling swarm), and bioluminescent coral glow.

## Encounter (mobs.cpp)
- **8 "Fathomless"** — the `Mob` horde, level-aware spawn set (`reef_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Fathomless*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–41 and the `Boss` are untouched. `mob_level` now covers
  `… || KILN || REEF`. **No new global struct/vector** — `reef_layout` fills a local
  `std::vector<Vector4>` (corals: xyz + w=scale) shared by build+draw; kelp/anemones
  seeded in draw; only `s_wisps` reused. Bioluminescent lights built in `build_reef`
  (so `build_crystals`/`draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_reef` lays the seabed (the deep fog,
  not a water plane, makes it read as underwater).
- CLI: `reef` / `coral` / `seabed` launches straight into it.
