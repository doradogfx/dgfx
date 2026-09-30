#pragma once

#include "cubemap.h"
#include "light.h"
#include "material.h"
#include "mesh.h"
#include "model.h"
#include "texture.h"

#include <glm/glm.hpp>

#include <vector>

struct Object {
    const Mesh* mesh;
    glm::vec3 position;
    float yaw; // degrees around Y
    glm::vec3 scale;
    Material material;
};

// A loaded model placed in the world. Each part keeps its own material from the file.
struct ModelInstance {
    const Model* model;
    glm::vec3 position; // where the model's base (bottom center) rests
    float yaw;          // degrees around Y
    float scale;        // uniform, see Model::fitScale
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

    TextureCache textures; // declared before the models, so it outlives them
    Model shiba;

    std::vector<Object> objects;
    std::vector<ModelInstance> models;

    DirLight sun;
    PointLight points[kMaxPointLights];
    SpotLight flashlight;
    bool orbitLights = true;

    // Objects point into this scene's own meshes and textures, so a copy would point back into the original.
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
};
