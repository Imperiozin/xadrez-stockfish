#pragma once
#include "Object.h"
#include <memory>

class Piece : public Object {
  protected:
    Primitives &primitives;
    Vec3 color;
    void base(Shader &, const Mat4 &);
    void part(Shader &, const Mat4 &, Primitive, Vec3, Vec3, float angle = 0);

  public:
    const bool white;
    explicit Piece(Primitives &p, bool white);
    static std::unique_ptr<Piece> create(char symbol, Primitives &p);
};
