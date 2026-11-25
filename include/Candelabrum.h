#pragma once
#include "Object.h"

class Candelabrum : public Object {
    struct Impl;
    std::unique_ptr<Impl> impl;

  public:
    explicit Candelabrum(float height = 4.2f);
    ~Candelabrum() override;
    void draw(Shader &shader, const Mat4 &parent) override;
    void drawTransparent(Shader &shader, const Mat4 &parent) override;
    void collectLights(std::vector<Light> &result, const Mat4 &parent, double seconds) const override;
};
