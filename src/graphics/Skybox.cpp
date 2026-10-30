#include "Skybox.h"
#include "stockfish.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../../vendor/stb_image.h"
#include <stdexcept>
#include <string>

Skybox::Skybox(const std::string &assetDirectory) {
    const char *faces[] = {"right.png", "left.png", "top.png", "bottom.png",
                           "front.png", "back.png"};
    const auto directory = pastaDoExecutavel() / assetDirectory;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    int faceSize = 0;
    try {
        // Cubemap faces use their original top-to-bottom image rows.
        stbi_set_flip_vertically_on_load(false);
        for (int i = 0; i < 6; ++i) {
            const auto path = (directory / faces[i]).string();
            int width, height, channels;
            auto *pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
            if (!pixels) throw std::runtime_error("Falha ao carregar skybox: " + path);
            if (width != height || (faceSize && width != faceSize)) {
                stbi_image_free(pixels);
                throw std::runtime_error("Faces da skybox devem ser quadradas e do mesmo tamanho: " + path);
            }
            faceSize = width;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8,
                         width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            stbi_image_free(pixels);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glGenVertexArrays(1, &vao);
    } catch (...) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        glDeleteTextures(1, &texture);
        throw;
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

Skybox::~Skybox() {
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &texture);
}

void Skybox::draw(Vec3 eye, Vec3 target, float aspect) {
    const auto forward = normalized(target - eye);
    const auto right = normalized(cross(forward, {0, 1, 0}));
    const auto up = cross(right, forward);
    // Same vertical field of view as perspective(), without camera translation.
    const auto x = right * (std::tan(.65f / 2) * aspect);
    const auto y = up * std::tan(.65f / 2);
    Mat4 rays;
    rays.v[0] = x.x; rays.v[1] = x.y; rays.v[2] = x.z;
    rays.v[4] = y.x; rays.v[5] = y.y; rays.v[6] = y.z;
    rays.v[8] = forward.x; rays.v[9] = forward.y; rays.v[10] = forward.z;
    shader.use(rays, eye);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}
