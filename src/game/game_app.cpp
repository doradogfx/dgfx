#include "game/game_app.h"

#include "game/demo.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

GameApp::GameApp() {
    registerGameComponents(components);
    loadScene("level.json");
    game.emplace(scene, assets);
}

void GameApp::update(float dt) {
    if (input.pressed(GLFW_KEY_F1))
        flyMode = !flyMode;

    if (input.pressed(GLFW_KEY_F2)) {
        camera = Camera();

        if (game) {
            game.reset();
            loadScene("demo.json");
            camera.position = glm::vec3(0.0f, 2.0f, 6.0f);
            camera.pitch = -15.0f;
        } else {
            loadScene("level.json");
            game.emplace(scene, assets);
        }
    }

    if (flyMode || !game)
        flyCamera(camera, input, dt);
    else
        game->update(scene, camera, input, dt);

    if (!game)
        updateDemo(scene, camera, dt);
}

void GameApp::statsOverlay() {
    ImGui::Text("%zu enemies", static_cast<size_t>(scene.registry.view<const Enemy>().size()));

    for (auto [entity, health, player] : scene.registry.view<const Health, const Player>().each())
        ImGui::Text("Health %.0f / %.0f", health.current, health.max);

    ImGui::TextDisabled("F1: fly camera | F2: switch scene");
}
