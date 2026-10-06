#pragma once

#include "assets/assets.h"
#include "core/camera.h"
#include "core/input.h"
#include "renderer/material.h"
#include "scene/component_registry.h"
#include "scene/scene.h"
#include "game/upgrades.h"

#include <entt/entt.hpp>

#include <array>
#include <random>

struct Player {
    float speed = 5.0f;     // units per second
    float jumpSpeed = 7.0f; // upward velocity at takeoff
    float verticalVelocity = 0.0f;
    bool grounded = true;
};

struct Health {
    float current = 100.0f;
    float max = 100.0f;
};

// Walks straight at the player and hurts them while touching.
struct Enemy {
    float speed = 3.0f;
    float radius = 0.4f;
    float damage = 5.0f; // per second of contact
    float xpValue = 1.0f; // XP in the orb that this enemy drops
};

// Spawns enemies on a ring around the player.
struct Spawner {
    float interval = 1.0f; // seconds between spawns
    float timer = 0.0f;
    int maxEnemies = 100;
    float ringRadius = 16.0f;
    float enemyHealth = 20.0f; // health of each new enemy
};

struct Game {
    Game(Scene& scene, Assets& assets); // for a scene loaded from level.json: finds the player entity

    void update(Scene& scene, Camera& camera, const Input& input, float dt);

    entt::entity player;
    int kills = 0;
    float elapsed = 0.0f; // seconds survived
    bool dead = false;    // health reached 0: the run is over

    // Level-ups that wait for a choice. While one waits, the game is paused.
    int pendingLevelUps = 0;
    std::array<Upgrade, 3> choices{};

    // Applies one of the 3 choices and closes that level-up.
    void choose(Scene& scene, int index);

    // Orbit camera, in the Camera's conventions (yaw -90 looks down -Z, negative pitch looks down).
    float cameraYaw = -90.0f;
    float cameraPitch = -20.0f;
    float cameraDistance = 6.0f;

private:
    void updateEnemies(Scene& scene, float dt);
    void rollChoices(); // 3 different random upgrades

    const Mesh* enemyMesh = nullptr;
    std::mt19937 rng{1};
    Material enemyMaterial{nullptr, nullptr, glm::vec3(0.6f, 0.02f, 0.02f), glm::vec3(0.3f), 32.0f}; // linear red plastic
    Material projectileMaterial = plastic({1.0f, 0.85f, 0.2f}, 64.0f);
    Material orbMaterial = plastic({0.3f, 0.7f, 1.0f}, 64.0f);
};

// Registers the game's components (and the demo's Rotator) with the inspector.
void registerGameComponents(ComponentRegistry& components);
