#include "Table.h"
#include <utility>

void Table::addProp(std::unique_ptr<Object> prop) {
    if (prop) props.push_back(std::move(prop));
}

void Table::draw(Shader &shader, const Mat4 &parent) {
    const auto tableModel = model(parent);
    auto timber = [&](Vec3 position, Vec3 size, Vec3 color, float angle = 0.f) {
        shader.object(tableModel * transform(position, size, angle), color, true);
        cube.draw();
    };
    constexpr float rim = .65f, thickness = .38f;
    constexpr float innerWidth = width - 2 * rim;
    constexpr float innerDepth = depth - 2 * rim;
    constexpr float topCenter = surfaceY - thickness / 2;
    const Vec3 frameColor{.31f, .145f, .060f};
    // Four solid rails surround the slatted inset, with flush upper surfaces.
    for (float side : {-1.f, 1.f}) {
        timber({side * (width - rim) / 2, topCenter, 0},
               {rim, thickness, depth}, frameColor);
        timber({0, topCenter, side * (depth - rim) / 2},
               {rim, thickness, innerWidth}, frameColor, 1.5707963f);
    }
    constexpr int plankCount = 16;
    constexpr float gap = .055f;
    constexpr float plankWidth = (innerWidth - gap * (plankCount + 1)) / plankCount;
    for (int i = 0; i < plankCount; ++i) {
        const float x = -innerWidth / 2 + gap + plankWidth / 2 + i * (plankWidth + gap);
        const float shade = .94f + .035f * ((i * 7) % 5);
        timber({x, topCenter, 0}, {plankWidth, thickness, innerDepth - 2 * gap},
               Vec3{.46f, .255f, .115f} * shade);
    }
    // Recessed aprons and four legs leave the gaps in the tabletop open.
    constexpr float legHeight = 5.8f;
    for (float x : {-1.f, 1.f}) {
        for (float z : {-1.f, 1.f}) {
            timber({x * (width / 2 - 1), surfaceY - thickness - legHeight / 2,
                    z * (depth / 2 - 1)}, {.65f, legHeight, .65f}, frameColor);
        }
        timber({x * (width / 2 - 1), surfaceY - thickness - .45f, 0},
               {.25f, .9f, depth - 2}, frameColor);
        timber({0, surfaceY - thickness - .45f, x * (depth / 2 - 1)},
               {.25f, .9f, width - 2}, frameColor, 1.5707963f);
    }
    for (auto &prop : props) prop->draw(shader, tableModel);
}
