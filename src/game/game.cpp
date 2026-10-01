#include "game/game.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

static constexpr float kGroundHeight = -0.5f; // top of the floor
static constexpr float kGravity = 20.0f;      // stronger than real gravity: snappier jumps

Game::Game(Scene& scene) {
    // "Shiba" by zixisun02, CC-BY-4.0 (see models/shiba/license.txt).
    player = scene.create("Player", {.position = {0.0f, kGroundHeight, 5.0f}});
    scene.registry.emplace<Player>(player);

    // The model faces +Z but entities face -Z, so it's turned around on a child.
    const entt::entity model = scene.create("Shiba", {.rotation = {0.0f, 180.0f, 0.0f}, .scale = glm::vec3(scene.shiba.fitScale(1.2f)), .parent = player});
    scene.registry.emplace<ModelRenderer>(model, &scene.shiba);
}

void Game::update(Scene& scene, Camera& camera, const Input& input, float dt) {
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

    // Orbit: look at a point above the feet from `cameraDistance` behind it, never below the floor.
    camera.yaw = cameraYaw;
    camera.pitch = cameraPitch;
    const glm::vec3 target = transform.position + glm::vec3(0.0f, 1.0f, 0.0f);
    camera.position = target - camera.front() * cameraDistance;
    camera.position.y = std::max(camera.position.y, kGroundHeight + 0.2f);
}
