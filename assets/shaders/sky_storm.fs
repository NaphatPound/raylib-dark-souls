#version 330
// Stormy overcast sky for the Sunken Colosseum: dark storm-grey-blue zenith fading to a
// pale wet haze at the horizon, with a brighter overcast "break" where the light breaks
// through the clouds.
in vec3 vDir;
uniform vec3 uMoonDir;     // direction of the bright overcast break
out vec4 finalColor;

void main() {
    vec3 d = normalize(vDir);
    float up = clamp(d.y * 0.5 + 0.5, 0.0, 1.0);

    vec3 horizon = vec3(0.50, 0.56, 0.62);   // pale grey rain haze
    vec3 zenith  = vec3(0.17, 0.23, 0.31);   // dark storm grey-blue
    vec3 col = mix(horizon, zenith, pow(up, 0.7));

    float m = max(dot(d, normalize(uMoonDir)), 0.0);
    col += vec3(0.22, 0.26, 0.30) * pow(m, 3.0);   // light breaking through cloud

    finalColor = vec4(col, 1.0);
}
