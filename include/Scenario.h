#pragma once
#include "Math.h"
#include <string>

struct Scenario {
    std::string skybox;
    Vec3 lightPosition{}, lightColor{}, ambientColor{}, fogColor{};
    float lightIntensity = 1, ambientStrength = .42f, diffuseStrength = .65f;
    float specularStrength = .32f, woodSpecularStrength = .10f, shininess = 48;
    float fogDensity = 0, shadowFov = 50, shadowNear = 1, shadowFar = 150;
    float shadowBias = .0008f, shadowMinBias = .00015f;
    float shadowNormalOffset = .012f;
    bool shadowAutoFit = true;
    float shadowRadius = 20.f;

    static Scenario load(const std::string &file);
    Mat4 lightViewProjection() const;
};
