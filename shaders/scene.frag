#version 330 core

in vec3 worldPosition;
in vec3 localPosition;
uniform bool woodMaterial;
in vec3 worldNormal;
in vec4 lightSpacePosition;

uniform sampler2DShadow shadowMap;
uniform bool shadowsEnabled;
uniform vec3 lightPosition;

uniform vec3 color;
uniform vec3 eye;

out vec4 pixel;

uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 ambientColor;
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float woodSpecularStrength;
uniform float shininess;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float shadowBias;
uniform float shadowMinBias;

float shadowAmount(vec3 normal, vec3 lightDirection) {
    if (!shadowsEnabled || lightSpacePosition.w <= 0.0) return 0.0;
    vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
    projected = projected * 0.5 + 0.5;
    if (projected.z <= 0.0 || projected.z >= 1.0 ||
        any(lessThan(projected.xy, vec2(0.0))) ||
        any(greaterThan(projected.xy, vec2(1.0)))) return 0.0;

    // Stable normal bias; no inversion of screen-space derivatives at silhouettes.
    float bias = max(shadowBias * (1.0 - max(dot(normal, lightDirection), 0.0)), shadowMinBias);
    vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;
    // PCF 3 × 3 suaviza a transição entre luz e sombra.
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(x, y) * texel;
            // Hardware compares depths before filtering, smoothing shadow edges.
            shadow += 1.0 - texture(shadowMap, vec3(projected.xy + offset, projected.z - bias));
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
    float specular = diffuse > 0.0 ? pow(max(dot(normal, halfwayDirection), 0.0), shininess) : 0.0;
    float visibility = 1.0 - shadowAmount(normal, lightDirection);
    vec3 materialColor = color;
    if (woodMaterial) {
        float warp = sin(localPosition.z * 9.0 + localPosition.x * 5.0) * .65
                   + sin(localPosition.z * 23.0) * .18;
        float grain = sin(localPosition.x * 180.0 + warp * 3.0);
        float fineGrain = sin(localPosition.x * 420.0 + warp * 5.0);
        float filterWidth = fwidth(localPosition.x);
        materialColor *= .92 + .07 * grain * exp(-filterWidth * 180.0)
                              + .025 * fineGrain * exp(-filterWidth * 420.0);
    }
    vec3 litColor = materialColor * ambientColor * ambientStrength
                  + visibility * lightColor * lightIntensity
                  * (materialColor * diffuseStrength * diffuse
                  + vec3(woodMaterial ? woodSpecularStrength : specularStrength) * specular);

    // A nevoa aumenta com o quadrado da distancia ate a camera.
    float distanceSquared = dot(eyeOffset, eyeOffset);
    float fog = 1.0 - exp(-fogDensity * distanceSquared);
    pixel = vec4(mix(litColor, fogColor, fog), 1.0);
}
