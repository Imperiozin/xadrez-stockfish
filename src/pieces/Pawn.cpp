#include "Pawn.h"

void Pawn::draw(Shader &s, const Mat4 &parent) {
    drawModel(s, parent, "Pawn.fbx", .82f);
}
