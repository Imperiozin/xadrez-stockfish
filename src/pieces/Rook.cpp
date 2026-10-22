#include "Rook.h"

void Rook::draw(Shader &s, const Mat4 &parent) {
    auto m = model(parent);
    base(s, m);
    part(s, m, Primitive::Cylinder, {0, .28f, 0}, {.21f, .42f, .21f});
    part(s, m, Primitive::Cylinder, {0, .68f, 0}, {.30f, .11f, .30f});
    for (int i = 0; i < 6; ++i) {
        float a = i * 6.2831853f / 6;
        part(s, m, Primitive::Cube, {.23f * std::cos(a), .85f, .23f * std::sin(a)},
             {.15f, .18f, .15f}, -a);
    }
}
