#version 330
// Render a flat quad as a glowing blood-moon disc: circular mask, classic limb
// darkening (bright centre fading toward the rim), a warm blood-orange body, and
// the blood texture used as SUBTLE crater detail -- not a brightness crush (the old
// version multiplied the dark texture down to near-black, leaving only a red rim).
in vec2 fragTexCoord;
uniform sampler2D texture0;     // blood / crater texture
out vec4 finalColor;

void main() {
    vec2 p = fragTexCoord * 2.0 - 1.0;     // -1..1 across the quad
    float r2 = dot(p, p);
    if (r2 > 1.0) discard;                 // circular disc
    float z = sqrt(max(0.0, 1.0 - r2));    // front-of-sphere height
    vec3 N = vec3(p.x, p.y, z);

    // gentle spherical warp: blend planar<->asin so detail isn't crunched at centre
    vec2 uvp = p * 0.5 + 0.5;
    vec2 uvs = vec2(0.5) + vec2(asin(clamp(p.x, -1.0, 1.0)),
                                asin(clamp(p.y, -1.0, 1.0))) / 3.14159;
    vec2 uv = mix(uvp, uvs, 0.35);
    float tex = dot(texture(texture0, uv).rgb, vec3(0.5, 0.32, 0.18));
    float detail = mix(0.72, 1.12, clamp(tex * 1.7, 0.0, 1.0));   // craters darken a touch

    float limb = pow(clamp(z, 0.0, 1.0), 0.42);                  // bright centre -> soft rim
    vec3 body = vec3(1.45, 0.55, 0.33);                          // warm blood-orange glow
    float ndl = clamp(dot(N, normalize(vec3(-0.35, 0.32, 0.88))), 0.0, 1.0);

    vec3 col = body * detail * (0.45 + 0.6 * limb) * (0.85 + 0.25 * ndl);
    finalColor = vec4(col, 1.0);
}
