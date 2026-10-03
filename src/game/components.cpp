#include "game/demo.h"
#include "game/game.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

void registerGameComponents(ComponentRegistry& components) {
    components.add<Rotator>("Rotator", [](Rotator& rotator) {
        ImGui::Checkbox("Enabled", &rotator.enabled);
        ImGui::DragFloat3("Speed", glm::value_ptr(rotator.degreesPerSecond), 1.0f, 0.0f, 0.0f, "%.1f deg/s");
    });

    components.add<Player>("Player", [](Player& player) {
        ImGui::DragFloat("Speed", &player.speed, 0.1f, 0.0f, 50.0f, "%.1f u/s");
        ImGui::DragFloat("Jump speed", &player.jumpSpeed, 0.1f, 0.0f, 50.0f, "%.1f u/s");
    });

    components.add<Health>("Health", [](Health& health) {
        ImGui::DragFloat("Current", &health.current, 1.0f, 0.0f, health.max);
        ImGui::DragFloat("Max", &health.max, 1.0f, 1.0f, 10000.0f);
    });

    components.add<Enemy>("Enemy", [](Enemy& enemy) {
        ImGui::DragFloat("Speed", &enemy.speed, 0.1f, 0.0f, 50.0f, "%.1f u/s");
        ImGui::DragFloat("Radius", &enemy.radius, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Damage", &enemy.damage, 0.5f, 0.0f, 1000.0f, "%.1f /s");
    });

    components.add<Spawner>("Spawner", [](Spawner& spawner) {
        ImGui::DragFloat("Interval", &spawner.interval, 0.05f, 0.05f, 60.0f, "%.2f s");
        ImGui::DragInt("Max enemies", &spawner.maxEnemies, 1.0f, 0, 2000);
        ImGui::DragFloat("Ring radius", &spawner.ringRadius, 0.1f, 1.0f, 40.0f);
    });
}
