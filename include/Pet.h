#pragma once
#include "Object.h"
#include "avaliacao.hpp"

// Animated companion and a small review plaque on the physical table.
class Pet : public Object {
    struct Impl;
    std::unique_ptr<Impl> impl;
  public:
    explicit Pet(const std::string &name);
    ~Pet() override;
    void update(float dt);
    void review(const AvaliacaoJogada &evaluation);
    void thinking();
    void reset();
    void renderLabel(); // After ImGui::Render(), before rendering the 3D scene.
    void draw(Shader &shader, const Mat4 &parent) override;
};
