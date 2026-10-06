#include "game/demo.h"
#include "game/game.h"
#include "game/props.h"
#include "game/terrain.h"
#include "game/weapons.h"
#include "game/xp.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

// Authored settings only: runtime state (velocity, the spawn timer) is not saved.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Rotator, degreesPerSecond, enabled)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Player, speed, jumpSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Health, current, max)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Enemy, speed, radius, damage, xpValue)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Experience, base, step, pickupRadius, pickupSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Spawner, interval, minInterval, maxEnemies, ringRadius, ramp, enemyHealth,
                                                enemySpeed, maxEnemySpeed, enemyDamage, bossTime, bossHealth)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Weapon, damage, interval, range, projectileSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Props, rocks, trees, clearRadius)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Terrain,size, resolution, base, amplitude, frequency, octaves, seed, color)

// A tag has nothing to save.
static void to_json(Json& json, const FollowCamera&) {
    json = Json::object();
}

static void from_json(const Json&, FollowCamera&) {}

void registerGameComponents(ComponentRegistry& components) {
    components.add<Rotator>("Rotator", [](Rotator& rotator) {
        ImGui::Checkbox("Enabled", &rotator.enabled);
        ImGui::DragFloat3("Speed", glm::value_ptr(rotator.degreesPerSecond), 1.0f, 0.0f, 0.0f, "%.1f deg/s");
    });

    components.add<FollowCamera>("Follow camera", [](FollowCamera&) {
        ImGui::TextDisabled("Follows the camera");
    });

    components.add<Player>("Player", [](Player& player) {
        ImGui::DragFloat("Speed", &player.speed, 0.1f, 0.0f, 50.0f, "%.1f u/s");
        ImGui::DragFloat("Jump speed", &player.jumpSpeed, 0.1f, 0.0f, 50.0f, "%.1f u/s");
    });

    components.add<Weapon>("Weapon", [](Weapon& weapon) {
        ImGui::DragFloat("Damage", &weapon.damage, 0.5f, 0.0f, 1000.0f);
        ImGui::DragFloat("Interval", &weapon.interval, 0.01f, 0.05f, 10.0f, "%.2f s");
        ImGui::DragFloat("Range", &weapon.range, 0.1f, 1.0f, 100.0f);
        ImGui::DragFloat("Projectile speed", &weapon.projectileSpeed, 0.1f, 1.0f, 100.0f, "%.1f u/s");
    });

    components.add<Experience>("Experience", [](Experience& experience) {
        ImGui::TextDisabled("Level %d, %.0f / %.0f XP", experience.level, experience.xp, xpToNext(experience));
        ImGui::DragFloat("First level XP", &experience.base, 0.1f, 1.0f, 1000.0f);
        ImGui::DragFloat("More XP each level", &experience.step, 0.1f, 0.0f, 1000.0f);
        ImGui::DragFloat("Pickup radius", &experience.pickupRadius, 0.1f, 0.0f, 50.0f);
        ImGui::DragFloat("Pickup speed", &experience.pickupSpeed, 0.1f, 0.0f, 100.0f, "%.1f u/s");
    });

    components.add<Health>("Health", [](Health& health) {
        ImGui::DragFloat("Current", &health.current, 1.0f, 0.0f, health.max);
        ImGui::DragFloat("Max", &health.max, 1.0f, 1.0f, 10000.0f);
    });

    components.add<Enemy>("Enemy", [](Enemy& enemy) {
        ImGui::DragFloat("Speed", &enemy.speed, 0.1f, 0.0f, 50.0f, "%.1f u/s");
        ImGui::DragFloat("Radius", &enemy.radius, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Damage", &enemy.damage, 0.5f, 0.0f, 1000.0f, "%.1f /s");
        ImGui::DragFloat("XP value", &enemy.xpValue, 0.1f, 0.0f, 1000.0f);
    });

    components.add<Spawner>("Spawner", [](Spawner& spawner) {
        ImGui::DragFloat("Interval", &spawner.interval, 0.05f, 0.05f, 60.0f, "%.2f s");
        ImGui::DragFloat("Min interval", &spawner.minInterval, 0.01f, 0.01f, 60.0f, "%.2f s");
        ImGui::DragInt("Max enemies", &spawner.maxEnemies, 1.0f, 0, 2000);
        ImGui::DragFloat("Ring radius", &spawner.ringRadius, 0.1f, 1.0f, 40.0f);
        ImGui::DragFloat("Ramp", &spawner.ramp, 0.01f, 0.0f, 10.0f, "%.2f /min");
        ImGui::DragFloat("Enemy health", &spawner.enemyHealth, 1.0f, 1.0f, 10000.0f);
        ImGui::DragFloat("Enemy speed", &spawner.enemySpeed, 0.1f, 0.0f, 50.0f, "%.1f u/s");
        ImGui::DragFloat("Max enemy speed", &spawner.maxEnemySpeed, 0.1f, 0.0f, 50.0f, "%.1f u/s");
        ImGui::DragFloat("Enemy damage", &spawner.enemyDamage, 0.5f, 0.0f, 1000.0f, "%.1f /s");
        ImGui::DragFloat("Boss time", &spawner.bossTime, 1.0f, 0.0f, 3600.0f, "%.0f s");
        ImGui::DragFloat("Boss health", &spawner.bossHealth, 10.0f, 1.0f, 100000.0f);
    });

    components.add<Terrain>("Terrain", [](Terrain& terrain) {
        ImGui::DragFloat("Size", &terrain.size, 0.5f, 1.0f, 500.0f);
        ImGui::DragInt("Resolution", &terrain.resolution, 1.0f, 1, 512);
        ImGui::DragFloat("Base", &terrain.base, 0.05f);
        ImGui::DragFloat("Amplitude", &terrain.amplitude, 0.05f, 0.0f, 50.0f);
        ImGui::DragFloat("Frequency", &terrain.frequency, 0.001f, 0.001f, 1.0f, "%.3f");
        ImGui::DragInt("Octaves", &terrain.octaves, 0.1f, 1, 8);
        ImGui::DragInt("Seed", &terrain.seed);
        ImGui::ColorEdit3("Color", glm::value_ptr(terrain.color));
    });

    components.add<Props>("Props", [](Props& props) {
        ImGui::DragInt("Rocks", &props.rocks, 0.2f, 0, 500);
        ImGui::DragInt("Trees", &props.trees, 0.2f, 0, 500);
        ImGui::DragFloat("Clear radius", &props.clearRadius, 0.1f, 0.0f, 50.0f);
    });
}
