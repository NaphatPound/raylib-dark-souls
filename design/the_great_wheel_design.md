# The Great Wheel — Level Design (LEVEL_WHEEL = 73)

A colossal observation wheel turning slowly at dusk: a steel rim of spokes hung with
colourful upright gondolas rotates on a great A-frame, string-lights chasing around it,
above a plaza with a boarding platform, a ticket booth, a central kiosk and lamp posts.
The seventieth **non-boss** level — the fairground's fallen riders ("The Suspended") rise
in the plaza. Clear them all to win; the bonfire then ignites as usual.

New procedural geometry, not a recolour — a giant **Ferris-wheel** silhouette (an animated
vertical rim + gondolas on an A-frame) found in none of the other seventy-three levels. A
**DRY** level (plaza floor). Deliberately distinct from the Forsaken Fairground (a small
*horizontal* spinning carousel + tents), and from the Watermill / Sawmill / Clockwork
wheels (a paddle-wheel, a sawblade, brass gears): this is one towering vertical
passenger wheel.

## Theme & palette
- **Mood:** wistful, festive-melancholy — a great wheel still turning over an empty fair at
  sunset.
- **Light (render.cpp):** low golden-dusk sun `(0.40,0.66,0.42)`, warm sunset glow
  `(1.00,0.86,0.74)`, soft violet-blue evening fill, dusky haze (`~#75708A`, density
  `0.0080`). Reuses `sky_storm.fs`; **DRY** (`draw_wheel` lays the plaza floor).
- **Glow:** warm festive string-light `(1.00,0.72,0.42)`.

## Geometry (arena.cpp :: build_wheel / draw_wheel)
Reuses `draw_bone_seg` (rim arcs / spokes / axle / hub / A-frame legs + braces), `s_column`
(gondola bodies + roofs / boarding platform / plaza), `s_cyl` (gondola hangers / lamp posts
/ kiosk), `s_dome` (lamp globes), `s_cone` (kiosk roof), and `draw_cottage` (ticket booth).
1. **The wheel** — set at the back as a rising backdrop (hub ~12u up at z≈-8). A rim of 14
   `draw_bone_seg` arc segments + spokes, **rotating about the depth axis via `s_time`**;
   each rim point hangs an **upright gondola** (it does NOT spin with the wheel — stays
   level, as a real Ferris-wheel car) in one of four festive colours.
2. **Supports** — two steel A-frames flanking the wheel plane up to the axle/hub, with a
   cross brace.
3. **Plaza** — the floor, a boarding platform at the wheel's foot, a ticket booth, lamp
   posts, and a central kiosk with a conical roof (the landmark obstacle).
4. **Atmosphere** — additive **rotating rim string-lights** (alternating warm/cool) +
   spoke fairy-lights + lamp glow + drifting dusk haze.

## Encounter (mobs.cpp)
- **8 "Suspended"** — the `Mob` horde, level-aware spawn set (`wheel_defs`), in the plaza in
  front of the wheel. Same lightweight AI, lock-on, aggregate HUD bar (named *The
  Suspended*), and victory-on-clear.

## Integration notes
- Purely additive: levels 0–72 and the `Boss` are untouched. `mob_level` now covers
  `… || THEATRE || WHEEL`. **No new global struct/vector** — the wheel is a deterministic
  rotating loop (animated by `s_time`); support feet / platform / booth obstacles fixed;
  only `s_wisps` reused. Festive lights built in `build_wheel` (so `build_crystals`/
  `draw_crystals` early-return).
- DRY level: `draw_water` early-returns; `draw_wheel` lays the plaza floor.
- NOTE: like the Skydock airship, the wheel rises overhead — it was placed at the back and
  lowered so the eye-level demo camera frames its lower arc, gondolas and supports.
- CLI: `wheel` / `ferris` / `fairwheel` launches straight into it.
