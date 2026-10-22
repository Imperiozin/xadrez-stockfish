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
        primitives.draw(s, Primitive::Cube, m, {p.x, 0, p.z}, {.99f, .06f, .99f}, c);
        if (std::find(destinations.begin(), destinations.end(), i) != destinations.end()) {
            primitives.draw(s, Primitive::Cylinder, m, {p.x, .034f, p.z}, {.115f, .012f, .115f},
                            {.30f, .77f, .53f});
        }
    }
    for (auto &p : pieces) {
        if (p) {
            p->draw(s, m);
        }
    }
}
