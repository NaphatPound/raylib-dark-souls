#version 330
// Flooded colosseum floor: grey-blue reflective floodwater with busy rain ripples, a
// strong mirror, and warm brazier light pooled on the surface.
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

    // busier, faster rain ripples
    float a1 = 1.0 * P.x + 1.8 * t, b1 = 0.9 * P.y - 1.5 * t;
    float a2 = 2.4 * P.x - 2.1 * t, b2 = 2.0 * P.y + 1.7 * t;
    float dx = 0.050 * (1.0 * cos(a1) * cos(b1)) + 0.025 * (2.4 * cos(a2) * cos(b2));
    float dz = 0.050 * (-0.9 * sin(a1) * sin(b1)) + 0.025 * (-2.0 * sin(a2) * sin(b2));
    vec3 N = normalize(vec3(-dx, 1.0, -dz));

    vec2 ruv = vec2(1.0 - gl_FragCoord.x / uScreen.x, gl_FragCoord.y / uScreen.y);
    ruv += vec2(dx, dz) * 0.10;
    ruv = clamp(ruv, vec2(0.001), vec2(0.999));
    vec3 refl = texture(uReflect, ruv).rgb * 1.2;

    vec3 deep    = vec3(0.10, 0.14, 0.18);
    vec3 shallow = vec3(0.32, 0.40, 0.46);
    vec3 base = mix(deep, shallow, clamp(N.y, 0.0, 1.0));

    vec3 V = normalize(uViewPos - fragWorld);
    float fres = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 3.0);
    float k = clamp(0.52 + 0.44 * fres, 0.0, 0.96);
    vec3 col = mix(base, refl, k);

    // warm brazier light pools on the grey water
    float cglow = 0.0;
    for (int i = 0; i < MAX_CRYSTALS; i++) {
        if (i >= uCrystalN) break;
        float dd = distance(P, uCrystals[i].xz);
        float f = clamp(1.0 - dd / uCrystals[i].w, 0.0, 1.0);
        cglow += f * f;
    }
    col += vec3(0.85, 0.45, 0.18) * cglow * 0.6;

    finalColor = vec4(col, 0.97);
}
