#pragma once
#include "Math.h"
#include <glad/gl.h>

class Shader {
    GLuint id = 0;

  public:
    Shader(const char *vertexFile = "scene.vert", const char *fragmentFile = "scene.frag");
    ~Shader();
    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    void use(const Mat4 &viewProjection, Vec3 eye);
    void shadows(const Mat4 &lightViewProjection, Vec3 lightPosition, GLuint depthMap);
    void object(const Mat4 &model, Vec3 color);
};
