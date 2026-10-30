#pragma once
#include "Shader.h"

class Skybox {
    Shader shader{"skybox.vert", "skybox.frag"};
    GLuint texture = 0, vao = 0;

  public:
    explicit Skybox(const std::string &directory);
    ~Skybox();
    Skybox(const Skybox &) = delete;
    Skybox &operator=(const Skybox &) = delete;
    void draw(Vec3 eye, Vec3 target, float aspect);
};
