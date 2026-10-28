#pragma once
#include "Piece.h"
#include <array>
#include <string>
#include <vector>

class Board : public Object {
    Primitives &primitives;
    std::array<std::unique_ptr<Piece>, 64> pieces;
    std::array<char, 64> symbols{};

  public:
    int selected = -1, hovered = -1;
    std::vector<int> captures, castling;
    std::vector<int> destinations;

    explicit Board(Primitives &p) : primitives(p) {}

    void sync(const std::string &fen);
    bool isWhite(int square) const;
    void draw(Shader &, const Mat4 &) override;

    static Vec3 squarePosition(int square) {
        return {float(square % 8) - 3.5f, .035f, 3.5f - float(square / 8)};
    }
};
