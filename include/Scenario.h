#pragma once
#include "Math3D.h"
#include <cstdint>
#include <string>

struct Scenario {
    enum class Animation { Static, BinarySuns, Storm };
    std::string skybox;
    std::string pet = "Fox";
    Vec3 lightPosition{}, lightColor{}, ambientColor{}, fogColor{};
    float lightIntensity = 1, ambientStrength = .42f, diffuseStrength = .65f;
    float specularStrength = .32f, woodSpecularStrength = .10f, shininess = 48;
    float fogDensity = 0, shadowFov = 50, shadowNear = 1, shadowFar = 150;
    float shadowBias = .0008f, shadowMinBias = .00015f;
    float shadowNormalOffset = .012f;
    bool shadowAutoFit = true;
    float shadowRadius = 20.f;
    float skyBrightness = 1;
    Animation animation = Animation::Static;
    Vec3 secondaryLightPosition{60, 35, 80}, secondaryLightColor{1, .12f, .035f};
    float secondaryLightIntensity = 0;
    // Angular radii in radians; both lights follow the same visible solar discs.
    float sunRadius = .065f, secondarySunRadius = .035f;
    float orbitPeriod = 240, eclipsePeriod = 70;
    float lightningInterval = 12, lightningJitter = 4, lightningStrength = 2.4f;
    // Sampled animation state, never read from configuration files.
    float lightningFlash = 0, lightningSeed = 0;
    Vec3 lightningDirection{0, .6f, 1};

    static Scenario load(const std::string &file);
    static std::string fileForDifficulty(int level);
    Scenario sample(double seconds, std::uint32_t seed = 0) const;
    Mat4 lightViewProjection(bool secondary = false) const;
};
