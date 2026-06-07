#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
uniform mat4 mvp;
uniform mat4 matModel;
uniform float uTime;
out vec3 fragWorld;
out vec2 fragUV;

float w(vec2 p) { return sin(p.x * 0.8 + uTime * 1.2) * cos(p.y * 0.7 - uTime * 1.0); }

void main() {
    vec3 pos = vertexPosition;
    vec3 wxz = vec3(matModel * vec4(pos, 1.0)).xyz;
    float h = 0.035 * w(wxz.xz) + 0.018 * w(wxz.xz * 2.3 + 7.0);   // small surface waves
    pos.y += h;
    vec4 wp = matModel * vec4(pos, 1.0);
    fragWorld = wp.xyz;
    fragUV = vertexTexCoord;
    gl_Position = mvp * vec4(pos, 1.0);
}
