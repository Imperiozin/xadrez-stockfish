#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;

uniform vec3 color;
uniform vec3 eye;

out vec4 pixel;

const vec3 LIGHT_POSITION = vec3(-4.0, 10.0, 5.0);
const float AMBIENT_STRENGTH = 0.42;
const float DIFFUSE_STRENGTH = 0.65;
const float SPECULAR_STRENGTH = 0.32;
const float SHININESS = 48.0;
const vec3 FOG_COLOR = vec3(0.035, 0.055, 0.075);
const float FOG_DENSITY = 0.00055;

void main() {
    vec3 normal = normalize(worldNormal);
    vec3 lightDirection = normalize(LIGHT_POSITION - worldPosition);
    vec3 eyeOffset = eye - worldPosition;
    vec3 viewDirection = normalize(eyeOffset);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);

    // Iluminacao ambiente, difusa e especular (Blinn-Phong).
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float specular = pow(max(dot(normal, halfwayDirection), 0.0), SHININESS);
    vec3 litColor = color * (AMBIENT_STRENGTH + DIFFUSE_STRENGTH * diffuse)
                  + vec3(SPECULAR_STRENGTH) * specular;

    // A nevoa aumenta com o quadrado da distancia ate a camera.
    float distanceSquared = dot(eyeOffset, eyeOffset);
    float fog = 1.0 - exp(-FOG_DENSITY * distanceSquared);
    pixel = vec4(mix(litColor, FOG_COLOR, fog), 1.0);
}
