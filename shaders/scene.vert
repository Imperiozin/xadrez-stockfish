#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model;
uniform mat4 viewProjection;
uniform mat4 lightViewProjection;
uniform float shadowNormalOffset;

out vec3 worldPosition;
out vec3 localPosition;
out vec3 worldNormal;
out vec4 lightSpacePosition;

void main() {
    vec4 transformedPosition = model * vec4(position, 1.0);
    mat3 normalMatrix = mat3(transpose(inverse(model)));

    vec3 surfaceNormal = normalize(normalMatrix * normal);
    // Move only the shadow lookup, keeping the visible geometry unchanged.
    lightSpacePosition = lightViewProjection * vec4(
        transformedPosition.xyz + surfaceNormal * shadowNormalOffset, 1.0);
    worldPosition = transformedPosition.xyz;
    localPosition = position;
    worldNormal = normalMatrix * normal;
    gl_Position = viewProjection * transformedPosition;
}
