# The Sunken Colosseum Level Design Spec

## Intent

Ancient flooded amphitheater boss arena. This is an enclosed circular structure with stepped concentric walls, not an open field with scattered pillars. The player fights in a shallow reflective pit below stadium-like tiers, with four arched gateways cutting through the ring at the cardinal directions.

## Palette

| Use | Hex | Notes |
| --- | --- | --- |
| storm sky | `#16222B` | Overcast upper sky and heavy cloud base. |
| wet stone | `#394D4D` | Main desaturated blue-green stone. |
| mossy stone | `#53685C` | Damp moss staining on upper ledges and broken blocks. |
| floodwater | `#2D575D` | Shallow reflective arena water. |
| brazier fire | `#E9832C` | Warm local contrast at gateway braziers. |
| lightning | `#D8F4FF` | Occasional cold flash and rim highlight. |

## Core Geometry

Use world origin as the center of the pit. `+Z` is north, `+X` is east. Build as procedural rings from cylinders or segmented box wedges. Keep the playable floor clear except for the low dais.

| Element | Value |
| --- | --- |
| Playable pit radius | `24.0` |
| Floodwater surface | cylinder/plane radius `25.0`, y `0.02`, depth visual `0.12` |
| Pit stone floor | cylinder radius `25.5`, y `-0.08`, height `0.16` |
| Outer collision radius | `43.5` except at sealed gateway backs |
| Recommended radial segments | `96` for circular meshes, or `64` minimum |

## Tiered Wall Rings

Build six concentric stepped seating/wall rings. Each ring is a raised annular band with a vertical face toward the pit and a flat tread above it. Leave gateway gaps open at the four cardinal angles.

| Ring | Inner Radius | Outer Radius | Top Y | Vertical Face Height | Material |
| --- | ---: | ---: | ---: | ---: | --- |
| Pit curb | `24.0` | `25.5` | `0.35` | `0.35` | wet stone |
| Tier 1 | `26.0` | `28.5` | `0.95` | `0.60` | wet stone |
| Tier 2 | `29.2` | `31.7` | `1.65` | `0.70` | wet stone + moss edges |
| Tier 3 | `32.4` | `35.0` | `2.45` | `0.80` | wet stone |
| Tier 4 | `35.8` | `38.5` | `3.35` | `0.90` | mossy stone |
| Outer wall | `39.5` | `43.5` | `5.25` | `1.90` | dark wet stone |

Procedural notes:

- Split every ring into arc segments and skip a gateway gap centered at `0, 90, 180, 270` degrees.
- Gateway gap width by angle: `18` degrees for tiers 1-4, `14` degrees for pit curb, `22` degrees for outer wall.
- Add low broken parapet blocks on the outer wall: `1.2 x 0.7 x 0.9` boxes every `10` degrees, y base `5.25`, but omit them within gateway gaps.
- Add uneven ruined blocks on tier treads: small boxes `0.8-2.0` wide, `0.25-0.55` high, placed sparsely on rings 2-5; keep the pit clear.

## Gateways

Four tall arched gateways break the enclosure at the cardinal directions. Each gateway should read as a tunnel through the tiered wall, with a visible arch silhouette and dry raised threshold just above water level.

| Gateway | Angle | Center Position | Facing |
| --- | ---: | --- | --- |
| North | `0 deg` | `vec3(0, 0, 42.0)` | toward `-Z` |
| East | `90 deg` | `vec3(42.0, 0, 0)` | toward `-X` |
| South | `180 deg` | `vec3(0, 0, -42.0)` | toward `+Z` |
| West | `270 deg` | `vec3(-42.0, 0, 0)` | toward `+X` |

Arch dimensions:

- Clear opening width: `7.0`.
- Vertical side height before arch curve: `5.2`.
- Arch crown height: `8.4`.
- Arch depth through wall: `5.0`.
- Pillar thickness: `1.2` each side.
- Threshold slab: `8.8 x 5.5 x 0.35`, top y `0.28`, centered at radius `37.8`.
- Use box pillars plus a half-torus/segmented arch, or approximate the arch with 10-14 wedge blocks above the opening.

## Central Dais

Low raised circular platform for boss staging without becoming a major obstacle.

| Element | Value |
| --- | --- |
| Dais radius | `4.2` |
| Dais height | `0.38` |
| Top y | `0.40` |
| Beveled lower radius | `5.0`, height `0.18` |
| Material | wet stone, lighter worn top `#53685C` |
| Collision | enabled, low step; player can roll/walk over edge |

## Braziers

Each gateway has two lit braziers flanking the inner mouth of the arch.

Placement formula for gateway angle `a`:

- Gateway radial unit: `r = vec2(sin(a), cos(a))`.
- Tangent unit: `t = vec2(cos(a), -sin(a))`.
- Brazier positions: `r * 34.6 + t * +/-5.4`, y base `0.95`.

Concrete positions:

| Gateway | Brazier A | Brazier B |
| --- | --- | --- |
| North | `vec3(-5.4, 0.95, 34.6)` | `vec3(5.4, 0.95, 34.6)` |
| East | `vec3(34.6, 0.95, 5.4)` | `vec3(34.6, 0.95, -5.4)` |
| South | `vec3(5.4, 0.95, -34.6)` | `vec3(-5.4, 0.95, -34.6)` |
| West | `vec3(-34.6, 0.95, -5.4)` | `vec3(-34.6, 0.95, 5.4)` |

Brazier mesh: stone pedestal cylinder radius `0.45`, height `1.0`; bowl radius `0.75`, y `2.05`; flame billboard or particles height `1.0`. Local light color `vec3(1.00, 0.45, 0.16)`, intensity `2.4`, radius `8.0`.

## Water And Atmosphere

- Floodwater is shallow and covers the pit floor up to y `0.02`; the dais and gateway thresholds remain above it.
- Reflection strength `0.45`, roughness `0.22`, specular `0.75`.
- Add slow scrolling normal map or procedural ripples with amplitude `0.025`.
- Add low rain-mist as translucent horizontal quads around y `0.25-1.2`, strongest at radii `18-34`.
- Rain streaks should be visible against the dark wall, falling direction `vec3(-0.18, -1.0, -0.08)`.

## Lighting Values

Use these as shader/gameplay constants; convert to linear space if the renderer expects linear colors.

| Setting | Value |
| --- | --- |
| Directional light direction | `normalize(vec3(-0.28, -0.86, -0.43))` |
| Directional light color | `vec3(0.62, 0.74, 0.76)` |
| Directional light intensity | `0.82` |
| Ambient color | `vec3(0.10, 0.16, 0.18)` |
| Ambient intensity | `0.34` |
| Fog color | `vec3(0.43, 0.52, 0.53)` |
| Fog density | `0.038` |
| Fog start/end fallback | `10.0 / 62.0` world units |
| Water color | `vec3(0.18, 0.34, 0.36)` |
| Water specular | `0.75` |
| Water roughness | `0.22` |
| Lightning flash color | `vec3(0.85, 0.96, 1.00)` |
| Lightning flash intensity | `2.8` for `0.08-0.16` seconds |
| Brazier light color | `vec3(1.00, 0.45, 0.16)` |
| Brazier light intensity | `2.4` |

## Readability Constraints

- Keep the central radius `0-16` mostly empty so boss movement is readable.
- The enclosure must be visible from the pit: tier tops should rise above player eye level by ring 3 and fully wall off the horizon by the outer wall.
- Do not use scattered monoliths as the primary silhouette. The identity comes from continuous stepped rings, arched breaks, shallow water, and warm braziers against cold storm light.
