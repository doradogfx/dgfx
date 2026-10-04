#pragma once

#include "renderer/material.h"
#include "renderer/mesh.h"
#include "scene/scene.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <vector>

// The player weapon. It shoots at the nearest enemy by itself. Put it on the player entity.
struct Weapon {
    float damage = 10.0f;          // health that one hit removes
    float interval = 0.5f;         // seconds between two shots
    float range = 15.0f;           // distance to the farthest target
    float projectileSpeed = 14.0f; // units per second
    float timer = 0.0f;            // counts up to the interval
};

// One shot in flight. It moves in a straight line until it hits an enemy or its lifetime ends.
struct Projectile {
    glm::vec3 velocity{0.0f};
    float damage = 0.0f;
    float lifetime = 2.0f; // seconds left
    float radius = 0.1f;
};

// Fires the weapon at the nearest enemy in range when the weapon is ready.
void updateWeapon(Scene& scene, entt::entity player, const Mesh& mesh, const Material& material, float dt);

// Where an enemy died, and how much XP it gives.
struct Death {
    glm::vec3 position;
    float xp;
};

// Moves the projectiles and applies the hits. Destroys the finished projectiles and the dead enemies.
// Returns one Death for each enemy that died.
std::vector<Death> updateProjectiles(Scene& scene, float dt);
