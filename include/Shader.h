#pragma once
#include "Math.h"
#include <glad/gl.h>

class Shader {
    GLuint id = 0;

  public:
    Shader();
    ~Shader();
    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    void use(const Mat4 &viewProjection, Vec3 eye);
    void object(const Mat4 &model, Vec3 color);
};
