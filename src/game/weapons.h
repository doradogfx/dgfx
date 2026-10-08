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

// Blades that turn around the player and hurt the enemies they touch. Put it on the player entity.
// With 0 blades, the weapon is off. A level-up upgrade adds blades.
struct Orbit {
    int blades = 0;
    float radius = 1.2f;      // distance from the player. Enemies stop 0.8 away, so the blades must reach them there.
    float speed = 180.0f;     // degrees per second
    float damage = 8.0f;      // health that one touch removes
    float hitInterval = 0.5f; // seconds before the same enemy can take damage again
    float angle = 0.0f;       // where the first blade is now. Not saved.
};

// Tag on each blade entity. The game makes the blades, the scene file does not save them.
struct OrbitBlade {};

// Fires the weapon at the nearest enemy in range when the weapon is ready. Returns true when it fires.
bool updateWeapon(Scene& scene, entt::entity player, const Mesh& mesh, const Material& material, float dt);

// Where an enemy died, and how much XP it gives.
struct Death {
    glm::vec3 position;
    float xp;
};

struct ProjectileResult {
    std::vector<Death> deaths; // one for each enemy that died
    int hits = 0;              // projectiles that hit an enemy
};

// Moves the projectiles and applies the hits. Destroys the finished projectiles and the dead enemies.
ProjectileResult updateProjectiles(Scene& scene, float dt);

// Moves the orbit blades and applies their hits. The dead enemies stay until updateProjectiles removes them.
// Returns the number of hits.
int updateOrbit(Scene& scene, entt::entity player, const Mesh& mesh, const Material& material, float dt);
