#pragma once
#include "Object.h"
#include "avaliacao.hpp"

enum class ResultadoPartida;

// Companheiro Cube Pets (Kenney, https://kenney.nl, CC0) sobre a mesa.
class Pet : public Object {
    struct Impl;
    std::unique_ptr<Impl> impl;
  public:
    explicit Pet(const std::string &name, const Pet *previous = nullptr);
    ~Pet() override;
    void update(float dt);
    void review(const AvaliacaoJogada &evaluation);
    void thinking();
    void finish(ResultadoPartida result);
    void reset();
    void draw(Shader &shader, const Mat4 &parent) override;
};
