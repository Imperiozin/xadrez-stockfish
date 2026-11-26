#pragma once
#include "Math3D.h"
#include "Scenario.h"
#include <glad/gl.h>
#include <array>

class Shader {
    GLuint id = 0;
    struct LightUniforms {
        GLint type, position, direction, color, intensity, range, innerCos, outerCos;
    };
    GLint localLightCountLocation = -1;
    std::array<LightUniforms, Light::maxCount> lightUniforms{};

  public:
    Shader(const char *vertexFile = "scene.vert", const char *fragmentFile = "scene.frag");
    ~Shader();
    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    void bind() const;
    void use(const Mat4 &viewProjection, Vec3 eye);
    void lighting(const Scenario &scenario);
    void localLights(const std::vector<Light> &lights);
    void environment(const Scenario &scenario);
    void shadows(const Mat4 &lightViewProjection, Vec3 lightPosition, GLuint depthMap, bool secondary = false);
    void localShadows(GLuint depthArray);
    void object(const Mat4 &model, Vec3 color, bool wood = false);
    bool emission(Vec3 radiance, bool textured = false);
    void pageTexture(GLuint texture, float height, GLuint surfaceTexture);
    void modelTexture(GLuint texture);
    void ruledPaper(Vec2 origin, Vec2 pixelScale);
};
