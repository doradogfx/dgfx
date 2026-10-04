#include "assets/model.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

static const Texture* loadMap(const aiMaterial* m, aiTextureType type, const std::string& dir, bool srgb, TextureCache& textures) {
    aiString file;

    if (m->GetTexture(type, 0, &file) != AI_SUCCESS)
        return nullptr;

    return &textures.get(dir + file.C_Str(), srgb);
}

static Material loadMaterial(const aiMaterial* m, const std::string& dir, TextureCache& textures) {
    Material material{};
    material.diffuseMap = loadMap(m, aiTextureType_DIFFUSE, dir, true, textures);
    material.specularMap = loadMap(m, aiTextureType_SPECULAR, dir, false, textures);

    // glTF colors are already linear, unlike colors picked by eye, so no sRGB conversion here.
    aiColor3D diffuse(1.0f, 1.0f, 1.0f);
    m->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
    material.diffuse = {diffuse.r, diffuse.g, diffuse.b};

    aiColor3D specular(0.0f, 0.0f, 0.0f);
    m->Get(AI_MATKEY_COLOR_SPECULAR, specular);
    material.specular = {specular.r, specular.g, specular.b};

    // Sketchfab-style exports often write a zero specular factor next to a specular map that is clearly meant
    // to be used. Per the glTF spec the factor would switch the map off; use the map with a neutral tint instead.
    if (material.specularMap && material.specular == glm::vec3(0.0f))
        material.specular = glm::vec3(0.5f);

    // Glossiness 0..1 -> Blinn-Phong exponent 2..2048, exponential like the perceived highlight size.
    float glossiness;
    float shininess = 0.0f;

    if (m->Get(AI_MATKEY_GLOSSINESS_FACTOR, glossiness) == AI_SUCCESS)
        shininess = std::exp2(1.0f + 10.0f * glossiness);
    else
        m->Get(AI_MATKEY_SHININESS, shininess);

    material.shininess = shininess > 0.0f ? shininess : 32.0f; // OBJ files often say 0

    int twoSided = 0;
    m->Get(AI_MATKEY_TWOSIDED, twoSided);
    material.doubleSided = twoSided != 0;

    return material;
}

Model::Model(const std::string& path, TextureCache& textures) {
    Assimp::Importer importer;

    // PreTransformVertices bakes each node's transform into its vertices, turning the file's hierarchy into a
    // flat list of meshes already in place. No FlipUVs: Texture flips images on load, which matches Assimp's
    // bottom-left UV origin (its glTF importer converts glTF's top-left UVs).
    const unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices;
    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::fprintf(stderr, "Failed to load model %s: %s\n", path.c_str(), importer.GetErrorString());
        return;
    }

    // Texture paths in the file are relative to the model's folder.
    const std::string dir = path.substr(0, path.find_last_of("/\\") + 1);

    std::vector<Material> materials;

    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
        materials.push_back(loadMaterial(scene->mMaterials[i], dir, textures));

    boundsMin = glm::vec3(std::numeric_limits<float>::max());
    boundsMax = glm::vec3(std::numeric_limits<float>::lowest());

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        const aiMesh* mesh = scene->mMeshes[i];
        std::vector<Vertex> vertices(mesh->mNumVertices);

        for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
            const aiVector3D& p = mesh->mVertices[v];
            const aiVector3D& n = mesh->mNormals[v];
            vertices[v].position = {p.x, p.y, p.z};
            vertices[v].normal = {n.x, n.y, n.z};

            if (mesh->HasTextureCoords(0))
                vertices[v].uv = {mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y};

            boundsMin = glm::min(boundsMin, vertices[v].position);
            boundsMax = glm::max(boundsMax, vertices[v].position);
        }

        std::vector<unsigned int> indices;
        indices.reserve(mesh->mNumFaces * 3);

        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            const aiFace& face = mesh->mFaces[f];
            indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
        }

        parts.push_back({Mesh(vertices, indices), materials[mesh->mMaterialIndex]});
    }
}

glm::vec3 Model::base() const {
    return {(boundsMin.x + boundsMax.x) * 0.5f, boundsMin.y, (boundsMin.z + boundsMax.z) * 0.5f};
}
