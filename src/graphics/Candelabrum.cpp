#include "Candelabrum.h"
#include "stockfish.hpp"
#include "../../vendor/stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cfloat>
#include <map>
#include <numeric>
#include <random>
#include <stdexcept>

namespace {
struct Bounds {
    Vec3 low{FLT_MAX, FLT_MAX, FLT_MAX}, high{-FLT_MAX, -FLT_MAX, -FLT_MAX};
    void add(Vec3 p) {
        low = {std::min(low.x, p.x), std::min(low.y, p.y), std::min(low.z, p.z)};
        high = {std::max(high.x, p.x), std::max(high.y, p.y), std::max(high.z, p.z)};
    }
};

// Cada chama usa planos cruzados. Agrupa os planos pelo centro para obter uma fonte por pavio.
std::vector<Vec3> flameCenters(const aiMesh &mesh, Vec3 origin, float factor, float mergeDistance) {
    std::vector<unsigned> roots(mesh.mNumVertices);
    std::iota(roots.begin(), roots.end(), 0);
    auto root = [&](unsigned i) {
        while (roots[i] != i) i = roots[i] = roots[roots[i]];
        return i;
    };
    for (unsigned f = 0; f < mesh.mNumFaces; ++f) {
        const auto &face = mesh.mFaces[f];
        for (unsigned i = 1; i < face.mNumIndices; ++i)
            roots[root(face.mIndices[i])] = root(face.mIndices[0]);
    }
    std::map<unsigned, Bounds> groups;
    for (unsigned i = 0; i < mesh.mNumVertices; ++i) {
        const auto p = mesh.mVertices[i];
        groups[root(i)].add((Vec3{p.x, p.y, p.z} - origin) * factor);
    }
    std::vector<Vec3> centers;
    for (const auto &group : groups) {
        const auto center = (group.second.low + group.second.high) * .5f;
        if (std::none_of(centers.begin(), centers.end(), [&](Vec3 previous) {
            const auto offset = center - previous;
            return dot(offset, offset) < mergeDistance * mergeDistance;
        })) centers.push_back(center);
    }
    return centers;
}

GLuint loadTexture(const arquivos::path &path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);
    auto *pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    if (!pixels) throw std::runtime_error("Falha ao carregar textura do candelabro: " + path.string());
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    return texture;
}
}

struct Candelabrum::Impl {
    struct Part {
        std::unique_ptr<Mesh> mesh;
        bool flame;
    };
    std::vector<Part> parts;
    Light draft;
    float flameBrightness = 1;
    GLuint brassTexture = 0, flameTexture = 0;
    ~Impl() {
        glDeleteTextures(1, &brassTexture);
        glDeleteTextures(1, &flameTexture);
    }
};

Candelabrum::Candelabrum(float height) : impl(std::make_unique<Impl>()) {
    if (!std::isfinite(height) || height <= 0)
        throw std::runtime_error("Altura inválida para o candelabro");
    const auto folder = pastaDoExecutavel() / "assets/objects/candelabrum";
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_PP_PTV_KEEP_HIERARCHY, true);
    const auto *scene = importer.ReadFile((folder / "candelabro_maior_4k.fbx").string(),
        aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices | aiProcess_ValidateDataStructure | aiProcess_FlipUVs);
    if (!scene || !scene->HasMeshes() || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
        throw std::runtime_error("Falha ao importar o candelabro: " + std::string(importer.GetErrorString()));
    Bounds bounds;
    for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
        const auto &mesh = *scene->mMeshes[i];
        if (!mesh.HasNormals() || !mesh.HasTextureCoords(0))
            throw std::runtime_error("Candelabro sem normais ou coordenadas de textura");
        for (unsigned j = 0; j < mesh.mNumVertices; ++j) {
            const auto p = mesh.mVertices[j];
            if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
                throw std::runtime_error("Coordenadas inválidas no candelabro");
            bounds.add({p.x, p.y, p.z});
        }
    }
    if (bounds.high.y <= bounds.low.y)
        throw std::runtime_error("Candelabro sem geometria válida");
    const float factor = height / (bounds.high.y - bounds.low.y);
    const Vec3 origin{(bounds.low.x + bounds.high.x) * .5f, bounds.low.y,
                      (bounds.low.z + bounds.high.z) * .5f};
    std::mt19937 random(std::random_device{}());
    std::uniform_real_distribution<float> phase(0, 4096), speed(2, 3.5f);
    // Uma corrente de ar lenta afeta o conjunto, além da variação de cada pavio.
    impl->draft.flickerAmount = .15f;
    impl->draft.flickerSpeed = .6f;
    impl->draft.flickerPhase = phase(random);
    for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
        const auto &mesh = *scene->mMeshes[i];
        aiString material;
        scene->mMaterials[mesh.mMaterialIndex]->Get(AI_MATKEY_NAME, material);
        const bool flame = std::string(material.C_Str()).find("flame") != std::string::npos;
        std::vector<Vertex> vertices;
        vertices.reserve(mesh.mNumFaces * 3);
        for (unsigned f = 0; f < mesh.mNumFaces; ++f) {
            const auto &face = mesh.mFaces[f];
            if (face.mNumIndices != 3) continue;
            for (unsigned j = 0; j < 3; ++j) {
                const unsigned index = face.mIndices[j];
                const auto p = mesh.mVertices[index], n = mesh.mNormals[index];
                const auto uv = mesh.mTextureCoords[0][index];
                vertices.push_back({(Vec3{p.x, p.y, p.z} - origin) * factor,
                                    {n.x, n.y, n.z}, {uv.x, uv.y}});
            }
        }
        impl->parts.push_back({std::make_unique<Mesh>(vertices), flame});
        if (flame) {
            for (auto center : flameCenters(mesh, origin, factor, height * .02f)) {
                auto light = Light::candle(center, 6.f, 14.f);
                light.flickerAmount = .30f;
                light.flickerPhase = phase(random);
                light.flickerSpeed = speed(random);
                lights.push_back(light);
            }
        }
    }
    if (lights.empty() || lights.size() > Light::maxCount)
        throw std::runtime_error("Não foi possível identificar as chamas do candelabro");
    impl->brassTexture = loadTexture(folder / "textures/brass_candleholders_03_diff_4k.png");
    impl->flameTexture = loadTexture(folder / "textures/brass_candleholders_flame_diff_4k.png");
}

Candelabrum::~Candelabrum() = default;

void Candelabrum::collectLights(std::vector<Light> &result, const Mat4 &parent, double seconds) const {
    const auto first = result.size();
    Object::collectLights(result, parent, seconds);
    const float draft = impl->draft.flickerMultiplier(seconds);
    float baseIntensity = 0, sampledIntensity = 0;
    for (const auto &light : lights)
        if (light.enabled) baseIntensity += light.intensity;
    for (auto i = first; i < result.size(); ++i) {
        result[i].intensity *= draft;
        sampledIntensity += result[i].intensity;
    }
    // O brilho das chamas acompanha a mesma amostra que ilumina a mesa.
    impl->flameBrightness = baseIntensity > 0 ? sampledIntensity / baseIntensity : 0;
}

void Candelabrum::draw(Shader &shader, const Mat4 &parent) {
    const auto world = model(parent);
    for (const auto &part : impl->parts) {
        if (part.flame) continue;
        shader.object(world, {1, 1, 1});
        shader.modelTexture(impl->brassTexture);
        part.mesh->draw();
    }
}

void Candelabrum::drawTransparent(Shader &shader, const Mat4 &parent) {
    shader.object(model(parent), {1, 1, 1});
    // A emissão texturizada só existe na passagem de cor; as chamas não lançam sombras.
    const float brightness = 2.5f * impl->flameBrightness;
    if (!shader.emission({brightness, brightness, brightness}, true)) return;
    shader.modelTexture(impl->flameTexture);
    GLboolean depthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    const bool blending = glIsEnabled(GL_BLEND) == GL_TRUE;
    GLint sourceRgb, destinationRgb, sourceAlpha, destinationAlpha, equationRgb, equationAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &sourceRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &destinationRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &sourceAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &destinationAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &equationRgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &equationAlpha);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    for (const auto &part : impl->parts) if (part.flame) part.mesh->draw();
    glDepthMask(depthMask);
    glBlendEquationSeparate(equationRgb, equationAlpha);
    glBlendFuncSeparate(sourceRgb, destinationRgb, sourceAlpha, destinationAlpha);
    if (!blending) glDisable(GL_BLEND);
    shader.emission({});
}
