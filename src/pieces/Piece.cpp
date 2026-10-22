#include "Piece.h"
#include "Bishop.h"
#include "King.h"
#include "Knight.h"
#include "Pawn.h"
#include "Queen.h"
#include "Rook.h"
#include <cctype>

Piece::Piece(Primitives &p, bool w)
    : primitives(p), color(w ? Vec3{.91f, .83f, .66f} : Vec3{.22f, .32f, .38f}), white(w) {
    angle = w ? 0 : 3.14159265f;
}

void Piece::part(Shader &s, const Mat4 &m, Primitive t, Vec3 p, Vec3 z, float a) {
    primitives.draw(s, t, m, p, z, color, a);
}

void Piece::base(Shader &s, const Mat4 &m) {
    part(s, m, Primitive::Cylinder, {0, 0, 0}, {.34f, .08f, .34f});
    part(s, m, Primitive::Cylinder, {0, .08f, 0}, {.29f, .08f, .29f});
    part(s, m, Primitive::Cone, {0, .16f, 0}, {.27f, .40f, .27f});
    primitives.draw(s, Primitive::Cylinder, m, {0, .105f, 0}, {.298f, .025f, .298f},
                    {.60f, .40f, .13f});
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
