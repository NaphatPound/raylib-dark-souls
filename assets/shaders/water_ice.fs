#version 330
// Frozen-lake floor: a near-still icy reflective surface (gentler ripples than the
// blood-water), deep cold-blue base, strong cold mirror, a cold glint, pale fracture
// lines so it reads as ice rather than open water, and ice-blue crystal light pools.
in vec3 fragWorld;
in vec2 fragUV;

uniform sampler2D uReflect;
uniform vec2 uScreen;
uniform float uTime;
uniform vec3 uViewPos;
uniform vec3 uMoonPos;

#define MAX_CRYSTALS 32
uniform int  uCrystalN;
uniform vec4 uCrystals[MAX_CRYSTALS];

out vec4 finalColor;

void main() {
    float t = uTime;
    vec2 P = fragWorld.xz;

    // slow, shallow ripples (the lake is mostly frozen)
    float a1 = 0.6 * P.x + 0.5 * t, b1 = 0.5 * P.y - 0.4 * t;
    float dx = 0.035 * (0.6 * cos(a1) * cos(b1));
    float dz = 0.035 * (-0.5 * sin(a1) * sin(b1));
    vec3 N = normalize(vec3(-dx, 1.0, -dz));

    // planar reflection (same screen-space + horizontal flip as the blood-water)
    vec2 ruv = vec2(1.0 - gl_FragCoord.x / uScreen.x, gl_FragCoord.y / uScreen.y);
    ruv += vec2(dx, dz) * 0.06;
    ruv = clamp(ruv, vec2(0.001), vec2(0.999));
    vec3 refl = texture(uReflect, ruv).rgb * 1.15;

    // icy water body
    vec3 deep    = vec3(0.090, 0.190, 0.280);   // #173047
    vec3 shallow = vec3(0.280, 0.430, 0.520);
    vec3 base = mix(deep, shallow, clamp(N.y, 0.0, 1.0));

    vec3 V = normalize(uViewPos - fragWorld);
    float fres = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 3.0);
    float k = clamp(0.55 + 0.42 * fres, 0.0, 0.97);   // very mirror-like (ice)
    vec3 col = mix(base, refl, k);

    // pale fracture lines so the floor reads as cracked ice, not open water
    float cr = sin(P.x * 0.9 + 1.7) * sin(P.y * 0.8 - 2.3) + sin(P.x * 0.35 - P.y * 0.5);
    float crack = smoothstep(0.96, 1.0, abs(fract(cr * 0.5) - 0.5) * 2.0);
    col += vec3(0.45, 0.62, 0.72) * crack * 0.25;

    // cold specular glint toward the light
    vec3 Lm = normalize(uMoonPos - fragWorld);
    float nh = max(dot(N, normalize(V + Lm)), 0.0);
    col += vec3(0.62, 0.74, 0.88) * pow(nh, 180.0) * 0.8;

    // ice-blue crystal light pooled on the lake
    float cglow = 0.0;
    for (int i = 0; i < MAX_CRYSTALS; i++) {
        if (i >= uCrystalN) break;
        float dd = distance(P, uCrystals[i].xz);
        float f = clamp(1.0 - dd / uCrystals[i].w, 0.0, 1.0);
        cglow += f * f;
    }
    col += vec3(0.28, 0.55, 0.82) * cglow * 0.8;

    finalColor = vec4(col, 0.97);
}
