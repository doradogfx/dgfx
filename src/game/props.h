#pragma once

#include "game/terrain.h"
#include "renderer/mesh.h"
#include "scene/scene.h"

#include <glm/glm.hpp>

// Rocks and trees on the terrain. Put it on the Terrain entity. The game makes the props at the start of a run,
// from the terrain seed, and does not save them.
struct Props {
    int rocks = 25;
    int trees = 15;
    float clearRadius = 4.0f; // no props this close to the player start

    bool operator==(const Props&) const = default;
};

// Makes the props. Each one gets an Obstacle, so it blocks the player, the enemies and the shots.
void spawnProps(Scene& scene, const Props& props, const Terrain& terrain, glm::vec3 playerStart, const Mesh& cube,
                const Mesh& sphere);
