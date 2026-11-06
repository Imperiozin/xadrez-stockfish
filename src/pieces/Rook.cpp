#include "Rook.h"

void Rook::draw(Shader &s, const Mat4 &parent) {
    drawModel(s, parent, "Rook.fbx", 1.03f);
}
