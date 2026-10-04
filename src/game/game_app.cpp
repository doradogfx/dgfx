#include "game/game_app.h"

#include "game/demo.h"
#include "game/xp.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

GameApp::GameApp() {
    registerGameComponents(components);
    openScene("level.json");
}

void GameApp::openScene(const std::string& file) {
    loadScene(file);
    game.reset();
    flyMode = false;

    // The starting view for scenes without a player to follow.
    camera = Camera();
    camera.position = glm::vec3(0.0f, 2.0f, 6.0f);
    camera.pitch = -15.0f;

    if (!scene.registry.view<Player>().empty())
        game.emplace(scene, assets);
}

void GameApp::update(float dt) {
    if (input.pressed(GLFW_KEY_F1))
        flyMode = !flyMode;

    if (flyMode || !game)
        flyCamera(camera, input, dt);
    else
        game->update(scene, camera, input, dt);

    if (!game)
        updateDemo(scene, camera, dt);
}

void GameApp::statsOverlay() {
    ImGui::Text("%zu enemies", static_cast<size_t>(scene.registry.view<const Enemy>().size()));

    if (game)
        ImGui::Text("%d kills", game->kills);

    for (auto [entity, experience, player] : scene.registry.view<const Experience, const Player>().each())
        ImGui::Text("Level %d (%.0f / %.0f XP)", experience.level, experience.xp, xpToNext(experience));

    for (auto [entity, health, player] : scene.registry.view<const Health, const Player>().each())
        ImGui::Text("Health %.0f / %.0f", health.current, health.max);

    ImGui::TextDisabled("F1: fly camera");
}
