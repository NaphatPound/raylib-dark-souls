# Soulslike — Boss Arena (raylib/C++ port) · Build Log (logs.md)

Running log of bugs hit while porting the Godot 4 souls-like to raylib/C++, and how
each was fixed. Newest at the bottom. Design/port notes: [[README.md]].

---

## Asset pipeline (FBX → glb)

- **BUG: glb exported only 2 static clips instead of 34/12 animations.**
  Godot's `GLTFDocument` exported the FBX's bundled bind clips (`Take 001`,
  `mixamo_com`, 1 frame each) — not the per-character `AnimationLibrary` (34 player
  / 12 enemy clips). **Cause:** the glTF exporter resolves animation track paths
  (`Skeleton3D:bone`) relative to the AnimationPlayer's *parent*; my AnimationPlayer
  was a sibling of the model, so the tracks didn't resolve and the model's own
  built-in AnimationPlayer won. **Fix** (`tools/export_glb.gd`): free the FBX's
  built-in AnimationPlayer and parent my AnimationPlayer **inside** the model
  (sibling of `Skeleton3D`) so `Skeleton3D:bone` resolves. → `json_anims=34 / 12`.

- **BUG (critical): player torso/clothes mesh invisible (head + legs only), enemy
  fully invisible.** Two root causes in Godot's glTF export, both unsupported by
  raylib's CPU skinning:
  1. **8 bone influences per vertex** (`JOINTS_1`/`WEIGHTS_1`). raylib only uses the
     first 4 (`JOINTS_0`), so heavily-blended torso/arm vertices lost most of their
     weight and collapsed toward the origin (skin mesh = head/hands/feet survived
     with ≤4 influences).
  2. **Non-topologically-sorted bones** (enemy: 9 violations). raylib assumes
     `parent index < child index` (rmodels.c ~4170/5024) and *skips* out-of-order
     bones → the whole skin tears apart.
  **Fix:** `tools/fix_glb_skin.py` post-processes each glb — merges to the top-4
  renormalized influences (drops `JOINTS_1`/`WEIGHTS_1`) and topologically sorts the
  joints (reorders `inverseBindMatrices`, remaps `JOINTS_0`). Verify with
  `tools/inspect_glb.py` (want 0 violations, no `JOINTS_1`). Also: Godot exported the
  body materials at alpha 0.8, so `shaders/lit.fs` now forces opaque output.

- **NOTE:** raylib truncates `ModelAnimation.name` to 31 chars and samples glTF
  animations at `GLTF_ANIMDELAY=17ms` (~58.82 fps). `anim.cpp` matches clips on the
  truncated name; natural clip duration = `(frameCount-1)/58.82`. Mixamo finger-bone
  tracks don't survive the export (body animation is intact).

## Build / compile

- **BUG: MSVC rejected C99 compound literals** `(Color){...}` (error C4576). **Fix:**
  C++ aggregate init `Color{...}` across hud/screens/pause/main.
- **BUG: `rotate_y` used before defined.** **Fix:** added the Y-rotation helper to
  `mathx.h`.

## Runtime crash

- **BUG (segfault during combat).** The boss's `update` can change the *player's*
  animation (player `on_hurt` runs inside the boss's `hitbox.scan`) **after** the
  player's `anim.update` already ran that frame. The sword's `bone_local_matrix`
  then read `framePoses[last_frame_]` with `last_frame_` from the previous (longer)
  clip → out of bounds on the new shorter clip → segfault. **Fix** (`anim.cpp`):
  reset `last_frame_=0` whenever the clip changes (`set_anim`/`play_fitted`), and
  clamp `last_frame_` to the current clip's `frameCount` in `bone_local_matrix`.

## Port-fidelity review (C++ vs GDScript)

Adversarial multi-agent review found 12 confirmed behavioral divergences; all fixed:
hit-stop re-entrancy (every hit re-armed the freeze → only re-arm when idle); boss
attack/combo selection (RNG → deterministic `state_time` formula); music crossfade
(linear lerp → constant-rate dB `move_toward`); SFX (single voice → 8 sound-alias
pool); boss death pose (fabricated 1.5 s → natural speed); spurious `is_dead` guards
(riposte FX / lock-on / `begin_attack` aim); blood green-channel grouping; pause
slider quantization.

## Visual fixes (user feedback)

- **Sword.** (1) Floated at a fixed world angle → now uses the full right-hand bone
  transform so it tracks the hand. (2) Pointed down → user wanted up: the blade lies
  along the grip's rotation axis, so the X-rotation was a no-op; the **Z** grip angle
  controls blade direction (`-90` → up). (3) Grip sat at the wrist → now anchored at
  the **fist centre**, computed from the `mixamorig_RightHandMiddle1` knuckle bone
  (60 % wrist→knuckle, in the hand's local frame) — robust to the bone's axes.
  Tunables: `GRIP_ROT_DEG`, `FIST_T`, `GRIP_EXTRA` in `sword.cpp`.

- **Blood moon texture.** (1) Flat billboard showed the whole equirectangular
  rectangle in the sky. (2) UV sphere pole-pinched (target artifact) because the
  camera views the moon from below. **Fix:** render a **camera-facing disc shaded as
  a sphere** (`shaders/moon.fs`: circular mask + limb darkening + directional term +
  crater texture warped toward the rim).

- **Water reflection.** Implemented planar reflection (water-mirrored camera →
  render texture → screen-space sample). It looked wrong/empty because tall/edge
  objects (the moon) fell **just outside** the same-FOV mirror camera's frustum.
  **Fix:** widen the mirror camera FOV (×1.5) and compensate the sample UV by
  `1/fovScale` about screen centre, with the correct vertical flip (a point on the
  water plane projects vertically-mirrored in the reflection camera). Reworked the
  water to read **clear with small waves**: subdivided plane + vertex-wave
  displacement, cleaner blue base, brighter mirror, fine ripple distortion, moon
  glint on crests.

- **Movement ripples not visible.** The system worked but the rings were too faint /
  small on the dark scene (and the downscaled screenshots hid them). **Fix:** raised
  them above the water surface, enlarged, brightened, shortened the step distance.

- **Blood not playing on hits.** Wired correctly (`hit_landed → Juice → Fx.hit`) but
  the droplets were tiny and dark-red on a dark target. **Fix:** bigger, brighter,
  more droplets + an impact spark; now reads on every connecting hit.

- **Moon rendered as a black disc with a red rim.** Root cause was NOT the moon
  shader — `boss_arena.glb` ships its **own** dark `Moon` sphere (node/mesh at
  `[0,30,-130]`, `MoonMat`) **and** a `WaterField` plane, both drawn by the lit
  shader. The glb moon sphere sat exactly over our moon disc and occluded it (the
  "rim" was our disc's edge peeking out); the glb water plane poked through our
  wave troughs. **Fix** (`arena.cpp`): detect those two meshes by bounding box at
  load (`is_moon` = centre near `MOON_POS`; `is_water` = flat + wide + low) and skip
  them in `draw_world` (per-mesh `DrawMesh` instead of `DrawModel`). Also reworked
  `moon.fs`: the old version multiplied the dark blood texture down to near-black —
  now classic limb darkening (bright centre → soft rim), a warm blood-orange body,
  and the texture used as subtle crater detail. Reads as a glowing blood moon.

- **Sky was an empty black void / "add some sky light".** Added a night-sky dome
  (`shaders/sky.{vs,fs}`, `arena::draw_sky`): a large unit sphere drawn centred on
  the camera with a vertical gradient (dim-violet zenith → blood horizon) and a soft
  moon-glow halo. Drawn first in **both** the main pass and the reflection pass, so
  it also gives the water a real sky to reflect instead of black.

- **Water reflection: "some fields reflect, some are dark".** The mirror-camera
  reflection texture had **no sky** (cleared to near-black), so any water fragment
  reflecting upward sampled the void → dark patches; the FOV-widen + screen-space
  fudge also misaligned. **Fix:** mirror camera now uses the **same FOV** as the
  main camera and the scene (incl. the sky dome) is sampled at each water fragment's
  own screen position (`gl_FragCoord/uScreen`) — a textbook planar reflection, so
  there are no voids. Added a **moon glade**: an analytic wave-normal Blinn-Phong
  glitter toward `uMoonPos` that draws the shimmering moonlight path across the water
  toward the viewer — the signature "real moonlit water" look.

- **Player/boss not reflected in the water.** The reflection pass only drew
  `draw_sky` + `draw_world` (scenery), so the fighters cast no reflection. **Fix**
  (`main.cpp`): also call `player.draw()` + `boss.draw()` in the reflection pass,
  inside the `rlDisableBackfaceCulling()` region (the mirror flips winding). Both
  `draw()`s are pure rendering (no state mutation) and the CPU skinning is already
  baked by the per-frame anim update, so the extra draw is just two more draw calls.
  Characters stand at `y≈0` (above the `0.02` water plane) so no clip plane is
  needed; their mirrored figures now shimmer in the moon glade below them.

- **Crystals reworked to warm clear-gem boxes.** The red tapered-cylinder shards
  became clusters of **box gems** (`Gem` = base + size + yaw/tilt + size-phase): each
  is an upright rotated box that **breathes** in size (`1 + 0.14·sin`), drawn full-
  bright in rlgl immediate mode in two batched passes — a translucent amber body
  (`BLEND_ALPHA`, depth-write off so it reads as clear glass) and additive glowing
  facet edges (`DrawCubeWiresV`) — plus two additive warm halos for the "shines warm
  light" glow. Rotated via `rlPushMatrix`/`rlRotatef` (vertices bake per-cube, so a
  whole pass batches under one blend mode). Palette pulled to amber/orange because
  additive edges over the amber body otherwise saturate the green channel to yellow-
  green. Reflect in the water via the same `draw_world` path.
  - **Follow-up (user):** make them **static, red, and cloudy**. Dropped the size
    breathing + glow pulse (no `s_time` in `draw_crystals` now) — sizes are still
    varied per gem, just not animated. Body is a fairly opaque deep-red (`alpha 212`,
    depth write on) so it reads as a cloudy gem rather than clear glass; facet lines
    are soft dark-red (alpha, not additive); a soft static red additive halo remains.
    Per-gem `shade` (fixed 0.7–1.0) gives cluster variety without animation.
  - **Follow-up (user): crystal glow lights up the water.** Each cluster becomes a red
    point light fed to `water.fs`: `arena.cpp` builds `s_crystal_lights` (one `vec4`
    per cluster — xyz pos, w = glow radius scaled by cluster size) and uploads it once
    in `load()` via `SetShaderValueV` (`uCrystals[32]` + `uCrystalN`; positions are
    static so it's set once, not per-frame). `water.fs` loops the lights and adds a red
    radial falloff (`(1 - d/r)²`) on the water surface with a little ripple shimmer, so
    a red pool glows on the water around each crystal.

- **BUG: water reflection horizontally mirrored (slid the wrong way when the camera
  panned).** The mirror camera was built with `mcam.up.y = -up.y`; raylib's
  `MatrixLookAt` derives `right = normalize(cross(up, fwd))`, so negating up negates
  the right vector too — the reflection texture comes out flipped **both** vertically
  (wanted) **and horizontally** (the bug). Invisible on the near-symmetric arena, but
  obvious once the view is off-centre: an object on the right reflected on the left,
  so the reflection appeared to "follow the camera." Proven with a green marker on the
  right whose reflection landed on the left. **Fix** (`water.fs`): sample the
  reflection at `(1 - x, y)` instead of `(x, y)` — undo the horizontal mirror. (The
  vertical flip from the y-down up vector is exactly the wanted reflection inversion,
  so y is left alone.) Marker then reflected directly below itself.

- **Combat VFX (user): sword trail, impact wave, reworked telegraph.**
  - *Sword trail* — `sword_blade_segment()` (sword.cpp) returns the blade base/tip
    world points (blade runs along the hand-bone's local +X after the grip rotation,
    so it's computed straight from the hand frame — no matrix-order guessing). Player
    samples it each frame during ATTACK/RIPOSTE into `sword_trail_` (distance-gated so
    slow wind-ups don't clump), and `draw_sword_trail()` connects the segments into a
    faint additive white ribbon (bright at the tip, ~0.28× alpha at the base, 0.12 s
    life) — a subtle "wind" streak, not a solid sheet.
    Final tuning per user feedback: short life (0.08 s) so it's born and gone fast;
    alpha driven by age with a steep cubic falloff so only the freshest segments at
    the blade read bright white and the tail thins out to clear like wind. Colour is
    now pure white `255,255,255` (the earlier blue-white tint read muddy over the red
    scene) and the solid-white default texture is bound explicitly; additive blending
    only adds white light, so the trail is white where visible and clear where faded —
    never black.
  - *Impact wave* — `Fx::impact_wave()` spawns two quick pale camera-facing shockwave
    rings; called from `Juice::on_hit_landed` alongside the blood, so every connecting
    hit gets a little wave punch.
  - *Telegraph* — replaced the growing red/orange orb with a small **blinking light**
    above the boss: a single `sin`-swell pulse during the wind-up (peaks early so the
    player can react) plus a faint colour sustain. Colour now means parry-ability:
    **orange = normal (parryable)**, **red = heavy/unblockable (cannot parry)** — the
    inverse of the old mapping, per the player's request.

## Next-action pass (worked the to-do.html board)

- **Crystal point lights on geometry** — `lit.fs` now takes `uPLights[32]` (xyz + radius)
  + a shared colour; `LitShader::set_point_lights` (render.cpp) uploads the cluster
  positions once (static), so rocks and fighters near a crystal pick up red light, not
  just the water. Re-used the existing `s_crystal_lights` list (lifted to y=1.0 for the
  geometry; water still uses xz only).
- **FX reflect in the water** — `g_fx.draw(mcam)` added to the reflection pass (main.cpp).
- **Unblockable audio tell** — heavy/red attacks play a deeper swoosh (pitch 0.55) + a
  low growl in `Boss::start_attack`, so the warning is audible, not just coloured.
- **Impact wave scales with damage** — `Fx::impact_wave(pos, amount)` (fx.cpp/juice.cpp).
- **Repo + README** — committed and pushed to GitHub (repo renamed `raylib-dark-souls`);
  added a Screenshots section + `docs/screenshots/`. Verified the full loop (combat →
  phase-2 enrage → boss death → VICTORY) via the auto demo.
- Then cleared the last three (user asked to complete the whole board):
  - **Alpha-blend white trail** (`player.cpp`) — the trail washes toward white over any
    background, so it no longer goes pink crossing the bright moon (additive did);
    depth-write off, low peak so it stays a subtle wisp.
  - **Oblique reflection clip plane** (`lit.fs` `uClipBelow`/`uClipY` + `LitShader::set_clip`,
    toggled per-pass in `main.cpp`) — the reflection only mirrors above-water geometry.
  - **New phase-2 boss attack** (`boss.cpp`) — an unblockable `zombie_scream` roar-slam
    added to the phase-2 pool (red telegraph + growl cue); verified through to victory.

- **BUG: heavy-attack sword trail not showing.** A debug trace confirmed the trail
  *was* emitting segments for the heavy attack (the autopilot only ever did light
  attacks, so heavy was never exercised). The cause was the b4 alpha-blend change: the
  heavy `standing_melee_attack_360_high` spin holds the blade up at head height right
  over the **bright** moon/sky, where 35%-alpha white only nudges the already-bright
  red a little → invisible (the light attacks swing lower over darker areas, so they
  still read). **Fix** (`player.cpp`): raised the trail's leading-edge alpha (90→160);
  the cubic falloff keeps the tail clear, so it now reads over bright backgrounds for
  both attacks while staying a concentrated wisp.
