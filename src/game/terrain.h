#pragma once

#include "renderer/mesh.h"

#include <glm/glm.hpp>

// Hills made from Perlin noise. The terrain is always centered on the world origin: the Transform of its entity
// is not used. The game builds the mesh from these settings and builds it again when they change.
struct Terrain {
    float size = 40.0f;      // length of one side
    int resolution = 128;    // quads on one side
    float base = -0.5f;      // height of the ground where the noise is 0
    float amplitude = 1.5f;  // height of the hills
    float frequency = 0.08f; // a larger value gives smaller hills
    int octaves = 4;         // noise layers. More layers add small details.
    int seed = 0;            // a different seed gives different hills
    glm::vec3 color{0.42f, 0.5f, 0.3f}; // sRGB

    bool operator==(const Terrain&) const = default;
};

float heightAt(const Terrain& terrain, float x, float z);

Mesh makeTerrainMesh(const Terrain& terrain);
