# Pumpkin Patch — Level Design (LEVEL_PUMPKIN = 53)

A hallowed autumn-dusk harvest field: big ribbed pumpkins sprawling on vines, carved
glowing jack-o'-lanterns, teepee corn-shocks, round hay bales and a pumpkin-headed
scarecrow under a golden dusk. The fiftieth **non-boss** level — the patch's reaped
dead ("The Harvested") rise among the gourds. Clear them all to win; the bonfire then
ignites as usual.

New procedural geometry, not a recolour — a **pumpkin patch** silhouette (ribbed
gourds + glowing jack-o'-lanterns + corn shocks) found in none of the other fifty-three
levels. Distinct from the Windmill Fields (wheat + windmills + scarecrows on
farmland) — the pumpkin/jack-o'-lantern/corn-shock focus is the differentiator.

## Theme & palette
- **Mood:** crisp, eerie, golden — low amber dusk over an orange harvest field, carved
  pumpkins glowing in the gloom and leaves drifting.
- **Light (render.cpp):** low golden sun, rich amber dusk `(1.08, 0.78, 0.46)`, warm
  dusky ambient, dim autumn haze (`~#765C4C`, density `0.0100`). Reuses `sky_forge.fs`
  (warm dusk); **dry** — its own earthy field is the floor.
- **Glow:** jack-o'-lantern orange `(1.00, 0.55, 0.15)`.

## Geometry (arena.cpp :: build_pumpkin / draw_pumpkin / draw_pumpkin_gourd / draw_cornshock / pumpkin_layout)
Reuses `s_dome` (gourd halves / scarecrow body / hay), `s_cyl` (stems / hay bales /
scarecrow post), `s_column` (field / furrows / face insets / scarecrow arms),
`draw_bone_seg` (ribs / vines / corn stalks).
1. **Pumpkins** — `draw_pumpkin_gourd`: a squashed sphere (a top dome + a flipped
   bottom dome) banded by darker ribs with a green stem. 13 seeded gourds (the larger
   ones obstacles) + a **central great pumpkin**.
2. **Jack-o'-lanterns** — every third gourd is carved (dark eye/mouth insets on the
   front) and glows from within (additive).
3. **Corn shocks** — `draw_cornshock`: teepee bundles of dried cornstalks with a tie
   band, in rows.
4. **Harvest dressing** — round hay bales, sprawling vine tendrils, earthen furrows,
   and a pumpkin-headed **scarecrow** on crossed poles.
5. **Atmosphere** — additive jack-o'-lantern glow, drifting autumn **leaves**, and warm
   dusk sparkle.

## Encounter (mobs.cpp)
- **8 "Harvested"** — the `Mob` horde, level-aware spawn set (`pumpkin_defs`). Same
  lightweight AI, lock-on, aggregate HUD bar (named *The Harvested*), and
  victory-on-clear.

## Integration notes
- Purely additive: levels 0–52 and the `Boss` are untouched. `mob_level` now covers
  `… || PLAZA || PUMPKIN`. **No new global struct/vector** — `pumpkin_layout` fills a
  local `std::vector<Vector4>` (gourds: xyz + w=radius) shared by build+draw; corn
  shocks/hay/scarecrow seeded/fixed; only `s_wisps` reused. Jack lights built in
  `build_pumpkin` (so `build_crystals`/`draw_crystals` early-return).
- Dry level: `draw_water` early-returns; `draw_pumpkin` lays the field floor.
- CLI: `pumpkin` / `patch` / `hallow` launches straight into it.
