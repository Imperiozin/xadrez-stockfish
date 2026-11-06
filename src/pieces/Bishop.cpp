#include "Bishop.h"

void Bishop::draw(Shader &s, const Mat4 &parent) {
    drawModel(s, parent, "Bishop.fbx", 1.22f);
}
