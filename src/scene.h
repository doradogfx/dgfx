#pragma once

#include "cubemap.h"
#include "light.h"
#include "mesh.h"
#include "texture.h"

#include <glm/glm.hpp>

#include <vector>

struct Material {
    const Texture* diffuseMap;
    const Texture* specularMap;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    float reflectivity = 0.0f; // 0 = none, 1 = perfect mirror of the sky
    float refractivity = 0.0f; // 0 = opaque, 1 = see-through glass
    float ior = 1.52f;         // index of refraction: air 1.0, water 1.33, glass ~1.52
};

struct Object {
    const Mesh* mesh;
    glm::vec3 position;
    float yaw; // degrees around Y
    glm::vec3 scale;
    Material material;
};

// Everything in the world: the resources objects use, the objects, and the lights.
struct Scene {
    Scene(); // builds the demo scene

    Mesh cube;
    Mesh sphere;

    Texture white; // bound wherever a material has no map
    Texture crateDiffuse;
    Texture crateSpecular;

    Cubemap sky;

    std::vector<Object> objects;

    DirLight sun;
    PointLight points[kMaxPointLights];
    SpotLight flashlight;
    bool orbitLights = true;

    // Objects point into this scene's own meshes and textures, so a copy would point back into the original.
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
};
