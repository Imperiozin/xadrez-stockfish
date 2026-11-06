#include "Queen.h"

void Queen::draw(Shader &s, const Mat4 &parent) {
    drawModel(s, parent, "Queen.fbx", 1.31f);
}
