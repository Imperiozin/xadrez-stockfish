#include "Knight.h"

void Knight::draw(Shader &s, const Mat4 &parent) {
    auto m = model(parent);
    base(s, m);
    part(s, m, Primitive::Cube, {0, .48f, .03f}, {.28f, .42f, .30f});
    part(s, m, Primitive::Sphere, {0, .72f, -.07f}, {.20f, .24f, .24f});
    part(s, m, Primitive::Cube, {0, .73f, -.23f}, {.24f, .19f, .32f});
    for (float x : {-.115f, .115f}) {
        part(s, m, Primitive::Cone, {x, .86f, .01f}, {.065f, .22f, .08f});
    }
    for (int i = 0; i < 4; ++i) {
        part(s, m, Primitive::Cube, {0, .43f + i * .10f, .18f}, {.12f, .11f, .12f});
    }
}
