#pragma once

#include "assets/model.h"
#include "core/camera.h"
#include "renderer/cubemap.h"
#include "renderer/light.h"
#include "renderer/mesh.h"
#include "renderer/texture.h"
#include "scene/components.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <string>

// Euler rotation (degrees) that points an entity's -Z axis along `direction`.
glm::vec3 aimRotation(glm::vec3 direction);

// The world: resources (meshes, textures, models) and the entities that use them. It starts with no entities,
// a scene is filled by buildDemo() or by the game.
struct Scene {
    Scene(); // loads the shared resources

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
    glm::vec3 position(entt::entity entity) const;
    // The direction the entity points: its local -Z axis in world space.
    glm::vec3 forward(entt::entity entity) const;

    // Per-frame systems that change the scene (rotators, entities following the camera).
    void update(float dt, const Camera& camera);

    // Components point into this scene's own meshes and textures, so a copy would point back into the original.
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
};
