#pragma once

#include "renderer/shader.h"

#include <glm/glm.hpp>

// Light components only describe the light. Where it is and where it points come from the entity's
// transform: position, and its local -Z axis as the direction.

// The sun: parallel rays, no position, no falloff.
struct DirLight {
    bool enabled = true;
    glm::vec3 ambient{0.05f};
    glm::vec3 diffuse{0.3f};
    glm::vec3 specular{0.3f};
};

// A bulb: shines everywhere from a position, fading with distance as 1 / (constant + linear*d + quadratic*d^2).
struct PointLight {
    bool enabled = true;
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

// A light placed in the world for this frame: the component plus its world-space position/direction.
struct WorldDirLight {
    DirLight light;
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
};

struct WorldPointLight {
    PointLight light;
    glm::vec3 position{0.0f};
};

struct WorldSpotLight {
    SpotLight light;
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
};

// One point or spot light as lit.frag reads it from the storage buffer (std430: each vec3 shares 16 bytes with
// the float after it). A point light is a spot light with a cone wider than the sphere.
struct GpuLight {
    glm::vec3 position;
    float constant;
    glm::vec3 direction;
    float linear;
    glm::vec3 ambient;
    float quadratic;
    glm::vec3 diffuse;
    float cutOff; // cos of the inner angle
    glm::vec3 specular;
    float outerCutOff; // cos of the outer angle
};
static_assert(sizeof(GpuLight) == 80, "must match struct Light in lit.frag");

GpuLight toGpu(const WorldPointLight& point);
GpuLight toGpu(const WorldSpotLight& spot);

// Sends the sun to the shader as uniforms.
void setSun(const Shader& shader, const WorldDirLight& dir);

// ImGui widgets for editing a light.
void lightUI(DirLight& light);
void lightUI(PointLight& light);
void lightUI(SpotLight& light);
