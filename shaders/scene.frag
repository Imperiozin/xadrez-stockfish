#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;
in vec4 lightSpacePosition;

uniform sampler2D shadowMap;
uniform bool shadowsEnabled;
uniform vec3 lightPosition;

uniform vec3 color;
uniform vec3 eye;

out vec4 pixel;

const float AMBIENT_STRENGTH = 0.42;
const float DIFFUSE_STRENGTH = 0.65;
const float SPECULAR_STRENGTH = 0.32;
const float SHININESS = 48.0;
const vec3 FOG_COLOR = vec3(0.035, 0.055, 0.075);
const float FOG_DENSITY = 0.00055;

float shadowAmount(vec3 normal, vec3 lightDirection) {
    if (!shadowsEnabled || lightSpacePosition.w <= 0.0) return 0.0;
    vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
    projected = projected * 0.5 + 0.5;
    if (projected.z <= 0.0 || projected.z >= 1.0 ||
        any(lessThan(projected.xy, vec2(0.0))) ||
        any(greaterThan(projected.xy, vec2(1.0)))) return 0.0;

    // Bias dependente da inclinação evita auto-sombreamento (shadow acne).
    float bias = max(0.0008 * (1.0 - dot(normal, lightDirection)), 0.00015);
    vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;
    // PCF 3 × 3 suaviza a transição entre luz e sombra.
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float depth = texture(shadowMap, projected.xy + vec2(x, y) * texel).r;
            shadow += projected.z - bias > depth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

void main() {
    vec3 normal = normalize(worldNormal);
    vec3 lightDirection = normalize(lightPosition - worldPosition);
    vec3 eyeOffset = eye - worldPosition;
    vec3 viewDirection = normalize(eyeOffset);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);

    // Iluminacao ambiente, difusa e especular (Blinn-Phong).
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float specular = pow(max(dot(normal, halfwayDirection), 0.0), SHININESS);
    float visibility = 1.0 - shadowAmount(normal, lightDirection);
    vec3 litColor = color * AMBIENT_STRENGTH
                  + visibility * (color * DIFFUSE_STRENGTH * diffuse
                  + vec3(SPECULAR_STRENGTH) * specular);

    // A nevoa aumenta com o quadrado da distancia ate a camera.
    float distanceSquared = dot(eyeOffset, eyeOffset);
    float fog = 1.0 - exp(-FOG_DENSITY * distanceSquared);
    pixel = vec4(mix(litColor, FOG_COLOR, fog), 1.0);
}
