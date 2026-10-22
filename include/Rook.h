#pragma once
#include "Piece.h"

class Rook : public Piece {
  public:
    using Piece::Piece;
    void draw(Shader &, const Mat4 &) override;
};
