#pragma once
#include "Object.h"
#include <memory>

class Piece : public Object {
  protected:
    Primitives &primitives;
    Vec3 color;
    void drawModel(Shader &, const Mat4 &, const char *file, float height, float rotation = 0);

  public:
    const bool white;
    explicit Piece(Primitives &p, bool white);
    static std::unique_ptr<Piece> create(char symbol, Primitives &p);
};
