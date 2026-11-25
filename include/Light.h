#pragma once
#include "Math3D.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

// Luzes locais usam unidades da cena; os ângulos do cone são dados em graus.
struct Light {
    enum class Type { Point, Spot };
    static constexpr std::size_t maxCount = 16; // Mesmo limite de scene.frag.

    Type type = Type::Point;
    Vec3 position{}, direction{0, -1, 0}, color{1, 1, 1};
    float intensity = 1, range = 10;
    float innerAngle = 20, outerAngle = 30;
    float flickerAmount = 0, flickerSpeed = 8, flickerPhase = 0;
    bool enabled = true;

    static Light point(Vec3 position, Vec3 color = {1, 1, 1},
                       float intensity = 10, float range = 12) {
        Light light;
        light.position = position;
        light.color = color;
        light.intensity = intensity;
        light.range = range;
        return light;
    }

    static Light spot(Vec3 position, Vec3 direction, Vec3 color = {1, 1, 1},
                      float intensity = 20, float range = 15,
                      float innerAngle = 15, float outerAngle = 25) {
        Light light = point(position, color, intensity, range);
        light.type = Type::Spot;
        light.direction = direction;
        light.innerAngle = innerAngle;
        light.outerAngle = outerAngle;
        return light;
    }

    static Light candle(Vec3 position, float intensity = 3, float range = 6) {
        Light light = point(position, {1, .55f, .18f}, intensity, range);
        light.flickerAmount = .25f;
        return light;
    }

    void validate() const {
        auto finite = [](Vec3 v) {
            return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
        };
        if ((type != Type::Point && type != Type::Spot) || !finite(position) || !finite(color) ||
            color.x < 0 || color.y < 0 || color.z < 0 ||
            !std::isfinite(intensity) || intensity < 0 || !std::isfinite(range) || range <= 0 ||
            !std::isfinite(flickerAmount) || flickerAmount < 0 || flickerAmount > 1 ||
            !std::isfinite(flickerSpeed) || flickerSpeed <= 0 || !std::isfinite(flickerPhase))
            throw std::runtime_error("Configuração de luz local inválida: verifique posição, cor, intensidade, alcance e oscilação");
        if (type == Type::Spot &&
            (!finite(direction) || !std::isfinite(dot(direction, direction)) ||
             dot(direction, direction) < 1e-12f ||
             !std::isfinite(innerAngle) || !std::isfinite(outerAngle) ||
             innerAngle < 0 || outerAngle <= innerAngle || outerAngle >= 90))
            throw std::runtime_error("Cone de luz inválido: use direção não nula e 0 <= ângulo interno < externo < 90 graus");
    }

    float flickerMultiplier(double seconds) const {
        if (flickerAmount == 0) return 1;
        if (!std::isfinite(seconds))
            throw std::runtime_error("Tempo inválido para a oscilação da luz");
        // O hash escolhe novos alvos; a interpolação suave evita saltos entre eles.
        const float phase = flickerPhase + position.x * 1.37f + position.y * 2.11f + position.z * .73f;
        std::uint32_t seed;
        static_assert(sizeof(seed) == sizeof(phase));
        std::memcpy(&seed, &phase, sizeof(seed));
        auto random = [](std::uint32_t value) {
            value ^= value >> 16;
            value *= 0x7feb352du;
            value ^= value >> 15;
            value *= 0x846ca68bu;
            value ^= value >> 16;
            return 2.f * float(value & 0xffffffu) / float(0xffffffu) - 1.f;
        };
        auto noise = [&](double time, std::uint32_t salt) {
            const double interval = std::floor(time);
            const auto index = std::uint32_t(std::fmod(interval, 4294967296.0));
            const float fraction = float(time - interval);
            const float blend = fraction * fraction * fraction * (fraction * (fraction * 6 - 15) + 10);
            const float a = random(index ^ seed ^ salt), b = random((index + 1u) ^ seed ^ salt);
            return a + (b - a) * blend;
        };
        const double t = std::fmod(std::max(0.0, seconds), 4294967296.0) * flickerSpeed;
        const float wave = .7f * noise(t * .45, 0x9e3779b9u) + .3f * noise(t * 1.7, 0x85ebca6bu);
        return 1 + flickerAmount * std::clamp(wave, -1.f, 1.f);
    }

    // A transformação afeta posição e direção; o alcance permanece em unidades do mundo.
    Light sample(double seconds, const Mat4 &parent = Mat4::identity()) const {
        validate();
        Light frame = *this;
        frame.position = transformPoint(parent, position);
        if (type == Type::Spot) {
            frame.direction = transformDirection(parent, direction);
            frame.validate();
            frame.direction = normalized(frame.direction);
        }
        frame.intensity *= flickerMultiplier(seconds);
        frame.validate();
        return frame;
    }
};
