#pragma once
#include "Math.h"
#include <glad/gl.h>
#include <vector>

struct Vertex {
    Vec3 position, normal;
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
    static std::vector<Vertex> cube();
    static std::vector<Vertex> cylinder(float topRadius = 1);
    static std::vector<Vertex> sphere();
};
