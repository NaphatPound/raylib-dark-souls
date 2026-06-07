#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragWorld;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 uLightDir;     // direction TO the moon (normalized)
uniform vec3 uLightColor;   // moonlight colour * energy
uniform vec3 uAmbient;      // ambient term
uniform vec3 uViewPos;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform float uEmissive;    // 0 = lit, 1 = emit albedo unshaded (crystals/moon)

out vec4 finalColor;

void main() {
    vec4 tex = texture(texture0, fragTexCoord);
    vec3 albedo = tex.rgb * colDiffuse.rgb * fragColor.rgb;
    vec3 N = normalize(fragNormal);
    float ndl = max(dot(N, normalize(uLightDir)), 0.0);
    // soft wrap so back faces aren't pure black under the single moon
    float wrap = 0.5 + 0.5 * dot(N, normalize(uLightDir));
    vec3 lit = albedo * (uAmbient + uLightColor * (ndl * 0.8 + wrap * 0.3));
    vec3 shaded = mix(lit, albedo * 1.4, uEmissive);

    float dist = length(uViewPos - fragWorld);
    float f = clamp(1.0 - exp(-uFogDensity * dist), 0.0, 1.0);
    vec3 col = mix(shaded, uFogColor, f * (1.0 - uEmissive));

    // lit geometry (arena + characters) is opaque; ignore the glTF base-color
    // alpha (Godot exported 0.8) and the texture alpha so nothing turns translucent.
    finalColor = vec4(col, 1.0);
}
