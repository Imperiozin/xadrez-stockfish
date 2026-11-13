#pragma once
#include "Mesh.h"
#include <memory>

// CPU skinning keeps the same deformed mesh in the scene and every shadow pass.
// This class owns the Assimp scene and can also be sampled without an OpenGL context.
class AnimatedModel {
    struct Impl;
    std::unique_ptr<Impl> impl;
  public:
    explicit AnimatedModel(const std::string &path, float height = 2.8f);
    ~AnimatedModel();
    AnimatedModel(const AnimatedModel &) = delete;
    AnimatedModel &operator=(const AnimatedModel &) = delete;
    void sample(const std::string &clip, double seconds);
    double duration(const std::string &clip) const;
    const std::vector<Vertex> &vertices() const;
    const std::vector<unsigned char> &texturePng() const;
};
