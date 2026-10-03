#include "game/demo.h"

void updateDemo(Scene& scene, const Camera& camera, float dt) {
    for (auto [entity, rotator, transform] : scene.registry.view<Rotator, Transform>().each()) {
        if (rotator.enabled)
            transform.rotation = glm::mod(transform.rotation + rotator.degreesPerSecond * dt, glm::vec3(360.0f));
    }

    for (auto [entity, transform] : scene.registry.view<Transform, FollowCamera>().each()) {
        transform.position = camera.position;
        transform.rotation = aimRotation(camera.front());
    }
}
