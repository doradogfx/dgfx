#pragma once

#include "core/camera.h"
#include "core/input.h"
#include "scene/scene.h"

#include <entt/entt.hpp>

struct Player {
    float speed = 5.0f;     // units per second
    float jumpSpeed = 7.0f; // upward velocity at takeoff
    float verticalVelocity = 0.0f;
    bool grounded = true;
};

// The gameplay: the player and the third-person camera that orbits it.
struct Game {
    explicit Game(Scene& scene); // spawns the player

    void update(Scene& scene, Camera& camera, const Input& input, float dt);

    entt::entity player;

    // Orbit camera, in the Camera's conventions (yaw -90 looks down -Z, negative pitch looks down).
    float cameraYaw = -90.0f;
    float cameraPitch = -20.0f;
    float cameraDistance = 6.0f;
};
