#pragma once
#include "Mesh.h"
#include "Shader.h"
#include <memory>
#include <unordered_map>

// The scene uses parent transforms, as in GLFW_Cubo_OO.
class Object {
  public:
    Vec3 position{}, scale{1, 1, 1};
    Vec3 rotation{};
    float angle = 0;
    virtual ~Object() = default;
    virtual void draw(Shader &shader, const Mat4 &parent) = 0;

    Mat4 model(const Mat4 &parent) const {
        return parent * transform(position, scale, Vec3{rotation.x, rotation.y + angle, rotation.z});
    }
};
enum class Primitive { Cube, Cylinder, Cone, Sphere };

class Primitives {
    std::unordered_map<std::string, std::unique_ptr<Mesh>> models;
    Mesh cubeMesh{Mesh::cube()}, cylinderMesh{Mesh::cylinder()}, coneMesh{Mesh::cylinder(0)},
        sphereMesh{Mesh::sphere()};

  public:
    void drawModel(Shader &shader, const std::string &file, float height,
                   const Mat4 &model, Vec3 color);
    void draw(Shader &shader, Primitive type, const Mat4 &parent, Vec3 position, Vec3 scale,
              Vec3 color, float angle = 0);
};
