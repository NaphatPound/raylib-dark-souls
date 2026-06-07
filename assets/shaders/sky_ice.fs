#version 330
// Frozen-cathedral sky dome: deep cold-blue zenith fading to a pale icy fog at the
// horizon, with a soft pale cold-light glow (the "sun" behind the mist). Moonless.
in vec3 vDir;
uniform vec3 uMoonDir;     // normalized world direction to the cold light glow
out vec4 finalColor;

void main() {
    vec3 d = normalize(vDir);
    float up = clamp(d.y * 0.5 + 0.5, 0.0, 1.0);

    vec3 horizon = vec3(0.34, 0.45, 0.55);   // pale cold fog band
    vec3 zenith  = vec3(0.05, 0.09, 0.17);   // deep cold blue
    vec3 col = mix(horizon, zenith, pow(up, 0.6));

    // soft cold sun glow behind the mist (pale, no hard disc)
    float m = max(dot(d, normalize(uMoonDir)), 0.0);
    col += vec3(0.42, 0.54, 0.64) * pow(m, 5.0);
    col += vec3(0.18, 0.26, 0.34) * pow(m, 1.6) * 0.6;

    finalColor = vec4(col, 1.0);
}
