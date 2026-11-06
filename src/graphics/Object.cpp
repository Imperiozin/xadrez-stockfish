#include "Object.h"

void Primitives::drawModel(Shader &shader, const std::string &file, float height,
                           const Mat4 &model, Vec3 color) {
    auto found = models.find(file);
    if (found == models.end()) {
        auto mesh = std::make_unique<Mesh>(Mesh::loadModel(file, height));
        found = models.emplace(file, std::move(mesh)).first;
    }
    shader.object(model, color);
    found->second->draw();
}

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
