#version 330
// Volcanic-forge sky: a dark smoky cavern roof up high fading to a hot orange-red
// glow near the horizon (lava light from below), with a brighter glow patch.
in vec3 vDir;
uniform vec3 uMoonDir;     // reused as the direction of the hottest glow patch
out vec4 finalColor;

void main() {
    vec3 d = normalize(vDir);
    float up = clamp(d.y * 0.5 + 0.5, 0.0, 1.0);

    vec3 horizon = vec3(0.55, 0.20, 0.06);   // hot orange-red
    vec3 zenith  = vec3(0.06, 0.03, 0.04);   // dark smoke
    vec3 col = mix(horizon, zenith, pow(up, 0.5));

    float m = max(dot(d, normalize(uMoonDir)), 0.0);
    col += vec3(0.60, 0.26, 0.07) * pow(m, 4.0) * 0.7;

    finalColor = vec4(col, 1.0);
}
