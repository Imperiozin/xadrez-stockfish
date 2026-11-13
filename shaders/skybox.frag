#version 330 core
in vec3 direction;
uniform samplerCube environmentMap;
uniform float skyBrightness;
uniform vec3 fogColor;
uniform float fogDensity;
uniform bool binarySuns;
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform vec3 secondaryLightPosition;
uniform vec3 secondaryLightColor;
uniform float sunRadius;
uniform float secondarySunRadius;
uniform float lightningFlash;
uniform float lightningSeed;
uniform vec3 lightningDirection;
out vec4 fragmentColor;

vec3 sun(vec3 background, vec3 ray, vec3 position, vec3 color, float radius) {
    float angle = acos(clamp(dot(ray, normalize(position)), -1.0, 1.0));
    float edge = max(fwidth(angle), .0005);
    float disc = 1.0 - smoothstep(radius - edge, radius + edge, angle);
    vec3 halo = color * .22 * exp(-angle * angle / (radius * radius * 5.0));
    float radial = clamp(angle / radius, 0.0, 1.0);
    float limb = .75 + .65 * sqrt(max(0.0, 1.0 - radial * radial));
    return mix(background + halo, color * limb, disc);
}

float hash(float n) { return fract(sin(n * 127.1 + lightningSeed) * 43758.5453); }
float segmentDistance(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    return length(p - a - ab * clamp(dot(p - a, ab) / dot(ab, ab), 0.0, 1.0));
}

void main() {
    vec3 ray = normalize(direction);
    vec3 sky = texture(environmentMap, ray).rgb * skyBrightness;
    float haze = clamp(fogDensity * 500.0, 0.0, .35) * exp(-abs(ray.y) * 4.0);
    sky = mix(sky, fogColor, haze);
    if (binarySuns) {
        // Paint the farther disc first so the nearer one really occludes it.
        if (dot(lightPosition, lightPosition) > dot(secondaryLightPosition, secondaryLightPosition)) {
            sky = sun(sky, ray, lightPosition, lightColor, sunRadius);
            sky = sun(sky, ray, secondaryLightPosition, secondaryLightColor, secondarySunRadius);
        } else {
            sky = sun(sky, ray, secondaryLightPosition, secondaryLightColor, secondarySunRadius);
            sky = sun(sky, ray, lightPosition, lightColor, sunRadius);
        }
    }
    if (lightningFlash > .001) {
        vec3 axis = normalize(lightningDirection);
        vec3 right = normalize(cross(axis, vec3(0, 1, 0)));
        vec3 up = cross(right, axis);
        float facing = dot(ray, axis);
        // Cloud flash is directional; the jagged bolt changes location and shape each event.
        sky += vec3(.25, .34, .52) * lightningFlash * (.12 + pow(max(facing, 0.0), 8.0));
        if (facing > .45) {
            vec2 p = vec2(dot(ray, right), dot(ray, up)) / facing;
            float nearest = 10.0;
            vec2 a = vec2(0, .34);
            for (int i = 1; i <= 12; ++i) {
                float t = float(i) / 12.0;
                vec2 b = vec2((hash(float(i)) - .5) * .11, .34 - t * .68);
                nearest = min(nearest, segmentDistance(p, a, b));
                if (i == 5 || i == 8) {
                    vec2 branch = b + vec2((hash(float(i) + 20.0) - .5) * .28, -.13);
                    nearest = min(nearest, segmentDistance(p, b, branch));
                }
                a = b;
            }
            float width = max(fwidth(p.x), .001);
            float bolt = 1.0 - smoothstep(.001, .001 + width, nearest);
            float glow = exp(-nearest * 100.0) * .5;
            sky += vec3(.68, .82, 1.0) * (bolt * 2.5 + glow) * lightningFlash;
        }
    }
    fragmentColor = vec4(sky, 1.0);
}
