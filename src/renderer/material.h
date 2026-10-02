#pragma once

#include "renderer/texture.h"

#include <glm/glm.hpp>

struct Material {
    const Texture* diffuseMap;
    const Texture* specularMap;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    float reflectivity = 0.0f; // 0 = none, 1 = perfect mirror of the sky
    float refractivity = 0.0f; // 0 = opaque, 1 = see-through glass
    float ior = 1.52f;         // index of refraction: air 1.0, water 1.33, glass ~1.52
    bool doubleSided = false;  // thin surfaces seen from both sides: drawn without back-face culling
};

// Colors picked by eye are sRGB (gamma-encoded); lighting math needs linear values.
// 2.2 approximates the exact sRGB curve closely enough for picked colors.
inline glm::vec3 srgb(glm::vec3 color) {
    return glm::pow(color, glm::vec3(2.2f));
}

// Plastic: the surface color everywhere, plus a white-ish highlight (the light's color, not the surface's).
inline Material plastic(glm::vec3 color, float shininess = 128.0f) {
    return {nullptr, nullptr, srgb(color), glm::vec3(0.5f), shininess};
}

// Rubber: the surface color, almost no highlight, and what little there is is wide and dull.
inline Material rubber(glm::vec3 color) {
    return {nullptr, nullptr, srgb(color), glm::vec3(0.1f), 8.0f};
}
