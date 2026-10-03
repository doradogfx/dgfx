#pragma once

#include "renderer/cubemap.h"
#include "renderer/light.h"
#include "scene/components.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <string>

// Euler rotation (degrees) that points an entity's -Z axis along `direction`.
glm::vec3 aimRotation(glm::vec3 direction);

// The world: the entities, plus the skybox. Meshes, textures and models live in Assets, which components point
// into. It starts empty, a scene is filled by loading a scene file.
struct Scene {
    // All entities and their components.
    entt::registry registry;

    const Cubemap* sky = nullptr; // drawn behind the scene and reflected by shiny materials; none = no sky

    // A new entity with a Name and a Transform.
    entt::entity create(const std::string& name, const Transform& transform = {});

    // Local transform combined with every parent's, i.e. where the entity is in the world.
    glm::mat4 worldMatrix(entt::entity entity) const;
    glm::vec3 position(entt::entity entity) const;
    // The direction the entity points: its local -Z axis in world space.
    glm::vec3 forward(entt::entity entity) const;

    // Destroys the entity and everything parented under it.
    void destroy(entt::entity entity);
};
