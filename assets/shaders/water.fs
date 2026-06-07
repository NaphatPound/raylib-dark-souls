#version 330
// Planar water + moonlit glitter path.
//  * Planar reflection: the scene is rendered from a camera mirrored across the
//    water plane (same FOV); we sample it at this fragment's own screen position,
//    distorted by the wave normal. The sky dome is in that texture, so there are no
//    black voids -- foreground water correctly reflects the (dim) night zenith.
//  * Moon glade: a Blinn-Phong glitter toward the moon, broken up by the wave
//    normal, draws the shimmering moonlight path across the water toward the viewer.
in vec3 fragWorld;
in vec2 fragUV;

uniform sampler2D uReflect;   // scene from the water-mirrored camera (same FOV)
uniform vec2 uScreen;
uniform float uTime;
uniform vec3 uViewPos;
uniform vec3 uMoonPos;

#define MAX_CRYSTALS 32
uniform int  uCrystalN;
uniform vec4 uCrystals[MAX_CRYSTALS];   // xyz = world pos, w = glow radius (red crystal lights)

out vec4 finalColor;

void main() {
    float t = uTime;
    vec2 P = fragWorld.xz;

    // analytic wave-surface normal (two octaves) -> ripples + glitter
    float a1 = 0.8 * P.x + 1.2 * t, b1 = 0.7 * P.y - 1.0 * t;
    float a2 = 1.8 * P.x - 1.6 * t, b2 = 1.6 * P.y + 1.3 * t;
    float dx = 0.06 * (0.8 * cos(a1) * cos(b1)) + 0.03 * (1.8 * cos(a2) * cos(b2));
    float dz = 0.06 * (-0.7 * sin(a1) * sin(b1)) + 0.03 * (-1.6 * sin(a2) * sin(b2));
    vec3 N = normalize(vec3(-dx, 1.0, -dz));

    // Planar reflection sampled at this fragment's screen position. The mirror
    // camera (up flipped to y-down) renders the reflection texture horizontally
    // mirrored as well as vertically, so undo the horizontal flip here (1 - x) --
    // otherwise the reflection slides the wrong way as the camera pans.
    vec2 ruv = vec2(1.0 - gl_FragCoord.x / uScreen.x, gl_FragCoord.y / uScreen.y);
    ruv += vec2(dx, dz) * 0.10;               // ripple distortion
    ruv = clamp(ruv, vec2(0.001), vec2(0.999));
    vec3 refl = texture(uReflect, ruv).rgb * 1.25;

    // moonlit water body
    vec3 deep    = vec3(0.022, 0.045, 0.075);
    vec3 shallow = vec3(0.050, 0.100, 0.140);
    vec3 base = mix(deep, shallow, clamp(N.y, 0.0, 1.0));

    // fresnel mirror blend (grazing -> near-perfect mirror)
    vec3 V = normalize(uViewPos - fragWorld);
    float fres = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 3.0);
    float k = clamp(0.50 + 0.45 * fres, 0.0, 0.97);
    vec3 col = mix(base, refl, k);

    // moonlight reflection path: a Blinn-Phong glitter toward the moon
    vec3 Lm = normalize(uMoonPos - fragWorld);
    float nh = max(dot(N, normalize(V + Lm)), 0.0);
    col += vec3(1.5, 0.62, 0.40) * pow(nh, 220.0) * 1.0;    // sharp sparkles
    col += vec3(1.0, 0.40, 0.28) * pow(nh, 26.0) * 0.22;    // soft moon glade

    // red crystal light pooled on the water: radial falloff from each cluster,
    // with a little ripple shimmer so the pools aren't flat
    float cglow = 0.0;
    for (int i = 0; i < MAX_CRYSTALS; i++) {
        if (i >= uCrystalN) break;
        float d = distance(P, uCrystals[i].xz);
        float fall = clamp(1.0 - d / uCrystals[i].w, 0.0, 1.0);
        cglow += fall * fall;
    }
    cglow *= 0.85 + 0.15 * sin(P.x * 2.0 + P.y * 1.7 + t * 1.5);
    col += vec3(0.95, 0.13, 0.09) * cglow * 0.9;

    finalColor = vec4(col, 0.97);
}
