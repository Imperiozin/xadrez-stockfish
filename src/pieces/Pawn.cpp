#include "Pawn.h"

void Pawn::draw(Shader &s, const Mat4 &parent) {
    auto m = model(parent);
    base(s, m);
    part(s, m, Primitive::Cylinder, {0, .38f, 0}, {.17f, .08f, .17f});
    part(s, m, Primitive::Sphere, {0, .61f, 0}, {.21f, .21f, .21f});
}
