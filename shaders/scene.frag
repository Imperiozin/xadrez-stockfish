#version 330 core

in vec3 worldPosition;
in vec2 modelUv;
uniform bool modelTextureEnabled;
uniform sampler2D modelTexture;
uniform bool pageTextureEnabled;
uniform sampler2D pageTexture;
uniform sampler2D pageSurface;
uniform float pageHeight;
in vec3 localPosition;
in vec3 localNormal;
uniform bool ruledPaperEnabled;
uniform vec4 rulingMapping;
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
uniform float shadowNormalOffset;
uniform vec3 secondaryLightPosition;
uniform vec3 secondaryLightColor;
uniform float secondaryLightIntensity;
uniform mat4 secondaryLightViewProjection;
uniform sampler2DShadow secondaryShadowMap;
uniform bool secondaryShadowsEnabled;

const int MAX_LOCAL_LIGHTS = 16;
struct LocalLight {
    int type; // 0: pontual; 1: cone.
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float range;
    float innerCos;
    float outerCos;
};
uniform int localLightCount;
uniform LocalLight localLights[MAX_LOCAL_LIGHTS];
uniform vec3 emissionColor;
uniform bool texturedEmission;

// Uma única resposta de material para todas as fontes (Blinn-Phong).
vec3 directLighting(vec3 normal, vec3 viewDirection, vec3 lightDirection,
                    vec3 materialColor, vec3 radiance) {
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 halfway = lightDirection + viewDirection;
    halfway *= inversesqrt(max(dot(halfway, halfway), 0.00000001));
    float specular = diffuse > 0.0 ? pow(max(dot(normal, halfway), 0.0), shininess) : 0.0;
    return radiance * (materialColor * diffuseStrength * diffuse
        + vec3(woodMaterial ? woodSpecularStrength : specularStrength) * specular);
}

vec3 localLighting(LocalLight light, vec3 normal, vec3 viewDirection, vec3 materialColor) {
    vec3 offset = light.position - worldPosition;
    float distanceSquared = dot(offset, offset);
    float relativeDistance = sqrt(distanceSquared) / light.range;
    if (relativeDistance >= 1.0 || light.intensity <= 0.0) return vec3(0.0);
    vec3 direction = offset * inversesqrt(max(distanceSquared, 0.00000001));

    // Queda com a distância, sem singularidade na origem nem corte brusco no alcance.
    float fade = 1.0 - relativeDistance * relativeDistance;
    float attenuation = fade * fade / (1.0 + distanceSquared);
    if (light.type == 1) {
        float angleCos = dot(-direction, light.direction);
        // Cones muito estreitos podem ter cossenos iguais na precisão da GPU.
        attenuation *= light.innerCos > light.outerCos
            ? smoothstep(light.outerCos, light.innerCos, angleCos)
            : step(light.outerCos, angleCos);
    }
    return directLighting(normal, viewDirection, direction, materialColor,
                          light.color * light.intensity * attenuation);
}

float shadowAmount(vec3 normal, vec3 lightDirection, vec4 lightSpace,
                   sampler2DShadow depthMap, bool enabled) {
    if (!enabled || lightSpace.w <= 0.0) return 0.0;
    vec3 projected = lightSpace.xyz / lightSpace.w;
    projected = projected * 0.5 + 0.5;
    if (projected.z <= 0.0 || projected.z >= 1.0 ||
        any(lessThan(projected.xy, vec2(0.0))) ||
        any(greaterThan(projected.xy, vec2(1.0)))) return 0.0;

    // Stable normal bias; no inversion of screen-space derivatives at silhouettes.
    float bias = max(shadowBias * (1.0 - max(dot(normal, lightDirection), 0.0)), shadowMinBias);
    vec2 texel = 1.0 / vec2(textureSize(depthMap, 0));
    float shadow = 0.0;
    // PCF 3 × 3 suaviza a transição entre luz e sombra.
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(x, y) * texel;
            // Hardware compares depths before filtering, smoothing shadow edges.
            shadow += 1.0 - texture(depthMap, vec3(projected.xy + offset, projected.z - bias));
        }
    }
    return shadow / 9.0;
}

// Cobertura filtrada de um traço de dois pixels da textura original da UI.
float rulingCoverage(float distanceToLine, float footprint) {
    float filterWidth = max(footprint, 1.0);
    return clamp((2.0 + filterWidth) * 0.5 - distanceToLine,
                 0.0, min(2.0, filterWidth)) / filterWidth;
}

void main() {
    vec3 normal = normalize(worldNormal);
    vec3 lightDirection = normalize(lightPosition - worldPosition);
    vec3 eyeOffset = eye - worldPosition;
    vec3 viewDirection = normalize(eyeOffset);
    float visibility = 1.0 - shadowAmount(normal, lightDirection, lightSpacePosition, shadowMap, shadowsEnabled);
    vec3 materialColor = color;
    if (modelTextureEnabled) materialColor *= texture(modelTexture, modelUv).rgb;
    if (ruledPaperEnabled) {
        // Coordenadas locais: as pautas acompanham o bloco e a curva do papel.
        vec2 paperPixel = (localPosition.xz - rulingMapping.xy) * rulingMapping.zw;
        vec2 footprint = fwidth(paperPixel);
        float topFace = smoothstep(0.15, 0.65, normalize(localNormal).y);
        float sideMargin = step(18.0, paperPixel.x) * step(paperPixel.x, 750.0);
        float distanceToRule = abs(mod(paperPixel.y - 168.0 + 38.0, 76.0) - 38.0);
        float rule = rulingCoverage(distanceToRule, footprint.y) * sideMargin * topFace;
        materialColor = mix(materialColor, vec3(96.0, 131.0, 160.0) / 255.0, rule * (95.0 / 255.0));
        float margin = rulingCoverage(abs(paperPixel.x - 82.0), footprint.x) * topFace;
        materialColor = mix(materialColor, vec3(187.0, 87.0, 77.0) / 255.0, margin * (75.0 / 255.0));
    }
    if (pageTextureEnabled) {
        vec2 uv = (localPosition.xz - rulingMapping.xy) * rulingMapping.zw / vec2(textureSize(pageTexture, 0));
        vec4 ink = texture(pageTexture, clamp(vec2(uv.x, 1.0 - uv.y), vec2(0.0), vec2(1.0)));
        // A altura superior em cada ponto impede a tinta nas folhas sob a curva.
        float surfaceHeight = texture(pageSurface, uv).r;
        if (all(greaterThanEqual(uv, vec2(0.0))) && all(lessThanEqual(uv, vec2(1.0))) &&
            localPosition.y >= max(pageHeight, surfaceHeight) - 0.0005 && normalize(localNormal).y > 0.15) {
            // O framebuffer do ImGui contém cor pré-multiplicada pelo alfa.
            materialColor = materialColor * (1.0 - ink.a) + ink.rgb;
        }
    }
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
        + directLighting(normal, viewDirection, lightDirection, materialColor,
                         visibility * lightColor * lightIntensity);

    if (secondaryLightIntensity > 0.0) {
        vec3 secondaryDirection = normalize(secondaryLightPosition - worldPosition);
        vec4 secondarySpace = secondaryLightViewProjection * vec4(worldPosition + normal * shadowNormalOffset, 1.0);
        float secondaryVisibility = 1.0 - shadowAmount(normal, secondaryDirection,
            secondarySpace, secondaryShadowMap, secondaryShadowsEnabled);
        litColor += directLighting(normal, viewDirection, secondaryDirection, materialColor,
                                  secondaryVisibility * secondaryLightColor * secondaryLightIntensity);
    }

    for (int i = 0; i < localLightCount; ++i)
        litColor += localLighting(localLights[i], normal, viewDirection, materialColor);
    litColor += emissionColor;

    // A névoa aumenta com o quadrado da distância até a câmera.
    float distanceSquared = dot(eyeOffset, eyeOffset);
    float fog = 1.0 - exp(-fogDensity * distanceSquared);
    // Nas chamas, o preto da textura permanece transparente na composição aditiva.
    if (texturedEmission) {
        vec4 flame = texture(modelTexture, modelUv);
        pixel = vec4(flame.rgb * color * emissionColor * (1.0 - fog), flame.a);
    } else {
        pixel = vec4(mix(litColor, fogColor, fog), 1.0);
    }
}
