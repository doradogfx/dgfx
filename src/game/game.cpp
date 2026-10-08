#include "game/game.h"

#include "game/collision.h"
#include "game/weapons.h"
#include "game/xp.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

static constexpr float kGravity = 20.0f;      // stronger than real gravity: snappier jumps
static constexpr float kArenaHalfSize = 19.0f; // inside the 40x40 terrain
static constexpr float kPlayerRadius = 0.4f;
static constexpr float kEnemyScale = 0.8f;

Game::Game(Scene& scene, Assets& assets) {
    enemyMesh = &assets.mesh("sphere");
    cubeMesh = &assets.mesh("cube");

    bool found = false;

    for (auto [entity, p] : scene.registry.view<Player>().each()) {
        player = entity;
        found = true;
    }

    if (!found)
        throw std::runtime_error("The level has no Player entity");

    playerStart = scene.registry.get<Transform>(player).position;

    // New hills and props each run. Perlin noise repeats every 256 units, so a larger seed gives no new hills.
    for (auto [entity, t] : scene.registry.view<Terrain>().each())
        t.seed = static_cast<int>(std::random_device{}() % 256);

    updateWorld(scene);
}

void Game::updateWorld(Scene& scene) {
    entt::registry& registry = scene.registry;
    const Terrain* terrainSettings = nullptr;
    const Props* propSettings = nullptr;

    for (auto [entity, t] : registry.view<Terrain>().each())
        terrainSettings = &t;

    for (auto [entity, p] : registry.view<Props>().each())
        propSettings = &p;

    // The editor can delete the mesh entity. Then make it again.
    const bool meshMissing = !registry.valid(terrainMeshEntity);
    const bool terrainChanged = terrainSettings && (!(*terrainSettings == terrain) || !terrainMesh || meshMissing);

    if (terrainChanged) {
        terrain = *terrainSettings;
        terrainMesh = std::make_unique<Mesh>(makeTerrainMesh(terrain));

        Material material = plastic(terrain.color, 8.0f);
        material.specular = glm::vec3(0.1f);

        if (meshMissing) {
            terrainMeshEntity = scene.create("Terrain mesh");
            registry.emplace<Transient>(terrainMeshEntity);
        }

        registry.emplace_or_replace<MeshRenderer>(terrainMeshEntity, terrainMesh.get(), material);
    }

    // The props sit on the terrain, so they move with it too.
    if (!propSettings || (!terrainChanged && propsMade && *propSettings == props))
        return;

    props = *propSettings;
    propsMade = true;

    std::vector<entt::entity> old;

    for (auto [entity, obstacle] : registry.view<Obstacle>().each())
        old.push_back(entity);

    for (entt::entity entity : old)
        scene.destroy(entity); // with the children: the trunk and the leaves of a tree

    spawnProps(scene, props, terrain, playerStart, *cubeMesh, *enemyMesh);
}

void Game::update(Scene& scene, Camera& camera, const Input& input, Audio& audio, float dt) {
    // The player can be deleted from the editor UI.
    if (!scene.registry.valid(player) || !scene.registry.all_of<Player, Health>(player))
        return;

    updateWorld(scene);

    // Stopped when the run is over, and paused while a level-up waits for a choice.
    if (dead || won || pendingLevelUps > 0)
        return;

    elapsed += dt;

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
        audio.play("jump.wav", 0.5f);
        p.grounded = false;
    }

    pushOutOfObstacles(scene.registry, transform.position, kPlayerRadius);
    transform.position.x = std::clamp(transform.position.x, -kArenaHalfSize, kArenaHalfSize);
    transform.position.z = std::clamp(transform.position.z, -kArenaHalfSize, kArenaHalfSize);

    p.verticalVelocity -= kGravity * dt;
    transform.position.y += p.verticalVelocity * dt;

    // Land on the ground. A grounded player also stays on it when walking down a slope, instead of a small fall
    // each frame.
    const float ground = heightAt(terrain, transform.position.x, transform.position.z);

    if (transform.position.y <= ground || (p.grounded && transform.position.y - ground < 0.3f)) {
        transform.position.y = ground;
        p.verticalVelocity = 0.0f;
        p.grounded = true;
    } else {
        p.grounded = false;
    }

    // Orbit: look at a point above the feet from `cameraDistance` behind it, never below the ground.
    camera.yaw = cameraYaw;
    camera.pitch = cameraPitch;
    const glm::vec3 target = transform.position + glm::vec3(0.0f, 1.0f, 0.0f);
    camera.position = target - camera.front() * cameraDistance;
    camera.position.y = std::max(camera.position.y, heightAt(terrain, camera.position.x, camera.position.z) + 0.2f);

    const float healthBefore = scene.registry.get<Health>(player).current;
    updateEnemies(scene, dt);

    // Contact damage is a small amount each frame, so the sound has a long gap.
    if (scene.registry.get<Health>(player).current < healthBefore)
        audio.play("player_hurt.wav", 0.5f, 0.4f);

    if (updateWeapon(scene, player, *enemyMesh, projectileMaterial, dt))
        audio.play("shoot.wav", 0.5f);

    // Before the projectiles: their death pass also removes the enemies that the blades kill.
    const int bladeHits = updateOrbit(scene, player, *cubeMesh, bladeMaterial, dt);
    const ProjectileResult shots = updateProjectiles(scene, dt);

    if (shots.hits + bladeHits > 0)
        audio.play("hit.wav", 0.5f, 0.05f);
    if (!shots.deaths.empty())
        audio.play("enemy_death.wav", 0.5f, 0.05f);

    // A hit enemy is white for a short time, then it gets its own color again.
    for (auto [entity, enemy, renderer] : scene.registry.view<Enemy, MeshRenderer>().each()) {
        enemy.flash = std::max(0.0f, enemy.flash - dt);
        renderer.material = enemy.flash > 0.0f ? flashMaterial : (entity == boss ? bossMaterial : enemyMaterial);
    }

    // Each dead enemy that gives XP drops an orb on the floor where it died.
    for (const Death& death : shots.deaths) {
        kills++;

        if (death.xp > 0.0f)
            spawnOrb(scene, *enemyMesh, orbMaterial, {death.position.x, heightAt(terrain, death.position.x, death.position.z) + 0.2f, death.position.z}, death.xp);
    }

    // The run is won when the boss is dead. Its entity is then not valid.
    if (bossSpawned && !scene.registry.valid(boss)) {
        boss = entt::null;
        won = true;
    }

    const OrbResult orbs = updateOrbs(scene, player, kPlayerRadius, dt);

    if (orbs.taken > 0)
        audio.play("xp_pickup.wav", 0.5f, 0.05f);

    if (orbs.levels > 0) {
        audio.play("level_up.wav", 0.5f);

        if (pendingLevelUps == 0)
            rollChoices();
    }

    pendingLevelUps += orbs.levels;
}

void Game::updateEnemies(Scene& scene, float dt) {
    entt::registry& registry = scene.registry;
    const glm::vec3 playerPos = registry.get<Transform>(player).position;
    Health& health = registry.get<Health>(player);

    // Creates an enemy at a random position on the spawner ring around the player.
    auto spawn = [&](const Spawner& spawner, const char* name, float scale, const Enemy& stats, float health,
                     const Material& material) {
        const float angle = std::uniform_real_distribution<float>(0.0f, 6.2831853f)(rng);
        glm::vec3 position = playerPos + glm::vec3(std::cos(angle), 0.0f, std::sin(angle)) * spawner.ringRadius;
        position.x = std::clamp(position.x, -kArenaHalfSize, kArenaHalfSize);
        position.z = std::clamp(position.z, -kArenaHalfSize, kArenaHalfSize);
        position.y = heightAt(terrain, position.x, position.z) + 0.5f * scale; // sphere radius is 0.5

        const entt::entity enemy = scene.create(name, {.position = position, .scale = glm::vec3(scale)});
        registry.emplace<Enemy>(enemy, stats);
        registry.emplace<Health>(enemy, Health{health, health});
        registry.emplace<Transient>(enemy); // spawned while playing, not part of the level file
        registry.emplace<MeshRenderer>(enemy, enemyMesh, material);
        return enemy;
    };

    for (auto [entity, spawner] : registry.view<Spawner>().each()) {
        // The boss comes once. The normal spawns continue.
        if (!bossSpawned && elapsed >= spawner.bossTime) {
            bossSpawned = true;
            boss = spawn(spawner, "Boss", 3.0f, Enemy{.speed = 2.5f, .radius = 1.2f, .damage = 40.0f, .xpValue = 0.0f},
                         spawner.bossHealth, bossMaterial);
        }

        const float difficulty = 1.0f + spawner.ramp * elapsed / 60.0f;
        spawner.timer += dt;

        if (spawner.timer < std::max(spawner.minInterval, spawner.interval / difficulty))
            continue;

        spawner.timer = 0.0f;

        if (static_cast<int>(registry.view<Enemy>().size()) >= spawner.maxEnemies)
            continue;

        const Enemy stats{.speed = std::min(spawner.maxEnemySpeed, spawner.enemySpeed * std::sqrt(difficulty)),
                          .damage = spawner.enemyDamage};
        spawn(spawner, "Enemy", kEnemyScale, stats, spawner.enemyHealth * difficulty, enemyMaterial);
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

    // The enemies stay on the ground. The sphere mesh has a radius of 0.5, so the center is half the scale above it.
    for (auto [entity, transform, enemy] : registry.view<Transform, Enemy>().each())
        transform.position.y = heightAt(terrain, transform.position.x, transform.position.z) + 0.5f * transform.scale.y;

    // An enemy that touches the player removes health. The collision step moves it out to exactly the sum of
    // the two radii, so the test adds a small margin.
    for (auto [entity, transform, enemy] : registry.view<Transform, Enemy>().each()) {
        glm::vec3 toPlayer = playerPos - transform.position;
        toPlayer.y = 0.0f;

        if (glm::length(toPlayer) < enemy.radius + kPlayerRadius + 0.1f)
            health.current -= enemy.damage * dt;
    }

    // The run ends at 0 health. GameApp shows the game over screen.
    if (health.current <= 0.0f) {
        health.current = 0.0f;
        dead = true;
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
