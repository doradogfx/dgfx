#pragma once

#include "assets/model.h"
#include "renderer/material.h"
#include "renderer/mesh.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <string>

// Components are plain data. The logic that uses them lives in systems (the renderer, Scene::update).
// Lights reuse DirLight, PointLight and SpotLight from light.h as components.

struct Name {
    std::string value;
};

// Local transform, relative to the parent (or the world, for a root entity).
struct Transform {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f}; // Euler angles in degrees: pitch (X), yaw (Y), roll (Z)
    glm::vec3 scale{1.0f};
    entt::entity parent = entt::null;
};

struct MeshRenderer {
    const Mesh* mesh;
    Material material;
};

// Each part of the model keeps the material it was loaded with.
struct ModelRenderer {
    const Model* model;
};

// Spins the entity's Transform continuously.
struct Rotator {
    glm::vec3 degreesPerSecond{0.0f};
    bool enabled = true;
};

// Tag: the entity's transform is set to the camera's every frame (e.g. a flashlight). Root entities only:
// it writes the local transform, so under a parent it would be offset by the parent's.
struct FollowCamera {};
