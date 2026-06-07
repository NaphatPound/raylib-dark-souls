#version 330
// Night-sky dome: a vertical gradient (deep violet zenith -> dim blood horizon)
// plus a soft halo around the blood moon. It gives the scene a little ambient sky
// light AND a non-empty backdrop, so the water reflects a real sky instead of the
// black void it used to (the reflection render-texture now has sky in it).
in vec3 vDir;
uniform vec3 uMoonDir;     // normalized world direction from the camera to the moon
out vec4 finalColor;

void main() {
    vec3 d = normalize(vDir);
    float up = clamp(d.y * 0.5 + 0.5, 0.0, 1.0);

    vec3 horizon = vec3(0.115, 0.055, 0.075);
    vec3 zenith  = vec3(0.030, 0.026, 0.060);
    vec3 col = mix(horizon, zenith, pow(up, 0.65));

    // blood-moon glow: a tight bright core feathered into a broad faint halo
    float m = max(dot(d, normalize(uMoonDir)), 0.0);
    col += vec3(0.55, 0.16, 0.11) * pow(m, 7.0);
    col += vec3(0.22, 0.07, 0.06) * pow(m, 2.0) * 0.5;

    finalColor = vec4(col, 1.0);
}
