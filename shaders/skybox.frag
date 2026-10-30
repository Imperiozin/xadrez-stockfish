#version 330 core
in vec3 direction;
uniform samplerCube environmentMap;
out vec4 fragmentColor;
void main() {
    fragmentColor = vec4(texture(environmentMap, direction).rgb, 1.0);
}
