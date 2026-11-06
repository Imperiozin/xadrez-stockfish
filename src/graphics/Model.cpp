#include "Mesh.h"
#include "stockfish.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <limits>
#include <stdexcept>

std::vector<Vertex> Mesh::loadModel(const std::string &file, float height) {
    const auto path = (pastaDoExecutavel() / file).string();
    Assimp::Importer importer;
    // Bake the object node hierarchy (including axis/unit transforms) before
    // combining meshes. Assimp also transforms normals into the same space.
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate |
        aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals |
        aiProcess_ValidateDataStructure);
    if (!scene || !scene->HasMeshes() || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
        throw std::runtime_error("Falha ao importar modelo " + path + ": " + importer.GetErrorString());

    std::vector<Vertex> vertices;
    const float infinity = std::numeric_limits<float>::infinity();
    Vec3 low{infinity, infinity, infinity}, high{-infinity, -infinity, -infinity};
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh &mesh = *scene->mMeshes[i];
        if (!mesh.HasNormals())
            throw std::runtime_error("Modelo sem normais: " + path);
        for (unsigned int f = 0; f < mesh.mNumFaces; ++f) {
            const aiFace &face = mesh.mFaces[f];
            if (face.mNumIndices != 3) continue;
            for (unsigned int index = 0; index < 3; ++index) {
                const auto v = face.mIndices[index];
                const auto &p = mesh.mVertices[v];
                const auto &n = mesh.mNormals[v];
                if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
                    !std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z))
                    throw std::runtime_error("Coordenadas invalidas no modelo: " + path);
                vertices.push_back({{p.x, p.y, p.z}, {n.x, n.y, n.z}});
                low = {std::min(low.x, p.x), std::min(low.y, p.y), std::min(low.z, p.z)};
                high = {std::max(high.x, p.x), std::max(high.y, p.y), std::max(high.z, p.z)};
            }
        }
    }
    const Vec3 extent = high - low;
    if (vertices.empty() || extent.y <= 0 || height <= 0)
        throw std::runtime_error("Modelo vazio ou sem altura: " + path);
    // Preserve proportions while keeping every piece inside its square and
    // selection ring. Rest its base on the top face of the board (y = .03).
    const float width = std::max(extent.x, extent.z);
    const float scale = std::min(height / extent.y, width > 0 ? .68f / width : height / extent.y);
    const Vec3 origin{(low.x + high.x) * .5f, low.y, (low.z + high.z) * .5f};
    for (auto &vertex : vertices)
        vertex.position = (vertex.position - origin) * scale;
    return vertices;
}
