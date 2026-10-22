#include "Object.h"

void Primitives::draw(Shader &shader, Primitive type, const Mat4 &parent, Vec3 position, Vec3 scale,
                      Vec3 color, float angle) {
    shader.object(parent * transform(position, scale, angle), color);
    switch (type) {
    case Primitive::Cube:
        cubeMesh.draw();
        break;
    case Primitive::Cylinder:
        cylinderMesh.draw();
        break;
    case Primitive::Cone:
        coneMesh.draw();
        break;
    case Primitive::Sphere:
        sphereMesh.draw();
        break;
    }
}
