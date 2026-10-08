#include "scene/scene.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <vector>

// Inverts the transform's yaw-then-pitch rotation of (0, 0, -1), which gives (-cos p sin y, sin p, -cos p cos y).
glm::vec3 aimRotation(glm::vec3 direction) {
    const glm::vec3 d = glm::normalize(direction);
    return {glm::degrees(std::asin(d.y)), glm::degrees(std::atan2(-d.x, -d.z)), 0.0f};
}

entt::entity Scene::create(const std::string& name, const Transform& transform) {
    const entt::entity entity = registry.create();
    registry.emplace<Name>(entity, name);
    registry.emplace<Transform>(entity, transform);
    return entity;
}

glm::mat4 Scene::worldMatrix(entt::entity entity) const {
    const Transform& t = registry.get<Transform>(entity);

    // Right to left: scale, then rotate (roll, pitch, yaw), then move into place.
    glm::mat4 local = glm::translate(glm::mat4(1.0f), t.position);
    local = glm::rotate(local, glm::radians(t.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    local = glm::rotate(local, glm::radians(t.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    local = glm::rotate(local, glm::radians(t.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    local = glm::scale(local, t.scale);

    // ponytail: walks the parent chain on every call, cache world matrices with dirty flags if scenes get large.
    return t.parent == entt::null ? local : worldMatrix(t.parent) * local;
}

glm::vec3 Scene::position(entt::entity entity) const {
    return glm::vec3(worldMatrix(entity)[3]); // translation column
}

glm::vec3 Scene::forward(entt::entity entity) const {
    return glm::normalize(glm::mat3(worldMatrix(entity)) * glm::vec3(0.0f, 0.0f, -1.0f));
}

void Scene::destroy(entt::entity entity) {
    // Collected first: destroying while walking the registry would invalidate the walk.
    std::vector<entt::entity> doomed = {entity};

    for (size_t i = 0; i < doomed.size(); i++) {
        for (auto [child, transform] : registry.view<const Transform>().each()) {
            if (transform.parent == doomed[i])
                doomed.push_back(child);
        }
    }

    registry.destroy(doomed.begin(), doomed.end());
}

entt::entity Scene::camera() const {
    auto view = registry.view<const Camera>();
    return view.empty() ? entt::null : view.front();
}
