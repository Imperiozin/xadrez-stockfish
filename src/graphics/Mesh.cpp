#include "Mesh.h"

namespace {
constexpr float pi = 3.14159265f;

void tri(std::vector<Vertex> &v, Vec3 a, Vec3 b, Vec3 c, Vec3 na, Vec3 nb, Vec3 nc) {
    v.push_back({a, na});
    v.push_back({b, nb});
    v.push_back({c, nc});
}
} // namespace

Mesh::Mesh(const std::vector<Vertex> &v) : count(static_cast<GLsizei>(v.size())) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(Vertex), v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void *>(sizeof(Vec3)));
    glEnableVertexAttribArray(1);
}

Mesh::~Mesh() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void Mesh::draw() const {
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, count);
}

std::vector<Vertex> Mesh::cube() {
    std::vector<Vertex> v;
    Vec3 p[] = {{-.5f, -.5f, -.5f}, {.5f, -.5f, -.5f}, {.5f, .5f, -.5f}, {-.5f, .5f, -.5f},
                {-.5f, -.5f, .5f},  {.5f, -.5f, .5f},  {.5f, .5f, .5f},  {-.5f, .5f, .5f}};
    int faces[][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 4, 7, 3},
                      {1, 2, 6, 5}, {3, 7, 6, 2}, {0, 1, 5, 4}};
    for (auto &f : faces) {
        auto n = normalized(cross(p[f[1]] - p[f[0]], p[f[2]] - p[f[0]]));
        tri(v, p[f[0]], p[f[1]], p[f[2]], n, n, n);
        tri(v, p[f[0]], p[f[2]], p[f[3]], n, n, n);
    }
    return v;
}

std::vector<Vertex> Mesh::cylinder(float top) {
    std::vector<Vertex> v;
    for (int i = 0; i < 32; ++i) {
        float a = 2 * pi * i / 32, b = 2 * pi * (i + 1) / 32;
        Vec3 p{std::cos(a), 0, std::sin(a)}, q{std::cos(b), 0, std::sin(b)},
            r{p.x * top, 1, p.z * top}, s{q.x * top, 1, q.z * top};
        auto n = normalized(Vec3{p.x, 1 - top, p.z}), m = normalized(Vec3{q.x, 1 - top, q.z});
        tri(v, p, r, s, n, n, m);
        tri(v, p, s, q, n, m, m);
        tri(v, {0, 0, 0}, q, p, {0, -1, 0}, {0, -1, 0}, {0, -1, 0});
        if (top > 0) {
            tri(v, {0, 1, 0}, r, s, {0, 1, 0}, {0, 1, 0}, {0, 1, 0});
        }
    }
    return v;
}

std::vector<Vertex> Mesh::sphere() {
    std::vector<Vertex> v;
    auto point = [](float a, float b) {
        return Vec3{std::sin(a) * std::cos(b), std::cos(a), std::sin(a) * std::sin(b)};
    };
    for (int j = 0; j < 16; ++j) {
        for (int i = 0; i < 24; ++i) {
            float a = pi * j / 16, b = pi * (j + 1) / 16, c = 2 * pi * i / 24,
                  d = 2 * pi * (i + 1) / 24;
            auto p = point(a, c), q = point(b, c), r = point(b, d), s = point(a, d);
            tri(v, p, q, r, p, q, r);
            tri(v, p, r, s, p, r, s);
        }
    }
    return v;
}
