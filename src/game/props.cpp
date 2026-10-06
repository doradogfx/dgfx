#include "game/props.h"

#include "game/collision.h"

#include <algorithm>
#include <random>
#include <vector>

void spawnProps(Scene& scene, const Props& props, const Terrain& terrain, glm::vec3 playerStart, const Mesh& cube,
                const Mesh& sphere) {
    entt::registry& registry = scene.registry;
    std::mt19937 rng(static_cast<unsigned>(terrain.seed)); // the same seed gives the same layout
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    const float half = 0.5f * terrain.size - 2.0f; // a margin from the edge

    struct Placed {
        glm::vec2 position;
        float radius;
    };

    std::vector<Placed> placed;

    // Finds a random spot away from the player start and from the other props. Returns false when it finds none.
    auto findSpot = [&](float radius, glm::vec2& spot) {
        for (int attempt = 0; attempt < 30; attempt++) {
            const glm::vec2 p(half * (2.0f * unit(rng) - 1.0f), half * (2.0f * unit(rng) - 1.0f));

            if (glm::distance(p, glm::vec2(playerStart.x, playerStart.z)) < props.clearRadius + radius)
                continue;

            const bool free = std::none_of(placed.begin(), placed.end(), [&](const Placed& other) {
                return glm::distance(p, other.position) < other.radius + radius + 1.0f;
            });

            if (free) {
                placed.push_back({p, radius});
                spot = p;
                return true;
            }
        }

        return false;
    };

    const Material rockMaterial = plastic({0.45f, 0.45f, 0.43f}, 16.0f);
    const Material trunkMaterial = plastic({0.4f, 0.26f, 0.13f}, 8.0f);
    const Material leavesMaterial = plastic({0.2f, 0.45f, 0.15f}, 8.0f);

    // Rock: a flat sphere, partly in the ground. The circle is a bit smaller than the widest side.
    for (int i = 0; i < props.rocks; i++) {
        const float size = 1.0f + 1.2f * unit(rng);
        const glm::vec3 scale(size, 0.6f * size, size * (0.7f + 0.5f * unit(rng)));
        const float radius = 0.45f * std::max(scale.x, scale.z);
        glm::vec2 spot;

        if (!findSpot(radius, spot))
            continue;

        const float ground = heightAt(terrain, spot.x, spot.y);
        const entt::entity rock = scene.create("Rock", {.position = {spot.x, ground + 0.2f * scale.y, spot.y},
                                                        .rotation = {0.0f, 360.0f * unit(rng), 0.0f},
                                                        .scale = scale});
        registry.emplace<MeshRenderer>(rock, &sphere, rockMaterial);
        registry.emplace<Obstacle>(rock, radius, ground + 0.7f * scale.y); // the sphere top is 0.5 * scale above the center
        registry.emplace<Transient>(rock);
    }

    // Tree: a root entity with no scale, so the trunk scale does not change the leaves. Only the trunk blocks.
    for (int i = 0; i < props.trees; i++) {
        glm::vec2 spot;

        if (!findSpot(0.35f, spot))
            continue;

        const float ground = heightAt(terrain, spot.x, spot.y);
        const entt::entity tree = scene.create("Tree", {.position = {spot.x, ground, spot.y},
                                                        .rotation = {0.0f, 360.0f * unit(rng), 0.0f}});
        registry.emplace<Obstacle>(tree, 0.35f, ground + 3.1f);
        registry.emplace<Transient>(tree);

        const entt::entity trunk = scene.create("Trunk", {.position = {0.0f, 0.8f, 0.0f}, .scale = {0.3f, 1.6f, 0.3f}, .parent = tree});
        registry.emplace<MeshRenderer>(trunk, &cube, trunkMaterial);
        registry.emplace<Transient>(trunk);

        const entt::entity leaves = scene.create("Leaves", {.position = {0.0f, 2.2f, 0.0f}, .scale = glm::vec3(1.8f), .parent = tree});
        registry.emplace<MeshRenderer>(leaves, &sphere, leavesMaterial);
        registry.emplace<Transient>(leaves);
    }
}
