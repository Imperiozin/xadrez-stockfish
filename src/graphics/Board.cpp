#include "Board.h"
#include <cctype>
#include <stdexcept>

void Board::sync(const std::string &fen) {
    std::array<char, 64> next{};
    int rank = 7, file = 0;
    for (char c : fen) {
        if (c == ' ') {
            break;
        }
        if (c == '/') {
            --rank;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += c - '0';
        } else {
            if (rank < 0 || file > 7) {
                throw std::runtime_error("FEN invalida");
            }
            next[rank * 8 + file++] = c;
        }
    }
    for (int i = 0; i < 64; ++i) {
        if (next[i] != symbols[i]) {
            pieces[i] = Piece::create(next[i], primitives);
        }
        if (pieces[i]) {
            pieces[i]->position = squarePosition(i);
        }
    }
    symbols = next;
}

bool Board::isWhite(int s) const {
    return s >= 0 && s < 64 && std::isupper(static_cast<unsigned char>(symbols[s]));
}

void Board::draw(Shader &s, const Mat4 &parent) {
    auto m = model(parent);
    primitives.draw(s, Primitive::Cube, m, {0, -.38f, 0}, {9.2f, .65f, 9.2f}, {.12f, .065f, .035f});
    primitives.draw(s, Primitive::Cube, m, {0, -.12f, 0}, {8.9f, .09f, 8.9f}, {.63f, .43f, .16f});
    primitives.draw(s, Primitive::Cube, m, {0, -.07f, 0}, {8.65f, .10f, 8.65f},
                    {.20f, .11f, .055f});
    for (int i = 0; i < 64; ++i) {
        auto p = squarePosition(i);
        bool light = (i % 8 + i / 8) % 2;
        Vec3 c = light ? Vec3{.77f, .70f, .54f} : Vec3{.18f, .30f, .30f};
        if (i == selected) {
            c = {.83f, .56f, .16f};
        }
        if (i == hovered && i != selected) {
            c = c * .75f + Vec3{.25f, .28f, .22f};
        }
        primitives.draw(s, Primitive::Cube, m, {p.x, 0, p.z}, {.99f, .06f, .99f}, c);
        const Vec3 green{.30f, .77f, .53f};
        if (std::find(captures.begin(), captures.end(), i) != captures.end() || i == selected) {
            Vec3 color = i == selected ? Vec3{1.f, .68f, .15f} : Vec3{1.f, .08f, .06f};
            // Pequenas esferas formam um contorno continuo ao redor da base.
            for (int segment = 0; segment < 64; ++segment) {
                float a = segment * 6.2831853f / 64;
                primitives.draw(s, Primitive::Sphere, m,
                    {p.x + .38f * std::cos(a), .09f, p.z + .38f * std::sin(a)},
                    {.025f, .025f, .025f}, color);
            }
        }
        if (std::find(castling.begin(), castling.end(), i) != castling.end()) {
            for (int side = 0; side < 4; ++side) {
                for (int dash = 0; dash < 5; ++dash) {
                    float offset = -.36f + dash * .18f;
                    bool horizontal = side < 2;
                    primitives.draw(s, Primitive::Cube, m,
                        {p.x + (horizontal ? offset : (side == 2 ? -.44f : .44f)), .045f,
                         p.z + (horizontal ? (side == 0 ? -.44f : .44f) : offset)},
                        horizontal ? Vec3{.10f, .02f, .025f} : Vec3{.025f, .02f, .10f}, green);
                }
            }
        }
        if (std::find(destinations.begin(), destinations.end(), i) != destinations.end()) {
            primitives.draw(s, Primitive::Sphere, m, {p.x, .10f, p.z}, {.095f, .095f, .095f},
                            {.30f, .77f, .53f});
        }
    }
    for (auto &p : pieces) {
        if (p) {
            p->draw(s, m);
        }
    }
}
