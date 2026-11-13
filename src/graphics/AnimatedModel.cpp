#include "AnimatedModel.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/material.h>
#include <limits>
#include <fstream>
#include <iterator>
#include "stockfish.hpp"
#include <stdexcept>
#include <unordered_map>

namespace {
template<class Key, class Value, class Blend>
Value interpolate(const Key *keys, unsigned count, double tick, Value fallback, Blend blend) {
    if (!count) return fallback;
    if (count == 1 || tick <= keys[0].mTime) return keys[0].mValue;
    for (unsigned i = 1; i < count; ++i) {
        if (tick < keys[i].mTime) {
            const double interval = keys[i].mTime - keys[i-1].mTime;
            const float t = interval > 0 ? float((tick - keys[i-1].mTime) / interval) : 0.f;
            return blend(keys[i-1].mValue, keys[i].mValue, t);
        }
    }
    return keys[count-1].mValue;
}
Vec3 vector(const aiVector3D &p) { return {p.x, p.y, p.z}; }
}

struct AnimatedModel::Impl {
    Assimp::Importer importer;
    const aiScene *scene = nullptr;
    struct Node { const aiNode *source; int parent; aiMatrix4x4 global; };
    struct Part { const aiMesh *source; int node; };
    std::vector<Node> nodes;
    std::vector<Part> parts;
    std::unordered_map<std::string, int> nodeIds;
    std::unordered_map<std::string, const aiAnimation *> clips;
    std::vector<Vertex> output;
    std::vector<unsigned char> png;
    aiMatrix4x4 inverseRoot;
    Vec3 origin{};
    float scale = 1;
    bool grounded = false;

    void addNode(const aiNode *source, int parent) {
        const int index = int(nodes.size());
        if (!nodeIds.emplace(source->mName.C_Str(), index).second)
            throw std::runtime_error("Pet com nomes de ossos duplicados");
        nodes.push_back({source, parent, {}});
        for (unsigned i = 0; i < source->mNumMeshes; ++i)
            parts.push_back({scene->mMeshes[source->mMeshes[i]], index});
        for (unsigned i = 0; i < source->mNumChildren; ++i) addNode(source->mChildren[i], index);
    }

    void sample(const aiAnimation *animation, double seconds) {
        std::unordered_map<std::string, const aiNodeAnim *> channels;
        const double rate = animation->mTicksPerSecond > 0 ? animation->mTicksPerSecond : 25;
        const double tick = std::fmod(std::max(0.0, seconds) * rate, animation->mDuration);
        for (unsigned i = 0; i < animation->mNumChannels; ++i)
            channels.emplace(animation->mChannels[i]->mNodeName.C_Str(), animation->mChannels[i]);
        for (auto &node : nodes) {
            aiMatrix4x4 local = node.source->mTransformation;
            const auto found = channels.find(node.source->mName.C_Str());
            if (found != channels.end()) {
                const auto &channel = *found->second;
                aiVector3D s, p;
                aiQuaternion r;
                local.Decompose(s, r, p);
                auto lerp = [](aiVector3D a, aiVector3D b, float t) { return a + (b-a)*t; };
                p = interpolate(channel.mPositionKeys, channel.mNumPositionKeys, tick, p, lerp);
                s = interpolate(channel.mScalingKeys, channel.mNumScalingKeys, tick, s, lerp);
                r = interpolate(channel.mRotationKeys, channel.mNumRotationKeys, tick, r,
                    [](const aiQuaternion &a, const aiQuaternion &b, float t) {
                        aiQuaternion result;
                        aiQuaternion::Interpolate(result, a, b, t);
                        return result.Normalize();
                    });
                local = aiMatrix4x4(s, r, p);
            }
            node.global = node.parent < 0 ? local : nodes[node.parent].global * local;
        }
        output.clear();
        for (const auto &part : parts) {
            const auto &mesh = *part.source;
            std::vector<aiVector3D> positions(mesh.mNumVertices), normals(mesh.mNumVertices);
            std::vector<float> weights(mesh.mNumVertices);
            for (unsigned b = 0; b < mesh.mNumBones; ++b) {
                const auto &bone = *mesh.mBones[b];
                const auto found = nodeIds.find(bone.mName.C_Str());
                if (found == nodeIds.end()) throw std::runtime_error("Osso do pet sem nó correspondente");
                const auto matrix = inverseRoot * nodes[found->second].global * bone.mOffsetMatrix;
                auto normalMatrix = aiMatrix3x3(matrix);
                normalMatrix.Inverse().Transpose();
                for (unsigned w = 0; w < bone.mNumWeights; ++w) {
                    const auto &weight = bone.mWeights[w];
                    positions[weight.mVertexId] += (matrix * mesh.mVertices[weight.mVertexId]) * weight.mWeight;
                    normals[weight.mVertexId] += (normalMatrix * mesh.mNormals[weight.mVertexId]) * weight.mWeight;
                    weights[weight.mVertexId] += weight.mWeight;
                }
            }
            const auto rigid = inverseRoot * nodes[part.node].global;
            auto rigidNormal = aiMatrix3x3(rigid);
            rigidNormal.Inverse().Transpose();
            for (unsigned i = 0; i < mesh.mNumVertices; ++i) {
                if (weights[i] > 0) positions[i] /= weights[i];
                else {
                    positions[i] = rigid * mesh.mVertices[i];
                    normals[i] = rigidNormal * mesh.mNormals[i];
                }
            }
            for (unsigned f = 0; f < mesh.mNumFaces; ++f) {
                const auto &face = mesh.mFaces[f];
                if (face.mNumIndices != 3) continue;
                for (unsigned k = 0; k < 3; ++k) {
                    const auto i = face.mIndices[k];
                    const Vec2 uv = mesh.HasTextureCoords(0)
                        ? Vec2{mesh.mTextureCoords[0][i].x, mesh.mTextureCoords[0][i].y} : Vec2{};
                    output.push_back({(vector(positions[i]) - origin)*scale,
                        normalized(vector(normals[i])), uv});
                }
            }
        }
        // Impede que rotações dos gestos atravessem o tampo; preserva os saltos.
        if (grounded && !output.empty()) {
            float lowest = 0;
            for (const auto &vertex : output) lowest = std::min(lowest, vertex.position.y);
            for (auto &vertex : output) vertex.position.y -= lowest;
        }
    }
};

AnimatedModel::AnimatedModel(const std::string &path, float height) : impl(std::make_unique<Impl>()) {
    auto &m = *impl;
    // A paleta Cube Pets, por Kenney (https://kenney.nl, CC0), é carregada sem
    // inversão no stb_image; ajusta V para a orientação da imagem em memória.
    m.scene = m.importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
        aiProcess_ValidateDataStructure | aiProcess_FlipUVs);
    if (!m.scene || !m.scene->mRootNode || !m.scene->HasMeshes() || !m.scene->HasAnimations())
        throw std::runtime_error("Falha ao importar pet " + path + ": " + m.importer.GetErrorString());
    m.inverseRoot = m.scene->mRootNode->mTransformation;
    m.inverseRoot.Inverse();
    m.addNode(m.scene->mRootNode, -1);
    for (unsigned i = 0; i < m.scene->mNumAnimations; ++i) {
        const auto *a = m.scene->mAnimations[i];
        if (a->mDuration > 0) m.clips.emplace(a->mName.C_Str(), a);
    }
    for (const auto *name : {"static", "idle", "walk", "run", "eat", "dance",
                             "gesture-positive", "gesture-negative"})
        if (!m.clips.count(name)) throw std::runtime_error("Animação ausente no pet: " + std::string(name));
    // Os FBX compartilham uma paleta PNG externa; glTF pode incorporá-la.
    aiString texture;
    const auto *material = m.scene->mMaterials[m.parts.front().source->mMaterialIndex];
    if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texture) != AI_SUCCESS)
        material->GetTexture(aiTextureType_DIFFUSE, 0, &texture);
    const auto *embedded = m.scene->GetEmbeddedTexture(texture.C_Str());
    if (embedded && embedded->mHeight == 0) {
        const auto *bytes = reinterpret_cast<const unsigned char *>(embedded->pcData);
        m.png.assign(bytes, bytes + embedded->mWidth);
    } else {
        std::string relative = texture.C_Str();
        std::replace(relative.begin(), relative.end(), '\\', '/');
        const auto folder = arquivos::path(path).parent_path();
        auto image = folder / relative;
        if (!arquivos::is_regular_file(image))
            image = folder / "Textures" / arquivos::path(relative).filename();
        std::ifstream input(image.string(), std::ios::binary);
        if (!input) throw std::runtime_error("Falha ao abrir textura do pet: " + image.string());
        m.png.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }
    m.sample(m.clips.at("static"), 0);
    const float inf = std::numeric_limits<float>::infinity();
    Vec3 low{inf, inf, inf}, high{-inf, -inf, -inf};
    for (const auto &v : m.output) {
        const auto p = v.position;
        low = {std::min(low.x, p.x), std::min(low.y, p.y), std::min(low.z, p.z)};
        high = {std::max(high.x, p.x), std::max(high.y, p.y), std::max(high.z, p.z)};
    }
    if (!(height > 0) || !std::isfinite(high.y-low.y) || high.y <= low.y)
        throw std::runtime_error("Pet sem altura válida: " + path);
    m.origin = {(low.x+high.x)*.5f, low.y, (low.z+high.z)*.5f};
    m.scale = height / (high.y-low.y);
    m.grounded = true;
    sample("idle", 0);
}
AnimatedModel::~AnimatedModel() = default;
void AnimatedModel::sample(const std::string &clip, double seconds) {
    impl->sample(impl->clips.at(clip), seconds);
}
double AnimatedModel::duration(const std::string &clip) const {
    const auto *a = impl->clips.at(clip);
    return a->mDuration / (a->mTicksPerSecond > 0 ? a->mTicksPerSecond : 25);
}
const std::vector<Vertex> &AnimatedModel::vertices() const { return impl->output; }
const std::vector<unsigned char> &AnimatedModel::texturePng() const { return impl->png; }
