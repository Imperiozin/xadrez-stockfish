#pragma once
#include "Mesh.h"
#include "Shader.h"

// The scene uses parent transforms, as in GLFW_Cubo_OO.
class Object {
  public:
    Vec3 position{}, scale{1, 1, 1};
    float angle = 0;
    virtual ~Object() = default;
    virtual void draw(Shader &shader, const Mat4 &parent) = 0;

    Mat4 model(const Mat4 &parent) const {
        return parent * transform(position, scale, angle);
    }
};
enum class Primitive { Cube, Cylinder, Cone, Sphere };

class Primitives {
    Mesh cubeMesh{Mesh::cube()}, cylinderMesh{Mesh::cylinder()}, coneMesh{Mesh::cylinder(0)},
        sphereMesh{Mesh::sphere()};

  public:
    void draw(Shader &shader, Primitive type, const Mat4 &parent, Vec3 position, Vec3 scale,
              Vec3 color, float angle = 0);
};
