#pragma once

#include "core/camera.h"
#include "scene/scene.h"

// Spins the entity's Transform continuously.
struct Rotator {
    glm::vec3 degreesPerSecond{0.0f};
    bool enabled = true;
};

// Tag: the entity's transform is set to the camera's every frame (e.g. a flashlight). Root entities only:
// it writes the local transform, so under a parent it would be offset by the parent's.
struct FollowCamera {};

// Moves the Rotator and FollowCamera entities of scenes/demo.json.
void updateDemo(Scene& scene, const Camera& camera, float dt);
