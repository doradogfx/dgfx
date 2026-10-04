#pragma once

#include "renderer/material.h"
#include "renderer/mesh.h"
#include "scene/scene.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>

// The player experience. Put it on the player entity.
struct Experience {
    int level = 1;             // starts at 1
    float xp = 0.0f;           // XP towards the next level
    float base = 5.0f;         // XP for level 1 to level 2
    float step = 3.0f;         // each level needs this much more XP than the one before
    float pickupRadius = 3.0f; // an orb inside this distance flies to the player
    float pickupSpeed = 10.0f; // units per second
};

// A pickup that gives XP. An enemy drops one when it dies.
struct XpOrb {
    float value = 1.0f;
};

// The XP that the player needs to go from the current level to the next level.
inline float xpToNext(const Experience& experience) {
    return experience.base + experience.step * static_cast<float>(experience.level - 1);
}

void spawnOrb(Scene& scene, const Mesh& mesh, const Material& material, glm::vec3 position, float value);

// Moves the orbs that are close to the player, and takes the orbs that touch the player.
// Returns the number of levels that the player gained.
int updateOrbs(Scene& scene, entt::entity player, float playerRadius, float dt);
