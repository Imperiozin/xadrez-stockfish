#include "King.h"

void King::draw(Shader &s, const Mat4 &parent) {
    auto m = model(parent);
    base(s, m);
    part(s, m, Primitive::Cylinder, {0, .40f, 0}, {.16f, .39f, .16f});
    part(s, m, Primitive::Cylinder, {0, .76f, 0}, {.25f, .09f, .25f});
    part(s, m, Primitive::Sphere, {0, .94f, 0}, {.21f, .16f, .21f});
    part(s, m, Primitive::Cube, {0, 1.22f, 0}, {.10f, .35f, .10f});
    part(s, m, Primitive::Cube, {0, 1.26f, 0}, {.31f, .10f, .10f});
}
