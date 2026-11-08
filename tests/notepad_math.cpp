#include "Ray.h"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
void near(float actual, float expected, const char *message) {
    check(std::abs(actual - expected) < .0002f, message);
}
void surfaceFollowsTransform(const Mat4 &matrix) {
    Mat4 inverse;
    check(inverseAffine(matrix, inverse), "A transformação deveria ser invertível.");
    const PageSurface page{{-2, .3f, -1}, {2, .3f, 2}};
    for (const auto uv : {Vec2{.1f, .1f}, Vec2{.5f, .5f}, Vec2{.9f, .9f}}) {
        const Vec3 point{-2 + 4 * uv.x, .3f, -1 + 3 * uv.y};
        const Vec3 worldPoint = transformPoint(matrix, point);
        // Observador acima da folha em seu espaço local, após a mesma transformação.
        const Vec3 origin = transformPoint(matrix, point + Vec3{0, 5, 0});
        const Ray world{origin, normalized(worldPoint - origin)};
        Vec2 hit;
        float distance;
        check(page.intersect(transformRay(inverse, world), hit, distance),
              "O clique não acompanhou a página transformada.");
        near(hit.x, uv.x, "A coordenada horizontal do clique mudou.");
        near(hit.y, uv.y, "A coordenada vertical do clique mudou.");
        near(distance, std::sqrt(dot(worldPoint - origin, worldPoint - origin)),
             "A escala alterou a distância de interseção no mundo.");
    }
}
}

int main() {
    try {
        const auto previousYaw = transform({1, 2, 3}, {.5f, 2, 3}, .73f);
        const auto eulerYaw = transform({1, 2, 3}, {.5f, 2, 3}, Vec3{0, .73f, 0});
        for (int i = 0; i < 16; ++i)
            near(eulerYaw.v[i], previousYaw.v[i], "A rotação existente das peças foi alterada.");
        surfaceFollowsTransform(Mat4::identity());
        surfaceFollowsTransform(transform({7, -.7f, 3}, {1, 1, 1}, Vec3{}));
        surfaceFollowsTransform(transform({-3, 2, 8}, {2, .4f, .6f}, Vec3{.7f, 1.2f, -.4f}));
        surfaceFollowsTransform(transform({2, -4, 1}, {.8f, 1.7f, .3f}, Vec3{-.3f, -.8f, .6f}) *
                                transform({7, .2f, -5}, {1.4f, .9f, 2.1f}, Vec3{.5f, 2.4f, -.2f}));
        surfaceFollowsTransform(transform({1, 2, 3}, {-1, 2, .5f}, Vec3{.4f, -.8f, .1f}));
        Mat4 inverse;
        check(!inverseAffine(transform({}, {1, 0, 1}), inverse),
              "A escala nula deveria desabilitar os cliques.");
        const PageSurface page{{-1, 0, -1}, {1, 0, 1}};
        Vec2 uv;
        float distance;
        check(!page.intersect({{0, -1, 0}, {0, 1, 0}}, uv, distance),
              "O verso da página não deve aceitar cliques.");
        check(!page.intersect({{0, 1, 0}, {1, 0, 0}}, uv, distance),
              "Um raio paralelo não deve atingir a página.");
        check(!page.intersect({{2, 1, 0}, {0, -1, 0}}, uv, distance),
              "Um clique fora dos limites não deve atingir a página.");
        const Ray ray{{0, 2, 0}, {0, -1, 0}};
        check(intersectBox(ray, {-1, 0, -1}, {1, 1, 1}, distance),
              "O raio deveria atingir os limites do bloco.");
        near(distance, 1, "Distância incorreta dos limites do bloco.");
        check(!intersectBox({{2, 2, 0}, {0, -1, 0}}, {-1, 0, -1}, {1, 1, 1}, distance),
              "Um raio paralelo fora da caixa deveria ser descartado.");
        check(!intersectBox({{0, 2, 0}, {0, 1, 0}}, {-1, 0, -1}, {1, 1, 1}, distance),
              "Uma caixa atrás do observador não deveria bloquear cliques.");
        check(intersectTriangle(ray, {-1, 1, -1}, {1, 1, -1}, {0, 1, 1}, distance),
              "A geometria sobre a folha deve ser detectada para oclusão.");
        near(distance, 1, "Distância incorreta do obstáculo sobre a folha.");
        std::cout << "Transformações, herança, escala, limites, verso e oclusão validados.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Falha na verificação do bloco de notas: " << e.what() << '\n';
        return 1;
    }
}
