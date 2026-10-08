#include "game/weapons.h"

#include "game/collision.h"
#include "game/game.h"

#include <algorithm>
#include <cmath>
#include <vector>

static constexpr float kHitFlashTime = 0.1f; // seconds that a hit enemy stays white
static constexpr float kBladeRadius = 0.3f;  // the size of an orbit blade for hits

bool updateWeapon(Scene& scene, entt::entity player, const Mesh& mesh, const Material& material, float dt) {
    entt::registry& registry = scene.registry;
    Weapon* weapon = registry.try_get<Weapon>(player);

    if (!weapon)
        return false;

    weapon->timer = std::min(weapon->timer + dt, weapon->interval);

    if (weapon->timer < weapon->interval)
        return false;

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
        return false;

    weapon->timer = 0.0f;

    const glm::vec3 muzzle = playerPosition + glm::vec3(0.0f, 0.5f, 0.0f);
    const glm::vec3 direction = glm::normalize(target->position - muzzle);
    const Projectile projectile{direction * weapon->projectileSpeed, weapon->damage, 2.0f, 0.1f};

    // Do not use the target pointer after this line. A new entity can move the transforms in memory.
    const entt::entity shot = scene.create("Projectile", {.position = muzzle, .scale = glm::vec3(0.2f)});
    registry.emplace<Projectile>(shot, projectile);
    registry.emplace<Transient>(shot); // Made while playing. The scene file does not save it.
    registry.emplace<MeshRenderer>(shot, &mesh, material);
    return true;
}

ProjectileResult updateProjectiles(Scene& scene, float dt) {
    entt::registry& registry = scene.registry;
    ProjectileResult result;

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
                enemy.flash = kHitFlashTime;
                result.hits++;
                finished.push_back(entity);
                break;
            }
        }
    }

    // Each dead enemy gives one death, even if two projectiles hit it in the same frame.
    for (auto [entity, transform, enemy, health] : registry.view<Transform, Enemy, Health>().each()) {
        if (health.current <= 0.0f) {
            finished.push_back(entity);
            result.deaths.push_back({transform.position, enemy.xpValue});
        }
    }

    std::sort(finished.begin(), finished.end());
    finished.erase(std::unique(finished.begin(), finished.end()), finished.end());
    registry.destroy(finished.begin(), finished.end());

    return result;
}

int updateOrbit(Scene& scene, entt::entity player, const Mesh& mesh, const Material& material, float dt) {
    entt::registry& registry = scene.registry;
    Orbit* orbit = registry.try_get<Orbit>(player);
    const int blades = orbit ? std::max(orbit->blades, 0) : 0;

    // When the number of blades changes (an upgrade, the editor), make all of them again.
    auto view = registry.view<OrbitBlade>();

    if (static_cast<int>(view.size()) != blades) {
        const std::vector<entt::entity> old(view.begin(), view.end()); // destroying while iterating the view is not safe
        registry.destroy(old.begin(), old.end());

        for (int i = 0; i < blades; i++) {
            const entt::entity blade = scene.create("Orbit blade", {.scale = {0.6f, 0.1f, 0.25f}});
            registry.emplace<OrbitBlade>(blade);
            registry.emplace<Transient>(blade);
            registry.emplace<MeshRenderer>(blade, &mesh, material);
        }
    }

    if (blades == 0)
        return 0;

    orbit->angle = std::fmod(orbit->angle + orbit->speed * dt, 360.0f);

    // The blades are root entities, so they do not turn when the player turns.
    const glm::vec3 center = registry.get<Transform>(player).position + glm::vec3(0.0f, 0.5f, 0.0f);
    std::vector<glm::vec3> positions;
    int i = 0;

    for (auto [entity, transform] : registry.view<OrbitBlade, Transform>().each()) {
        const float degrees = orbit->angle + 360.0f * static_cast<float>(i++) / static_cast<float>(blades);
        const float radians = glm::radians(degrees);
        transform.position = center + glm::vec3(std::cos(radians), 0.0f, std::sin(radians)) * orbit->radius;
        transform.rotation.y = -degrees - 90.0f; // the long side (local X) along the circle
        positions.push_back(transform.position);
    }

    int hits = 0;

    // Each enemy has its own wait time after a hit. Without it, a blade would hurt it in every frame of a touch.
    for (auto [entity, transform, enemy, health] : registry.view<Transform, Enemy, Health>().each()) {
        enemy.orbitTimer = std::max(0.0f, enemy.orbitTimer - dt);

        if (enemy.orbitTimer > 0.0f)
            continue;

        for (const glm::vec3& position : positions) {
            const glm::vec2 offset(transform.position.x - position.x, transform.position.z - position.z);

            if (glm::length(offset) < enemy.radius + kBladeRadius) {
                health.current -= orbit->damage;
                enemy.flash = kHitFlashTime;
                enemy.orbitTimer = orbit->hitInterval;
                hits++;
                break;
            }
        }
    }

    return hits;
}
