#include "Bishop.h"

void Bishop::draw(Shader &s, const Mat4 &parent) {
    auto m = model(parent);
    base(s, m);
    part(s, m, Primitive::Cylinder, {0, .40f, 0}, {.14f, .25f, .14f});
    part(s, m, Primitive::Cylinder, {0, .63f, 0}, {.23f, .065f, .23f});
    part(s, m, Primitive::Sphere, {0, .83f, 0}, {.18f, .23f, .18f});
    part(s, m, Primitive::Cone, {0, .89f, 0}, {.14f, .27f, .14f});
    part(s, m, Primitive::Sphere, {0, 1.16f, 0}, {.06f, .06f, .06f});
}
