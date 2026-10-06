#pragma once

#include "scene/scene.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>

// A static circle on the ground plane that blocks the player, the enemies and the shots. Put it on a root
// entity only: the code reads Transform::position as the world position.
struct Obstacle {
    float radius = 0.5f;
    float top = 0.0f; // world height of the top. Shots above it fly over.
};

// Keeps the enemies apart. All shapes are circles on the ground plane.
// 1. Two enemies that overlap move apart, each by half of the overlap.
// 2. An enemy that overlaps the player moves out. The player does not move.
// 3. An enemy that overlaps an obstacle moves out.
// 4. Each enemy stays inside the arena: a square around the origin with this half size.
void resolveEnemyCollisions(Scene& scene, glm::vec3 playerPosition, float playerRadius, float arenaHalfSize);

// Moves a circle out of each obstacle that it overlaps.
// ponytail: no pathfinding. A body slides around an obstacle, but it can stop behind one when it comes straight at
// it. Add steering if enemies get stuck.
void pushOutOfObstacles(const entt::registry& registry, glm::vec3& position, float radius);

// True if the point is inside an obstacle circle and below its top.
bool insideObstacle(const entt::registry& registry, glm::vec3 point);
