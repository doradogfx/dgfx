#pragma once

#include "assets/assets.h"
#include "core/camera.h"
#include "core/input.h"
#include "renderer/material.h"
#include "scene/component_registry.h"
#include "scene/scene.h"

#include <entt/entt.hpp>

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
};

// Spawns enemies on a ring around the player.
struct Spawner {
    float interval = 1.0f; // seconds between spawns
    float timer = 0.0f;
    int maxEnemies = 100;
    float ringRadius = 16.0f;
};

struct Game {
    Game(Scene& scene, Assets& assets); // for a scene loaded from level.json: finds the player entity

    void update(Scene& scene, Camera& camera, const Input& input, float dt);

    entt::entity player;

    // Orbit camera, in the Camera's conventions (yaw -90 looks down -Z, negative pitch looks down).
    float cameraYaw = -90.0f;
    float cameraPitch = -20.0f;
    float cameraDistance = 6.0f;

private:
    void updateEnemies(Scene& scene, float dt);

    const Mesh* enemyMesh = nullptr;
    std::mt19937 rng{1};
    Material enemyMaterial{nullptr, nullptr, glm::vec3(0.6f, 0.02f, 0.02f), glm::vec3(0.3f), 32.0f}; // linear red plastic
};

// Registers the game's components (and the demo's Rotator) with the inspector.
void registerGameComponents(ComponentRegistry& components);
