#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model;
uniform mat4 viewProjection;

out vec3 worldPosition;
out vec3 worldNormal;

void main() {
    vec4 transformedPosition = model * vec4(position, 1.0);
    mat3 normalMatrix = mat3(transpose(inverse(model)));

    worldPosition = transformedPosition.xyz;
    worldNormal = normalMatrix * normal;
    gl_Position = viewProjection * transformedPosition;
}
