#include "Scenario.h"
#include "stockfish.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
constexpr float pi = 3.14159265358979323846f;

float randomUnit(std::uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return float(value & 0xffffffu) / float(0x1000000u);
}

// Area shared by two apparent solar discs, including full/annular eclipses.
float overlap(float r, float s, float d) {
    if (d >= r + s) return 0;
    if (d <= std::abs(r - s)) return pi * std::min(r * r, s * s);
    const float a = std::acos(std::clamp((d*d + r*r - s*s) / (2*d*r), -1.f, 1.f));
    const float b = std::acos(std::clamp((d*d + s*s - r*r) / (2*d*s), -1.f, 1.f));
    return r*r*a + s*s*b - .5f * std::sqrt(std::max(0.f,
        (-d+r+s)*(d+r-s)*(d-r+s)*(d+r+s)));
}
}

Scenario Scenario::load(const std::string &file) {
    Scenario scene;
    auto read = [&](const arquivos::path &path) {
        std::ifstream input(path.string());
        if (!input) throw std::runtime_error("Nao foi possivel abrir configuracao: " + path.string());
        std::string line;
        int number = 0;
        while (std::getline(input, line)) {
            ++number;
            line = line.substr(0, line.find('#'));
            std::istringstream values(line);
            std::string key;
            if (!(values >> key)) continue;
            auto vector = [&](Vec3 &v) { values >> v.x >> v.y >> v.z; };
            if (key == "skybox") values >> scene.skybox;
            else if (key == "lightPosition") vector(scene.lightPosition);
            else if (key == "lightColor") vector(scene.lightColor);
            else if (key == "ambientColor") vector(scene.ambientColor);
            else if (key == "fogColor") vector(scene.fogColor);
            else if (key == "lightIntensity") values >> scene.lightIntensity;
            else if (key == "ambientStrength") values >> scene.ambientStrength;
            else if (key == "diffuseStrength") values >> scene.diffuseStrength;
            else if (key == "specularStrength") values >> scene.specularStrength;
            else if (key == "woodSpecularStrength") values >> scene.woodSpecularStrength;
            else if (key == "shininess") values >> scene.shininess;
            else if (key == "fogDensity") values >> scene.fogDensity;
            else if (key == "shadowFov") values >> scene.shadowFov;
            else if (key == "shadowNear") values >> scene.shadowNear;
            else if (key == "shadowFar") values >> scene.shadowFar;
            else if (key == "shadowBias") values >> scene.shadowBias;
            else if (key == "shadowMinBias") values >> scene.shadowMinBias;
            else if (key == "shadowNormalOffset") values >> scene.shadowNormalOffset;
            else if (key == "shadowAutoFit") values >> scene.shadowAutoFit;
            else if (key == "shadowRadius") values >> scene.shadowRadius;
            else if (key == "skyBrightness") values >> scene.skyBrightness;
            else if (key == "secondaryLightPosition") vector(scene.secondaryLightPosition);
            else if (key == "secondaryLightColor") vector(scene.secondaryLightColor);
            else if (key == "secondaryLightIntensity") values >> scene.secondaryLightIntensity;
            else if (key == "sunRadius") values >> scene.sunRadius;
            else if (key == "secondarySunRadius") values >> scene.secondarySunRadius;
            else if (key == "orbitPeriod") values >> scene.orbitPeriod;
            else if (key == "eclipsePeriod") values >> scene.eclipsePeriod;
            else if (key == "lightningInterval") values >> scene.lightningInterval;
            else if (key == "lightningJitter") values >> scene.lightningJitter;
            else if (key == "lightningStrength") values >> scene.lightningStrength;
            else if (key == "animation") {
                std::string mode;
                values >> mode;
                if (mode == "static") scene.animation = Animation::Static;
                else if (mode == "binary") scene.animation = Animation::BinarySuns;
                else if (mode == "storm") scene.animation = Animation::Storm;
                else throw std::runtime_error("Animacao desconhecida em " + path.string() + ":" + std::to_string(number));
            }
            else throw std::runtime_error("Parametro desconhecido em " + path.string() + ":" +
                                          std::to_string(number) + ": " + key);
            std::string extra;
            if (!values || (values >> extra))
                throw std::runtime_error("Valor invalido em " + path.string() + ":" + std::to_string(number));
        }
    };
    // Shared defaults always come first, independent of where the scenario lives.
    read(pastaDoExecutavel() / "assets/scenarios/global.cfg");
    const auto path = pastaDoExecutavel() / file;
    read(path);
    if (scene.skybox.empty() || scene.shadowNear <= 0 || scene.shadowFar <= scene.shadowNear ||
        scene.shadowFov <= 0 || scene.shadowFov >= 179 || scene.lightIntensity < 0 ||
        scene.ambientStrength < 0 || scene.diffuseStrength < 0 || scene.specularStrength < 0 ||
        scene.woodSpecularStrength < 0 || scene.shininess <= 0 || scene.fogDensity < 0 ||
        scene.shadowBias < 0 || scene.shadowMinBias < 0 || scene.shadowNormalOffset < 0 || scene.shadowRadius <= 0 ||
        scene.skyBrightness < 0 || scene.secondaryLightIntensity < 0 ||
        scene.sunRadius <= 0 || scene.sunRadius >= .5f ||
        scene.secondarySunRadius <= 0 || scene.secondarySunRadius >= scene.sunRadius ||
        scene.orbitPeriod <= 0 || scene.eclipsePeriod <= 0 || scene.lightningStrength < 0 ||
        scene.lightningInterval <= 1 || scene.lightningJitter < 0 ||
        scene.lightningJitter >= scene.lightningInterval - 1 ||
        dot(scene.lightPosition, scene.lightPosition) < .001f ||
        scene.lightPosition.x * scene.lightPosition.x + scene.lightPosition.z * scene.lightPosition.z < .001f)
        throw std::runtime_error("Configuracao de cenario invalida: " + path.string());
    scene.lightViewProjection();
    if (scene.animation == Animation::BinarySuns) {
        if (scene.secondaryLightPosition.x * scene.secondaryLightPosition.x +
            scene.secondaryLightPosition.z * scene.secondaryLightPosition.z < .001f ||
            (scene.shadowAutoFit && scene.shadowRadius >= 59.9f))
            throw std::runtime_error("Volume de sombras ou segunda luz invalido: " + path.string());
        scene.lightViewProjection(true);
    }
    return scene;
}

std::string Scenario::fileForDifficulty(int level) {
    const int index = (std::clamp(level, -20, 20) + 25) / 10 + 1;
    return "assets/scenarios/" + std::to_string(index) + ".cfg";
}

Scenario Scenario::sample(double seconds, std::uint32_t seed) const {
    Scenario frame = *this;
    seconds = std::max(0.0, seconds);
    if (animation == Animation::BinarySuns) {
        const float orbit = float(std::fmod(seconds, double(orbitPeriod)) / orbitPeriod) * 2*pi;
        const float phase = float(std::fmod(seconds, double(eclipsePeriod)) / eclipsePeriod) * 2*pi + .9f;
        const float azimuth = std::atan2(lightPosition.x, lightPosition.z) + orbit;
        const float initialElevation = std::atan2(lightPosition.y,
            std::sqrt(lightPosition.x*lightPosition.x + lightPosition.z*lightPosition.z));
        const float elevation = std::clamp(initialElevation, .2f, 1.2f) + .10f * std::sin(orbit);
        auto direction = [](float az, float el) {
            return Vec3{std::sin(az)*std::cos(el), std::sin(el), std::cos(az)*std::cos(el)};
        };
        frame.lightPosition = direction(azimuth, elevation) * 120;
        // The smaller red sun alternates between foreground and background.
        frame.secondaryLightPosition = direction(azimuth + .28f*std::sin(phase),
            elevation + .018f*std::sin(2*phase)) * (100 + 40*std::cos(phase));
        const float separation = std::acos(std::clamp(dot(normalized(frame.lightPosition),
            normalized(frame.secondaryLightPosition)), -1.f, 1.f));
        const float area = overlap(sunRadius, secondarySunRadius, separation);
        if (dot(frame.lightPosition, frame.lightPosition) >
            dot(frame.secondaryLightPosition, frame.secondaryLightPosition))
            frame.lightIntensity *= std::clamp(1 - area/(pi*sunRadius*sunRadius), 0.f, 1.f);
        else
            frame.secondaryLightIntensity *= std::clamp(1 - area/(pi*secondarySunRadius*secondarySunRadius), 0.f, 1.f);
        const float energy = (frame.lightIntensity + frame.secondaryLightIntensity) /
            std::max(.001f, lightIntensity + secondaryLightIntensity);
        frame.ambientStrength *= .35f + .65f*energy;
        frame.skyBrightness *= .45f + .55f*energy;
    } else if (animation == Animation::Storm) {
        // Jittered slots give bounded, irregular intervals without a per-frame lottery.
        const auto slot = std::uint32_t(seconds / lightningInterval);
        const auto event = slot * 747796405u + seed;
        const double start = slot * double(lightningInterval) + 1 + randomUnit(event)*lightningJitter;
        const float age = float(seconds - start);
        frame.lightningSeed = randomUnit(event + 1)*1000;
        const float azimuth = randomUnit(event + 2)*2*pi;
        frame.lightningDirection = normalized(Vec3{std::sin(azimuth),
            .45f + .45f*randomUnit(event + 3), std::cos(azimuth)});
        if (age >= 0 && age < .65f) {
            const float second = .12f + .10f*randomUnit(event + 4);
            const auto pulse = [age](float start, float amplitude) {
                return age < start ? 0.f : amplitude * std::exp(-(age-start)*28);
            };
            frame.lightningFlash = pulse(0, 1) + pulse(second, .7f) +
                pulse(second + .14f, .3f + .3f*randomUnit(event + 5));
        }
        frame.secondaryLightPosition = frame.lightningDirection * 110;
        frame.secondaryLightColor = {.68f, .80f, 1};
        frame.secondaryLightIntensity = lightningStrength * frame.lightningFlash;
        frame.ambientStrength += .20f*frame.lightningFlash;
        frame.fogColor = fogColor + Vec3{.16f, .20f, .28f}*frame.lightningFlash;
    }
    return frame;
}

Mat4 Scenario::lightViewProjection(bool secondary) const {
    const Vec3 position = secondary ? secondaryLightPosition : lightPosition;
    float nearPlane = shadowNear, farPlane = shadowFar, fov = shadowFov;
    if (shadowAutoFit) {
        // Enclose the scene in a sphere centered on the board, including legs
        // and shadow receivers. Moving the light cannot clip that volume.
        const float distance = std::sqrt(dot(position, position));
        if (distance <= shadowRadius + .1f)
            throw std::runtime_error("Luz dentro do volume das sombras: afaste a luz ou desative shadowAutoFit");
        nearPlane = distance - shadowRadius;
        farPlane = distance + shadowRadius;
        fov = 2.f * std::asin(shadowRadius / distance) * 180.f / pi;
    }
    Mat4 projection;
    const float f = 1.f / std::tan(fov * pi / 360.f);
    projection.v[0] = projection.v[5] = f;
    projection.v[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    projection.v[11] = -1;
    projection.v[14] = -2 * farPlane * nearPlane / (farPlane - nearPlane);
    return projection * lookAt(position);
}
