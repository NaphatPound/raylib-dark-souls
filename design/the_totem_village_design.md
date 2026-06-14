# The Totem Village — Level Design (LEVEL_TOTEM = 86)

A Pacific Northwest village in coastal mist: tall carved **totem poles** — stacked bears,
winged figures and beaked thunderbirds painted in red, black and teal with outspread
wing-boards — ring a cedar-plank **longhouse** whose painted facade frames a round entrance,
while dugout canoes rest on the shore, a fire pit smoulders, salmon dry on a rack and dark
cedars loom in the fog. The eighty-third **non-boss** level — the village's risen dead ("The
Ancestral") rise among the poles. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a **Northwest-Coast totem-village** silhouette
(carved stacked-figure poles + a painted longhouse + canoes) found in none of the other
eighty-six levels. A **DRY** level (mossy clearing). A wholly new cultural tradition — distinct
from every other settlement/village level: the Plague Hamlet (timber cottages), Siege Camp
(tents), Stilt Village (huts on stilts), and Canals (Venetian houses).

## Theme & palette
- **Mood:** ancient, watchful, hushed — carved ancestors in a damp cedar fog.
- **Light (render.cpp):** soft overcast forest light `(0.32,0.74,0.36)`, cool grey-green
  daylight `(0.86,0.90,0.88)`, cool even fill, cool coastal mist (`~#99A8A3`, density
  `0.0098`). Reuses `sky_storm.fs`; **DRY** (`draw_totem_village` lays the mossy clearing).
- **Glow:** warm fire-pit `(1.00,0.58,0.30)`.

## Geometry (arena.cpp :: draw_totem / build_totem / draw_totem_village)
Reuses the lit `s_cyl` (pole shafts / fire-pit ring / rack posts / cedar trunks via
`draw_bone_seg`), `s_column` (carved figure blocks / wing-boards / longhouse / canoes / drying
fish), `s_cone` (beaks / cedar-tree tiers), `s_dome` (scrub mounds), and `draw_bone_seg`
(fire logs / rack rail / cedar trunks).
1. **Totem poles** (`draw_totem`) — a pole carved into three stacked figures (a bear with eyes
   + snout, a winged figure with red wing-boards, a thunderbird with a beak + teal wings),
   painted red/black/teal. Five village poles + a central great pole (the landmark obstacle).
2. **Longhouse** — a cedar-plank building with a low gable, a round entrance, and a painted
   thunderbird facade (obstacle).
3. **Village life** — a smouldering fire pit (obstacle), two dugout canoes with high prows, a
   salmon-drying rack, and seven dark cedar trees around the rim.
4. **Atmosphere** — additive fire glow + smoke rising from the longhouse + drifting coastal
   mist.

## Encounter (mobs.cpp)
- **8 "Ancestral"** — the `Mob` horde, level-aware spawn set (`totem_defs`), among the poles.
  Same lightweight AI, lock-on, aggregate HUD bar (named *The Ancestral*), victory-on-clear.

## Integration notes
- Purely additive: levels 0–85 and the `Boss` are untouched. `mob_level` now covers
  `… || MOSQUE || TOTEM`. **No new global struct/vector** — the pole positions are a fixed
  array shared by build (obstacles) + draw; longhouse/fire/canoes/cedars fixed or seeded; only
  `s_wisps` reused. Fire/pole lights built in `build_totem` (so `build_crystals`/`draw_crystals`
  early-return).
- DRY level: `draw_water` early-returns; `draw_totem_village` lays the mossy clearing.
- CLI: `totem` / `totems` / `longhouse` launches straight into it.
