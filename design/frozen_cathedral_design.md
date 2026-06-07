# Frozen Cathedral Level Design Spec

## Mood
Moonless, pale, and breath-cold. The arena should feel sacred but dead: gothic cathedral ruins rising out of a frozen reflective lake, cold fog hanging low, faint god-rays from an unseen overcast sky, and pale-blue bioluminescent ice crystals replacing the existing level's red blood-moon palette.

## Palette

| Use | Hex | Notes |
| --- | --- | --- |
| sky/zenith | `#07111F` | Near-black blue upper sky, no moon disk. |
| horizon fog | `#D8EEF3` | Milky cold haze behind ruins. |
| ice | `#9BD9EE` | Main ice pillar and rim-light color. |
| cold light | `#C9F3FF` | Directional light tint and bright edge highlights. |
| crystal glow | `#70E7FF` | Bioluminescent crystal emissive color. |
| frozen-water | `#173047` | Deep reflective lake base. |
| stone | `#2C3942` | Gothic ruin material, desaturated blue-gray. |

## Lighting And Atmosphere

Use linear RGB values in shader uniforms after converting from the hex palette if the renderer expects linear space.

| Setting | Value |
| --- | --- |
| Directional light direction | `normalize(vec3(-0.35, -0.82, -0.45))` |
| Directional light color | `vec3(0.78, 0.94, 1.00)` |
| Directional light intensity | `1.15` |
| Ambient color | `vec3(0.10, 0.18, 0.24)` |
| Ambient intensity | `0.38` |
| Fog color | `vec3(0.78, 0.90, 0.94)` |
| Fog density | `0.030` |
| Fog start/end fallback | `18.0 / 78.0` world units |
| Crystal emissive color | `vec3(0.44, 0.91, 1.00)` |
| Crystal emissive intensity | `2.2` |
| Floor specular | `0.82` |
| Floor roughness | `0.18` |

## Arena Layout

- Circular boss arena radius: `32.0` world units.
- Playable safe center: radius `18.0`; keep this mostly clear for combat readability.
- Frozen lake floor: one circular plane at `y = 0.0`, radius `34.0`, with a subtle raised icy rim at radius `32.0`.
- Boundary: broken cathedral wall fragments and low ice shelves around radius `30.0` to `34.0`; collisions should prevent leaving the lake.
- Main gothic columns: place 10 tall ruined columns on a ring at radius `27.5`; angles `0, 35, 72, 112, 151, 204, 238, 276, 315, 342` degrees. Vary height from `7.0` to `15.0`.
- Broken arches: place 4 large arch silhouettes at angles `30, 125, 215, 305` degrees, radius `29.0`; rotate each to face the arena center.
- Shattered ice pillars: place 7 jagged pillars at radius `17.0` to `24.0`, angles `18, 66, 139, 188, 247, 292, 333` degrees. Use uneven heights `3.5` to `9.0`; keep gaps wide enough for dodging.
- Crystal clusters: place 6 clusters at radius `20.0` to `30.5`, angles `45, 95, 164, 232, 280, 330` degrees. Use them as pale-blue local lights, not hard cover.
- Lone altar ruin: optional low broken dais at `vec3(0.0, 0.05, -10.5)`, radius `2.2`, height `0.35`; keep collision low.

## Frozen-Lake Treatment

- Use a dark blue reflective base material with pale ice cracks in a decal or secondary mesh pass.
- Reflection should be visible but imperfect: screen-space reflection or planar reflection at `35%` to `50%` strength, blurred by roughness.
- Add faint radial scuffs and long white fracture lines so the floor does not read as open water.
- Put fog slightly above the floor, strongest at ankle height, so columns and ice pillars fade into the lake reflection.
- Avoid red, purple-red, blood-water, rose moonlight, or visible moon imagery; the contrast should be cold, blue, and moonless.

## Free CC0 3D Model Types To Source

- Gothic stone column, broken and intact variants.
- Broken gothic arch or cathedral window tracery.
- Ice rock / jagged ice pillar.
- Crystal cluster with simple shard geometry.
- Ruined stone block / fallen masonry pile.
- Frosted altar slab or low circular dais.
- Snow or ice crack decal planes.
