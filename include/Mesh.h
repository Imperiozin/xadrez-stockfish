#pragma once
#include "Math3D.h"
#include <glad/gl.h>
#include <vector>
#include <string>

struct Vertex {
    Vec3 position, normal;
    Vec2 uv{};
};

class Mesh {
    GLuint vao = 0, vbo = 0;
    GLsizei count = 0;

  public:
    explicit Mesh(const std::vector<Vertex> &vertices);
    ~Mesh();
    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;
    void draw() const;
    void update(const std::vector<Vertex> &vertices);
    static std::vector<Vertex> cube();
    static std::vector<Vertex> cylinder(float topRadius = 1);
    static std::vector<Vertex> sphere();
    static std::vector<Vertex> loadModel(const std::string &file, float height);
};
