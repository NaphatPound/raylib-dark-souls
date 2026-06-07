#version 330
// Molten-lava floor: emissive flowing magma (value-noise) from a dark crust through
// hot orange to bright yellow cracks. Not a mirror (lava is opaque/emissive), but it
// still picks up the crystal (ember) light pools. Reflection uniforms go unused.
in vec3 fragWorld;
in vec2 fragUV;

uniform sampler2D uReflect;   // unused (lava isn't reflective) -> loc resolves to -1
uniform vec2 uScreen;
uniform float uTime;
uniform vec3 uViewPos;
uniform vec3 uMoonPos;

#define MAX_CRYSTALS 32
uniform int  uCrystalN;
uniform vec4 uCrystals[MAX_CRYSTALS];

out vec4 finalColor;

float hash(vec2 p) { return fract(sin(dot(p, vec2(41.3, 289.1))) * 43758.5453); }
float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
    float a = hash(i), b = hash(i + vec2(1, 0)), c = hash(i + vec2(0, 1)), d = hash(i + vec2(1, 1));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void main() {
    vec2 P = fragWorld.xz;
    float t = uTime * 0.25;
    float n = vnoise(P * 0.35 + vec2(t, -t * 0.7)) * 0.6
            + vnoise(P * 0.95 - vec2(t * 0.5, t)) * 0.4;

    vec3 crust  = vec3(0.10, 0.030, 0.020);
    vec3 hot    = vec3(0.95, 0.350, 0.050);
    vec3 bright = vec3(1.30, 0.800, 0.200);
    float crack = smoothstep(0.55, 0.86, n);
    vec3 col = mix(crust, hot, smoothstep(0.22, 0.70, n));
    col = mix(col, bright, crack * crack);

    // ember crystal light pools
    float cglow = 0.0;
    for (int i = 0; i < MAX_CRYSTALS; i++) {
        if (i >= uCrystalN) break;
        float dd = distance(P, uCrystals[i].xz);
        float f = clamp(1.0 - dd / uCrystals[i].w, 0.0, 1.0);
        cglow += f * f;
    }
    col += vec3(0.90, 0.40, 0.10) * cglow * 0.5;

    finalColor = vec4(col, 1.0);
}
