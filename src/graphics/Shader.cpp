#include "Shader.h"
#include "stockfish.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
std::string readShader(const arquivos::path &path) {
    std::ifstream file(path.string());
    if (!file) {
        throw std::runtime_error("Não foi possível abrir o shader: " + path.string());
    }
    std::ostringstream source;
    source << file.rdbuf();
    if (file.bad()) {
        throw std::runtime_error("Falha ao ler o shader: " + path.string());
    }
    return source.str();
}

GLuint compile(GLenum type, const arquivos::path &path) {
    const std::string source = readShader(path);
    const char *sourceText = source.c_str();
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &sourceText, nullptr);
    glCompileShader(id);
    GLint ok;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        glDeleteShader(id);
        throw std::runtime_error("Falha ao compilar " + path.string() + ":\n" + log);
    }
    return id;
}
} // namespace

Shader::Shader(const char *vertexFile, const char *fragmentFile) {
    const auto shaderDirectory = pastaDoExecutavel() / "shaders";
    GLuint vs = compile(GL_VERTEX_SHADER, shaderDirectory / vertexFile), fs = 0;
    try {
        fs = compile(GL_FRAGMENT_SHADER, shaderDirectory / fragmentFile);
    } catch (...) {
        glDeleteShader(vs);
        throw;
    }
    id = glCreateProgram();
    glAttachShader(id, vs);
    glAttachShader(id, fs);
    glLinkProgram(id);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok;
    glGetProgramiv(id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(id, sizeof(log), nullptr, log);
        glDeleteProgram(id);
        throw std::runtime_error(log);
    }
    localLightCountLocation = glGetUniformLocation(id, "localLightCount");
    if (localLightCountLocation >= 0) {
        for (std::size_t i = 0; i < Light::maxCount; ++i) {
            const auto prefix = "localLights[" + std::to_string(i) + "].";
            auto location = [&](const char *field) { return glGetUniformLocation(id, (prefix + field).c_str()); };
            lightUniforms[i] = {location("type"), location("position"), location("direction"),
                location("color"), location("intensity"), location("range"),
                location("innerCos"), location("outerCos")};
        }
    }
}

Shader::~Shader() {
    glDeleteProgram(id);
}

void Shader::bind() const {
    glUseProgram(id);
}

void Shader::use(const Mat4 &vp, Vec3 eye) {
    bind();
    glUniform1i(localLightCountLocation, 0);
    glUniform1i(glGetUniformLocation(id, "shadowsEnabled"), GL_FALSE);
    glUniform1i(glGetUniformLocation(id, "secondaryShadowsEnabled"), GL_FALSE);
    // Samplers of different types must use different units even in inactive branches.
    glUniform1i(glGetUniformLocation(id, "secondaryShadowMap"), 2);
    glUniform1i(glGetUniformLocation(id, "pageTexture"), 1);
    glUniform1i(glGetUniformLocation(id, "pageSurface"), 4);
    glUniform1i(glGetUniformLocation(id, "modelTexture"), 3);
    glUniformMatrix4fv(glGetUniformLocation(id, "viewProjection"), 1, GL_FALSE, vp.v);
    glUniform3f(glGetUniformLocation(id, "eye"), eye.x, eye.y, eye.z);
}

void Shader::lighting(const Scenario &scene) {
    auto vector = [&](const char *name, Vec3 v) {
        glUniform3f(glGetUniformLocation(id, name), v.x, v.y, v.z);
    };
    auto scalar = [&](const char *name, float value) {
        glUniform1f(glGetUniformLocation(id, name), value);
    };
    vector("lightPosition", scene.lightPosition);
    vector("lightColor", scene.lightColor);
    vector("secondaryLightPosition", scene.secondaryLightPosition);
    vector("secondaryLightColor", scene.secondaryLightColor);
    vector("ambientColor", scene.ambientColor);
    vector("fogColor", scene.fogColor);
    scalar("lightIntensity", scene.lightIntensity);
    scalar("secondaryLightIntensity", scene.secondaryLightIntensity);
    scalar("ambientStrength", scene.ambientStrength);
    scalar("diffuseStrength", scene.diffuseStrength);
    scalar("specularStrength", scene.specularStrength);
    scalar("woodSpecularStrength", scene.woodSpecularStrength);
    scalar("shininess", scene.shininess);
    scalar("fogDensity", scene.fogDensity);
    scalar("shadowBias", scene.shadowBias);
    scalar("shadowMinBias", scene.shadowMinBias);
    scalar("shadowNormalOffset", scene.shadowNormalOffset);
    localLights(scene.localLights);
}

void Shader::localLights(const std::vector<Light> &lights) {
    if (localLightCountLocation < 0) return;
    const auto count = std::count_if(lights.begin(), lights.end(), [](const Light &light) { return light.enabled; });
    if (count > static_cast<std::ptrdiff_t>(Light::maxCount))
        throw std::runtime_error("Limite de 16 luzes locais ativas excedido na cena");
    for (const auto &light : lights)
        if (light.enabled) light.validate();

    std::size_t index = 0;
    constexpr float radians = 3.14159265358979323846f / 180.f;
    for (const auto &light : lights) {
        if (!light.enabled) continue;
        const auto &u = lightUniforms[index++];
        const Vec3 direction = light.type == Light::Type::Spot ? normalized(light.direction) : Vec3{0, -1, 0};
        glUniform1i(u.type, light.type == Light::Type::Spot ? 1 : 0);
        glUniform3f(u.position, light.position.x, light.position.y, light.position.z);
        glUniform3f(u.direction, direction.x, direction.y, direction.z);
        glUniform3f(u.color, light.color.x, light.color.y, light.color.z);
        glUniform1f(u.intensity, light.intensity);
        glUniform1f(u.range, light.range);
        glUniform1f(u.innerCos, std::cos(light.innerAngle * radians));
        glUniform1f(u.outerCos, std::cos(light.outerAngle * radians));
    }
    glUniform1i(localLightCountLocation, static_cast<GLint>(index));
}

void Shader::environment(const Scenario &scene) {
    lighting(scene);
    glUniform1i(glGetUniformLocation(id, "binarySuns"), scene.animation == Scenario::Animation::BinarySuns);
    glUniform1f(glGetUniformLocation(id, "sunRadius"), scene.sunRadius);
    glUniform1f(glGetUniformLocation(id, "secondarySunRadius"), scene.secondarySunRadius);
    glUniform1f(glGetUniformLocation(id, "skyBrightness"), scene.skyBrightness);
    glUniform1f(glGetUniformLocation(id, "lightningFlash"), scene.lightningFlash);
    glUniform1f(glGetUniformLocation(id, "lightningSeed"), scene.lightningSeed);
    glUniform3f(glGetUniformLocation(id, "lightningDirection"), scene.lightningDirection.x,
        scene.lightningDirection.y, scene.lightningDirection.z);
}

void Shader::object(const Mat4 &m, Vec3 c, bool wood) {
    emission({});
    glUniform1i(glGetUniformLocation(id, "modelTextureEnabled"), GL_FALSE);
    glUniform1i(glGetUniformLocation(id, "pageTextureEnabled"), GL_FALSE);
    glUniform1i(glGetUniformLocation(id, "ruledPaperEnabled"), GL_FALSE);
    glUniform1i(glGetUniformLocation(id, "woodMaterial"), wood);
    glUniformMatrix4fv(glGetUniformLocation(id, "model"), 1, GL_FALSE, m.v);
    glUniform3f(glGetUniformLocation(id, "color"), c.x, c.y, c.z);
}

bool Shader::emission(Vec3 radiance, bool textured) {
    const auto location = glGetUniformLocation(id, "emissionColor");
    glUniform3f(location, radiance.x, radiance.y, radiance.z);
    glUniform1i(glGetUniformLocation(id, "texturedEmission"), textured);
    return location >= 0;
}

void Shader::modelTexture(GLuint texture) {
    glUniform1i(glGetUniformLocation(id, "modelTextureEnabled"), texture != 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE0);
}

void Shader::pageTexture(GLuint texture, float height, GLuint surfaceTexture) {
    glUniform1i(glGetUniformLocation(id, "pageTextureEnabled"), GL_TRUE);
    glUniform1f(glGetUniformLocation(id, "pageHeight"), height);
    glUniform1i(glGetUniformLocation(id, "pageTexture"), 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, surfaceTexture);
    glActiveTexture(GL_TEXTURE0);
}

void Shader::ruledPaper(Vec2 origin, Vec2 pixelScale) {
    glUniform1i(glGetUniformLocation(id, "ruledPaperEnabled"), GL_TRUE);
    glUniform4f(glGetUniformLocation(id, "rulingMapping"), origin.x, origin.y,
                pixelScale.x, pixelScale.y);
}

void Shader::shadows(const Mat4 &lightViewProjection, Vec3 lightPosition, GLuint depthMap, bool secondary) {
    glUniformMatrix4fv(glGetUniformLocation(id, secondary ? "secondaryLightViewProjection" : "lightViewProjection"), 1, GL_FALSE,
                       lightViewProjection.v);
    glUniform3f(glGetUniformLocation(id, secondary ? "secondaryLightPosition" : "lightPosition"), lightPosition.x, lightPosition.y,
                lightPosition.z);
    glUniform1i(glGetUniformLocation(id, secondary ? "secondaryShadowsEnabled" : "shadowsEnabled"), GL_TRUE);
    glUniform1i(glGetUniformLocation(id, secondary ? "secondaryShadowMap" : "shadowMap"), secondary ? 2 : 0);
    glActiveTexture(secondary ? GL_TEXTURE2 : GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glActiveTexture(GL_TEXTURE0);
}
