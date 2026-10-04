#pragma once

#include "scene/scene.h"

#include <glm/glm.hpp>

// Keeps the enemies apart. All shapes are circles on the ground plane.
// 1. Two enemies that overlap move apart, each by half of the overlap.
// 2. An enemy that overlaps the player moves out. The player does not move.
// 3. Each enemy stays inside the arena: a square around the origin with this half size.
void resolveEnemyCollisions(Scene& scene, glm::vec3 playerPosition, float playerRadius, float arenaHalfSize);
