#version 330
in vec3 vertexPosition;
uniform mat4 mvp;
out vec3 vDir;
void main() {
    // The dome is only translated/scaled to the camera (never rotated), so the
    // unit-sphere local position doubles as the world view direction.
    vDir = vertexPosition;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
