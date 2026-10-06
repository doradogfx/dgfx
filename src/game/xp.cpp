#include "game/xp.h"

#include <algorithm>
#include <vector>

static constexpr float kOrbScale = 0.25f;
static constexpr float kOrbRadius = 0.5f * kOrbScale; // the sphere mesh has a radius of 0.5

void spawnOrb(Scene& scene, const Mesh& mesh, const Material& material, glm::vec3 position, float value) {
    entt::registry& registry = scene.registry;

    const entt::entity orb = scene.create("XP orb", {.position = position, .scale = glm::vec3(kOrbScale)});
    registry.emplace<XpOrb>(orb, XpOrb{value});
    registry.emplace<Transient>(orb); // Made while playing. The scene file does not save it.
    registry.emplace<MeshRenderer>(orb, &mesh, material);
}

int updateOrbs(Scene& scene, entt::entity player, float playerRadius, float dt) {
    entt::registry& registry = scene.registry;
    Experience* experience = registry.try_get<Experience>(player);

    if (!experience)
        return 0;

    const glm::vec3 playerPosition = registry.get<Transform>(player).position;

    // ponytail: an orb stays until the player takes it. Many orbs cost one draw call each. Add a cap or merge them.
    std::vector<entt::entity> taken; // Destroy them after the loop, because destroying inside a loop breaks the loop.

    for (auto [entity, transform, orb] : registry.view<Transform, XpOrb>().each()) {
        // In 3D, so the orb also goes up or down a hill to the player.
        const glm::vec3 toPlayer = playerPosition - transform.position;
        const float distance = glm::length(toPlayer);

        if (distance < playerRadius + kOrbRadius) {
            experience->xp += orb.value;
            taken.push_back(entity);
        } else if (distance < experience->pickupRadius) {
            transform.position += toPlayer / distance * std::min(experience->pickupSpeed * dt, distance);
        }
    }

    registry.destroy(taken.begin(), taken.end());

    int levels = 0;

    while (experience->xp >= xpToNext(*experience)) {
        experience->xp -= xpToNext(*experience);
        experience->level++;
        levels++;
    }

    return levels;
}
