#pragma once

#include "assets/model.h"
#include "renderer/cubemap.h"
#include "renderer/light.h"
#include "renderer/mesh.h"
#include "renderer/texture.h"
#include "scene/components.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <string>

// The world: resources (meshes, textures, models) and the entities that use them.
struct Scene {
    Scene(); // builds the demo scene

    Mesh cube;
    Mesh sphere;

    Texture white; // bound wherever a material has no map
    Texture crateDiffuse;
    Texture crateSpecular;

    Cubemap sky;

    TextureCache textures; // declared before the models, so it outlives them
    Model shiba;

    // All entities and their components.
    entt::registry registry;

    // A new entity with a Name and a Transform.
    entt::entity create(const std::string& name, const Transform& transform = {});

    // Local transform combined with every parent's, i.e. where the entity is in the world.
    glm::mat4 worldMatrix(entt::entity entity) const;

    // Per-frame systems that change the scene (rotators).
    void update(float dt);

    // Components point into this scene's own meshes and textures, so a copy would point back into the original.
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
};
