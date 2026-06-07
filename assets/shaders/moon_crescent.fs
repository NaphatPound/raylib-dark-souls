#version 330
// Cold crescent moon for the Frozen Cathedral. A camera-facing quad shaded as a disc,
// then masked to a CRESCENT by cutting away an offset "shadow" circle (the unlit part
// is discarded so the night sky shows through). Pale white-blue, limb-darkened, with
// faint crater detail. The soft cold halo around it comes from the sky dome (sky_ice.fs).
in vec2 fragTexCoord;
uniform sampler2D texture0;     // reused crater texture (sampled as grey detail)
out vec4 finalColor;

void main() {
    vec2 p = fragTexCoord * 2.0 - 1.0;     // -1..1 across the quad
    float r2 = dot(p, p);
    if (r2 > 1.0) discard;                 // moon disc
    float z = sqrt(max(0.0, 1.0 - r2));    // front-of-sphere height

    // crescent terminator: a shadow circle offset to the right; lit = outside it
    vec2 sc = p - vec2(0.42, -0.05);
    float lit = smoothstep(0.0, 0.12, dot(sc, sc) - 0.80 * 0.80);
    if (lit < 0.02) discard;               // cut away the shadowed part -> shows sky

    float limb = pow(clamp(z, 0.0, 1.0), 0.42);
    vec2 uv = p * 0.5 + 0.5;
    float tex = dot(texture(texture0, uv).rgb, vec3(0.33));
    float detail = mix(0.85, 1.08, clamp(tex * 1.4, 0.0, 1.0));

    vec3 body = vec3(0.82, 0.92, 1.08);    // cold pale moon
    vec3 col = body * detail * (0.55 + 0.5 * limb) * lit;
    finalColor = vec4(col, 1.0);
}
