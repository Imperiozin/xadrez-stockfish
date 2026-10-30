#version 330 core
uniform mat4 viewProjection;
out vec3 direction;
void main() {
    // Fullscreen triangle: sample the cubemap using a world-space camera ray.
    vec2 corners[3] = vec2[](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));
    vec2 position = corners[gl_VertexID];
    direction = (viewProjection * vec4(position, 1.0, 0.0)).xyz;
    gl_Position = vec4(position, 1.0, 1.0);
}
