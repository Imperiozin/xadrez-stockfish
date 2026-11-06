#include "Knight.h"
#include <assimp/defs.h>

void Knight::draw(Shader &s, const Mat4 &parent) {
    // The FBX faces -X; rotate it toward -Z (the opponent for white).
    drawModel(s, parent, "Knight.fbx", 1.08f, -AI_MATH_HALF_PI_F);
}
