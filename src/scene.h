#pragma once

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

    std::vector<Object> objects;

    DirLight sun;
    PointLight points[kMaxPointLights];
    SpotLight flashlight;
    bool orbitLights = true;

    // Objects point into this scene's own meshes and textures, so a copy would point back into the original.
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
};
