#pragma once

#include "renderer/material.h"
#include "renderer/mesh.h"
#include "renderer/texture.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

// A 3D model loaded from a file (glTF, OBJ): a list of meshes, each with its own material.
// Static only: the file's node hierarchy is baked into the vertices at load time.
class Model {
public:
    struct Part {
        Mesh mesh;
        Material material;
    };

    // Textures come from the cache, so files shared between meshes or models load once.
    Model(const std::string& path, TextureCache& textures);

    std::vector<Part> parts;

    // Axis-aligned bounds of all vertices, in the model's own units.
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};

    // Uniform scale that makes the model's largest dimension `size` units.
    float fitScale(float size) const;
    // Bottom center of the bounds: the point that rests on the ground when placing the model.
    glm::vec3 base() const;
};
