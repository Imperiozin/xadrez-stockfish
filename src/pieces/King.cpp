#include "King.h"

void King::draw(Shader &s, const Mat4 &parent) {
    drawModel(s, parent, "King.fbx", 1.40f);
}
