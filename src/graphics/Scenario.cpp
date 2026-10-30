#include "Scenario.h"
#include "stockfish.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

Scenario Scenario::load(const std::string &file) {
    const auto path = pastaDoExecutavel() / file;
    std::ifstream input(path.string());
    if (!input) throw std::runtime_error("Nao foi possivel abrir cenario: " + path.string());
    Scenario scene;
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
        else throw std::runtime_error("Parametro de cenario desconhecido: " + key);
        std::string extra;
        if (!values || (values >> extra))
            throw std::runtime_error("Valor invalido em " + path.string() + ":" + std::to_string(number));
    }
    if (scene.skybox.empty() || scene.shadowNear <= 0 || scene.shadowFar <= scene.shadowNear ||
        scene.shadowFov <= 0 || scene.shadowFov >= 179 || scene.lightIntensity < 0 ||
        scene.ambientStrength < 0 || scene.diffuseStrength < 0 || scene.specularStrength < 0 ||
        scene.woodSpecularStrength < 0 || scene.shininess <= 0 || scene.fogDensity < 0 ||
        scene.shadowBias < 0 || scene.shadowMinBias < 0 || scene.shadowNormalOffset < 0 || scene.shadowRadius <= 0 ||
        dot(scene.lightPosition, scene.lightPosition) < .001f ||
        scene.lightPosition.x * scene.lightPosition.x + scene.lightPosition.z * scene.lightPosition.z < .001f)
        throw std::runtime_error("Configuracao de cenario invalida: " + path.string());
    return scene;
}

Mat4 Scenario::lightViewProjection() const {
    float nearPlane = shadowNear, farPlane = shadowFar, fov = shadowFov;
    if (shadowAutoFit) {
        // Enclose the scene in a sphere centered on the board, including legs
        // and shadow receivers. Moving the light cannot clip that volume.
        const float distance = std::sqrt(dot(lightPosition, lightPosition));
        if (distance <= shadowRadius + .1f)
            throw std::runtime_error("Luz dentro do volume das sombras: afaste a luz ou desative shadowAutoFit");
        nearPlane = distance - shadowRadius;
        farPlane = distance + shadowRadius;
        fov = 2.f * std::asin(shadowRadius / distance) * 180.f / 3.14159265f;
    }
    Mat4 projection;
    const float f = 1.f / std::tan(fov * 3.14159265f / 360.f);
    projection.v[0] = projection.v[5] = f;
    projection.v[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    projection.v[11] = -1;
    projection.v[14] = -2 * farPlane * nearPlane / (farPlane - nearPlane);
    return projection * lookAt(lightPosition);
}
