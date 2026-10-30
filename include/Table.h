#pragma once
#include "Object.h"
#include <memory>

// Table-local coordinates also locate future props and diegetic controls.
class Table : public Object {
    Mesh cube{Mesh::cube()};
    std::vector<std::unique_ptr<Object>> props;

  public:
    static constexpr float width = 22.f, depth = width;
    static constexpr float surfaceY = -.705f;
    void addProp(std::unique_ptr<Object> prop);
    void draw(Shader &shader, const Mat4 &parent) override;
};
