#pragma once

#include "renderer/shader.h"

#include <glm/glm.hpp>

#include <span>

// Must match MAX_POINT_LIGHTS in lit.frag.
constexpr int kMaxPointLights = 4;

// The sun: parallel rays, no position, no falloff.
struct DirLight {
    bool enabled = true;
    glm::vec3 direction{-0.2f, -1.0f, -0.3f};
    glm::vec3 ambient{0.05f};
    glm::vec3 diffuse{0.3f};
    glm::vec3 specular{0.3f};
};

// A bulb: shines everywhere from a position, fading with distance as 1 / (constant + linear*d + quadratic*d^2).
struct PointLight {
    bool enabled = true;
    glm::vec3 position{0.0f}; // world space, filled in from the entity's transform when rendering
    glm::vec3 ambient{0.0f};
    glm::vec3 diffuse{1.0f};
    glm::vec3 specular{1.0f};
    float constant = 1.0f;
    float linear = 0.35f;
    float quadratic = 0.44f;
};

// A point light limited to a cone, with a soft edge between the inner and outer angle (degrees).
struct SpotLight {
    bool enabled = false;
    glm::vec3 position{0.0f}; // world space, filled in when rendering (the flashlight follows the camera)
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
    glm::vec3 ambient{0.0f};
    glm::vec3 diffuse{1.0f};
    glm::vec3 specular{1.0f};
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float innerAngle = 12.5f;
    float outerAngle = 17.5f;
};

// Attenuation factors for a light reaching about `name` units, from the Ogre3D table.
struct AttenuationPreset {
    const char* name;
    float linear;
    float quadratic;
};

inline constexpr AttenuationPreset kAttenuationPresets[] = {
    {"7", 0.7f, 1.8f}, {"13", 0.35f, 0.44f}, {"20", 0.22f, 0.20f},
    {"32", 0.14f, 0.07f}, {"50", 0.09f, 0.032f}, {"100", 0.045f, 0.0075f},
};

// Sends all lights to the shader. Only enabled point lights are sent, packed at the front of the array.
void setLights(const Shader& shader, const DirLight& dir, std::span<const PointLight> points, const SpotLight& spot);

// ImGui widgets for editing a light.
void lightUI(DirLight& light);
void lightUI(PointLight& light);
void lightUI(SpotLight& light);
