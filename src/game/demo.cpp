#include "game/demo.h"

void updateDemo(Scene& scene, float dt) {
    for (auto [entity, rotator, transform] : scene.registry.view<Rotator, Transform>().each()) {
        if (rotator.enabled)
            transform.rotation = glm::mod(transform.rotation + rotator.degreesPerSecond * dt, glm::vec3(360.0f));
    }

    const entt::entity camera = scene.camera();

    if (camera == entt::null)
        return;

    const glm::vec3 position = scene.position(camera);
    const glm::vec3 forward = scene.forward(camera);

    for (auto [entity, transform] : scene.registry.view<Transform, FollowCamera>().each()) {
        transform.position = position;
        transform.rotation = aimRotation(forward);
    }
}
