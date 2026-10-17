#pragma once

#include "assets/model.h"
#include "renderer/material.h"
#include "renderer/mesh.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <string>

// Components are plain data. The logic that uses them lives in systems (the renderer, the game's update code).
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

// Tag: made while running (spawned enemies and the like), so saving a scene skips it.
struct Transient {};

// A view into the scene. Where it is and where it looks come from the entity's Transform: its position and
// its local -Z axis. Roll is not used: the view always keeps the world up direction up.
struct Camera {
    float fov = 45.0f;        // vertical field of view in degrees. Smaller = zoomed in
    float nearPlane = 0.1f;   // closer than this is not drawn
    float farPlane = 100.0f;  // farther than this is not drawn
};

// A sky made from sunlight scattered by the air, instead of the scene's sky texture. Put it on the entity with
// the DirLight: that light is the sun. Its rotation moves the sun in the sky, and the sky sets the light's
// colors (the sunlight that reaches the ground, and the light from the sky as ambient).
struct Sky {
    float sunIntensity = 10.0f;   // the brightness of the sky
    float sunLight = 1.0f;        // the strength of the sun's light on the objects when it is straight up
    float moonLight = 0.15f;      // the light at night, from opposite the sun
    float rayleigh = 1.0f;        // x the air's scattering: more = deeper blue, redder sunsets
    float mie = 0.15f;            // x the haze: more = whiter sky, bigger glow around the sun
    float sunSize = 2.5f;         // the sun disk, in degrees. The real sun is 0.53, which looks tiny on a screen
    float ambientStrength = 0.5f;  // x the light from the sky on the objects. Below 1: no ambient occlusion, so
                                   // the full sky light would also reach places the sky cannot see

    bool operator==(const Sky&) const = default;
};
