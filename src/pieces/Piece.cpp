#include "Piece.h"
#include <assimp/defs.h>
#include "Bishop.h"
#include "King.h"
#include "Knight.h"
#include "Pawn.h"
#include "Queen.h"
#include "Rook.h"
#include <cctype>

Piece::Piece(Primitives &p, bool w)
    : primitives(p), color(w ? Vec3{.91f, .83f, .66f} : Vec3{.22f, .32f, .38f}), white(w) {
    angle = w ? 0 : AI_MATH_PI_F;
}

void Piece::drawModel(Shader &s, const Mat4 &parent, const char *file, float height, float rotation) {
    primitives.drawModel(s, std::string("assets/objects/pieces/") + file, height,
                         model(parent) * transform({}, {1, 1, 1}, rotation), color);
}

std::unique_ptr<Piece> Piece::create(char c, Primitives &p) {
    bool w = std::isupper(static_cast<unsigned char>(c)) != 0;
    switch (std::tolower(static_cast<unsigned char>(c))) {
    case 'p':
        return std::make_unique<Pawn>(p, w);
    case 'r':
        return std::make_unique<Rook>(p, w);
    case 'n':
        return std::make_unique<Knight>(p, w);
    case 'b':
        return std::make_unique<Bishop>(p, w);
    case 'q':
        return std::make_unique<Queen>(p, w);
    case 'k':
        return std::make_unique<King>(p, w);
    default:
        return nullptr;
    }
}
