#include "game/weapons.h"

#include "game/collision.h"
#include "game/game.h"

#include <algorithm>
#include <vector>

void updateWeapon(Scene& scene, entt::entity player, const Mesh& mesh, const Material& material, float dt) {
    entt::registry& registry = scene.registry;
    Weapon* weapon = registry.try_get<Weapon>(player);

    if (!weapon)
        return;

    weapon->timer = std::min(weapon->timer + dt, weapon->interval);

    if (weapon->timer < weapon->interval)
        return;

    const glm::vec3 playerPosition = registry.get<Transform>(player).position;

    // Find the nearest enemy in range. The distance is on the ground plane.
    const Transform* target = nullptr;
    float nearest = weapon->range;

    for (auto [entity, transform, enemy] : registry.view<Transform, Enemy>().each()) {
        glm::vec3 offset = transform.position - playerPosition;
        offset.y = 0.0f;
        const float distance = glm::length(offset);

        if (distance < nearest) {
            nearest = distance;
            target = &transform;
        }
    }

    // With no target, the timer stays full. The weapon fires when the first enemy comes in range.
    if (!target)
        return;

    weapon->timer = 0.0f;

    const glm::vec3 muzzle = playerPosition + glm::vec3(0.0f, 0.5f, 0.0f);
    const glm::vec3 direction = glm::normalize(target->position - muzzle);
    const Projectile projectile{direction * weapon->projectileSpeed, weapon->damage, 2.0f, 0.1f};

    // Do not use the target pointer after this line. A new entity can move the transforms in memory.
    const entt::entity shot = scene.create("Projectile", {.position = muzzle, .scale = glm::vec3(0.2f)});
    registry.emplace<Projectile>(shot, projectile);
    registry.emplace<Transient>(shot); // Made while playing. The scene file does not save it.
    registry.emplace<MeshRenderer>(shot, &mesh, material);
}

std::vector<Death> updateProjectiles(Scene& scene, float dt) {
    entt::registry& registry = scene.registry;

    // The entities to destroy. Destroy them after the loops, because destroying inside a loop breaks the loop.
    std::vector<entt::entity> finished;

    // ponytail: this tests each projectile against each enemy. Use a spatial grid above a few thousand pairs.
    for (auto [entity, transform, projectile] : registry.view<Transform, Projectile>().each()) {
        transform.position += projectile.velocity * dt;
        projectile.lifetime -= dt;

        if (projectile.lifetime <= 0.0f || insideObstacle(registry, transform.position)) {
            finished.push_back(entity);
            continue;
        }

        for (auto [enemyEntity, enemyTransform, enemy, health] : registry.view<Transform, Enemy, Health>().each()) {
            if (glm::distance(enemyTransform.position, transform.position) < enemy.radius + projectile.radius) {
                health.current -= projectile.damage;
                finished.push_back(entity);
                break;
            }
        }
    }

    // Each dead enemy gives one death, even if two projectiles hit it in the same frame.
    std::vector<Death> deaths;

    for (auto [entity, transform, enemy, health] : registry.view<Transform, Enemy, Health>().each()) {
        if (health.current <= 0.0f) {
            finished.push_back(entity);
            deaths.push_back({transform.position, enemy.xpValue});
        }
    }

    std::sort(finished.begin(), finished.end());
    finished.erase(std::unique(finished.begin(), finished.end()), finished.end());
    registry.destroy(finished.begin(), finished.end());

    return deaths;
}
