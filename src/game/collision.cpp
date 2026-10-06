#include "game/collision.h"

#include "game/game.h"

#include <algorithm>
#include <vector>

void resolveEnemyCollisions(Scene& scene, glm::vec3 playerPosition, float playerRadius, float arenaHalfSize) {
    struct Body {
        Transform* transform;
        float radius;
    };

    std::vector<Body> bodies;

    for (auto [entity, transform, enemy] : scene.registry.view<Transform, Enemy>().each())
        bodies.push_back({&transform, enemy.radius});

    // ponytail: this tests every pair. Use a spatial grid above a few hundred enemies.
    for (size_t i = 0; i < bodies.size(); i++) {
        for (size_t j = i + 1; j < bodies.size(); j++) {
            glm::vec3 offset = bodies[j].transform->position - bodies[i].transform->position;
            offset.y = 0.0f;
            const float distance = glm::length(offset);
            const float minDistance = bodies[i].radius + bodies[j].radius;

            if (distance >= minDistance)
                continue;

            // Two enemies at the same point have no direction. Use a fixed one.
            const glm::vec3 direction = distance > 0.0001f ? offset / distance : glm::vec3(1.0f, 0.0f, 0.0f);
            const glm::vec3 push = direction * ((minDistance - distance) * 0.5f);
            bodies[i].transform->position -= push;
            bodies[j].transform->position += push;
        }
    }

    for (const Body& body : bodies) {
        glm::vec3& position = body.transform->position;

        glm::vec3 fromPlayer = position - playerPosition;
        fromPlayer.y = 0.0f;
        const float distance = glm::length(fromPlayer);
        const float minDistance = body.radius + playerRadius;

        if (distance < minDistance && distance > 0.0001f)
            position += fromPlayer / distance * (minDistance - distance);

        pushOutOfObstacles(scene.registry, position, body.radius);

        position.x = std::clamp(position.x, -arenaHalfSize, arenaHalfSize);
        position.z = std::clamp(position.z, -arenaHalfSize, arenaHalfSize);
    }
}

void pushOutOfObstacles(const entt::registry& registry, glm::vec3& position, float radius) {
    for (auto [entity, transform, obstacle] : registry.view<const Transform, const Obstacle>().each()) {
        glm::vec3 offset = position - transform.position;
        offset.y = 0.0f;
        const float distance = glm::length(offset);
        const float minDistance = obstacle.radius + radius;

        if (distance < minDistance && distance > 0.0001f)
            position += offset / distance * (minDistance - distance);
    }
}

bool insideObstacle(const entt::registry& registry, glm::vec3 point) {
    for (auto [entity, transform, obstacle] : registry.view<const Transform, const Obstacle>().each()) {
        const glm::vec2 offset(point.x - transform.position.x, point.z - transform.position.z);

        if (glm::length(offset) < obstacle.radius && point.y < obstacle.top)
            return true;
    }

    return false;
}
