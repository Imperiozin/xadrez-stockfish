#include "Queen.h"

void Queen::draw(Shader &s, const Mat4 &parent) {
    auto m = model(parent);
    base(s, m);
    part(s, m, Primitive::Cylinder, {0, .40f, 0}, {.14f, .37f, .14f});
    part(s, m, Primitive::Cylinder, {0, .73f, 0}, {.25f, .07f, .25f});
    part(s, m, Primitive::Sphere, {0, .87f, 0}, {.20f, .16f, .20f});
    for (int i = 0; i < 7; ++i) {
        float a = i * 6.2831853f / 7;
        Vec3 p{.20f * std::cos(a), .89f, .20f * std::sin(a)};
        part(s, m, Primitive::Cone, p, {.07f, .27f, .07f});
        part(s, m, Primitive::Sphere, p + Vec3{0, .27f, 0}, {.065f, .065f, .065f});
    }
    part(s, m, Primitive::Sphere, {0, 1.14f, 0}, {.10f, .10f, .10f});
}
