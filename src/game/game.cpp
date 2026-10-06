#include "game/game.h"

#include "game/collision.h"
#include "game/weapons.h"
#include "game/xp.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

static constexpr float kGroundHeight = -0.5f; // top of the floor
static constexpr float kGravity = 20.0f;      // stronger than real gravity: snappier jumps
static constexpr float kArenaHalfSize = 19.0f; // inside the 40x40 floor
static constexpr float kPlayerRadius = 0.4f;
static constexpr float kEnemyScale = 0.8f;

Game::Game(Scene& scene, Assets& assets) {
    enemyMesh = &assets.mesh("sphere");

    bool found = false;

    for (auto [entity, p] : scene.registry.view<Player>().each()) {
        player = entity;
        found = true;
    }

    if (!found)
        throw std::runtime_error("The level has no Player entity");
}

void Game::update(Scene& scene, Camera& camera, const Input& input, float dt) {
    // The player can be deleted from the editor UI.
    if (!scene.registry.valid(player) || !scene.registry.all_of<Player, Health>(player))
        return;

    // Paused while a level-up waits for a choice.
    if (pendingLevelUps > 0)
        return;

    Transform& transform = scene.registry.get<Transform>(player);
    Player& p = scene.registry.get<Player>(player);

    const float sensitivity = 0.1f; // degrees per pixel
    cameraYaw += input.look.x * sensitivity;
    cameraPitch = std::clamp(cameraPitch - input.look.y * sensitivity, -70.0f, 20.0f);
    cameraDistance = std::clamp(cameraDistance - input.scroll * 0.5f, 2.0f, 12.0f);

    // WASD relative to where the camera looks, flattened onto the ground.
    const float yaw = glm::radians(cameraYaw);
    const glm::vec3 forward(std::cos(yaw), 0.0f, std::sin(yaw));
    const glm::vec3 right(-forward.z, 0.0f, forward.x);
    glm::vec3 move(0.0f);

    if (input.down(GLFW_KEY_W))
        move += forward;
    if (input.down(GLFW_KEY_S))
        move -= forward;
    if (input.down(GLFW_KEY_D))
        move += right;
    if (input.down(GLFW_KEY_A))
        move -= right;

    if (glm::length(move) > 0.0f) {
        move = glm::normalize(move); // or diagonals would be faster
        transform.position += move * p.speed * dt;

        // Turn towards the move direction (the entity faces -Z) the short way round, easing in.
        const float target = glm::degrees(std::atan2(-move.x, -move.z));
        const float diff = std::fmod(target - transform.rotation.y + 540.0f, 360.0f) - 180.0f;
        transform.rotation.y += diff * std::min(1.0f, 12.0f * dt);
    }

    if (p.grounded && input.down(GLFW_KEY_SPACE)) {
        p.verticalVelocity = p.jumpSpeed;
        p.grounded = false;
    }

    p.verticalVelocity -= kGravity * dt;
    transform.position.y += p.verticalVelocity * dt;

    if (transform.position.y <= kGroundHeight) {
        transform.position.y = kGroundHeight;
        p.verticalVelocity = 0.0f;
        p.grounded = true;
    }

    transform.position.x = std::clamp(transform.position.x, -kArenaHalfSize, kArenaHalfSize);
    transform.position.z = std::clamp(transform.position.z, -kArenaHalfSize, kArenaHalfSize);

    // Orbit: look at a point above the feet from `cameraDistance` behind it, never below the floor.
    camera.yaw = cameraYaw;
    camera.pitch = cameraPitch;
    const glm::vec3 target = transform.position + glm::vec3(0.0f, 1.0f, 0.0f);
    camera.position = target - camera.front() * cameraDistance;
    camera.position.y = std::max(camera.position.y, kGroundHeight + 0.2f);

    updateEnemies(scene, dt);
    updateWeapon(scene, player, *enemyMesh, projectileMaterial, dt);

    // Each dead enemy drops an orb on the floor where it died.
    for (const Death& death : updateProjectiles(scene, dt)) {
        kills++;
        spawnOrb(scene, *enemyMesh, orbMaterial, {death.position.x, kGroundHeight + 0.2f, death.position.z}, death.xp);
    }

    const int levels = updateOrbs(scene, player, kPlayerRadius, dt);

    if (levels > 0 && pendingLevelUps == 0)
        rollChoices();

    pendingLevelUps += levels;
}

void Game::updateEnemies(Scene& scene, float dt) {
    entt::registry& registry = scene.registry;
    const glm::vec3 playerPos = registry.get<Transform>(player).position;
    Health& health = registry.get<Health>(player);

    for (auto [entity, spawner] : registry.view<Spawner>().each()) {
        spawner.timer += dt;

        if (spawner.timer < spawner.interval)
            continue;

        spawner.timer = 0.0f;

        if (static_cast<int>(registry.view<Enemy>().size()) >= spawner.maxEnemies)
            continue;

        // Spawn enemy in a random position around the player in a radius
        const float angle = std::uniform_real_distribution<float>(0.0f, 6.2831853f)(rng);
        glm::vec3 position = playerPos + glm::vec3(std::cos(angle), 0.0f, std::sin(angle)) * spawner.ringRadius;
        position.x = std::clamp(position.x, -kArenaHalfSize, kArenaHalfSize);
        position.z = std::clamp(position.z, -kArenaHalfSize, kArenaHalfSize);
        position.y = kGroundHeight + 0.5f * kEnemyScale; // sphere radius is 0.5

        const entt::entity enemy = scene.create("Enemy", {.position = position, .scale = glm::vec3(kEnemyScale)});
        registry.emplace<Enemy>(enemy);
        registry.emplace<Health>(enemy, Health{spawner.enemyHealth, spawner.enemyHealth});
        registry.emplace<Transient>(enemy); // spawned while playing, not part of the level file
        registry.emplace<MeshRenderer>(enemy, enemyMesh, enemyMaterial);
    }

    // The enemies follow the player.
    for (auto [entity, transform, enemy] : registry.view<Transform, Enemy>().each()) {
        glm::vec3 toPlayer = playerPos - transform.position;
        toPlayer.y = 0.0f;
        const float distance = glm::length(toPlayer);

        if (distance > 0.001f)
            transform.position += toPlayer / distance * std::min(enemy.speed * dt, distance);
    }

    resolveEnemyCollisions(scene, playerPos, kPlayerRadius, kArenaHalfSize);

    // An enemy that touches the player removes health. The collision step moves it out to exactly the sum of
    // the two radii, so the test adds a small margin.
    for (auto [entity, transform, enemy] : registry.view<Transform, Enemy>().each()) {
        glm::vec3 toPlayer = playerPos - transform.position;
        toPlayer.y = 0.0f;

        if (glm::length(toPlayer) < enemy.radius + kPlayerRadius + 0.1f)
            health.current -= enemy.damage * dt;
    }

    // Placeholder game over
    if (health.current <= 0.0f) {
        health.current = health.max;
        kills = 0;
        pendingLevelUps = 0;

        std::vector<entt::entity> enemies; // and the projectiles and the orbs

        for (entt::entity entity : registry.view<Enemy>())
            enemies.push_back(entity);

        for (entt::entity entity : registry.view<Projectile>())
            enemies.push_back(entity);

        for (entt::entity entity : registry.view<XpOrb>())
            enemies.push_back(entity);

        if (Experience* experience = registry.try_get<Experience>(player)) {
            experience->level = 1;
            experience->xp = 0.0f;
        }

        registry.destroy(enemies.begin(), enemies.end());
    }
}

void Game::rollChoices() {
    std::array<Upgrade, static_cast<size_t>(Upgrade::Count)> all;

    for (size_t i = 0; i < all.size(); i++)
        all[i] = static_cast<Upgrade>(i);

    std::shuffle(all.begin(), all.end(), rng);
    std::copy_n(all.begin(), choices.size(), choices.begin());
}

void Game::choose(Scene& scene, int index) {
    if (pendingLevelUps == 0)
        return;

    applyUpgrade(scene, player, choices[index]);
    pendingLevelUps--;

    if (pendingLevelUps > 0)
        rollChoices();
}
