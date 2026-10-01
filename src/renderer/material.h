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
